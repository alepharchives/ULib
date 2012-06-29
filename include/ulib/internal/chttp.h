/* ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    chttp.h - HTTP definition for C binding
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================ */

#ifndef ULIB_CHTTP_H
#define ULIB_CHTTP_H 1
/* -----------------------------------------------------------------------------------------------------------------------------
//  _     _   _
//  | |__ | |_| |_ _ __
//  | '_ \| __| __| '_ \
//  | | | | |_| |_| |_) |
//  |_| |_|\__|\__| .__/
//                  |_|
//
// ----------------------------------------------------------------------------------------------------------------------------- */
/* HTTP message handler
//
// The status code is a three-digit integer, and the first digit identifies the general category of response:
// ----------------------------------------------------------------------------------------------------------------------------- */

/* 1xx indicates an informational message only */
#define HTTP_CONTINUE                        100
#define HTTP_SWITCH_PROT                     101

/* 2xx indicates success of some kind */
#define HTTP_OK                              200
#define HTTP_CREATED                         201
#define HTTP_ACCEPTED                        202
#define HTTP_NOT_AUTHORITATIVE               203
#define HTTP_NO_CONTENT                      204
#define HTTP_RESET                           205
#define HTTP_PARTIAL                         206

#define HTTP_OPTIONS_RESPONSE                222

/* 3xx redirects the client to another URL */
#define HTTP_MULT_CHOICE                     300
#define HTTP_MOVED_PERM                      301
#define HTTP_MOVED_TEMP                      302
#define HTTP_FOUND                           302
#define HTTP_SEE_OTHER                       303
#define HTTP_NOT_MODIFIED                    304
#define HTTP_USE_PROXY                       305
#define HTTP_TEMP_REDIR                      307

/* 4xx indicates an error on the client's part */
#define HTTP_BAD_REQUEST                     400
#define HTTP_UNAUTHORIZED                    401
#define HTTP_PAYMENT_REQUIRED                402
#define HTTP_FORBIDDEN                       403
#define HTTP_NOT_FOUND                       404
#define HTTP_BAD_METHOD                      405
#define HTTP_NOT_ACCEPTABLE                  406
#define HTTP_PROXY_AUTH                      407
#define HTTP_CLIENT_TIMEOUT                  408
#define HTTP_CONFLICT                        409
#define HTTP_GONE                            410
#define HTTP_LENGTH_REQUIRED                 411
#define HTTP_PRECON_FAILED                   412
#define HTTP_ENTITY_TOO_LARGE                413
#define HTTP_REQ_TOO_LONG                    414
#define HTTP_UNSUPPORTED_TYPE                415
#define HTTP_REQ_RANGE_NOT_OK                416
#define HTTP_EXPECTATION_FAILED              417

#define HTTP_UNPROCESSABLE_ENTITY            422
#define HTTP_PRECONDITION_REQUIRED           428
#define HTTP_TOO_MANY_REQUESTS               429
#define HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE 431

/* 5xx indicates an error on the server's part */
#define HTTP_INTERNAL_ERROR                  500
#define HTTP_NOT_IMPLEMENTED                 501
#define HTTP_BAD_GATEWAY                     502
#define HTTP_UNAVAILABLE                     503
#define HTTP_GATEWAY_TIMEOUT                 504
#define HTTP_VERSION                         505

#define HTTP_NETWORK_AUTHENTICATION_REQUIRED 511

#define U_IS_HTTP_INFO(x)           (((x) >= 100)&&((x) < 200)) /* is the status code informational */
#define U_IS_HTTP_SUCCESS(x)        (((x) >= 200)&&((x) < 300)) /* is the status code OK ? */
#define U_IS_HTTP_REDIRECT(x)       (((x) >= 300)&&((x) < 400)) /* is the status code a redirect */
#define U_IS_HTTP_ERROR(x)          (((x) >= 400)&&((x) < 600)) /* is the status code a error (client or server) */
#define U_IS_HTTP_CLIENT_ERROR(x)   (((x) >= 400)&&((x) < 500)) /* is the status code a client error  */
#define U_IS_HTTP_SERVER_ERROR(x)   (((x) >= 500)&&((x) < 600)) /* is the status code a server error  */
#define U_IS_HTTP_VALID_RESPONSE(x) (((x) >= 100)&&((x) < 600)) /* is the status code a (potentially) valid response code ? */
 
/* should the status code drop the connection */
#define U_STATUS_DROPS_CONNECTION(x)              \
(((x) == HTTP_MOVED_TEMP)                      || \
 ((x) == HTTP_BAD_REQUEST)                     || \
 ((x) == HTTP_CLIENT_TIMEOUT)                  || \
 ((x) == HTTP_LENGTH_REQUIRED)                 || \
 ((x) == HTTP_PRECON_FAILED)                   || \
 ((x) == HTTP_ENTITY_TOO_LARGE)                || \
 ((x) == HTTP_REQ_TOO_LONG)                    || \
 ((x) == HTTP_INTERNAL_ERROR)                  || \
 ((x) == HTTP_UNAVAILABLE)                     || \
 ((x) == HTTP_NETWORK_AUTHENTICATION_REQUIRED) || \
 ((x) == HTTP_NOT_IMPLEMENTED))

/* -----------------------------------------------------------------------------------------------------------------------------
// HTTP header representation
// ----------------------------------------------------------------------------------------------------------------------------- */

typedef struct uhttpinfo {
   const char* method;
   const char* uri;
   const char* query;
   const char* host;
   const char* range;
   const char* accept;
   const char* cookie;
   const char* referer;
   const char* ip_client;
   const char* user_agent;
   const char* content_type;
   const char* accept_language;
   time_t      if_modified_since;
   uint32_t    nResponseCode, startHeader, endHeader, szHeader, clength, method_len, uri_len, query_len, host_len, host_vlen,
               range_len, accept_len, cookie_len, referer_len, ip_client_len, user_agent_len, content_type_len, accept_language_len;
         char  flag[12];
} uhttpinfo;

#define U_http_upgrade              u_http_info.flag[0]
#define U_http_version              u_http_info.flag[1]
#define U_http_keep_alive           u_http_info.flag[2]
#define U_http_method_type          u_http_info.flag[3]
#define U_http_request_check        u_http_info.flag[4]
#define U_http_is_accept_gzip       u_http_info.flag[5]
#define U_http_is_connection_close  u_http_info.flag[6]
#define U_http_no_cache             u_http_info.flag[7]
#define U_http_is_navigation        u_http_info.flag[8]
#define U_http_chunked              u_http_info.flag[9]
#define U_http_unused1              u_http_info.flag[10]
#define U_http_unused2              u_http_info.flag[11]

enum HTTPMethodType { HTTP_POST = '1', HTTP_PUT = '2', HTTP_DELETE = '3', HTTP_GET = '4', HTTP_HEAD = '5', HTTP_OPTIONS = '6', HTTP_COPY = '7' };

#define U_HTTP_METHOD_TO_PARAM          u_http_info.method, u_http_info.method_len
#define U_HTTP_METHOD_TO_TRACE          u_http_info.method_len, u_http_info.method

#define U_HTTP_URI_TO_TRACE             u_http_info.uri_len, u_http_info.uri
#define U_HTTP_URI_TO_PARAM             u_http_info.uri, u_http_info.uri_len

#define U_HTTP_QUERY_TO_PARAM           u_http_info.query, u_http_info.query_len
#define U_HTTP_QUERY_TO_TRACE           u_http_info.query_len, u_http_info.query

#define U_HTTP_URI_QUERY_LEN            (u_http_info.uri_len + u_http_info.query_len + (u_http_info.query_len ? 1 : 0))

#define U_HTTP_URI_QUERY_TO_PARAM       u_http_info.uri, U_HTTP_URI_QUERY_LEN 
#define U_HTTP_URI_QUERY_TO_TRACE       U_HTTP_URI_QUERY_LEN, u_http_info.uri

#define U_HTTP_CTYPE_TO_PARAM           u_http_info.content_type, u_http_info.content_type_len
#define U_HTTP_CTYPE_TO_TRACE           u_http_info.content_type_len, u_http_info.content_type

#define U_HTTP_RANGE_TO_PARAM           u_http_info.range, u_http_info.range_len
#define U_HTTP_RANGE_TO_TRACE           u_http_info.range_len, u_http_info.range

#define U_HTTP_ACCEPT_TO_PARAM          u_http_info.accept, u_http_info.accept_len
#define U_HTTP_ACCEPT_TO_TRACE          u_http_info.accept_len, u_http_info.accept

#define U_HTTP_COOKIE_TO_PARAM          u_http_info.cookie, u_http_info.cookie_len
#define U_HTTP_COOKIE_TO_TRACE          u_http_info.cookie_len, u_http_info.cookie

#define U_HTTP_REFERER_TO_PARAM         u_http_info.referer, u_http_info.referer_len
#define U_HTTP_REFERER_TO_TRACE         u_http_info.referer_len, u_http_info.referer

#define U_HTTP_IP_CLIENT_TO_PARAM       u_http_info.ip_client, u_http_info.ip_client_len
#define U_HTTP_IP_CLIENT_TO_TRACE       u_http_info.ip_client_len, u_http_info.ip_client

#define U_HTTP_USER_AGENT_TO_PARAM      u_http_info.user_agent, u_http_info.user_agent_len
#define U_HTTP_USER_AGENT_TO_TRACE      u_http_info.user_agent_len, u_http_info.user_agent

#define U_HTTP_ACCEPT_LANGUAGE_TO_PARAM u_http_info.accept_language, u_http_info.accept_language_len
#define U_HTTP_ACCEPT_LANGUAGE_TO_TRACE u_http_info.accept_language_len, u_http_info.accept_language

/* The hostname of your server from header's request.
 * The difference between U_HTTP_HOST_.. and U_HTTP_VHOST_.. is that
 * U_HTTP_HOST_.. can include the «:PORT» text, and U_HTTP_VHOST_.. only the name
 */

#define U_HTTP_HOST_TO_PARAM       u_http_info.host, u_http_info.host_len
#define U_HTTP_HOST_TO_TRACE       u_http_info.host_len, u_http_info.host

#define U_HTTP_VHOST_TO_PARAM      u_http_info.host, u_http_info.host_vlen
#define U_HTTP_VHOST_TO_TRACE      u_http_info.host_vlen, u_http_info.host

#define U_HTTP_URI_STRNEQ(str)                                    U_STRNEQ(u_http_info.uri,          str)
#define U_HTTP_HOST_STRNEQ(str)   (u_http_info.host_len         ? U_STRNEQ(u_http_info.host,         str) : false)
#define U_HTTP_QUERY_STRNEQ(str)  (u_http_info.query_len        ? U_STRNEQ(u_http_info.query,        str) : false)
#define U_HTTP_CTYPE_STRNEQ(str)  (u_http_info.content_type_len ? U_STRNEQ(u_http_info.content_type, str) : false)

#ifdef __cplusplus
extern "C" {
#endif

extern U_EXPORT uhttpinfo u_http_info;

#ifdef __cplusplus
}
#endif

#define U_HTTP_INFO_INIT(c) (void)U_SYSCALL(memset,"%p,%d,%u",&u_http_info,c,sizeof(uhttpinfo))

#endif
