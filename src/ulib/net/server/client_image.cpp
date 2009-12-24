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
   U_ASSERT_DIFFERS(rbuffer->empty(), true)
   U_INTERNAL_ASSERT_POINTER(pClientImage->logbuf)
   U_INTERNAL_ASSERT_EQUALS(UServer_Base::isLog(), true)

   UServer_Base::log->log("%sreceived request (%u bytes) %#.*S from %.*s\n", UServer_Base::mod_name, rbuffer->size(),
                                                                             U_STRING_TO_TRACE(*rbuffer), U_STRING_TO_TRACE(*(pClientImage->logbuf)));

/* Test for GCC == 3.3.3 (SuSE Linux) */
#if GCC_VERSION != 30303 
   if (filereq) (void) UFile::writeToTmpl(filereq, *rbuffer, true);
#endif
}

void UClientImage_Base::logResponse(const char* fileres)
{
   U_TRACE(0+256, "UClientImage_Base::logResponse(%S)", fileres)

   U_INTERNAL_ASSERT_POINTER(wbuffer)
   U_INTERNAL_ASSERT_POINTER(pClientImage)
   U_ASSERT_DIFFERS(wbuffer->empty(), true)
   U_INTERNAL_ASSERT_POINTER(pClientImage->logbuf)
   U_INTERNAL_ASSERT_EQUALS(UServer_Base::isLog(), true)

   UServer_Base::log->log("%ssent response (%u bytes) %#.*S to %.*s\n", UServer_Base::mod_name, wbuffer->size(),
                                                                        U_STRING_TO_TRACE(*wbuffer), U_STRING_TO_TRACE(*(pClientImage->logbuf)));

/* Test for GCC == 3.3.3 (SuSE Linux) */
#if GCC_VERSION != 30303 
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

#ifdef HAVE_LIBEVENT
   pevent = 0;
#endif

   pClientImage = this;

   if (UServer_Base::isLog() == false) logbuf = 0;
   else
      {
      logbuf = U_NEW(UString(U_CAPACITY));

      socket->getRemoteInfo(*logbuf);
      }

   clientAddress = socket->remoteIPAddress();
}

void UClientImage_Base::destroy()
{
   U_TRACE(0, "UClientImage_Base::destroy()")

   U_INTERNAL_ASSERT_POINTER(socket)
   U_INTERNAL_ASSERT_POINTER(pClientImage)

   UServer_Base::num_connection--;

   U_INTERNAL_DUMP("num_connection = %d", UServer_Base::num_connection)

   if (UServer_Base::isLog())
      {
      UServer_Base::log->log("client closed connection from %.*s, %u clients still connected\n",
                              U_STRING_TO_TRACE(*(pClientImage->logbuf)), UServer_Base::num_connection);

      U_INTERNAL_DUMP("fd = %d", pClientImage->UEventFd::fd)

      U_ASSERT(pClientImage->UEventFd::fd == pClientImage->logbuf->strtol())
      }

#ifdef DEBUG
   if (pClientImage->logbuf) delete pClientImage->logbuf;
#else
   if (UServer_Base::isClassic()) ::exit(0);
#endif

   if (socket->isOpen())
      {
      // NB: we need this because we reuse the same object USocket...

      socket->iSockDesc = pClientImage->UEventFd::fd;

      socket->closesocket();
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
      U_INTERNAL_ASSERT_POINTER(pClientImage->logbuf)
      U_INTERNAL_ASSERT_EQUALS(UServer_Base::isLog(), true)

      UCertificate::setForLog((X509*)x509, *(pClientImage->logbuf));

      U_INTERNAL_DUMP("logbuf = %.*S", U_STRING_TO_TRACE(*(pClientImage->logbuf)))
      }
#endif
}

void UClientImage_Base::genericReset()
{
   U_TRACE(0, "UClientImage_Base::genericReset()")

   U_INTERNAL_ASSERT_POINTER(rbuffer)
   U_INTERNAL_ASSERT_POINTER(wbuffer)
   U_INTERNAL_ASSERT_EQUALS(rbuffer->isNull(), false);

   if      (wbuffer->same(*rbuffer)) wbuffer->clear();
   else if (wbuffer->uniq())         wbuffer->setEmpty();

   if (body->isNull() == false) body->clear();

   // NB: we force for U_SUBSTR_INC_REF case (string can be referenced more)...

   rbuffer->setEmptyForce();

   // NB: we need this because we reuse the same object USocket...

   socket->iState    = USocket::CONNECT;
   socket->iSockDesc = pClientImage->UEventFd::fd;

   U_INTERNAL_DUMP("fd = %d", pClientImage->UEventFd::fd)
}

const char* UClientImage_Base::getRemoteInfo(uint32_t* plen)
{
   U_TRACE(0, "UClientImage_Base::getRemoteInfo(%p)", plen)

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

   ptr = pClientImage->clientAddress.getAddressString();

   if (plen) *plen = strlen(ptr);

end:
   U_RETURN_POINTER(ptr, const char);
}

// check if read data already available... (pipelining)

bool UClientImage_Base::isPipeline()
{
   U_TRACE(0, "UClientImage_Base::isPipeline()")

   U_INTERNAL_ASSERT_POINTER(rbuffer)

   uint32_t size = rbuffer->size();

   U_INTERNAL_DUMP("rbuffer->size() = %u size_message = %u pcount = %d pbuffer = %p",
                     size, USocketExt::size_message, USocketExt::pcount, USocketExt::pbuffer)

   if ((int32_t)USocketExt::size_message  > 0 && size &&
       (USocketExt::pcount                > 0 || size > USocketExt::size_message))
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

         U_RETURN(true);
         }
      }

   USocketExt::pcount = 0; // reset read data available...

   U_RETURN(false);
}

// check if close connection... (read() == 0)

bool UClientImage_Base::isClose()
{
   U_TRACE(0, "UClientImage_Base::isClose()")

   U_INTERNAL_ASSERT_POINTER(socket)
   U_INTERNAL_ASSERT_POINTER(pClientImage)

   U_INTERNAL_DUMP("timeout = %b", socket->isTimeout())

   if (socket->isTimeout()) U_SRV_LOG_TIMEOUT(pClientImage);

   if (rbuffer->empty() || socket->isConnected() == false) U_RETURN(true);

   U_RETURN(false);
}

int UClientImage_Base::genericHandlerRead()
{
   U_TRACE(0, "UClientImage_Base::genericHandlerRead()")

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
#  endif

      if (result == U_PLUGIN_HANDLER_ERROR)
         {
         USocketExt::pcount = 0; // reset read data available...

         U_RETURN(U_NOTIFIER_DELETE); // return false at method UClientImage_Base::run()...
         }
      }
   while (isPipeline());

   U_RETURN(U_NOTIFIER_OK);
}

void UClientImage_Base::run()
{
   U_TRACE(0, "UClientImage_Base::run()")

   U_INTERNAL_ASSERT_POINTER(socket)
   U_INTERNAL_ASSERT_POINTER(pClientImage)

   if (UServer_Base::isLog())
      {
#  ifdef HAVE_SSL
      if (socket->isSSL()) ((UClientImage<USSLSocket>*)pClientImage)->logCertificate();
#  endif

      UServer_Base::log->log("new client connected from %.*s, %u clients currently connected\n",
                                 U_STRING_TO_TRACE(*(pClientImage->logbuf)), UServer_Base::num_connection);
      }

   if (msg_welcome)
      {
      U_SRV_LOG_MSG_WITH_ADDR("sent welcome message to");

      if (USocketExt::write(socket, *msg_welcome) == false) goto dtor;
      }

   pClientImage->UEventFd::fd      = socket->iSockDesc;
   pClientImage->UEventFd::op_mask = R_OK; // W_OK == 2, R_OK == 4...

   if (pClientImage->handlerRead() != U_NOTIFIER_DELETE)
      {
#  ifndef HAVE_LIBEVENT
      UNotifier::insert(pClientImage); // NB: this new object (pClientImage) is deleted by UNotifier (when response U_NOTIFIER_DELETE from handlerRead()...)
#  else
      pClientImage->delEvent();

      pClientImage->pevent = U_NEW(UEvent<UClientImage_Base>(pClientImage->UEventFd::fd, EV_READ | EV_PERSIST, *pClientImage));

      UDispatcher::add(*pevent);
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

int UClientImage_Base::genericHandlerWrite()
{
   U_TRACE(0, "UClientImage_Base::genericHandlerWrite()")

   U_INTERNAL_ASSERT_POINTER(socket)

   // NB: we not send response because we can have used sendfile() etc...

   if (UClientImage_Base::write_off)
      {
      UClientImage_Base::write_off = false;

      U_RETURN(U_NOTIFIER_OK);
      }

   // check for error...

   if (socket->isConnected() == false) U_RETURN(U_NOTIFIER_DELETE);

   U_INTERNAL_ASSERT_POINTER(wbuffer)

   uint32_t size = wbuffer->size();

   U_INTERNAL_DUMP("wbuffer(%u) = %#.*S", size, U_STRING_TO_TRACE(*wbuffer));

   if (size == 0)
      {
      U_INTERNAL_DUMP("size_message = %u", USocketExt::size_message);

      U_INTERNAL_ASSERT_MAJOR((int32_t)USocketExt::size_message,0)

      *wbuffer = rbuffer->substr(0U, (size = USocketExt::size_message));
      }

   if (UServer_Base::isLog()) logResponse();

   int result = (USocketExt::write(socket, wbuffer->data(), size) ? U_NOTIFIER_OK
                                                                  : U_NOTIFIER_DELETE);

   U_RETURN(result);
}

#ifdef HAVE_LIBEVENT
void UClientImage_Base::delEvent()
{
   U_TRACE(0, "UClientImage_Base::delEvent()")

   if (pevent)
      {
      UDispatcher::del(pevent);

      delete pevent;
             pevent = 0;
      }
}

void UClientImage_Base::operator()(int fd, short event)
{
   U_TRACE(0, "UClientImage_Base::operator()(%d,%hd)", fd, event)

   U_INTERNAL_ASSERT_EQUALS(event, EV_READ)

   if (handlerRead() == U_NOTIFIER_DELETE)
      {
      delEvent();

      delete this; // as UNotifier do...
      }
}
#endif

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

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UClientImage_Base::dump(bool reset) const
{
   *UObjectIO::os << "bIPv6                             " << bIPv6              << '\n'
                  << "body            (UString          " << (void*)body        << ")\n"
                  << "logbuf          (UString          " << (void*)logbuf      << ")\n"
                  << "rbuffer         (UString          " << (void*)rbuffer     << ")\n"
                  << "wbuffer         (UString          " << (void*)wbuffer     << ")\n"
                  << "msg_welcome     (UString          " << (void*)msg_welcome << ")\n"
                  << "socket          (USocket          " << (void*)socket      << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
