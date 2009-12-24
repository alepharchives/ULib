// test_ssl_client.cpp

#include <ulib/utility/services.h>
#include <ulib/ssl/net/sslsocket.h>

static const char* getArg(const char* param) { return (param && *param ? param : 0); }

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   USSLSocket x;

   // Check server certificates agains our known trusted certificate

   if (x.useDHFile() &&
       x.setContext(getArg(argv[1]), getArg(argv[2]), getArg(argv[3]), getArg(argv[4]), getArg(argv[5]), atoi(argv[6])) &&
       x.connectServer(U_STRING_FROM_CONSTANT("localhost"), 80))
      {
      U_DUMP("getPeerCertificate() = %p", x.getPeerCertificate())

      const char* str = argv[7];
      int size        = strlen(str);

      for (int i = 0; i < 2; ++i)
         {
         if (x.send((void*)str, size) == size)
            {
            cout << str << '\n';

            char buffer[1024];

            if (x.recv(buffer, size) == size)
               {
               cout.write(buffer, size);

               cout << '\n';
               }
            }
         }
      }
}
