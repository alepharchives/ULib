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
#include <ulib/utility/websocket.h>
#include <ulib/net/server/server.h>
#include <ulib/net/server/plugin/mod_socket.h>

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_socket, UWebSocketPlugIn)
#endif

iPFpv     UWebSocketPlugIn::on_message;
UCommand* UWebSocketPlugIn::command;

const UString* UWebSocketPlugIn::str_MAX_MESSAGE_SIZE;

void UWebSocketPlugIn::str_allocate()
{
   U_TRACE(0, "UWebSocketPlugIn::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_MAX_MESSAGE_SIZE,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("MAX_MESSAGE_SIZE") }
   };

   U_NEW_ULIB_OBJECT(str_MAX_MESSAGE_SIZE, U_STRING_FROM_STRINGREP_STORAGE(0));
}

UWebSocketPlugIn::UWebSocketPlugIn()
{
   U_TRACE_REGISTER_OBJECT(0, UWebSocketPlugIn, "")

   if (str_MAX_MESSAGE_SIZE == 0) str_allocate();
}

UWebSocketPlugIn::~UWebSocketPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, UWebSocketPlugIn)

   if (command) delete command;
}

RETSIGTYPE UWebSocketPlugIn::handlerForSigTERM(int signo)
{
   U_TRACE(0, "[SIGTERM] UWebSocketPlugIn::handlerForSigTERM(%d)", signo)

   UInterrupt::sendOurselves(SIGTERM);
}

// Server-wide hooks

int UWebSocketPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UWebSocketPlugIn::handlerConfig(%p)", &cfg)

   // ----------------------------------------------------------------------------------------------
   // Perform registration of web socket method
   // ----------------------------------------------------------------------------------------------
   // COMMAND                          command (alternative to USP websocket) to execute
   // ENVIRONMENT      environment for command (alternative to USP websocket) to execute
   //
   // MAX_MESSAGE_SIZE Maximum size (in bytes) of a message to accept; default is approximately 4GB
   // ----------------------------------------------------------------------------------------------

   if (cfg.loadTable())
      {
      command = UServer_Base::loadConfigCommand(cfg);

      UWebSocket::max_message_size = cfg.readLong(*str_MAX_MESSAGE_SIZE, U_STRING_LIMIT);
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UWebSocketPlugIn::handlerRun()
{
   U_TRACE(0, "UWebSocketPlugIn::handlerRun()")

   UString key = U_STRING_FROM_CONSTANT("/modsocket");

   UHTTP::UServletPage* usp = UHTTP::getUSP(key);

   if (usp)
      {
      U_INTERNAL_DUMP("usp->alias = %b usp->runDynamicPage = %p", usp->alias, usp->runDynamicPage)

      U_INTERNAL_ASSERT_POINTER(usp->runDynamicPage)

      on_message = usp->runDynamicPage;

      goto end;
      }

   if (command) goto end;

   U_SRV_LOG("initialization of plugin FAILED");

   U_RETURN(U_PLUGIN_HANDLER_ERROR);

end:
   U_SRV_LOG("initialization of plugin success");

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int UWebSocketPlugIn::handlerRequest()
{
   U_TRACE(0, "UWebSocketPlugIn::handlerRequest()")

   if (U_http_websocket)
      {
      U_INTERNAL_ASSERT_MAJOR(u_http_info.websocket_len, 0)

      int sock = 0, fdmax = 0;
      fd_set fd_set_read, read_set;
      bool bcommand = (command && on_message == 0);
      USocket* csocket = UServer_Base::pClientImage->socket;

      if (bcommand)
         {
         // Set environment for the command application server

         UString environment = UHTTP::getCGIEnvironment();

         if (environment.empty()) U_RETURN(U_PLUGIN_HANDLER_ERROR);

         (void) environment.append(command->getStringEnvironment());

         command->setEnvironment(&environment);

         static int fd_stderr = UServices::getDevNull("/tmp/UWebSocketPlugIn.err");

         if (command->execute((UString*)-1, (UString*)-1, -1, fd_stderr))
            {
            UServer_Base::logCommandMsgError(command->getCommand(), true);

            UInterrupt::setHandlerForSignal(SIGTERM, (sighandler_t)UWebSocketPlugIn::handlerForSigTERM); // sync signal
            }

         sock  = csocket->iSockDesc;
         fdmax = U_max(sock, UProcess::filedes[2]) + 1;

         FD_ZERO(                     &fd_set_read);
         FD_SET(sock,                 &fd_set_read);
         FD_SET(UProcess::filedes[2], &fd_set_read);
         }

      UWebSocket::checkForPipeline();

      if (bcommand == false) goto handle_data;

loop:
      read_set = fd_set_read;

      if (U_SYSCALL(select, "%d,%p,%p,%p,%p", fdmax, &read_set, 0, 0, 0) > 0)
         {
         if (FD_ISSET(UProcess::filedes[2], &read_set))
            {
            UClientImage_Base::rbuffer->setEmpty();

            if (UServices::read(UProcess::filedes[2], *UClientImage_Base::rbuffer) &&
                UWebSocket::sendData(UWebSocket::message_type, (const unsigned char*)U_STRING_TO_PARAM(*UClientImage_Base::rbuffer)))
               {
               UClientImage_Base::rbuffer->setEmpty();

               goto loop;
               }
            }
         else if (FD_ISSET(sock, &read_set))
            {
handle_data:
            if (UWebSocket::handleDataFraming(csocket) == STATUS_CODE_OK &&
                (bcommand == false ? on_message(UServer_Base::pClientImage) == 0
                                   : UNotifier::write(UProcess::filedes[1], U_STRING_TO_PARAM(*UClientImage_Base::wbuffer))))
               {
               UClientImage_Base::rbuffer->setEmpty();

               if (bcommand == false) goto handle_data;

               goto loop;
               }
            }
         }

      // Send server-side closing handshake

      if (csocket->isOpen()) (void) UWebSocket::sendClose();

      UHTTP::setHTTPRequestProcessed();

      U_http_is_connection_close = U_YES;

      UClientImage_Base::write_off = true;
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UWebSocketPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "on_message        " << (void*)on_message << '\n'
                  << "command (UCommand " << (void*)command    << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
