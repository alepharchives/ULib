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
#  define RDB_size     ((URDB::cache_struct*)journal.map)->size
#  define RDB_page     ((URDB::cache_struct*)journal.map)->page
#  define RDB_lrecord  ((URDB::cache_struct*)journal.map)->lrecord
#  define RDB_capacity (RDB_size - RDB_off)

#  define RDB_ptr      (journal.map+sizeof(URDB::cache_struct))
#  define RDB_eof      (journal.map+RDB_size)
#  define RDB_ptr_off  (journal.map+RDB_off)
#  define RDB_ptr_page (journal.map+RDB_page)

#  define RDB_hashtab  (((URDB::cache_struct*)journal.map)->hashtab)

#  define RDB_node     ((URDB::cache_node*)(journal.map+node))

#  define RDB_node_key_pr  u_get_unaligned(RDB_node->key.dptr)
#  define RDB_node_key_sz  u_get_unaligned(RDB_node->key.dsize)
#  define RDB_node_data_pr u_get_unaligned(RDB_node->data.dptr)
#  define RDB_node_data_sz u_get_unaligned(RDB_node->data.dsize)

#  define RDB_node_key     (journal.map+ptr2int(RDB_node_key_pr))
#  define RDB_node_data    (journal.map+ptr2int(RDB_node_data_pr))

#  define RDB_ptr_node(offset) ((URDB::cache_node*)(ptr_rdb->journal.map+offset))

#  define RDB_cache_node(n,name) (u_get_unaligned(n->name))

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

   bool open(uint32_t log_size = 1024 * 1024, int flag = 0);

   bool open(const UString& pathdb, uint32_t log_size = 1024 * 1024, int flag = 0)
      {
      U_TRACE(0, "URDB::open(%.*S,%u,%d)", U_STRING_TO_TRACE(pathdb), log_size, flag)

      UFile::setPath(pathdb);

      return URDB::open(log_size, flag);
      }

   // Close a Reliable DataBase

   void close();

   // Combines the old cdb file and the diffs in a new cdb file.
   // Close the database and deletes the obsolete journal file if everything worked out

   bool closeReorganize()
      {
      U_TRACE(0, "URDB::closeReorganize()")

      if (reorganize())
         {
         URDB::close();

         (void) journal.unlink();

         U_RETURN(true);
         }

      U_RETURN(false);
      }

   void reset();

   uint32_t size() const
      {
      U_TRACE(0, "URDB::size()")

      U_RETURN(UCDB::nrecord + RDB_lrecord);
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

   int store(                                           int flag);
   int store(const UString& _key, const UString& _data, int flag = RDB_INSERT);

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

   // TRANSACTION

   void setShared()
      {
      U_TRACE(0, "URDB::setShared()")

      lock.init(0);
      }

   bool beginTransaction();
   void abortTransaction();
   void commitTransaction();

   // Call function for all entry

   void getKeys(UVector<UString>& vec);

   void callForAllEntry(      vPFprpr function);
   void callForAllEntrySorted(vPFprpr function);

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
   ULock lock;
   UFile journal;

   // CACHE for CDB:
   // ----------------------------------------------------------------------------------------------------------------
   // This code implements a chained hash table where we use binary search trees instead of linked lists as chains.
   // Let's call this a hash tree. This code uses blobs, not 0-terminated strings. While these routines can be used
   // as associative hash, they are meant as a cache for a larger, disk-base database or mmap'ed file. And I provide
   // functions to mark a record as deleted, so that this can be used to cache deltas to a constant database like CDB
   // NB: offsets relative to the starting address of the mapping should be employed...
   // ----------------------------------------------------------------------------------------------------------------

   typedef struct rdb_datum {
      uint32_t dptr;
      uint32_t dsize;
   } rdb_datum;

   typedef struct rdb_cache_node {
      rdb_datum key;
      rdb_datum data;
      uint32_t left;  // Two cache_node* of the binary search tree
      uint32_t right; // behind every entry of the hash table
   } cache_node;

   typedef struct rdb_cache_struct {
      uint32_t off;
      uint32_t size;
      uint32_t page;
       int32_t lrecord;
      uint32_t hashtab[CACHE_HASHTAB_LEN];
      // -----> unnamed array of char...
   } cache_struct;

   // Manage shared cache

   typedef void (*vPFu)(uint32_t offset);

   bool isActive() const
      {
      U_TRACE(0, "URDB::isActive()")

      U_CHECK_MEMORY

      U_INTERNAL_DUMP("RDB_off = %u", RDB_off)

      bool result = (RDB_off > sizeof(URDB::cache_struct));

      U_RETURN(result);
      }

   bool isDeleted() const
      {
      U_TRACE(0, "URDB::isDeleted()")

      U_CHECK_MEMORY

      bool result = (RDB_node_data_pr == 0);

#  ifdef DEBUG
      if (result) { U_INTERNAL_ASSERT_EQUALS(RDB_node_data_sz, U_NOT_FOUND) }
#  endif

      U_RETURN(result);
      }

   bool cdbLookup() // NB: valorizza struct UCDB::data..
      {
      U_TRACE(0, "URDB::cdbLookup()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_EQUALS(node, 0)

      bool result = (UFile::st_size && UCDB::find());

      U_RETURN(result);
      }

   UString at();

   int remove();
   int substitute(UCDB::datum* new_key, int flag);

   // utility - especially create for net interface class

   static char* parseLine(const char* ptr, UCDB::datum* key, UCDB::datum* data);

          char* parseLine(const char* ptr) { return parseLine(ptr, &key, &data); }

private:
   uint32_t* pnode;
   uint32_t   node;

   static URDB* ptr_rdb;
   static UCDB::datum key1;
   static UVector<UString>* kvec;

   bool reorganize();             // Combines the old cdb file and the diffs in a new cdb file
   bool htLookup() U_NO_EXPORT;   // Search one key/data pair in the cache
   void htInsert() U_NO_EXPORT;   // Insert one key/data pair in the cache

   bool logJournal(int op) U_NO_EXPORT;
   bool writev(const struct iovec* iov, int n, uint32_t size) U_NO_EXPORT;

   inline void makeAdd() U_NO_EXPORT;
   inline bool resizeJournal(char* ptr) U_NO_EXPORT;
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
