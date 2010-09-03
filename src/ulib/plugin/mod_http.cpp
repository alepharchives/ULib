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

#include <ulib/file_config.h>
#include <ulib/utility/uhttp.h>
#include <ulib/plugin/mod_http.h>
#include <ulib/net/server/server.h>
#include <ulib/container/hash_map.h>
#include <ulib/utility/string_ext.h>

U_CREAT_FUNC(UHttpPlugIn)

UString* UHttpPlugIn::str_CACHE_FILE_MASK;
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
      { U_STRINGREP_FROM_CONSTANT("CACHE_FILE_MASK") },
      { U_STRINGREP_FROM_CONSTANT("URI_PROTECTED_MASK") },
      { U_STRINGREP_FROM_CONSTANT("URI_REQUEST_CERT_MASK") },
      { U_STRINGREP_FROM_CONSTANT("URI_PROTECTED_ALLOWED_IP") },
   };

   U_NEW_ULIB_OBJECT(str_CACHE_FILE_MASK,          U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_URI_PROTECTED_MASK,       U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_URI_REQUEST_CERT_MASK,    U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_URI_PROTECTED_ALLOWED_IP, U_STRING_FROM_STRINGREP_STORAGE(3));
}

UHttpPlugIn::~UHttpPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, UHttpPlugIn)

   // delete global HTTP var...

   UHTTP::dtor();
}

// Server-wide hooks

int UHttpPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UHttpPlugIn::handlerConfig(%p)", &cfg)

   // ------------------------------------------------------------------------------------------------------------------------------------------------
   // ALIAS                         vector of URI redirection (request -> alias)
   // REWRITE_RULE_NF               vector of URI rewrite rule applied after checks that files do not exist (regex1 -> uri1 ...)
   //
   // CACHE_FILE_MASK               mask (DOS regexp) of pathfile that be cached in memory
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

   (void) cfg.loadVector(valias, "ALIAS");

   UVector<UString> tmp;

   if (cfg.loadVector(tmp, "REWRITE_RULE_NF"))
      {
      uint32_t n = tmp.size();

      U_INTERNAL_ASSERT_MAJOR(n, 0)

      UHTTP::RewriteRule* rule;
      UHTTP::vRewriteRule = U_NEW(UVector<UHTTP::RewriteRule*>(n));

      for (uint32_t i = 0; i < n; i += 2)
         {
         rule = U_NEW(UHTTP::RewriteRule(tmp[i], tmp[i+1]));

         UHTTP::vRewriteRule->push_back(rule);
         }
      }

   if (cfg.loadTable())
      {
      U_INTERNAL_ASSERT_EQUALS(UHTTP::cache_file_mask, 0)

       UHTTP::cache_file_mask  = U_NEW(UString);
      *UHTTP::cache_file_mask  = cfg[*str_CACHE_FILE_MASK];

      uri_protected_mask       = cfg[*str_URI_PROTECTED_MASK];
      uri_protected_allowed_ip = cfg[*str_URI_PROTECTED_ALLOWED_IP];

#  ifdef HAVE_SSL
      uri_request_cert_mask    = cfg[*str_URI_REQUEST_CERT_MASK];
#  endif

      UHTTP::virtual_host                 = cfg.readBoolean(*UServer_Base::str_VIRTUAL_HOST);
      UServer_Base::digest_authentication = cfg.readBoolean(*UServer_Base::str_DIGEST_AUTHENTICATION);
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UHttpPlugIn::handlerInit()
{
   U_TRACE(0, "UHttpPlugIn::handlerInit()")

   // init global HTTP var...

   UHTTP::ctor();

   // USP (ULib Servlet Page)

   if (UHTTP::pages &&
       UHTTP::pages->empty())
      {
      U_SRV_LOG_MSG("initialization of plugin FAILED");

      U_RETURN(U_PLUGIN_HANDLER_ERROR);
      }

   // URI PROTECTED and ALIAS

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

   // CACHE FILE

   if (UHTTP::cache_file_mask &&
       UHTTP::cache_file_mask->empty() == false)
      {
      UHTTP::searchFileForCache();
      }

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

   // manage buffered read (pipelining)

   UClientImage_Base::manageForPipeline();

   int nResponseCode;
   uint32_t host_end;

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

      U_INTERNAL_DUMP("alias = %.*S", U_STRING_TO_TRACE(*UHTTP::alias))

      if (valias.empty() == false)
         {
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
   //                      Sends 505 for protocol != HTTP/1.[0-1]

   nResponseCode = HTTP_NOT_IMPLEMENTED;

   if (UHTTP::http_info.method_type)
      {
      if (UHTTP::http_info.uri_len == 0)
         {
         UHTTP::setHTTPBadRequest();

         goto send_response;
         }

           if (UHTTP::isHttpPOST())            nResponseCode = HTTP_LENGTH_REQUIRED;
      else if (UHTTP::http_info.szHeader == 0) nResponseCode = HTTP_VERSION;
      }

   UHTTP::http_info.is_connection_close = U_YES;

   UHTTP::setHTTPResponse(nResponseCode, 0, 0);

send_response:

   U_INTERNAL_ASSERT_POINTER(UClientImage_Base::pClientImage)

   (void) UClientImage_Base::pClientImage->handlerWrite();

   U_RETURN(U_PLUGIN_HANDLER_ERROR);
}

int UHttpPlugIn::handlerRequest()
{
   U_TRACE(0, "UHttpPlugIn::handlerRequest()")

   // process the HTTP request

   if (UHTTP::checkHTTPRequest() == 1)
      {
      if (UHTTP::isPHPRequest() ||
          UHTTP::isCGIRequest())
         {
         (void) UHTTP::processCGIRequest((UCommand*)0, UHTTP::penvironment);
         }
      else
         {
         // NB: we don't want to process the form here (other plugin, TSA or ...)

         if (UHTTP::isHttpPOST()) U_RETURN(U_PLUGIN_HANDLER_GO_ON);

         UHTTP::processHTTPGetRequest(); // GET,HEAD
         }
      }

   int result = UHTTP::checkForHTTPConnectionClose(); // check for "Connection: close" in headers...

   U_RETURN(result);
}

int UHttpPlugIn::handlerReset()
{
   U_TRACE(0, "UUspPlugIn::handlerReset()")

   // check if dynamic page (ULib Servlet Page)

   if (UHTTP::runDynamicPage)
      {
      U_INTERNAL_ASSERT_POINTER(UHTTP::pages)

      UHTTP::runDynamicPage((void*)-1); // call reset for module...

      UHTTP::runDynamicPage = 0;
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
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
