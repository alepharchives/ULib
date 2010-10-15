// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    common.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================
 
#include <ulib/utility/services.h>
#include <ulib/utility/interrupt.h>

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>
#endif

#ifdef HAVE_MAGIC
#  include <ulib/magic/magic.h>
#endif

#ifdef HAVE_SSL
#  include <openssl/ssl.h>
#  include <openssl/rand.h>
#  include <openssl/conf.h>
#endif

#ifdef HAVE_LIBXML2
#  include <libxml/parser.h>
#  include <libxml/xmlversion.h>
#endif

void ULib_init()
{
   U_TRACE(1, "ULib_init()")

   // setting from u_init(char** argv)...

   U_INTERNAL_DUMP("u_progname(%u) = %.*S u_cwd(%u) = %.*S", u_progname_len, u_progname_len, u_progname, u_cwd_len, u_cwd_len, u_cwd)

   UInterrupt::init();

   // TMPDIR is the canonical Unix environment variable which points to user scratch space. Most Unix utilities will honor the setting of this
   // variable and use its value to denote the scratch area for temporary files instead of the common default of /tmp.
   // Other forms sometimes accepted are TEMP, TEMPDIR, and TMP but these are used more commonly by non-POSIX Operating systems.

   // if defined and we aren't setuid/setgid, otherwise it will be created in /tmp

   if (UServices::isSetuidRoot() == false) u_tmpdir = (const char*) U_SYSCALL(getenv, "%S", "TMPDIR");

   if (u_tmpdir == 0) u_tmpdir = "/tmp";

#ifdef DEBUG
    UObjectIO::init();
#endif

#ifdef HAVE_MAGIC
   (void) UMagic::init();
#endif

#ifdef __MINGW32__
   WSADATA wsaData;
   WORD version_requested = MAKEWORD(2, 2); // version_high, version_low
   int err = U_SYSCALL(WSAStartup, "%d.%p", version_requested, &wsaData);

   // Confirm that the WinSock DLL supports 2.2. Note that if the DLL supports versions greater than 2.2 in addition to 2.2,
   // it will still return 2.2 in wVersion since that is the version we requested.

   if (err                             ||
       LOBYTE( wsaData.wVersion ) != 2 ||
       HIBYTE( wsaData.wVersion ) != 2)
      {
      WSACleanup();

      // Tell the user that we could not find a usable WinSock DLL

      U_ERROR("Couldn't find useable Winsock DLL. Must be at least 2.2. Aborting...", 0);
      }

   (void) U_SYSCALL(atexit, "%p", (vPF)&WSACleanup);
#endif

#ifdef HAVE_SSL
   // A typical TLS/SSL application will start with the library initialization,
   // will provide readable error messages and will seed the PRNG (Pseudo Random Number Generator).
   // The environment variable OPENSSL_CONFIG can be set to specify the location of the configuration file

   U_SYSCALL_VOID_NO_PARAM(SSL_load_error_strings);
   U_SYSCALL_VOID_NO_PARAM(SSL_library_init);

#  ifdef HAVE_OPENSSL_97
   U_SYSCALL_VOID(OPENSSL_config, "%S", 0);
#  endif
   U_SYSCALL_VOID_NO_PARAM(OpenSSL_add_all_ciphers);
   U_SYSCALL_VOID_NO_PARAM(OpenSSL_add_all_digests);

   // OpenSSL makes sure that the PRNG state is unique for each thread. On systems that provide "/dev/urandom",
   // the randomness device is used to seed the PRNG transparently. However, on all other systems, the application
   // is responsible for seeding the PRNG by calling RAND_add(),

#  ifdef __MINGW32__
   U_SYSCALL_VOID(srand, "%ld", u_start_time); // seed with time

   while (RAND_status() == 0) // Seed PRNG only if needed
      {
      // PRNG may need lots of seed data

      int tmp = U_SYSCALL_NO_PARAM(rand);

      RAND_seed(&tmp, sizeof(int));
      }
#  endif
#endif

#ifdef HAVE_LIBXML2
   U_SYSCALL_VOID_NO_PARAM(xmlInitParser); // init libxml

   LIBXML_TEST_VERSION
#endif

#if defined(SOLARIS) && (defined(SPARC) || defined(sparc))
   // make this if there are pointer misalligned (because pointers must be always a multiple of 4
   // (when running 32 bit applications))
   asm("ta 6");
#endif

   U_INTERNAL_DUMP("UStringRep::string_rep_null = %p", UStringRep::string_rep_null)
}
