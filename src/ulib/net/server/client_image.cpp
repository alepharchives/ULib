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
bool               UClientImage_Base::write_off;  // NB: we not send response because we can have used sendfile() etc...
USocket*           UClientImage_Base::socket;
UString*           UClientImage_Base::body;
UString*           UClientImage_Base::rbuffer;
UString*           UClientImage_Base::wbuffer;
UString*           UClientImage_Base::msg_welcome;
UClientImage_Base* UClientImage_Base::pClientImage;

#undef  GCC_VERSION
#define GCC_VERSION (__GNUC__       * 10000 + \
                     __GNUC_MINOR__ *   100 + \
                     __GNUC_PATCHLEVEL__)

// NB: these are for ULib Servlet Page (USP) - U_DYNAMIC_PAGE_OUTPUT...

UString* UClientImage_Base::_value;
UString* UClientImage_Base::_buffer;
UString* UClientImage_Base::_encoded;

// NB: we cannot put this in .h for the dependency of UServer_Base class...
// ------------------------------------------------------------------------
void UClientImage_Base::logRequest(const char* filereq)
{
   U_TRACE(0+256, "UClientImage_Base::logRequest(%S)", filereq)

   U_INTERNAL_ASSERT_POINTER(rbuffer)
   U_INTERNAL_ASSERT_POINTER(pClientImage)
   U_INTERNAL_ASSERT(UServer_Base::isLog())
   U_ASSERT_EQUALS(rbuffer->empty(), false)
   U_INTERNAL_ASSERT_POINTER(pClientImage->logbuf)

   uint32_t u_printf_string_max_length_save = u_printf_string_max_length;

   U_INTERNAL_DUMP("u_printf_string_max_length = %d UHTTP::http_info.endHeader = %u", u_printf_string_max_length, UHTTP::http_info.endHeader)

   if (u_printf_string_max_length == -1)
      {
      if (UHTTP::isHTTPRequest()) // NB: only HTTP header...
         {
         u_printf_string_max_length = (UHTTP::http_info.endHeader ? UHTTP::http_info.endHeader : rbuffer->size()); 

         U_INTERNAL_ASSERT_MAJOR(u_printf_string_max_length, 0)
         }
      }

   UServer_Base::log->log("%sreceived request (%u bytes) %#.*S from %.*s\n", UServer_Base::mod_name, rbuffer->size(),
                                                                             U_STRING_TO_TRACE(*rbuffer), U_STRING_TO_TRACE(*(pClientImage->logbuf)));

   u_printf_string_max_length = u_printf_string_max_length_save;

#if GCC_VERSION != 30303 /* Test for GCC == 3.3.3 (SuSE Linux) */
   if (filereq) (void) UFile::writeToTmpl(filereq, *rbuffer, true);
#endif
}

void UClientImage_Base::logResponse(const char* fileres)
{
   U_TRACE(0+256, "UClientImage_Base::logResponse(%S)", fileres)

   U_INTERNAL_ASSERT_POINTER(wbuffer)
   U_INTERNAL_ASSERT_POINTER(pClientImage)
   U_INTERNAL_ASSERT(UServer_Base::isLog())
   U_ASSERT_EQUALS(wbuffer->empty(), false)
   U_INTERNAL_ASSERT_POINTER(pClientImage->logbuf)

   U_INTERNAL_DUMP("u_printf_string_max_length = %d", u_printf_string_max_length)

   uint32_t u_printf_string_max_length_save = u_printf_string_max_length;

   if (u_printf_string_max_length == -1)
      {
      if (UHTTP::isHTTPRequest()) // NB: only HTTP header...
         {
         u_printf_string_max_length = u_findEndHeader(U_STRING_TO_PARAM(*wbuffer));

         U_INTERNAL_ASSERT_MAJOR(u_printf_string_max_length, 0)
         }
      }

   UServer_Base::log->log("%ssent response (%u bytes) %#.*S to %.*s\n", UServer_Base::mod_name, wbuffer->size(),
                                                                        U_STRING_TO_TRACE(*wbuffer), U_STRING_TO_TRACE(*(pClientImage->logbuf)));

   u_printf_string_max_length = u_printf_string_max_length_save;

#if GCC_VERSION != 30303 /* Test for GCC == 3.3.3 (SuSE Linux) */
   if (fileres) (void) UFile::writeToTmpl(fileres, *wbuffer, true);
#endif
}
// ------------------------------------------------------------------------

void UClientImage_Base::init(USocket* p)
{
   U_TRACE(0, "UClientImage_Base::init(%p)", p)

   U_INTERNAL_ASSERT_EQUALS(body, 0);
   U_INTERNAL_ASSERT_EQUALS(socket, 0);
   U_INTERNAL_ASSERT_EQUALS(rbuffer, 0);
   U_INTERNAL_ASSERT_EQUALS(wbuffer, 0);
   U_INTERNAL_ASSERT_EQUALS(_value, 0);
   U_INTERNAL_ASSERT_EQUALS(_buffer, 0);
   U_INTERNAL_ASSERT_EQUALS(_encoded, 0);

   socket  = p;
   body    = U_NEW(UString);
   rbuffer = U_NEW(UString(U_CAPACITY));
   wbuffer = U_NEW(UString(U_CAPACITY));

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
   U_INTERNAL_ASSERT_POINTER(_value);
   U_INTERNAL_ASSERT_POINTER(_buffer);
   U_INTERNAL_ASSERT_POINTER(_encoded);

   if (msg_welcome) delete msg_welcome;

   delete body;
   delete wbuffer;
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
   U_TRACE_REGISTER_OBJECT(0, UClientImage_Base, "", 0)

   U_INTERNAL_ASSERT_POINTER(socket)

   pClientImage      = this;
   UEventFd::fd      = socket->iSockDesc;
   UEventFd::op_mask = U_READ_IN;

   if (UServer_Base::isLog() == false) logbuf = 0;
   else
      {
      logbuf        = U_NEW(UString(U_CAPACITY));
      clientAddress = U_NEW(UIPAddress(socket->cRemoteAddress));

      socket->getRemoteInfo(*logbuf);
      }

#ifdef HAVE_LIBEVENT
   pevent = 0;
#endif
}

void UClientImage_Base::destroy()
{
   U_TRACE(0, "UClientImage_Base::destroy()")

   U_INTERNAL_ASSERT_POINTER(socket)
   U_INTERNAL_ASSERT_POINTER(pClientImage)

   UServer_Base::handlerCloseConnection();

#ifdef HAVE_LIBEVENT
   if (pClientImage->pevent)
      {
      UNotifier::erase(pClientImage, false);

      (void) UDispatcher::del(pClientImage->pevent);
                       delete pClientImage->pevent;
      }
#endif

#ifdef DEBUG
   if (pClientImage->logbuf)
      {
      U_ASSERT_EQUALS(pClientImage->UEventFd::fd, pClientImage->logbuf->strtol())

      delete pClientImage->logbuf;
      delete pClientImage->clientAddress;
      }
#else
   if (UServer_Base::isClassic()) ::exit(0);
#endif

   // socket->~USocket();

   if (socket->isOpen()) socket->closesocket();
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

   U_INTERNAL_ASSERT_POINTER(pClientImage)

   // NB: OpenSSL already tested the cert validity during SSL handshake and returns a X509 ptr just if the certificate is valid...

#ifdef HAVE_SSL
   if (x509)
      {
      U_INTERNAL_ASSERT(UServer_Base::isLog())
      U_INTERNAL_ASSERT_POINTER(pClientImage->logbuf)

      UCertificate::setForLog((X509*)x509, *(pClientImage->logbuf));

      U_INTERNAL_DUMP("logbuf = %.*S", U_STRING_TO_TRACE(*(pClientImage->logbuf)))
      }
#endif
}

const char* UClientImage_Base::getRemoteInfo(uint32_t* plen)
{
   U_TRACE(0, "UClientImage_Base::getRemoteInfo(%p)", plen)

   U_INTERNAL_ASSERT_POINTER(socket)
   U_INTERNAL_ASSERT_POINTER(pClientImage)

   const char* ptr;

   if (plen)
      {
      // check for X-Forwarded-For: client1, proxy1, proxy2

      ptr = UHTTP::getHTTPHeaderValuePtr(*USocket::str_X_Forwarded_For);

      if (ptr)
         {
         char c;
         uint32_t len = 0;

         while (true)
            {
            c = ptr[len];

            if (u_isspace(c) || c == ',') break;

            ++len;
            }

         U_INTERNAL_DUMP("X-Forwarded-For = %.*S", len, ptr)

         *plen = len;

         goto end;
         }
      }

   if (pClientImage->logbuf) ptr = pClientImage->clientAddress->getAddressString();
   else
      {
      socket->setRemote();

      ptr = socket->cRemoteAddress.getAddressString();
      }

   if (plen) *plen = strlen(ptr);

end:
   U_RETURN_POINTER(ptr, const char);
}

void UClientImage_Base::resetBuffer()
{
   U_TRACE(0, "UClientImage_Base::resetBuffer()")

   U_INTERNAL_ASSERT_POINTER(rbuffer)
   U_INTERNAL_ASSERT_POINTER(wbuffer)
   U_INTERNAL_ASSERT_EQUALS(rbuffer->isNull(), false);

   if      (wbuffer->same(*rbuffer)) wbuffer->clear();
   else if (wbuffer->uniq())         wbuffer->setEmpty();

   if (body->isNull() == false) body->clear();

   // NB: we force for U_SUBSTR_INC_REF case (string can be referenced more)...

   rbuffer->setEmptyForce();
}

// check if read data already available... (pipelining)

bool UClientImage_Base::isPipeline()
{
   U_TRACE(0, "UClientImage_Base::isPipeline()")

   U_INTERNAL_ASSERT_POINTER(rbuffer)

   uint32_t size = rbuffer->size();

   if (isPipeline(size))
      {
      // NB: for RPC message...

      bool bsize = (size == USocketExt::size_message);

      // NB: we force for U_SUBSTR_INC_REF case (string can be referenced more than one)...

      if (bsize) rbuffer->size_adjust_force(USocketExt::size_message + USocketExt::pcount);

      // NB: check for spurios white space (IE)...

      if (rbuffer->isWhiteSpace(USocketExt::size_message) == false)
         {
         rbuffer->moveToBeginDataInBuffer(USocketExt::size_message);

         U_INTERNAL_DUMP("rbuffer = %.*S", U_STRING_TO_TRACE(*rbuffer));

         resetBuffer();

         U_RETURN(true);
         }
      }

   USocketExt::pcount = 0; // reset read data available...

   U_RETURN(false);
}

void UClientImage_Base::run()
{
   U_TRACE(0, "UClientImage_Base::run()")

   U_INTERNAL_ASSERT_POINTER(socket)
   U_INTERNAL_ASSERT_POINTER(pClientImage)

   if (msg_welcome)
      {
      U_SRV_LOG_MSG_WITH_ADDR("sent welcome message to");

      if (USocketExt::write(socket, *msg_welcome) == false) goto dtor;
      }

   if (pClientImage->handlerRead() != U_NOTIFIER_DELETE)
      {
      UNotifier::insert(pClientImage); // NB: this new object (pClientImage) is deleted by UNotifier (when response U_NOTIFIER_DELETE from handlerRead()...)

#  ifdef HAVE_LIBEVENT
      pClientImage->pevent = U_NEW(UEvent<UClientImage_Base>(pClientImage->UEventFd::fd, EV_READ | EV_PERSIST, *pClientImage));

      UDispatcher::add(*(pClientImage->pevent));
#  endif

      return;
      }

   // NB: if server with no prefork (ex: nodog) process the HTTP CGI request with fork....

   if (UServer_Base::preforked_num_kids == 0     &&
       UServer_Base::flag_loop          == false &&
       UServer_Base::proc->child())
      {
      U_EXIT(0);
      }

dtor:
   delete pClientImage;
}

// define method VIRTUAL of class UClientImage_Base

void UClientImage_Base::genericReset()
{
   U_TRACE(0, "UClientImage_Base::genericReset()")

   resetBuffer();

   // manage buffered read (pipelining)

   USocketExt::size_message = 0;

   // NB: we need this because we reuse the same object UClientImage_Base...

   U_INTERNAL_ASSERT_POINTER(pClientImage)

   pClientImage->resetSocket(USocket::CONNECT);
}

// define method VIRTUAL of class UEventFd

int UClientImage_Base::handlerRead()
{
   U_TRACE(0, "UClientImage_Base::handlerRead()")

   reset(); // virtual method

   U_INTERNAL_ASSERT_POINTER(pClientImage)
   U_INTERNAL_ASSERT_EQUALS(socket->iSockDesc, pClientImage->UEventFd::fd)

   int result = U_PLUGIN_HANDLER_ERROR;

   do {
#  ifdef HAVE_MODULES // Connection-wide hooks
      result = UServer_Base::pluginsHandlerRead(); // read request...

      if (result == U_PLUGIN_HANDLER_FINISHED)
         {
         result = UServer_Base::pluginsHandlerRequest(); // manage request...

         if (result                       != U_PLUGIN_HANDLER_FINISHED ||
             pClientImage->handlerWrite() == U_NOTIFIER_DELETE)
            {
            result = U_PLUGIN_HANDLER_ERROR;
            }

         (void) UServer_Base::pluginsHandlerReset(); // manage reset...
         }

      if (result == U_PLUGIN_HANDLER_AGAIN) U_RETURN(U_NOTIFIER_OK); // NONBLOCKING...
#  endif

      if (result == U_PLUGIN_HANDLER_ERROR)
         {
         USocketExt::pcount = 0; // reset read data available...

         U_RETURN(U_NOTIFIER_DELETE);
         }
      }
   while (isPipeline());

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

   U_INTERNAL_DUMP("wbuffer(%u) = %#.*S", wbuffer->size(), U_STRING_TO_TRACE(*wbuffer));

   U_ASSERT_DIFFERS(wbuffer->empty(), true)

   if (UServer_Base::isLog()) logResponse();

   int result = (USocketExt::write(socket, *wbuffer) ? U_NOTIFIER_OK
                                                     : U_NOTIFIER_DELETE);

   U_RETURN(result);
}

#ifdef HAVE_LIBEVENT
void UClientImage_Base::operator()(int fd, short event)
{
   U_TRACE(0, "UClientImage_Base::operator()(%d,%hd)", fd, event)

   U_INTERNAL_ASSERT_EQUALS(event, EV_READ)

   if (handlerRead() == U_NOTIFIER_DELETE) delete this; // same as UNotifier do...
}
#endif

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UClientImage_Base::dump(bool reset) const
{
   *UObjectIO::os << "bIPv6                              " << bIPv6                 << '\n'
                  << "write_off                          " << write_off             << '\n'
                  << "body            (UString           " << (void*)body           << ")\n"
                  << "logbuf          (UString           " << (void*)logbuf         << ")\n"
                  << "rbuffer         (UString           " << (void*)rbuffer        << ")\n"
                  << "wbuffer         (UString           " << (void*)wbuffer        << ")\n"
                  << "msg_welcome     (UString           " << (void*)msg_welcome    << ")\n"
                  << "socket          (USocket           " << (void*)socket         << ")\n"
                  << "clientAddress   (UIPAddress        " << (void*)clientAddress  << ")\n"
                  << "pClientImage    (UClientImage_Base " << (void*)pClientImage   << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#ifdef HAVE_SSL

const char* UClientImage<USSLSocket>::dump(bool reset) const
{
   UClientImage_Base::dump(false);

   *UObjectIO::os << '\n'
                  << "ssl                                " << (void*)ssl;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
#endif
