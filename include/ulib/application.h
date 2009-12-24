// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    application.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_APPLICATION_H
#define ULIB_APPLICATION_H 1

#include <ulib/internal/common.h>

#ifdef U_STD_STRING
class UOptions;
#else
#  include <ulib/options.h>
#endif

#ifdef DEBUG
#define U_MAIN_END(value) return value
#else
#define U_MAIN_END(value) ::exit(value)
#endif

#define U_MAIN(_class) \
int U_EXPORT main(int argc, char* argv[], char* env[]) \
{ \
   U_ULIB_INIT(argv); \
   U_TRACE(5, "::main(%d,%p,%p)", argc, argv, env) \
   _class application; \
   application.run(argc, argv, env); \
   U_MAIN_END(UApplication::exit_value); \
}

/*
#define U_MAIN(_class) \
int WINAPI WinMain (HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR command_line, int cmd_show) \
{ \
   U_ULIB_INIT(__argv); \
   int argc = 0; \
   char** _argv = __argv; \
   while (*_argv++) ++argc; \
   U_TRACE(5, "::main(%d,%p,%p)", argc, __argv, 0) \
   _class application; \
   application.run(argc, argv, 0); \
   U_MAIN_END(UApplication::exit_value); \
}
*/

class U_EXPORT UApplication {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   static UString* str;
   static UOptions* opt;
   static int exit_value;
   static uint32_t num_args;

   // COSTRUTTORI

   UApplication()
      {
      U_TRACE_REGISTER_OBJECT(0, UApplication, "", 0)
      }

   ~UApplication();

   // SERVICES

   static void run(int argc, char* argv[], char* env[])
      {
      U_TRACE(0+256, "UApplication::run(%d,%p,%p)", argc, argv, env)

      U_DUMP_EXEC(argv, env);

#  ifdef U_OPTIONS
      if (argc > 1) // se esistono opzioni, queste vengono processate...
         {
         U_INTERNAL_ASSERT_EQUALS_MSG(opt, 0, "you can't allocate more than ONE INSTANCE of UOptions for class UApplication")

         U_NEW_ULIB_OBJECT(opt, UOptions(argc));

#     ifndef U_OPTIONS_1
#     define U_OPTIONS_1 ""
#     endif
#     ifndef U_OPTIONS_2
#     define U_OPTIONS_2 ""
#     endif
#     ifndef U_OPTIONS_3
#     define U_OPTIONS_3 ""
#     endif

         U_NEW_ULIB_OBJECT(str, UString(U_CONSTANT_TO_PARAM(U_OPTIONS U_OPTIONS_1 U_OPTIONS_2 U_OPTIONS_3)));

         opt->load(*str);

         num_args = opt->getopt(argc, argv, &optind);
         }
#  endif
      }

   static bool isOptions() { return (opt != 0); }

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

private:
   UApplication(const UApplication&)            {}
   UApplication& operator=(const UApplication&) { return *this; }
};

#endif
