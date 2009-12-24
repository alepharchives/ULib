// test_server.cpp

#define U_NO_SSL

#include <ulib/net/tcpsocket.h>
#include <ulib/container/vector.h>
#include <ulib/net/server/server.h>

U_MACROSERVER(UServerExample, UClientImage<UTCPSocket>, UTCPSocket);

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   UServerExample server(0);
   UString plugin_dir( argv[1]);
   UString plugin_list(argv[2]);
   UVector<UString> vname(plugin_list);

   server.loadPlugins(plugin_dir, vname, 0);

   server.go();
}
