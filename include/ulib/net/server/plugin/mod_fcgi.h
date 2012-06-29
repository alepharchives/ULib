// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_fcgi.h - Fast CGI
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_FCGI_H
#define U_MOD_FCGI_H 1

#include <ulib/string.h>
#include <ulib/net/server/server_plugin.h>

class UClient_Base;

class U_EXPORT UFCGIPlugIn : public UServerPlugIn {
public:

   static const UString* str_FCGI_URI_MASK;
   static const UString* str_FCGI_KEEP_CONN;

   static void str_allocate();

   // COSTRUTTORI

   UFCGIPlugIn()
      {
      U_TRACE_REGISTER_OBJECT(0, UFCGIPlugIn, "")

      connection = 0;

      if (str_FCGI_URI_MASK == 0) str_allocate();
      }

   virtual ~UFCGIPlugIn();

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
   bool fcgi_keep_conn;

          void set_FCGIBeginRequest();
   static void fill_FCGIBeginRequest(u_char type, u_short content_length);

private:
   UFCGIPlugIn(const UFCGIPlugIn&) : UServerPlugIn() {}
   UFCGIPlugIn& operator=(const UFCGIPlugIn&)        { return *this; }
};

#endif
