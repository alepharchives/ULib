// test_rdb_server.cpp

#include <ulib/file_config.h>
#include <ulib/net/server/server_rdb.h>

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   UCDB y(false);
   UFileConfig fcg;
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

   if (s.open(name))
      {
      UString plugin_dir( argv[2]);
      UString plugin_list(argv[3]);

      if (argv[4])
         {
         UString x(argv[4]);

         (void) fcg.open(x);
         }

      s.loadPlugins(plugin_dir, plugin_list, &fcg);

      s.go();
      }
}
