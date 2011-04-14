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
#include <ulib/utility/services.h>

#ifdef HAVE_LIBXML2
#  include <libxml/parser.h>
#  include <libxml/xmlmemory.h>
#endif

int      UApplication::exit_value;
bool     UApplication::is_options;
uint32_t UApplication::num_args;

UApplication::UApplication() : opt(126)
{
   U_TRACE_REGISTER_OBJECT(0, UApplication, "")
}

UApplication::~UApplication()
{
   U_TRACE_UNREGISTER_OBJECT(0, UApplication)

#ifdef HAVE_SSL
   if (UServices::CApath) delete UServices::CApath;
#endif

#ifdef HAVE_LIBXML2 // Shutdown libxml
   U_SYSCALL_VOID_NO_PARAM(xmlCleanupParser);
   U_SYSCALL_VOID_NO_PARAM(xmlMemoryDump);
#endif

#ifdef DEBUG
   // AT EXIT

   U_INTERNAL_DUMP("u_fns_index = %d", u_fns_index)

   for (int i = 0; i < u_fns_index; ++i) { U_INTERNAL_DUMP("u_fns[%2u] = %p", i, u_fns[i]) }
#endif
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UApplication::dump(bool reset) const
{
   *UObjectIO::os << "num_args                       " << num_args    << '\n'
                  << "exit_value                     " << exit_value  << '\n'
                  << "is_options                     " << is_options  << '\n'
                  << "str (UString                   " << (void*)&str << ")\n"
                  << "opt (UOptions                  " << (void*)&opt << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
