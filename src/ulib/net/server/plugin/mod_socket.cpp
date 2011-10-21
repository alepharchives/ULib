// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_socket.cpp - this is a plugin web socket for userver
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/command.h>
#include <ulib/file_config.h>
#include <ulib/utility/uhttp.h>
#include <ulib/utility/services.h>
#include <ulib/net/server/server.h>
#include <ulib/net/server/plugin/mod_socket.h>

#ifdef HAVE_BYTESWAP_H
#  include <byteswap.h>
#else
#  define rotr32(x,n) (((x) >> n) | ((x) << (32 - n)))
#  define bswap_32(x) ((rotr32((x), 24) & 0x00ff00ff) | (rotr32((x), 8) & 0xff00ff00))
#  define bswap_64(x) ((((uint64_t)(bswap_32((uint32_t)(x)))) << 32) | (bswap_32((uint32_t)((x) >> 32))))
#endif

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_socket, UWebSocketPlugIn)
#endif

bool UWebSocketPlugIn::bUseSizePreamble;

const UString* UWebSocketPlugIn::str_USE_SIZE_PREAMBLE;

void UWebSocketPlugIn::str_allocate()
{
   U_TRACE(0, "UWebSocketPlugIn::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_USE_SIZE_PREAMBLE,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("USE_SIZE_PREAMBLE") }
   };

   U_NEW_ULIB_OBJECT(str_USE_SIZE_PREAMBLE, U_STRING_FROM_STRINGREP_STORAGE(0));
}

UWebSocketPlugIn::~UWebSocketPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, UWebSocketPlugIn)

   if (command) delete command;
}

void UWebSocketPlugIn::getPart(const char* key, unsigned char* part)
{
   U_TRACE(0, "UWebSocketPlugIn::getPart(%20S,%p)", key, part)

   char number[10];
   uint32_t i = 0, n = 0, space = 0;
   char c1, c2 = u_line_terminator[0];

   while ((c1 = key[i++]) != c2)
      {
           if (u_isdigit(c1)) number[n++] = c1;
      else if (u_isspace(c1)) ++space;
      }

   number[n] = '\0';

   U_INTERNAL_DUMP("number = %S", number)

   uint32_t division = atoi(number);

   if (space) division /= space;

   U_INTERNAL_DUMP("division = %u", division)

   // big-endian unsigned 32-bit integer

   division = htonl(division);

   (void) u_memcpy(part, &division, sizeof(uint32_t));
}

RETSIGTYPE UWebSocketPlugIn::handlerForSigTERM(int signo)
{
   U_TRACE(0, "[SIGTERM] UWebSocketPlugIn::handlerForSigTERM(%d)", signo)

   UInterrupt::sendOurselves(SIGTERM);
}

bool UWebSocketPlugIn::handleDataFraming(USocket* csocket)
{
   U_TRACE(0, "UWebSocketPlugIn::handleDataFraming(%p)", csocket)

   UString frame;
   uint64_t frame_length;
   fd_set fd_set_read, read_set;
   int n, sock = csocket->iSockDesc, fdmax = U_max(sock, UProcess::filedes[2]) + 1;

   FD_ZERO(                     &fd_set_read);
   FD_SET(sock,                 &fd_set_read);
   FD_SET(UProcess::filedes[2], &fd_set_read);

   if (UClientImage_Base::isPipeline())
      {
      UClientImage_Base::rbuffer->moveToBeginDataInBuffer(UClientImage_Base::size_request);

      U_INTERNAL_DUMP("UClientImage_Base::rbuffer = %#.*S", U_STRING_TO_TRACE(*UClientImage_Base::rbuffer))

      goto handle_data;
      }

loop:
   read_set = fd_set_read;

   n = U_SYSCALL(select, "%d,%p,%p,%p,%p", fdmax, &read_set, 0, 0, 0);

   if (n <= 0) goto end;

   if (FD_ISSET(sock, &read_set))
      {
      uint32_t sz;
      unsigned char type;

      UClientImage_Base::rbuffer->setEmptyForce(); // NB: can be referenced by frame...

      if (USocketExt::read(csocket, *UClientImage_Base::rbuffer) == false) goto end;

handle_data:
      sz   =                 UClientImage_Base::rbuffer->size();
      type = (unsigned char) UClientImage_Base::rbuffer->first_char();

      if (bUseSizePreamble)
         {
         // big-endian 64 bit unsigned integer

         frame_length = *((uint64_t*)UClientImage_Base::rbuffer->c_pointer(1));

#     if __BYTE_ORDER == __LITTLE_ENDIAN
         frame_length = bswap_64(frame_length);
#     endif

         if (frame_length == 0)
            {
            if (type == 0x00) goto end;

            goto loop;
            }

         U_INTERNAL_ASSERT_EQUALS(type,0xff)

         sz -= 1 + sizeof(uint64_t);

         if (frame_length > sz)
            {
            // wait max 3 sec for other data...

            if (USocketExt::read(csocket, *UClientImage_Base::rbuffer, frame_length - sz, 3 * 1000) == false) goto end;
            }

         frame = UClientImage_Base::rbuffer->substr(1 + sizeof(uint64_t), frame_length);
         }
      else
         {
         U_INTERNAL_ASSERT_EQUALS(type,0x00)
         U_ASSERT_EQUALS((unsigned char)UClientImage_Base::rbuffer->last_char(),0xff)

         frame = UClientImage_Base::rbuffer->substr(1,       // skip 0x00
                                                    sz - 2); // skip 0xff
         }

      U_SRV_LOG_WITH_ADDR("received message (%u bytes) %.*S from", frame.size(), U_STRING_TO_TRACE(frame))

      if (UNotifier::write(UProcess::filedes[1], U_STRING_TO_PARAM(frame))) goto loop;

      U_RETURN(true);
      }
   else if (FD_ISSET(UProcess::filedes[2], &read_set))
      {
      UClientImage_Base::wbuffer->setEmptyForce();

      if (UServices::read(UProcess::filedes[2], *UClientImage_Base::wbuffer) == false) goto end;

      if (bUseSizePreamble)
         {
         (void) frame.assign(1, '\0');

         // big-endian 64 bit unsigned integer

         frame_length = UClientImage_Base::wbuffer->size();

#     if __BYTE_ORDER == __LITTLE_ENDIAN
         frame_length = bswap_64(frame_length);
#     endif

         (void) frame.append((const char*)&frame_length, sizeof(uint64_t));
         (void) frame.append(*UClientImage_Base::wbuffer);
         }
      else
         {
         frame = '\0' + *UClientImage_Base::wbuffer + '\377';
         }

      U_SRV_LOG_WITH_ADDR("sent message (%u bytes) %.*S to", frame.size(), U_STRING_TO_TRACE(frame))

      if (USocketExt::write(csocket, U_STRING_TO_PARAM(frame))) goto loop;
      }

end:
   U_RETURN(false);
}

// Server-wide hooks

int UWebSocketPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UWebSocketPlugIn::handlerConfig(%p)", &cfg)

   // ---------------------------------------------------------------------------------------------
   // Perform registration of web socket method
   // ---------------------------------------------------------------------------------------------
   // COMMAND                            command to execute
   // ENVIRONMENT        environment for command to execute
   //
   // USE_SIZE_PREAMBLE  use last specification (http://www.whatwg.org/specs/web-socket-protocol/)
   // ---------------------------------------------------------------------------------------------

   if (cfg.loadTable())
      {
      command          = UServer_Base::loadConfigCommand(cfg);
      bUseSizePreamble = cfg.readBoolean(*str_USE_SIZE_PREAMBLE);
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UWebSocketPlugIn::handlerInit()
{
   U_TRACE(0, "UWebSocketPlugIn::handlerInit()")

   if (command)
      {
      U_SRV_LOG("initialization of plugin success");

      goto end;
      }

   U_SRV_LOG("initialization of plugin FAILED");

end:
   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int UWebSocketPlugIn::handlerRequest()
{
   U_TRACE(0, "UWebSocketPlugIn::handlerRequest()")

   U_INTERNAL_DUMP("method = %.*S uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_HTTP_URI_TO_TRACE)

   if (command &&
       U_http_upgrade == '1')
      {
      // process handshake

      const char* origin = UHTTP::getHTTPHeaderValuePtr(*UClientImage_Base::request, *UHTTP::str_origin, false);

      if (origin)
         {
         char c;
         UString tmp(100U);
         uint32_t origin_len = 0;

         for (c = u_line_terminator[0]; origin[origin_len] != c; ++origin_len) {}

         U_INTERNAL_DUMP("origin = %.*S", origin_len, origin)

         const char* protocol = UHTTP::getHTTPHeaderValuePtr(*UClientImage_Base::request, *UHTTP::str_websocket_prot, false);

         if (protocol)
            {
            uint32_t protocol_len = 0;

            for (c = u_line_terminator[0]; protocol[protocol_len] != c; ++protocol_len) {}

            U_INTERNAL_DUMP("protocol = %.*S", protocol_len, protocol)

            tmp.snprintf("%.*s: %.*s\r\n", U_STRING_TO_TRACE(*UHTTP::str_websocket_prot), protocol_len, protocol); 
            }

         UClientImage_Base::wbuffer->setBuffer(100U + origin_len + tmp.size() +
                                               u_http_info.uri_len +
                                               u_http_info.host_len +
                                               UHTTP::str_frm_websocket->size());

         UClientImage_Base::wbuffer->snprintf(UHTTP::str_frm_websocket->data(),
                                              origin_len, origin,
                                              U_HTTP_HOST_TO_TRACE, U_HTTP_URI_TO_TRACE,
                                              U_STRING_TO_TRACE(tmp));

         const char* key1 = UHTTP::getHTTPHeaderValuePtr(*UClientImage_Base::request, *UHTTP::str_websocket_key1, false);
         const char* key2 = UHTTP::getHTTPHeaderValuePtr(*UClientImage_Base::request, *UHTTP::str_websocket_key2, false);

         if (key1 &&
             key2)
            {
            // big-endian 128 bit string

            unsigned char challenge[16];

              getPart(key1, challenge);
              getPart(key2, challenge+4);
            (void) u_memcpy(challenge+8, UClientImage_Base::body->data(), UClientImage_Base::body->size());

            // MD5(challenge)

            UClientImage_Base::body->setBuffer(U_CAPACITY);

            UServices::generateDigest(U_HASH_MD5, 0, challenge, sizeof(challenge), *UClientImage_Base::body, -1);
            }

         if (UServer_Base::pClientImage->handlerWrite() == U_NOTIFIER_OK)
            {
            UClientImage_Base::write_off = true;

#        ifdef DEBUG
            int fd_stderr = UFile::creat("/tmp/UWebSocketPlugIn.err", O_WRONLY | O_APPEND, PERM_FILE);
#        else
            int fd_stderr = UServices::getDevNull();
#        endif

            U_INTERNAL_ASSERT_POINTER(command)

            // Set environment for the command application server

            UClientImage_Base::body->setBuffer(U_CAPACITY);

            UString environment = UHTTP::getCGIEnvironment() + command->getStringEnvironment();

            command->setEnvironment(&environment);

            if (command->execute((UString*)-1, (UString*)-1, -1, fd_stderr))
               {
               UServer_Base::logCommandMsgError(command->getCommand());

               UInterrupt::setHandlerForSignal(SIGTERM, (sighandler_t)UWebSocketPlugIn::handlerForSigTERM); // sync signal

               USocket* csocket = UServer_Base::pClientImage->socket;

               if (handleDataFraming(csocket))
                  {
                  // Send nine 0x00 bytes to the client to indicate the start of the closing handshake

                  char closing[9] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };

                  if (USocketExt::write(csocket, closing, sizeof(closing)))
                     {
                     UClientImage_Base::wbuffer->setEmpty();

                     // client terminated: receive nine 0x00 bytes

                     (void) USocketExt::read(csocket, *UClientImage_Base::rbuffer, closing, sizeof(closing));
                     }
                  }
               }
            }

         UHTTP::setHTTPRequestProcessed();
         }
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UWebSocketPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "bUseSizePreamble  " << bUseSizePreamble  << '\n'
                  << "command (UCommand " << (void*)command    << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
