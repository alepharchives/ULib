// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_scgi.h - Simple CGI
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_SCGI_H
#define U_MOD_SCGI_H 1

#include <ulib/string.h>
#include <ulib/net/server/server_plugin.h>

class UClient_Base;

class U_EXPORT USCGIPlugIn : public UServerPlugIn {
public:

   static const UString* str_SCGI_URI_MASK;
   static const UString* str_SCGI_KEEP_CONN;

   static void str_allocate();

   // COSTRUTTORI

   USCGIPlugIn()
      {
      U_TRACE_REGISTER_OBJECT(0, USCGIPlugIn, "")

      connection = 0;

      if (str_SCGI_URI_MASK == 0) str_allocate();
      }

   virtual ~USCGIPlugIn();

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
   UClient_Base* connection;
   bool scgi_keep_conn;

private:
   USCGIPlugIn(const USCGIPlugIn&) : UServerPlugIn() {}
   USCGIPlugIn& operator=(const USCGIPlugIn&)        { return *this; }
};

#endif
