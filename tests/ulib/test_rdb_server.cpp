// test_rdb_server.cpp

#include <ulib/string.h>

#ifndef U_NO_SSL
#define U_NO_SSL
#endif

#include <ulib/net/server/server_rdb.h>

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   UCDB y(false);
   off_t sz = 30000L;
   UString name(argv[1]);

   if (y.UFile::creat(name))
      {
      y.UFile::ftruncate(sz);
      y.UFile::memmap(PROT_READ | PROT_WRITE);

      cin >> y; // do ftruncate() and munmap()...

      y.UFile::close();
      y.UFile::reset();
      }

   URDBServer s(0, false);

   if (s.open(name)) s.go();
}
