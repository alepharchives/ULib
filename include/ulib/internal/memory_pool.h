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

#include <memory>
#include <iostream>

// ---------------------------------------------------------------------------------------------------------------
// U_MAX_SIZE_PREALLOCATE: richiesta massima soddisfatta con metodo di preallocazione altrimenti chiamata a malloc

#define U_MAX_SIZE_PREALLOCATE 4096U

// U_STACK_TYPE_* 'tipi' stack per cui la richiesta viene gestita tramite preallocazione

#ifdef HAVE_ARCH64
/* NO DEBUG (64 bit)
-------------------------
   1 sizeof(UMagic)
   1 sizeof(UNotifier)
   8 sizeof(UCrl)
   8 sizeof(UPKCS10)
   8 sizeof(UString) <==
   8 sizeof(UCertificate)
-------------------------
   U_STACK_TYPE_0

  12 sizeof(UProcess)
  16 sizeof(ULock)
  16 sizeof(UTimer)
  16 sizeof(UPKCS7)
  16 sizeof(UTimeVal)
  16 sizeof(UTimeDate)
  16 sizeof(UXMLParser)
  16 sizeof(URDBServer)
  16 sizeof(UVector<UString>)
  16 sizeof(UServer<UTCPSocket>)
  24 sizeof(USemaphore)
  24 sizeof(UStringRep) <==
  24 sizeof(USOAPObject)
-------------------------
   U_STACK_TYPE_1

  32 sizeof(ULDAPEntry)
  32 sizeof(UQueryNode)
  32 sizeof(USOAPFault)
  32 sizeof(UTokenizer)
  32 sizeof(UHashMapNode) <==
  32 sizeof(UXMLAttribute)
  32 sizeof(UTree<UString>)
-------------------------
   U_STACK_TYPE_2

  40 sizeof(Url)
  40 sizeof(ULDAP)
  40 sizeof(UCache)
  40 sizeof(UHashMap<UString>)
  48 sizeof(UCURL)
  48 sizeof(UDialog)
  48 sizeof(UMimeHeader)
  48 sizeof(UMimeEntity)
  48 sizeof(UXMLElement)
  48 sizeof(UQueryParser)
  48 sizeof(USOAPEncoder)
  48 sizeof(USOAPGenericMethod)
  56 sizeof(UOptions)
  56 sizeof(UIPAddress)
  56 sizeof(UPlugIn<void*>)
  56 sizeof(UHTTP::UFileCacheData) <==
-------------------------
   U_STACK_TYPE_3

  64 sizeof(UPCRE)
  64 sizeof(UCommand)
  64 sizeof(UApplication)
  72 sizeof(UMimePKCS7)
  72 sizeof(UClientImage<UTCPSocket>)
  80 sizeof(UZIP)
  88 sizeof(UMimeMultipartMsg)
  96 sizeof(UMimeMessage)
 128 sizeof(USOAPParser)
 128 sizeof(UMimeMultipart)
-------------------------
   U_STACK_TYPE_4

 144 sizeof(USocket)
 144 sizeof(UTCPSocket)
 144 sizeof(UUDPSocket)
 176 sizeof(USSLSocket)
 184 sizeof(ULog)
 184 sizeof(UFile)
 192 sizeof(URDBClient<UTCPSocket>)
 216 sizeof(UBison)
 216 sizeof(UFlexer)
 232 sizeof(USmtpClient)
 256 sizeof(UFileConfig)
 256 sizeof(UHttpClient<UTCPSocket>)
-------------------------
   U_STACK_TYPE_5

 296 sizeof(UCDB)
 304 sizeof(UModNoCatPeer: 64bit) <==
 304 sizeof(USOAPClient<UTCPSocket>)
 360 sizeof(UFtpClient)
 512 sizeof(URDB)
-------------------------
   U_STACK_TYPE_6
*/
#else
/* NO DEBUG (32 bit)
-------------------------
   1 sizeof(UMagic)
   1 sizeof(UNotifier)
   4 sizeof(UCrl)
   4 sizeof(UPKCS10)
   4 sizeof(UString) <==
   4 sizeof(UCertificate)
-------------------------
   U_STACK_TYPE_0

   8 sizeof(ULock)
   8 sizeof(UTimer)
   8 sizeof(UPKCS7)
   8 sizeof(UTimeVal)
  12 sizeof(UProcess)
  12 sizeof(USemaphore)
  12 sizeof(URDBServer)
  12 sizeof(UVector<UString>)
  12 sizeof(UServer<UTCPSocket>)
  16 sizeof(UTimeDate)
  16 sizeof(UXMLParser)
  16 sizeof(UQueryNode)
  16 sizeof(USOAPFault)
  16 sizeof(UTokenizer)
  16 sizeof(UStringRep) <==
  16 sizeof(USOAPObject)
  16 sizeof(UHashMapNode) <==
-------------------------
   U_STACK_TYPE_1

  20 sizeof(UCache)
  20 sizeof(UTree<UString>)
  24 sizeof(UQueryParser)
  24 sizeof(USOAPGenericMethod)
  28 sizeof(UCURL)
  28 sizeof(UDialog)
  28 sizeof(UMimeEntity)
  28 sizeof(USOAPEncoder)
  28 sizeof(UPlugIn<void*>)
  32 sizeof(UOptions)
  32 sizeof(UHashMap<UString>)
  36 sizeof(Url)
  36 sizeof(UApplication)
  36 sizeof(UMimeHeader)
  36 sizeof(UHTTP::UFileCacheData) <==
-------------------------
   U_STACK_TYPE_2

  40 sizeof(UPCRE)
  40 sizeof(UCommand)
  40 sizeof(UMimePKCS7)
  44 sizeof(UZIP)
  48 sizeof(UIPAddress)
  48 sizeof(UClientImage<UTCPSocket>)
  56 sizeof(UMimeMessage)
  68 sizeof(USOAPParser)
  80 sizeof(UMimeMultipart)
  80 sizeof(UMimeMultipartMsg)
 120 sizeof(ULog)
 120 sizeof(UFile)
 120 sizeof(URDBClient<UTCPSocket>)
 124 sizeof(USocket)
 124 sizeof(UTCPSocket)
 124 sizeof(UUDPSocket)
 128
-------------------------
   U_STACK_TYPE_3

 144 sizeof(USSLSocket)
 156 sizeof(UHttpClient<UTCPSocket>)
 172 sizeof(UFileConfig)
 176 sizeof(USmtpClient)
 180 sizeof(USOAPClient<UTCPSocket>)
 196 sizeof(UModNoCatPeer: 32bit) <==
 200 sizeof(UCDB)
-------------------------
   U_STACK_TYPE_4

 256
-------------------------
   U_STACK_TYPE_5

 300 sizeof(UFtpClient)
 336 sizeof(URDB)
 512
-------------------------
   U_STACK_TYPE_6
*/
#endif
/*
 1024
-------------------------
   U_STACK_TYPE_7

 2048
-------------------------
   U_STACK_TYPE_8

 4096
-------------------------
   U_STACK_TYPE_9
*/

#ifdef HAVE_ARCH64
#  define U_STACK_TYPE_0   8U
#  define U_STACK_TYPE_1  24U
#  define U_STACK_TYPE_2  32U
#  define U_STACK_TYPE_3  56U
#  define U_STACK_TYPE_4 128U
#else
#  define U_STACK_TYPE_0   4U
#  define U_STACK_TYPE_1  16U
#  define U_STACK_TYPE_2  36U
#  define U_STACK_TYPE_3 128U
#  define U_STACK_TYPE_4 196U // sizeof(UModNoCatPeer: 32bit) 
#endif

// NB: con U_NUM_ENTRY_MEM_BLOCK == 32 sono necessari i tipi stack
//     multipli di 2 a partire da 128 per i blocchi puntatori per 32bit arch...

#define U_STACK_TYPE_5  256U
#define U_STACK_TYPE_6  512U
#define U_STACK_TYPE_7 1024U
#define U_STACK_TYPE_8 2048U
#define U_STACK_TYPE_9 U_MAX_SIZE_PREALLOCATE

// U_NUM_STACK_TYPE: numero 'tipi' stack per cui la richiesta viene gestita tramite preallocazione

#define U_NUM_STACK_TYPE 10

/* Implements a simple stack allocator */

#define U_SIZE_TO_STACK_INDEX(sz) ((sz) <= U_STACK_TYPE_0 ? 0 : \
                                   (sz) <= U_STACK_TYPE_1 ? 1 : \
                                   (sz) <= U_STACK_TYPE_2 ? 2 : \
                                   (sz) <= U_STACK_TYPE_3 ? 3 : \
                                   (sz) <= U_STACK_TYPE_4 ? 4 : \
                                   (sz) <= U_STACK_TYPE_5 ? 5 : \
                                   (sz) <= U_STACK_TYPE_6 ? 6 : \
                                   (sz) <= U_STACK_TYPE_7 ? 7 : \
                                   (sz) <= U_STACK_TYPE_8 ? 8 : 9)

struct U_EXPORT UMemoryPool {

#if defined(DEBUG) || defined(U_TEST)
   static sig_atomic_t index_stack_busy; // Segnala operazione in corso su stack (per check rientranza)
#  ifdef DEBUG
   static void printInfo(std::ostream& os);
#  endif
#endif

   static void deallocate(void* ptr, uint32_t length);
   static void      _free(void* ptr, uint32_t num, uint32_t type_size = 1);

   static void allocateMemoryBlocks(int stack_index, uint32_t n);

   static void* _malloc(uint32_t   num, uint32_t type_size = 1, bool bzero = false);
   static void* _malloc(uint32_t* pnum, uint32_t type_size = 1, bool bzero = false);

   // -------------------------------------------------------------------
   // NB: allocazione area di memoria <= U_MAX_SIZE_PREALLOCATE
   //     ritorna area di memoria preallocata su stack predefiniti
   //     (non garantito azzerata a meno di azzerare le deallocazioni...)
   // -------------------------------------------------------------------
   static void* pop(            int stack_index);
   static void  push(void* ptr, int stack_index);
   // -------------------------------------------------------------------
};

template <class T> T* u_new_vector(uint32_t& n)
{
   U_TRACE(0, "u_new_vector<T>(%u)", n)

#if !defined(ENABLE_MEMPOOL) || !defined(__linux__)
   T* _vec = new T[n];
#else
   T* _vec = (T*) UMemoryPool::_malloc(&n, sizeof(T));

   (void) new(_vec) T[n]; // NB: Initializers cannot be specified for arrays...
#endif

   U_RETURN_POINTER(_vec, T);
}

template <class T> void u_delete_vector(T* _vec, uint32_t offset, uint32_t n)
{
   U_TRACE(0, "u_delete_vector<T>(%p,%u,%u)", _vec, offset, n)

#if !defined(ENABLE_MEMPOOL) || !defined(__linux__)
   delete[] _vec;
#else
   for (uint32_t i = 0; i < n; ++i) _vec[i].~T();

   UMemoryPool::_free((char*)_vec - offset, n, sizeof(T));
#endif
}

#if !defined(ENABLE_MEMPOOL) || !defined(__linux__)
#  define U_MEMORY_ALLOCATOR
#  define U_MEMORY_DEALLOCATOR
#  define U_MALLOC(  sz)               malloc(sz);
#  define U_MALLOC_TYPE(  type) (type*)malloc(sizeof(type));
#  define U_FREE(ptr,sz)               free(ptr);
#  define U_FREE_TYPE(ptr,type)        free(ptr);
#else
#  define U_MEMORY_ALLOCATOR \
 void* operator new(  size_t sz)          { U_INTERNAL_ASSERT(sz <= U_MAX_SIZE_PREALLOCATE); return UMemoryPool::pop(U_SIZE_TO_STACK_INDEX(sz)); } \
 void* operator new[](size_t sz)          { U_INTERNAL_ASSERT(sz <= U_MAX_SIZE_PREALLOCATE); return UMemoryPool::pop(U_SIZE_TO_STACK_INDEX(sz)); } \
 void* operator new[](size_t sz, void* p) { return p; }
#  define U_MEMORY_DEALLOCATOR \
 void  operator delete(  void* _ptr, size_t sz) { U_INTERNAL_ASSERT(sz <= U_MAX_SIZE_PREALLOCATE); UMemoryPool::push( _ptr, U_SIZE_TO_STACK_INDEX(sz)); } \
 void  operator delete[](void* _ptr, size_t sz) { U_INTERNAL_ASSERT(sz <= U_MAX_SIZE_PREALLOCATE); UMemoryPool::push( _ptr, U_SIZE_TO_STACK_INDEX(sz)); }
#  define U_MALLOC(  sz)               UMemoryPool::pop(     U_SIZE_TO_STACK_INDEX(sz));          U_INTERNAL_ASSERT(sz          <=U_MAX_SIZE_PREALLOCATE);
#  define U_MALLOC_TYPE(  type) (type*)UMemoryPool::pop(     U_SIZE_TO_STACK_INDEX(sizeof(type)));U_INTERNAL_ASSERT(sizeof(type)<=U_MAX_SIZE_PREALLOCATE);
#  define U_FREE(ptr,sz)              {UMemoryPool::push(ptr,U_SIZE_TO_STACK_INDEX(sz));          U_INTERNAL_ASSERT(sz          <=U_MAX_SIZE_PREALLOCATE);}
#  define U_FREE_TYPE(ptr,type)       {UMemoryPool::push(ptr,U_SIZE_TO_STACK_INDEX(sizeof(type)));U_INTERNAL_ASSERT(sizeof(type)<=U_MAX_SIZE_PREALLOCATE);}
#endif

#endif
