// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    rdb.h - A Reliable DataBase library (Felix von Leitner)
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_RDB_H
#define ULIB_RDB_H 1

#include <ulib/db/cdb.h>
#include <ulib/utility/lock.h>

/**
   @class URDB

   @brief URDB is a fast, Reliable, simple class for creating and reading DataBases

   The idea behind URDB is to take UCDB and put a journal over it.
   Then provide an abstraction layer that looks like ndbm and writes updates to the journal.
   Read operations are answered by consulting the cache (build with journal) and the cdb file.
   The result should be a reasonably small yet crash-proof read-write database
*/

class URDBServer;
class URDBClient_Base;
class URDBClientImage;

// The interface is very similar to the gdbm one

#  define CACHE_HASHTAB_LEN 769

#  define RDB_off      ((URDB::cache_struct*)journal.map)->off
#  define RDB_capacity (journal.st_size - RDB_off)
#  define RDB_eof      (journal.map+(ptrdiff_t)journal.st_size)
#  define RDB_allocate (journal.map+(ptrdiff_t)RDB_off)

#  define RDB_sync      ((URDB::cache_struct*)journal.map)->sync
#  define RDB_nrecord   ((URDB::cache_struct*)journal.map)->nrecord
#  define RDB_reference ((URDB::cache_struct*)journal.map)->reference
#  define RDB_hashtab  (((URDB::cache_struct*)journal.map)->hashtab)

#  define RDB_ptr      (journal.map+sizeof(URDB::cache_struct))
#  define RDB_node     ((URDB::cache_node*)(journal.map+node))

#  define RDB_node_key_pr  u_get_unaligned(RDB_node->key.dptr)
#  define RDB_node_key_sz  u_get_unaligned(RDB_node->key.dsize)
#  define RDB_node_data_pr u_get_unaligned(RDB_node->data.dptr)
#  define RDB_node_data_sz u_get_unaligned(RDB_node->data.dsize)

#  define RDB_node_key     (journal.map+RDB_node_key_pr)
#  define RDB_node_data    (journal.map+RDB_node_data_pr)

#  define RDB_ptr_node(offset)           ((URDB::cache_node*)(ptr_rdb->journal.map+offset))
#  define RDB_cache_node(node,attribute) (u_get_unaligned(node->attribute))

class U_EXPORT URDB : public UCDB {
public:

   // COSTRUTTORI

   URDB(bool _ignore_case) : UCDB(_ignore_case)
      {
      U_TRACE_REGISTER_OBJECT(0, URDB, "%b", _ignore_case)

      node = 0;
      }

   URDB(const UString& pathdb, bool _ignore_case) : UCDB(pathdb, _ignore_case)
      {
      U_TRACE_REGISTER_OBJECT(0, URDB, "%.*S,%b", U_STRING_TO_TRACE(pathdb), _ignore_case)

      node = 0;
      }

   ~URDB()
      {
      U_TRACE_UNREGISTER_OBJECT(0, URDB)
      }

   // Open a Reliable DataBase

   bool open(                       uint32_t log_size = 1024 * 1024, bool btruncate = false, bool brdonly = true);
   bool open(const UString& pathdb, uint32_t log_size = 1024 * 1024, bool btruncate = false, bool brdonly = true)
      {
      U_TRACE(0, "URDB::open(%.*S,%u,%b)", U_STRING_TO_TRACE(pathdb), log_size, btruncate)

      UFile::setPath(pathdb);

      return URDB::open(log_size, btruncate);
      }

   // Close a Reliable DataBase

   void close();

   // Combines the old cdb file and the diffs in a new cdb file.
   // Close the database and deletes the obsolete journal file if everything worked out

   bool closeReorganize();

   void reset();

   uint32_t size() const
      {
      U_TRACE(0, "URDB::size()")

      U_RETURN(UCDB::nrecord + RDB_nrecord);
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

#  define RDB_INSERT  0 // Insertion of new entries only
#  define RDB_REPLACE 1 // Allow replacing existing entries

   int store(                                                                                             int flag);
   int store(   UStringRep* _key, const UString& _data,                                                   int flag);
   int store(const UString& _key, const UString& _data,                                                   int flag);
   int store(const UString& _key, const UString& new_rec, const UString& old_rec, const UString& padding, int flag);

   // ---------------------------------------------------------------------
   // Mark a key/value as deleted
   // ---------------------------------------------------------------------
   // RETURN VALUE
   // ---------------------------------------------------------------------
   //  0: Everything was OK
   // -1: The entry was not in the database
   // -2: The entry was already marked deleted in the hash-tree
   // -3: there is not enough (virtual) memory available on writing journal
   // ---------------------------------------------------------------------

   int remove(const UString& _key);

   // ----------------------------------------------------------------------
   // Substitute a key/value with a new key/value (remove+store)
   // ----------------------------------------------------------------------
   // RETURN VALUE
   // ----------------------------------------------------------------------
   //  0: Everything was OK
   // -1: The entry was not in the database
   // -2: The entry was marked deleted in the hash-tree
   // -3: there is not enough (virtual) memory available on writing journal
   // -4: flag was RDB_INSERT and the new key already existed
   // ----------------------------------------------------------------------

   int substitute(const UString& _key, const UString& new_key, const UString& _data, int flag = RDB_INSERT);

   // Ricerche

   bool fetch();
   bool find(const UString& _key);

   uint32_t getDataSize() const     { return RDB_node_data_sz; }
   void*    getDataPointer() const  { return RDB_node_data; }

   // operator []

   UString operator[](const UString& _key)
      {
      U_TRACE(0, "URDB::operator[](%.*S)", U_STRING_TO_TRACE(_key))

      UCDB::setKey(_key);

      return at();
      }

   UString operator[](UStringRep* _key)
      {
      U_TRACE(0, "URDB::operator[](%.*S)", U_STRING_TO_TRACE(*_key))

      UCDB::setKey(_key);

      return at();
      }

   // flushes changes made to the log file back to disk

   void msync();
   void fsync() { journal.fsync(); }

   // lock

   void setShared(sem_t* ptr = 0)
      {
      U_TRACE(0, "URDB::setShared(%p)", ptr)

      _lock.init(ptr);
      }

   void   lock() { _lock.lock(0); }
   void unlock() { _lock.unlock(); }

   // TRANSACTION

   bool  beginTransaction();
   void  abortTransaction();
   void commitTransaction();

   // Call function for all entry

   void getKeys(UVector<UString>& vec);

   void callForAllEntry(      vPFprpr function, UVector<UString>* vec = 0);
   void callForAllEntrySorted(vPFprpr function, UVector<UString>* vec = 0, qcompare compare_obj = 0);

   // PRINT

   UString print();
   UString printSorted();

   // STREAM

   friend U_EXPORT ostream& operator<<(ostream& os, URDB& rdb);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   ULock _lock;
   UFile journal;

   // ----------------------------------------------------------------------------------------------------------------
   // CACHE for CDB:
   // ----------------------------------------------------------------------------------------------------------------
   // This code implements a chained hash table where we use binary search trees instead of linked lists as chains.
   // Let's call this a hash tree. This code uses blobs, not 0-terminated strings. While these routines can be used
   // as associative hash, they are meant as a cache for a larger, disk-base database or mmap'ed file. And I provide
   // functions to mark a record as deleted, so that this can be used to cache deltas to a constant database like CDB
   // ----------------------------------------------------------------------------------------------------------------
   // NB: offsets relative to the starting address of the mapping should be employed...
   // ----------------------------------------------------------------------------------------------------------------

   typedef struct rdb_datum {
      uint32_t dptr;
      uint32_t dsize;
   } rdb_datum;

   typedef struct rdb_cache_node {
      rdb_datum key;
      rdb_datum data;
      uint32_t left;  // Two cache_node 'pointer' of the binary search tree
      uint32_t right; // behind every entry of the hash table
   } cache_node;

   typedef struct rdb_cache_struct {
      uint32_t off;                        // RDB_off
      uint32_t sync;                       // RDB_sync
      uint32_t nrecord;                    // RDB_nrecord
      uint32_t reference;                  // RDB_reference
      uint32_t hashtab[CACHE_HASHTAB_LEN]; // RDB_hashtab
      // -----> data storage...            // RDB_ptr
   } cache_struct;

   typedef void (*vPFu)(uint32_t offset);

   // Manage shared cache

   UString at();

   int  remove();
   bool isDeleted();
   int  substitute(UCDB::datum* new_key, int flag);

   bool cdbLookup() // NB: valorizza struct UCDB::data..
      {
      U_TRACE(0, "URDB::cdbLookup()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_EQUALS(node, 0)

      bool result = (UFile::st_size && UCDB::find());

      U_RETURN(result);
      }

   // utility - especially created for net interface class

   static char* parseLine(const char* ptr, UCDB::datum* key, UCDB::datum* data);

          char* parseLine(const char* ptr) { return parseLine(ptr, &key, &data); }

private:
   uint32_t   node; // RDB_node
   uint32_t* pnode;

   static URDB* ptr_rdb;
   static UCDB::datum key1;
   static UVector<UString>* kvec;

   void htAlloc() U_NO_EXPORT;       // Alloc one node for the hash tree
   bool htLookup() U_NO_EXPORT;      // Search one key/data pair in the cache
   void htInsert() U_NO_EXPORT;      // Insert one key/data pair in the cache
   void htRemoveAlloc() U_NO_EXPORT; // remove one node allocated for the hash tree

   bool reorganize() U_NO_EXPORT;    // Combines the old cdb file and the diffs in a new cdb file

   bool logJournal(int op) U_NO_EXPORT;
   bool writev(const struct iovec* iov, int n, uint32_t size) U_NO_EXPORT;

   inline void makeAdd() U_NO_EXPORT;
   inline bool resizeJournal(uint32_t oversize) U_NO_EXPORT;
   inline void call(vPFu function1, vPFpc function2) U_NO_EXPORT;

   static inline void call2(char* src) U_NO_EXPORT;
   static inline void print2(char* src) U_NO_EXPORT;
   static inline void makeAdd2(char* src) U_NO_EXPORT;
   static inline void getKeys2(char* src) U_NO_EXPORT;
   static        void call1(uint32_t offset) U_NO_EXPORT;
   static        void print1(uint32_t offset) U_NO_EXPORT;
   static        void makeAdd1(uint32_t offset) U_NO_EXPORT;
   static        void getKeys1(uint32_t offset) U_NO_EXPORT;

   URDB(const URDB& r) : UCDB(r) {}
   URDB& operator=(const URDB&)  { return *this; }

   friend class URDBServer;
   friend class URDBClient_Base;
   friend class URDBClientImage;
};

#endif
