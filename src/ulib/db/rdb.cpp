// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    rdb.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/db/rdb.h>
#include <ulib/container/vector.h>

URDB*             URDB::ptr_rdb;
UCDB::datum       URDB::key1;
UVector<UString>* URDB::kvec;

// Search one key/data pair in the cache.
// To save code, I use only a single function to do the lookup in the hash table and binary search tree

U_NO_EXPORT bool URDB::htLookup()
{
   U_TRACE(0, "URDB::htLookup()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(UCDB::key.dptr)
   U_INTERNAL_ASSERT_MAJOR(UCDB::key.dsize,0)

   // Because the insertion routine has to know where to insert the cache_node, this code has to assign
   // a pointer to the empty pointer to manipulate in that case, so we have to do a nasty indirection...

   pnode = RDB_hashtab + (UCDB::khash % CACHE_HASHTAB_LEN);

   U_INTERNAL_DUMP("pnode = %p slot = %u", pnode, UCDB::khash % CACHE_HASHTAB_LEN)

   int result;
   uint32_t len;

   while ((node = u_get_unalignedp(pnode)))
      {
      U_INTERNAL_DUMP("pnode = %p node = %u", pnode, node)

      len = RDB_node_key_sz;

      if (len == 0)
         {
         U_WARNING("null key size at offset %u, pnode %p", node, pnode);

         node = 0;

         u_put_unalignedp(0, pnode);

         break;
         }

      result = u_equal(UCDB::key.dptr, RDB_node_key, U_min(UCDB::key.dsize, len), UCDB::ignore_case);

      // RDB_node => ((URDB::cache_node*)(journal.map+node))

      U_INTERNAL_DUMP("result = %d len = %d", result, len)

      if (result < 0) pnode = &(RDB_node->left);
      else
         {
         if (result == 0 &&
             len    == UCDB::key.dsize)
            {
            U_RETURN(true);
            }

         pnode = &(RDB_node->right);
         }
      }

   // NB: i riferimenti in memoria alla cache dati devono puntare sulla memoria mappata...

   U_INTERNAL_DUMP("pnode = %p node = %u", pnode, node)

   U_INTERNAL_ASSERT_RANGE(RDB_hashtab, pnode, (uint32_t*)RDB_allocate)

   U_RETURN(false);
}

// Alloc one node for the hash tree

U_NO_EXPORT void URDB::htAlloc()
{
   U_TRACE(0, "URDB::htAlloc()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_EQUALS(node,0)
   U_INTERNAL_ASSERT_POINTER(UCDB::key.dptr)
   U_INTERNAL_ASSERT_MAJOR(UCDB::key.dsize,0)

   U_INTERNAL_DUMP("RDB_capacity = %u", RDB_capacity)

   U_INTERNAL_ASSERT_MAJOR((uint32_t)RDB_capacity, sizeof(URDB::cache_node))

   node = RDB_off;
          RDB_off += sizeof(URDB::cache_node);

   u_put_unalignedp(node, pnode);
   u_put_unaligned(    0, RDB_node->key.dptr);
   u_put_unaligned(    0, RDB_node->key.dsize);
   u_put_unaligned(    0, RDB_node->data.dptr);
   u_put_unaligned(    0, RDB_node->data.dsize);
   u_put_unaligned(    0, RDB_node->left);
   u_put_unaligned(    0, RDB_node->right);
}

// remove one node allocated for the hash tree

U_NO_EXPORT void URDB::htRemoveAlloc()
{
   U_TRACE(0, "URDB::htRemoveAlloc()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(UCDB::key.dptr)
   U_INTERNAL_ASSERT_MAJOR(UCDB::key.dsize,0)

   U_INTERNAL_DUMP("RDB_capacity = %u", RDB_capacity)

   U_INTERNAL_ASSERT_MAJOR((uint32_t)RDB_capacity, sizeof(URDB::cache_node))

   RDB_off -= sizeof(URDB::cache_node);

   U_INTERNAL_ASSERT_EQUALS(RDB_off, u_get_unalignedp(pnode))

   u_put_unalignedp(0, pnode);
}

U_NO_EXPORT inline bool URDB::resizeJournal(uint32_t oversize)
{
   U_TRACE(0, "URDB::resizeJournal(%u)", oversize)

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("RDB_reference = %u", RDB_reference)

   U_INTERNAL_ASSERT_EQUALS(RDB_reference, 1)

   uint32_t _size = (journal.st_size / 2);

   if (oversize < _size) oversize = _size;

   U_INTERNAL_DUMP("oversize = %u", oversize)

   uint32_t _offset = (char*)pnode - journal.map;

   U_INTERNAL_DUMP("pnode = %p node = %u offset = %u", pnode, node, _offset)

#if defined(__CYGWIN__) || defined(__MINGW32__)
   journal.munmap(); // for ftruncate()...
#endif

   if (journal.ftruncate(journal.st_size + oversize))
      {
#  if defined(__CYGWIN__) || defined(__MINGW32__)
      (void) journal.memmap(PROT_READ | PROT_WRITE);
#  endif

      pnode = (uint32_t*)(journal.map + _offset);

      U_INTERNAL_DUMP("pnode = %p node = %u offset = %u", pnode, node, _offset)

      U_RETURN(true);
      }

   // save entry

   key1              = UCDB::key;
   UCDB::datum data1 = UCDB::data;
   uint32_t save     = UCDB::khash;

   if (reorganize())
      {
      // set old entry

      UCDB::key   = key1;
      UCDB::data  = data1;
      UCDB::khash = save;

      (void) htLookup();

      U_RETURN(true);
      }

   U_RETURN(false);
}

U_NO_EXPORT bool URDB::writev(const struct iovec* _iov, int n, uint32_t _size)
{
   U_TRACE(0, "URDB::writev(%p,%d,%u)", _iov, n, _size)

   U_INTERNAL_ASSERT_MAJOR(_size,0)

   U_INTERNAL_DUMP("RDB_off = %u", RDB_off)

   uint32_t sz = RDB_off + _size + (sizeof(URDB::cache_node) * 2);

   if (sz > journal.st_size &&
       resizeJournal(sz - journal.st_size) == false)
      {
      U_RETURN(false);
      }

   char* journal_ptr = journal.map + RDB_off;

   for (int i = 0; i < n; ++i)
      {
      U__MEMCPY(journal_ptr, _iov[i].iov_base, _iov[i].iov_len);

      // NB: Una volta scritti i dati sul journal si cambiano i riferimenti in memoria ai dati
      // e alle chiavi in modo che puntino appunto sul journal mappato in memoria...

      if (n < 5) // remove(), store()
         {
         if      (i == 1) UCDB::key.dptr  = journal_ptr; // remove(), store()
         else if (i == 2) UCDB::data.dptr = journal_ptr; // store()
         }
      else // substitute()
         {
         if      (i == 1)      key1.dptr  = journal_ptr;
         if      (i == 3) UCDB::key.dptr  = journal_ptr;
         else if (i == 4) UCDB::data.dptr = journal_ptr;
         }

      journal_ptr += _iov[i].iov_len;
      }

   RDB_off = (journal_ptr - journal.map);

   U_INTERNAL_DUMP("RDB_off = %u", RDB_off)

   U_RETURN(true);
}

bool URDB::logJournal(int op)
{
   U_TRACE(0, "URDB::logJournal(%d)", op)

   // Records are stored without special alignment. A record
   // states a key length, a data length, the key, and the data

   if (op == 0) // remove
      {
      UCDB::cdb_record_header hrec = { UCDB::key.dsize, U_NOT_FOUND };

      U_INTERNAL_DUMP("hrec = { %u, %u }", hrec.klen, hrec.dlen)

      struct iovec _iov[2] = { { (caddr_t)&hrec, sizeof(UCDB::cdb_record_header) },
                               { (caddr_t)UCDB::key.dptr, UCDB::key.dsize } };

      if (writev(_iov, 2, sizeof(UCDB::cdb_record_header) + UCDB::key.dsize) == false) U_RETURN(false);
      }
   else if (op == 1) // store
      {
      UCDB::cdb_record_header hrec = { UCDB::key.dsize, UCDB::data.dsize };

      U_INTERNAL_DUMP("hrec = { %u, %u }", hrec.klen, hrec.dlen)

      struct iovec _iov[3] = { { (caddr_t)&hrec, sizeof(UCDB::cdb_record_header) },
                               { (caddr_t)UCDB::key.dptr,  UCDB::key.dsize },
                               { (caddr_t)UCDB::data.dptr, UCDB::data.dsize } };

      if (writev(_iov, 3, sizeof(UCDB::cdb_record_header) + UCDB::key.dsize + UCDB::data.dsize) == false) U_RETURN(false);
      }
   else // substitute
      {
      U_INTERNAL_ASSERT_EQUALS(op,2)
      U_INTERNAL_ASSERT_POINTER(UCDB::key.dptr)
      U_INTERNAL_ASSERT_POINTER(key1.dptr)
      U_INTERNAL_ASSERT_POINTER(UCDB::data.dptr)
      U_INTERNAL_ASSERT_MAJOR(UCDB::key.dsize,0)
      U_INTERNAL_ASSERT_MAJOR(key1.dsize,0)
      U_INTERNAL_ASSERT_MAJOR(UCDB::data.dsize,0)

      UCDB::cdb_record_header hrec1 = {      key1.dsize, U_NOT_FOUND },
                              hrec  = { UCDB::key.dsize, UCDB::data.dsize };

      U_INTERNAL_DUMP("hrec1 = { %u, %u } hrec = { %u, %u }", hrec1.klen, hrec1.dlen, hrec.klen, hrec.dlen)

      struct iovec _iov[5] = { { (caddr_t)&hrec1, sizeof(UCDB::cdb_record_header) },
                               { (caddr_t)key1.dptr, key1.dsize },
                               { (caddr_t)&hrec, sizeof(UCDB::cdb_record_header) },
                               { (caddr_t)UCDB::key.dptr,  UCDB::key.dsize },
                               { (caddr_t)UCDB::data.dptr, UCDB::data.dsize } };

      if (writev(_iov, 5, (sizeof(UCDB::cdb_record_header) * 2) + key1.dsize + UCDB::key.dsize + UCDB::data.dsize) == false) U_RETURN(false);
      }

   U_RETURN(true);
}

// Insert one key/data pair in the cache

U_NO_EXPORT void URDB::htInsert()
{
   U_TRACE(0, "URDB::htInsert()")

   U_CHECK_MEMORY

   // NB: i riferimenti in memoria ai dati e alle chiavi devono puntare sul journal mappato in memoria...

   U_INTERNAL_DUMP("key  = { %p, %u }", UCDB::key.dptr,  UCDB::key.dsize)
   U_INTERNAL_DUMP("data = { %p, %u }", UCDB::data.dptr, UCDB::data.dsize)

#ifdef DEBUG
                          U_INTERNAL_ASSERT_RANGE(RDB_ptr, UCDB::key.dptr,  RDB_allocate)
   if (UCDB::data.dptr) { U_INTERNAL_ASSERT_RANGE(RDB_ptr, UCDB::data.dptr, RDB_allocate) }
#endif

   // NB: i riferimenti in memoria alla cache dati devono puntare sulla memoria mappata...

   U_INTERNAL_DUMP("pnode = %p node = %u", pnode, node)

   U_INTERNAL_ASSERT_RANGE(sizeof(URDB::cache_struct), node, journal.st_size - sizeof(URDB::cache_node))
   U_INTERNAL_ASSERT_RANGE(RDB_hashtab,               pnode, (uint32_t*)RDB_allocate)

   uint32_t offset1 =                     (char*)UCDB::key.dptr  - journal.map,
            offset2 = (UCDB::data.dptr ? ((char*)UCDB::data.dptr - journal.map) : 0);

   u_put_unaligned(offset1,          RDB_node->key.dptr);
   u_put_unaligned(UCDB::key.dsize,  RDB_node->key.dsize);
   u_put_unaligned(offset2,          RDB_node->data.dptr);
   u_put_unaligned(UCDB::data.dsize, RDB_node->data.dsize);
}

// open a Reliable DataBase

bool URDB::open(uint32_t log_size, bool btruncate, bool brdonly)
{
   U_TRACE(0, "URDB::open(%u,%b)", log_size, btruncate, brdonly)

   (void) UCDB::open(brdonly);

   journal.setPath(*(const UFile*)this, 0, U_CONSTANT_TO_PARAM(".jnl"));

   int            flags  = O_RDWR;
   if (btruncate) flags |= O_TRUNC;

   if (journal.creat(flags))
      {
      uint32_t journal_size   = journal.size(),
               journal_new_sz = (journal_size   ? journal_size   :
                                 log_size       ? log_size       :
                                 UFile::st_size ? UFile::st_size : 512 * 1024);

      if (journal_new_sz == journal_size ||
          journal.ftruncate(journal_new_sz))
         {
#     if !defined(__CYGWIN__) && !defined(__MINGW32__)
         if (journal_new_sz < 32 * 1024 * 1024) journal_new_sz = 32 * 1024 * 1024; // oversize mmap for optimize resizeJournal() with ftruncate()
#     endif

         if (journal.memmap(PROT_READ | PROT_WRITE, 0, 0, journal_new_sz))
            {
            if (RDB_off == 0) RDB_off = sizeof(URDB::cache_struct);

            U_INTERNAL_DUMP("RDB_off = %u RDB_sync = %u capacity = %u nrecord = %u RDB_reference = %u",
                             RDB_off,     RDB_sync, RDB_capacity, RDB_nrecord,     RDB_reference)

            RDB_reference++;

            U_RETURN(true);
            }
         }
      }

   U_RETURN(false);
}

void URDB::close()
{
   U_TRACE(0, "URDB::close()")

   U_CHECK_MEMORY

   if (UFile::map_size) UFile::munmap(); // Constant DB

   lock();

   RDB_reference--;

   uint32_t reference = RDB_reference;

   U_INTERNAL_DUMP("RDB_reference = %u", RDB_reference)

   uint32_t sz = RDB_sync = RDB_off;

   journal.munmap();

   if (reference == 0) (void) journal.ftruncate(sz);

   journal.close();
   journal.reset();

   unlock();
}

void URDB::reset()
{
   U_TRACE(1, "URDB::reset()")

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("RDB_reference = %u", RDB_reference)

   U_INTERNAL_ASSERT_EQUALS(RDB_reference, 1)
   U_INTERNAL_ASSERT_DIFFERS(journal.map, MAP_FAILED)

   RDB_off     = sizeof(URDB::cache_struct);
   RDB_sync    = 0;
   RDB_nrecord = 0;

   // Initialize the cache to contain no entries

   (void) U_SYSCALL(memset, "%p,%d,%d", RDB_hashtab, 0, sizeof(RDB_hashtab));
}

bool URDB::beginTransaction()
{
   U_TRACE(0, "URDB::beginTransaction()")

   lock();

   return reorganize();
}

void URDB::commitTransaction()
{
   U_TRACE(0, "URDB::commitTransaction()")

   msync();
   fsync();

   unlock();
}

void URDB::abortTransaction()
{
   U_TRACE(0, "URDB::abortTransaction()")

   reset();

   unlock();
}

void URDB::msync()
{
   U_TRACE(0, "URDB::msync()")

   U_CHECK_MEMORY

   lock();

   U_INTERNAL_DUMP("RDB_off = %u RDB_sync = %u", RDB_off, RDB_sync)

   UFile::msync(journal.map + RDB_off, journal.map + RDB_sync);

   RDB_sync = RDB_off;

   U_INTERNAL_DUMP("RDB_off = %u RDB_sync = %u", RDB_off, RDB_sync)

   unlock();
}

// Close a Reliable DataBase

bool URDB::closeReorganize()
{
   U_TRACE(0, "URDB::closeReorganize()")

   if (reorganize())
      {
      URDB::close();

      (void) journal._unlink();

      U_RETURN(true);
      }

   U_RETURN(false);
}

// Combines the old cdb file and the diffs in a new cdb file

U_NO_EXPORT bool URDB::reorganize()
{
   U_TRACE(0, "URDB::reorganize()")

   U_CHECK_MEMORY

   bool result = true;

   lock();

   U_INTERNAL_DUMP("RDB_off = %u", RDB_off)

   if (RDB_off > sizeof(URDB::cache_struct))
      {
      U_INTERNAL_DUMP("RDB_reference = %u", RDB_reference)

      U_INTERNAL_ASSERT_EQUALS(RDB_reference, 1)

      UCDB cdb(UCDB::ignore_case);
      char cdb_buffer_path[MAX_FILENAME_LEN];

      cdb.setPath(*(const UFile*)this, cdb_buffer_path, U_CONSTANT_TO_PARAM(".tmp"));

      result = cdb.creat(O_RDWR) &&
               cdb.ftruncate(UFile::st_size + journal.st_size + UCDB::sizeFor(4096));

      if (result)
         {
         if (cdb.memmap(PROT_READ | PROT_WRITE) == false) U_RETURN(false);

         // prima si leggono le entry nella cache
         // poi si scansiona il constant database e si cercano le entry NON presenti nella cache...

         cdb.makeStart();

         ptr_rdb = this;
         UCDB::setCDB(&cdb);

         makeAdd();

         uint32_t pos = cdb.makeFinish(false);

         U_INTERNAL_ASSERT(pos <= cdb.st_size)

#     if defined(__CYGWIN__) || defined(__MINGW32__)
         cdb.munmap(); // for ftruncate()...
#     endif

         if (cdb.ftruncate(pos) == false) U_RETURN(false);

#     if defined(__MINGW32__) || defined(__CYGWIN__)
         UFile::munmap(); // for rename()...
#        ifdef   __MINGW32__
         cdb.UFile::close();
#        endif
#     endif

         if (cdb._rename(UFile::path_relativ) == false) U_RETURN(false);

         URDB::reset();

#     if defined(__MINGW32__) || defined(__CYGWIN__)
#        ifdef   __MINGW32__
         result = cdb.UFile::open(UFile::path_relativ);
                  cdb.st_size = pos;
#        endif
         result = cdb.memmap(); // read only...
#     endif

         cdb.UFile::close();

         UFile::substitute(cdb);

         UCDB::nrecord               = cdb.nrecord;
         UCDB::start_hash_table_slot = cdb.start_hash_table_slot;

         U_INTERNAL_DUMP("UCDB::nrecord = %u RDB_nrecord = %u", UCDB::nrecord, RDB_nrecord)
         }
      }

   unlock();

   U_RETURN(result);
}

char* URDB::parseLine(const char* ptr, UCDB::datum* _key, UCDB::datum* _data)
{
   U_TRACE(0, "URDB::parseLine(%p,%p,%p)", ptr, _key, _data)

   U_INTERNAL_DUMP("*ptr = %C", *ptr)

   U_INTERNAL_ASSERT_EQUALS(*ptr,'+')

   _key->dsize = (*++ptr == '0' ? (++ptr, 0) : strtol(ptr, (char**)&ptr, 10));

   U_INTERNAL_ASSERT_EQUALS(*ptr,',')

   if (*++ptr == '-')
      {
      // special case: deleted key

      _data->dsize = U_NOT_FOUND;

      ptr += 2;

      U_INTERNAL_ASSERT_EQUALS(*(ptr-1),'1')
      }
   else
      {
      _data->dsize = strtol(ptr, (char**)&ptr, 10);
      }

   U_INTERNAL_ASSERT_EQUALS(*ptr,':')

   _key->dptr = (void*)++ptr;

   ptr += _key->dsize + 2;

   U_INTERNAL_ASSERT_EQUALS(*(ptr-2),'-')
   U_INTERNAL_ASSERT_EQUALS(*(ptr-1),'>')

   if (_data->dsize == U_NOT_FOUND)
      { 
      // special case: deleted key

      _data->dptr = 0;
      }
   else
      {
      _data->dptr = (void*)ptr;

      ptr += _data->dsize;
      }

   U_INTERNAL_DUMP("*ptr = %C key = %.*S data = %.*S", *ptr, _key->dsize, _key->dptr, _data->dsize, _data->dptr)

   if (*ptr == '\n') ++ptr;

   U_RETURN((char*)ptr);
}

// Call function for all entry

U_NO_EXPORT void URDB::makeAdd1(uint32_t offset) // entry presenti nella cache...
{
   U_TRACE(0, "URDB::makeAdd1(%u)", offset)

   URDB::cache_node* n = RDB_ptr_node(offset);

   if (RDB_cache_node(n,left))  makeAdd1(RDB_cache_node(n,left));
   if (RDB_cache_node(n,right)) makeAdd1(RDB_cache_node(n,right));

   // check if node is mark for deleted (dptr == NULL)...

   if (RDB_cache_node(n,data.dptr))
      {
      UCDB::datum _key  = { (void*)((ptrdiff_t)RDB_cache_node(n,key.dptr)  +
                                    (ptrdiff_t)ptr_rdb->journal.map), RDB_cache_node(n,key.dsize) },
                  _data = { (void*)((ptrdiff_t)RDB_cache_node(n,data.dptr) +
                                    (ptrdiff_t)ptr_rdb->journal.map), RDB_cache_node(n,data.dsize) };

      UCDB::makeAdd(_key, _data);
      }
   else if (RDB_cache_node(n,data.dsize) != U_NOT_FOUND)
      {
      U_WARNING("data node at offset %u is invalid, key: %#.*S", offset,
                  RDB_cache_node(n,key.dsize), (const char*)((ptrdiff_t)RDB_cache_node(n,key.dptr) + (ptrdiff_t)ptr_rdb->journal.map));

      u_put_unaligned(U_NOT_FOUND, n->data.dsize);
      }
}

U_NO_EXPORT inline void URDB::makeAdd2(char* src)
{
   U_TRACE(0, "URDB::makeAdd2(%p)", src)

   // ...entry NON presenti nella cache...

   U_INTERNAL_DUMP("key = %.*S", ptr_rdb->UCDB::key.dsize, ptr_rdb->UCDB::key.dptr)

   if (ptr_rdb->htLookup() == false) UCDB::makeAdd(src);
}

U_NO_EXPORT inline void URDB::makeAdd()
{
   U_TRACE(0, "URDB::makeAdd()")

   call(&URDB::makeAdd1, &URDB::makeAdd2);
}

// PRINT DATABASE

U_NO_EXPORT void URDB::print1(uint32_t offset) // entry presenti nella cache...
{
   U_TRACE(0, "URDB::print1(%u)", offset)

   URDB::cache_node* n = RDB_ptr_node(offset);

   if (RDB_cache_node(n,left))  print1(RDB_cache_node(n,left));
   if (RDB_cache_node(n,right)) print1(RDB_cache_node(n,right));

   // check if node is mark for deleted (dptr == NULL)...

   if (RDB_cache_node(n,data.dptr))
      {
      char tmp[40];
      uint32_t size = u__snprintf(tmp, sizeof(tmp), "+%u,%u:", RDB_cache_node(n,key.dsize), RDB_cache_node(n,data.dsize));

      UCDB::pbuffer->append(tmp, size);
      UCDB::pbuffer->append((const char*)((ptrdiff_t)RDB_cache_node(n,key.dptr) +
                                          (ptrdiff_t)ptr_rdb->journal.map), RDB_cache_node(n,key.dsize));
      UCDB::pbuffer->append(U_CONSTANT_TO_PARAM("->"));
      UCDB::pbuffer->append((const char*)((ptrdiff_t)RDB_cache_node(n,data.dptr) +
                                          (ptrdiff_t)ptr_rdb->journal.map), RDB_cache_node(n,data.dsize));
      UCDB::pbuffer->push_back('\n');
      }
   else if (RDB_cache_node(n,data.dsize) != U_NOT_FOUND)
      {
      U_WARNING("data node at offset %u is invalid, key: %#.*S", offset,
                  RDB_cache_node(n,key.dsize), (const char*)((ptrdiff_t)RDB_cache_node(n,key.dptr) + (ptrdiff_t)ptr_rdb->journal.map));

      u_put_unaligned(U_NOT_FOUND, n->data.dsize);
      }
}

U_NO_EXPORT inline void URDB::print2(char* src)
{
   U_TRACE(0, "URDB::print2(%p)", src)

   // ...entry NON presenti nella cache...

   if (ptr_rdb->htLookup() == false) UCDB::print(src);
}

UString URDB::print()
{
   U_TRACE(0, "URDB::print()")

   U_CHECK_MEMORY

   uint32_t _size = UFile::st_size + RDB_off;

   U_INTERNAL_DUMP("size = %u", _size)

   if (_size)
      {
      lock();

      ptr_rdb = this;

      UCDB::setCDB(0);

      UString buffer(_size);

      UCDB::pbuffer = &buffer;

      call(&URDB::print1, &URDB::print2);

      unlock();

      U_RETURN_STRING(buffer);
      }

   U_RETURN_STRING(UString::getStringNull());
}

// Call function for all entry

U_NO_EXPORT inline void URDB::call(vPFu function1, vPFpc function2)
{
   U_TRACE(0, "URDB::call(%p,%p)", function1, function2)

   U_INTERNAL_DUMP("ptr_rdb = %p", ptr_rdb)
   U_INTERNAL_DUMP("ptr_cdb = %p", UCDB::internal.ptr_cdb)

   U_CHECK_MEMORY

   // prima si leggono le entry nella cache,..

   for (int i = 0; i < CACHE_HASHTAB_LEN; ++i)
      {
      if (RDB_hashtab[i])
         {
         function1(RDB_hashtab[i]);

         if (UCDB::internal.stop_call_for_all_entry)
            {
             UCDB::internal.stop_call_for_all_entry = false;

            return;
            }
         }
      }

   // ...poi si scansiona il constant database e si cercano le entry NON presenti nella cache...

   if (UFile::st_size) UCDB::callForAllEntryExt(function2);
}

U_NO_EXPORT void URDB::call1(uint32_t offset) // entry presenti nella cache...
{
   U_TRACE(0, "URDB::call1(%u)", offset)

   URDB::cache_node* n = RDB_ptr_node(offset);

   if (RDB_cache_node(n,left))  call1(RDB_cache_node(n,left));
   if (RDB_cache_node(n,right)) call1(RDB_cache_node(n,right));

   // check if node is mark for deleted (dptr == NULL)...

   if (RDB_cache_node(n,data.dptr))
      {
      UCDB::call((const char*)((ptrdiff_t)RDB_cache_node(n,key.dptr)  +
                               (ptrdiff_t)ptr_rdb->journal.map), RDB_cache_node(n,key.dsize),
                 (const char*)((ptrdiff_t)RDB_cache_node(n,data.dptr) +
                               (ptrdiff_t)ptr_rdb->journal.map), RDB_cache_node(n,data.dsize));
      }
   else if (RDB_cache_node(n,data.dsize) != U_NOT_FOUND)
      {
      U_WARNING("data node at offset %u is invalid, key: %#.*S", offset,
                  RDB_cache_node(n,key.dsize), (const char*)((ptrdiff_t)RDB_cache_node(n,key.dptr) + (ptrdiff_t)ptr_rdb->journal.map));

      u_put_unaligned(U_NOT_FOUND, n->data.dsize);
      }
}

U_NO_EXPORT inline void URDB::call2(char* src)
{
   U_TRACE(0, "URDB::call2(%p)", src)

   // ...entry NON presenti nella cache...

   U_INTERNAL_DUMP("key = %.*S", ptr_rdb->UCDB::key.dsize, ptr_rdb->UCDB::key.dptr)

   if (ptr_rdb->htLookup() == false) UCDB::call(src);
}

void URDB::callForAllEntry(vPFprpr function, UVector<UString>* vec)
{
   U_TRACE(0, "URDB::callForAllEntry(%p,%p)", function, vec)

   lock();

   ptr_rdb = this;

   UCDB::setVector(vec);
   UCDB::setFunctionToCall(function);

   call(&URDB::call1, &URDB::call2);

   UCDB::setFunctionToCall(0);

   unlock();
}

// gets KEY

U_NO_EXPORT void URDB::getKeys1(uint32_t offset) // entry presenti nella cache...
{
   U_TRACE(0, "URDB::getKeys1(%u)", offset)

   URDB::cache_node* n = RDB_ptr_node(offset);

   if (RDB_cache_node(n,left))  getKeys1(RDB_cache_node(n,left));
   if (RDB_cache_node(n,right)) getKeys1(RDB_cache_node(n,right));

   // check if node is mark for deleted (dptr == NULL)...

   if (RDB_cache_node(n,data.dptr))
      {
      UStringRep* rep = U_NEW(UStringRep((const char*)((ptrdiff_t)ptr_rdb->journal.map+(ptrdiff_t)RDB_cache_node(n,key.dptr)),RDB_cache_node(n,key.dsize)));

      kvec->UVector<void*>::push(rep);
      }
   else if (RDB_cache_node(n,data.dsize) != U_NOT_FOUND)
      {
      U_WARNING("data node at offset %u is invalid, key: %#.*S", offset,
                  RDB_cache_node(n,key.dsize), (const char*)((ptrdiff_t)RDB_cache_node(n,key.dptr) + (ptrdiff_t)ptr_rdb->journal.map));

      u_put_unaligned(U_NOT_FOUND, n->data.dsize);
      }
}

U_NO_EXPORT inline void URDB::getKeys2(char* src)
{
   U_TRACE(0, "URDB::getKeys2(%p)", src)

   // ...entry NON presenti nella cache...

   U_INTERNAL_DUMP("key = %.*S", ptr_rdb->UCDB::key.dsize, ptr_rdb->UCDB::key.dptr)

   if (ptr_rdb->htLookup() == false)
      {
      UStringRep* rep = U_NEW(UStringRep((const char*)ptr_rdb->UCDB::key.dptr, ptr_rdb->UCDB::key.dsize));

      kvec->UVector<void*>::push(rep);
      }
}

void URDB::getKeys(UVector<UString>& vec)
{
   U_TRACE(0, "URDB::getKeys(%p)", &vec)

   lock();

   kvec = &vec;

   ptr_rdb = this;

   call(&URDB::getKeys1, &URDB::getKeys2);

   unlock();
}

void URDB::callForAllEntrySorted(vPFprpr function, UVector<UString>* vec, qcompare compare_obj)
{
   U_TRACE(0, "URDB::callForAllEntrySorted(%p,%p,%p)", function, vec, compare_obj)

   U_INTERNAL_DUMP("UCDB::nrecord = %u RDB_nrecord = %u", UCDB::nrecord, RDB_nrecord)

   uint32_t n = UCDB::nrecord + RDB_nrecord;

   if (n)
      {
      lock();

      UVector<UString> vkey(n);

      getKeys(vkey);

      if (n > 1)
         {
         if (compare_obj) vkey.UVector<void*>::sort(compare_obj);
         else             vkey.sort(ignore_case);
         }

      UCDB::setVector(vec);
      UCDB::setFunctionToCall(function);

      UStringRep* r;

      for (uint32_t i = 0; i < n; ++i)
         {
         r = vkey.UVector<UStringRep*>::at(i);

         UCDB::setKey(r);

         if (fetch())
            {
            UCDB::call((const char*)UCDB::key.dptr,  UCDB::key.dsize,
                       (const char*)UCDB::data.dptr, UCDB::data.dsize);
            }
         }

      UCDB::setFunctionToCall(0);

      unlock();
      }
}

UString URDB::printSorted()
{
   U_TRACE(0, "URDB::printSorted()")

   U_CHECK_MEMORY

   uint32_t _size = UFile::st_size + RDB_off;

   U_INTERNAL_DUMP("size = %u", _size)

   if (_size)
      {
      UString buffer(_size);

      U_INTERNAL_DUMP("UCDB::nrecord = %u RDB_nrecord = %u", UCDB::nrecord, RDB_nrecord)

      uint32_t n = UCDB::nrecord + RDB_nrecord;

      U_INTERNAL_DUMP("n = %u", n)

      UVector<UString> vkey(n);

      getKeys(vkey);

      if (n > 1) vkey.sort(ignore_case);

      char tmp[40];
      UStringRep* r;

      for (uint32_t i = 0; i < n; ++i)
         {
         r = vkey.UVector<UStringRep*>::at(i);

         UCDB::setKey(r);

         if (fetch())
            {
            _size = u__snprintf(tmp, sizeof(tmp), "+%u,%u:", UCDB::key.dsize, UCDB::data.dsize);

            buffer.append(tmp, _size);
            buffer.append((const char*) UCDB::key.dptr, UCDB::key.dsize);
            buffer.append(U_CONSTANT_TO_PARAM("->"));
            buffer.append((const char*)UCDB::data.dptr, UCDB::data.dsize);
            buffer.push_back('\n');
            }
         }

      U_RETURN_STRING(buffer);
      }

   U_RETURN_STRING(UString::getStringNull());
}

bool URDB::isDeleted()
{
   U_TRACE(0, "URDB::isDeleted()")

   U_CHECK_MEMORY

   bool result = (RDB_node_data_pr == 0);

   if (result &&
       RDB_node_data_sz != U_NOT_FOUND)
      {
      U_WARNING("data node is invalid, key: %#.*S", RDB_node_key_sz, RDB_node_key_pr);

      u_put_unaligned(U_NOT_FOUND, RDB_node_data_sz);
      }

   U_RETURN(result);
}

// SERVICES

// --------------------------------------------------------------------
// Fetch the value for a given key from the database.
// --------------------------------------------------------------------
// If the lookup failed, datum is NULL
// If the lookup succeeded, datum points to the value from the database
// --------------------------------------------------------------------

bool URDB::fetch()
{
   U_TRACE(0, "URDB::fetch()")

   bool result;

   UCDB::cdb_hash();

   lock();

   // Search one key/data pair in the cache or in the cdb

   if (htLookup() == false) result = cdbLookup();
   else
      {
      if (isDeleted()) result = false;
      else
         {
         result           = true;
         UCDB::data.dptr  = RDB_node_data;
         UCDB::data.dsize = RDB_node_data_sz;
         }
      }

   unlock();

   U_RETURN(result);
}

UString URDB::at()
{
   U_TRACE(0, "URDB::at()")

   if (fetch()) return UCDB::elem();

   U_RETURN_STRING(UString::getStringNull());
}

// ---------------------------------------------------------------------
// Write a key/value pair to a reliable database
// ---------------------------------------------------------------------
// RETURN VALUE
// ---------------------------------------------------------------------
//  0: Everything was OK
// -1: flag was RDB_INSERT and this key already existed
// -3: there is not enough (virtual) memory available on writing journal
// ---------------------------------------------------------------------

int URDB::store(int flag)
{
   U_TRACE(0, "URDB::store(%d)", flag)

   int result = 0;

   UCDB::cdb_hash();

   lock();

   bool exist = htLookup(); // Search one key/data pair in the cache

   if (exist)
      {
      if (flag == RDB_INSERT && // Insertion of new entries only
          isDeleted() == false)
         {
         result = -1; // -1: flag was RDB_INSERT and this key already existed

         goto end;
         }

      if (flag             == RDB_REPLACE      &&
          UCDB::data.dsize == RDB_node_data_sz &&
          memcmp(RDB_node_data, UCDB::data.dptr, UCDB::data.dsize) == 0)
         {
         goto end;
         }
      }
   else
      {
      UCDB::datum data_new = UCDB::data;

      exist = cdbLookup(); // Search one key/data pair in the cdb

      if (exist)
         {
         if (flag == RDB_INSERT) // Insertion of new entries only
            {
            result = -1; // -1: flag was RDB_INSERT and this key already existed

            goto end;
            }

         if (flag             == RDB_REPLACE    &&
             UCDB::data.dsize == data_new.dsize &&
             memcmp(data_new.dptr, UCDB::data.dptr, UCDB::data.dsize) == 0)
            {
            goto end;
            }
         }

      htAlloc();

      UCDB::data = data_new;
      }

   U_INTERNAL_ASSERT_EQUALS(result,0)

   if (logJournal(1) == false)
      {
      result = -3; // -3: there is not enough (virtual) memory available on writing journal

      htRemoveAlloc();

      goto end;
      }

   // NB: i riferimenti in memoria ai dati e alle chiavi devono puntare sul journal mappato in memoria...

   htInsert(); // Insertion of new entry in the cache

   if (exist == false) RDB_nrecord++;

   U_INTERNAL_DUMP("nrecord = %u", RDB_nrecord)

end:
   unlock();

   U_RETURN(result);
}

// ----------------------------------------------------------------------
// Mark a key/value as deleted
// ----------------------------------------------------------------------
// RETURN VALUE
// ----------------------------------------------------------------------
//  0: Everything was OK
// -1: The entry was not in the database
// -2: The entry was already marked deleted in the cache
// -3: there is not enough (virtual) memory available on writing journal
// ----------------------------------------------------------------------

int URDB::remove()
{
   U_TRACE(0, "URDB::remove()")

   int result = 0;

   UCDB::cdb_hash();

   lock();

   if (htLookup()) // Search one key/data pair in the cache
      {
      if (isDeleted()) result = -2; // -2: The entry was already marked deleted in the cache
      }
   else
      {
      // Search one key/data pair in the cdb

      if (cdbLookup()) htAlloc();
      else             result = -1; // -1: The entry was not in the database
      }

   if (result == 0)
      {
      if (logJournal(0) == false)
         {
         result = -3; // -3: there is not enough (virtual) memory available on writing journal

         htRemoveAlloc();

         goto end;
         }

      // NB: i riferimenti in memoria ai dati e alle chiavi devono puntare sul journal mappato in memoria...

      UCDB::data.dptr  = 0;
      UCDB::data.dsize = U_NOT_FOUND;

      htInsert(); // Insertion or update of new entry of the cache

      RDB_nrecord--;

      U_INTERNAL_DUMP("nrecord = %u", RDB_nrecord)
      }

end:
   unlock();

   U_RETURN(result);
}

// inlining failed in call to 'URDB::remove(UString const&)': call is unlikely and code size would grow

int URDB::remove(const UString& _key)
{
   U_TRACE(0, "URDB::remove(%.*S)", U_STRING_TO_TRACE(_key))

   UCDB::setKey(_key);

   int result = remove();

   U_RETURN(result);
}

// ---------------------------------------------------------------------
// Substitute a key/value with a new key/value (remove+store)
// ---------------------------------------------------------------------
// RETURN VALUE
// ---------------------------------------------------------------------
//  0: Everything was OK
// -1: The entry was not in the database
// -2: The entry was marked deleted in the cache 
// -3: there is not enough (virtual) memory available on writing journal
// -4: flag was RDB_INSERT and the new key already existed
// ---------------------------------------------------------------------

int URDB::substitute(UCDB::datum* key2, int flag)
{
   U_TRACE(0, "URDB::substitute(%p,%d)", key2, flag)

   int result        = 0;
   UCDB::datum data2 = UCDB::data;

   UCDB::cdb_hash();

   key1 = UCDB::key;

   lock();

   // search for remove

   if (htLookup()) // Search one key/data pair in the cache
      {
      if (isDeleted()) result = -2; // -2: The entry was already marked deleted in the cache
      }
   else
      {
      // Search one key/data pair in the cdb

      if (cdbLookup() == false) result = -1; // -1: The entry was not in the database
      }

   if (result == 0)
      {
      // save cache pointer

      uint32_t   node1 =  node;
      uint32_t* pnode1 = pnode;

      // search for store

      UCDB::key = *key2;

      UCDB::cdb_hash();

      if (htLookup()) // Search one key/data pair in the cache
         {
         if (flag == RDB_INSERT && // Insertion of new entries only
             isDeleted() == false)
            {
            result = -4; // -4: flag was RDB_INSERT and this key already existed
            }
         }
      else
         {
         if (cdbLookup() &&      // Search one key/data pair in the cdb
             flag == RDB_INSERT) // Insertion of new entries only
            {
            result = -4; // -4: flag was RDB_INSERT and this key already existed
            }
         else
            {
            U_INTERNAL_ASSERT_EQUALS(result,0)

            htAlloc();
            }
         }

      if (result == 0) // ok, substitute
         {
         UCDB::data = data2;

         if (logJournal(2) == false)
            {
            result = -3; // -3: disk full writing to the journal file

            htRemoveAlloc();

            goto end;
            }

         // NB: i riferimenti in memoria ai dati e alle chiavi devono puntare sul journal mappato in memoria...

         htInsert(); // Insertion of new entry in the cache

         // remove of old entry

         UCDB::key        = key1;
         UCDB::data.dptr  = 0;
         UCDB::data.dsize = U_NOT_FOUND;

                   pnode = pnode1;
         if (node1) node =  node1;
         else       htAlloc();

         htInsert(); // Insertion or update of new entry in the cache
         }
      }

end:
   unlock();

   U_RETURN(result);
}

// inlining failed in call to ...: call is unlikely and code size would grow

bool URDB::find(const UString& _key)
{
   U_TRACE(0, "URDB::find(%.*S)", U_STRING_TO_TRACE(_key))

   UCDB::setKey(_key);

   return fetch(); // Fetch the value for a given key from the database
}

int URDB::store(UStringRep* _key, const UString& _data, int flag)
{
   U_TRACE(0, "URDB::store(%.*S,%.*S,%d)", U_STRING_TO_TRACE(*_key), U_STRING_TO_TRACE(_data), flag)

   UCDB::setKey(_key);
   UCDB::setData(_data);

   int result = store(flag);

   U_RETURN(result);
}

int URDB::store(const UString& _key, const UString& _data, int flag)
{
   U_TRACE(0, "URDB::store(%.*S,%.*S,%d)", U_STRING_TO_TRACE(_key), U_STRING_TO_TRACE(_data), flag)

   UCDB::setKey(_key);
   UCDB::setData(_data);

   int result = store(flag);

   U_RETURN(result);
}

int URDB::store(const UString& _key, const UString& new_rec, const UString& old_rec, const UString& padding, int flag)
{
   U_TRACE(0, "URDB::store(%.*S,%.*S,%.*S,%.*S,%d)", U_STRING_TO_TRACE(_key), U_STRING_TO_TRACE(new_rec),
                                                     U_STRING_TO_TRACE(old_rec), U_STRING_TO_TRACE(padding), flag)

   U_INTERNAL_DUMP("new_rec.size() = %u old_rec.size() = %u", new_rec.size(), old_rec.size())

   UString x = new_rec;

   if (flag == RDB_REPLACE)
      {
      if (new_rec.size() <= old_rec.size())
         {
         uint32_t n = new_rec.size();
         char* _ptr = old_rec.data();

         U__MEMCPY(_ptr, new_rec.data(), n);

         (void) memset(_ptr + n, ' ', old_rec.size() - n);

         U_INTERNAL_DUMP("_ptr = %.*s", old_rec.size(), _ptr)

         U_RETURN(0);
         }
      }
   else
      {
      U_INTERNAL_ASSERT_EQUALS(flag, RDB_INSERT)

      x += padding; 
      }

   UCDB::setKey(_key);
   UCDB::setData(x);

   int result = store(flag);

   U_RETURN(result);
}

int URDB::substitute(const UString& _key, const UString& new_key, const UString& _data, int flag)
{
   U_TRACE(0, "URDB::substitute(%.*S,%.*S,%.*S,%d)", U_STRING_TO_TRACE(_key), U_STRING_TO_TRACE(new_key),
                                                     U_STRING_TO_TRACE(_data), flag)

   UCDB::setKey(_key);
   UCDB::setData(_data);
   UCDB::datum key2 = { (void*) new_key.data(), new_key.size() };

   int result = substitute(&key2, flag);

   U_RETURN(result);
}

// STREAM

U_EXPORT ostream& operator<<(ostream& os, URDB& rdb)
{
   U_TRACE(0+256, "URDB::operator<<(%p,%p)", &os, &rdb)

   UString text = rdb.print();

   (void) os.write(text.data(), text.size());

   os.put('\n');

   return os;
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* URDB::dump(bool _reset) const
{
   UCDB::dump(false);

   *UObjectIO::os << "\n"
                  << "node                      " << node            << '\n'
                  << "pnode                     " << (void*)pnode    << '\n'
                  << "_lock   (ULock            " << (void*)&_lock   << ")\n"
                  << "journal (UFile            " << (void*)&journal << ')';

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
