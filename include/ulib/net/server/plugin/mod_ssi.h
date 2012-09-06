// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_ssi.h - Server Side Includes (SSI)
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_SSI_H
#define U_MOD_SSI_H 1

#include <ulib/net/server/server_plugin.h>

class U_EXPORT USSIPlugIn : public UServerPlugIn {
public:

   static const UString* str_expr;
   static const UString* str_var;
   static const UString* str_cmd;
   static const UString* str_cgi;
   static const UString* str_file;
   static const UString* str_value;
   static const UString* str_bytes;
   static const UString* str_direct;
   static const UString* str_abbrev;
   static const UString* str_errmsg;
   static const UString* str_virtual;
   static const UString* str_timefmt;
   static const UString* str_sizefmt;
   static const UString* str_DATE_GMT;
   static const UString* str_USER_NAME;
   static const UString* str_DATE_LOCAL;
   static const UString* str_DOCUMENT_URI;
   static const UString* str_DOCUMENT_NAME;
   static const UString* str_LAST_MODIFIED;
   static const UString* str_SSI_AUTOMATIC_ALIASING;
   static const UString* str_errmsg_default;
   static const UString* str_timefmt_default;
   static const UString* str_encoding;
   static const UString* str_encoding_none;
   static const UString* str_encoding_url;
   static const UString* str_encoding_entity;
   static const UString* str_servlet;

   static void str_allocate();

   // COSTRUTTORI

            USSIPlugIn();
   virtual ~USSIPlugIn();

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerInit();

   // Connection-wide hooks

   virtual int handlerRequest();

   // SERVICES

   static UString* environment;
   static UString* alternative_include;
   static int      alternative_response;

   static void setMessagePage(       const UString& tmpl, const char* title_txt, const char* message);
   static void setMessagePageWithVar(const UString& tmpl, const char* title_txt, const char* fmt, ...);

   static void setAlternativeRedirect(const char* fmt, ...);
   static void setAlternativeResponse(const UString& _body);
   static void setAlternativeInclude(const UString& tmpl, const char* title_txt, const char* head_html, const char* body_style, ...);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   time_t last_modified;
   UString docname, timefmt, errmsg;
   bool use_size_abbrev;

   static UString* body;
   static UString* header;

private:
   UString getInclude(const UString& include, int include_level) U_NO_EXPORT;
   UString processSSIRequest(const UString& content, int include_level) U_NO_EXPORT;

   static bool    callService(const UString& name, const UString& value) U_NO_EXPORT;
   static UString getPathname(const UString& name, const UString& value, const UString& directory) U_NO_EXPORT;

   USSIPlugIn(const USSIPlugIn&) : UServerPlugIn() {}
   USSIPlugIn& operator=(const USSIPlugIn&)        { return *this; }
};

#endif
