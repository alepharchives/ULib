// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    ruby.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/dynamic/ruby.h>

// The types are needed because ANSI C distinguishes between pointer-to-object (data) and pointer-to-function.

typedef void (*ruby_init_t)    (void);
typedef void (*ruby_options_t) (int, char**);
typedef int  (*ruby_exec_t)    (void);
typedef int  (*ruby_cleanup_t) (int);

/*
typedef int  (*rb_str_new2_t)          (const char*);
typedef void (*ruby_script_t)          (char*);
typedef void (*rb_load_protect_t)      (int, int, int*);
typedef void (*ruby_init_loadpath_t)   (void);
*/

static ruby_init_t      ruby_init;
static ruby_options_t   ruby_options;
static ruby_exec_t      ruby_exec;
static ruby_cleanup_t   ruby_cleanup;

/*
static ruby_script_t        ruby_script;
static rb_str_new2_t        rb_str_new2;
static rb_load_protect_t    rb_load_protect;
static ruby_init_loadpath_t ruby_init_loadpath;
*/

void URUBY::setError()
{
   U_TRACE(0, "URUBY::URUBY::setError()")

   U_CHECK_MEMORY

   /*
   #define TAG_RETURN  0x1
   #define TAG_BREAK   0x2
   #define TAG_NEXT    0x3
   #define TAG_RETRY   0x4
   #define TAG_REDO    0x5
   #define TAG_RAISE   0x6
   #define TAG_THROW   0x7
   #define TAG_FATAL   0x8
   #define TAG_MASK    0xf
   */

   static const char* errlist[] = {
      "",                                 //  0 0x00
      "unexpected return",                //  1 0x01
      "unexpected break",                 //  2 0x02
      "unexpected next",                  //  3 0x03
      "retry outside of rescue clause",   //  4 0x04
      "unexpected redo",                  //  5 0x05
      "unhandled exception",              //  6 0x06
      "unknown longjmp status",           //  7 0x07
      "unhandled exception"               //  8 0x08
   };

   static char buffer[1024];

   (void) sprintf(buffer, "(%d, %s)", result,
                  (result >= 0 && result < 9 ? errlist[result] : "unknown longjmp status"));

   err = buffer;
}

// RUBY operations

bool URUBY::run(int argc, char** argv)
{
   U_TRACE(1, "URUBY::run(%d,%p)", argc, argv)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(argv)

   U_DUMP_EXEC(argv, environ)

   if (ruby_init == 0)
      {
      if (load())
         {
         ruby_init    = (ruby_init_t)    operator[]("ruby_init");
         ruby_options = (ruby_options_t) operator[]("ruby_options");
         ruby_exec    = (ruby_exec_t)    operator[]("ruby_exec");
         ruby_cleanup = (ruby_cleanup_t) operator[]("ruby_cleanup");
         }
      }

   U_INTERNAL_ASSERT_POINTER(ruby_init)
   U_INTERNAL_ASSERT_POINTER(ruby_options)
   U_INTERNAL_ASSERT_POINTER(ruby_exec)
   U_INTERNAL_ASSERT_POINTER(ruby_cleanup)

   U_SYSCALL_VOID_NO_PARAM(ruby_init);

   U_SYSCALL_VOID(ruby_options, "%d,%p", argc, argv);

   result = U_SYSCALL_NO_PARAM(ruby_exec);
   result = U_SYSCALL(ruby_cleanup, "%d", result);

   if (result)
      {
      setError();

      U_RETURN(false);
      }

   U_RETURN(true);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* URUBY::dump(bool reset) const
{
   UDynamic::dump(false);

   *UObjectIO::os << '\n'
                  << "result       " << result              << '\n'
                  << "ruby_init    " << (void*)ruby_init    << '\n'
                  << "ruby_exec    " << (void*)ruby_exec    << '\n'
                  << "ruby_options " << (void*)ruby_options << '\n'
                  << "ruby_cleanup " << (void*)ruby_cleanup;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
