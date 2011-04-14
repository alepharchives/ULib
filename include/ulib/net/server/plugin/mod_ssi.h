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

/*
The plugin interface is an integral part of UServer which provides a flexible way to add specific functionality to UServer.
Plugins allow you to enhance the functionality of UServer without changing the core of the server. They can be loaded at
startup time and can change virtually some aspect of the behaviour of the server.

UServer has 5 hooks which are used in different states of the execution of the request:
--------------------------------------------------------------------------------------------
* Server-wide hooks:
````````````````````
1) handlerConfig: called when the server finished to process its configuration
2) handlerInit:   called when the server finished its init, and before start to run

* Connection-wide hooks:
````````````````````````
3) handlerREAD:
4) handlerRequest:
5) handlerReset:
  called in `UClientImage_Base::handlerRead()`
--------------------------------------------------------------------------------------------

RETURNS:
  U_PLUGIN_HANDLER_GO_ON    if ok
  U_PLUGIN_HANDLER_FINISHED if the final output is prepared
  U_PLUGIN_HANDLER_AGAIN    if the request is empty (NONBLOCKING)

  U_PLUGIN_HANDLER_ERROR    on error
*/

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
   static const UString* str_usp;

   static void str_allocate();

   // COSTRUTTORI

   USSIPlugIn()
      {
      U_TRACE_REGISTER_OBJECT(0, USSIPlugIn, "")

      if (str_SSI_AUTOMATIC_ALIASING == 0) str_allocate();
      }

   virtual ~USSIPlugIn()
      {
      U_TRACE_UNREGISTER_OBJECT(0, USSIPlugIn)
      }

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerInit();

   // Connection-wide hooks

   virtual int handlerRequest();

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   time_t last_modified;
   UString environment, docname, timefmt, errmsg;
   bool use_size_abbrev;

private:
   UString getInclude(const UString& include, int include_level) U_NO_EXPORT;
   UString processSSIRequest(const UString& content, int include_level) U_NO_EXPORT;

   static UString getPathname(const UString& name, const UString& value, const UString& directory) U_NO_EXPORT;

   USSIPlugIn(const USSIPlugIn&) : UServerPlugIn() {}
   USSIPlugIn& operator=(const USSIPlugIn&)        { return *this; }
};

#endif
