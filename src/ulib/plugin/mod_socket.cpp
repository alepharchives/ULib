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
#include <ulib/plugin/mod_socket.h>
#include <ulib/net/server/server.h>

U_CREAT_FUNC(UWebSocketPlugIn)

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

   (void) U_SYSCALL(memcpy, "%p,%p,%u", part, &division, sizeof(uint32_t));

   /*
   part[0] = (char)( division        >> 24);
   part[1] = (char)((division <<  8) >> 24);
   part[2] = (char)((division << 16) >> 24);
   part[3] = (char)((division << 24) >> 24);
   */
}

void UWebSocketPlugIn::dataFraming()
{
   U_TRACE(0, "UWebSocketPlugIn::dataFraming()")

   UString buffer(U_CAPACITY);
   fd_set fd_set_read, read_set;
   int n, sock = UClientImage_Base::socket->getFd(), fdmax = U_max(sock, UProcess::filedes[2]) + 1;

   FD_ZERO(                     &fd_set_read);
   FD_SET(sock,                 &fd_set_read);
   FD_SET(UProcess::filedes[2], &fd_set_read);

   if (UClientImage_Base::isPipeline())
      {
      (void) buffer.append(UClientImage_Base::rbuffer->data(), USocketExt::pcount);

      goto start;
      }

loop:
   read_set = fd_set_read;

   n = U_SYSCALL(select, "%d,%p,%p,%p,%p", fdmax, &read_set, 0, 0, 0);

   if (n > 0)
      {
      if (FD_ISSET(sock, &read_set))
         {
         buffer.setEmpty();

         if (UServices::read(sock, buffer))
            {
start:      if (UNotifier::write(UProcess::filedes[1], U_STRING_TO_PARAM(buffer))) goto loop;
            }
         }
      else if (FD_ISSET(UProcess::filedes[2], &read_set))
         {
         buffer.setEmpty();

         if (UServices::read(UProcess::filedes[2], buffer))
            {
            if (UNotifier::write(sock, U_STRING_TO_PARAM(buffer))) goto loop;
            }
         }
      }
}

// Server-wide hooks

int UWebSocketPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UWebSocketPlugIn::handlerConfig(%p)", &cfg)

   // -----------------------------------------------
   // Perform registration of userver method
   // -----------------------------------------------
   // COMMAND                      command to execute
   // ENVIRONMENT  environment for command to execute
   // -----------------------------------------------

   if (cfg.loadTable()) command = UServer_Base::loadConfigCommand(cfg);

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UWebSocketPlugIn::handlerInit()
{
   U_TRACE(0, "UWebSocketPlugIn::handlerInit()")

   if (command)
      {
      U_SRV_LOG_MSG("initialization of plugin success");

      U_RETURN(U_PLUGIN_HANDLER_GO_ON);
      }

   U_SRV_LOG_MSG("initialization of plugin FAILED");

   U_RETURN(U_PLUGIN_HANDLER_ERROR);
}

// Connection-wide hooks

int UWebSocketPlugIn::handlerRequest()
{
   U_TRACE(0, "UWebSocketPlugIn::handlerRequest()")

   U_INTERNAL_DUMP("method = %.*S uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_HTTP_URI_TO_TRACE)

   if (UHTTP::http_info.upgrade == 0) U_RETURN(U_PLUGIN_HANDLER_GO_ON);

   // process handshake

   const char* key1   = UHTTP::getHTTPHeaderValuePtr(*UHTTP::str_websocket_key1);
   const char* key2   = UHTTP::getHTTPHeaderValuePtr(*UHTTP::str_websocket_key2);
   const char* origin = UHTTP::getHTTPHeaderValuePtr(*UHTTP::str_origin);

   if (key1   == 0 ||
       key2   == 0 ||
       origin == 0)
      {
      U_RETURN(U_PLUGIN_HANDLER_ERROR);
      }

   char c;
   uint32_t origin_len = 0;

   for (c = u_line_terminator[0]; origin[origin_len] != c; ++origin_len) {}

   U_INTERNAL_DUMP("origin = %.*S", origin_len, origin)

   // big-endian 128 bit string

   unsigned char challenge[16];

                          getPart(key1, challenge);
                          getPart(key2, challenge+4);
   (void) U_SYSCALL(memcpy, "%p,%p,%u", challenge+8, U_STRING_TO_PARAM(*UClientImage_Base::body));

   // MD5(challenge)

   UClientImage_Base::body->setBuffer(U_CAPACITY);

   UServices::generateDigest(U_HASH_MD5, 0, challenge, sizeof(challenge), *UClientImage_Base::body, -1);

   UString tmp(100U);
   uint32_t protocol_len = 0;
   const char* protocol  = UHTTP::getHTTPHeaderValuePtr(*UHTTP::str_websocket_prot);

   if (protocol)
      {
      for (c = u_line_terminator[0]; protocol[protocol_len] != c; ++protocol_len) {}

      U_INTERNAL_DUMP("protocol = %.*S", protocol_len, protocol)

      tmp.snprintf("%.*s: %.*s\r\n", U_STRING_TO_TRACE(*UHTTP::str_websocket_prot), protocol_len, protocol); 
      }

   UClientImage_Base::wbuffer->setBuffer(100U + origin_len + tmp.size() +
                                         UHTTP::http_info.uri_len +
                                         UHTTP::http_info.host_len +
                                         UHTTP::str_frm_websocket->size());

   UClientImage_Base::wbuffer->snprintf(UHTTP::str_frm_websocket->data(),
                                        origin_len, origin,
                                        U_HTTP_HOST_TO_TRACE, U_HTTP_URI_TO_TRACE,
                                        U_STRING_TO_TRACE(tmp));

   if (UClientImage_Base::pClientImage->handlerWrite() == U_NOTIFIER_OK)
      {
      UClientImage_Base::write_off = true;

#  ifdef DEBUG
      int fd_stderr = UFile::creat("/tmp/UWebSocketPlugIn.err", O_WRONLY | O_APPEND, PERM_FILE);
#  else
      int fd_stderr = UServices::getDevNull();
#  endif

      U_INTERNAL_ASSERT_POINTER(command)

      // Set environment for the command application server

      UClientImage_Base::body->setBuffer(U_CAPACITY);

      UString environment = command->getStringEnvironment() + *UHTTP::penvironment + UHTTP::getCGIEnvironment(command->isShellScript());

      command->setEnvironment(&environment);

      UHTTP::penvironment->setEmpty();

      if (command->execute((UString*)-1, (UString*)-1, -1, fd_stderr))
         {
         UServer_Base::logCommandMsgError(command->getCommand());

         dataFraming();

         U_RETURN(U_PLUGIN_HANDLER_FINISHED);
         }
      }

   U_RETURN(U_PLUGIN_HANDLER_ERROR);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UWebSocketPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "command (UCommand " << (void*)command << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
