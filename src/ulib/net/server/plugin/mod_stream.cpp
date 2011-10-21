// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_stream.cpp - distributing realtime input
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
#include <ulib/net/server/plugin/mod_stream.h>

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_stream, UStreamPlugIn)
#endif

pid_t UStreamPlugIn::pid = (pid_t)-1;

const UString* UStreamPlugIn::str_URI_PATH;
const UString* UStreamPlugIn::str_METADATA;
const UString* UStreamPlugIn::str_CONTENT_TYPE;

void UStreamPlugIn::str_allocate()
{
   U_TRACE(0, "UStreamPlugIn::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_URI_PATH,0)
   U_INTERNAL_ASSERT_EQUALS(str_METADATA,0)
   U_INTERNAL_ASSERT_EQUALS(str_CONTENT_TYPE,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("URI_PATH") },
      { U_STRINGREP_FROM_CONSTANT("METADATA") },
      { U_STRINGREP_FROM_CONSTANT("CONTENT_TYPE") }
   };

   U_NEW_ULIB_OBJECT(str_URI_PATH,     U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_METADATA,     U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_CONTENT_TYPE, U_STRING_FROM_STRINGREP_STORAGE(2));
}

RETSIGTYPE UStreamPlugIn::handlerForSigTERM(int signo)
{
   U_TRACE(0, "[SIGTERM] UStreamPlugIn::handlerForSigTERM(%d)", signo)

   if (pid != -1) UProcess::kill(pid, SIGTERM);

   UInterrupt::sendOurselves(SIGTERM);
}

UStreamPlugIn::~UStreamPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, UStreamPlugIn)

   if (command)
      {
                     delete command;
      if (fmetadata) delete fmetadata;

      if (pid != -1) UProcess::kill(pid, SIGTERM);
      }
}

// Server-wide hooks

int UStreamPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UStreamPlugIn::handlerConfig(%p)", &cfg)

   // ------------------------------------------------------------------------------------------------------------------------
   // mod_stream - plugin parameters
   // ------------------------------------------------------------------------------------------------------------------------
   // URI_PATH     specifies the local part of the URL path at which you would like the content to appear (Ex. /my/video.mjpeg)
   // METADATA     specifies the needs to have setup headers prepended for each codec stream (Ex. /my/audio.ogg)
   // CONTENT_TYPE specifies the Internet media type of the stream, which will appear in the Content-Type HTTP response header
   //
   // COMMAND                      command to execute
   // ENVIRONMENT  environment for command to execute
   // ------------------------------------------------------------------------------------------------------------------------

   if (cfg.loadTable())
      {
      uri_path     = cfg[*str_URI_PATH];
      metadata     = cfg[*str_METADATA];
      content_type = cfg[*str_CONTENT_TYPE];

      command = UServer_Base::loadConfigCommand(cfg);
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UStreamPlugIn::handlerInit()
{
   U_TRACE(0, "UStreamPlugIn::handlerInit()")

   if (command)
      {
      UServer_Base::runAsUser();

#  ifdef DEBUG
      int fd_stderr = UFile::creat("/tmp/UStreamPlugIn.err", O_WRONLY | O_APPEND, PERM_FILE);
#  else
      int fd_stderr = UServices::getDevNull();
#  endif

      if (command->execute(0, (UString*)-1, -1, fd_stderr))
         {
         UServer_Base::logCommandMsgError(command->getCommand());

         rbuf.init(2 * 1024 * 1024); // 2M size ring buffer

         if (metadata.empty() == false)
            {
            fmetadata = U_NEW(UFile(metadata));

            if (fmetadata->open()) fmetadata->readSize();
            }

         // NB: feeding by a child of this...

         U_INTERNAL_ASSERT_POINTER(UServer_Base::proc)

         if (UServer_Base::proc->fork() &&
             UServer_Base::proc->parent())
            {
            pid = UServer_Base::proc->pid();

            /*
            pid_t pgid = U_SYSCALL_NO_PARAM(getpgrp);

            UProcess::setProcessGroup(pid, pgid);
            */

            (void) content_type.append(U_CONSTANT_TO_PARAM(U_CRLF));

            U_SRV_LOG("initialization of plugin success");

            goto end;
            }

         if (UServer_Base::proc->child())
            {
            UTimeVal to_sleep(0L, 50 * 1000L);

            pid = UCommand::pid;

            if (UServer_Base::isLog()) u_unatexit(&ULog::close); // NB: needed because all instance try to close the log... (inherits from its parent)

            UInterrupt::insert(SIGTERM, (sighandler_t)UStreamPlugIn::handlerForSigTERM); // async signal

            int nread;

            while (UNotifier::waitForRead(UProcess::filedes[2]) >= 1)
               {
               nread = rbuf.readFromFdAndWrite(UProcess::filedes[2]);

               if (nread == 0) break;                // EOF
               if (nread  < 0) to_sleep.nanosleep(); // EAGAIN
               }

            handlerForSigTERM(SIGTERM);

            goto end;
            }
         }
      }

   U_SRV_LOG("initialization of plugin FAILED");

end:
   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int UStreamPlugIn::handlerRequest()
{
   U_TRACE(0, "UStreamPlugIn::handlerRequest()")

   if (command &&
       U_HTTP_URI_EQUAL(uri_path))
      {
      USocket* csocket = UServer_Base::pClientImage->socket;

      u_http_info.nResponseCode  = HTTP_OK;
      U_http_is_connection_close = U_YES;

      UHTTP::setHTTPResponse(&content_type);

      csocket->setTcpCork(1U);

      if (UServer_Base::pClientImage->handlerWrite() == U_NOTIFIER_OK)
         {
         int readd;

         UClientImage_Base::write_off = true;

         if (UHTTP::isHttpHEAD()) goto end;

         readd = rbuf.open();

         if (readd != -1)
            {
            if (fmetadata &&
                csocket->sendfile(fmetadata->getFd(), 0, fmetadata->getSize()) == false) goto end;

            UTimeVal to_sleep(0L, 10 * 1000L);

            while (UServer_Base::flag_loop)
               {
               if (rbuf.isEmpty(readd) == false &&
                   (rbuf.readAndWriteToFd(readd, csocket->iSockDesc) <= 0 && errno != EAGAIN)) break;

               to_sleep.nanosleep();
               }

            rbuf.close(readd);
            }
         }

end:
      UHTTP::setHTTPRequestProcessed();
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UStreamPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "pid                       " << pid                  << '\n'
                  << "fmetadata    (UFile       " << (void*)fmetadata     << ")\n"
                  << "uri_path     (UString     " << (void*)&uri_path     << ")\n"
                  << "metadata     (UString     " << (void*)&metadata     << ")\n"
                  << "content_type (UString     " << (void*)&content_type << ")\n"
                  << "command      (UCommand    " << (void*)command       << ")\n"
                  << "rbuf         (URingBuffer " << (void*)&rbuf         << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
