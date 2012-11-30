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
const UString* UHttpPlugIn::str_TELNET_ENABLE;
const UString* UHttpPlugIn::str_MIN_SIZE_FOR_SENDFILE;
const UString* UHttpPlugIn::str_URI_REQUEST_STRICT_TRANSPORT_SECURITY_MASK;
const UString* UHttpPlugIn::str_SESSION_COOKIE_OPTION;
const UString* UHttpPlugIn::str_MAINTENANCE_MODE;
const UString* UHttpPlugIn::str_APACHE_LIKE_LOG;

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
   U_INTERNAL_ASSERT_EQUALS(str_TELNET_ENABLE,0)
   U_INTERNAL_ASSERT_EQUALS(str_MIN_SIZE_FOR_SENDFILE,0)
   U_INTERNAL_ASSERT_EQUALS(str_URI_REQUEST_STRICT_TRANSPORT_SECURITY_MASK,0)
   U_INTERNAL_ASSERT_EQUALS(str_SESSION_COOKIE_OPTION,0)
   U_INTERNAL_ASSERT_EQUALS(str_MAINTENANCE_MODE,0)
   U_INTERNAL_ASSERT_EQUALS(str_APACHE_LIKE_LOG,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("CACHE_FILE_MASK") },
      { U_STRINGREP_FROM_CONSTANT("URI_PROTECTED_MASK") },
      { U_STRINGREP_FROM_CONSTANT("URI_REQUEST_CERT_MASK") },
      { U_STRINGREP_FROM_CONSTANT("URI_PROTECTED_ALLOWED_IP") },
      { U_STRINGREP_FROM_CONSTANT("LIMIT_REQUEST_BODY") },
      { U_STRINGREP_FROM_CONSTANT("REQUEST_READ_TIMEOUT") },
      { U_STRINGREP_FROM_CONSTANT("ENABLE_INOTIFY") },
      { U_STRINGREP_FROM_CONSTANT("ENABLE_CACHING_BY_PROXY_SERVERS") },
      { U_STRINGREP_FROM_CONSTANT("TELNET_ENABLE") },
      { U_STRINGREP_FROM_CONSTANT("MIN_SIZE_FOR_SENDFILE") },
      { U_STRINGREP_FROM_CONSTANT("URI_REQUEST_STRICT_TRANSPORT_SECURITY_MASK") },
      { U_STRINGREP_FROM_CONSTANT("SESSION_COOKIE_OPTION") },
      { U_STRINGREP_FROM_CONSTANT("MAINTENANCE_MODE") },
      { U_STRINGREP_FROM_CONSTANT("APACHE_LIKE_LOG") }
   };

   U_NEW_ULIB_OBJECT(str_CACHE_FILE_MASK,                            U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_URI_PROTECTED_MASK,                         U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_URI_REQUEST_CERT_MASK,                      U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_URI_PROTECTED_ALLOWED_IP,                   U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_LIMIT_REQUEST_BODY,                         U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_REQUEST_READ_TIMEOUT,                       U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_ENABLE_INOTIFY,                             U_STRING_FROM_STRINGREP_STORAGE(6));
   U_NEW_ULIB_OBJECT(str_ENABLE_CACHING_BY_PROXY_SERVERS,            U_STRING_FROM_STRINGREP_STORAGE(7));
   U_NEW_ULIB_OBJECT(str_TELNET_ENABLE,                              U_STRING_FROM_STRINGREP_STORAGE(8));
   U_NEW_ULIB_OBJECT(str_MIN_SIZE_FOR_SENDFILE,                      U_STRING_FROM_STRINGREP_STORAGE(9));
   U_NEW_ULIB_OBJECT(str_URI_REQUEST_STRICT_TRANSPORT_SECURITY_MASK, U_STRING_FROM_STRINGREP_STORAGE(10));
   U_NEW_ULIB_OBJECT(str_SESSION_COOKIE_OPTION,                      U_STRING_FROM_STRINGREP_STORAGE(11));
   U_NEW_ULIB_OBJECT(str_MAINTENANCE_MODE,                           U_STRING_FROM_STRINGREP_STORAGE(12));
   U_NEW_ULIB_OBJECT(str_APACHE_LIKE_LOG,                            U_STRING_FROM_STRINGREP_STORAGE(13));
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

void UHttpPlugIn::handlerDelete()
{
   U_TRACE(0, "UHttpPlugIn::handlerDelete()")

   U_INTERNAL_DUMP("UEventFd::fd = %d", UEventFd::fd)

   UEventFd::fd = 0;
}

// Server-wide hooks

int UHttpPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UHttpPlugIn::handlerConfig(%p)", &cfg)

   // ------------------------------------------------------------------------------------------------------------------------------------------------
   // ALIAS                        vector of URI redirection (request -> alias)
   // REWRITE_RULE_NF              vector of URI rewrite rule applied after checks that files do not exist (regex1 -> uri1 ...)
   //
   // MAINTENANCE_MODE             to switch the site to a maintenance page only
   //
   // APACHE_LIKE_LOG              file to write NCSA extended/combined log format: "%h %l %u %t \"%r\" %>s %b \"%{Referer}i\" \"%{User-agent}i\""
   //
   // ENABLE_INOTIFY               enable automatic update of document root image with inotify
   // TELNET_ENABLE                accept fragmentation of header request (as happen with telnet)
   // CACHE_FILE_MASK              mask (DOS regexp) of pathfile that be cached in memory
   //
   // MIN_SIZE_FOR_SENDFILE        for major size it is better to use sendfile() to serve static content
   //
   // VIRTUAL_HOST                 flag to activate practice of maintaining more than one server on one machine,
   //                              as differentiated by their apparent hostname
   // DIGEST_AUTHENTICATION        flag authentication method (yes = digest, no = basic)
   //
   // ENABLE_CACHING_BY_PROXY_SERVERS enable caching by proxy servers (add "Cache control: public" directive)
   //
   // URI_PROTECTED_MASK           mask (DOS regexp) of URI protected from prying eyes
   // URI_PROTECTED_ALLOWED_IP     list of comma separated client address for IP-based access control (IPADDR[/MASK]) for URI_PROTECTED_MASK
   //
   // URI_REQUEST_CERT_MASK                      mask (DOS regexp) of URI where client must comunicate a certificate in the SSL connection
   // URI_REQUEST_STRICT_TRANSPORT_SECURITY_MASK mask (DOS regexp) of URI where use HTTP Strict Transport Security to force client to use only SSL
   //
   // SESSION_COOKIE_OPTION        eventual params for session cookie (lifetime, path, domain, secure, HttpOnly)  
   // ----------------------------------------------------------------------------------------------------------------------------------------------------
   // This directive gives greater control over abnormal client request behavior, which may be useful for avoiding some forms of denial-of-service attacks
   // ----------------------------------------------------------------------------------------------------------------------------------------------------
   // LIMIT_REQUEST_BODY           restricts the total size of the HTTP request body sent from the client
   // REQUEST_READ_TIMEOUT         set timeout for receiving requests
   // ------------------------------------------------------------------------------------------------------------------------------------------------

   UVector<UString> tmp;

   if (cfg.loadVector(tmp, "ALIAS") &&
       tmp.empty() == false)
      {
      U_INTERNAL_ASSERT_EQUALS(UHTTP::valias,0)

      UHTTP::valias = UVector<UString>::duplicate(&tmp);

      tmp.clear();
      }

   if (cfg.loadVector(tmp, "REWRITE_RULE_NF") &&
       tmp.empty() == false)
      {
      U_INTERNAL_ASSERT_EQUALS(UHTTP::vRewriteRule,0)

      UHTTP::RewriteRule* rule;
      int32_t i, n = tmp.size();
      UHTTP::vRewriteRule = U_NEW(UVector<UHTTP::RewriteRule*>(n));

      for (i = 0; i < n; i += 2)
         {
         rule = U_NEW(UHTTP::RewriteRule(tmp[i], tmp[i+1]));

         UHTTP::vRewriteRule->push_back(rule);
         }
      }

   if (cfg.loadTable())
      {
      UHTTP::virtual_host                    = cfg.readBoolean(*UServer_Base::str_VIRTUAL_HOST);
      UHTTP::telnet_enable                   = cfg.readBoolean(*str_TELNET_ENABLE);
      UHTTP::limit_request_body              = cfg.readLong(*str_LIMIT_REQUEST_BODY, U_STRING_LIMIT);
      UHTTP::request_read_timeout            = cfg.readLong(*str_REQUEST_READ_TIMEOUT);
      UHTTP::min_size_for_sendfile           = cfg.readLong(*str_MIN_SIZE_FOR_SENDFILE);
      UHTTP::enable_caching_by_proxy_servers = cfg.readBoolean(*str_ENABLE_CACHING_BY_PROXY_SERVERS);

      U_INTERNAL_DUMP("UHTTP::limit_request_body = %u", UHTTP::limit_request_body)

      U_INTERNAL_ASSERT_EQUALS(UHTTP::cookie_option,0)
      U_INTERNAL_ASSERT_EQUALS(UHTTP::cache_file_mask,0)
      U_INTERNAL_ASSERT_EQUALS(UHTTP::uri_protected_mask,0)
      U_INTERNAL_ASSERT_EQUALS(UHTTP::uri_request_cert_mask,0)
      U_INTERNAL_ASSERT_EQUALS(UHTTP::maintenance_mode_page,0)
      U_INTERNAL_ASSERT_EQUALS(UHTTP::uri_strict_transport_security_mask,0)

      UString x = cfg[*UServer_Base::str_DIGEST_AUTHENTICATION];

      if (x.empty() == false) UHTTP::digest_authentication = x.strtob();
      else
         {
#     ifdef USE_LIBSSL
         if (UServer_Base::bssl) UHTTP::digest_authentication = false;
         else
#     endif
                                 UHTTP::digest_authentication = true;
         }

#  ifdef USE_LIBSSL
      x = cfg[*str_URI_REQUEST_CERT_MASK];

      if (x.empty() == false)
         {
         if (x.someWhiteSpace()) x = UStringExt::removeWhiteSpace(x);

         UHTTP::uri_request_cert_mask = U_NEW(UString(x));
         }

      x = cfg[*str_URI_REQUEST_STRICT_TRANSPORT_SECURITY_MASK];

      if (x.empty() == false)
         {
         if (x.someWhiteSpace()) x = UStringExt::removeWhiteSpace(x);

         UHTTP::uri_strict_transport_security_mask = U_NEW(UString(x));
         }
#  endif

      x = cfg[*str_CACHE_FILE_MASK];

      if (x.empty() == false)
         {
         if (x.someWhiteSpace()) x = UStringExt::removeWhiteSpace(x);

         UHTTP::cache_file_mask = U_NEW(UString(x));
         }

      x = cfg[*str_URI_PROTECTED_MASK];

      if (x.empty() == false)
         {
         if (x.someWhiteSpace()) x = UStringExt::removeWhiteSpace(x);

         UHTTP::uri_protected_mask = U_NEW(UString(x));
         }

      x = cfg[*str_SESSION_COOKIE_OPTION];

      if (x.empty() == false) UHTTP::cookie_option = U_NEW(UString(x));

      x = cfg[*str_MAINTENANCE_MODE];

      if (x.empty() == false) UHTTP::maintenance_mode_page = U_NEW(UString(x));

#  if defined(HAVE_SYS_INOTIFY_H) && defined(U_HTTP_INOTIFY_SUPPORT)
      if (cfg.readBoolean(*str_ENABLE_INOTIFY))
         {
         // NB: we ask to notify for change of file system (inotify)
         //     in the thread approach this is very dangerous...

         UServer_Base::handler_inotify = this;

#     if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD) && defined(U_SERVER_THREAD_APPROACH_SUPPORT)
         if (UNotifier::pthread)
            {
            U_SRV_LOG("Sorry, I can't enable inode based directory notification because PREFORK_CHILD == -1 (server thread approach)");
            }
#     endif
         }
#  endif

      // URI PROTECTED

      x = cfg[*str_URI_PROTECTED_ALLOWED_IP];

      if (x.empty() == false)
         {
         U_INTERNAL_ASSERT_EQUALS(UHTTP::vallow_IP,0)

         UHTTP::vallow_IP = U_NEW(UVector<UIPAllow*>);

         if (UIPAllow::parseMask(x, *UHTTP::vallow_IP) == 0)
            {
            delete UHTTP::vallow_IP;
                   UHTTP::vallow_IP = 0;
            }
         }

      x = cfg[*str_APACHE_LIKE_LOG];

      if (x.empty() == false)
         {
         UHTTP::apache_like_log = U_NEW(UFile(x));

         if (UServer_Base::addLog(UHTTP::apache_like_log) == false)
            {
            delete UHTTP::apache_like_log;
                   UHTTP::apache_like_log = 0;
            }
         }
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UHttpPlugIn::handlerInit()
{
   U_TRACE(0, "UHttpPlugIn::handlerInit()")

#ifdef USE_LIBSSL
   if (UServer_Base::bssl) UHTTP::min_size_for_sendfile = U_NOT_FOUND;

   U_INTERNAL_DUMP("UServer_Base::bssl = %b min_size_for_sendfile = %u", UServer_Base::bssl, UHTTP::min_size_for_sendfile)
#endif

   UHTTP::ctor(); // init HTTP context

   U_SRV_LOG("initialization of plugin success");

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UHttpPlugIn::handlerRun()
{
   U_TRACE(0, "UHttpPlugIn::handlerRun()")

   U_INTERNAL_ASSERT_POINTER(UHTTP::cache_file)

   // NB: we use this method because now we have the shared data allocated by UServer...

   if (UServer_Base::isLog()) UServer_Base::mod_name->snprintf("[usp_init] ", 0);

   UHTTP::cache_file->callForAllEntry(UHTTP::callInitForAllUSP);

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UHttpPlugIn::handlerStop()
{
   U_TRACE(0, "UHttpPlugIn::handlerStop()")

   U_INTERNAL_ASSERT_POINTER(UHTTP::cache_file)

   if (UServer_Base::isLog()) UServer_Base::mod_name->snprintf("[usp_end] ", 0);

   UHTTP::cache_file->callForAllEntry(UHTTP::callEndForAllUSP);

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int UHttpPlugIn::handlerREAD()
{
   U_TRACE(0, "UHttpPlugIn::handlerREAD()")

   int result;

#ifdef U_HTTP_CACHE_REQUEST
   U_INTERNAL_DUMP("cbuffer(%u) = %.*S",             UHTTP::cbuffer->size(), U_STRING_TO_TRACE(*UHTTP::cbuffer))
   U_INTERNAL_DUMP("rbuffer(%u) = %.*S", UClientImage_Base::rbuffer->size(), U_STRING_TO_TRACE(*UClientImage_Base::rbuffer))

   if (UHTTP::cbuffer->isNull() == false)
      {
      U_INTERNAL_DUMP("expire        = %ld", UServer_Base::expire)
      U_INTERNAL_DUMP("u_now->tv_sec = %ld", u_now->tv_sec)

      if (UServer_Base::expire >= u_now->tv_sec)
         {
         result = UHTTP::checkHTTPRequestCache();

         if (result != U_PLUGIN_HANDLER_FINISHED)
            {
            if (UHTTP::apache_like_log) UHTTP::writeApacheLikeLog();

            U_RETURN(result);
            }
         }

      UHTTP::clearHTTPRequestCache();
      }

   if (UClientImage_Base::isPipeline() == false) UClientImage_Base::initAfterGenericRead();
#endif

   if (UHTTP::manageHTTPRequest() == false) U_RETURN(U_PLUGIN_HANDLER_ERROR);

   // NB: we check if we can shortcut the http request processing (only in the context of U_PLUGIN_HANDLER_FINISHED)...

   UServer_Base::bpluginsHandlerRequest = (UClientImage_Base::write_off           == false &&
                                           UHTTP::isHTTPRequestAlreadyProcessed() == false);

   U_INTERNAL_DUMP("UServer_Base::bpluginsHandlerRequest = %b", UServer_Base::bpluginsHandlerRequest)

   if (UServer_Base::bpluginsHandlerRequest) U_RETURN(U_PLUGIN_HANDLER_FINISHED);

   result = handlerRequest();

   U_RETURN(result);
}

int UHttpPlugIn::handlerRequest()
{
   U_TRACE(0, "UHttpPlugIn::handlerRequest()")

   // process the HTTP request

   U_INTERNAL_DUMP("method = %.*S method_type = %C uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_http_method_type, U_HTTP_URI_TO_TRACE)

   switch (U_http_request_check)
      {
      case U_HTTP_REQUEST_IS_FORBIDDEN:
         {
         U_ASSERT(UHTTP::isHTTPRequestForbidden())

         UHTTP::setHTTPForbidden(); // set forbidden error response...
         }
      break;

      case U_HTTP_REQUEST_IS_NOT_FOUND:
         {
         U_ASSERT(UHTTP::isHTTPRequestNotFound())

         if (U_http_method_type == HTTP_OPTIONS)
            {
            u_http_info.nResponseCode = HTTP_OPTIONS_RESPONSE;

            UHTTP::setHTTPResponse(0, 0);

            goto end;
            }

         UHTTP::setHTTPNotFound(); // set not found error response...
         }
      break;

      case U_HTTP_REQUEST_NEED_PROCESSING:
         {
         U_ASSERT(UHTTP::isHTTPRequestNeedProcessing())
         U_INTERNAL_ASSERT(*UClientImage_Base::request)

         if (UHTTP::isHttpGETorHEAD() == false)
            {
            if (U_http_method_type == HTTP_OPTIONS)
               {
               u_http_info.nResponseCode = HTTP_OPTIONS_RESPONSE;

               UHTTP::setHTTPResponse(0, 0);

               goto end;
               }

            // NB: we don't want to process this kind of request here...

            u_http_info.nResponseCode = HTTP_NOT_IMPLEMENTED;

            UHTTP::setHTTPResponse(0, 0);

            // NB: maybe there are other plugin after this...

            U_RETURN(U_PLUGIN_HANDLER_GO_ON);
            }

         UHTTP::processHTTPGetRequest(); // GET,HEAD
         }
      break;

#  ifdef DEBUG
      default: U_ASSERT(UHTTP::isHTTPRequestAlreadyProcessed()) break;
#  endif
      }

end:
   UHTTP::endHTTPRequestProcessing();

   // check for "Connection: close" in headers

   U_INTERNAL_DUMP("U_http_is_connection_close = %d", U_http_is_connection_close)

   if (U_http_is_connection_close == U_YES) U_RETURN(U_PLUGIN_HANDLER_ERROR);

   U_RETURN(U_PLUGIN_HANDLER_FINISHED);
}
