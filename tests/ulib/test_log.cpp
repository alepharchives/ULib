// test_log.cpp

#include <ulib/log.h>

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   u_init_ulib_hostname();
   u_init_ulib_username();

   ULog y(U_STRING_FROM_CONSTANT("$PWD/test_log.log"), 1024, "%10D> ");

   int i, n = (argc > 1 ? atoi(argv[1]) : 10);

   for (i = 1; i <= n; ++i)
      {
      y.log("message %6d - %H %U %w \n", i);

      y.msync();
      }

   cout << "ok" << '\n';
}
