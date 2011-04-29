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

bool               UClientImage_Base::bIPv6;
bool               UClientImage_Base::pipeline;
bool               UClientImage_Base::write_off;  // NB: we not send response because we can have used sendfile() etc...
uint32_t           UClientImage_Base::rstart;
uint32_t           UClientImage_Base::size_request;
USocket*           UClientImage_Base::socket;
UString*           UClientImage_Base::body;
UString*           UClientImage_Base::rbuffer;
UString*           UClientImage_Base::wbuffer;
UString*           UClientImage_Base::request; // NB: it is only a pointer, not a string object...
UString*           UClientImage_Base::pbuffer;
UString*           UClientImage_Base::msg_welcome;
const char*        UClientImage_Base::rpointer;
UClientImage_Base* UClientImage_Base::pClientImage;

// NB: these are for ULib Servlet Page (USP) - U_DYNAMIC_PAGE_OUTPUT...

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

   uint32_t u_printf_string_max_length_save = u_printf_string_max_length;

   U_INTERNAL_DUMP("u_printf_string_max_length = %d UHTTP::http_info.endHeader = %u", u_printf_string_max_length, UHTTP::http_info.endHeader)

   if (u_printf_string_max_length == -1)
      {
      if (UHTTP::isHTTPRequest()) // NB: only HTTP header...
         {
         u_printf_string_max_length = (UHTTP::http_info.endHeader ? UHTTP::http_info.endHeader : request->size()); 

         U_INTERNAL_ASSERT_MAJOR(u_printf_string_max_length, 0)
         }
      }

   UServer_Base::log->log("%sreceived request (%u bytes) %.*S from %.*s\n", UServer_Base::mod_name, request->size(),
                                                                            U_STRING_TO_TRACE(*request), U_STRING_TO_TRACE(*logbuf));

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

   U_INTERNAL_DUMP("u_printf_string_max_length = %d", u_printf_string_max_length)

   uint32_t u_printf_string_max_length_save = u_printf_string_max_length;

   if (u_printf_string_max_length == -1)
      {
      if (UHTTP::isHTTPRequest()) // NB: only HTTP header...
         {
         U_ASSERT_DIFFERS(wbuffer->empty(), true)

         u_printf_string_max_length = u_findEndHeader(U_STRING_TO_PARAM(*wbuffer)); 

         if (u_printf_string_max_length == -1)
            {
            U_ASSERT_DIFFERS(body->empty(), true)

            u_printf_string_max_length = wbuffer->size();
            }

         U_INTERNAL_ASSERT_MAJOR(u_printf_string_max_length, 0)
         }
      }

   UServer_Base::log->log("%ssent response (%u bytes) %.*S to %.*s\n", UServer_Base::mod_name, wbuffer->size() + body->size(),
                                                                       U_STRING_TO_TRACE(*wbuffer), U_STRING_TO_TRACE(*logbuf));

   u_printf_string_max_length = u_printf_string_max_length_save;

#if GCC_VERSION_NUM != 30303 /* Test for GCC == 3.3.3 (SuSE Linux) */
   if (fileres) (void) UFile::writeToTmpl(fileres, *wbuffer, true);
#endif
}
// ------------------------------------------------------------------------

void UClientImage_Base::init(USocket* p)
{
   U_TRACE(0, "UClientImage_Base::init(%p)", p)

   U_INTERNAL_ASSERT_EQUALS(body,     0);
   U_INTERNAL_ASSERT_EQUALS(socket,   0);
   U_INTERNAL_ASSERT_EQUALS(rbuffer,  0);
   U_INTERNAL_ASSERT_EQUALS(wbuffer,  0);
   U_INTERNAL_ASSERT_EQUALS(pbuffer,  0);
   U_INTERNAL_ASSERT_EQUALS(_value,   0);
   U_INTERNAL_ASSERT_EQUALS(_buffer,  0);
   U_INTERNAL_ASSERT_EQUALS(_encoded, 0);

   socket  = p;
   body    = U_NEW(UString);
   rbuffer = U_NEW(UString(U_CAPACITY));
   wbuffer = U_NEW(UString);
   pbuffer = U_NEW(UString);

   // NB: these are for ULib Servlet Page (USP) - U_DYNAMIC_PAGE_OUTPUT...

   _value   = U_NEW(UString(U_CAPACITY));
   _buffer  = U_NEW(UString(U_CAPACITY));
   _encoded = U_NEW(UString(U_CAPACITY));
}

void UClientImage_Base::clear()
{
   U_TRACE(0, "UClientImage_Base::clear()")

   U_INTERNAL_ASSERT_POINTER(body);
   U_INTERNAL_ASSERT_POINTER(socket);
   U_INTERNAL_ASSERT_POINTER(rbuffer);
   U_INTERNAL_ASSERT_POINTER(wbuffer);
   U_INTERNAL_ASSERT_POINTER(pbuffer);
   U_INTERNAL_ASSERT_POINTER(_value);
   U_INTERNAL_ASSERT_POINTER(_buffer);
   U_INTERNAL_ASSERT_POINTER(_encoded);

   if (msg_welcome) delete msg_welcome;

   delete body;
   delete wbuffer;
   delete pbuffer;
   delete rbuffer;

          socket->iSockDesc = -1;
   delete socket;

   // NB: these are for ULib Servlet Page (USP) - U_DYNAMIC_PAGE_OUTPUT...

   delete _value;
   delete _buffer;
   delete _encoded;
}

UClientImage_Base::UClientImage_Base()
{
   U_TRACE_REGISTER_OBJECT(0, UClientImage_Base, "")

#ifdef HAVE_SSL
   ssl = 0;
#endif

   if (UServer_Base::isLog())
      {
      logbuf        = U_NEW(UString(4000U));
      clientAddress = U_NEW(UIPAddress);
      }
   else
      {
      logbuf        = 0;
      clientAddress = 0;
      }
}

UIPAddress& UClientImage_Base::remoteIPAddress()
{
   U_TRACE(0, "UClientImage_Base::remoteIPAddress()")

   U_INTERNAL_ASSERT_POINTER(socket)
   U_INTERNAL_ASSERT_POINTER(pClientImage)

   if (pClientImage->logbuf) return *(pClientImage->clientAddress);

   socket->setRemote();

   return socket->remoteIPAddress();
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

#ifdef HAVE_SSL
   // NB: OpenSSL already tested the cert validity during SSL handshake and returns a X509 ptr just if the certificate is valid...

   if (x509)
      {
      U_INTERNAL_ASSERT(UServer_Base::isLog())
      U_INTERNAL_ASSERT_POINTER(logbuf)

      UCertificate::setForLog((X509*)x509, *logbuf);

      U_INTERNAL_DUMP("logbuf = %.*S", U_STRING_TO_TRACE(*logbuf))
      }
#endif
}

void UClientImage_Base::setRequestSize(uint32_t n)
{
   U_TRACE(0, "UClientImage_Base::setRequestSize(%u)", n)

   U_INTERNAL_ASSERT_MAJOR(n, 0)

   U_INTERNAL_DUMP("pipeline = %b", pipeline)

   size_request = n;

   if (pipeline)
      {
      U_INTERNAL_ASSERT_DIFFERS(rstart, 0U)
      U_INTERNAL_ASSERT(request == pbuffer && pbuffer->isNull() == false && pbuffer->same(*rbuffer) == false)

      pbuffer->size_adjust(n);
      }
   else
      {
      pipeline = (rbuffer->size() > n);

      if (pipeline)
         {
         U_INTERNAL_ASSERT_EQUALS(rstart, 0U)

         *pbuffer = rbuffer->substr(0U, n);
          request = pbuffer;
         }
      }
}

int UClientImage_Base::genericRead()
{
   U_TRACE(0, "UClientImage_Base::genericRead()")

   U_INTERNAL_ASSERT_POINTER(body)
   U_INTERNAL_ASSERT_POINTER(socket)
   U_INTERNAL_ASSERT_POINTER(rbuffer)
   U_INTERNAL_ASSERT_POINTER(pbuffer)
   U_INTERNAL_ASSERT_POINTER(wbuffer)

   // Check if it is the first use of this object...

   U_INTERNAL_DUMP("fd = %d sock_fd = %d", UEventFd::fd, socket->iSockDesc)

   if (UEventFd::fd == 0)
      {
      UEventFd::fd = socket->iSockDesc;

      if (logbuf)
         {
         bool berror = false;

         socket->getRemoteInfo(*logbuf);

         *clientAddress = socket->remoteIPAddress();

         UString tmp(U_CAPACITY);

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

         if (berror) goto error;
         }
      }

   // reset buffer before read

   U_INTERNAL_ASSERT_EQUALS(rbuffer->isNull(), false);
   U_ASSERT(                rbuffer->writeable())

   rbuffer->setBuffer(U_CAPACITY); // NB: this string can be referenced more than one (often if U_SUBSTR_INC_REF is defined)...

   // NB: we need this because we use the same object USocket...

   handlerError(USocket::CONNECT);

   if (USocketExt::read(socket, *(request = rbuffer), U_SINGLE_READ, UServer_Base::timeoutMS) == false)
      {
      // check if close connection... (read() == 0)

      if (socket->isClosed()) goto error;
      if (rbuffer->empty())   U_RETURN(U_PLUGIN_HANDLER_AGAIN); // NONBLOCKING...
      }

   pClientImage = this;

   // reset buffer after read

   if (   body->isNull() == false)    body->clear();
   if (pbuffer->isNull() == false) pbuffer->clear();
                                   wbuffer->clear();

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);

error:
   if (UServer_Base::flag_loop == false &&
       UServer_Base::isParallelization())
      {
      U_EXIT(0);
      }

   U_RETURN(U_PLUGIN_HANDLER_ERROR);
}

// define method VIRTUAL of class UEventFd

int UClientImage_Base::handlerRead()
{
   U_TRACE(0, "UClientImage_Base::handlerRead()")

   U_INTERNAL_ASSERT_POINTER(body)
   U_INTERNAL_ASSERT_POINTER(socket)
   U_INTERNAL_ASSERT_POINTER(rbuffer)
   U_INTERNAL_ASSERT_POINTER(wbuffer)
   U_INTERNAL_ASSERT_POINTER(pbuffer)

   // Connection-wide hooks

   int result = genericRead(); // read request...

   if (result == U_PLUGIN_HANDLER_AGAIN) U_RETURN(U_NOTIFIER_OK); // NONBLOCKING...
   if (result == U_PLUGIN_HANDLER_ERROR) U_RETURN(U_NOTIFIER_DELETE);

   // init after read

   rstart   = size_request = 0;
   pipeline = false;
   rpointer = rbuffer->data();

loop:
   if (logbuf) logRequest();

   result = UServer_Base::pluginsHandlerREAD(); // check request type...

   if (result == U_PLUGIN_HANDLER_FINISHED)
      {
      // NB: it is possible a resize of the read buffer string...

      if (rpointer != rbuffer->data())
         {
          rpointer  = rbuffer->data();

         if (pipeline) *pbuffer = rbuffer->substr(rstart);
         }

      result = UServer_Base::pluginsHandlerRequest(); // manage request...

      if (result         != U_PLUGIN_HANDLER_FINISHED ||
          handlerWrite() == U_NOTIFIER_DELETE)
         {
         result = U_PLUGIN_HANDLER_ERROR;
         }

      (void) UServer_Base::pluginsHandlerReset(); // manage reset...
      }

   if (UServer_Base::flag_loop == false)
      {
      if (UServer_Base::isParallelization()) U_EXIT(0);

      U_RETURN(U_NOTIFIER_DELETE);
      }

   U_INTERNAL_DUMP("pipeline = %b", pipeline)

   if (result == U_PLUGIN_HANDLER_ERROR &&
       (pipeline              == false  ||
        socket->isConnected() == false))
      {
      U_RETURN(U_NOTIFIER_DELETE);
      }

   if (pipeline)
      {
      // manage pipelining

      U_INTERNAL_ASSERT_POINTER(request)
      U_INTERNAL_ASSERT(request == pbuffer && pbuffer->isNull() == false && pbuffer->same(*rbuffer) == false)

      U_INTERNAL_DUMP("size_request = %u request->size() = %u", size_request, request->size())

      U_ASSERT_EQUALS(size_request, request->size())
      U_ASSERT_EQUALS(rstart, rbuffer->distance(request->data()))

      rstart += size_request;

      U_INTERNAL_DUMP("rstart = %u rbuffer->size() = %u", rstart, rbuffer->size())

      if (rbuffer->size() > rstart)
         {
         // NB: check for spurios white space (IE)...

         if (rbuffer->isWhiteSpace(rstart) == false)
            {
            *pbuffer = rbuffer->substr(rstart);

               body->clear();
            wbuffer->clear();

            goto loop;
            }
         }
      }

   U_RETURN(U_NOTIFIER_OK);
}

int UClientImage_Base::handlerWrite()
{
   U_TRACE(0, "UClientImage_Base::handlerWrite()")

   U_INTERNAL_ASSERT_POINTER(socket)
   U_INTERNAL_ASSERT_EQUALS(pClientImage, this)

   if (UClientImage_Base::write_off)
      {
      // NB: we not send response because we can have used sendfile() etc...

      UClientImage_Base::write_off = false;

      U_RETURN(U_NOTIFIER_OK);
      }

   U_INTERNAL_ASSERT_POINTER(wbuffer)
   U_INTERNAL_ASSERT(socket->isOpen())

   U_INTERNAL_DUMP("wbuffer(%u) = %.*S", wbuffer->size(), U_STRING_TO_TRACE(*wbuffer))
   U_INTERNAL_DUMP("   body(%u) = %.*S",    body->size(), U_STRING_TO_TRACE(*body))

   U_ASSERT_DIFFERS(wbuffer->empty(), true)

   if (logbuf) logResponse();

   int result = (USocketExt::write(socket, *wbuffer, *body, 3 * 1000) ? U_NOTIFIER_OK
                                                                      : U_NOTIFIER_DELETE);

   U_RETURN(result);
}

void UClientImage_Base::handlerDelete()
{
   U_TRACE(0, "UClientImage_Base::handlerDelete()")

   UServer_Base::handlerCloseConnection(this);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UClientImage_Base::dump(bool _reset) const
{
   *UObjectIO::os << "bIPv6                              " << bIPv6                 << '\n'
                  << "write_off                          " << write_off             << '\n'
                  << "body            (UString           " << (void*)body           << ")\n"
                  << "logbuf          (UString           " << (void*)logbuf         << ")\n"
                  << "rbuffer         (UString           " << (void*)rbuffer        << ")\n"
                  << "wbuffer         (UString           " << (void*)wbuffer        << ")\n"
                  << "request         (UString           " << (void*)request        << ")\n"
                  << "pbuffer         (UString           " << (void*)pbuffer        << ")\n"
                  << "msg_welcome     (UString           " << (void*)msg_welcome    << ")\n"
                  << "socket          (USocket           " << (void*)socket         << ")\n"
                  << "clientAddress   (UIPAddress        " << (void*)clientAddress  << ")\n"
                  << "pClientImage    (UClientImage_Base " << (void*)pClientImage   << ')';

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#ifdef HAVE_SSL
const char* UClientImage<USSLSocket>::dump(bool _reset) const
{
   UClientImage_Base::dump(false);

   *UObjectIO::os << '\n'
                  << "ssl                                " << (void*)ssl;

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}
#endif
#endif
