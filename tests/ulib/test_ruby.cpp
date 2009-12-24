// test_ruby.cpp

#include <ulib/dynamic/ruby.h>

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   URUBY ruby;

   if (ruby.loadLibrary(argv[1]))
      {
      char* myargv[] = { "ruby", "ruby/fib.rb", 0 };

      ruby.run(2, myargv);

      if (argc > 2) ruby.run(argc - 2, argv + 2);
      }

   exit(0);
}
