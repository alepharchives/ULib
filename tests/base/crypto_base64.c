// crypto_base64.c

#include <ulib/base/coder/base64.h>

#include <stdlib.h>

#define U_ENCRYPT    1
#define U_DECRYPT    0
#define U_BUFLEN     4096

static const char* usage = "Usage: crypto_base64 [-d]\n";

static void do_cipher(char* pw, int operation)
{
   long ebuflen;
   unsigned char  buf[U_BUFLEN];
   unsigned char ebuf[U_BUFLEN + 8];

   U_INTERNAL_TRACE("do_cipher(%s,%p)", pw, operation)

#  ifdef __MINGW32__
   (void) setmode(1, O_BINARY);
#  endif

   while (true)
      {
      int readlen = read(STDIN_FILENO, buf, sizeof(buf));

      if (readlen <= 0)
         {
         if (readlen == 0) break;
         else
            {
            perror("read");

            exit(1);
            }
         }

      if (operation == U_ENCRYPT) ebuflen = u_base64_encode(buf, readlen, ebuf);
      else                        ebuflen = u_base64_decode(buf, readlen, ebuf);

      write(STDOUT_FILENO, ebuf, ebuflen);
      }
}

int main(int argc, char* argv[])
{
   u_init_ulib(argv);

   U_INTERNAL_TRACE("main(%d,%p)", argc, argv)

   if      (argc == 1)                                  do_cipher(argv[1], U_ENCRYPT);
   else if (argc == 2 && U_STRNCMP(argv[1], "-d") == 0) do_cipher(argv[2], U_DECRYPT);
   else
      {
      fprintf(stderr, "%s", usage);

      exit(1);
      }

   return 0;
}
