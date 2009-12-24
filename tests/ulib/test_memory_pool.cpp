// test_memory_pool.cpp

#include <ulib/debug/crono.h>
#include <ulib/string.h>
#include <ulib/utility/interrupt.h>
#include <sys/time.h>

// #define PRINT_SIZE

#ifdef PRINT_SIZE 
#  include <ulib/all.h>

#define U_PRINT_SIZEOF(class) printf("%ld sizeof(%s)\n", sizeof(class), #class);

static void print_size()
{
   U_TRACE(5, "print_size()")

   U_PRINT_SIZEOF(UApplication)
   U_PRINT_SIZEOF(UBison)
   U_PRINT_SIZEOF(UCDB)
   U_PRINT_SIZEOF(UCGI)
   U_PRINT_SIZEOF(UCURL)
   U_PRINT_SIZEOF(UCache)
   U_PRINT_SIZEOF(UCertificate)
// U_PRINT_SIZEOF(UCgiInput)
// U_PRINT_SIZEOF(UClassSlot)
   U_PRINT_SIZEOF(UClientImage<UTCPSocket>)
   U_PRINT_SIZEOF(UCommand)
// U_PRINT_SIZEOF(UConnection)
// U_PRINT_SIZEOF(UConstMethodSlot)
   U_PRINT_SIZEOF(UCrl)
   U_PRINT_SIZEOF(UDate)
   U_PRINT_SIZEOF(UDialog)
// U_PRINT_SIZEOF(UFCgi)
   U_PRINT_SIZEOF(UFile)
   U_PRINT_SIZEOF(UFileConfig)
   U_PRINT_SIZEOF(UFlexer)
   U_PRINT_SIZEOF(UFtpClient)
// U_PRINT_SIZEOF(UFuncSlot)
   U_PRINT_SIZEOF(UHashMap<UString>)
   U_PRINT_SIZEOF(UHashMapNode)
   U_PRINT_SIZEOF(UHttpClient<UTCPSocket>)
   U_PRINT_SIZEOF(UIPAddress)
   U_PRINT_SIZEOF(ULDAP)
   U_PRINT_SIZEOF(ULDAPEntry)
   U_PRINT_SIZEOF(ULock)
   U_PRINT_SIZEOF(ULog)
   U_PRINT_SIZEOF(UMagic)
// U_PRINT_SIZEOF(UMethodSlot)
   U_PRINT_SIZEOF(UMimeEntity)
   U_PRINT_SIZEOF(UMimeHeader)
   U_PRINT_SIZEOF(UMimeMessage)
   U_PRINT_SIZEOF(UMimeMultipart)
   U_PRINT_SIZEOF(UMimeMultipartMsg)
   U_PRINT_SIZEOF(UMimePKCS7)
   U_PRINT_SIZEOF(UNotifier)
   U_PRINT_SIZEOF(UOptions)
   U_PRINT_SIZEOF(UPCRE)
   U_PRINT_SIZEOF(UPKCS10)
   U_PRINT_SIZEOF(UPKCS7)
   U_PRINT_SIZEOF(UPlugIn<void*>)
   U_PRINT_SIZEOF(UProcess)
   U_PRINT_SIZEOF(UQueryNode)
   U_PRINT_SIZEOF(UQueryParser)
   U_PRINT_SIZEOF(URDB)
   U_PRINT_SIZEOF(URDBClient<UTCPSocket>)
   U_PRINT_SIZEOF(URDBServer)
// U_PRINT_SIZEOF(URUBY)
   U_PRINT_SIZEOF(USOAPClient<UTCPSocket>)
   U_PRINT_SIZEOF(USOAPEncoder)
   U_PRINT_SIZEOF(USOAPFault)
   U_PRINT_SIZEOF(USOAPGenericMethod)
   U_PRINT_SIZEOF(USOAPObject)
   U_PRINT_SIZEOF(USOAPParser)
   U_PRINT_SIZEOF(USSHSocket)
   U_PRINT_SIZEOF(USSLFtpClient)
   U_PRINT_SIZEOF(USSLSocket)
   U_PRINT_SIZEOF(USemaphore)
   U_PRINT_SIZEOF(UServer<UTCPSocket>)
   U_PRINT_SIZEOF(USignature)
   U_PRINT_SIZEOF(USmtpClient)
   U_PRINT_SIZEOF(USocket)
   U_PRINT_SIZEOF(UString)
   U_PRINT_SIZEOF(UStringRep)
   U_PRINT_SIZEOF(UTCPSocket)
// U_PRINT_SIZEOF(UTimeStamp)
   U_PRINT_SIZEOF(UTimeVal)
   U_PRINT_SIZEOF(UTimer)
   U_PRINT_SIZEOF(UTokenizer)
#ifdef DEBUG
   U_PRINT_SIZEOF(UTrace)
#endif
   U_PRINT_SIZEOF(UTree<UString>)
   U_PRINT_SIZEOF(UUDPSocket)
   U_PRINT_SIZEOF(UVector<UString>)
   U_PRINT_SIZEOF(UXMLAttribute)
   U_PRINT_SIZEOF(UXMLElement)
   U_PRINT_SIZEOF(UXMLParser)
   U_PRINT_SIZEOF(UZIP)
   U_PRINT_SIZEOF(Url)
}
#endif

/* NO DEBUG (64 bit)
------------------------------
   1 sizeof(UApplication)
   1 sizeof(UMagic)

   8 sizeof(UCGI)
   8 sizeof(UCertificate)
   8 sizeof(UCrl)
   8 sizeof(UPKCS10)
   8 sizeof(UString) 
-------------------------
   U_STACK_TYPE_0

  12 sizeof(UProcess)
  16 sizeof(UDate)

  16 sizeof(ULock)
  16 sizeof(UNotifier)
  16 sizeof(UPKCS7)
  16 sizeof(USemaphore)
  16 sizeof(UTimeVal)
  16 sizeof(UVector<UString>)
  16 sizeof(UXMLParser)

  24 sizeof(USOAPObject)
  24 sizeof(UStringRep)
-------------------------
   U_STACK_TYPE_1

  32 sizeof(UCache)
  32 sizeof(UHashMapNode)                                                      
  32 sizeof(ULDAPEntry)
  32 sizeof(UQueryNode)
  32 sizeof(USOAPFault)
  32 sizeof(UTimer)
  32 sizeof(UTokenizer)
  32 sizeof(UTree<UString>)
  32 sizeof(UXMLAttribute)
-------------------------
   U_STACK_TYPE_2

  40 sizeof(UHashMap<UString>)
  40 sizeof(ULDAP)
  40 sizeof(USOAPGenericMethod)
  40 sizeof(Url)
-------------------------
   U_STACK_TYPE_3

  48 sizeof(UCURL)
  48 sizeof(UDialog)
  48 sizeof(UMimeHeader)
  48 sizeof(UPlugIn<void*>)
  48 sizeof(UQueryParser)
  48 sizeof(USOAPEncoder)
  48 sizeof(UXMLElement)
  56 sizeof(UMimeEntity)
  56 sizeof(UOptions)
  64 sizeof(UClientImage<UTCPSocket>)
  64 sizeof(USignature)
-------------------------
   U_STACK_TYPE_4

  80 sizeof(UZIP)
  80 sizeof(UCommand)
  80 sizeof(UIPAddress)
  80 sizeof(UMimePKCS7)
  88 sizeof(UPCRE)
  88 sizeof(UMimeMultipartMsg)
  96 sizeof(UServer<UTCPSocket>)
-------------------------
   U_STACK_TYPE_5

 104 sizeof(URDBServer)
 112 sizeof(UMimeMessage)
 128 sizeof(USOAPParser)
-------------------------
   U_STACK_TYPE_6

 136 sizeof(UMimeMultipart)
 168 sizeof(URDBClient<UTCPSocket>)
 184 sizeof(UFile)
 184 sizeof(ULog)
-------------------------
   U_STACK_TYPE_7

 192 sizeof(USocket)
 192 sizeof(UTCPSocket)
 192 sizeof(UUDPSocket)
-------------------------
   U_STACK_TYPE_8

 216 sizeof(UBison)
 216 sizeof(UFlexer)
 216 sizeof(UHttpClient<UTCPSocket>)
 216 sizeof(USSLSocket)
 256 sizeof(UFileConfig)
 256 sizeof(USSHSocket)
 256 sizeof(USmtpClient)
-------------------------
   U_STACK_TYPE_9

 264 sizeof(UCDB)
 264 sizeof(USOAPClient<UTCPSocket>)
 416 sizeof(UFtpClient)
 416 sizeof(USSLFtpClient)
 480 sizeof(URDB)
 512
-------------------------
   U_STACK_TYPE_10

 1024
-------------------------
   U_STACK_TYPE_11

 2048
-------------------------
   U_STACK_TYPE_12

 4096
-------------------------
   U_STACK_TYPE_13
*/

static void check_size()
{
   U_TRACE(5, "check_size()")

   /*
   uint32_t stack_index;

   for (uint32_t sz = 1; sz <= U_MAX_SIZE_PREALLOCATE; ++sz)
      {
      stack_index = UMemoryPool::findStackIndex(sz);

      printf("%4u %2u\n", sz, stack_index);
      }
   */

   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_0 - 1) ==  0 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_0 - 0) ==  0 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_0 + 1) ==  1 )

   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_1 - 1) ==  1 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_1 - 0) ==  1 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_1 + 1) ==  2 )

   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_2 - 1) ==  2 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_2 - 0) ==  2 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_2 + 1) ==  3 )

   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_3 - 1) ==  3 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_3 - 0) ==  3 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_3 + 1) ==  4 )

   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_4 - 1) ==  4 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_4 - 0) ==  4 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_4 + 1) ==  5 )

   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_5 - 1) ==  5 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_5 - 0) ==  5 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_5 + 1) ==  6 )

   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_6 - 1) ==  6 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_6 - 0) ==  6 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_6 + 1) ==  7 )

   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_7 - 1) ==  7 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_7 - 0) ==  7 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_7 + 1) ==  8 )

   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_8 - 1) ==  8 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_8 - 0) ==  8 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_8 + 1) ==  9 )

   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_9 - 1) ==  9 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_9 - 0) ==  9 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_9 + 1) ==  10 )

   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_10 - 1) ==  10 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_10 - 0) ==  10 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_10 + 1) ==  11 )

   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_11 - 1) ==  11 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_11 - 0) ==  11 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_11 + 1) ==  12 )

   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_12 - 1) ==  12 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_12 - 0) ==  12 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_12 + 1) ==  13 )

   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_13 - 1) ==  13 )
   U_ASSERT( UMemoryPool::findStackIndex(U_STACK_TYPE_13 - 0) ==  13 )
}

static struct itimerval timeval = { { 0, 2000 }, { 0, 2000 } };

static RETSIGTYPE
manage_alarm(int signo)
{
   U_TRACE(5,"[SIGALRM} manage_alarm(%d)",signo)

   static char* ptr;
   static size_t sz = 255;

   if (ptr) delete[] ptr;

   ptr = new char[sz];

   (void) U_SYSCALL(setitimer,"%d,%p,%p",ITIMER_REAL,&timeval,0);
}

int
U_EXPORT main(int argc, char** argv)
{
   U_ULIB_INIT(argv);

   U_TRACE(5, "main(%d)", argc)

   if (argc > 3)
      {
      UInterrupt::setHandlerForSignal(SIGALRM, (sighandler_t)&manage_alarm);

      (void) U_SYSCALL(setitimer,"%d,%p,%p",ITIMER_REAL,&timeval,0);
      }

   check_size();

#  define U_NUM_ENTRY_MEM_BLOCK 32

   int i, j, k;
   int n = U_NUM_ENTRY_MEM_BLOCK * (argc > 1 ? atoi(argv[1]) : 1);

   char* vptr[n];
   UString* obj[n];

   UCrono crono;

   crono.start();

   for (i = 0; i < 10; ++i)
      {
      for (j = 0; j <= U_NUM_STACK_TYPE; ++j)
         {
         for (k = 0; k < n; ++k)
            {
            vptr[k] = new char[3 << j];

            obj[k] = U_NEW(UString(U_CONSTANT_TO_PARAM("allocated")));
            }

         for (k = 0; k < n; ++k)
            {
            delete[] vptr[k];

            delete obj[k];
            }
         }
      }

   crono.stop();

   if (argc > 2) printf("Time Consumed with U_NUM_ENTRY_MEM_BLOCK(%d) = %ld ms\n", n, crono.getTimeElapsed());

#ifdef DEBUG
   UMemoryPool::printInfo();
#endif

   // RISULTATI

#ifdef NDEBUG
#  ifdef U_MEMORY_POOL
   // Time Consumed with U_NUM_ENTRY_MEM_BLOCK(32)   =     3 ms
   // Time Consumed with U_NUM_ENTRY_MEM_BLOCK(320)  =    44 ms
   // Time Consumed with U_NUM_ENTRY_MEM_BLOCK(3200) =   670 ms
#  else
   // Time Consumed with U_NUM_ENTRY_MEM_BLOCK(32)   =     6 ms
   // Time Consumed with U_NUM_ENTRY_MEM_BLOCK(320)  =   260 ms
   // Time Consumed with U_NUM_ENTRY_MEM_BLOCK(3200) =  3680 ms
#  endif
#else
#  ifdef U_MEMORY_POOL
   // Time Consumed with U_NUM_ENTRY_MEM_BLOCK(32)   =    60 ms
   // Time Consumed with U_NUM_ENTRY_MEM_BLOCK(320)  =  1463 ms
   // Time Consumed with U_NUM_ENTRY_MEM_BLOCK(3200) = 15500 ms
#  else
   // Time Consumed with U_NUM_ENTRY_MEM_BLOCK(32)   =    51 ms
   // Time Consumed with U_NUM_ENTRY_MEM_BLOCK(320)  =   760 ms
   // Time Consumed with U_NUM_ENTRY_MEM_BLOCK(3200) =  8400 ms
#  endif
#endif

#ifdef PRINT_SIZE 
   print_size();
#endif
}
