// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    cache.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file.h>
#include <ulib/cache.h>
#include <ulib/container/vector.h>

#define U_NO_TTL (uint32_t)-1

void UCache::init(uint32_t size)
{
   U_TRACE(0, "UCache::init(%u)", size)

   // 100 <= size <= 1000000000

   U_INTERNAL_ASSERT_RANGE(100U, size, 1000U * 1000U * 1000U)

   info->size   =
   info->oldest =
   info->unused = size - sizeof(UCache::cache_info);

   // sizeof(uint32_t) <= hsize <= size/sizeof(cache_hash_table_entry) (hsize is a power of 2)

   info->hsize = sizeof(uint32_t);

   while (info->hsize <= (info->size / sizeof(UCache::cache_hash_table_entry))) info->hsize <<= 1U;

   info->writer = info->hsize;

   // hsize <= writer <= oldest <= unused <= size

   U_INTERNAL_DUMP("hsize = %u writer = %u, oldest = %u, unused = %u size = %u", info->hsize, info->writer, info->oldest, info->unused, info->size)
}

bool UCache::open(const UString& path, uint32_t size, const UString* environment)
{
   U_TRACE(0, "UCache::open(%.*S,%u,%p)", U_STRING_TO_TRACE(path), size, environment)

   U_CHECK_MEMORY

   bool exist = false;
   UFile _x(path, environment);

   if (_x.creat(O_RDWR))
      {
      if (_x.size()) exist = true;
      else           (void) _x.ftruncate(size);

      (void) _x.memmap(PROT_READ | PROT_WRITE);
             _x.close();

      set(_x.getMap());

      if (exist == false)
         {
         reset();

         init(size);
         }

      initStart();
      }

   U_RETURN(exist);
}

UString UCache::addContentOf(const UString& pathname)
{
   U_TRACE(1, "UCache::addContentOf(%.*S)", U_STRING_TO_TRACE(pathname))

   UString content = UFile::contentOf(pathname);

   const char*  key = pathname.data();
   const char* data =  content.data();
   uint32_t keylen  = pathname.size(),
            datalen =  content.size() + 1; // plus null-terminator...

   char* ptr = add(key, keylen, datalen, 0);

   (void) u__memcpy(ptr,           key,  keylen);
   (void) u__memcpy(ptr + keylen, data, datalen);

   U_RETURN_STRING(content);
}

UString UCache::contentOf(const UString& pathname)
{
   U_TRACE(0, "UCache::contentOf(%.*S)", U_STRING_TO_TRACE(pathname))

   UString content = (*this)[pathname];

   if (content.empty()) content = addContentOf(pathname);
   else                 content.size_adjust(content.size() - 1); // minus null-terminator...

   U_RETURN_STRING(content);
}

UString UCache::contentOf(const char* fmt, ...)
{
   U_TRACE(0, "UCache::contentOf(%S)", fmt)

   U_CHECK_MEMORY

   UString pathname(200U);

   va_list argp;
   va_start(argp, fmt);

   pathname.vsnprintf(fmt, argp);

   va_end(argp);

   return contentOf(pathname);
}

void UCache::loadContentOf(const UString& dir, const char* filter, uint32_t filter_len)
{
   U_TRACE(0, "UCache::loadContentOf(%.*S,%.*S,%u)", U_STRING_TO_TRACE(dir), filter_len, filter, filter_len)

   UVector<UString> vec;

   for (uint32_t i = 0, n = UFile::listContentOf(vec, &dir, filter, filter_len); i < n; ++i) (void) addContentOf(vec[i]);
}

inline uint32_t UCache::hash(const char* key, uint32_t keylen)
{
   U_TRACE(0, "UCache::hash(%.*S,%u)", keylen, key, keylen)

   uint32_t keyhash = u_cdb_hash((unsigned char*)key, keylen, false) * sizeof(uint32_t) % info->hsize;

   U_RETURN(keyhash);
}

char* UCache::add(const char* key, uint32_t keylen, uint32_t datalen, uint32_t _ttl)
{
   U_TRACE(0, "UCache::add(%.*S,%u,%u,%u)", keylen, key, keylen, datalen, _ttl)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(_ttl <= U_MAX_TTL)
   U_INTERNAL_ASSERT_RANGE(1U,  keylen, U_MAX_KEYLEN)
   U_INTERNAL_ASSERT_RANGE(1U, datalen, U_MAX_DATALEN)

   cache_hash_table_entry* e;
   uint32_t index, pos, entrylen = sizeof(UCache::cache_hash_table_entry) + keylen + datalen;

   U_INTERNAL_DUMP("writer = %u, entrylen = %u oldest = %u, unused = %u", info->writer, entrylen, info->oldest, info->unused)

   while ((info->writer + entrylen) > info->oldest)
      {
      if (info->oldest == info->unused)
         {
         if (info->writer == info->hsize)
            {
            U_RETURN((char*)0);
            }

         info->unused = info->writer;
         info->oldest = info->writer = info->hsize;
         }

      e = entry(info->oldest);

      U_INTERNAL_DUMP("entry = { %u, %u, %.*S, %u, %.*S, %u }", u_get_unaligned(e->link),
                        u_get_unaligned(e->keylen),
                        u_get_unaligned(e->keylen),                                (char*)(e+1),
                        u_get_unaligned(e->datalen),
                        u_get_unaligned(e->datalen), (u_get_unaligned(e->keylen) + (char*)(e+1)),
                        u_get_unaligned(e->time_expire))

      pos = u_get_unaligned(e->link);

      replace(pos, info->oldest);

      info->oldest += sizeof(UCache::cache_hash_table_entry) + u_get_unaligned(e->keylen) + u_get_unaligned(e->datalen);

      U_INTERNAL_ASSERT(info->oldest <= info->unused)

      if (info->oldest == info->unused) info->unused = info->oldest = info->size;
      }

   index = hash(key, keylen);
   pos   = getLink(index);

   e = setHead(index, info->writer);

   if (pos) replace(pos, index ^ info->writer);

   u_put_unaligned(pos ^ index,                          e->link);
   u_put_unaligned(keylen,                               e->keylen);
   u_put_unaligned(datalen,                              e->datalen);
   u_put_unaligned((_ttl ? getTime() + _ttl : U_NO_TTL), e->time_expire);

   U_INTERNAL_DUMP("entry = { %u, %u, %.*S, %u, %.*S, %u }", u_get_unaligned(e->link),
                        u_get_unaligned(e->keylen),
                        u_get_unaligned(e->keylen),                                (char*)(e+1),
                        u_get_unaligned(e->datalen),
                        u_get_unaligned(e->datalen), (u_get_unaligned(e->keylen) + (char*)(e+1)),
                        u_get_unaligned(e->time_expire))

   char* p = x + info->writer + sizeof(UCache::cache_hash_table_entry);

   info->writer += entrylen;
// cache_motion += entrylen;

   U_RETURN(p);
}

UString UCache::operator[](const UString& _key)
{
   U_TRACE(0, "UCache::operator[](%.*S)", U_STRING_TO_TRACE(_key))

   U_CHECK_MEMORY

   const char* key = _key.data();
   uint32_t keylen = _key.size();

   U_INTERNAL_ASSERT_RANGE(1, keylen, U_MAX_KEYLEN)

   const char* p;
   cache_hash_table_entry* e;

   uint32_t time_expire, loop = 0,
            index = hash(key, keylen),
            pos   = getLink(index);

   uint32_t prevpos = index, nextpos;

   while (pos)
      {
      e = entry(pos);

      time_expire = u_get_unaligned(e->time_expire);

      U_INTERNAL_DUMP("entry = { %u, %u, %.*S, %u, %.*S, %u }", u_get_unaligned(e->link),
                        u_get_unaligned(e->keylen),
                        u_get_unaligned(e->keylen),                                (char*)(e+1),
                        u_get_unaligned(e->datalen),
                        u_get_unaligned(e->datalen), (u_get_unaligned(e->keylen) + (char*)(e+1)),
                        time_expire)

      if (time_expire && // chek if entry is expired...
          u_get_unaligned(e->keylen) == keylen)
         {
         U_INTERNAL_ASSERT((pos + sizeof(UCache::cache_hash_table_entry) + keylen) <= info->size)

         p = x + pos + sizeof(UCache::cache_hash_table_entry);

         if (memcmp(p, key, keylen) == 0)
            {
            if (time_expire != U_NO_TTL &&
                getTime() >= time_expire)
               {
               u_put_unaligned(0, e->time_expire); // set entry expired...

               break;
               }

            U_INTERNAL_ASSERT(u_get_unaligned(e->datalen) <= (info->size - pos - sizeof(UCache::cache_hash_table_entry) - keylen))

            ttl = time_expire;

            UString str(p + keylen, u_get_unaligned(e->datalen));

            U_RETURN_STRING(str);
            }
         }

      if (++loop > 100U) break; /* to protect against hash flooding */

      nextpos = prevpos ^ getLink(pos);
      prevpos = pos;
      pos     = nextpos;
      }

   U_RETURN_STRING(UString::getStringNull());
}

// STREAM

U_EXPORT istream& operator>>(istream& is, UCache& cache)
{
   U_TRACE(0+256, "UCache::operator>>(%p,%p)", &is, &cache)

   char c;
   char* ptr;
   char key[U_MAX_KEYLEN];
   uint32_t keylen, datalen;

   while (is >> c)
      {
      U_INTERNAL_DUMP("c = %C", c)

      if (c == '#')
         {
         for (int ch = is.get(); (ch != '\n' && ch != EOF); ch = is.get()) {}

         continue;
         }

      if (c != '+') break;

      is >> keylen;

      is.get(); // skip ','
      is >> datalen;
      is.get(); // skip ':'

      is.read(key, keylen);

      U_INTERNAL_DUMP("key = %.*S keylen = %u", keylen, key, keylen)

      U_INTERNAL_ASSERT_MINOR(keylen, U_MAX_KEYLEN)

      ptr = cache.add(key, keylen, datalen, 0);

      (void) u__memcpy(ptr, key, keylen);

      is.get(); // skip '-'
      is.get(); // skip '>'

      ptr += keylen;

      is.read(ptr, datalen);

      U_INTERNAL_DUMP("data = %.*S datalen = %u", datalen, ptr, datalen)

      is.get(); // skip '\n'
      }

   return is;
}

void UCache::print(ostream& os, uint32_t& pos) const
{
   U_INTERNAL_TRACE("UCache::print(%p,%u)", &os, pos)

   U_CHECK_MEMORY

   UCache::cache_hash_table_entry* e = (UCache::cache_hash_table_entry*)(x + pos);

   os.put('+');
   os << u_get_unaligned(e->keylen);
   os.put(',');
   os << u_get_unaligned(e->datalen);
   os.put(':');

   pos += sizeof(UCache::cache_hash_table_entry);
   os.write(x + pos, u_get_unaligned(e->keylen));

   os.put('-');
   os.put('>');

   pos += e->keylen;
   os.write(x + pos, u_get_unaligned(e->datalen));
   pos += e->datalen;

   os.put('\n');
}

U_EXPORT ostream& operator<<(ostream& os, const UCache& c)
{
   U_TRACE(0, "UCache::operator<<(%p,%p)", &os, &c)

   uint32_t pos = c.info->oldest;

   while (pos < c.info->unused) c.print(os, pos);

   pos = c.info->hsize;

   while (pos < c.info->writer) c.print(os, pos);

   os.put('\n');

   return os;
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UCache::dump(bool _reset) const
{
   *UObjectIO::os << "x      " << (void*)x       << '\n'
                  << "ttl    " << ttl            << '\n'
                  << "start  " << (void*)start   << '\n'
                  << "size   " << info->size     << '\n'
                  << "hsize  " << info->hsize    << '\n'
                  << "writer " << info->writer   << '\n'
                  << "oldest " << info->oldest   << '\n'
                  << "unused " << info->unused;

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
