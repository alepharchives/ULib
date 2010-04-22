// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_http.cpp - this is a plugin http for userver
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/url.h>
#include <ulib/file_config.h>
#include <ulib/mime/entity.h>
#include <ulib/utility/uhttp.h>
#include <ulib/plugin/mod_http.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/string_ext.h>

U_CREAT_FUNC(UHttpPlugIn)

UString* UHttpPlugIn::str_URI_PROTECTED_MASK;
UString* UHttpPlugIn::str_URI_REQUEST_CERT_MASK;
UString* UHttpPlugIn::str_URI_PROTECTED_ALLOWED_IP;

void UHttpPlugIn::str_allocate()
{
   U_TRACE(0, "UHttpPlugIn::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_URI_PROTECTED_MASK,0)
   U_INTERNAL_ASSERT_EQUALS(str_URI_REQUEST_CERT_MASK,0)
   U_INTERNAL_ASSERT_EQUALS(str_URI_PROTECTED_ALLOWED_IP,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("URI_PROTECTED_MASK") },
      { U_STRINGREP_FROM_CONSTANT("URI_REQUEST_CERT_MASK") },
      { U_STRINGREP_FROM_CONSTANT("URI_PROTECTED_ALLOWED_IP") }
   };

   U_NEW_ULIB_OBJECT(str_URI_PROTECTED_MASK,       U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_URI_REQUEST_CERT_MASK,    U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_URI_PROTECTED_ALLOWED_IP, U_STRING_FROM_STRINGREP_STORAGE(2));
}

UHttpPlugIn::~UHttpPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, UHttpPlugIn)

   if (UHTTP::file)
      {
      delete UHTTP::file;
      delete UHTTP::alias;
      delete UHTTP::tmpdir;
      delete UHTTP::qcontent;
      delete UHTTP::formMulti;
      delete UHTTP::penvironment;
      delete UHTTP::form_name_value;
      }

   if (UHTTP::vallow_IP)   delete UHTTP::vallow_IP;
   if (UHTTP::request_uri) delete UHTTP::request_uri;
}

// Server-wide hooks

int UHttpPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UHttpPlugIn::handlerConfig(%p)", &cfg)

   // ------------------------------------------------------------------------------------------------------------------------------------------------
   // ALIAS                         vector of URI redirection (request -> alias)
   //
   // VIRTUAL_HOST                  flag to activate practice of maintaining more than one server on one machine,
   //                               as differentiated by their apparent hostname
   // DIGEST_AUTHENTICATION         flag authentication method (yes = digest, no = basic)
   //
   // URI_PROTECTED_MASK            mask (DOS regexp) of URI protected from prying eyes
   // URI_PROTECTED_ALLOWED_IP      list of comma separated client address for IP-based access control (IPADDR[/MASK]) for URI_PROTECTED_MASK
   //
   // URI_REQUEST_CERT_MASK         mask (DOS regexp) of URI where client must comunicate a certificate in the SSL connection
   // ------------------------------------------------------------------------------------------------------------------------------------------------

   (void) cfg.loadVector(valias);

   if (cfg.loadTable())
      {
      UHTTP::virtual_host                 = cfg.readBoolean(*UServer_Base::str_VIRTUAL_HOST);
      UServer_Base::digest_authentication = cfg.readBoolean(*UServer_Base::str_DIGEST_AUTHENTICATION);

      uri_protected_mask                  = cfg[*str_URI_PROTECTED_MASK];
      uri_protected_allowed_ip            = cfg[*str_URI_PROTECTED_ALLOWED_IP];

#  ifdef HAVE_SSL
      uri_request_cert_mask               = cfg[*str_URI_REQUEST_CERT_MASK];
#  endif
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UHttpPlugIn::handlerInit()
{
   U_TRACE(0, "UHttpPlugIn::handlerInit()")

   // init global var...

   UHTTP::str_allocate();

   U_INTERNAL_ASSERT_EQUALS(UHTTP::file,0)

   UHTTP::file = U_NEW(UFile);

   U_INTERNAL_ASSERT_EQUALS(UHTTP::penvironment,0)

   UHTTP::alias        = U_NEW(UString(U_CAPACITY));
   UHTTP::penvironment = U_NEW(UString(U_CAPACITY));

   U_INTERNAL_ASSERT_POINTER(USocket::str_host)
   U_INTERNAL_ASSERT_POINTER(USocket::str_connection)

   UHTTP::ptrH = USocket::str_host->c_pointer(1);           // "Host"
   UHTTP::ptrC = USocket::str_connection->c_pointer(1);     // "Connection"
   UHTTP::ptrT = USocket::str_content_type->c_pointer(1);   // "Content-Type"

   // init form processing var...

   U_INTERNAL_ASSERT_EQUALS(UHTTP::tmpdir,0)
   U_INTERNAL_ASSERT_EQUALS(UHTTP::qcontent,0)
   U_INTERNAL_ASSERT_EQUALS(UHTTP::formMulti,0)
   U_INTERNAL_ASSERT_EQUALS(UHTTP::form_name_value,0)

   UHTTP::tmpdir          = U_NEW(UString(100U));
   UHTTP::qcontent        = U_NEW(UString);
   UHTTP::formMulti       = U_NEW(UMimeMultipart);
   UHTTP::form_name_value = U_NEW(UVector<UString>);

   if (        Url::str_ftp  == 0)         Url::str_allocate();
   if (UMimeHeader::str_name == 0) UMimeHeader::str_allocate();

   if (uri_protected_allowed_ip.empty() == false)
      {
      UHTTP::vallow_IP = U_NEW(UVector<UIPAllow*>);

      if (UIPAllow::parseMask(uri_protected_allowed_ip, *UHTTP::vallow_IP) == 0)
         {
         delete UHTTP::vallow_IP;
                UHTTP::vallow_IP = 0;
         }
      }

   if (valias.empty() == false) UHTTP::request_uri = U_NEW(UString);

   U_SRV_LOG_MSG("initialization of plugin success");

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int UHttpPlugIn::handlerRead()
{
   U_TRACE(0, "UHttpPlugIn::handlerRead()")

   U_INTERNAL_ASSERT_POINTER(UClientImage_Base::socket)

   bool is_http_req = UHTTP::readHTTPRequest();

   // check if close connection... (read() == 0)

   if (UClientImage_Base::socket->isClosed()) U_RETURN(U_PLUGIN_HANDLER_ERROR);
   if (UClientImage_Base::rbuffer->empty())   U_RETURN(U_PLUGIN_HANDLER_AGAIN);

   if (UServer_Base::isLog()) UClientImage_Base::logRequest();

   uint32_t host_end;
   const char* content_type = 0;
   int nResponseCode = HTTP_NOT_IMPLEMENTED;

   if (is_http_req)
      {
      UHTTP::getTimeIfNeeded(false);

      // HTTP 1.1 want header "Host: ..."

      if (UHTTP::http_info.version  == 1 &&
          UHTTP::http_info.host_len == 0)
         {
         UHTTP::setHTTPBadRequest();

         goto send_response;
         }

      U_INTERNAL_DUMP("UHTTP::http_info.version = %d", UHTTP::http_info.version)

      if (UHTTP::virtual_host)
         {
         // manage virtual host

         UString host(U_HTTP_HOST_TO_PARAM);

         // Host: hostname[:port]

         host_end = host.find(':');

         if (host_end != U_NOT_FOUND) host.size_adjust(host_end);

         U_INTERNAL_DUMP("host = %.*S", U_STRING_TO_TRACE(host))

         UHTTP::alias->snprintf("/%.*s%.*s", U_STRING_TO_TRACE(host), U_HTTP_URI_TO_TRACE);
         }

      if (valias.empty() == false)
         {
         U_INTERNAL_DUMP("alias = %.*S", U_STRING_TO_TRACE(*UHTTP::alias))

         // NB: check if needed to reset prev alias uri

         if (UHTTP::virtual_host == false) UHTTP::alias->setEmpty();

         // manage alias uri

         (void) UHTTP::request_uri->assign(U_HTTP_URI_TO_PARAM);

         // gcc - warning: cannot optimize loop, the loop counter may overflow... ???

         for (uint32_t i = 0, n = valias.size(); i < n; i += 2)
            {
            if (*UHTTP::request_uri == valias[i])
               {
               (void) UHTTP::alias->append(valias[i+1]);

               goto next;
               }
            }

         UHTTP::request_uri->clear();
         }
next:
      if (UHTTP::alias->empty() == false)
         {
         UHTTP::setHTTPUri(UHTTP::alias->data(), UHTTP::alias->size());

         U_SRV_LOG_VAR("ALIAS: URI request changed to: %.*s", U_STRING_TO_TRACE(*UHTTP::alias));
         }

#  ifdef HAVE_SSL
      if (uri_request_cert_mask.empty() == false &&
          u_dosmatch_with_OR(U_HTTP_URI_TO_PARAM, U_STRING_TO_PARAM(uri_request_cert_mask), 0))
         {
         if (((UServer<USSLSocket>*)UServer_Base::pthis)->askForClientCertificate() == false)
            {
            U_SRV_LOG_VAR("URI_REQUEST_CERT: request %.*S denied by mandatory certificate from client", U_HTTP_URI_TO_TRACE);

            UHTTP::setHTTPForbidden();

            goto send_response;
            }
         }
#  endif

      if (uri_protected_mask.empty() == false                                               &&
          u_dosmatch_with_OR(U_HTTP_URI_TO_PARAM, U_STRING_TO_PARAM(uri_protected_mask), 0) &&
          UHTTP::checkUriProtected() == false)
         {
         goto send_response;
         }

      U_RETURN(U_PLUGIN_HANDLER_FINISHED);
      }

   // HTTP/1.1 compliance: Sends 411 for missing Content-Length on POST requests
   //                      Sends 400 for broken Request-Line
   //                      Sends 501 for request-method != (GET|POST|HEAD)
   //                      Sends 505 for protocol != HTTP/1.0 or HTTP/1.1

   if (UHTTP::http_info.method_type)
      {
      if (UHTTP::http_info.uri_len == 0)
         {
         nResponseCode            = HTTP_BAD_REQUEST;
         content_type             = U_CTYPE_HTML;
         *UClientImage_Base::body = *UHTTP::str_frm_bad_request;
         }
      else if (UHTTP::http_info.szHeader == 0)
         {
         nResponseCode = HTTP_VERSION;
         }
      else if (UHTTP::isHttpPOST())
         {
         nResponseCode = HTTP_LENGTH_REQUIRED;
         }
      }

   UHTTP::http_info.is_connection_close = U_YES;

   *UClientImage_Base::wbuffer = UHTTP::getHTTPHeaderForResponse(nResponseCode, content_type, *UClientImage_Base::body);

send_response:

   U_INTERNAL_ASSERT_POINTER(UClientImage_Base::pClientImage)

   (void) UClientImage_Base::pClientImage->handlerWrite();

   U_RETURN(U_PLUGIN_HANDLER_ERROR);
}

int UHttpPlugIn::handlerRequest()
{
   U_TRACE(0, "UHttpPlugIn::handlerRequest()")

   // process the HTTP request

   if (UHTTP::checkHTTPRequest())
      {
      if (UHTTP::isPHPRequest() ||
          UHTTP::isCGIRequest())
         {
         (void) UHTTP::processCGIRequest((UCommand*)0, UHTTP::penvironment);
         }
      else
         {
         // NB: we don't want to process the form here (other plugin, USP or TSA)...

         if (UHTTP::isHttpPOST()) U_RETURN(U_PLUGIN_HANDLER_GO_ON);

         (void) UHTTP::processHTTPGetRequest(); // GET,HEAD
         }
      }

   int result = UHTTP::checkForHTTPConnectionClose(); // check for "Connection: close" in headers...

   U_RETURN(result);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UHttpPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "uri_protected_mask       (UString          " << (void*)&uri_protected_mask         << ")\n"
                  << "uri_request_cert_mask    (UString          " << (void*)&uri_request_cert_mask      << ")\n"
                  << "uri_protected_allowed_ip (UString          " << (void*)&uri_protected_allowed_ip   << ")\n"
                  << "valias                   (UVector<UString> " << (void*)&valias                     << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
