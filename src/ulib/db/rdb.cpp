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

UCDB::datum URDB::key1;

// Search one key/data pair in the cache.
// To save code, I use only a single function to do the lookup in the hash table and binary search tree

U_NO_EXPORT bool URDB::htLookup()
{
   U_TRACE(0, "URDB::htLookup()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(key.dptr)

   // Because the insertion routine has to know where to insert the cache_node, this code has to assign
   // a pointer to the empty pointer to manipulate in that case, so we have to do a nasty indirection...

   pnode = RDB_hashtab + (UCDB::khash % CACHE_HASHTAB_LEN);

   U_INTERNAL_DUMP("pnode = %p slot = %u", pnode, UCDB::khash % CACHE_HASHTAB_LEN)

   while ((node = u_get_unalignedp(pnode)))
      {
      U_INTERNAL_DUMP("pnode = %p node = %u", pnode, node)

      int result = u_equal(key.dptr, RDB_node_key, U_min(key.dsize, RDB_node_key_sz), UCDB::ignore_case);

      if (result    == 0 &&
          key.dsize == RDB_node_key_sz)
         {
         U_RETURN(true);
         }

      pnode = &(result < 0 ? RDB_node->left
                           : RDB_node->right);
      }

   // NB: i riferimenti in memoria alla cache dati devono puntare sulla memoria mappata...

   U_INTERNAL_DUMP("pnode = %p node = %u", pnode, node)

   U_INTERNAL_ASSERT(node <= RDB_capacity)
   U_INTERNAL_ASSERT_RANGE(RDB_hashtab, pnode, (uint32_t*)RDB_eof)

   U_RETURN(false);
}

// Insert one key/data pair in the cache

U_NO_EXPORT void URDB::htInsert()
{
   U_TRACE(1, "URDB::htInsert()")

   U_CHECK_MEMORY

   if (node == 0) // Alloc one node for the hash tree
      {
      U_INTERNAL_ASSERT_POINTER(key.dptr)

      U_INTERNAL_DUMP("RDB_capacity = %u", RDB_capacity)

      U_INTERNAL_ASSERT_MAJOR(RDB_capacity,sizeof(URDB::cache_node))

      node = RDB_off;

      u_put_unalignedp(node, pnode);

      RDB_off += sizeof(URDB::cache_node);

      U_INTERNAL_DUMP("pnode = %p node = %u", pnode, node)

      u_put_unaligned(0, RDB_node->left);
      u_put_unaligned(0, RDB_node->right);
      }

   // NB: i riferimenti in memoria ai dati e alle chiavi devono puntare sul journal mappato in memoria...

   U_INTERNAL_DUMP("key  = { %p, %u }",  key.dptr,  key.dsize)
   U_INTERNAL_DUMP("data = { %p, %u }", data.dptr, data.dsize)

#ifdef DEBUG
                    U_INTERNAL_ASSERT_RANGE(RDB_ptr,  key.dptr, RDB_eof)
   if (data.dptr) { U_INTERNAL_ASSERT_RANGE(RDB_ptr, data.dptr, RDB_eof) }
#endif

   // NB: i riferimenti in memoria alla cache dati devono puntare sulla memoria mappata...

   U_INTERNAL_ASSERT_RANGE(sizeof(URDB::cache_struct), node, RDB_size - sizeof(URDB::cache_node))
   U_INTERNAL_ASSERT_RANGE(RDB_hashtab,               pnode, (uint32_t*)RDB_eof)

   u_put_unaligned(((char*)key.dptr - journal.map),                    RDB_node->key.dptr);
   u_put_unaligned(key.dsize,                                          RDB_node->key.dsize);
   u_put_unaligned((data.dptr ? ((char*)data.dptr - journal.map) : 0), RDB_node->data.dptr);
   u_put_unaligned(data.dsize,                                         RDB_node->data.dsize);
}

// open a Reliable DataBase

bool URDB::open(uint32_t log_size, int flag)
{
   U_TRACE(0, "URDB::open(%u,%d)", log_size, flag)

   if (UCDB::open() && log_size == 0) log_size = UFile::st_size;

   journal.setPath(*(const UFile*)this, 0, U_CONSTANT_TO_PARAM(".jnl"));

   bool result = false;

   lock.lock();

   if (journal.creat(flag | O_RDWR) &&
       journal.ftruncate(journal.size() + (log_size ? log_size : 512L*1024L)))
      {
#if defined(__CYGWIN__) || defined(__MINGW32__)
      off_t journal_size =       journal.st_size;
#  else
      off_t journal_size = U_max(journal.st_size, 32L*1024L*1024L); // oversize mmap for optimize resizeJournal() with ftruncate()
#  endif

      if (journal.memmap(PROT_READ | PROT_WRITE, 0, 0, journal_size))
         {
      // U_INTERNAL_ASSERT_EQUALS(RDB_off, RDB_size)

                           RDB_page = RDB_size;
                           RDB_size = journal.st_size;
         if (RDB_off == 0) RDB_off  = sizeof(URDB::cache_struct);

         U_INTERNAL_DUMP("RDB_off = %u RDB_page = %u capacity = %u lrecord = %d", RDB_off, RDB_page, RDB_capacity, RDB_lrecord)

         result = true;
         }
      }

   lock.unlock();

   U_RETURN(result);
}

// Close a Reliable DataBase

void URDB::close()
{
   U_TRACE(0, "URDB::close()")

   U_CHECK_MEMORY

   if (UFile::map_size) UFile::munmap(); // Constant DB

   lock.lock();

   bool bactive = isActive(); // there are diff's ?...

   if (bactive)
      {
      RDB_size = RDB_off;

#  if defined(__CYGWIN__) || defined(__MINGW32__)
      journal.munmap(); // for ftruncate()...
#  endif

      (void) journal.ftruncate(RDB_off);
      }

   journal.close();

#if defined(__CYGWIN__) || defined(__MINGW32__)
   if (bactive == false)
#endif
   journal.munmap();
   journal.reset();

   lock.unlock();
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

static URDB* ptr_rdb;

inline void URDB::makeAdd1(uint32_t offset) // entry presenti nella cache...
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
#ifdef DEBUG
   else { U_INTERNAL_ASSERT_EQUALS(RDB_cache_node(n,data.dsize), U_NOT_FOUND) }
#endif
}

inline void URDB::makeAdd2(char* src)
{
   U_TRACE(0, "URDB::makeAdd2(%p)", src)

   // ...entry NON presenti nella cache...

   U_INTERNAL_DUMP("key = %.*S", ptr_rdb->key.dsize, ptr_rdb->key.dptr)

   if (ptr_rdb->htLookup() == false) UCDB::makeAdd(src);
}

inline void URDB::call(vPFu function1, vPFpc function2)
{
   U_TRACE(0, "URDB::call(%p,%p)", function1, function2)

   U_INTERNAL_DUMP("ptr_rdb = %p", ptr_rdb)
   U_INTERNAL_DUMP("ptr_cdb = %p", UCDB::getCDB())

   U_CHECK_MEMORY

   // prima si leggono le entry nella cache,..

   for (int i = 0; i < CACHE_HASHTAB_LEN; ++i) if (RDB_hashtab[i]) function1(RDB_hashtab[i]);

   // ...poi si scansiona il constant database e si cercano le entry NON presenti nella cache...

   if (UFile::st_size) UCDB::callForAllEntryExt(function2);
}

inline void URDB::makeAdd()
{
   U_TRACE(0, "URDB::makeAdd()")

   call(&URDB::makeAdd1, &URDB::makeAdd2);
}

void URDB::reset()
{
   U_TRACE(1, "URDB::reset()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_DIFFERS(journal.map,MAP_FAILED)

   RDB_off     = sizeof(URDB::cache_struct);
   RDB_page    = 0;
   RDB_lrecord = 0;

   // Initialize the cache to contain no entries

   (void) U_SYSCALL(memset, "%p,%C,%d", RDB_hashtab, 0, sizeof(RDB_hashtab));
}

void URDB::abortTransaction()
{
   U_TRACE(0, "URDB::abortTransaction()")

   URDB::reset();

   lock.unlock();
}

// Combines the old cdb file and the diffs in a new cdb file

bool URDB::reorganize()
{
   U_TRACE(0, "URDB::reorganize()")

   U_CHECK_MEMORY

   lock.lock();

   bool result = true;

   if (isActive()) // there are diff's ?...
      {
      UCDB cdb(UCDB::ignore_case);
      char cdb_buffer_path[MAX_FILENAME_LEN];

      cdb.setPath(*(const UFile*)this, cdb_buffer_path, U_CONSTANT_TO_PARAM(".tmp"));

      result = cdb.creat(O_RDWR) &&
               cdb.ftruncate(UFile::st_size + RDB_size + sizeFor(4096));

      if (result)
         {
         result = cdb.memmap(PROT_READ | PROT_WRITE);

         if (result == false) U_RETURN(false);

         // prima si leggono le entry nella cache
         // poi si scansiona il constant database e si cercano le entry NON presenti nella cache...

         cdb.makeStart();

         ptr_rdb = this;
         UCDB::setCDB(&cdb);

         makeAdd();

         off_t pos = cdb.makeFinish(false);

         U_INTERNAL_ASSERT(pos <= (off_t)cdb.st_size)

#     if defined(__CYGWIN__) || defined(__MINGW32__)
         cdb.munmap(); // for ftruncate()...
#     endif

         result = cdb.ftruncate(pos);

         U_INTERNAL_ASSERT_EQUALS(result,true)

         if (result)
            {
#        if defined(__MINGW32__) || defined(__CYGWIN__)
            UFile::munmap(); // for rename()...
#           ifdef   __MINGW32__
            cdb.UFile::close();
#           endif
#        endif

            result = cdb.rename(UFile::path_relativ);
            }

         U_INTERNAL_ASSERT_EQUALS(result,true)

         if (result)
            {
            URDB::reset();

#        if defined(__MINGW32__) || defined(__CYGWIN__)
#           ifdef   __MINGW32__
            result = cdb.UFile::open(UFile::path_relativ);
                     cdb.st_size = pos;
#           endif
            result = cdb.memmap(); // read only...
#        endif

            cdb.UFile::close();

            UFile::substitute(cdb);

            UCDB::nrecord               = cdb.nrecord;
            UCDB::start_hash_table_slot = cdb.start_hash_table_slot;

            U_INTERNAL_DUMP("UCDB::nrecord = %u RDB_lrecord = %u", UCDB::nrecord, RDB_lrecord)
            }
         }
      }

   lock.unlock();

   U_RETURN(result);
}

// PRINT DATABASE

inline void URDB::print1(uint32_t offset) // entry presenti nella cache...
{
   U_TRACE(0, "URDB::print1(%u)", offset)

   URDB::cache_node* n = RDB_ptr_node(offset);

   if (RDB_cache_node(n,left))  print1(RDB_cache_node(n,left));
   if (RDB_cache_node(n,right)) print1(RDB_cache_node(n,right));

   // check if node is mark for deleted (dptr == NULL)...

   if (RDB_cache_node(n,data.dptr))
      {
      char tmp[40];
      uint32_t size = u_snprintf(tmp, sizeof(tmp), "+%u,%u:", RDB_cache_node(n,key.dsize), RDB_cache_node(n,data.dsize));

      UCDB::pbuffer->append(tmp, size);
      UCDB::pbuffer->append((const char*)((ptrdiff_t)RDB_cache_node(n,key.dptr) +
                                          (ptrdiff_t)ptr_rdb->journal.map), RDB_cache_node(n,key.dsize));
      UCDB::pbuffer->append(U_CONSTANT_TO_PARAM("->"));
      UCDB::pbuffer->append((const char*)((ptrdiff_t)RDB_cache_node(n,data.dptr) +
                                          (ptrdiff_t)ptr_rdb->journal.map), RDB_cache_node(n,data.dsize));
      UCDB::pbuffer->push_back('\n');
      }
#ifdef DEBUG
   else { U_INTERNAL_ASSERT_EQUALS(RDB_cache_node(n,data.dsize), U_NOT_FOUND) }
#endif
}

inline void URDB::print2(char* src)
{
   U_TRACE(0, "URDB::print2(%p)", src)

   // ...entry NON presenti nella cache...

   if (ptr_rdb->htLookup() == false) UCDB::print(src);
}

UString URDB::print()
{
   U_TRACE(0, "URDB::print()")

   U_CHECK_MEMORY

   uint32_t size = UFile::st_size + RDB_off;

   U_INTERNAL_DUMP("size = %u", size)

   if (size)
      {
      lock.lock();

      ptr_rdb = this;

      UCDB::setCDB(0);

      UString buffer(size);

      UCDB::pbuffer = &buffer;

      call(&URDB::print1, &URDB::print2);

      lock.unlock();

      U_RETURN_STRING(buffer);
      }

   U_RETURN_STRING(UString::getStringNull());
}

// Call function for all entry

inline void URDB::call1(uint32_t offset) // entry presenti nella cache...
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
#ifdef DEBUG
   else { U_INTERNAL_ASSERT_EQUALS(RDB_cache_node(n,data.dsize), U_NOT_FOUND) }
#endif
}

inline void URDB::call2(char* src)
{
   U_TRACE(0, "URDB::call2(%p)", src)

   // ...entry NON presenti nella cache...

   U_INTERNAL_DUMP("key = %.*S", ptr_rdb->key.dsize, ptr_rdb->key.dptr)

   if (ptr_rdb->htLookup() == false) UCDB::call(src);
}

void URDB::callForAllEntry(vPFprpr function)
{
   U_TRACE(0, "URDB::callForAllEntry(%p)", function)

   lock.lock();

   ptr_rdb = this;

   UCDB::setFunctionToCall(function);

   call(&URDB::call1, &URDB::call2);

   UCDB::setFunctionToCall(0);

   lock.unlock();
}

// gets KEY

static UVector<UString>* kvec;

inline void URDB::getKeys1(uint32_t offset) // entry presenti nella cache...
{
   U_TRACE(0, "URDB::getKeys1(%u)", offset)

   URDB::cache_node* n = RDB_ptr_node(offset);

   if (RDB_cache_node(n,left))  getKeys1(RDB_cache_node(n,left));
   if (RDB_cache_node(n,right)) getKeys1(RDB_cache_node(n,right));

   // check if node is mark for deleted (dptr == NULL)...

   if (RDB_cache_node(n,data.dptr))
      {
      UStringRep* rep = UStringRep::create((const char*)((ptrdiff_t)RDB_cache_node(n,key.dptr) +
                                                         (ptrdiff_t)ptr_rdb->journal.map), RDB_cache_node(n,key.dsize), 0U);

      kvec->UVector<void*>::push(rep);
      }
#ifdef DEBUG
   else { U_INTERNAL_ASSERT_EQUALS(RDB_cache_node(n,data.dsize), U_NOT_FOUND) }
#endif
}

inline void URDB::getKeys2(char* src)
{
   U_TRACE(0, "URDB::getKeys2(%p)", src)

   // ...entry NON presenti nella cache...

   U_INTERNAL_DUMP("key = %.*S", ptr_rdb->key.dsize, ptr_rdb->key.dptr)

   if (ptr_rdb->htLookup() == false)
      {
      UStringRep* rep = UStringRep::create((const char*)ptr_rdb->key.dptr, ptr_rdb->key.dsize, 0U);

      kvec->UVector<void*>::push(rep);
      }
}

void URDB::getKeys(UVector<UString>& vec)
{
   U_TRACE(0, "URDB::getKeys(%p)", &vec)

   lock.lock();

   kvec = &vec;

   ptr_rdb = this;

   call(&URDB::getKeys1, &URDB::getKeys2);

   U_DUMP("vec.size() = %u vec.capacity() = %u", vec.size(), vec.capacity())

   U_ASSERT_EQUALS(vec.size(), vec.capacity())

   lock.unlock();
}

void URDB::callForAllEntrySorted(vPFprpr function)
{
   U_TRACE(0, "URDB::callForAllEntrySorted(%p)", function)

   U_INTERNAL_DUMP("UCDB::nrecord = %u RDB_lrecord = %u", UCDB::nrecord, RDB_lrecord)

   uint32_t n = UCDB::nrecord + RDB_lrecord;

   U_INTERNAL_DUMP("n = %u", n)

   UVector<UString> vkey(n);

   getKeys(vkey);

   if (n > 1) vkey.sort(ignore_case);

   UStringRep* r;

   UCDB::internal.function_to_call = function;

   for (uint32_t i = 0; i < n; ++i)
      {
      r = vkey.UVector<UStringRep*>::at(i);

      UCDB::setKey(r);

      if (fetch())
         {
         UCDB::call((const char*) key.dptr,  key.dsize,
                    (const char*)data.dptr, data.dsize);
         }
      }

   UCDB::internal.function_to_call = 0;
}

UString URDB::printSorted()
{
   U_TRACE(0, "URDB::printSorted()")

   U_CHECK_MEMORY

   uint32_t size = UFile::st_size + RDB_off;

   U_INTERNAL_DUMP("size = %u", size)

   if (size)
      {
      UString buffer(size);

      U_INTERNAL_DUMP("UCDB::nrecord = %u RDB_lrecord = %u", UCDB::nrecord, RDB_lrecord)

      uint32_t n = UCDB::nrecord + RDB_lrecord;

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
            size = u_snprintf(tmp, sizeof(tmp), "+%u,%u:", key.dsize, data.dsize);

            buffer.append(tmp, size);
            buffer.append((const char*) key.dptr, key.dsize);
            buffer.append(U_CONSTANT_TO_PARAM("->"));
            buffer.append((const char*)data.dptr, data.dsize);
            buffer.push_back('\n');
            }
         }

      U_RETURN_STRING(buffer);
      }

   U_RETURN_STRING(UString::getStringNull());
}

// SERVICES

void URDB::msync()
{
   U_TRACE(0, "URDB::msync()")

   U_CHECK_MEMORY

   lock.lock();

   U_INTERNAL_DUMP("RDB_off = %u RDB_page = %u", RDB_off, RDB_page)

   UFile::msync(RDB_ptr_off, RDB_ptr_page);

   RDB_page = RDB_off;

   U_INTERNAL_DUMP("RDB_off = %u RDB_page = %u", RDB_off, RDB_page)

   lock.unlock();
}

inline bool URDB::resizeJournal(char* ptr)
{
   U_TRACE(0, "URDB::resizeJournal(%p)", ptr)

   U_CHECK_MEMORY

   off_t     size = (journal.st_size / 2),
         oversize = (ptr - RDB_eof);

   if (oversize < size) oversize = size;

   U_INTERNAL_DUMP("oversize = %ld", oversize)

// msync();

   uint32_t offset = (char*)pnode - journal.map;

   U_INTERNAL_DUMP("pnode = %p node = %u offset = %u", pnode, node, offset)

#if defined(__CYGWIN__) || defined(__MINGW32__)
   journal.munmap(); // for ftruncate()...
#endif

   if (journal.ftruncate(journal.st_size + oversize))
      {
#  if defined(__CYGWIN__) || defined(__MINGW32__)
      (void) journal.memmap(PROT_READ | PROT_WRITE);
#  endif

      RDB_size = journal.st_size;

      U_INTERNAL_DUMP("RDB_size = %u", RDB_size)

      pnode = (uint32_t*)(journal.map + offset);

      U_INTERNAL_DUMP("pnode = %p node = %u offset = %u", pnode, node, offset)

      U_RETURN(true);
      }

   // save entry

   key1              = key;
   UCDB::datum data1 = data;

   uint32_t save = UCDB::khash;

   if (reorganize())
      {
      // set old entry

      key  = key1;
      data = data1;

      UCDB::khash = save;
      
      (void) htLookup();

      U_RETURN(true);
      }

   U_RETURN(false);
}

U_NO_EXPORT bool URDB::writev(const struct iovec* iov, int n, uint32_t size)
{
   U_TRACE(0, "URDB::writev(%p,%d,%u)", iov, n, size)

   U_INTERNAL_ASSERT_MAJOR(size,0)

   char* ptr = RDB_ptr_off + size + (sizeof(URDB::cache_node) * 2);

   if (ptr > RDB_eof &&
       resizeJournal(ptr) == false)
      {
      U_RETURN(false);
      }

   U_INTERNAL_DUMP("RDB_off = %u", RDB_off)

   char* journal_ptr = RDB_ptr_off;

   for (int i = 0; i < n; ++i)
      {
      (void) memcpy(journal_ptr, iov[i].iov_base, iov[i].iov_len);

      // NB: Una volta scritti i dati sul journal si cambiano i riferimenti in memoria ai dati
      // e alle chiavi in modo che puntino appunto sul journal mappato in memoria...

      if (n < 5) // remove(), store()
         {
         if      (i == 1)  key.dptr = journal_ptr; // remove(), store()
         else if (i == 2) data.dptr = journal_ptr; // store()
         }
      else // substitute()
         {
         if      (i == 1) key1.dptr = journal_ptr;
         if      (i == 3)  key.dptr = journal_ptr;
         else if (i == 4) data.dptr = journal_ptr;
         }

      journal_ptr += iov[i].iov_len;
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
      UCDB::cdb_record_header hrec = { key.dsize, U_NOT_FOUND };

      U_INTERNAL_DUMP("hrec = { %u, %u }", hrec.klen, hrec.dlen)

      struct iovec iov[2] = { { (caddr_t)&hrec, sizeof(UCDB::cdb_record_header) },
                              { (caddr_t)key.dptr, key.dsize } };

      if (writev(iov, 2, sizeof(UCDB::cdb_record_header) + key.dsize) == false) U_RETURN(false);
      }
   else if (op == 1) // store
      {
      UCDB::cdb_record_header hrec = { key.dsize, data.dsize };

      U_INTERNAL_DUMP("hrec = { %u, %u }", hrec.klen, hrec.dlen)

      struct iovec iov[3] = { { (caddr_t)&hrec, sizeof(UCDB::cdb_record_header) },
                              { (caddr_t) key.dptr,  key.dsize },
                              { (caddr_t)data.dptr, data.dsize } };

      if (writev(iov, 3, sizeof(UCDB::cdb_record_header) + key.dsize + data.dsize) == false) U_RETURN(false);
      }
   else // substitute
      {
      U_INTERNAL_ASSERT_EQUALS(op,2)
      U_INTERNAL_ASSERT_POINTER(key.dptr)
      U_INTERNAL_ASSERT_POINTER(key1.dptr)
      U_INTERNAL_ASSERT_POINTER(data.dptr)
      U_INTERNAL_ASSERT_MAJOR(key.dsize,0)
      U_INTERNAL_ASSERT_MAJOR(key1.dsize,0)
      U_INTERNAL_ASSERT_MAJOR(data.dsize,0)

      UCDB::cdb_record_header hrec1 = { key1.dsize, U_NOT_FOUND },
                              hrec  = {  key.dsize, data.dsize };

      U_INTERNAL_DUMP("hrec1 = { %u, %u } hrec = { %u, %u }", hrec1.klen, hrec1.dlen, hrec.klen, hrec.dlen)

      struct iovec iov[5] = { { (caddr_t)&hrec1, sizeof(UCDB::cdb_record_header) },
                              { (caddr_t)key1.dptr, key1.dsize },
                              { (caddr_t)&hrec, sizeof(UCDB::cdb_record_header) },
                              { (caddr_t)key.dptr, key.dsize },
                              { (caddr_t)data.dptr, data.dsize } };

      if (writev(iov, 5, 2*sizeof(UCDB::cdb_record_header) + key1.dsize + key.dsize + data.dsize) == false) U_RETURN(false);
      }

   U_RETURN(true);
}

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

   lock.lock();

   // Search one key/data pair in the cache or in the cdb

   if (htLookup())
      {
      if (isDeleted()) result = false;
      else
         {
         data.dptr  = RDB_node_data;
         data.dsize = RDB_node_data_sz;

         result = true;
         }
      }
   else
      {
      result = cdbLookup();
      }

   lock.unlock();

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

   lock.lock();

   bool exist = htLookup(); // Search one key/data pair in the cache

   if (exist)
      {
      if (flag == RDB_INSERT && // Insertion of new entries only
          isDeleted() == false)
         {
         result = -1; // -1: flag was RDB_INSERT and this key already existed

         goto end;
         }
      }
   else
      {
      UCDB::datum data1 = data;

      exist = cdbLookup(); // Search one key/data pair in the cdb

      if (exist)
         {
         if (flag == RDB_INSERT) // Insertion of new entries only
            {
            result = -1; // -1: flag was RDB_INSERT and this key already existed

            goto end;
            }
         }

      data = data1;
      }

   U_INTERNAL_ASSERT_EQUALS(result,0)

   if (logJournal(1) == false)
      {
      result = -3; // -3: there is not enough (virtual) memory available on writing journal

      goto end;
      }

   // NB: i riferimenti in memoria ai dati e alle chiavi devono puntare sul journal mappato in memoria...

   htInsert(); // Insertion of new entry in the cache

   if (exist == false) RDB_lrecord++;

   U_INTERNAL_DUMP("lrecord = %d", RDB_lrecord)

end:
   lock.unlock();

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

   lock.lock();

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
      if (logJournal(0) == false)
         {
         result = -3; // -3: there is not enough (virtual) memory available on writing journal

         goto end;
         }

      // NB: i riferimenti in memoria ai dati e alle chiavi devono puntare sul journal mappato in memoria...

      data.dptr  = 0;
      data.dsize = U_NOT_FOUND;

      htInsert(); // Insertion or update of new entry of the cache

      RDB_lrecord--;

      U_INTERNAL_DUMP("lrecord = %d", RDB_lrecord)
      }

end:
   lock.unlock();

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
   UCDB::datum data2 = data;

   UCDB::cdb_hash();

   key1 = key;

   lock.lock();

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

      key = *key2;

      UCDB::cdb_hash();

      if (htLookup()) // Search one key/data pair in the cache
         {
         if (flag == RDB_INSERT && // Insertion of new entries only
             isDeleted() == false)
            {
            result = -4; // -4: flag was RDB_INSERT and this key already existed
            }
         }
      else if (cdbLookup()) // Search one key/data pair in the cdb
         {
         if (flag == RDB_INSERT) // Insertion of new entries only
            {
            result = -4; // -4: flag was RDB_INSERT and this key already existed
            }
         }

      if (result == 0) // ok, substitute
         {
         data = data2;

         if (logJournal(2) == false)
            {
            result = -3; // -3: disk full writing to the journal file

            goto end;
            }

         // NB: i riferimenti in memoria ai dati e alle chiavi devono puntare sul journal mappato in memoria...

         htInsert(); // Insertion of new entry in the cache

         // remove of old entry

          node =  node1;
         pnode = pnode1;

         key        = key1;
         data.dptr  = 0;
         data.dsize = U_NOT_FOUND;

         htInsert(); // Insertion or update of new entry in the cache
         }
      }

end:
   lock.unlock();

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

const char* URDB::dump(bool reset) const
{
   UCDB::dump(false);

   *UObjectIO::os << "\n"
                  << "node                      " << node            << '\n'
                  << "pnode                     " << (void*)pnode    << '\n'
                  << "lock    (ULock            " << (void*)&lock    << ")\n"
                  << "journal (UFile            " << (void*)&journal << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
