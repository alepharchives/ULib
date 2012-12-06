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

#include <ulib/cache.h>
#include <ulib/utility/string_ext.h>

#define U_NO_TTL (uint32_t)-1

UCache::~UCache()
{
   U_TRACE_UNREGISTER_OBJECT(0, UCache)

   if (fd != -1)
      {
      UFile::close(fd);

      UFile::munmap(info, sizeof(UCache::cache_info) + info->size);
      }
}

U_NO_EXPORT void UCache::init(UFile& _x, uint32_t size, bool bexist)
{
   U_TRACE(0, "UCache::init(%.*S,%u,%b)", U_FILE_TO_TRACE(_x), size, bexist)

   U_CHECK_MEMORY

   fd = _x.getFd();

   (void) _x.memmap(PROT_READ | PROT_WRITE);

   char* ptr = _x.getMap();

   info = (cache_info*)ptr;
      x =              ptr + sizeof(UCache::cache_info);

   if (bexist) start = (_x.fstat(), _x.st_mtime);
   else
      {
      // 100 <= size <= 1000000000

      U_INTERNAL_ASSERT_RANGE(100U, size, 1000U * 1000U * 1000U)

      // hsize <= writer <= oldest <= unused <= size

      info->hsize = info->writer = getHSize((info->oldest = info->unused = info->size = (size - sizeof(UCache::cache_info))));

      U_INTERNAL_DUMP("hsize = %u writer = %u oldest = %u unused = %u size = %u", info->hsize, info->writer, info->oldest, info->unused, info->size)

      U_gettimeofday; // NB: optimization if it is enough a time resolution of one second...

      start = u_now->tv_sec;
      }
}

bool UCache::open(const UString& path, uint32_t size, const UString* environment)
{
   U_TRACE(0, "UCache::open(%.*S,%u,%p)", U_STRING_TO_TRACE(path), size, environment)

   U_CHECK_MEMORY

   UFile _x(path, environment);

   if (_x.creat(O_RDWR))
      {
      init(_x, size, (_x.size() ? true : ((void)_x.ftruncate(size), false)));

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool UCache::open(const UString& path, const UString& dir_template, const UString* environment)
{
   U_TRACE(0, "UCache::open(%.*S,%.*S,%p)", U_STRING_TO_TRACE(path), U_STRING_TO_TRACE(dir_template), environment)

   U_CHECK_MEMORY

   UFile _x(        path, environment),
         _y(dir_template, environment);

   if (_y.stat() &&
       _x.creat(O_RDWR))
      {
      bool exist = true;
      UVector<UString> vec1, vec2;
      uint32_t i, n, size = 0, hsize;

      if (( _x.size() == 0                          ||
           (_x.fstat(), _x.st_mtime < _y.st_mtime)) &&
          (n = _y.listContentOf(vec1, 0, 0)))
         {
         exist = false;

         UString content, name;
         char buffer[U_PATH_MAX];
         char* ptr = buffer + u__snprintf(buffer, sizeof(buffer), "%s/", _y.getPathRelativ());
         uint32_t buffer_size = ptr - buffer;

         for (i = 0; i < n; ++i)
            {
            name = vec1[i];

            (void) u__snprintf(ptr, buffer_size, "%.*s", U_STRING_TO_TRACE(name));

            _y.setPath(buffer);

            content = _y.getContent();

            if (content.empty() == false)
               {
               vec2.push_back(content);

               size += sizeof(UCache::cache_hash_table_entry) + name.size() + _y.getSize() + 1; // NB: + null-terminator...
               }
            }

          size += sizeof(UCache::cache_info);
         hsize  = getHSize(size);
         hsize  = getHSize(size + hsize);
          size +=                 hsize;

         (void) _x.ftruncate(size);
         }

      init(_x, size, exist);

      if (exist == false)
         {
         U_INTERNAL_ASSERT_EQUALS(info->hsize, hsize)
         U_INTERNAL_ASSERT_EQUALS(info->size, size - sizeof(UCache::cache_info))

         for (i = 0, n = vec2.size(); i < n; ++i) addContent(vec1[i], vec2[i]);

         U_INTERNAL_DUMP("hsize = %u writer = %u oldest = %u unused = %u size = %u", info->hsize, info->writer, info->oldest, info->unused, info->size)

         U_INTERNAL_ASSERT_EQUALS(info->writer, info->size)

         (void) U_SYSCALL(msync, "%p,%u,%d", (char*)info, sizeof(UCache::cache_info) + info->size, MS_SYNC); // flushes changes made to memory mapped file back to disk
         }

      U_RETURN(true);
      }

   U_RETURN(false);
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

   U_INTERNAL_DUMP("writer = %u entrylen = %u oldest = %u unused = %u", info->writer, entrylen, info->oldest, info->unused)

   while ((info->writer + entrylen) > info->oldest)
      {
      if (info->oldest == info->unused)
         {
         if (info->writer <= info->hsize)
            {
            U_ERROR("cache exhausted... exit");

            U_RETURN((char*)0);
            }

         info->unused = info->writer;
         info->oldest = info->writer = info->hsize;
         }

      e = entry(info->oldest);

      U_INTERNAL_DUMP("entry = { %u, %u, %.*S, %u, %.*S, %#3D }", u_get_unaligned(e->link),
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
     pos = getLink(index);

   e = setHead(index, info->writer);

   if (pos) replace(pos, index ^ info->writer);

   time_t expire = (_ttl ? getTime() + _ttl : U_NO_TTL);

   u_put_unaligned(pos ^ index, e->link);
   u_put_unaligned(keylen,      e->keylen);
   u_put_unaligned(datalen,     e->datalen);
   u_put_unaligned(expire,      e->time_expire);

   U_INTERNAL_DUMP("entry = { %u, %u, %.*S, %u, %.*S, %#3D }", u_get_unaligned(e->link),
                        u_get_unaligned(e->keylen),
                        u_get_unaligned(e->keylen),                                (char*)(e+1),
                        u_get_unaligned(e->datalen),
                        u_get_unaligned(e->datalen), (u_get_unaligned(e->keylen) + (char*)(e+1)),
                        u_get_unaligned(e->time_expire))

   char* p = x + info->writer + sizeof(UCache::cache_hash_table_entry);

   info->writer += entrylen;

   U_INTERNAL_DUMP("writer = %u entrylen = %u oldest = %u unused = %u", info->writer, entrylen, info->oldest, info->unused)

   U_RETURN(p);
}

void UCache::add(const UString& _key, const UString& _data, uint32_t _ttl)
{
   U_TRACE(0, "UCache::add(%.*S,%.*S,%u)", U_STRING_TO_TRACE(_key), U_STRING_TO_TRACE(_data), _ttl)

   const char*  key =  _key.data();
   const char* data = _data.data();
   uint32_t keylen  =  _key.size(),
            datalen = _data.size();

   char* ptr = add(key, keylen, datalen, _ttl);

   U__MEMCPY(ptr,           key,  keylen);
   U__MEMCPY(ptr + keylen, data, datalen);
}

void UCache::addContent(const UString& _key, const UString& content, uint32_t _ttl)
{
   U_TRACE(0, "UCache::addContent(%.*S,%.*S,%u)", U_STRING_TO_TRACE(_key), U_STRING_TO_TRACE(content), _ttl)

   U_INTERNAL_ASSERT(_key)
   U_INTERNAL_ASSERT(content)

   const char*  key =    _key.data();
   const char* data = content.data();
   uint32_t keylen  =    _key.size(),
            datalen = content.size() + 1; // NB: + null-terminator...

   char* ptr = add(key, keylen, datalen, _ttl);

   U__MEMCPY(ptr,           key,  keylen);
   U__MEMCPY(ptr + keylen, data, datalen);
}

UString UCache::get(const char* key, uint32_t keylen)
{
   U_TRACE(0, "UCache::get(%.*S,%u)", keylen, key, keylen)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_RANGE(1, keylen, U_MAX_KEYLEN)

   const char* p;
   cache_hash_table_entry* e;

   uint32_t time_expire, loop = 0,
            index = hash(key, keylen),
              pos = getLink(index);

   uint32_t prevpos = index, nextpos;

   while (pos)
      {
      e = entry(pos);

      time_expire = u_get_unaligned(e->time_expire);

      U_INTERNAL_DUMP("entry = { %u, %u, %.*S, %u, %.*S, %#3D }", u_get_unaligned(e->link),
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

UString UCache::getContent(const char* key, uint32_t keylen)
{
   U_TRACE(0, "UCache::getContent(%.*S,%u)", keylen, key, keylen)

   UString content = get(key, keylen);

   U_INTERNAL_ASSERT(content)

   content.size_adjust(content.size() - 1); // NB: minus null-terminator...

   U_RETURN_STRING(content);
}

void UCache::loadContentOf(const UString& dir, const char* filter, uint32_t filter_len)
{
   U_TRACE(1, "UCache::loadContentOf(%.*S,%.*S,%u)", U_STRING_TO_TRACE(dir), filter_len, filter, filter_len)

   UVector<UString> vec;
   UString item, content;

   for (uint32_t i = 0, n = UFile::listContentOf(vec, &dir, filter, filter_len); i < n; ++i)
      {
      item    = vec[i];
      content = UFile::contentOf(item);

      if (content.empty() == false) addContent(UStringExt::basename(item), content);
      }

   U_INTERNAL_DUMP("hsize = %u writer = %u oldest = %u unused = %u size = %u", info->hsize, info->writer, info->oldest, info->unused, info->size)

   if (info->writer < info->size &&
       U_SYSCALL(ftruncate, "%d,%u", fd, info->writer + sizeof(UCache::cache_info)) == 0)
      {
      info->oldest = info->unused = info->size = info->writer;
      }
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

      U__MEMCPY(ptr, key, keylen);

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
                  << "fd     " << fd             << '\n'
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
