// test_json.cpp

#include <ulib/file.h>
#include <ulib/json/value.h>

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   UValue root;
   UString filename, dati;

   while (cin >> filename)
      {
      if (root.parse(UFile::contentOf(filename))) cout << root << '\n';

          root.clear();
      filename.clear();
      }
}
