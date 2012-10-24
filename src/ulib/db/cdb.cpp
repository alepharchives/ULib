// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    cdb.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/db/cdb.h>
#include <ulib/container/vector.h>

UString*           UCDB::pbuffer;
UCDB::cdb_internal UCDB::internal;

bool UCDB::open(bool brdonly)
{
   U_TRACE(0, "UCDB::open(%b)", brdonly)

   if (UFile::isOpen() ||
       UFile::open(brdonly ? O_RDONLY : O_CREAT | O_RDWR))
      {
      UFile::readSize();

      if (UFile::st_size == 0) nrecord = start_hash_table_slot = 0;
      else
         {
         (void) UFile::memmap(PROT_READ | (brdonly ? 0 : PROT_WRITE));

         if (UFile::map == MAP_FAILED)
            {
            data.dptr = 0;

            hp   =   &hp_buf;
            hr   =   &hr_buf;
            slot = &slot_buf;

            (void) UFile::pread(&start_hash_table_slot, sizeof(uint32_t), 0);
            }
         else
            {
            UFile::close();

            start_hash_table_slot = *(uint32_t*)UFile::map;
            }

         nrecord = (UFile::st_size - start_hash_table_slot) / sizeof(cdb_hash_table_slot);
         }

      U_INTERNAL_DUMP("nrecord = %u", nrecord)

   // U_ASSERT(invariant())

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool UCDB::find()
{
   U_TRACE(0, "UCDB::find()")

   U_INTERNAL_ASSERT_MAJOR(UFile::st_size,0)

   // A record is located as follows. Compute the hash value of the key in the record.
   // The hash value modulo CDB_NUM_HASH_TABLE_POINTER is the number of a hash table

   offset = (khash % CDB_NUM_HASH_TABLE_POINTER) * sizeof(cdb_hash_table_pointer);

   if (UFile::map == MAP_FAILED) (void) UFile::pread(&hp_buf, sizeof(cdb_hash_table_pointer), offset);
   else                          hp = (cdb_hash_table_pointer*) (UFile::map + offset);

   U_INTERNAL_DUMP("hp[%d] = { %u, %u }", offset / sizeof(cdb_hash_table_pointer), hp->pos, hp->slots)

   if (hp->slots)
      {
      // The hash value divided by CDB_NUM_HASH_TABLE_POINTER, modulo the length of that table, is a slot number

      nslot  = (khash / CDB_NUM_HASH_TABLE_POINTER) % hp->slots;
      offset = hp->pos + (nslot * sizeof(cdb_hash_table_slot));

      if (UFile::map == MAP_FAILED) (void) UFile::pread(&slot_buf, sizeof(cdb_hash_table_slot), offset);
      else                          slot = (cdb_hash_table_slot*) (UFile::map + offset);

      U_INTERNAL_DUMP("slot[%d] = { %u, %u }", nslot, u_get_unaligned(slot->hash), u_get_unaligned(slot->pos))

      // Each hash table slot states a hash value and a byte position.
      // If the byte position is 0, the slot is empty.
      // Otherwise, the slot points to a record whose key has that hash value

      if (u_get_unaligned(slot->pos))
         {
         loop = 0;

         return findNext();
         }
      }

   U_RETURN(false);
}

inline bool UCDB::match(uint32_t pos)
{
   U_TRACE(0, "UCDB::match(%u)", pos)

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("UFile::map = %p", UFile::map)

   if (UFile::map == MAP_FAILED)
      {
      char _buf[32];
      uint32_t len = key.dsize, n;
      char* pkey = (char*) key.dptr;

      pos += sizeof(cdb_record_header);

      while (len)
         {
         n = U_min(len, sizeof(_buf));

         (void) UFile::pread(_buf, n, pos);

         if (u_equal(pkey, _buf, n, ignore_case)) U_RETURN(false);

         pkey += n;
         pos  += n;
         len  -= n;
         }

      if (data.dptr) U_FREE_N(data.dptr,data.dsize,char); // free old data...

      data.dsize = u_get_unaligned(hr->dlen);
      data.dptr  = U_MALLOC_N(data.dsize,char);

      (void) UFile::pread(data.dptr, data.dsize, pos);

      U_RETURN(true);
      }

   data.dsize = u_get_unaligned(hr->dlen);

   if (u_equal(key.dptr, ++hr, key.dsize, ignore_case) == 0)
      {
      data.dptr = (char*)hr + key.dsize;

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool UCDB::findNext()
{
   U_TRACE(0, "UCDB::findNext()")

   uint32_t pos;

   // Probe that slot, the next higher slot, and so on, until you find the record or run into an empty slot

   while (++loop <= hp->slots)
      {
      U_INTERNAL_DUMP("loop = %u", loop)

      if (loop > 1)
         {
         // handles repeated keys...

         if (++nslot == hp->slots)
            {
            nslot = 0;

            if (UFile::map == MAP_FAILED) (void) UFile::pread(&slot_buf, sizeof(cdb_hash_table_slot), hp->pos);
            else                          slot = (cdb_hash_table_slot*) (UFile::map + hp->pos);
            }
         else
            {
            if (UFile::map != MAP_FAILED) ++slot;
            else
               {
               offset += sizeof(cdb_hash_table_slot);

               (void) UFile::pread(&slot_buf, sizeof(cdb_hash_table_slot), offset);
               }
            }
         }

      // Each hash table slot states a hash value and a byte position. If the
      // byte position is 0, the slot is empty. Otherwise, the slot points to
      // a record whose key has that hash value

      pos = u_get_unaligned(slot->pos);

      U_INTERNAL_DUMP("slot[%d] = { %u, %u }", nslot, u_get_unaligned(slot->hash), pos)

      if (pos == 0) break;

      if (u_get_unaligned(slot->hash) == khash)
         {
         // Records are stored sequentially, without special alignment.
         // A record states a key length, a data length, the key, and the data

         if (UFile::map == MAP_FAILED) (void) UFile::pread(&hr_buf, sizeof(cdb_record_header), pos);
         else                          hr = (cdb_record_header*)(UFile::map + pos);

         U_INTERNAL_DUMP("hr = { %u, %u }", u_get_unaligned(hr->klen), u_get_unaligned(hr->dlen))

         if (u_get_unaligned(hr->klen) == key.dsize && match(pos)) U_RETURN(true);
         }
      }

   U_RETURN(false);
}

UString UCDB::at()
{
   U_TRACE(0, "UCDB::at()")

   cdb_hash();

   if (find()) return elem();

   U_RETURN_STRING(UString::getStringNull());
}

// FOR RDB

void UCDB::makeAdd(const datum& _key, const datum& _data) // entry presenti nella cache...
{
   U_TRACE(1, "UCDB::makeAdd(%.*S,%.*S)", _key.dsize, _key.dptr, _data.dsize, _data.dptr)

   U_INTERNAL_DUMP("ptr_cdb = %p", internal.ptr_cdb)

   U_INTERNAL_ASSERT_POINTER(internal.ptr_cdb)

   UCDB::cdb_record_header* ptr_hr = internal.ptr_cdb->hr;

   u_put_unaligned( _key.dsize, ptr_hr->klen);
   u_put_unaligned(_data.dsize, ptr_hr->dlen);

   U_INTERNAL_DUMP("hr(%p) = { %u, %u }", ptr_hr, u_get_unaligned(ptr_hr->klen), u_get_unaligned(ptr_hr->dlen))

   ++ptr_hr;
   char* ptr = (char*)ptr_hr;

   U__MEMCPY(ptr, _key.dptr, _key.dsize);

   ptr += _key.dsize;

   U__MEMCPY(ptr, _data.dptr, _data.dsize);

   ptr += _data.dsize;

   internal.ptr_cdb->hr = (UCDB::cdb_record_header*)ptr;

   internal.ptr_cdb->nrecord++;
}

void UCDB::makeAdd(char* src) // entry NON presenti nella cache...
{
   U_TRACE(1, "UCDB::makeAdd(%p)", src)

   U_INTERNAL_DUMP("ptr_cdb = %p", internal.ptr_cdb)

   U_INTERNAL_ASSERT_POINTER(internal.ptr_cdb)

   UCDB::cdb_record_header* s_hr = (UCDB::cdb_record_header*)src;
   UCDB::cdb_record_header* d_hr = internal.ptr_cdb->hr;

   U_INTERNAL_DUMP("src = { %#.*S %#.*S }", u_get_unaligned(s_hr->klen), src + sizeof(UCDB::cdb_record_header),
                                            u_get_unaligned(s_hr->dlen), src + sizeof(UCDB::cdb_record_header) +
                                            u_get_unaligned(s_hr->klen))

   uint32_t sz = sizeof(UCDB::cdb_record_header) + u_get_unaligned(s_hr->klen) + u_get_unaligned(s_hr->dlen);

   U__MEMCPY((char*)d_hr, (char*)s_hr, sz);

   U_INTERNAL_DUMP("hr(%p) = { %u, %u }", d_hr, u_get_unaligned(d_hr->klen), u_get_unaligned(d_hr->dlen))

   internal.ptr_cdb->hr = (UCDB::cdb_record_header*)((char*)d_hr + sz);

   internal.ptr_cdb->nrecord++;
}

uint32_t UCDB::makeFinish(bool _reset)
{
   U_TRACE(1+256, "UCDB::makeFinish(%b)", _reset)

   U_INTERNAL_ASSERT_DIFFERS(UFile::map, MAP_FAILED)

   // Each of the CDB_NUM_HASH_TABLE_POINTER initial pointers states a position and a length.
   // The position is the starting byte position of the hash table.
   // The length is the number of slots in the hash table

   char* eod = (char*)hr; // END OF DATA (eod) -> start of hash table slot...

   start_hash_table_slot = eod - UFile::map;

   uint32_t pos = start_hash_table_slot;

   U_INTERNAL_DUMP("nrecord = %u", nrecord)

   if (nrecord > 0)
      {
      uint32_t klen; // key length

      struct cdb_tmp {
         uint32_t hash;
         uint32_t pos;
         uint32_t index;
      };

      cdb_tmp*  tmp = U_MALLOC_N(nrecord, cdb_tmp);
      cdb_tmp* ptmp = tmp;

      cdb_hash_table_slot* pslot;

      hp = (cdb_hash_table_pointer*) UFile::map;

      (void) U_SYSCALL(memset, "%p,%d,%u", hp, 0, CDB_NUM_HASH_TABLE_POINTER * sizeof(cdb_hash_table_pointer));

      for (char* ptr = start(); ptr < eod; ++ptmp)
         {
         ptmp->pos = (ptr - UFile::map);

         hr = (cdb_record_header*)ptr;

         ptr += sizeof(UCDB::cdb_record_header);

         // The hash value modulo CDB_NUM_HASH_TABLE_POINTER is the number of a hash table

         klen        = u_get_unaligned(hr->klen);
         ptmp->hash  = cdb_hash(ptr, klen);
         ptmp->index = ptmp->hash % CDB_NUM_HASH_TABLE_POINTER;

         /*
         U_INTERNAL_DUMP("pos   = %u", ptmp->pos)
         U_INTERNAL_DUMP("index = %u", ptmp->index)
         */

         U_INTERNAL_ASSERT_MINOR(ptmp->index, CDB_NUM_HASH_TABLE_POINTER)

         hp[ptmp->index].slots++;

         ptr += klen + u_get_unaligned(hr->dlen);
         }

      uint32_t i;

      for (i = 0; i < CDB_NUM_HASH_TABLE_POINTER; ++i)
         {
         hp[i].pos = pos;

         pos += hp[i].slots * sizeof(cdb_hash_table_slot);

         /*
#     ifdef DEBUG
         if (hp[i].slots) U_INTERNAL_DUMP("hp[%3d] = { %u, %u }", i, hp[i].pos, hp[i].slots)
#     endif
         */

         U_INTERNAL_ASSERT(pos <= (uint32_t)st_size)
         }

      U_INTERNAL_DUMP("nrecord = %u num_hash_slot = %u", nrecord, (pos - start_hash_table_slot) / sizeof(cdb_hash_table_slot))

      if (_reset) (void) U_SYSCALL(memset, "%p,%d,%u", eod, 0, pos - start_hash_table_slot);

      for (i = 0; i < nrecord; ++i)
         {
         slot = (cdb_hash_table_slot*)(UFile::map + hp[tmp[i].index].pos);

         /*
         U_INTERNAL_DUMP("slot = %u", (char*)slot - UFile::map)
         */

         // The hash value divided by CDB_NUM_HASH_TABLE_POINTER, modulo the length of that table, is a slot number

         nslot = (tmp[i].hash / CDB_NUM_HASH_TABLE_POINTER) % hp[tmp[i].index].slots;

         /*
         U_INTERNAL_DUMP("nslot = %u", nslot)
         */

         // handles repeated keys...

         while (true)
            {
            pslot = slot + nslot;

            if (u_get_unaligned(pslot->pos) == 0) break;

            if (++nslot == hp[tmp[i].index].slots) nslot = 0;
            }

         u_put_unaligned(tmp[i].hash, pslot->hash);
         u_put_unaligned(tmp[i].pos,  pslot->pos);

         /*
         U_INTERNAL_DUMP("slot[%u] = { %u, %u }", nslot, tmp[i].hash, tmp[i].pos)
         */
         }

      U_FREE_N(tmp, nrecord, cdb_tmp);

      U_ASSERT(invariant())
      }

   U_RETURN(pos);
}

// Call function for all entry

void UCDB::callForAllEntry(vPFpc function)
{
   U_TRACE(0, "UCDB::callForAllEntry(%p)", function)

   U_INTERNAL_DUMP("nrecord = %u", nrecord)

   U_INTERNAL_ASSERT_MAJOR(UFile::st_size,0)
   U_INTERNAL_ASSERT_DIFFERS(UFile::map, MAP_FAILED)

   char* ptr  = start();
   char* _end = UCDB::end();

   U_INTERNAL_DUMP("ptr = %p end = %p", ptr, _end)

   U_INTERNAL_ASSERT_MINOR(ptr,_end)

   while (ptr < _end)
      {
      hr = (UCDB::cdb_record_header*) ptr;

      U_INTERNAL_DUMP("hr(%p) = { %u, %u }", hr, u_get_unaligned(hr->klen), u_get_unaligned(hr->dlen))
      U_INTERNAL_DUMP("key = %.*S", u_get_unaligned(hr->klen), ptr+sizeof(UCDB::cdb_record_header))

      function(ptr);

      ptr += sizeof(UCDB::cdb_record_header) + u_get_unaligned(hr->klen) + u_get_unaligned(hr->dlen);
      }
}

void UCDB::callForAllEntryExt(vPFpc function)
{
   U_TRACE(0, "UCDB::callForAllEntryExt(%p)", function)

   U_INTERNAL_DUMP("nrecord = %u", nrecord)

   U_INTERNAL_ASSERT_MAJOR(UFile::st_size,0)
   U_INTERNAL_ASSERT_DIFFERS(UFile::map, MAP_FAILED)

   char* ptr;
   char* _eof = UFile::map + (ptrdiff_t)UFile::st_size;
   slot       = (UCDB::cdb_hash_table_slot*) UCDB::end();

   uint32_t pos;

   while ((char*)slot < _eof)
      {
      pos = u_get_unaligned(slot->pos);

      if (pos)
         {
         khash = u_get_unaligned(slot->hash);

         ptr = UFile::map + pos;
         hr  = (UCDB::cdb_record_header*) ptr;

         U_INTERNAL_DUMP("hr(%p) = { %u, %u }", hr, u_get_unaligned(hr->klen), u_get_unaligned(hr->dlen))

         key.dptr  = ptr + sizeof(UCDB::cdb_record_header);
         key.dsize = u_get_unaligned(hr->klen);

         U_INTERNAL_DUMP("key = %.*S", key.dsize, key.dptr)

         function(ptr);

         if (internal.stop_call_for_all_entry)
            {
            internal.stop_call_for_all_entry = false;

            return;
            }
         }

      ++slot;
      }
}

void UCDB::getKeys(UVector<UString>& vec)
{
   U_TRACE(0, "UCDB::getKeys(%p)", &vec)

   U_INTERNAL_DUMP("nrecord = %u", nrecord)

   UStringRep* rep;
   char* ptr  = start();
   char* _end = UCDB::end();

   U_INTERNAL_DUMP("ptr = %p end = %p", ptr, _end)

   U_INTERNAL_ASSERT_MINOR(ptr,_end)

   uint32_t klen; // key length

   while (ptr < _end)
      {
      hr = (UCDB::cdb_record_header*) ptr;

      klen = u_get_unaligned(hr->klen);

      U_INTERNAL_DUMP("hr(%p) = { %u, %u }", hr, klen, u_get_unaligned(hr->dlen))
      U_INTERNAL_DUMP("key = %.*S",  klen, ptr+sizeof(UCDB::cdb_record_header))

      ptr += sizeof(UCDB::cdb_record_header);

      rep = UStringRep::create(ptr, klen, 0U);

      vec.UVector<void*>::push(rep);

      ptr += klen + u_get_unaligned(hr->dlen);
      }
}

void UCDB::_callForAllEntrySorted()
{
   U_TRACE(1, "UCDB::_callForAllEntrySorted()")

   U_INTERNAL_DUMP("ptr_cdb = %p", internal.ptr_cdb)

   U_INTERNAL_ASSERT_POINTER(internal.ptr_cdb)
   U_INTERNAL_ASSERT_EQUALS(internal.ptr_pattern,0)
   U_INTERNAL_ASSERT_POINTER(internal.function_to_call)
   U_INTERNAL_ASSERT_MAJOR(internal.ptr_cdb->UFile::st_size,0)
   U_INTERNAL_ASSERT_DIFFERS(internal.ptr_cdb->UFile::map, MAP_FAILED)

   uint32_t n = internal.ptr_cdb->size();

   U_INTERNAL_DUMP("n = %u", n)

   UVector<UString> vkey(n);

   internal.ptr_cdb->getKeys(vkey);

   U_ASSERT_EQUALS(n, vkey.size())

   if (n > 1) vkey.sort(internal.ptr_cdb->ignore_case);

   UStringRep* r;

   for (uint32_t i = 0; i < n; ++i)
      {
      r = vkey.UVector<UStringRep*>::at(i);

      internal.ptr_cdb->setKey(r);

      internal.ptr_cdb->cdb_hash();

      if (internal.ptr_cdb->find())
         {
         call((const char*)internal.ptr_cdb->key.dptr,  internal.ptr_cdb->key.dsize,
              (const char*)internal.ptr_cdb->data.dptr, internal.ptr_cdb->data.dsize);

         if (internal.stop_call_for_all_entry) goto fine;
         }
      }

fine:
   (void) U_SYSCALL(memset, "%p,%d,%u", &internal, 0, sizeof(UCDB::cdb_internal));
}

void UCDB::call(const char*  key_ptr, uint32_t  key_size,
                const char* data_ptr, uint32_t data_size)
{
   U_TRACE(0, "UCDB::call(%.*S,%u,%.*S,%u)",  key_size,  key_ptr,  key_size,
                                             data_size, data_ptr, data_size)

   U_INTERNAL_ASSERT_POINTER(internal.function_to_call)

#ifdef DEBUG
   UObjectDB::flag_ulib_object = true;
#endif

   UStringRep* skey  = UStringRep::create( key_ptr,  key_size, 0U);
   UStringRep* sdata = UStringRep::create(data_ptr, data_size, 0U);

#ifdef DEBUG
   UObjectDB::flag_ulib_object = false;
#endif

   U_INTERNAL_DUMP("skey = %#.*S sdata = %#.*S)", U_STRING_TO_TRACE(*skey), U_STRING_TO_TRACE(*sdata))

   internal.function_to_call(skey, sdata);

   if (internal.add_entry_to_vector)
      {
      U_INTERNAL_ASSERT_POINTER(internal.ptr_vector)

      internal.ptr_vector->push_back(skey);
      internal.ptr_vector->push_back(sdata);

      internal.add_entry_to_vector = false;
      }
   else
      {
       skey->release();
      sdata->release();
      }
}

void UCDB::call(char* src)
{
   U_TRACE(0, "UCDB::call(%p)", src)

   UCDB::cdb_record_header* ptr_hr = (UCDB::cdb_record_header*)src;

   uint32_t klen = u_get_unaligned(ptr_hr->klen); // key length

   U_INTERNAL_DUMP("hr(%p) = { %u, %u }", ptr_hr, klen, u_get_unaligned(ptr_hr->dlen))

   src += sizeof(UCDB::cdb_record_header);

   call(src,        klen,
        src + klen, u_get_unaligned(ptr_hr->dlen));
}

void UCDB::_callForAllEntry(UString* pattern)
{
   U_TRACE(1, "UCDB::_callForAllEntry(%p)", pattern)

   U_INTERNAL_DUMP("ptr_cdb = %p", internal.ptr_cdb)

   U_INTERNAL_ASSERT_POINTER(internal.ptr_cdb)
   U_INTERNAL_ASSERT_EQUALS(internal.ptr_pattern,0)
   U_INTERNAL_ASSERT_POINTER(internal.function_to_call)
   U_INTERNAL_ASSERT_MAJOR(internal.ptr_cdb->UFile::st_size,0)
   U_INTERNAL_ASSERT_DIFFERS(internal.ptr_cdb->UFile::map, MAP_FAILED)

   char* tmp;
   char* pattern_data = 0;
   uint32_t pattern_size = 0;

   char* ptr  = internal.ptr_cdb->start();
   char* _end = internal.ptr_cdb->end();

   U_INTERNAL_DUMP("ptr = %p _end = %p", ptr, _end)

   U_INTERNAL_ASSERT_MINOR(ptr,_end)

   if (pattern)
      {
      pattern_data = pattern->data();
      pattern_size = pattern->size();

      tmp = ptr + sizeof(UCDB::cdb_record_header);

      internal.ptr_pattern = (char*) u_find(tmp, _end - tmp, pattern_data, pattern_size);

      if (internal.ptr_pattern == 0) goto fine;
      }

   uint32_t klen; //  key length
   uint32_t dlen; // data length

   while (true)
      {
      klen = u_get_unaligned(((UCDB::cdb_record_header*)ptr)->klen); //  key length
      dlen = u_get_unaligned(((UCDB::cdb_record_header*)ptr)->dlen); // data length

      U_INTERNAL_DUMP("hr(%p) = { %u, %u }", ptr, klen, dlen)

      tmp = ptr + sizeof(UCDB::cdb_record_header) + klen + dlen;

      if (pattern &&
          tmp <= internal.ptr_pattern) goto end_loop;

      call(ptr + sizeof(UCDB::cdb_record_header), klen, tmp - dlen, dlen);

      if (tmp >= _end ||
          internal.stop_call_for_all_entry) break;

      if (pattern)
         {
         ptr = tmp + sizeof(UCDB::cdb_record_header);

         internal.ptr_pattern = (char*) u_find(ptr, _end - ptr, pattern_data, pattern_size);

         if (internal.ptr_pattern == 0) break;

         U_INTERNAL_ASSERT_MINOR(internal.ptr_pattern,_end)
         }

end_loop:
      ptr = tmp;
      }

fine:
   U_INTERNAL_ASSERT(tmp <= _end)

   (void) U_SYSCALL(memset, "%p,%d,%u", &internal, 0, sizeof(UCDB::cdb_internal));
}

uint32_t UCDB::getValuesWithKeyNask(UVector<UString>& vec_values, const UString& mask_key, uint32_t* _size)
{
   U_TRACE(0, "UCDB::getValuesWithKeyNask(%p,%.*S,%p)", &vec_values, U_STRING_TO_TRACE(mask_key), _size)

   U_INTERNAL_ASSERT_MAJOR(UFile::st_size,0)
   U_INTERNAL_ASSERT_DIFFERS(UFile::map, MAP_FAILED)

   char* tmp;
   UStringRep* r;

   char* ptr  = start();
   char* _end = UCDB::end();

   U_INTERNAL_DUMP("ptr = %p end = %p", ptr, _end)

   U_INTERNAL_ASSERT_MINOR(ptr,_end)

   char* mask_data    = mask_key.data();
   uint32_t mask_size = mask_key.size(), n = vec_values.size();

   uint32_t klen; //  key length
   uint32_t dlen; // data length

   if (ignore_case) u_pfn_flags |= FNM_CASEFOLD;

   if (_size) *_size = 0;

   while (true)
      {
      klen = u_get_unaligned(((UCDB::cdb_record_header*)ptr)->klen); //  key length
      dlen = u_get_unaligned(((UCDB::cdb_record_header*)ptr)->dlen); // data length

      U_INTERNAL_DUMP("hr(%p) = { %u, %u }", ptr, klen, dlen)

      tmp = ptr + sizeof(UCDB::cdb_record_header) + klen + dlen;

      if (u_pfn_match(ptr + sizeof(UCDB::cdb_record_header), klen, mask_data, mask_size, u_pfn_flags))
         {
         U_INTERNAL_DUMP("key = %#.*S data = %#.*S)", klen, ptr + sizeof(UCDB::cdb_record_header), dlen, tmp - dlen)

         r = UStringRep::create(tmp - dlen, dlen, 0U);

         if (_size) *_size += dlen;

         vec_values.UVector<void*>::push(r);
         }

      if (tmp >= _end) break;

      ptr = tmp;
      }

   U_INTERNAL_ASSERT(tmp <= _end)

   n = vec_values.size() - n;

   U_RETURN(n);
}

// PRINT DATABASE

void UCDB::print(char* src)
{
   U_TRACE(0, "UCDB::print(%p)", src)

   U_INTERNAL_ASSERT_POINTER(pbuffer)

   UCDB::cdb_record_header* ptr_hr = (UCDB::cdb_record_header*)src;

   uint32_t klen = u_get_unaligned(ptr_hr->klen); //  key length
   uint32_t dlen = u_get_unaligned(ptr_hr->dlen); // data length

   char tmp[40];
   uint32_t size = u__snprintf(tmp, sizeof(tmp), "+%u,%u:", klen, dlen);

   pbuffer->append(tmp, size);
   pbuffer->append(src + sizeof(UCDB::cdb_record_header), klen);
   pbuffer->append(U_CONSTANT_TO_PARAM("->"));
   pbuffer->append(src + sizeof(UCDB::cdb_record_header) + klen, dlen);
   pbuffer->push_back('\n');
}

UString UCDB::print()
{
   U_TRACE(0, "UCDB::print()")

   if (UFile::st_size)
      {
      UString buffer(UFile::st_size);

      pbuffer = &buffer;

      callForAllEntry(&UCDB::print);

      U_RETURN_STRING(buffer);
      }

   U_RETURN_STRING(UString::getStringNull());
}

// Save memory hash table as constant database

bool UCDB::writeTo(UCDB& cdb, UHashMap<void*>* table, pvPFpvpb func)
{
   U_TRACE(1, "UCDB::writeTo(%p,%p,%p)", &cdb, table, func)

   cdb.nrecord = (func ? 0
                       : table->size());
   bool result = cdb.creat(O_RDWR) &&
                 cdb.ftruncate(sizeFor(cdb.nrecord) + table->space());

   if (result)
      {
      result = cdb.memmap(PROT_READ | PROT_WRITE);

      if (result == false) U_RETURN(false);

      bool bdelete;
      UStringRep* _key;
      UStringRep* value;
      char* ptr = cdb.start(); // init of DATA
      UCDB::cdb_record_header* _hr;

      U_INTERNAL_DUMP("table->_length = %u", table->_length)

      UHashMapNode* node;
      UHashMapNode** pnode;
      UHashMapNode** tbl = table->table;

      uint32_t klen; //  key length
      uint32_t dlen; // data length

      for (uint32_t index = 0, capacity = table->_capacity; index < capacity; ++index)
         {
         if (tbl[index])
            {
            node  = tbl[index];
            pnode = tbl + index;

            do {
               if (func == 0) value = (UStringRep*) node->elem;
               else
                  {
                  value = (UStringRep*) func(node->elem, &bdelete);

                  U_INTERNAL_DUMP("bdelete = %b", bdelete)

                  if (bdelete) // ask for to delete node of table...
                     {
                     *pnode = node->next; // lo si toglie dalla lista collisioni...

                     /* va fatto nella func...
                     -------------------------
                     elem = (T*) node->elem;
                     u_destroy<T>(elem);
                     -------------------------
                     */

                     delete node;

                     table->_length--;
                     }
                  }

               if (value)
                  {
                  _key = node->key;

                  klen =  _key->size(); //  key length
                  dlen = value->size(); // data length

                  _hr = (UCDB::cdb_record_header*) ptr;

                  u_put_unaligned(klen, _hr->klen);
                  u_put_unaligned(dlen, _hr->dlen);

                  U_INTERNAL_DUMP("hr = { %u, %u }", klen, dlen)

                  ptr += sizeof(UCDB::cdb_record_header);

                  U__MEMCPY(ptr, _key->data(), klen);

                  U_INTERNAL_DUMP("key = %.*S", klen, ptr)

                  ptr += klen;

                  U__MEMCPY(ptr, value->data(), dlen);

                  U_INTERNAL_DUMP("data = %.*S", dlen, ptr)

                  ptr += dlen;

                  if (func)
                     {
                     cdb.nrecord++;

                     value->release();
                     }
                  }

               // check if asked to delete node of the table...

               if (func    == 0 ||
                   bdelete == false) pnode = &(*pnode)->next;
               }
            while ((node = *pnode));
            }
         }

      U_INTERNAL_DUMP("table->_length = %u", table->_length)

      cdb.hr = (UCDB::cdb_record_header*) ptr; // end of DATA

      uint32_t pos = cdb.makeFinish(true);

      U_INTERNAL_ASSERT(pos <= (uint32_t)cdb.st_size)

      if (pos < (uint32_t)cdb.st_size)
         {
                  cdb.munmap();
         result = cdb.ftruncate(pos);
         }

      cdb.UFile::close();
      }

   U_RETURN(result);
}

// STREAM

U_EXPORT istream& operator>>(istream& is, UCDB& cdb)
{
   U_TRACE(0+256, "UCDB::operator>>(%p,%p)", &is, &cdb)

   cdb.makeStart();

   char c;
   char* ptr = (char*) cdb.hr; // init of DATA

   uint32_t klen; //  key length
   uint32_t dlen; // data length

   UCDB::cdb_record_header* hr;

   while (is >> c)
      {
      U_INTERNAL_DUMP("c = %C", c)

      if (c == '#')
         {
         for (int ch = is.get(); (ch != '\n' && ch != EOF); ch = is.get()) {}

         continue;
         }

      if (c != '+') break;

      hr = (UCDB::cdb_record_header*)ptr;

      is >> klen;
      is.get(); // skip ','
      is >> dlen;
      is.get(); // skip ':'

      U_INTERNAL_DUMP("hr = { %u, %u }", klen, dlen)

      u_put_unaligned(klen, hr->klen);
      u_put_unaligned(dlen, hr->dlen);

      ptr += sizeof(UCDB::cdb_record_header);

      is.read(ptr, klen);

      U_INTERNAL_DUMP("key = %.*S", klen, ptr)

      is.get(); // skip '-'
      is.get(); // skip '>'

      ptr += klen;

      is.read(ptr, dlen);

      U_INTERNAL_DUMP("data = %.*S", dlen, ptr)

      ptr += dlen;

      cdb.nrecord++;

      is.get(); // skip '\n'
      }

   cdb.hr = (UCDB::cdb_record_header*) ptr; // end of DATA

   uint32_t pos = cdb.makeFinish(true);

          cdb.munmap();
   (void) cdb.ftruncate(pos);

   return is;
}

U_EXPORT ostream& operator<<(ostream& os, UCDB& cdb)
{
   U_TRACE(0+256, "UCDB::operator<<(%p,%p)", &os, &cdb)

   UString text = cdb.print();

   (void) os.write(text.data(), text.size());

   os.put('\n');

   return os;
}

// DEBUG

#if defined(DEBUG) || defined(U_TEST)

U_NO_EXPORT void UCDB::checkAllEntry(UStringRep* key, UStringRep* value)
{
   U_TRACE(0, "UCDB::checkAllEntry(%p,%p)", key, value)

   U_INTERNAL_DUMP("ptr_cdb = %p", internal.ptr_cdb)

   U_INTERNAL_ASSERT_POINTER(internal.ptr_cdb)

   UString x(value);

   if ((*internal.ptr_cdb)[key] != x)
      {
      while (internal.ptr_cdb->findNext())
         {
         if (internal.ptr_cdb->elem() == x) return;
         }

      U_INTERNAL_ASSERT(false)
      }
}

bool UCDB::invariant()
{
   U_TRACE(0, "UCDB::invariant()")

   callForAllEntry(checkAllEntry);

   return true;
}

#endif

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UCDB::dump(bool _reset) const
{
   UFile::dump(false);

   *UObjectIO::os << '\n'
                  << "hp                        " << (void*)hp      << '\n'
                  << "hr                        " << (void*)hr      << '\n'
                  << "key                       " << "{ "           << key.dptr
                                                  << ' '            << key.dsize
                                                                    << " }\n"
                  << "data                      " << "{ "           << data.dptr
                                                  << ' '            << data.dsize
                                                                    << " }\n"
                  << "slot                      " << (void*)slot    << '\n'
                  << "loop                      " << loop           << '\n'
                  << "nslot                     " << nslot          << '\n'
                  << "khash                     " << khash          << '\n'
                  << "offset                    " << offset         << '\n'
                  << "nrecord                   " << nrecord        << '\n'
                  << "ignore_case               " << ignore_case    << '\n'
                  << "start_hash_table_slot     " << start_hash_table_slot;

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
