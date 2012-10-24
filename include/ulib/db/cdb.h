// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    cdb.h - A structure for constant databases (Bernstein)
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_CDB_H
#define ULIB_CDB_H 1

#include <ulib/file.h>
#include <ulib/container/hash_map.h>

/**
   @class UCDB

   @brief UCDB is a fast, reliable, simple class for creating and reading constant databases.

   Its database structure provides several features:
   Fast lookups: A successful lookup in a large database normally takes just two disk accesses.
   An unsuccessful lookup takes only one.
   Low overhead: A database uses 4096 bytes, plus 16 bytes per record, plus the space for keys and data.
   A cdb is an associative array: it maps strings (keys) to strings (data).
   A cdb contains 512 pointers to linearly probed open hash tables.
   The hash tables contain pointers to (key,data) pairs. A cdb is stored in a single file on disk:
   +----------------+------------+-------+-------+-----+---------+
   | p0 p1 ... p511 | records... | hash0 | hash1 | ... | hash511 |
   +----------------+------------+-------+-------+-----+---------+
   Each of the 512 initial pointers states a position and a length. The position is the starting byte
   position of the hash table. The length is the number of slots in the hash table.
   Records are stored sequentially, without special alignment. A record states a key length, a data
   length, the key, and the data. Each hash table slot states a hash value and a byte position. If the
   byte position is 0, the slot is empty. Otherwise, the slot points to a record whose key has that hash value.
   Positions, lengths, and hash values are 32-bit quantities, stored in 4 bytes. Thus a cdb must fit into 4 gigabytes.
   A record is located as follows. Compute the hash value of the key in the record.
   The hash value modulo 512 is the number of a hash table.
   The hash value divided by 512, modulo the length of that table, is a slot number.
   Probe that slot, the next higher slot, and so on, until you find the record or run into an empty slot.
*/

#define CDB_NUM_HASH_TABLE_POINTER 512

class URDB;

typedef void  (*vPFpc)(char*);
typedef void* (*pvPFpvpb)(void*,bool*);
typedef void  (*vPFprpr)(UStringRep*, UStringRep*);

class U_EXPORT UCDB : public UFile {
public:

   typedef struct datum {
      void* dptr;
      uint32_t dsize;
   } datum;

   typedef struct cdb_hash_table_pointer {
      uint32_t pos;   // starting byte position of the hash table
      uint32_t slots; //        number of slots in the hash table
   } cdb_hash_table_pointer;

   typedef struct cdb_record_header {
      uint32_t klen; //  key length
      uint32_t dlen; // data length
   } cdb_record_header;

   typedef struct cdb_hash_table_slot {
      uint32_t hash; // hash value of the key
      uint32_t pos;  // starting byte position of the record (0 -> slot empty)
   } cdb_hash_table_slot;

   // COSTRUTTORI

   UCDB(bool _ignore_case)
      {
      U_TRACE_REGISTER_OBJECT(0, UCDB, "%b", _ignore_case)

      ignore_case = _ignore_case;
      }

   UCDB(const UString& path, bool _ignore_case) : UFile(path)
      {
      U_TRACE_REGISTER_OBJECT(0, UCDB, "%.*S,%b", U_STRING_TO_TRACE(path), _ignore_case)

      ignore_case = _ignore_case;
      }

   ~UCDB()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UCDB)
      }

   // Open a Constant DataBase

   bool open(bool brdonly = true);

   bool open(const UString& pathdb, bool brdonly = true)
      {
      U_TRACE(0, "UCDB::open(%.*S)", U_STRING_TO_TRACE(pathdb))

      UFile::setPath(pathdb);

      return UCDB::open(brdonly);
      }

   // Ricerche

   void setKey(     UStringRep* _key)       {  key.dptr = (void*) _key->data(); key.dsize =  _key->size(); }
   void setKey( const UString&  _key)       {  key.dptr = (void*) _key.data();  key.dsize =  _key.size(); }
   void setData(const UString& _data)       { data.dptr = (void*)_data.data(); data.dsize = _data.size(); }
   void setData(void* dptr, uint32_t dsize) { data.dptr = dptr;                data.dsize = dsize; }

   bool find(const UString& _key)
      {
      U_TRACE(0, "UCDB::find(%.*S)", U_STRING_TO_TRACE(_key))

      setKey(_key);

      cdb_hash();

      return find();
      }

   bool findNext(); // handles repeated keys...

   // Get methods

   uint32_t size() const
      {
      U_TRACE(0, "UCDB::size()")

      U_RETURN(nrecord);
      }

   UString elem()
      {
      U_TRACE(0, "UCDB::elem()")

      UString str((const char*)data.dptr, data.dsize);

      U_RETURN_STRING(str);
      }

   bool ignoreCase() const { return ignore_case; }

   // operator []

   UString operator[](const UString& _key)
      {
      U_TRACE(0, "UCDB::operator[](%.*S)", U_STRING_TO_TRACE(_key))

      setKey(_key);

      return at();
      }

   UString operator[](UStringRep* _key)
      {
      U_TRACE(0, "UCDB::operator[](%.*S)", U_STRING_TO_TRACE(*_key))

      setKey(_key);

      return at();
      }

   // hashing function

   uint32_t cdb_hash(const char* t, uint32_t tlen)
      {
      U_TRACE(0, "UCDB::cdb_hash(%.*S,%u)", tlen, t, tlen)

      uint32_t h = u_cdb_hash((unsigned char*)t, tlen, ignore_case);

      U_RETURN(h);
      }

   // Call function for all entry

          void setCDB()                         { internal.ptr_cdb = this; }
   static void setCDB(UCDB* ptr)                { internal.ptr_cdb = ptr; }
   static void setVector(UVector<UString>* ptr) { internal.ptr_vector = ptr; }

   static vPFprpr getFunctionToCall()          { return internal.function_to_call; }
   static void    setFunctionToCall(vPFprpr f) {        internal.function_to_call = f; }

   void callForAllEntry(vPFprpr function, UString* pattern = 0)
      { internal.ptr_cdb = this; internal.function_to_call = function; _callForAllEntry(pattern); }

   static void addEntryToVector()    { internal.add_entry_to_vector     = true; }
   static void stopCallForAllEntry() { internal.stop_call_for_all_entry = true; }

   static char* getPtrPattern() { return internal.ptr_pattern; }

   uint32_t getValuesWithKeyNask(UVector<UString>& vec_values, const UString& mask_key, uint32_t* size = 0);

   void getKeys(UVector<UString>& vec);

   void callForAllEntrySorted(vPFprpr function)
      { internal.ptr_cdb = this; internal.function_to_call = function; _callForAllEntrySorted(); }

   // Save memory hash table as Constant DataBase

   static uint32_t sizeFor(uint32_t _nrecord)
      {
      U_TRACE(0, "UCDB::sizeFor(%u)", _nrecord)

      uint32_t size = CDB_NUM_HASH_TABLE_POINTER * sizeof(cdb_hash_table_pointer) +
                      _nrecord * (sizeof(cdb_record_header) + sizeof(cdb_hash_table_slot));

      U_RETURN(size);
      }

   bool writeTo(UHashMap<void*>* t, pvPFpvpb f = 0) { return UCDB::writeTo(*this, t, f); }

   static bool writeTo(const UString& path, UHashMap<void*>* t, pvPFpvpb f = 0)
         { return UCDB(path, t->ignoreCase()).writeTo(t, f); }

   // STREAM

   UString print();

   friend U_EXPORT istream& operator>>(istream& is, UCDB& cdb);
   friend U_EXPORT ostream& operator<<(ostream& os, UCDB& cdb);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif
#if defined(DEBUG) || defined(U_TEST)
   bool invariant();
#endif

protected:
   uint32_t loop,              // number of hash slots searched under key
            nslot,             // initialized in find()
            khash,             // initialized in find()
            nrecord,           // initialized in makeStart()
            offset,
            start_hash_table_slot;

   datum key;                  // initialized in find()
   datum data;                 // initialized if findNext() returns 1
   cdb_record_header* hr;      // initialized if findNext() returns 1
   cdb_hash_table_slot* slot;  // initialized in find()
   cdb_hash_table_pointer* hp; // initialized in find()

   // when mmap not available we use this storage...

   cdb_hash_table_pointer hp_buf;
   cdb_record_header      hr_buf;
   cdb_hash_table_slot  slot_buf;

   bool ignore_case;

   typedef struct cdb_internal {
      UCDB* ptr_cdb;
      char* ptr_pattern;
      vPFprpr function_to_call;
      UVector<UString>* ptr_vector;
      bool stop_call_for_all_entry, add_entry_to_vector;
   } cdb_internal;

   static UString* pbuffer;
   static cdb_internal internal;

   bool find();
   UString at();

   void cdb_hash() { khash = cdb_hash((const char*)key.dptr, key.dsize); }

   // START-END of record data

   char* start() const { return (UFile::map + CDB_NUM_HASH_TABLE_POINTER * sizeof(cdb_hash_table_pointer)); }
   char*   end() const { return (UFile::map + start_hash_table_slot); }

   // Call function for all entry

   static void print(char* src);

          void  callForAllEntry(vPFpc function);
   static void _callForAllEntry(UString* pattern);
   static void _callForAllEntrySorted();

   // Save memory hash table as Constant DataBase

   static bool writeTo(UCDB& cdb, UHashMap<void*>* table, pvPFpvpb f = 0);

   // FOR RDB

   void makeStart()
      {
      U_TRACE(0, "UCDB::makeStart()")

      U_INTERNAL_ASSERT_DIFFERS(map, MAP_FAILED)

      nrecord = start_hash_table_slot = 0;

      hr = (UCDB::cdb_record_header*) start();
      }

   static void     call(char* src);
   static void     makeAdd(char* src);
          uint32_t makeFinish(bool reset);
          void     callForAllEntryExt(vPFpc function);
   static void     makeAdd(const datum& _key, const datum& _data);
   static void     call(const char*  key_ptr, uint32_t  key_size,
                        const char* data_ptr, uint32_t data_size);

private:
   UCDB(const UCDB&) : UFile()  {}
   UCDB& operator=(const UCDB&) { return *this; }

   inline bool match(uint32_t pos) U_NO_EXPORT;

#if defined(DEBUG) || defined(U_TEST)
   static void checkAllEntry(UStringRep* key, UStringRep* value) U_NO_EXPORT;
#endif

   friend class URDB;
};

#endif
