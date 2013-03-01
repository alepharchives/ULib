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
 
#include <ulib/file.h>
#include <ulib/utility/interrupt.h>
#include <ulib/container/hash_map.h>

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>
#endif

#ifdef USE_LIBSSL
#  include <openssl/ssl.h>
#  include <openssl/rand.h>
#  include <openssl/conf.h>

void ULib_init_openssl()
{
   U_TRACE(1, "ULib_init_openssl()")

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
}
#endif

void ULib_init()
{
   U_TRACE(1, "ULib_init()")

#ifdef DEBUG
   static bool init;

   U_INTERNAL_ASSERT_EQUALS(init, false)

   init = true;
#endif

   // setting from u_init_ulib(char** argv)...

   U_INTERNAL_DUMP("u_progname(%u) = %.*S u_cwd(%u) = %.*S", u_progname_len, u_progname_len, u_progname, u_cwd_len, u_cwd_len, u_cwd)

   // allocation from memory pool

   u_buffer =      (char*)UMemoryPool::pop(U_SIZE_TO_STACK_INDEX(U_MAX_SIZE_PREALLOCATE));

#ifdef DEBUG
   UObjectIO::init((char*)UMemoryPool::pop(U_SIZE_TO_STACK_INDEX(U_MAX_SIZE_PREALLOCATE)), U_MAX_SIZE_PREALLOCATE);
#endif

   UString::ptrbuf =
   UString::appbuf = (char*)UMemoryPool::pop(U_SIZE_TO_STACK_INDEX(1024));
   UFile::cwd_save = (char*)UMemoryPool::pop(U_SIZE_TO_STACK_INDEX(1024));

   UInterrupt::init();

   // TMPDIR is the canonical Unix environment variable which points to user scratch space. Most Unix utilities will honor the setting of this
   // variable and use its value to denote the scratch area for temporary files instead of the common default of /tmp.
   // Other forms sometimes accepted are TEMP, TEMPDIR, and TMP but these are used more commonly by non-POSIX Operating systems.

   // if defined, otherwise it will be created in /tmp

   u_tmpdir = (const char*) U_SYSCALL(getenv, "%S", "TMPDIR");

   if (u_tmpdir == NullPtr) u_tmpdir = "/tmp";

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

      U_ERROR("Couldn't find useable Winsock DLL. Must be at least 2.2. Aborting...");
      }

   (void) U_SYSCALL(atexit, "%p", (vPF)&WSACleanup);
#endif

#if defined(SOLARIS) && (defined(SPARC) || defined(sparc))
   // make this if there are pointer misalligned
   // (because pointers must be always a multiple of 4 (when running 32 bit applications))
   asm("ta 6");
#endif

   U_INTERNAL_ASSERT_EQUALS(sizeof(UStringRep), sizeof(ustringrep))

   U_INTERNAL_DUMP("u_is_tty = %b UStringRep::string_rep_null = %p UString::string_null = %p", u_is_tty, UStringRep::string_rep_null, UString::string_null)

   U_INTERNAL_DUMP("sizeof(off_t) = %u SIZEOF_OFF_T = %u", sizeof(off_t), SIZEOF_OFF_T)

/* NB: there are to many exceptions...
#if defined(_LARGEFILE_SOURCE) && !defined(__MINGW32__) 
   U_INTERNAL_ASSERT_EQUALS(sizeof(off_t), SIZEOF_OFF_T)
#endif
*/

   static ustringrep pkey = { U_STRINGREP_FROM_CONSTANT("") };
         uustringrep    u = { &pkey };

   U_INTERNAL_ASSERT_EQUALS(UHashMap<void*>::pkey, 0)

   UHashMap<void*>::pkey = u.p2;

   U_INTERNAL_ASSERT(UHashMap<void*>::pkey->invariant())

#ifdef USE_LIBSSL
   ULib_init_openssl();
#endif
}
