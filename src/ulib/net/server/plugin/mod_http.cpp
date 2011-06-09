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

#include <ulib/command.h>
#include <ulib/file_config.h>
#include <ulib/utility/uhttp.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/string_ext.h>
#include <ulib/net/server/plugin/mod_http.h>

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_http, UHttpPlugIn)
#endif

const UString* UHttpPlugIn::str_CACHE_FILE_MASK;
const UString* UHttpPlugIn::str_URI_PROTECTED_MASK;
const UString* UHttpPlugIn::str_URI_REQUEST_CERT_MASK;
const UString* UHttpPlugIn::str_URI_PROTECTED_ALLOWED_IP;
const UString* UHttpPlugIn::str_LIMIT_REQUEST_BODY;
const UString* UHttpPlugIn::str_REQUEST_READ_TIMEOUT;
const UString* UHttpPlugIn::str_ENABLE_INOTIFY;
const UString* UHttpPlugIn::str_ENABLE_CACHING_BY_PROXY_SERVERS;

void UHttpPlugIn::str_allocate()
{
   U_TRACE(0, "UHttpPlugIn::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_CACHE_FILE_MASK,0)
   U_INTERNAL_ASSERT_EQUALS(str_URI_PROTECTED_MASK,0)
   U_INTERNAL_ASSERT_EQUALS(str_URI_REQUEST_CERT_MASK,0)
   U_INTERNAL_ASSERT_EQUALS(str_URI_PROTECTED_ALLOWED_IP,0)
   U_INTERNAL_ASSERT_EQUALS(str_LIMIT_REQUEST_BODY,0)
   U_INTERNAL_ASSERT_EQUALS(str_REQUEST_READ_TIMEOUT,0)
   U_INTERNAL_ASSERT_EQUALS(str_ENABLE_INOTIFY,0)
   U_INTERNAL_ASSERT_EQUALS(str_ENABLE_CACHING_BY_PROXY_SERVERS,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("CACHE_FILE_MASK") },
      { U_STRINGREP_FROM_CONSTANT("URI_PROTECTED_MASK") },
      { U_STRINGREP_FROM_CONSTANT("URI_REQUEST_CERT_MASK") },
      { U_STRINGREP_FROM_CONSTANT("URI_PROTECTED_ALLOWED_IP") },
      { U_STRINGREP_FROM_CONSTANT("LIMIT_REQUEST_BODY") },
      { U_STRINGREP_FROM_CONSTANT("REQUEST_READ_TIMEOUT") },
      { U_STRINGREP_FROM_CONSTANT("ENABLE_INOTIFY") },
      { U_STRINGREP_FROM_CONSTANT("ENABLE_CACHING_BY_PROXY_SERVERS") }
   };

   U_NEW_ULIB_OBJECT(str_CACHE_FILE_MASK,                   U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_URI_PROTECTED_MASK,                U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_URI_REQUEST_CERT_MASK,             U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_URI_PROTECTED_ALLOWED_IP,          U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_LIMIT_REQUEST_BODY,                U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_REQUEST_READ_TIMEOUT,              U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_ENABLE_INOTIFY,                    U_STRING_FROM_STRINGREP_STORAGE(6));
   U_NEW_ULIB_OBJECT(str_ENABLE_CACHING_BY_PROXY_SERVERS,   U_STRING_FROM_STRINGREP_STORAGE(7));
}

UHttpPlugIn::~UHttpPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, UHttpPlugIn)

   UHTTP::dtor(); // delete global HTTP context...
}

// define method VIRTUAL of class UEventFd

int UHttpPlugIn::handlerRead()
{
   U_TRACE(0, "UHttpPlugIn::handlerRead()")

   UHTTP::in_READ();

   U_RETURN(U_NOTIFIER_OK);
}

// Server-wide hooks

int UHttpPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UHttpPlugIn::handlerConfig(%p)", &cfg)

   // ------------------------------------------------------------------------------------------------------------------------------------------------
   // ALIAS                        vector of URI redirection (request -> alias)
   // REWRITE_RULE_NF              vector of URI rewrite rule applied after checks that files do not exist (regex1 -> uri1 ...)
   //
   // ENABLE_INOTIFY               enable automatic update of document root image with inotify
   // CACHE_FILE_MASK              mask (DOS regexp) of pathfile that be cached in memory
   //
   // VIRTUAL_HOST                 flag to activate practice of maintaining more than one server on one machine,
   //                              as differentiated by their apparent hostname
   // DIGEST_AUTHENTICATION        flag authentication method (yes = digest, no = basic)
   //
   // ENABLE_CACHING_BY_PROXY_SERVERS enable caching by proxy servers (add Cache control: public directive)
   //
   // URI_PROTECTED_MASK           mask (DOS regexp) of URI protected from prying eyes
   // URI_PROTECTED_ALLOWED_IP     list of comma separated client address for IP-based access control (IPADDR[/MASK]) for URI_PROTECTED_MASK
   //
   // URI_REQUEST_CERT_MASK        mask (DOS regexp) of URI where client must comunicate a certificate in the SSL connection
   //
   // ----------------------------------------------------------------------------------------------------------------------------------------------------
   // This directive gives greater control over abnormal client request behavior, which may be useful for avoiding some forms of denial-of-service attacks
   // ----------------------------------------------------------------------------------------------------------------------------------------------------
   // LIMIT_REQUEST_BODY           restricts the total size of the HTTP request body sent from the client
   // REQUEST_READ_TIMEOUT         set timeout for receiving requests
   // ------------------------------------------------------------------------------------------------------------------------------------------------

   (void) cfg.loadVector(valias, "ALIAS");

   UVector<UString> tmp;

   if (cfg.loadVector(tmp, "REWRITE_RULE_NF") && tmp.empty() == false)
      {
      uint32_t n = tmp.size();

      U_INTERNAL_ASSERT_MAJOR(n, 0)

      UHTTP::RewriteRule* rule;
      UHTTP::vRewriteRule = U_NEW(UVector<UHTTP::RewriteRule*>(n));

      for (int32_t i = 0; i < (int32_t)n; i += 2)
         {
         rule = U_NEW(UHTTP::RewriteRule(tmp[i], tmp[i+1]));

         UHTTP::vRewriteRule->push_back(rule);
         }
      }

   if (cfg.loadTable())
      {
      uri_protected_allowed_ip = cfg[*str_URI_PROTECTED_ALLOWED_IP];

      UHTTP::virtual_host                    = cfg.readBoolean(*UServer_Base::str_VIRTUAL_HOST);
      UHTTP::limit_request_body              = cfg.readLong(*str_LIMIT_REQUEST_BODY, UString::max_size() - 4096);
      UHTTP::request_read_timeout            = cfg.readLong(*str_REQUEST_READ_TIMEOUT);
      UHTTP::digest_authentication           = cfg.readBoolean(*UServer_Base::str_DIGEST_AUTHENTICATION);
      UHTTP::enable_caching_by_proxy_servers = cfg.readBoolean(*str_ENABLE_CACHING_BY_PROXY_SERVERS);

#  ifdef HAVE_SSL
      uri_request_cert_mask                  = cfg[*str_URI_REQUEST_CERT_MASK];
#  endif

      UString x = cfg[*str_CACHE_FILE_MASK];

      U_INTERNAL_ASSERT_EQUALS(UHTTP::cache_file_mask,0)
      U_INTERNAL_ASSERT_EQUALS(UHTTP::uri_protected_mask,0)

      if (x.empty() == false) UHTTP::cache_file_mask = U_NEW(UString(x));

      x = cfg[*str_URI_PROTECTED_MASK];

      if (x.empty() == false) UHTTP::uri_protected_mask = U_NEW(UString(x));

      if (cfg.readBoolean(*str_ENABLE_INOTIFY)) UServer_Base::handler_event = this;
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UHttpPlugIn::handlerInit()
{
   U_TRACE(0, "UHttpPlugIn::handlerInit()")

   // init HTTP context

   UHTTP::ctor();

   // URI PROTECTED

   if (uri_protected_allowed_ip.empty() == false)
      {
      UHTTP::vallow_IP = U_NEW(UVector<UIPAllow*>);

      if (UIPAllow::parseMask(uri_protected_allowed_ip, *UHTTP::vallow_IP) == 0)
         {
         delete UHTTP::vallow_IP;
                UHTTP::vallow_IP = 0;
         }
      }

   U_SRV_LOG("initialization of plugin success");

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int UHttpPlugIn::handlerREAD()
{
   U_TRACE(0, "UHttpPlugIn::handlerREAD()")

   if (UHTTP::readHTTPRequest(UServer_Base::pClientImage->socket))
      {
      UHTTP::getTimeIfNeeded(false);

      // HTTP 1.1 want header "Host: ..."

      U_INTERNAL_DUMP("U_http_version = %C", U_http_version)

      if (U_http_version            == '1' &&
          UHTTP::http_info.host_len == 0)
         {
         UHTTP::setHTTPBadRequest();

         goto send_response;
         }

      // manage dynamic page request (ULib Servlet Page)

      if (UHTTP::isUSPRequest())
         {
         UHTTP::processUSPRequest(U_HTTP_URI_TO_PARAM);

         UHTTP::setHTTPRequestProcessed();

         U_RETURN(U_PLUGIN_HANDLER_FINISHED);
         }

      // manage virtual host

      if (UHTTP::virtual_host &&
          UHTTP::http_info.host_vlen)
         {
         // Host: hostname[:port]

         UHTTP::alias->setBuffer(1 + UHTTP::http_info.host_vlen + UHTTP::http_info.uri_len);

         UHTTP::alias->snprintf("/%.*s%.*s", U_HTTP_VHOST_TO_TRACE, U_HTTP_URI_TO_TRACE);
         }

      // manage alias uri

      if (UHTTP::ssi_alias ||
          valias.empty() == false)
         {
         UString item;

         for (int32_t i = 0, n = valias.size(); i < n; i += 2)
            {
            item = valias[i];

            if (U_HTTP_URI_EQUAL(item))
               {
               *UHTTP::request_uri = item;

               (void) UHTTP::alias->append(valias[i+1]);

               goto next;
               }
            }

         if (UHTTP::ssi_alias &&
             u_getsuffix(U_HTTP_URI_TO_PARAM) == 0)
            {
            uint32_t len = UHTTP::ssi_alias->size();

            (void) UHTTP::request_uri->assign(U_HTTP_URI_TO_PARAM);

            UHTTP::alias->setBuffer(UHTTP::alias->size() + 1 + len);

            UHTTP::alias->snprintf_add("/%.*s", len, UHTTP::ssi_alias->data());
            }
         }
next:
      if (UHTTP::alias->empty() == false)
         {
         uint32_t len    = UHTTP::alias->size();
         const char* ptr = UHTTP::alias->data();

         UHTTP::setHTTPUri(ptr, len);

         U_SRV_LOG("ALIAS: URI request changed to: %.*s", len, ptr);
         }

#  ifdef HAVE_SSL
      if (uri_request_cert_mask.empty() == false &&
          u_dosmatch_with_OR(U_HTTP_URI_TO_PARAM, U_STRING_TO_PARAM(uri_request_cert_mask), 0))
         {
         if (((UServer<USSLSocket>*)UServer_Base::pthis)->askForClientCertificate() == false)
            {
            U_SRV_LOG("URI_REQUEST_CERT: request %.*S denied by mandatory certificate from client", U_HTTP_URI_TO_TRACE);

            UHTTP::setHTTPForbidden();

            goto send_response;
            }
         }
#  endif

      if (UHTTP::uri_protected_mask &&
          UHTTP::checkUriProtected() == false)
         {
         goto send_response;
         }

      UHTTP::checkHTTPRequest();

      U_RETURN(U_PLUGIN_HANDLER_FINISHED);
      }

   U_http_is_connection_close = U_YES;

   if (UClientImage_Base::wbuffer->empty())
      {
      // HTTP/1.1 compliance: Sends 411 for missing Content-Length on POST requests
      //                      Sends 400 for broken Request-Line
      //                      Sends 501 for request-method != (GET|POST|HEAD)
      //                      Sends 505 for protocol != HTTP/1.[0-1]

      int nResponseCode = HTTP_NOT_IMPLEMENTED;

      if (U_http_method_type)
         {
         if (UHTTP::http_info.uri_len == 0)
            {
            UHTTP::setHTTPBadRequest();

            goto send_response;
            }

         if (UHTTP::http_info.szHeader == 0) nResponseCode = HTTP_VERSION;
         }

      UHTTP::setHTTPResponse(nResponseCode, 0, 0);
      }

send_response:
   (void) UServer_Base::pClientImage->handlerWrite();

   U_RETURN(U_PLUGIN_HANDLER_ERROR);
}

int UHttpPlugIn::handlerRequest()
{
   U_TRACE(0, "UHttpPlugIn::handlerRequest()")

   // process the HTTP request

   if (UHTTP::isHTTPRequestAlreadyProcessed()) goto end;

   if (UHTTP::isHTTPRequestNeedProcessing())
      {
      U_ASSERT_DIFFERS(UClientImage_Base::request->empty(), true)

      if (UHTTP::isCGIRequest())
         {
         // NB: if server no preforked (ex: nodog) process the HTTP CGI request with fork....

         bool async = (UServer_Base::preforked_num_kids <= 0 && UClientImage_Base::isPipeline() == false);

         if (UHTTP::processCGIRequest((UCommand*)0, 0, async, true) == false)
            {
                 if (UCommand::isTimeout())               UHTTP::setHTTPResponse(HTTP_GATEWAY_TIMEOUT, 0, 0);
            else if (UClientImage_Base::wbuffer->empty()) UHTTP::setHTTPInternalError();
            }
         }
      else
         {
         if (UHTTP::isHttpGETorHEAD() == false)
            {
            // NB: we don't want to process this request here (maybe other plugin after...)

            UHTTP::setHTTPResponse(HTTP_NOT_IMPLEMENTED, 0, 0);

            U_RETURN(U_PLUGIN_HANDLER_GO_ON);
            }

         UHTTP::processHTTPGetRequest(UServer_Base::pClientImage->socket, *UClientImage_Base::request); // GET,HEAD
         }
      }
   else if (UHTTP::isHTTPRequestNotFound())  UHTTP::setHTTPNotFound();  // set not found error response...
   else if (UHTTP::isHTTPRequestForbidden()) UHTTP::setHTTPForbidden(); // set forbidden error response...

end:  // check for "Connection: close" in headers...

   U_INTERNAL_DUMP("U_http_is_connection_close = %d", U_http_is_connection_close)

   if (U_http_is_connection_close == U_YES)
      {
      (void) UServer_Base::pClientImage->handlerWrite();

      U_RETURN(U_PLUGIN_HANDLER_ERROR);
      }

   U_RETURN(U_PLUGIN_HANDLER_FINISHED);
}

int UHttpPlugIn::handlerReset()
{
   U_TRACE(0, "UHttpPlugIn::handlerReset()")

   if (UHTTP::real_ip->empty() == false) UHTTP::real_ip->setEmpty();

   // check if dynamic page (ULib Servlet Page)

   if (UHTTP::usp_page)
      {
      U_INTERNAL_ASSERT_POINTER(UHTTP::usp_pages)
      U_INTERNAL_ASSERT_EQUALS(UHTTP::usp_pages->empty(),false)
      U_INTERNAL_ASSERT_POINTER(UHTTP::usp_page->runDynamicPage)

      UHTTP::usp_page->runDynamicPage((UClientImage_Base*)-1); // call reset for module...

      UHTTP::usp_page = 0;
      }

   // NB: check if needed to reset alias URI

   U_INTERNAL_DUMP("UHTTP::alias = %.*S UHTTP::request_uri = %.*S", U_STRING_TO_TRACE(*UHTTP::alias), U_STRING_TO_TRACE(*UHTTP::request_uri))

   if (UHTTP::alias->empty() == false)
      {
      UHTTP::alias->clear();
      UHTTP::request_uri->clear();
      }

   // check if timeout

   if (UServer_Base::pClientImage->socket->isBroken())
      {
      U_http_is_connection_close = U_YES;

      UHTTP::setHTTPResponse(HTTP_CLIENT_TIMEOUT, 0, 0);

      (void) UServer_Base::pClientImage->handlerWrite();

      U_RETURN(U_PLUGIN_HANDLER_ERROR);
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UHttpPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "uri_request_cert_mask    (UString          " << (void*)&uri_request_cert_mask      << ")\n"
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
