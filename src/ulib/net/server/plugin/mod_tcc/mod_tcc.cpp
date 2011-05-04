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

#include <ulib/file_config.h>
#include <ulib/utility/uhttp.h>
#include <ulib/net/server/server.h>
#include <ulib/net/server/plugin/mod_tcc.h>

U_CREAT_FUNC(mod_tcc, UTCCPlugIn)

const UString* UTCCPlugIn::str_CSP_directory;

void UTCCPlugIn::str_allocate()
{
   U_TRACE(0, "UTCCPlugIn::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_CSP_directory,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("CSP_directory") }
   };

   U_NEW_ULIB_OBJECT(str_CSP_directory, U_STRING_FROM_STRINGREP_STORAGE(0));
}

UTCCPlugIn::~UTCCPlugIn()
{
   U_TRACE(1, "UTCCPlugIn::~UTCCPlugIn()")
}

// Server-wide hooks

int UTCCPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UTCCPlugIn::handlerConfig(%p)", &cfg)

   // ------------------------------------------------------------------------------------------
   // CSP_directory
   // ------------------------------------------------------------------------------------------

   if (cfg.loadTable()) CSP_directory = cfg[*str_CSP_directory];

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UTCCPlugIn::handlerInit()
{
   U_TRACE(1, "UTCCPlugIn::handlerInit()")

   static char my_program[] =
"int main(int argc, char** argv)\n"
"{\n"
" printf(\"Hello World!\\n\");\n"
" return 0;\n"
"}\n";

   TCCState* s = tcc_new();

   tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

   if (tcc_compile_string(s, my_program) == -1)
      {
      U_SRV_LOG("initialization of plugin success");

      goto end;
      }

   U_SRV_LOG("initialization of plugin FAILED");

end:
   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int UTCCPlugIn::handlerREAD()
{
   U_TRACE(0, "UTCCPlugIn::handlerREAD()")

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UTCCPlugIn::handlerRequest()
{
   U_TRACE(0, "UTCCPlugIn::handlerRequest()")

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UTCCPlugIn::handlerReset()
{
   U_TRACE(0, "UTCCPlugIn::handlerReset()")

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UTCCPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "CSP_directory (UString " << (void*)&CSP_directory << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif