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

#ifndef EPOLLET
#define EPOLLET 0
#endif

bool               UClientImage_Base::bIPv6;
bool               UClientImage_Base::write_off;  // NB: we not send response because we can have used sendfile() etc...
USocket*           UClientImage_Base::socket;
UString*           UClientImage_Base::body;
UString*           UClientImage_Base::rbuffer;
UString*           UClientImage_Base::wbuffer;
UString*           UClientImage_Base::real_ip;
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

   UServer_Base::log->log("%sreceived request (%u bytes) %.*S from %.*s\n", UServer_Base::mod_name, rbuffer->size(),
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

   U_INTERNAL_ASSERT_EQUALS(body,     0);
   U_INTERNAL_ASSERT_EQUALS(socket,   0);
   U_INTERNAL_ASSERT_EQUALS(rbuffer,  0);
   U_INTERNAL_ASSERT_EQUALS(wbuffer,  0);
   U_INTERNAL_ASSERT_EQUALS(_value,   0);
   U_INTERNAL_ASSERT_EQUALS(_buffer,  0);
   U_INTERNAL_ASSERT_EQUALS(_encoded, 0);

   socket  = p;
   body    = U_NEW(UString);
   rbuffer = U_NEW(UString(U_CAPACITY));
   wbuffer = U_NEW(UString);
   real_ip = U_NEW(UString(20U));

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
   delete real_ip;

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

   U_INTERNAL_ASSERT_POINTER(socket)

   pClientImage      = this;
   UEventFd::fd      = socket->iSockDesc;
   UEventFd::op_mask = U_READ_IN | EPOLLET;

   if (UServer_Base::isLog() == false) logbuf = 0;
   else
      {
      logbuf        = U_NEW(UString(4000U));
      clientAddress = U_NEW(UIPAddress(socket->cRemoteAddress));

      socket->getRemoteInfo(*logbuf);
      }
}

void UClientImage_Base::destroy()
{
   U_TRACE(0, "UClientImage_Base::destroy()")

   U_INTERNAL_ASSERT_POINTER(socket)
   U_INTERNAL_ASSERT_POINTER(pClientImage)

   UServer_Base::handlerCloseConnection();

   if (socket->isOpen()) socket->closesocket();

   if (UServer_Base::isClassic()) U_EXIT(0);

   if (pClientImage->logbuf)
      {
      U_ASSERT_EQUALS(pClientImage->UEventFd::fd, pClientImage->logbuf->strtol())

      delete pClientImage->logbuf;
      delete pClientImage->clientAddress;
      }
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

bool UClientImage_Base::setRealIP()
{
   U_TRACE(0, "UClientImage_Base::setRealIP()")

   U_ASSERT_DIFFERS(rbuffer->empty(), true)

   // check for X-Forwarded-For: client, proxy1, proxy2 and X-Real-IP: ...

   uint32_t    ip_client_len = 0;
   const char* ip_client     = UHTTP::getHTTPHeaderValuePtr(*USocket::str_X_Forwarded_For);

   if (ip_client)
      {
      char c;
len:
      while (true)
         {
         c = ip_client[ip_client_len];

         if (u_isspace(c) || c == ',') break;

         ++ip_client_len;
         }

      U_INTERNAL_DUMP("ip_client = %.*S", ip_client_len, ip_client)

      (void) real_ip->replace(ip_client, ip_client_len);

      U_RETURN(true);
      }

   ip_client = UHTTP::getHTTPHeaderValuePtr(*USocket::str_X_Real_IP);

   if (ip_client) goto len;

   U_RETURN(false);
}

void UClientImage_Base::resetBuffer()
{
   U_TRACE(0, "UClientImage_Base::resetBuffer()")

   U_INTERNAL_ASSERT_POINTER(body);
   U_INTERNAL_ASSERT_POINTER(rbuffer)
   U_INTERNAL_ASSERT_POINTER(wbuffer)

      body->clear();
   wbuffer->clear();

   U_INTERNAL_ASSERT_EQUALS(rbuffer->isNull(), false);
   U_ASSERT(                rbuffer->writeable())

   rbuffer->setEmptyForce(); // NB: we force for U_SUBSTR_INC_REF case (string can be referenced more than one)...

   if (real_ip->empty() == false) real_ip->setEmpty();

   // manage buffered read (pipelining)

   USocketExt::size_message = 0;

   U_INTERNAL_DUMP("USocketExt::size_message = %u", USocketExt::size_message)
}

// check if read data already available... (pipelining)

bool UClientImage_Base::isPipeline()
{
   U_TRACE(0, "UClientImage_Base::isPipeline()")

   if (checkForPipeline())
      {
      // NB: for RPC message...

      if (rbuffer->size() == USocketExt::size_message)
         {
         // NB: we force for U_SUBSTR_INC_REF case (string can be referenced more than one)...

         rbuffer->size_adjust_force(USocketExt::size_message + USocketExt::pcount);
         }

      // NB: check for spurios white space (IE)...

      if (rbuffer->isWhiteSpace(USocketExt::size_message) == false)
         {
         rbuffer->moveToBeginDataInBuffer(USocketExt::size_message);

         U_INTERNAL_DUMP("rbuffer = %.*S", U_STRING_TO_TRACE(*rbuffer))

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
      U_SRV_LOG_WITH_ADDR("sent welcome message to");

      if (USocketExt::write(socket, *msg_welcome) == false) goto dtor;
      }

   if (pClientImage->handlerRead() != U_NOTIFIER_DELETE)
      {
      // NB: this new object (pClientImage) is deleted by UNotifier (when response U_NOTIFIER_DELETE from handlerRead()...)

      UNotifier::insert(pClientImage);

      return;
      }

   // NB: if server with no prefork (ex: nodog) process the HTTP CGI request with fork....

   U_INTERNAL_DUMP("UClientImage_Base::run() flag_loop = %b", UServer_Base::flag_loop)

   if (UServer_Base::flag_loop          == false &&
       UServer_Base::preforked_num_kids == 0     &&
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
      // Connection-wide hooks
      result = UServer_Base::pluginsHandlerREAD(); // read request...

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

      U_INTERNAL_DUMP("flag_loop = %b", UServer_Base::flag_loop)

      if (UServer_Base::flag_loop == false  ||
          (result == U_PLUGIN_HANDLER_ERROR &&
           (socket->isConnected() == false  ||
            checkForPipeline()    == false)))
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

   U_INTERNAL_DUMP("wbuffer(%u) = %.*S", wbuffer->size(), U_STRING_TO_TRACE(*wbuffer))
   U_INTERNAL_DUMP("   body(%u) = %.*S",    body->size(), U_STRING_TO_TRACE(*body))

   U_ASSERT_DIFFERS(wbuffer->empty(), true)

   if (UServer_Base::isLog()) logResponse();

   int result = (USocketExt::write(socket, *wbuffer, *body, 3 * 1000) ? U_NOTIFIER_OK
                                                                      : U_NOTIFIER_DELETE);

   U_RETURN(result);
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
