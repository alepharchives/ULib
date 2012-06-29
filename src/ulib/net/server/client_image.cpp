// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    client_image.cpp - Handles accepted TCP/IP connections
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/utility/uhttp.h>
#include <ulib/utility/escape.h>
#include <ulib/net/server/server.h>

bool        UClientImage_Base::bIPv6;
bool        UClientImage_Base::pipeline;
bool        UClientImage_Base::write_off;
uint32_t    UClientImage_Base::rstart;
uint32_t    UClientImage_Base::counter;
uint32_t    UClientImage_Base::size_request;
UString*    UClientImage_Base::body;
UString*    UClientImage_Base::rbuffer;
UString*    UClientImage_Base::wbuffer;
UString*    UClientImage_Base::request; // NB: it is only a pointer, not a string object...
UString*    UClientImage_Base::pbuffer;
UString*    UClientImage_Base::msg_welcome;
const char* UClientImage_Base::rpointer;

#ifdef USE_LIBSSL
SSL_CTX*    UClientImage_Base::ctx;
#endif

// NB: these are for ULib Servlet Page (USP) - USP_PRINTF...

UString* UClientImage_Base::_value;
UString* UClientImage_Base::_buffer;
UString* UClientImage_Base::_encoded;

// NB: we cannot put this in .h for the dependency of UServer_Base class...
// ------------------------------------------------------------------------
void UClientImage_Base::logRequest(const char* filereq)
{
   U_TRACE(0+256, "UClientImage_Base::logRequest(%S)", filereq)

   U_INTERNAL_ASSERT_POINTER(logbuf)
   U_INTERNAL_ASSERT_POINTER(request)
   U_INTERNAL_ASSERT(UServer_Base::isLog())
   U_ASSERT_EQUALS(request->empty(), false)

   const char* ptr = request->data();
   uint32_t sz = request->size(), u_printf_string_max_length_save = u_printf_string_max_length;

   U_INTERNAL_DUMP("u_printf_string_max_length = %d", u_printf_string_max_length)

   if (u_printf_string_max_length == -1)
      {
      u_printf_string_max_length = u_findEndHeader(ptr, sz);

      if (u_printf_string_max_length == -1) u_printf_string_max_length = sz;

      U_INTERNAL_ASSERT_MAJOR(u_printf_string_max_length, 0)
      }

   U_INTERNAL_DUMP("u_printf_string_max_length = %d", u_printf_string_max_length)

   UServer_Base::log->log("%.*sreceived request (%u bytes) %s%.*S from %.*s\n",
                           U_STRING_TO_TRACE(*UServer_Base::mod_name), sz, (pipeline ? "[pipeline] " : "" ), sz, ptr, U_STRING_TO_TRACE(*logbuf));

   u_printf_string_max_length = u_printf_string_max_length_save;

#if GCC_VERSION_NUM != 30303 /* Test for GCC == 3.3.3 (SuSE Linux) */
   if (filereq) (void) UFile::writeToTmpl(filereq, *request, true);
#endif
}

void UClientImage_Base::logResponse(const char* fileres)
{
   U_TRACE(0+256, "UClientImage_Base::logResponse(%S)", fileres)

   U_INTERNAL_ASSERT_POINTER(logbuf)
   U_INTERNAL_ASSERT_POINTER(wbuffer)
   U_INTERNAL_ASSERT(UServer_Base::isLog())
   U_ASSERT_EQUALS(wbuffer->empty(), false)

   const char* ptr = wbuffer->data();
   uint32_t sz = wbuffer->size(), u_printf_string_max_length_save = u_printf_string_max_length;

   U_INTERNAL_DUMP("u_printf_string_max_length = %d", u_printf_string_max_length)

   if (u_printf_string_max_length == -1)
      {
      U_ASSERT_EQUALS(wbuffer->empty(), false)

      u_printf_string_max_length = u_findEndHeader(ptr, sz);

      if (u_printf_string_max_length == -1) u_printf_string_max_length = sz;
#ifdef DEBUG
      else
         {
         U_ASSERT(wbuffer->isEndHeader(u_printf_string_max_length-4))
         }
#endif

      U_INTERNAL_ASSERT_MAJOR(u_printf_string_max_length, 0)
      }

   U_INTERNAL_DUMP("u_printf_string_max_length = %d", u_printf_string_max_length)
   U_INTERNAL_DUMP("pipeline = %b size_request = %u pbuffer->size() = %u", pipeline, size_request, pbuffer->size())

   UServer_Base::log->log("%.*ssent response (%u bytes) %s%.*S to %.*s\n",
         U_STRING_TO_TRACE(*UServer_Base::mod_name), sz + body->size(), (pipeline ? "[pipeline] " : "" ), sz, ptr, U_STRING_TO_TRACE(*logbuf));

   u_printf_string_max_length = u_printf_string_max_length_save;

#if GCC_VERSION_NUM != 30303 /* Test for GCC == 3.3.3 (SuSE Linux) */
   if (fileres) (void) UFile::writeToTmpl(fileres, *wbuffer, true);
#endif
}

UClientImage_Base::UClientImage_Base()
{
   U_TRACE_REGISTER_OBJECT(0, UClientImage_Base, "")

   socket        = 0;
   logbuf        = (UServer_Base::isLog() ? U_NEW(UString(4000U)) : 0);
   last_response = 0;

   start = count = 0;
   state = sfd = bclose = 0;
}

// ------------------------------------------------------------------------

void UClientImage_Base::init()
{
   U_TRACE(0, "UClientImage_Base::init()")

   U_INTERNAL_ASSERT_EQUALS(body,0)
   U_INTERNAL_ASSERT_EQUALS(_value,0)
   U_INTERNAL_ASSERT_EQUALS(rbuffer,0)
   U_INTERNAL_ASSERT_EQUALS(wbuffer,0)
   U_INTERNAL_ASSERT_EQUALS(pbuffer,0)
   U_INTERNAL_ASSERT_EQUALS(_buffer,0)
   U_INTERNAL_ASSERT_EQUALS(_encoded,0)

   body    = U_NEW(UString);
   rbuffer = U_NEW(UString(U_CAPACITY));
   wbuffer = U_NEW(UString);
   pbuffer = U_NEW(UString);

   // NB: these are for ULib Servlet Page (USP) - USP_PRINTF...

   _value   = U_NEW(UString(U_CAPACITY));
   _buffer  = U_NEW(UString(U_CAPACITY));
   _encoded = U_NEW(UString(U_CAPACITY));
}

void UClientImage_Base::clear()
{
   U_TRACE(0, "UClientImage_Base::clear()")

   U_INTERNAL_ASSERT_POINTER(body)
   U_INTERNAL_ASSERT_POINTER(wbuffer)
   U_INTERNAL_ASSERT_POINTER(pbuffer)
   U_INTERNAL_ASSERT_POINTER(rbuffer)

   if (msg_welcome) delete msg_welcome;

   if (body)
      {
      delete body;
      delete wbuffer;
      delete pbuffer;
      delete rbuffer;

      // NB: these are for ULib Servlet Page (USP) - USP_PRINTF...

      U_INTERNAL_ASSERT_POINTER(_value)
      U_INTERNAL_ASSERT_POINTER(_buffer)
      U_INTERNAL_ASSERT_POINTER(_encoded)

      delete _value;
      delete _buffer;
      delete _encoded;
      }
}

// Check whether the ip address client ought to be allowed

__pure bool UClientImage_Base::isAllowed(UVector<UIPAllow*>& vallow_IP)
{
   U_TRACE(0, "UClientImage_Base::isAllowed(%p)", &vallow_IP)

   U_INTERNAL_ASSERT_POINTER(socket)

   bool ok = UIPAllow::isAllowed(socket->remoteIPAddress().getInAddr(), vallow_IP);

   U_RETURN(ok);
}

void UClientImage_Base::setMsgWelcome(const UString& msg)
{
   U_TRACE(0, "UClientImage_Base::setMsgWelcome(%.*S)", U_STRING_TO_TRACE(msg))

   if (msg.empty() == false)
      {
      msg_welcome = U_NEW(UString(U_CAPACITY));

      if (UEscape::decode(msg, *msg_welcome) == false)
         {
         delete msg_welcome;
                msg_welcome = 0;
         }
      }
}

// aggiungo nel log il certificato Peer del client ("issuer","serial")

void UClientImage_Base::logCertificate(void* x509)
{
   U_TRACE(0, "UClientImage_Base::logCertificate(%p)", x509)

   // NB: OpenSSL already tested the cert validity during SSL handshake and returns a X509 ptr just if the certificate is valid...

#ifdef USE_LIBSSL
   if (x509)
      {
      U_INTERNAL_ASSERT_POINTER(logbuf)
      U_INTERNAL_ASSERT(UServer_Base::isLog())

      UCertificate::setForLog((X509*)x509, *logbuf);

      U_INTERNAL_DUMP("logbuf = %.*S", U_STRING_TO_TRACE(*logbuf))
      }
#endif
}

void UClientImage_Base::manageRequestSize(bool request_buffer_resize)
{
   U_TRACE(0, "UClientImage_Base::manageRequestSize(%b)", request_buffer_resize)

   U_INTERNAL_DUMP("pipeline = %b size_request = %u pbuffer->size() = %u", pipeline, size_request, pbuffer->size())

   if (pipeline)
      {
      U_INTERNAL_ASSERT_EQUALS(pbuffer->isNull(),false)
      U_INTERNAL_ASSERT_EQUALS(pbuffer->same(*rbuffer),false)

      if (rstart == 0U) pipeline = false;
      else
         {
         U_INTERNAL_ASSERT_EQUALS(request, pbuffer)

         if (request_buffer_resize == false) pbuffer->size_adjust(size_request);
         else
            {
            // NB: we use request as the new read buffer... 

            pipeline = false;
            rstart   = size_request = 0;
            rpointer = pbuffer->data();

            *(request = rbuffer) = *pbuffer;
                                    pbuffer->clear();
            }
         }
      }
   else if (size_request)
      {
      pipeline = (rbuffer->size() > size_request);

      if (pipeline)
         {
         U_INTERNAL_ASSERT_EQUALS(rstart,0U)
         U_INTERNAL_ASSERT(pbuffer->isNull())

         *(request = pbuffer) = rbuffer->substr(0U, size_request);
         }
      }

   U_INTERNAL_DUMP("pipeline = %b size_request = %u pbuffer->size() = %u", pipeline, size_request, pbuffer->size())
}

__pure int UClientImage_Base::genericRead()
{
   U_TRACE(0, "UClientImage_Base::genericRead()")

   U_INTERNAL_ASSERT_POINTER(body)
   U_INTERNAL_ASSERT_POINTER(socket)
   U_INTERNAL_ASSERT_POINTER(rbuffer)
   U_INTERNAL_ASSERT_POINTER(pbuffer)
   U_INTERNAL_ASSERT_POINTER(wbuffer)
   U_INTERNAL_ASSERT_EQUALS(socket->iSockDesc,UEventFd::fd)

   handlerError(USocket::CONNECT); // NB: we must call function cause of SSL (must be a virtual method)...

   // reset buffer before read

   rbuffer->setBuffer(U_CAPACITY); // NB: this string can be referenced more than one (often if U_SUBSTR_INC_REF is defined)...

   if (USocketExt::read(socket, *rbuffer, U_SINGLE_READ, 0) == false) // NB: the timeout at 0 means that we put the socket fd on epoll queue if EAGAIN...
      {
      // check if close connection... (read() == 0)

      if (socket->isClosed())
         {
         if (UServer_Base::isParallelization()) U_EXIT(0);

         U_RETURN(U_PLUGIN_HANDLER_ERROR);
         }

      if (rbuffer->empty()) U_RETURN(U_PLUGIN_HANDLER_AGAIN); // NONBLOCKING...
      }

   request = rbuffer;

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

void UClientImage_Base::initAfterGenericRead()
{
   U_TRACE(0, "UClientImage_Base::initAfterGenericRead()")

   U_INTERNAL_DUMP("pipeline = %b pbuffer = %.*S", pipeline, U_STRING_TO_TRACE(*pbuffer))

   U_INTERNAL_ASSERT(pbuffer->isNull())
   U_INTERNAL_ASSERT_EQUALS(pipeline,false)

   if (body->isNull() == false) body->clear();
                             wbuffer->clear();
}

// method VIRTUAL to redefine

bool UClientImage_Base::newConnection()
{
   U_TRACE(0, "UClientImage_Base::newConnection()")

   U_INTERNAL_ASSERT_POINTER(socket)
   U_INTERNAL_ASSERT_EQUALS(UEventFd::fd,0)

   U_INTERNAL_DUMP("fd = %d sock_fd = %d", UEventFd::fd, socket->iSockDesc)

   UEventFd::fd = socket->iSockDesc;

   if (logbuf)
      {
      bool berror = false;
      UString tmp(U_CAPACITY);

      socket->getRemoteInfo(*logbuf);

      if (ULog::prefix) tmp.snprintf(ULog::prefix);

      tmp.snprintf_add("new client connected from %.*s, %s clients currently connected\n", U_STRING_TO_TRACE(*logbuf), UServer_Base::getNumConnection());

      if (msg_welcome)
         {
         if (ULog::prefix) tmp.snprintf_add(ULog::prefix);

         tmp.snprintf_add("sent welcome message to %.*s\n", U_STRING_TO_TRACE(*logbuf));

         if (USocketExt::write(socket, *msg_welcome) == false) berror = true;
         }

      struct iovec iov[1] = { { (caddr_t)tmp.data(), tmp.size() } };

      UServer_Base::log->write(iov, 1);

      if (berror)
         {
         U_INTERNAL_ASSERT_EQUALS(socket->iSockDesc, 0)

         U_RETURN(false);
         }
      }

   U_RETURN(true);
}

// define method VIRTUAL of class UEventFd

void UClientImage_Base::handlerError(int sock_state)
{
   U_TRACE(0, "UClientImage_Base::handlerError(%d)", sock_state)

   U_INTERNAL_ASSERT_POINTER(socket)

   U_INTERNAL_DUMP("fd = %d sock_fd = %d", UEventFd::fd, socket->iSockDesc)

   socket->iState = sock_state;

   UServer_Base::pClientImage = this;

// if (socket->isTimeout()) {} // maybe we need some specific processing...
}

int UClientImage_Base::handlerRead()
{
   U_TRACE(0, "UClientImage_Base::handlerRead()")

   U_INTERNAL_ASSERT_POINTER(body)
   U_INTERNAL_ASSERT_POINTER(socket)
   U_INTERNAL_ASSERT_POINTER(rbuffer)
   U_INTERNAL_ASSERT_POINTER(wbuffer)
   U_INTERNAL_ASSERT_POINTER(pbuffer)

   // Connection-wide hooks

   state = genericRead(); // read request...

   if (state == U_PLUGIN_HANDLER_AGAIN) U_RETURN(U_NOTIFIER_OK); // NONBLOCKING...
   if (state == U_PLUGIN_HANDLER_ERROR) U_RETURN(U_NOTIFIER_DELETE);

   rstart   = size_request = 0;
   pipeline = false;
   rpointer = rbuffer->data();

#ifndef U_HTTP_CACHE_REQUEST
   initAfterGenericRead();
#endif

loop:
   ++counter;

   U_INTERNAL_DUMP("counter = %u", counter)

   if (logbuf) logRequest();

   state = UServer_Base::pluginsHandlerREAD(); // check request type...

   if (state == U_PLUGIN_HANDLER_FINISHED)
      {
      // NB: it is possible a resize of the read buffer string...

      if (rpointer != rbuffer->data())
         {
          rpointer  = rbuffer->data();

         if (pipeline) *pbuffer = rbuffer->substr(rstart);
         }

      state = UServer_Base::pluginsHandlerRequest(); // manage request...
      }

   U_INTERNAL_DUMP("state = %d pipeline = %b socket->isClosed() = %b write_off = %b", state, pipeline, socket->isClosed(), write_off)

   if (write_off ||
       socket->isClosed())
      {
      if (write_off)
         {
         write_off = false;

         goto next;
         }
      
      goto error;
      }

   if (handlerWrite() == U_NOTIFIER_DELETE) state = U_PLUGIN_HANDLER_ERROR;

#ifdef U_HTTP_CACHE_REQUEST
   if ((state & U_PLUGIN_HANDLER_AGAIN) != 0)
      {
      if ((state & U_PLUGIN_HANDLER_ERROR) == 0) goto end;
           state = U_PLUGIN_HANDLER_ERROR;
      }
   else
#endif
next:
      if (UServer_Base::bpluginsHandlerReset &&
          UServer_Base::pluginsHandlerReset() == U_PLUGIN_HANDLER_ERROR)
      {
      state = U_PLUGIN_HANDLER_ERROR;
      }

   U_INTERNAL_DUMP("state = %d pipeline = %b socket->isClosed() = %b", state, pipeline, socket->isClosed())

   // NB: it is difficult to change this tests...
   // -------------------------------------------
   if (socket->isClosed()                  ||
       (state    == U_PLUGIN_HANDLER_ERROR &&
        pipeline == false)                 ||
        UServer_Base::flag_loop == false)
      {
error:
      if (UServer_Base::isParallelization()) U_EXIT(0);
  
      resetPipeline();

      U_RETURN(U_NOTIFIER_DELETE);
      }
   // -------------------------------------------

   if (pipeline)
      {
      // manage pipelining

      U_INTERNAL_ASSERT_POINTER(request)
      U_INTERNAL_ASSERT(request == pbuffer && pbuffer->isNull() == false && pbuffer->same(*rbuffer) == false)

      U_INTERNAL_DUMP("size_request = %u request->size() = %u", size_request, request->size())

      if (size_request != request->size())
         {
         U_INTERNAL_ASSERT_EQUALS(state, U_PLUGIN_HANDLER_ERROR)
         }
      else
         {
         U_INTERNAL_DUMP("rstart = %u rbuffer->size() = %u", rstart, rbuffer->size())

         rstart += size_request;

         if (rbuffer->size() > rstart)
            {
            // NB: check for spurios white space (IE)...

            if (rbuffer->isWhiteSpace(rstart) == false)
               {
               *pbuffer = rbuffer->substr(rstart);

               // 18 -> "GET / HTTP/1.0\r\n\r\n"

               if (pbuffer->size() < 18) pbuffer->reserve(U_CAPACITY);

                  body->clear();
               wbuffer->clear();

               goto loop;
               }
            }
         }

      pbuffer->clear();
      }

#ifdef U_HTTP_CACHE_REQUEST
end:
#endif
   if (u_pthread_time)
      {
      last_response = u_now->tv_sec;

      U_INTERNAL_DUMP("last_response = %ld", last_response)
      }

   U_RETURN(U_NOTIFIER_OK);
}

int UClientImage_Base::handlerWrite()
{
   U_TRACE(0, "UClientImage_Base::handlerWrite()")

   U_INTERNAL_DUMP("wbuffer(%u) = %.*S", wbuffer->size(), U_STRING_TO_TRACE(*wbuffer))
   U_INTERNAL_DUMP("   body(%u) = %.*S",    body->size(), U_STRING_TO_TRACE(*body))

   U_INTERNAL_ASSERT(socket->isOpen())
   U_INTERNAL_ASSERT_EQUALS(write_off, false)

   if (count)
      {
      U_INTERNAL_ASSERT_MAJOR(sfd, 0)
      U_INTERNAL_ASSERT_EQUALS(socket->iSockDesc, UEventFd::fd)

      if (UEventFd::op_mask == U_WRITE_OUT) goto send;

      U_ASSERT(body->empty())

#  if defined(LINUX) || defined(__LINUX__) || defined(__linux__)
      socket->setTcpCork(1U); // On Linux, sendfile() depends on the TCP_CORK socket option to avoid undesirable packet boundaries
#  endif
      }

   U_ASSERT_EQUALS(wbuffer->empty(), false)

   if (logbuf) logResponse();

   {
   size_t sz1      = wbuffer->size(),
          sz2      =    body->size();
   uint32_t ncount = sz1;
   const char* ptr = wbuffer->data();

   struct iovec _iov[2] = { { (caddr_t)ptr,          sz1 },
                            { (caddr_t)body->data(), sz2 } };

   int iBytesWrite = (sz2 ? (ncount += sz2, USocketExt::writev(socket, _iov, 2, ncount, UServer_Base::timeoutMS))
                          :                 USocketExt::write( socket,        ptr, sz1, UServer_Base::timeoutMS));

   if (iBytesWrite < (int)ncount)
      {
      U_INTERNAL_ASSERT_EQUALS(_iov[0].iov_len + _iov[1].iov_len, ncount - iBytesWrite)

      if (sfd      == 0               &&
          pipeline == false           &&
#        ifdef USE_LIBSSL
          UServer_Base::bssl == false &&
#        endif
          socket->isOpen())
         {
         char path[MAX_FILENAME_LEN];

         uint32_t len = u__snprintf(path, sizeof(path), "%s/pwrite.%P.%4D", u_tmpdir);

         sfd = UFile::creat(path);

         if (sfd != -1            &&
             UFile::_unlink(path) &&
             (ncount -= iBytesWrite, UFile::writev(sfd, _iov, 2) == (int)ncount))
            {
            if (logbuf)
               {
               UServer_Base::log->log("%.*spartial write (%u bytes): create temporary file %.*S\n",
                                       U_STRING_TO_TRACE(*UServer_Base::mod_name), iBytesWrite, len, path);
               }

            start  = 0;
            count  = ncount;
            bclose = U_PARTIAL | U_http_is_connection_close;
                                 U_http_is_connection_close = U_NOT;

            UEventFd::op_mask = U_WRITE_OUT;

            if (UNotifier::find(UEventFd::fd)) UNotifier::modify(this);

#        ifdef U_HTTP_CACHE_REQUEST
            U_http_no_cache = true; 
#        endif

            U_RETURN(U_NOTIFIER_OK);
            }

         UFile::close(sfd);
                      sfd = 0;
         }

      U_RETURN(U_NOTIFIER_DELETE);
      }
   }

   if (count)
      {
      off_t offset;
send:
      U_INTERNAL_DUMP("sfd = %d count = %u bclose = %B", sfd, count, bclose)

      offset = start;

#  ifdef __MINGW32__
      ssize_t value = U_SYSCALL(sendfile, "%d,%d,%p,%u", socket->getFd(), sfd, &offset, count);
#  else
      ssize_t value = U_SYSCALL(sendfile, "%d,%d,%p,%u",    UEventFd::fd, sfd, &offset, count);
#  endif

      if (value < 0L)
         {
         U_INTERNAL_DUMP("errno = %d", errno)

         if (errno != EAGAIN) U_RETURN(U_NOTIFIER_DELETE);
         }

      count -= value;

#  if defined(LINUX) || defined(__LINUX__) || defined(__linux__)
      // On Linux, sendfile() depends on the TCP_CORK socket option to avoid undesirable packet boundaries
      if (UEventFd::op_mask != U_WRITE_OUT) socket->setTcpCork(0U);
#  endif

      if (count)
         {
         start += value;

         if (UEventFd::op_mask != U_WRITE_OUT)
            {
             UEventFd::op_mask  = U_WRITE_OUT;

            if (UNotifier::find(UEventFd::fd)) UNotifier::modify(this);
            }
         }
      else
         {
         bool bwrite = (UEventFd::op_mask == U_WRITE_OUT);
                        UEventFd::op_mask  = U_READ_IN;

         if ((bclose & U_PARTIAL) != 0) UFile::close(sfd);
                                                     sfd = 0;

         if ((bclose & U_YES)     != 0) U_RETURN(U_NOTIFIER_DELETE);

         if (bwrite) UNotifier::modify(this);
         }
      }

   U_RETURN(U_NOTIFIER_OK);
}

void UClientImage_Base::handlerDelete()
{
   U_TRACE(0, "UClientImage_Base::handlerDelete()")

   U_INTERNAL_DUMP("UNotifier::num_connection = %d UNotifier::min_connection = %d", UNotifier::num_connection, UNotifier::min_connection)

   if (UNotifier::num_connection > UNotifier::min_connection)
      {
      UServer_Base::handlerCloseConnection(this);

      if (socket->isOpen()) socket->closesocket();

      --UNotifier::num_connection;

      U_INTERNAL_DUMP("UNotifier::num_connection = %d UNotifier::min_connection = %d", UNotifier::num_connection, UNotifier::min_connection)
      U_INTERNAL_DUMP("sfd = %d count = %u UEventFd::op_mask = %B", sfd, count, UEventFd::op_mask)

      if (count)
         {
         // NB: pending sendfile...

         U_INTERNAL_ASSERT_MAJOR(sfd, 0)

         sfd   = 0;
         count = 0;

         UEventFd::op_mask = U_READ_IN;

      // if (USocket::accept4_flags & SOCK_NONBLOCK) socket->flags |= O_NONBLOCK;
         }

      // NB: to reuse object...

      UEventFd::fd = 0;

      U_INTERNAL_ASSERT_EQUALS(UEventFd::op_mask, U_READ_IN)
#  ifdef HAVE_ACCEPT4
      U_INTERNAL_ASSERT_EQUALS(((USocket::accept4_flags & SOCK_CLOEXEC)  != 0),((socket->flags & O_CLOEXEC)  != 0))
      U_INTERNAL_ASSERT_EQUALS(((USocket::accept4_flags & SOCK_NONBLOCK) != 0),((socket->flags & O_NONBLOCK) != 0))
#  endif
      }
#ifdef DEBUG
   else
      {
      delete socket;

      if (logbuf) delete logbuf;
      }
#endif
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UClientImage_Base::dump(bool _reset) const
{
   *UObjectIO::os << "sfd                                " << sfd                << '\n'
                  << "state                              " << state              << '\n'
                  << "start                              " << start              << '\n'
                  << "count                              " << count              << '\n'
                  << "bIPv6                              " << bIPv6              << '\n'
                  << "bclose                             " << bclose             << '\n'
                  << "write_off                          " << write_off          << '\n'
                  << "last_response                      " << last_response      << '\n'
                  << "body            (UString           " << (void*)body        << ")\n"
                  << "logbuf          (UString           " << (void*)logbuf      << ")\n"
                  << "rbuffer         (UString           " << (void*)rbuffer     << ")\n"
                  << "wbuffer         (UString           " << (void*)wbuffer     << ")\n"
                  << "request         (UString           " << (void*)request     << ")\n"
                  << "pbuffer         (UString           " << (void*)pbuffer     << ")\n"
                  << "socket          (USocket           " << (void*)socket      << ")\n"
                  << "msg_welcome     (UString           " << (void*)msg_welcome << ')';

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}
#endif
