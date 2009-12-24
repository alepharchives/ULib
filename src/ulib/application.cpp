// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    application.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/application.h>

#ifdef HAVE_LIBXML2
#  include <libxml/parser.h>
#  include <libxml/xmlmemory.h>
#endif

int       UApplication::exit_value;
UString*  UApplication::str;
UOptions* UApplication::opt;
uint32_t  UApplication::num_args;

UApplication::~UApplication()
{
   U_TRACE_UNREGISTER_OBJECT(0, UApplication)

   if (opt)
      {
      delete opt;
             opt = 0;
      delete str;
             str = 0;
      }

#ifdef HAVE_LIBXML2 // Shutdown libxml
   U_SYSCALL_VOID_NO_PARAM(xmlCleanupParser);
   U_SYSCALL_VOID_NO_PARAM(xmlMemoryDump);
#endif
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UApplication::dump(bool reset) const
{
   *UObjectIO::os << "num_args                       " << num_args   << '\n'
                  << "exit_value                     " << exit_value << '\n'
                  << "str (UString                   " << (void*)str << ")\n"
                  << "opt (UOptions                  " << (void*)opt << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
