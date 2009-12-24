// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    memory_pool.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_MEMORY_POOL_H
#define ULIB_MEMORY_POOL_H 1

// ---------------------------------------------------------------------------------------------------------------
// U_MAX_SIZE_PREALLOCATE: richiesta massima soddisfatta con metodo di preallocazione altrimenti chiamata a malloc

#define U_MAX_SIZE_PREALLOCATE 4096U

// U_STACK_TYPE_* 'tipi' stack per cui la richiesta viene gestita tramite preallocazione

/* NO DEBUG (64 bit)
-------------------------
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
  88 sizeof(UMimeMultipartMsg)
  88 sizeof(UPCRE)
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

#define U_STACK_TYPE_0     8U
#define U_STACK_TYPE_1    24U
#define U_STACK_TYPE_2    32U
#define U_STACK_TYPE_3    40U
#define U_STACK_TYPE_4    64U
#define U_STACK_TYPE_5    96U
#define U_STACK_TYPE_6   128U
#define U_STACK_TYPE_7   184U
#define U_STACK_TYPE_8   192U
#define U_STACK_TYPE_9   256U
#define U_STACK_TYPE_10  512U
#define U_STACK_TYPE_11 1024U
#define U_STACK_TYPE_12 2048U
#define U_STACK_TYPE_13 U_MAX_SIZE_PREALLOCATE

// U_NUM_STACK_TYPE: numero 'tipi' stack per cui la richiesta viene gestita tramite preallocazione

#define U_NUM_STACK_TYPE 14

/* Implements a simple stack allocator */

#define U_SIZE_TO_STACK_INDEX(sz)  ((sz) <= U_STACK_TYPE_0  ?  0 : \
                                    (sz) <= U_STACK_TYPE_1  ?  1 : \
                                    (sz) <= U_STACK_TYPE_2  ?  2 : \
                                    (sz) <= U_STACK_TYPE_3  ?  3 : \
                                    (sz) <= U_STACK_TYPE_4  ?  4 : \
                                    (sz) <= U_STACK_TYPE_5  ?  5 : \
                                    (sz) <= U_STACK_TYPE_6  ?  6 : \
                                    (sz) <= U_STACK_TYPE_7  ?  7 : \
                                    (sz) <= U_STACK_TYPE_8  ?  8 : \
                                    (sz) <= U_STACK_TYPE_9  ?  9 : \
                                    (sz) <= U_STACK_TYPE_10 ? 10 : \
                                    (sz) <= U_STACK_TYPE_11 ? 11 : \
                                    (sz) <= U_STACK_TYPE_12 ? 12 : 13)

#define U_SIZEOF_TO_STACK_INDEX(type) U_SIZE_TO_STACK_INDEX(sizeof(type))

struct U_EXPORT UMemoryPool {

#ifdef DEBUG
   // Segnala operazione in corso su stack (per check rientranza)
   static sig_atomic_t index_stack_busy;
#endif

   static uint32_t findStackIndex(uint32_t sz);

   // allocazione area di memoria <= U_MAX_SIZE_PREALLOCATE
   // ritorna area di memoria preallocata su stack predefiniti
   // (non garantito azzerata a meno di azzerare le deallocazioni...)

   static void* pop(            int stack_index);
   static void  push(void* ptr, int stack_index);

   static void* _malloc(size_t sz);
   static void _free(void* ptr, size_t sz);

   // special for class UString

   static void* _malloc_str(           size_t sz, uint32_t& capacity);
   static void    _free_str(void* ptr, size_t sz);

#ifdef U_OVERLOAD_NEW_DELETE
   // funzioni allocazione e deallocazione sostitutive a new() e delete()

   static void  _delete(void* ptr);
   static void* _new(int stack_index);
#endif

#ifdef DEBUG
   static void printInfo();
#endif
};

#ifdef U_OVERLOAD_NEW_DELETE // A macro for requiring the use of our versions of the C++ operators.

// The inlined new e delete operator (used by applications)

#include <new>

extern "C++" {
   inline void* operator new(std::size_t sz) throw(std::bad_alloc)
      {
      U_TRACE(0, "::new(%lu)", sz)

      U_INTERNAL_ASSERT_MAJOR(sz, 0)

      void* ptr = (sz <= U_MAX_SIZE_PREALLOCATE
                     ? UMemoryPool::_new(findStackIndex(sz + sizeof(int)))
                     : U_SYSCALL(malloc, "%u", sz));

      U_RETURN(ptr);
      }

   inline void* operator new(std::size_t sz, const std::nothrow_t& nothrow) throw()
      {
      U_TRACE(0, "::new(%lu,%p)", sz, &nothrow)

      return ::operator new(sz);
      }

   inline void* operator new[](std::size_t sz) throw (std::bad_alloc)
      {
      U_TRACE(0, "::new[](%lu)", sz)

      return ::operator new(sz);
      }

   inline void* operator new[](std::size_t sz, const std::nothrow_t& nothrow) throw()
      {
      U_TRACE(0, "::new[](%lu,%p)", sz, &nothrow)

      return ::operator new(sz);
      }

   inline void operator delete(void* ptr) throw()
      {
      U_TRACE(0, "::delete(%p)", ptr)

      UMemoryPool::_delete(ptr);
      }

   inline void operator delete(void* ptr, const std::nothrow_t& nothrow) throw()
      {
      U_TRACE(0, "::delete(%p,%p)", ptr, &nothrow)

      UMemoryPool::_delete(ptr);
      }

   inline void operator delete[](void* ptr) throw()
      {
      U_TRACE(0, "::delete[](%p)", ptr)

      UMemoryPool::_delete(ptr);
      }

   inline void operator delete[](void* ptr, const std::nothrow_t& nothrow) throw()
      {
      U_TRACE(0, "::delete[](%p,%p)", ptr, &nothrow)

      UMemoryPool::_delete(ptr);
      }
}
#endif

#endif
