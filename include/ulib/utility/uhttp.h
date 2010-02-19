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

// 3xx redirects the client to another URL
#define HTTP_MULT_CHOICE         300
#define HTTP_MOVED_PERM          301
#define HTTP_MOVED_TEMP          302
#define HTTP_SEE_OTHER           303
#define HTTP_NOT_MODIFIED        304
#define HTTP_USE_PROXY           305

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

typedef struct uhttpheader {
   const char* method;
   const char* uri;
   const char* query;
   const char* host;
   uint32_t nResponseCode, version,
            startHeader, endHeader, szHeader,
            method_len, uri_len, query_len, host_len,
            method_type, clength, keep_alive, is_connection_close, is_php;
} uhttpheader;

enum HTTPMethodType { HTTP_POST = 1, HTTP_GET = 2, HTTP_HEAD = 3 };

#define U_HTTP_HEADER(str) str.substr(UHTTP::http_info.startHeader, UHTTP::http_info.szHeader)

// NB: we check for pointer or offset...

#define U_HTTP_URI   (UHTTP::http_info.uri   > (const char*)0x0fff ? UHTTP::http_info.uri \
                                                    : UHTTP::http_info.method + (ptrdiff_t)UHTTP::http_info.uri)
#define U_HTTP_QUERY (UHTTP::http_info.query > (const char*)0xffff ? UHTTP::http_info.query \
                                                    : UHTTP::http_info.method + (ptrdiff_t)UHTTP::http_info.query)

#define U_HTTP_METHOD_TO_PARAM      UHTTP::http_info.method, UHTTP::http_info.method_len
#define U_HTTP_METHOD_TO_TRACE      UHTTP::http_info.method_len, UHTTP::http_info.method

#define U_HTTP_URI_TO_PARAM         U_HTTP_URI, UHTTP::http_info.uri_len
#define U_HTTP_URI_TO_TRACE         UHTTP::http_info.uri_len, U_HTTP_URI

#define U_HTTP_QUERY_TO_PARAM       U_HTTP_QUERY, UHTTP::http_info.query_len
#define U_HTTP_QUERY_TO_TRACE       UHTTP::http_info.query_len, U_HTTP_QUERY

#define U_HTTP_URI_QUERY_TO_PARAM   U_HTTP_URI, (UHTTP::http_info.uri_len + UHTTP::http_info.query_len + (UHTTP::http_info.query_len ? 1 : 0))
#define U_HTTP_URI_QUERY_TO_TRACE   (UHTTP::http_info.uri_len + UHTTP::http_info.query_len + (UHTTP::http_info.query_len ? 1 : 0)), U_HTTP_URI

#define U_HTTP_HOST_TO_PARAM        UHTTP::http_info.host, UHTTP::http_info.host_len
#define U_HTTP_HOST_TO_TRACE        UHTTP::http_info.host_len, UHTTP::http_info.host

#define U_HTTP_URI_TO_PARAM_SHIFT(n) U_HTTP_URI+U_min(n,UHTTP::http_info.uri_len), UHTTP::http_info.uri_len-U_min(n,UHTTP::http_info.uri_len)

// HTTP Compare

#define U_HTTP_URI_STREQ(str)    (strncmp(U_HTTP_URI,            str, U_max(U_CONSTANT_SIZE(str), UHTTP::http_info.uri_len))   == 0)
#define U_HTTP_HOST_STREQ(str)   (strncmp(UHTTP::http_info.host, str, U_max(U_CONSTANT_SIZE(str), UHTTP::http_info.host_len))  == 0)
#define U_HTTP_QUERY_STREQ(str)  (strncmp(U_HTTP_QUERY,          str, U_max(U_CONSTANT_SIZE(str), UHTTP::http_info.query_len)) == 0)

// HTTP Access Authentication

#define U_HTTP_REALM "Protected Area"

// Default content type

#define U_CTYPE_HTML "text/html; charset=iso-8859-1"

class UFile;
class UCommand;
class UMimeMultipart;

template <class T> class UVector;

class U_EXPORT UHTTP {
public:

   // HTTP main error message

   static void str_allocate();

   static const UString* str_frm_response;
   static const UString* str_frm_forbidden;
   static const UString* str_frm_not_found;
   static const UString* str_frm_moved_temp;
   static const UString* str_frm_bad_request;
   static const UString* str_frm_internal_error;
   static const UString* str_frm_service_unavailable;

   // HTTP header representation

   static const char* ptrC;
   static const char* ptrH;
   static uhttpheader http_info;

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
         http_info.method_type = HTTP_GET;

         U_INTERNAL_ASSERT_EQUALS(http_info.method_len, 3)
         U_INTERNAL_ASSERT_EQUALS(U_STRNCMP(http_info.method, "GET"), 0)
         }
      else if (c == 'P') // POST
         {
         http_info.method_type = HTTP_POST;

         U_INTERNAL_ASSERT_EQUALS(http_info.method_len, 4)
         U_INTERNAL_ASSERT_EQUALS(U_STRNCMP(http_info.method, "POST"), 0)
         }
      else // HEAD
         {
         http_info.method_type = HTTP_HEAD;

         U_INTERNAL_ASSERT_EQUALS(http_info.method_len, 4)
         U_INTERNAL_ASSERT_EQUALS(U_STRNCMP(http_info.method, "HEAD"), 0)
         }

      U_INTERNAL_DUMP("method_type = %u", http_info.method_type)
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

   static void resetHTTPInfo()
      {
      U_TRACE(0, "UHTTP::resetHTTPInfo()")

      cgi_dir[0] = '\0';

      (void) U_SYSCALL(memset, "%p,%d,%u", &http_info, 0, sizeof(uhttpheader));
      }

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

   static bool    isHTTPRequest(const char* ptr);
   static bool scanfHTTPHeader( const char* ptr);

   static const char* getHTTPStatus();
   static const char* getHTTPStatusDescription();

   static bool readHTTPRequest();
   static bool readHTTPHeader(USocket* socket, UString& rbuffer);
   static bool readHTTPBody(  USocket* socket, UString& rbuffer, UString& body);

   // TYPE

   static bool isHTTPRequest() { return (http_info.method_type && http_info.szHeader); }

   static bool isHttpGET()
      {
      U_TRACE(0, "UHTTP::isHttpGET()")

      bool result = (http_info.method_type >= HTTP_GET);

      U_RETURN(result);
      }

   static bool isHttpHEAD()
      {
      U_TRACE(0, "UHTTP::isHttpHEAD()")

      bool result = (http_info.method_type == HTTP_HEAD);

      U_RETURN(result);
      }

   static bool isHttpPOST()
      {
      U_TRACE(0, "UHTTP::isHttpPOST()")

      bool result = (http_info.method_type == HTTP_POST);

      U_RETURN(result);
      }

   static bool isSOAPRequest()
      {
      U_TRACE(0, "UHTTP::isSOAPRequest()")

      U_ASSERT(isHttpPOST())

      bool result = U_HTTP_URI_STREQ("/soap");

      U_RETURN(result);
      }

   static bool isTSARequest()
      {
      U_TRACE(0, "UHTTP::isTSARequest()")

      U_ASSERT(isHttpPOST())

      bool result = U_HTTP_URI_STREQ("/tsa");

      U_RETURN(result);
      }

   static bool isHTTPDirectoryRequest()
      {
      U_TRACE(0, "UHTTP::isHTTPDirectoryRequest()")

      U_INTERNAL_ASSERT(isHTTPRequest())

      bool result = (U_HTTP_URI[UHTTP::http_info.uri_len - 1] == '/');

      U_RETURN(result);
      }

   static bool isUSPRequest()
      {
      U_TRACE(0, "UHTTP::isUSPRequest()")

      U_INTERNAL_ASSERT(isHTTPRequest())

      bool result = (U_STRNCMP(U_HTTP_URI, "/usp/") == 0 &&
                     u_endsWith(U_HTTP_URI_TO_PARAM, U_CONSTANT_TO_PARAM(".usp")));

      U_RETURN(result);
      }

   // SERVICES

   static UFile* file;

   static bool isPHPRequest();
   static void getTimeIfNeeded();
   static bool checkHTTPRequest();
   static bool processHTTPGetRequest();
   static bool processHTTPAuthorization(bool digest, const UString& request_uri);

   static UString     getHTMLDirectoryList();
   static const char* getHTTPHeaderValuePtr(const UString& name);

   // retrieve information on specific HTML form elements
   // (such as checkboxes, radio buttons, and text fields), or uploaded files

   static UString* tmpdir;
   static UString* qcontent;
   static UMimeMultipart* formMulti;
   static UVector<UString>* form_name_value;

   static void     resetForm();
   static uint32_t processHTTPForm();
   static void     getFormValue(UString& buffer, uint32_t n);

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

   static UString getHTTPCookie(bool sh_script);
   static UString setHTTPCookie(const UString& param);

   // CGI

   static char cgi_dir[U_PATH_MAX];

   static bool isCGIRequest();
   static bool processCGIOutput();
   static void setCGIShellScript(UString& command);
   static void setCGIEnvironment(UString& environment, bool sh_script);
   static bool processCGIRequest(UCommand* pcmd, UString* penvironment);
   static void setHTTPCgiResponse(int nResponseCode, uint32_t content_length, bool header_content_length);

   // Accept-Language: en-us,en;q=0.5
   // ----------------------------------------------------
   // take only the first 2 character (it, en, de fr, ...)

   static const char* getAcceptLanguage();

   // check for "Accept-Encoding: deflate" in header request...

   static bool isHTTPAcceptEncodingDeflate(uint32_t content_length);

   // check for MSIE in "User-Agent: ...." in header request...

   static const char* getBrowserMSIE();

   // check for "Connection: close" in headers...

   static int checkForHTTPConnectionClose();

   // set HTTP main error message

   static void setHTTPNotFound();
   static void setHTTPForbidden();
   static void setHTTPBadRequest();
   static void setHTTPInternalError();
   static void setHTTPServiceUnavailable();
   static void setHTTPUnAuthorized(bool digest);

   // get HTTP response message

   static UString getHTTPHeaderForResponse(int nResponseCode, const char* content_type, const UString& body);
   static UString getHTTPRedirectResponse(const UString& ext, const char* ptr_location, uint32_t len_location);

private:
   static bool     openFile() U_NO_EXPORT;
   static UString  getHTTPHeaderForResponse() U_NO_EXPORT;
   static UString  getHTTPHeaderForResponse(int nResponseCode, UString& content) U_NO_EXPORT;

   UHTTP(const UHTTP&)            {}
   UHTTP& operator=(const UHTTP&) { return *this; }
};

#endif
