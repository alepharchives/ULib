// test_server.cpp

#include <ulib/container/vector.h>

#ifndef U_NO_SSL
#define U_NO_SSL
#endif

#include <ulib/net/tcpsocket.h>
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

   server.loadPlugins(plugin_dir, plugin_list, 0);

   server.go();
}
