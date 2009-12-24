// test_http_header.cpp

#include <examples/http_header/include/HttpCookie.h>

#ifdef U_PROXY
#  ifdef NDEBUG
#     undef NDEBUG
#  endif
#  include <assert.h>
#  include <DES3engine.h>
#  undef  U_ASSERT(expr)
#  define U_ASSERT(expr) assert(expr);
#else
#  include <ulib/base/ssl/des3.h>
#endif

#include <examples/http_header/include/HttpSetCookie.h>
#include <examples/http_header/include/OtpAuthToken.h>
#include <examples/http_header/include/HttpOtpPostLogin.h>
#include <examples/http_header/include/HttpBaAuthorization.h>

#define HEADER_1                                               \
   "GET / HTTP/1.1\r\n"                                        \
   "Host: dummy\r\n"                                           \
   "Content-Length: 1200\r\n"                                  \
   "Authorization: Digest Name1=Value1\r\n"                    \
   "Cookie: Name=value\r\n"                                    \
   "Cookie: $Version=1; NameA=valueA;\r\n"                     \
   "  NameB = ValueB;$Path=\"/\" ; $Domain=domain1;\r\n"       \
   "  NameC = ValueC;$Domain=\"/\" ; $Path=domain1;\r\n"       \
   "  $Port=\"123\"\r\n"                                       \
   "\r\n"

#define HEADER_2                                               \
   "HTTP/1.1 200 OK\r\n"                                       \
   "Date: Fri, 14 Nov 2003 10:57:35 GMT\r\n"                   \
   "Server: Apache/1.3.23 (Unix)  (Red-Hat/Linux) mod_ssl/2.8.7 OpenSSL/0.9.6b DAV/1.0.3 PHP/4.1.2 mod_perl/1.26\r\n" \
   "Last-Modified: Thu, 07 Nov 2002 08:40:12 GMT\r\n"          \
   "ETag: \"13b94-15f9-3dca26ec\"\r\n"                         \
   "Accept-Ranges: bytes\r\n"                                  \
   "Content-Length: 5625\r\n"                                  \
   "Content-Type: text/html\r\n"                               \
   "WWW-Authenticate: Basic realm=\"WallyWorld\"\r\n"          \
   "Location: https://user:passwd@dummy/path?query=value\r\n"  \
   "Set-Cookie: Version=1; NameA=valueA;\r\n"                  \
   "  NameB = ValueB;Path=\"/\" ; Domain=domain1;\r\n"         \
   "  NameC = ValueC;Domain=\"/\" ; Path=domain1;\r\n"         \
   "  Port=\"123\"\r\n"                                        \
   "Set-Cookie2: Version=1; NameA=valueA;\r\n"                 \
   "  NameB = ValueB;Path=\"/\" ; Domain=domain1;\r\n"         \
   "  NameC = ValueC;Domain=\"/\" ; Path=domain1;\r\n"         \
   "  Port=\"123\"\r\n"                                        \
   "\r\n"

#define COOKIE_2                                               \
   " $Version=1; NameA=valueA;\r\n"                            \
   "  NameB = ValueB;$Path=\"/\" ; $Domain=domain1;\r\n"       \
   "  NameC = ValueC;$Domain=\"/\" ; $Path=domain1;\r\n"       \
   "  $Port=\"123\"\r\n"

#define COOKIE_5                                               \
   " pippo=\"usr11 usr10\";otptoken=pluto\r\n"

#define SETCOOKIE_1                                            \
   " Version=1; NameA=valueA;\r\n"                             \
   "  NameB = ValueB;Secure;Path=\"/\" ; Domain=domain1;\r\n"  \
   "  NameC = ValueC;Domain=\"/\" ; Path=domain1;\r\n"         \
   "  Port=\"123\"\r\n"

#define SETCOOKIE_2                                               \
   " Version=1; NameA=valueA;\r\n"                                \
   "  NameB = ValueB; Secure; Path=\"/\" ; Domain=domain1;\r\n"   \
   "  NameC = ValueC; Discard; Domain=\"/\" ; Path=domain1;\r\n"  \
   "  Port=\"123\" Secure; Discard\r\n"

#define POST_BODY \
   "user=stefano+casazza&pin=12345&token=autorizzativo&cf=1234567890123456"

// #define SERGIO

#ifdef SERGIO
// TID=Current_Server_ID;UID=User_ID;SID=Session_ID;TS=20031125131800;CF=codicefiscale1;MIGRATE
#  define COOKIE_AUTH \
"AUTHTOKEN = " \
"U2FsdGVkX1+nc/vciIxpOY2qhdd9vaCv2C89HjkkSVvVZwdtCjFT/xdllSNwihkmkgq+vOtTDES2mnYwone04xyv5W+nBGulk1TixMiH3hOgLBCr181Asrw658esBZKPLJZMZOs2GyAmry+UEf1u8Q=="
#else
#  define COOKIE_AUTH \
"AUTHTOKEN = " \
"\"U2FsdGVkX1+nc/vciIxpOY2qhdd9vaCv2C89HjkkSVvVZwdtCjFT/xdllSNwihkmkgq+vOtTDES2mnYwone04xyv5W+nBGulk1TixMiH3hOgLBCr181Asrw658esBZKPLJZMZOs2GyAmry+UEf1u8Q==\""
#endif

// TID=Current_Server_ID;UID=User_ID;SID=;TS=20031125131800;CF=codicefiscale1;MIGRATE
#  define COOKIE_PROBLEM \
"otptoken = " \
"\"U2FsdGVkX1/hWkCAn/2KbIn2nZGfBLVAc6fwsA+7+ShXuCh2i9D7NEQd/xpF0+ry7UYyC637y87X0EUdoavde/cQDvXzUS68dbrdItRODMSOaIgXXXPNuw==\""

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   UString value, path, domain, port, not_found;

   HttpCookie c1(U_CONSTANT_TO_PARAM("Cookie"), U_CONSTANT_TO_PARAM("Name=value")),
              c2(U_CONSTANT_TO_PARAM("Cookie"), U_CONSTANT_TO_PARAM(COOKIE_2)),
              c5(U_CONSTANT_TO_PARAM("Cookie"), U_CONSTANT_TO_PARAM(COOKIE_5));

   U_ASSERT( c5.find(U_STRING_FROM_CONSTANT("otptoken"), value, path, domain, port)  == true )
   U_ASSERT( value   == U_STRING_FROM_CONSTANT("pluto") )

   U_ASSERT( c1.count(U_STRING_FROM_CONSTANT("Name"))    == 1 )
   U_ASSERT( c2.count(U_STRING_FROM_CONSTANT("$Domain")) == 2 )

   U_ASSERT( c1.find(U_STRING_FROM_CONSTANT("Name"), value, path, domain, port)  == true )
   U_ASSERT( value   == U_STRING_FROM_CONSTANT("value") )
   U_ASSERT( path    == not_found )
   U_ASSERT( domain  == not_found )
   U_ASSERT( port    == not_found )

   U_ASSERT( c2.find(U_STRING_FROM_CONSTANT("NameB"), value, path, domain, port)  == true )
   U_ASSERT( value   == U_STRING_FROM_CONSTANT("ValueB") )
   U_ASSERT( domain  == U_STRING_FROM_CONSTANT("domain1") )
   U_ASSERT( port    == not_found )

#ifdef SERGIO
   U_ASSERT( path    == U_STRING_FROM_CONSTANT("/") )
#else
   U_ASSERT( path    == U_STRING_FROM_CONSTANT("\"/\"") )
#endif

   U_ASSERT( c2.del(U_STRING_FROM_CONSTANT("NameB")) == true )

   value = path = domain = port = not_found;

   U_ASSERT( c2.find(U_STRING_FROM_CONSTANT("Name"), value, path, domain, port)  == false )
   U_ASSERT( value   == not_found )
   U_ASSERT( path    == not_found )
   U_ASSERT( domain  == not_found )
   U_ASSERT( port    == not_found )

   U_ASSERT( c2.find(U_STRING_FROM_CONSTANT("NameC"), value, path, domain, port)  == true )
   U_ASSERT( value   == U_STRING_FROM_CONSTANT("ValueC") )
   U_ASSERT( path    == U_STRING_FROM_CONSTANT("domain1") )

#ifdef SERGIO
   U_ASSERT( port    == U_STRING_FROM_CONSTANT("123") )
   U_ASSERT( domain  == U_STRING_FROM_CONSTANT("/") )
#else
   U_ASSERT( port    == U_STRING_FROM_CONSTANT("\"123\"") )
   U_ASSERT( domain  == U_STRING_FROM_CONSTANT("\"/\"") )
#endif

   HttpSetCookie s1(U_CONSTANT_TO_PARAM("Set-Cookie"),  U_CONSTANT_TO_PARAM(SETCOOKIE_1)),
                 s2(U_CONSTANT_TO_PARAM("Set-Cookie2"), U_CONSTANT_TO_PARAM(SETCOOKIE_2));

   U_ASSERT( s1.count(U_STRING_FROM_CONSTANT("Domain"))  == 2 )
   U_ASSERT( s2.count(U_STRING_FROM_CONSTANT("Port"))    == 1 )

   HttpCookie c3(U_CONSTANT_TO_PARAM("Cookie"), U_CONSTANT_TO_PARAM(COOKIE_AUTH) );

   U_ASSERT( c3.count(U_STRING_FROM_CONSTANT("AUTHTOKEN")) == 1 )

   value = path = domain = port = not_found;

   U_ASSERT( c3.find(U_STRING_FROM_CONSTANT("AUTHTOKEN"), value, path, domain, port)  == true )

   U_ASSERT( path    == not_found )
   U_ASSERT( domain  == not_found )
   U_ASSERT( port    == not_found )

   value.erase(value.size()-1, 1);
   value.erase(0, 1);

#ifdef U_PROXY_UNIT
   DES3engine eng("pippo");
   OtpAuthToken a(&eng, value);
#else
   u_des3_key("pippo");
   OtpAuthToken a(0, value);
#endif

   U_ASSERT( a.tid     == U_STRING_FROM_CONSTANT("Current_Server_ID") )
   U_ASSERT( a.uid     == U_STRING_FROM_CONSTANT("User_ID") )
   U_ASSERT( a.sid     == U_STRING_FROM_CONSTANT("Session_ID") )
   U_ASSERT( a.ts      == U_STRING_FROM_CONSTANT("20031125131800") )
   U_ASSERT( a.cf      == U_STRING_FROM_CONSTANT("codicefiscale1") )
   U_ASSERT( a.migrate == true )

   HttpHeader h;
   HttpField* f = new HttpField(U_STRING_FROM_CONSTANT("Content-Type"), U_STRING_FROM_CONSTANT(" application/x-www-form-urlencoded"));
   HttpBaAuthorization* ba = new HttpBaAuthorization(U_CONSTANT_TO_PARAM("Authorization"),
                                                     U_CONSTANT_TO_PARAM(" Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ=="));
   HttpBaAuthorization* ba1 = new HttpBaAuthorization(U_CONSTANT_TO_PARAM("Authorization"),
                                                      U_CONSTANT_TO_PARAM(" Basic dXRlbnRlMTpzaWQx"));

   h.add(ba);
   h.add(f);
   h.add(ba1);

   HttpOtpPostLogin p(U_CONSTANT_TO_PARAM(POST_BODY), U_STRING_FROM_CONSTANT("user"),
                                                    U_STRING_FROM_CONSTANT("pin"),
                                                    U_STRING_FROM_CONSTANT("token"),
                                                    U_STRING_FROM_CONSTANT("password"),
                                                    U_STRING_FROM_CONSTANT("cf"), h);

   U_ASSERT( p.user  == U_STRING_FROM_CONSTANT("stefano casazza") )
   U_ASSERT( p.pin   == U_STRING_FROM_CONSTANT("12345") )
   U_ASSERT( p.token == U_STRING_FROM_CONSTANT("autorizzativo") )

   HttpField* p1 = h.del(U_STRING_FROM_CONSTANT("Content-Type"));

   U_ASSERT( p1 !=  0 )
   U_ASSERT( p1 == f )

   HttpBaAuthorization* p2 = (HttpBaAuthorization*) h.find(U_STRING_FROM_CONSTANT("Authorization"));

   U_ASSERT( p2 !=  0 )
   U_ASSERT( p2 == ba )

   U_ASSERT( p2->user   == U_STRING_FROM_CONSTANT("Aladdin") )
   U_ASSERT( p2->passwd == U_STRING_FROM_CONSTANT("open sesame") )

   HttpBaAuthorization* p3 = (HttpBaAuthorization*) h.find(U_STRING_FROM_CONSTANT("Authorization"), 1);

   U_ASSERT( p3 !=  0 )
   U_ASSERT( p3 == ba1 )

   U_ASSERT( p3->user   == U_STRING_FROM_CONSTANT("utente1") )
   U_ASSERT( p3->passwd == U_STRING_FROM_CONSTANT("sid1") )

   h.clear();

   UString result;

   a.stringify(result);

// TID=trustACCESS1;UID=utente1;SID=;TS=20031201174127;CF=codicefiscale1
#  define COOKIE_AUTH_1 \
"U2FsdGVkX1/QsrBvmsVHx0rrX78ldh6IJu1+4GhKoJ9O5ETSbfSiDip1gszkZX7w5ah6vkYfRWI8271LcNKhUsZVehRoscudLO8uotQgeiiF1B46ITphGw=="

// TID=trustACCESS1;UID=utente1;SID=sid;TS=20031201174127;CF=codicefiscale1;HP1=Profile_Header1;HPn=Profile_Headern;MIGRATE
#  define COOKIE_AUTH_2 \
"U2FsdGVkX1+tUkpPi14NVlKhm5KUFbSH0JFvi23+8B75MnKgtyD/sc0hc0ESmSahiYozVbS6a3OoZfWDHX3G3zuUwCP7n1+3jXK0wu6niifYUW+cKBk1WUdpJZd0xjJernDsWtPfq9j30uatAhHULG57vdrKlbtxM/EIaiaUow1AeLuDiZDcTRonghpI/aaz"

#ifdef U_PROXY_UNIT
   DES3engine eng1("password");
   OtpAuthToken c(&eng1, U_STRING_FROM_CONSTANT(COOKIE_AUTH_2));
   DES3engine eng2("password");
   OtpAuthToken b(&eng2, U_STRING_FROM_CONSTANT(COOKIE_AUTH_1));
#else
   u_des3_key("password");
   OtpAuthToken c(0, U_STRING_FROM_CONSTANT(COOKIE_AUTH_2));
   u_des3_reset();
   OtpAuthToken b(0, U_STRING_FROM_CONSTANT(COOKIE_AUTH_1));
#endif

   U_ASSERT( b.is_valid() == false )
   U_ASSERT( c.is_valid() == true )

   U_ASSERT( c.tid     == U_STRING_FROM_CONSTANT("trustACCESS1") )
   U_ASSERT( c.uid     == U_STRING_FROM_CONSTANT("utente1") )
   U_ASSERT( c.sid     == U_STRING_FROM_CONSTANT("sid") )
   U_ASSERT( c.ts      == U_STRING_FROM_CONSTANT("20031201174127") )
   U_ASSERT( c.cf      == U_STRING_FROM_CONSTANT("codicefiscale1") )
   U_ASSERT( c.migrate == true )

   value = not_found;

   U_ASSERT( c.find(U_STRING_FROM_CONSTANT("HP1"), value)  == true )
   U_ASSERT( value == U_STRING_FROM_CONSTANT("Profile_Header1") )

   U_ASSERT( c.del(U_STRING_FROM_CONSTANT("HP1"))  == true )

   value = not_found;

   U_ASSERT( c.find(U_STRING_FROM_CONSTANT("HPn"), value)  == true )
   U_ASSERT( value == U_STRING_FROM_CONSTANT("Profile_Headern") )

   HttpCookie c4(U_CONSTANT_TO_PARAM("Cookie"), U_CONSTANT_TO_PARAM(COOKIE_PROBLEM) );

   U_ASSERT( c4.count(U_STRING_FROM_CONSTANT("otptoken")) == 1 )

   value = path = domain = port = not_found;

   U_ASSERT( c4.find(U_STRING_FROM_CONSTANT("otptoken"), value, path, domain, port) == true )

   value.erase(value.size()-1, 1);
   value.erase(0, 1);

#ifdef U_PROXY_UNIT
   DES3engine eng3("password");
   OtpAuthToken d(&eng3, value);
#else
   u_des3_reset();
   OtpAuthToken d(0, value);
#endif

   U_ASSERT( d.is_valid() == false )

   result.erase(result.size()-1, 1);
   result.erase(0, 1);

   cout.write(result.data(), result.size());
}
