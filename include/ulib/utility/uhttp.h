// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    uhttp.h - HTTP utility
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_HTTP_H
#define ULIB_HTTP_H 1

#include <ulib/string.h>
#include <ulib/net/socket.h>
#include <ulib/dynamic/dynamic.h>

#ifdef HAVE_PCRE
#  include <ulib/pcre/pcre.h>
#else
#  include <ulib/container/vector.h>
#endif

/* -----------------------------------------------------------------------------------------------------------------------------
//  _     _   _
//  | |__ | |_| |_ _ __
//  | '_ \| __| __| '_ \
//  | | | | |_| |_| |_) |
//  |_| |_|\__|\__| .__/
//                  |_|
//
// ----------------------------------------------------------------------------------------------------------------------------- */
// HTTP message handler
//
// The status code is a three-digit integer, and the first digit identifies the general category of response:
// -----------------------------------------------------------------------------------------------------------------------------

// 1xx indicates an informational message only
#define HTTP_CONTINUE            100
#define HTTP_SWITCH_PROT         101

// 2xx indicates success of some kind
#define HTTP_OK                  200
#define HTTP_CREATED             201
#define HTTP_ACCEPTED            202
#define HTTP_NOT_AUTHORITATIVE   203
#define HTTP_NO_CONTENT          204
#define HTTP_RESET               205
#define HTTP_PARTIAL             206

#define HTTP_OPTIONS_RESPONSE    222

// 3xx redirects the client to another URL
#define HTTP_MULT_CHOICE         300
#define HTTP_MOVED_PERM          301
#define HTTP_MOVED_TEMP          302
#define HTTP_FOUND               302
#define HTTP_SEE_OTHER           303
#define HTTP_NOT_MODIFIED        304
#define HTTP_USE_PROXY           305
#define HTTP_TEMP_REDIR          307

// 4xx indicates an error on the client's part
#define HTTP_BAD_REQUEST         400
#define HTTP_UNAUTHORIZED        401
#define HTTP_PAYMENT_REQUIRED    402
#define HTTP_FORBIDDEN           403
#define HTTP_NOT_FOUND           404
#define HTTP_BAD_METHOD          405
#define HTTP_NOT_ACCEPTABLE      406
#define HTTP_PROXY_AUTH          407
#define HTTP_CLIENT_TIMEOUT      408
#define HTTP_CONFLICT            409
#define HTTP_GONE                410
#define HTTP_LENGTH_REQUIRED     411
#define HTTP_PRECON_FAILED       412
#define HTTP_ENTITY_TOO_LARGE    413
#define HTTP_REQ_TOO_LONG        414
#define HTTP_UNSUPPORTED_TYPE    415
#define HTTP_REQ_RANGE_NOT_OK    416
#define HTTP_EXPECTATION_FAILED  417

// 5xx indicates an error on the server's part
#define HTTP_INTERNAL_ERROR      500
#define HTTP_NOT_IMPLEMENTED     501
#define HTTP_BAD_GATEWAY         502
#define HTTP_UNAVAILABLE         503
#define HTTP_GATEWAY_TIMEOUT     504
#define HTTP_VERSION             505

// -----------------------------------------------------------------------------------------------------------------------------
// HTTP header representation
// -----------------------------------------------------------------------------------------------------------------------------

typedef struct uhttpinfo {
   const char* method;
   const char* uri;
   const char* query;
   const char* host;
   const char* content_type;
   const char* range;
   const char* interpreter;
   time_t      if_modified_since;
   uint32_t    nResponseCode, startHeader, endHeader, szHeader, clength,
               method_len, uri_len, query_len, host_len, host_vlen, content_type_len, range_len;
   char        flag[8];
} uhttpinfo;

#define U_http_upgrade             UHTTP::http_info.flag[0]
#define U_http_version             UHTTP::http_info.flag[1]
#define U_http_sh_script           UHTTP::http_info.flag[2]
#define U_http_keep_alive          UHTTP::http_info.flag[3]
#define U_http_method_type         UHTTP::http_info.flag[4]
#define U_http_request_check       UHTTP::http_info.flag[5]
#define U_http_is_accept_deflate   UHTTP::http_info.flag[6]
#define U_http_is_connection_close UHTTP::http_info.flag[7]

enum HTTPMethodType { HTTP_POST = '1', HTTP_PUT = '2', HTTP_DELETE = '3', HTTP_GET = '4', HTTP_HEAD = '5', HTTP_OPTIONS = '6' };

#define U_HTTP_BODY(str)             (str).substr(UHTTP::http_info.endHeader, UHTTP::http_info.clength)
#define U_HTTP_HEADER(str)           (str).substr(UHTTP::http_info.startHeader, UHTTP::http_info.szHeader)

#define U_HTTP_METHOD_TO_PARAM       UHTTP::http_info.method, UHTTP::http_info.method_len
#define U_HTTP_METHOD_TO_TRACE       UHTTP::http_info.method_len, UHTTP::http_info.method

#define U_HTTP_URI_TO_TRACE          UHTTP::http_info.uri_len, UHTTP::http_info.uri
#define U_HTTP_URI_TO_PARAM          UHTTP::http_info.uri, UHTTP::http_info.uri_len

#define U_HTTP_QUERY_TO_PARAM        UHTTP::http_info.query, UHTTP::http_info.query_len
#define U_HTTP_QUERY_TO_TRACE        UHTTP::http_info.query_len, UHTTP::http_info.query

#define U_HTTP_URI_QUERY_TO_PARAM    UHTTP::http_info.uri, (UHTTP::http_info.uri_len + UHTTP::http_info.query_len + (UHTTP::http_info.query_len ? 1 : 0))
#define U_HTTP_URI_QUERY_TO_TRACE    (UHTTP::http_info.uri_len + UHTTP::http_info.query_len + (UHTTP::http_info.query_len ? 1 : 0)), UHTTP::http_info.uri

#define U_HTTP_CTYPE_TO_PARAM        UHTTP::http_info.content_type, UHTTP::http_info.content_type_len
#define U_HTTP_CTYPE_TO_TRACE        UHTTP::http_info.content_type_len, UHTTP::http_info.content_type

#define U_HTTP_RANGE_TO_PARAM        UHTTP::http_info.range, UHTTP::http_info.range_len
#define U_HTTP_RANGE_TO_TRACE        UHTTP::http_info.range_len, UHTTP::http_info.range

// The hostname of your server from header's request.
// The difference between U_HTTP_HOST_.. and U_HTTP_VHOST_.. is that
// U_HTTP_HOST_.. can include the «:PORT» text, and U_HTTP_VHOST_.. only the name

#define U_HTTP_HOST_TO_PARAM         UHTTP::http_info.host, UHTTP::http_info.host_len
#define U_HTTP_HOST_TO_TRACE         UHTTP::http_info.host_len, UHTTP::http_info.host

#define U_HTTP_VHOST_TO_PARAM        UHTTP::http_info.host, UHTTP::http_info.host_vlen
#define U_HTTP_VHOST_TO_TRACE        UHTTP::http_info.host_vlen, UHTTP::http_info.host

// HTTP Compare

#define U_HTTP_URI_STRNEQ(str)                                         U_STRNEQ(UHTTP::http_info.uri,          str)
#define U_HTTP_HOST_STRNEQ(str)   (UHTTP::http_info.host_len         ? U_STRNEQ(UHTTP::http_info.host,         str) : false)
#define U_HTTP_QUERY_STRNEQ(str)  (UHTTP::http_info.query_len        ? U_STRNEQ(UHTTP::http_info.query,        str) : false)
#define U_HTTP_CTYPE_STRNEQ(str)  (UHTTP::http_info.content_type_len ? U_STRNEQ(UHTTP::http_info.content_type, str) : false)

#define U_HTTP_URI_EQUAL(str)     (UHTTP::http_info.uri_len == str.size() && memcmp(UHTTP::http_info.uri, str.data(), str.size()) == 0)

// HTTP Access Authentication

#define U_HTTP_REALM "Protected Area"

#define U_MAX_UPLOAD_PROGRESS 32

class UFile;
class UValue;
class UEventFd;
class UCommand;
class UPageSpeed;
class UMimeMultipart;

template <class T> class UHashMap;

class U_EXPORT UHTTP {
public:

   // HTTP strings 

   static void str_allocate();

   static const UString* str_origin;
   static const UString* str_frm_body;
   static const UString* str_indexhtml;
   static const UString* str_websocket;
   static const UString* str_ctype_tsa;
   static const UString* str_frm_header;
   static const UString* str_ctype_html;
   static const UString* str_ctype_soap;
   static const UString* str_frm_websocket;
   static const UString* str_websocket_key1;
   static const UString* str_websocket_key2;
   static const UString* str_websocket_prot;
   static const UString* str_expect_100_continue;

   // HTTP header representation

   static uhttpinfo http_info;

   static const char* ptrH; // "Host"
   static const char* ptrR; // "Range"
   static const char* ptrC; // "Connection"
   static const char* ptrT; // "Content-Type"
   static const char* ptrL; // "Content-Lenght"
   static const char* ptrA; // "Accept-Encoding"
   static const char* ptrI; // "If-Modified-Since"

   // COSTRUTTORE e DISTRUTTORE

   static void ctor();
   static void dtor();

   static void setHTTPMethod(const char* method, uint32_t method_len)
      {
      U_TRACE(0, "UHTTP::setHTTPMethod(%.*S,%u)", method_len, method, method_len)

      http_info.method     = method;
      http_info.method_len = method_len;

      U_INTERNAL_DUMP("method = %.*S", U_HTTP_METHOD_TO_TRACE)
      }

   static void setHTTPMethodType(char c)
      {
      U_TRACE(0, "UHTTP::setHTTPMethodType(%C)", c)

      if (c == 'G') // GET
         {
         U_http_method_type = HTTP_GET;

         U_INTERNAL_ASSERT_EQUALS(http_info.method_len, 3)
         U_INTERNAL_ASSERT(U_STRNEQ(http_info.method, "GET"))
         }
      else if (c == 'P') // POST
         {
         U_http_method_type = HTTP_POST;

         U_INTERNAL_ASSERT_EQUALS(http_info.method_len, 4)
         U_INTERNAL_ASSERT(U_STRNEQ(http_info.method, "POST"))
         }
      else // HEAD
         {
         U_http_method_type = HTTP_HEAD;

         U_INTERNAL_ASSERT_EQUALS(http_info.method_len, 4)
         U_INTERNAL_ASSERT(U_STRNEQ(http_info.method, "HEAD"))
         }

      U_INTERNAL_DUMP("method_type = %C", U_http_method_type)
      }

   static UString getHTTPMethod()
      {
      U_TRACE(0, "UHTTP::getHTTPMethod()")

      if (http_info.method_len)
         {
         UString method((void*)http_info.method, http_info.method_len);

         U_RETURN_STRING(method);
         }

      U_RETURN_STRING(UString::getStringNull());
      }

   static void setHTTPUri(const char* uri, uint32_t uri_len)
      {
      U_TRACE(0, "UHTTP::setHTTPUri(%.*S,%u)", uri_len, uri, uri_len)

      http_info.uri     = uri;
      http_info.uri_len = uri_len;

      U_INTERNAL_DUMP("uri = %.*S", U_HTTP_URI_TO_TRACE)
      }

   static void setHTTPQuery(const char* query, uint32_t query_len)
      {
      U_TRACE(0, "UHTTP::setHTTPQuery(%.*S,%u)", query_len, query, query_len)

      http_info.query     = query;
      http_info.query_len = query_len;

      U_INTERNAL_DUMP("query = %.*S", U_HTTP_QUERY_TO_TRACE)
      }

   static void resetHTTPInfo();

   static void setHTTPInfo(const char* method, uint32_t method_len, const char* uri, uint32_t uri_len)
      {
      U_TRACE(0, "UHTTP::setHTTPInfo(%.*S,%u,%.*S,%u)", method_len, method, method_len, uri_len, uri, uri_len)

      resetHTTPInfo();

      setHTTPMethod(     method, method_len);
      setHTTPMethodType(*method);
      setHTTPUri(           uri,    uri_len);
      setHTTPQuery(           0,          0);
      }

   static void getHTTPInfo(const UString& request, UString& method,       UString& uri);
   static void setHTTPInfo(                  const UString& method, const UString& uri)
      { setHTTPInfo(U_STRING_TO_PARAM(method), U_STRING_TO_PARAM(uri)); }

   static bool    isHTTPRequest(const char* ptr) __pure;
   static bool scanfHTTPHeader( const char* ptr);

   static const char* getHTTPStatus();
   static const char* getHTTPStatusDescription(uint32_t nResponseCode);

   static bool readHTTPRequest(USocket* socket);
   static bool findEndHeader(             const UString& buffer);
   static bool readHTTPHeader( USocket* socket, UString& buffer);
   static bool readHTTPBody(   USocket* socket, UString* buffer, UString& body);

   // TYPE

   static bool isHTTPRequest() { return (U_http_method_type); }

   static bool isHTTPRequestTooLarge()
      {
      U_TRACE(0, "UHTTP::isHTTPRequestTooLarge()")

      U_INTERNAL_ASSERT_MAJOR(limit_request_body,0)

      bool result = (http_info.clength > limit_request_body);

      U_RETURN(result);
      }

   static bool isHttpGETorHEAD()
      {
      U_TRACE(0, "UHTTP::isHttpGETorHEAD()")

      bool result = (U_http_method_type >= HTTP_GET);

      U_RETURN(result);
      }

   static bool isHttpGET()
      {
      U_TRACE(0, "UHTTP::isHttpGET()")

      bool result = (U_http_method_type == HTTP_GET);

      U_RETURN(result);
      }

   static bool isHttpHEAD()
      {
      U_TRACE(0, "UHTTP::isHttpHEAD()")

      bool result = (U_http_method_type == HTTP_HEAD);

      U_RETURN(result);
      }

   static bool isHttpPOST()
      {
      U_TRACE(0, "UHTTP::isHttpPOST()")

      bool result = (U_http_method_type == HTTP_POST);

      U_RETURN(result);
      }

   static bool isHttpPUT()
      {
      U_TRACE(0, "UHTTP::isHttpPUT()")

      bool result = (U_http_method_type == HTTP_PUT);

      U_RETURN(result);
      }

   static bool isHttpDELETE()
      {
      U_TRACE(0, "UHTTP::isHttpDELETE()")

      bool result = (U_http_method_type == HTTP_DELETE);

      U_RETURN(result);
      }

   static bool isTSARequest() __pure;
   static bool isSOAPRequest() __pure;

   static bool isSSIRequest()
      {
      U_TRACE(0, "UHTTP::isSSIRequest()")

      U_INTERNAL_ASSERT(isHTTPRequest())

      bool result = (u_endsWith(U_HTTP_URI_TO_PARAM, U_CONSTANT_TO_PARAM(".shtml")));

      U_RETURN(result);
      }

   // SERVICES

   static UFile* file;
   static UString* alias;
   static UStringRep* pkey;
   static UString* pathname;
   static UString* ssi_alias;
   static UString* request_uri;
   static UString* uri_protected_mask;
   static uint32_t limit_request_body, request_read_timeout;
   static bool virtual_host, enable_caching_by_proxy_servers, telnet_enable;

   static void checkHTTPRequest();
   static void processHTTPGetRequest(USocket* socket, const UString& request);

   static bool checkHTTPOptionsRequest();
   static bool checkHTTPRequestForHeader(const UString& request);
   static bool checkHTTPServletRequest(const char* uri, uint32_t uri_len);
   static bool checkHTTPContentLength(UString& x, uint32_t length, uint32_t pos = U_NOT_FOUND);

   static UString     getDocumentName();
   static UString     getDirectoryURI();
   static UString     getRequestURI(bool bquery);
   static const char* getHTTPHeaderValuePtr(const UString& request, const UString& name, bool nocase) __pure;
   static UString     getHeaderMimeType(const char* content, const char* content_type, uint32_t size, time_t expire);

   // check for HTTP Header X-Forwarded-For: client, proxy1, proxy2 and X-Real-IP: client...

   static UString* real_ip;

   static bool    setRealIP();
   static UString getRemoteIP();

   // request state processing

   static void setHTTPRequestProcessed()
      {
      U_TRACE(0, "UHTTP::setHTTPRequestProcessed()")

      U_INTERNAL_ASSERT(isHTTPRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      U_http_request_check = '4';
      }

   static bool isHTTPRequestAlreadyProcessed()
      {
      U_TRACE(0, "UHTTP::isHTTPRequestAlreadyProcessed()")

      U_INTERNAL_ASSERT(isHTTPRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      bool result = (U_http_request_check == '4');

      U_RETURN(result);
      }

   static void setHTTPRequestInFileCache()
      {
      U_TRACE(0, "UHTTP::setHTTPRequestInFileCache()")

      U_INTERNAL_ASSERT(isHTTPRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      U_http_request_check = '3';
      }

   static bool isHTTPRequestInFileCache()
      {
      U_TRACE(0, "UHTTP::isHTTPRequestInFileCache()")

      U_INTERNAL_ASSERT(isHTTPRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      bool result = (U_http_request_check == '3');

      U_RETURN(result);
      }

   static void setHTTPRequestNeedProcessing()
      {
      U_TRACE(0, "UHTTP::setHTTPRequestNeedProcessing()")

      U_INTERNAL_ASSERT(isHTTPRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      U_http_request_check = '2';
      }

   static bool isHTTPRequestNeedProcessing()
      {
      U_TRACE(0, "UHTTP::isHTTPRequestNeedProcessing()")

      U_INTERNAL_ASSERT(isHTTPRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      bool result = (U_http_request_check == '2');

      U_RETURN(result);
      }

   static void setHTTPRequestNotFound()
      {
      U_TRACE(0, "UHTTP::setHTTPRequestNotFound()")

      U_INTERNAL_ASSERT(isHTTPRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      U_http_request_check = '\0';
      }

   static bool isHTTPRequestNotFound()
      {
      U_TRACE(0, "UHTTP::isHTTPRequestNotFound()")

      U_INTERNAL_ASSERT(isHTTPRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      bool result = (U_http_request_check == '\0');

      U_RETURN(result);
      }

   static void setHTTPRequestForbidden()
      {
      U_TRACE(0, "UHTTP::setHTTPRequestForbidden()")

      U_INTERNAL_ASSERT(isHTTPRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      U_http_request_check = '1';
      }

   static bool isHTTPRequestForbidden()
      {
      U_TRACE(0, "UHTTP::isHTTPRequestForbidden()")

      U_INTERNAL_ASSERT(isHTTPRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      bool result = (U_http_request_check == '1');

      U_RETURN(result);
      }

   // -----------------------------------------------------------------------
   // FORM
   // -----------------------------------------------------------------------
   // retrieve information on specific HTML form elements
   // (such as checkboxes, radio buttons, and text fields), or uploaded files
   // -----------------------------------------------------------------------

   static UValue* json;
   static UString* tmpdir;
   static UString* qcontent;
   static UMimeMultipart* formMulti;
   static UVector<UString>* form_name_value;

   static uint32_t processHTTPForm();
   static void     resetForm(bool brmdir);
   static void     getFormValue(UString& value, uint32_t n);

   // param: "[ data expire path domain secure HttpOnly ]"
   // ----------------------------------------------------------------------------------------------------------------------------
   // string -- Data to put into the cookie        -- must
   // int    -- Lifetime of the cookie in HOURS    -- must (0 -> valid until browser exit)
   // string -- Path where the cookie can be used  --  opt
   // string -- Domain which can read the cookie   --  opt
   // bool   -- Secure mode                        --  opt
   // bool   -- Only allow HTTP usage              --  opt
   // ----------------------------------------------------------------------------------------------------------------------------
   // RET: Set-Cookie: ulib_sid=data&expire&HMAC-MD5(data&expire); expires=expire(GMT); path=path; domain=domain; secure; HttpOnly

   static UString getHTTPCookie();
   static UString setHTTPCookie(const UString& param);

   // CGI

   static UString* geoip;
   static UCommand* pcmd;
   static char cgi_dir[U_PATH_MAX];

   static bool isCGIRequest()
      {
      U_TRACE(0, "UHTTP::isCGIRequest()")

      U_INTERNAL_DUMP("http_info.interpreter = %S cgi_dir = %S", http_info.interpreter, cgi_dir)

      bool result = (http_info.interpreter || cgi_dir[0]);

      U_RETURN(result);
      }

   static bool    processCGIOutput();
   static UString getCGIEnvironment();
   static bool    checkForCGIRequest();
   static bool    processCGIRequest(UCommand* pcmd, UString* penvironment, bool async, bool process_output);
   static void    setHTTPCgiResponse(int nResponseCode, bool header_content_length, bool header_content_type, bool content_encoding);

   // URI PROTECTED

   static UString* htpasswd;
   static UString* htdigest;
   static bool digest_authentication; // authentication method (digest|basic), for example directory listing...
   static UVector<UIPAllow*>* vallow_IP;

   static bool    checkUriProtected();
   static UString getUserHA1(const UString& user, const UString& realm);
   static bool    isUserAuthorized(const UString& user, const UString& password);

   // UPLOAD PROGRESS

   typedef struct upload_progress {
      char uuid[32];
      in_addr_t client;
      int byte_read, count;
   } upload_progress;

   static upload_progress* ptr_upload_progress;

   static UString getUploadProgress();

   // USP (ULib Servlet Page)

   class UServletPage : public UDynamic {
   public:

   iPFpv runDynamicPage;

   // COSTRUTTORI

   UServletPage()
      {
      U_TRACE_REGISTER_OBJECT(0, UServletPage, "")

      runDynamicPage = 0;
      }

   ~UServletPage()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UServletPage)
      }

#ifdef DEBUG
   const char* dump(bool reset) const U_EXPORT;
#endif

   private:
   UServletPage(const UServletPage&) : UDynamic() {}
   UServletPage& operator=(const UServletPage&)   { return *this; }
   };

   static void* argument;
   static UServletPage* usp_page;
   static UHashMap<UServletPage*>* usp_pages;

   static void initUSP();

   // ------------------------------
   // argument value for usp mudule:
   // ------------------------------
   //  0 -> init
   // -1 -> reset
   // -2 -> destroy
   // ------------------------------

   static void callRunDynamicPage(int arg);

   // CSP (C Servlet Page)

   typedef int (*iPFipvc)(int,const char**);

   class UCServletPage {
   public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   int size;
   void* relocated;
   iPFipvc prog_main;

   // COSTRUTTORI

   UCServletPage()
      {
      U_TRACE_REGISTER_OBJECT(0, UCServletPage, "")

      relocated = 0;
      prog_main = 0;
      }

   ~UCServletPage()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UCServletPage)

      if (relocated) U_FREE_GEN(relocated,size);
      }

   bool compile(const UString& program);

   // STREAM

#ifdef DEBUG
   const char* dump(bool reset) const U_EXPORT;
#endif

   private:
   UCServletPage(const UCServletPage&)            {}
   UCServletPage& operator=(const UCServletPage&) { return *this; }
   };

   static UCServletPage* csp_page;
   static UHashMap<UCServletPage*>* csp_pages;

   static void initCSP();

   typedef void (*vPFstr)(UString&);

#ifdef HAVE_PAGE_SPEED // (Google Page Speed)
   typedef void (*vPFpcstr)(const char*, UString&);

   class UPageSpeed : public UDynamic {
   public:

   vPFpcstr minify_html;
   vPFstr optimize_gif, optimize_png, optimize_jpg;

   // COSTRUTTORI

   UPageSpeed()
      {
      U_TRACE_REGISTER_OBJECT(0, UPageSpeed, "")

      minify_html = 0;
      optimize_gif = optimize_png = optimize_jpg = 0;
      }

   ~UPageSpeed()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UPageSpeed)
      }

#ifdef DEBUG
   const char* dump(bool reset) const U_EXPORT;
#endif

   private:
   UPageSpeed(const UPageSpeed&) : UDynamic() {}
   UPageSpeed& operator=(const UPageSpeed&)   { return *this; }
   };

   static UPageSpeed* page_speed;
#endif

#ifdef HAVE_V8 // (Google V8 JavaScript Engine)

   class UV8JavaScript : public UDynamic {
   public:

   vPFstr runv8;

   // COSTRUTTORI

   UV8JavaScript()
      {
      U_TRACE_REGISTER_OBJECT(0, UV8JavaScript, "")

      runv8 = 0;
      }

   ~UV8JavaScript()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UV8JavaScript)
      }

#ifdef DEBUG
   const char* dump(bool reset) const U_EXPORT;
#endif

   private:
   UV8JavaScript(const UV8JavaScript&) : UDynamic() {}
   UV8JavaScript& operator=(const UV8JavaScript&)   { return *this; }
   };

   static UV8JavaScript* v8_javascript;
#endif


   // REWRITE RULE

   class RewriteRule {
   public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

#ifdef HAVE_PCRE
   UPCRE key;
#endif
   UString replacement;

   RewriteRule(const UString& _key, const UString& _replacement) :
#  ifdef HAVE_PCRE
      key(_key, PCRE_FOR_REPLACE),
#  endif
       replacement(_replacement)
      {
      U_TRACE_REGISTER_OBJECT(0, RewriteRule, "%.*S,%.*S", U_STRING_TO_TRACE(_key), U_STRING_TO_TRACE(_replacement))

#  ifdef HAVE_PCRE
      key.study();
#  endif
      }

   ~RewriteRule()
      {
      U_TRACE_UNREGISTER_OBJECT(0, RewriteRule)
      }

#ifdef DEBUG
   const char* dump(bool reset) const U_EXPORT;
#endif

   private:
   RewriteRule(const RewriteRule&)            {}
   RewriteRule& operator=(const RewriteRule&) { return *this; }
   };

   static UVector<RewriteRule*>* vRewriteRule;

   // FILE SYSTEM CACHE

   class UFileCacheData {
   public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   UVector<UString>* array; // content, header, deflate(content, header)
   time_t mtime;            // time of last modification
   time_t expire;           // expire time of the entry
   uint32_t size;           // size content
   int wd;                  // if directory it is a "watch list" associated with an inotify instance...
   mode_t mode;             // file type
   int mime_index;          // index file mime type

   // COSTRUTTORI

    UFileCacheData();
   ~UFileCacheData();

   // STREAM

#ifdef DEBUG
   friend ostream& operator<<(ostream& os, const UFileCacheData& d)
      {
      U_TRACE(0, "UFileCacheData::operator<<(%p,%p)", &os, &d)

      os.put('{');
      os.put(' ');
      os << d.wd;
      os.put(' ');
      os << d.size;
      os.put(' ');
      os << d.mode;
      os.put(' ');
      os << d.mtime;
      os.put(' ');
      os << d.expire;
      os.put(' ');
      os << d.mime_index;
      os.put(' ');
   // os << d.array;
   // os.put(' ');
      os.put('}');

      return os;
      }

   const char* dump(bool reset) const U_EXPORT;
#endif

   private:
   UFileCacheData(const UFileCacheData&)            {}
   UFileCacheData& operator=(const UFileCacheData&) { return *this; }
   };

   static int inotify_wd;
   static UString* cache_file_mask;
   static UFileCacheData* file_data;
   static UHashMap<UFileCacheData*>* cache_file;

   static bool isDataFromCache()
      {
      U_TRACE(0, "UHTTP::isDataFromCache()")

      U_INTERNAL_ASSERT_POINTER(file_data)

      bool result = (file_data->array != 0);

      U_RETURN(result);
      }

   static bool isDataCompressFromCache()
      {
      U_TRACE(0, "UHTTP::isDataCompressFromCache()")

      U_INTERNAL_ASSERT_POINTER(file_data)
      U_INTERNAL_ASSERT_POINTER(file_data->array)

      bool result = (file_data->array->size() > 2);

      U_RETURN(result);
      }

   static void    in_READ();
   static bool    isFileInCache();
   static void    renewDataCache();
   static void    checkFileForCache();
   static UString getDataFromCache(bool header, bool deflate);

   // Accept-Language: en-us,en;q=0.5
   // ----------------------------------------------------
   // take only the first 2 character (it, en, de fr, ...)

   static const char* getAcceptLanguage() __pure;

   // set HTTP main error message

   static void setHTTPNotFound();
   static void setHTTPForbidden();
   static void setHTTPBadRequest();
   static void setHTTPUnAuthorized();
   static void setHTTPInternalError();
   static void setHTTPServiceUnavailable();

   // set HTTP response message

   static UString getHTTPHeaderForResponse(int nResponseCode, const UString& content);
   static void    setHTTPResponse(int nResponseCode, const UString* content_type, const UString* body);
   static void    setHTTPRedirectResponse(UString& ext, const char* ptr_location, uint32_t len_location);

private:
   static UString getHTMLDirectoryList() U_NO_EXPORT;

   static bool openFile() U_NO_EXPORT;
   static void in_CREATE() U_NO_EXPORT;
   static void in_DELETE() U_NO_EXPORT;
   static void checkPath() U_NO_EXPORT;
   static bool processFileCache() U_NO_EXPORT;
   static void processRewriteRule() U_NO_EXPORT;
   static bool initUploadProgress(int byte_read) U_NO_EXPORT;
   static void updateUploadProgress(int byte_read) U_NO_EXPORT;
   static bool setCGIShellScript(UString& command) U_NO_EXPORT;

   static void manageDataForCache() U_NO_EXPORT;
   static bool checkHTTPGetRequestIfRange(const UString& etag) U_NO_EXPORT;
   static bool processHTTPAuthorization(const UString& request) U_NO_EXPORT;
   static void _callRunDynamicPage(UStringRep* key, void* value) U_NO_EXPORT;
   static int  sortHTTPRange(const void* a, const void* b) __pure U_NO_EXPORT;
   static void putDataInCache(const UString& fmt, UString& content) U_NO_EXPORT;
   static void getInotifyPathDirectory(UStringRep* key, void* value) U_NO_EXPORT;
   static bool checkHTTPGetRequestIfModified(const UString& request) U_NO_EXPORT;
   static void checkInotifyForCache(int wd, char* name, uint32_t len) U_NO_EXPORT;
   static bool splitCGIOutput(const char*& ptr1, const char* ptr2, uint32_t endHeader, UString& ext) U_NO_EXPORT;
   static bool checkHTTPGetRequestForRange(uint32_t& start, uint32_t& size, UString& ext, const UString& data) U_NO_EXPORT;
   static void setResponseForRange(uint32_t start, uint32_t end, uint32_t& size, uint32_t header, UString& ext) U_NO_EXPORT;

   UHTTP(const UHTTP&)            {}
   UHTTP& operator=(const UHTTP&) { return *this; }
};

#endif
