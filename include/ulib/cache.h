// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    cache.h - A structure for fixed-size cache (Bernstein)
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_CACHE_H
#define ULIB_CACHE_H 1

#include <ulib/string.h>

/**
   @class UCache

   @brief UCache is a structure for fixed-size cache.

   The first part of cache is a hash tables with (hsize / sizeof(uint32_t)) pointers to consecutive bucket linked lists.
   +--------------------+--------------------------+------------+
   | p0 p1 ... phsize-1 | entry0 entry1 ... entryn | free space |
   +--------------------+--------------------------+------------+
   The internal data structure of cache is the following structure:

   x[    0....hsize-1]  hsize / sizeof(uint32_t) head links.
   x[hsize....writer-1] consecutive entries, newest entry on the right.
   x[writer...oldest-1] free space for new entries.
   x[oldest...unused-1] consecutive entries, oldest entry on the left.
   x[unused...size-1]   unused.

   Each hash bucket is a linked list containing the following items:
   the head link, the newest entry, the second-newest entry, etc.
   Each link is a 4-byte number giving the xor of the positions of the adjacent items in the list.
   Entries are always inserted immediately after the head and removed at the tail.
   Each entry contains the following information: struct cache_hash_table_entry + key + data.
*/

#define U_MAX_TTL         365L * U_ONE_DAY_IN_SECOND // 365 gg (1 anno)
#define U_MAX_KEYLEN     1000U
#define U_MAX_DATALEN 1000000U

class U_EXPORT UCache {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UCache()
      {
      U_TRACE_REGISTER_OBJECT(0, UCache, "")
      }

   ~UCache()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UCache)
      }

   // INIT

   void set(char* ptr)
      {
      U_TRACE(0, "UCache::set(%p)", ptr)

      x    = ptr + sizeof(UCache::cache_info);
      info = (cache_info*)ptr;
      }

   void initStart()
      {
      U_TRACE(0, "UCache::initStart()")

      if (u_pthread_time == 0) u_gettimeofday();

      start = u_now->tv_sec;
      }

   void init(uint32_t size);

   // OPEN/CREAT a cache file

   bool open(const UString& path, uint32_t size);

   // OPERATION

   void add(const UString& _key, const UString& _data, uint32_t _ttl = 0)
      {
      U_TRACE(1, "UCache::add(%.*S,%.*S,%u)", U_STRING_TO_TRACE(_key), U_STRING_TO_TRACE(_data), _ttl)

      const char*  key =  _key.data();
      const char* data = _data.data();
      uint32_t keylen  =  _key.size(),
               datalen = _data.size();

      char* ptr = add(key, keylen, datalen, _ttl);

      (void) u_memcpy(ptr,           key,  keylen);
      (void) u_memcpy(ptr + keylen, data, datalen);
      }

   // operator []

   UString operator[](const UString& key);

   // SERVICES

   UString     contentOf(const char* fmt, ...);
   UString  addContentOf(const UString& pathname);
   void    loadContentOf(const UString& directory);

   void reset() { (void) memset(x, 0, info->size); }

   uint32_t getTTL() const
      {
      U_TRACE(0, "UCache::getTTL()")

      U_RETURN(ttl);
      }

   uint32_t getTime() const
      {
      U_TRACE(0, "UCache::getTime()")

      if (u_pthread_time == 0) u_gettimeofday();

      uint32_t now = (uint32_t)(u_now->tv_sec - start);

      U_RETURN(now);
      }

   // STREAM

   friend U_EXPORT istream& operator>>(istream& is,       UCache& c);
   friend U_EXPORT ostream& operator<<(ostream& os, const UCache& c);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   typedef struct cache_info {
      uint32_t size;    // size of cache
      uint32_t hsize;   // size of hash table
      uint32_t writer;  // pointer to free space
      uint32_t oldest;  // pointer to oldest entries
      uint32_t unused;  // pointer to unused space
      } cache_info;

   typedef struct cache_hash_table_entry {
      uint32_t link;
      uint32_t keylen;
      uint32_t datalen;
      uint32_t time_expire;
   // ------> keylen  array of char...
   // ------> datalen array of char...
   } cache_hash_table_entry;

   char* x;          // cache pointer
   cache_info* info; // cache info pointer
   time_t start;     // time of reference
   uint32_t ttl;     // time to live (scadenza entry)

   uint32_t getLink(uint32_t pos) const
      {
      U_TRACE(0, "UCache::getLink(%u)", pos)

      U_INTERNAL_ASSERT(pos <= (info->size - sizeof(uint32_t)))

      uint32_t value = u_get_unalignedp(x + pos);

      U_INTERNAL_DUMP("value = %u", value)

      U_INTERNAL_ASSERT(value <= (info->size - sizeof(uint32_t)))

      U_RETURN(value);
      }

    cache_hash_table_entry* setHead(uint32_t pos, uint32_t value)
      {
      U_TRACE(0, "UCache::setHead(%u,%u)", pos, value)

      U_INTERNAL_ASSERT_MINOR(pos,info->hsize)
      U_INTERNAL_ASSERT(value <= (info->size - sizeof(uint32_t)))

      char* ptr = x + pos;

      U_INTERNAL_DUMP("ptr = %p *ptr = %u", ptr, u_get_unalignedp(ptr))

      u_put_unalignedp(value, ptr);

      ptr = x + value;

      U_RETURN_POINTER(ptr, cache_hash_table_entry);
      }

   cache_hash_table_entry* entry(uint32_t pos) const
      {
      U_TRACE(0, "UCache::entry(%u)", pos)

      U_INTERNAL_ASSERT(pos <= (info->size - sizeof(uint32_t)))

      cache_hash_table_entry* e = (cache_hash_table_entry*)(x + pos);

      U_RETURN_POINTER(e, cache_hash_table_entry);
      }

   void replace(uint32_t pos, uint32_t value)
      {
      U_TRACE(0, "UCache::replace(%u,%u)", pos, value)

      U_INTERNAL_ASSERT(pos   <= (info->size - sizeof(uint32_t)))
      U_INTERNAL_ASSERT(value <= (info->size - sizeof(uint32_t)))

      char* ptr = x + pos;

      U_INTERNAL_DUMP("*ptr = %u", u_get_unalignedp(ptr))

      value ^= u_get_unalignedp(ptr);

      u_put_unalignedp(value, ptr);

      U_INTERNAL_DUMP("*ptr = %u", value)
      }

   char* add(const char* key, uint32_t keylen, uint32_t datalen, uint32_t ttl);

   void print(ostream& os, uint32_t& pos) const;

private:
   inline uint32_t hash(const char* key, uint32_t keylen) U_NO_EXPORT;

   UCache(const UCache&)            {}
   UCache& operator=(const UCache&) { return *this; }
};

#endif
