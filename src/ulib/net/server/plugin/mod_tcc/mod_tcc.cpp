// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_tcc.cpp - plugin userver for TCC (Tiny C Compiler)
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/utility/uhttp.h>
#include <ulib/net/server/server.h>
#include <ulib/net/server/plugin/mod_tcc.h>

U_CREAT_FUNC(mod_tcc, UTCCPlugIn)

// Server-wide hooks

int UTCCPlugIn::handlerInit()
{
   U_TRACE(1, "UTCCPlugIn::handlerInit()")

   UHTTP::initCSP();

   U_SRV_LOG("initialization of plugin success");

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UTCCPlugIn::dump(bool reset) const
{
   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
