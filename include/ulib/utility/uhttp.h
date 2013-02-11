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
#include <ulib/internal/chttp.h>
#include <ulib/dynamic/dynamic.h>
#include <ulib/utility/string_ext.h>
#include <ulib/utility/data_session.h>

#ifdef USE_LIBPCRE
#  include <ulib/pcre/pcre.h>
#else
#  include <ulib/container/vector.h>
#endif

#ifndef U_HTTP_CACHE_REQUEST_DISABLE
#define U_HTTP_CACHE_REQUEST
#endif

#define U_HTTP_REALM "Protected Area" // HTTP Access Authentication

#define U_MAX_UPLOAD_PROGRESS         16
#define U_MIN_SIZE_FOR_PARTIAL_WRITE (16U * 1024U)

#define U_HTTP_BODY(str)                    (str).substr(u_http_info.endHeader, u_http_info.clength)
#define U_HTTP_HEADER(str)                  (str).substr(u_http_info.startHeader, u_http_info.szHeader)
#define U_HTTP_URI_EQUAL(str)               ((str).equal(U_HTTP_URI_TO_PARAM))
#define U_HTTP_URI_DOSMATCH(mask,len,flags) (u_dosmatch_with_OR(U_HTTP_URI_TO_PARAM, mask, len, flags))
#define U_HTTP_URI_OR_ALIAS_STRNEQ(req,str) (U_HTTP_URI_STRNEQ(str) || (req).equal(U_CONSTANT_TO_PARAM(str)))

class UFile;
class UEventFd;
class UCommand;
class UPageSpeed;
class UHttpPlugIn;
class UMimeMultipart;

template <class T> class UHashMap;

class U_EXPORT UHTTP {
public:

   // HTTP strings 

   static const UString* str_origin;
   static const UString* str_frm_body;
   static const UString* str_indexhtml;
   static const UString* str_ctype_tsa;
   static const UString* str_frm_header;
   static const UString* str_ctype_html;
   static const UString* str_ctype_soap;
   static const UString* str_ulib_header;
   static const UString* str_strict_transport_security;

   static void str_allocate();

   static const char* ptrH; // "Host"
   static const char* ptrR; // "Range"
   static const char* ptrA; // "Accept"
   static const char* ptrK; // "Cookie"
   static const char* ptrF; // "Referer"
   static const char* ptrT; // "Content-"
   static const char* ptrC; // "Connection"
   static const char* ptrU; // "User-Agent"
   static const char* ptrI; // "If-Modified-Since"
   static const char* ptrP; // "X-Real-IP"
   static const char* ptrX; // "X-Forwarded-For"
   static const char* ptrS; // "Sec-WebSocket-Key"

   // COSTRUTTORE e DISTRUTTORE

   static void ctor();
   static void dtor();

   static int  isValidRequest(const char* ptr) __pure;
   static bool scanfHeader(const char* ptr, uint32_t size);

   static const char* getStatus();
   static const char* getStatusDescription(uint32_t nResponseCode);

   static bool readRequest();
   static bool findEndHeader(         const UString& buffer);
   static bool readHeader( USocket* socket, UString& buffer);
   static bool readBody(   USocket* socket, UString* buffer, UString& body);

   // TYPE

   static bool isGETorHEAD()
      {
      U_TRACE(0, "UHTTP::isGETorHEAD()")

      bool result = (U_http_method_type >= HTTP_GET);

      U_RETURN(result);
      }

   static bool isGET()
      {
      U_TRACE(0, "UHTTP::isGET()")

      bool result = (U_http_method_type == HTTP_GET);

      U_RETURN(result);
      }

   static bool isHEAD()
      {
      U_TRACE(0, "UHTTP::isHEAD()")

      bool result = (U_http_method_type == HTTP_HEAD);

      U_RETURN(result);
      }

   static bool isPOST()
      {
      U_TRACE(0, "UHTTP::isPOST()")

      bool result = (U_http_method_type == HTTP_POST);

      U_RETURN(result);
      }

   static bool isPUT()
      {
      U_TRACE(0, "UHTTP::isPUT()")

      bool result = (U_http_method_type == HTTP_PUT);

      U_RETURN(result);
      }

   static bool isDELETE()
      {
      U_TRACE(0, "UHTTP::isDELETE()")

      bool result = (U_http_method_type == HTTP_DELETE);

      U_RETURN(result);
      }

   static bool isCOPY()
      {
      U_TRACE(0, "UHTTP::isCOPY()")

      bool result = (U_http_method_type == HTTP_COPY);

      U_RETURN(result);
      }

   static bool isTSARequest() __pure;
   static bool isSOAPRequest() __pure;

   static bool isValidRequest() { return (U_http_method_type != 0); }

   // SERVICES

   static UString* uri;
   static UString* alias;
   static UString* cbuffer;
   static UString* pathname;
   static UString* request_uri;
   static UString* global_alias;
   static UVector<UString>* valias;
   static UString* uri_protected_mask;
   static UString* uri_request_cert_mask;
   static UString* maintenance_mode_page;
   static UString* string_HTTP_Variables;
   static UString* uri_strict_transport_security_mask;

   static UFile*   file;
   static UFile*   apache_like_log;
   static bool     virtual_host, enable_caching_by_proxy_servers, telnet_enable, bsendfile;
   static uint32_t npathinfo, limit_request_body, request_read_timeout, min_size_for_sendfile, range_start, range_size;

   static int  manageRequest();
   static void processGetRequest();
   static bool callService(const UString& path);
   static void writeApacheLikeLog(bool prepare);
   static void manageServletRequest(bool as_service);
   static bool checkRequestForHeader(const UString& request);
   static bool checkContentLength(UString& x, uint32_t length, uint32_t pos = U_NOT_FOUND);

   static uint32_t getUserAgent();
   static UString  getRequestURI();
   static UString  getDocumentName();
   static UString  getDirectoryURI();
   static UString  getRequestURIWithQuery();
   static UString  getHeaderMimeType(const char* content, const char* content_type, uint32_t size, time_t expire);

   static void endRequestProcessing()
      {
      U_TRACE(0, "UHTTP::endRequestProcessing()")

      if (UHTTP::apache_like_log) UHTTP::writeApacheLikeLog(false);

      U_http_method_len = 0; // NB: this mark the end of http request processing...
      }

   static const char* getHeaderValuePtr(                        const UString& name, bool nocase) __pure;
   static const char* getHeaderValuePtr(const UString& request, const UString& name, bool nocase) __pure;

   // set HTTP main error message

   static void setNotFound();
   static void setForbidden();
   static void setBadMethod();
   static void setBadRequest();
   static void setUnAuthorized();
   static void setInternalError();
   static void setServiceUnavailable();

   // set HTTP response message

   static void setNoResponse();
   static void setResponse(const UString* content_type, const UString* body);
   static void setRedirectResponse(int mode, const UString& ext, const char* ptr_location, uint32_t len_location);

   // get HTTP response message

   static UString getUrlEncodedForResponse(const char* fmt);
   static UString getHeaderForResponse(const UString& content, bool connection_close);

#ifdef U_HTTP_CACHE_REQUEST
   static void  clearRequestCache();
   static int   checkRequestCache();
   static void manageRequestCache();
#endif

   // manage HTTP request service

   typedef struct service_info {
      const char* name;
      uint32_t    len;
      vPF         function;
   } service_info;

#  define  GET_ENTRY(name) {#name,U_CONSTANT_SIZE(#name), GET_##name}
#  define POST_ENTRY(name) {#name,U_CONSTANT_SIZE(#name),POST_##name}

   static void manageRequest(UString* request_uri, UString* client_address,
                             const struct UHTTP::service_info*  GET_table, uint32_t n1,
                             const struct UHTTP::service_info* POST_table, uint32_t n2);

   // request state processing

#  define U_HTTP_REQUEST_IS_ALREADY_PROCESSED  '4'
#  define U_HTTP_REQUEST_IS_IN_FILE_CACHE      '3'
#  define U_HTTP_REQUEST_NEED_PROCESSING       '2'
#  define U_HTTP_REQUEST_IS_FORBIDDEN          '1'
#  define U_HTTP_REQUEST_IS_NOT_FOUND           0

   static void setRequestProcessed()
      {
      U_TRACE(0, "UHTTP::setRequestProcessed()")

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      U_http_request_check = U_HTTP_REQUEST_IS_ALREADY_PROCESSED;
      }

   static bool isRequestAlreadyProcessed()
      {
      U_TRACE(0, "UHTTP::isRequestAlreadyProcessed()")

      U_INTERNAL_DUMP("method = %.*S method_type = %C uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_http_method_type, U_HTTP_URI_TO_TRACE)

      U_INTERNAL_ASSERT(isValidRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      bool result = (U_http_request_check == U_HTTP_REQUEST_IS_ALREADY_PROCESSED);

      U_RETURN(result);
      }

   static bool isRequestRedirected()
      {
      U_TRACE(0, "UHTTP::isRequestRedirected()")

      U_INTERNAL_DUMP("method = %.*S method_type = %C uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_http_method_type, U_HTTP_URI_TO_TRACE)

      U_INTERNAL_ASSERT(isValidRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C u_http_info.nResponseCode = %d", U_http_request_check, u_http_info.nResponseCode)

      bool result = (U_http_request_check       == U_HTTP_REQUEST_IS_ALREADY_PROCESSED &&
                     (u_http_info.nResponseCode == HTTP_MOVED_TEMP                     ||
                      u_http_info.nResponseCode == HTTP_NETWORK_AUTHENTICATION_REQUIRED));

      U_RETURN(result);
      }

   static void setRequestInFileCache()
      {
      U_TRACE(0, "UHTTP::setRequestInFileCache()")

      U_INTERNAL_DUMP("method = %.*S method_type = %C uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_http_method_type, U_HTTP_URI_TO_TRACE)

      U_INTERNAL_ASSERT(isValidRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      U_http_request_check = U_HTTP_REQUEST_IS_IN_FILE_CACHE;
      }

   static bool isRequestInFileCache()
      {
      U_TRACE(0, "UHTTP::isRequestInFileCache()")

      U_INTERNAL_DUMP("method = %.*S method_type = %C uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_http_method_type, U_HTTP_URI_TO_TRACE)

      U_INTERNAL_ASSERT(isValidRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      bool result = (U_http_request_check == U_HTTP_REQUEST_IS_IN_FILE_CACHE);

      U_RETURN(result);
      }

   static void setRequestNeedProcessing()
      {
      U_TRACE(0, "UHTTP::setRequestNeedProcessing()")

      U_INTERNAL_DUMP("method = %.*S method_type = %C uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_http_method_type, U_HTTP_URI_TO_TRACE)

      U_INTERNAL_ASSERT(isValidRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      U_http_request_check = U_HTTP_REQUEST_NEED_PROCESSING;
      }

   static bool isRequestNeedProcessing()
      {
      U_TRACE(0, "UHTTP::isRequestNeedProcessing()")

      U_INTERNAL_DUMP("method = %.*S method_type = %C uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_http_method_type, U_HTTP_URI_TO_TRACE)

      U_INTERNAL_ASSERT(isValidRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      bool result = (U_http_request_check == U_HTTP_REQUEST_NEED_PROCESSING);

      U_RETURN(result);
      }

   static void setRequestNotFound()
      {
      U_TRACE(0, "UHTTP::setRequestNotFound()")

      U_INTERNAL_DUMP("method = %.*S method_type = %C uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_http_method_type, U_HTTP_URI_TO_TRACE)

      U_INTERNAL_ASSERT(isValidRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      U_http_request_check = U_HTTP_REQUEST_IS_NOT_FOUND;
      }

   static bool isRequestNotFound()
      {
      U_TRACE(0, "UHTTP::isRequestNotFound()")

      U_INTERNAL_DUMP("method = %.*S method_type = %C uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_http_method_type, U_HTTP_URI_TO_TRACE)

      U_INTERNAL_ASSERT(isValidRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      bool result = (U_http_request_check == U_HTTP_REQUEST_IS_NOT_FOUND);

      U_RETURN(result);
      }

   static void setRequestForbidden()
      {
      U_TRACE(0, "UHTTP::setRequestForbidden()")

      U_INTERNAL_DUMP("method = %.*S method_type = %C uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_http_method_type, U_HTTP_URI_TO_TRACE)

      U_INTERNAL_ASSERT(isValidRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      U_http_request_check = U_HTTP_REQUEST_IS_FORBIDDEN;
      }

   static bool isRequestForbidden()
      {
      U_TRACE(0, "UHTTP::isRequestForbidden()")

      U_INTERNAL_DUMP("method = %.*S method_type = %C uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_http_method_type, U_HTTP_URI_TO_TRACE)

      U_INTERNAL_ASSERT(isValidRequest())

      U_INTERNAL_DUMP("U_http_request_check = %C", U_http_request_check)

      bool result = (U_http_request_check == U_HTTP_REQUEST_IS_FORBIDDEN);

      U_RETURN(result);
      }

   // -----------------------------------------------------------------------
   // FORM
   // -----------------------------------------------------------------------
   // retrieve information on specific HTML form elements
   // (such as checkboxes, radio buttons, and text fields), or uploaded files
   // -----------------------------------------------------------------------

   static UString* tmpdir;
   static UString* qcontent;
   static UMimeMultipart* formMulti;
   static UVector<UString>* form_name_value;

   static uint32_t processForm();
   static void     resetForm(bool brmdir);

   static void    getFormValue(UString& value, const char* name, uint32_t len);
   static void    getFormValue(UString& value,                                                 uint32_t pos);
   static UString getFormValue(                const char* name, uint32_t len, uint32_t start,               uint32_t end);
   static void    getFormValue(UString& value, const char* name, uint32_t len, uint32_t start, uint32_t pos, uint32_t end);

   // COOKIE

   static UString* set_cookie;
   static UString* cookie_option;

   static bool getCookie(UString* cookie);

   static void addSetCookie(const UString& cookie);

   // param: "[ data expire path domain secure HttpOnly ]"
   // -----------------------------------------------------------------------------------------------------------------------------------
   // string -- key_id or data to put in cookie    -- must
   // int    -- lifetime of the cookie in HOURS    -- must (0 -> valid until browser exit)
   // string -- path where the cookie can be used  --  opt
   // string -- domain which can read the cookie   --  opt
   // bool   -- secure mode                        --  opt
   // bool   -- only allow HTTP usage              --  opt
   // -----------------------------------------------------------------------------------------------------------------------------------
   // RET: Set-Cookie: ulib.s<counter>=data&expire&HMAC-MD5(data&expire); expires=expire(GMT); path=path; domain=domain; secure; HttpOnly

   static void setCookie(const UString& param);

   // HTTP SESSION

   static UString* keyID;
   static void* db_session;
   static UDataSession* data_session;
   static UDataSession* data_storage;
   static uint32_t sid_counter_gen, sid_counter_cur;

   static bool isNewSession()
      {
      U_TRACE(0, "UHTTP::isNewSession()")

      if (data_session &&
          data_session->last_access == data_session->creation)
         {
         U_RETURN(true);
         }

      U_RETURN(false);
      }

   static UString getSessionCreationTime()
      {
      U_TRACE(0, "UHTTP::getSessionCreationTime()")

      UString x(40U);

      if (data_session) x.snprintf("%#5D", data_session->creation);

      U_RETURN_STRING(x);
      }

   static UString getSessionLastAccessedTime()
      {
      U_TRACE(0, "UHTTP::getSessionLastAccessedTime()")

      UString x(40U);

      if (data_session) x.snprintf("%#5D", data_session->last_access);

      U_RETURN_STRING(x);
      }

   static void removeDataSession();
   static void setSessionCookie(UString* param);
   static bool initSession(const char* location, uint32_t sz);

   static bool getDataSession(uint32_t index, UString* value);
   static void putDataSession(uint32_t index, const char* val, uint32_t sz);

   static bool getDataStorage(uint32_t index, UString* value);
   static void putDataStorage(uint32_t index, const char* val, uint32_t sz);

   // HTML Pagination

   static uint32_t num_page_end,
                   num_page_cur,
                   num_page_start,
                   num_item_tot,
                   num_item_for_page;

   static UString getLinkPagination();

   static void addLinkPagination(UString& link, uint32_t num_page)
      {
      U_TRACE(0, "UHTTP::addLinkPagination(%.*S,%u)", U_STRING_TO_TRACE(link), num_page)

      UString x(100U);

      U_INTERNAL_DUMP("num_page_cur = %u", num_page_cur)

      if (num_page == num_page_cur) x.snprintf("<span class=\"pnow\">%u</span>",             num_page);
      else                          x.snprintf("<a href=\"?page=%u\" class=\"pnum\">%u</a>", num_page, num_page);

      (void) link.append(x);
             link.push_back(' ');
      }

   // CGI

   typedef struct ucgi {
      char        sh_script;
      char        dir[503];
      const char* interpreter;
   } ucgi;

   static UString* geoip;
   static UString* fcgi_uri_mask;
   static UString* scgi_uri_mask;

   static bool isCompressable()
      {
      U_TRACE(0, "UHTTP::isCompressable()")

      U_INTERNAL_ASSERT(isValidRequest())

      U_INTERNAL_DUMP("u_http_info.clength = %u U_http_is_accept_gzip = %C", u_http_info.clength, U_http_is_accept_gzip)

      bool result = (u_http_info.clength > 150 && U_http_is_accept_gzip); // NB: google advice is 150...

      U_RETURN(result);
      }

   static UString getCGIEnvironment(bool bHTTP_Variables);

   static bool processCGIOutput();
   static bool isGenCGIRequest() __pure;
   static void setCgiResponse(bool header_content_type, bool bcompress, bool connection_close);
   static bool processCGIRequest(UCommand& cmd, UString* environment, const char* cgi_dir, bool& async);

   // URI PROTECTED

   static UString* htpasswd;
   static UString* htdigest;
   static bool digest_authentication; // authentication method (digest|basic), for example directory listing...
   static UVector<UIPAllow*>* vallow_IP;

   static UString getUserHA1(const UString& user, const UString& realm);

   static bool checkUriProtected();
   static bool isUserAuthorized(const UString& user, const UString& password);

   // USP (ULib Servlet Page)

   class UServletPage : public UDynamic {
   public:

   iPFpv runDynamicPage;
   bool alias;

   // COSTRUTTORI

   UServletPage()
      {
      U_TRACE_REGISTER_OBJECT(0, UServletPage, "")

      runDynamicPage = 0;
      alias          = false;
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

   static UStringRep*   usp_page_key;
   static UServletPage* usp_page_to_check;

   static UServletPage* getUSP(const UString& key);

   static void callEndForAllUSP( UStringRep* key, void* value);
   static void callInitForAllUSP(UStringRep* key, void* value);

   typedef struct upload_progress {
      char uuid[32];
      in_addr_t client;
      uint32_t user_agent;
      int byte_read, count;
   } upload_progress;

   static uint32_t upload_progress_index;
   static upload_progress* ptr_upload_progress;

   static UString getUploadProgress();

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

      if (relocated) UMemoryPool::_free(relocated, size, 1);
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

   typedef void (*vPFstr)(UString&);

#ifdef USE_PAGE_SPEED // (Google Page Speed)
   typedef void (*vPFpcstr)(const char*, UString&);

   class UPageSpeed : public UDynamic {
   public:

   vPFpcstr minify_html;
   vPFstr optimize_gif, optimize_png, optimize_jpg;

   // COSTRUTTORI

   UPageSpeed()
      {
      U_TRACE_REGISTER_OBJECT(0, UPageSpeed, "")

      minify_html  = 0;
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

#ifdef USE_LIBV8 // (Google V8 JavaScript Engine)
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

#ifdef USE_LIBPCRE
   UPCRE key;
#endif
   UString replacement;

   RewriteRule(const UString& _key, const UString& _replacement) :
#  ifdef USE_LIBPCRE
      key(_key, PCRE_FOR_REPLACE),
#  endif
       replacement(_replacement)
      {
      U_TRACE_REGISTER_OBJECT(0, RewriteRule, "%.*S,%.*S", U_STRING_TO_TRACE(_key), U_STRING_TO_TRACE(_replacement))

#  ifdef USE_LIBPCRE
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

   // DOCUMENT ROOT CACHE

   class UFileCacheData {
   public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   void* ptr;               // data
   UVector<UString>* array; // content, header, gzip(content, header)
   time_t mtime;            // time of last modification
   time_t expire;           // expire time of the entry
   uint32_t size;           // size content
   int wd;                  // if directory it is a "watch list" associated with an inotify instance...
   mode_t mode;             // file type
   int mime_index;          // index file mime type
   int fd;                  // file descriptor

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
   // os << d.ptr;
   // os.put(' ');
   // os << d.array;
   // os.put(' ');
      os << d.mtime;
      os.put(' ');
      os << d.expire;
      os.put(' ');
      os << d.size;
      os.put(' ');
      os << d.wd;
      os.put(' ');
      os << d.mode;
      os.put(' ');
      os << d.mime_index;
      os.put(' ');
      os << d.fd;
      os.put(' ');
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
   static UString* cache_file_store;
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

   static void in_READ();
   static bool isFileInCache();
   static void renewDataCache();
   static void checkFileForCache();
   static void setCacheForDocumentRoot();

   static UString         getDataFromCache(bool header, bool gzip);
   static UFileCacheData* getFileInCache(const char* path, uint32_t len);

   // X-Sendfile

   static bool XSendfile(UString& pathname, const UString& ext);

private:
   static UString getHTMLDirectoryList() U_NO_EXPORT;

#ifdef DEBUG
   static bool cache_file_check_memory();
   static void check_memory(UStringRep* key, void* value) U_NO_EXPORT;
#endif

#ifdef U_HTTP_UPLOAD_PROGRESS_SUPPORT
   static bool   initUploadProgress(int byte_read) U_NO_EXPORT;
   static void updateUploadProgress(int byte_read) U_NO_EXPORT;
#endif

#if defined(HAVE_SYS_INOTIFY_H) && defined(U_HTTP_INOTIFY_SUPPORT)
   static void getInotifyPathDirectory(UStringRep* key, void* value) U_NO_EXPORT;
   static void checkInotifyForCache(int wd, char* name, uint32_t len) U_NO_EXPORT;
#endif

   static void in_CREATE() U_NO_EXPORT;
   static void in_DELETE() U_NO_EXPORT;
   static bool processFileCache() U_NO_EXPORT;
   static void processRewriteRule() U_NO_EXPORT;
   static bool checkPath(uint32_t len) U_NO_EXPORT;
   static void checkPath(bool bnostat) U_NO_EXPORT;
   static void setCGIShellScript(UString& command) U_NO_EXPORT;
   static bool runDynamicPage(UString* penvironment) U_NO_EXPORT;
   static void manageBufferResize(const char* rpointer1, const char* rpointer2) U_NO_EXPORT;

   static void deleteSession() U_NO_EXPORT;
   static void manageDataForCache() U_NO_EXPORT;
   static bool processAuthorization() U_NO_EXPORT;
   static bool checkGetRequestIfModified() U_NO_EXPORT;
   static bool isAlias(UServletPage* usp_page) U_NO_EXPORT;
   static bool isRequestTooLarge(UString& buffer) U_NO_EXPORT;
   static void removeDataSession(const UString& token) U_NO_EXPORT;
   static void checkIfUSP(UStringRep* key, void* value) U_NO_EXPORT;
   static void checkIfAlias(UStringRep* key, void* value) U_NO_EXPORT;
   static bool checkGetRequestIfRange(const UString& etag) U_NO_EXPORT;
   static int  sortRange(const void* a, const void* b) __pure U_NO_EXPORT;
   static void add_HTTP_Variables(UStringRep* key, void* value) U_NO_EXPORT;
   static void putDataInCache(const UString& fmt, UString& content) U_NO_EXPORT;
   static void processGetRequest(const UString& etag, UString& ext) U_NO_EXPORT;
   static int  checkGetRequestForRange(UString& ext, const UString& data) U_NO_EXPORT;
   static void checkDataSession(const UString& token, time_t expire, bool check) U_NO_EXPORT;
   static void setResponseForRange(uint32_t start, uint32_t end, uint32_t header, UString& ext) U_NO_EXPORT;
   static bool splitCGIOutput(const char*& ptr1, const char* ptr2, uint32_t endHeader, UString& ext) U_NO_EXPORT;

   UHTTP(const UHTTP&)            {}
   UHTTP& operator=(const UHTTP&) { return *this; }

   friend class UHttpPlugIn;
};

#endif
