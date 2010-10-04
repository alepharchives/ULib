// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    client_rdb.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_RDB_CLIENT_H
#define U_RDB_CLIENT_H 1

#include <ulib/db/rdb.h>
#include <ulib/net/rpc/rpc.h>
#include <ulib/net/client/client.h>

/**
   @class URDBClient_Base

   @brief Creates and manages a client connection with a RDB server
*/

class U_EXPORT URDBClient_Base : virtual public UClient_Base {
public:

   // SERVICES

   void reset()
      {
      U_TRACE(0, "URDBClient_Base::reset()")

      URPC::resetRPCInfo();

      if (request.uniq()) request.setEmpty();
      }

   bool isOK()
      {
      U_TRACE(0, "URDBClient_Base::isOK()")

      U_RETURN(nResponseCode == 200);
      }

   bool readResponse();
   bool processRequest(const char* token);
 
   // Combines the old cdb file and the diffs in a new cdb file

   bool closeReorganize();

   // ------------------------------------------------------
   // Write a key/value pair to a reliable database.
   // ------------------------------------------------------
   // RETURN VALUE
   // ------------------------------------------------------
   //  0: Everything was OK.
   // -1: flags was RDB_INSERT and this key already existed.
   // -3: disk full writing to the journal file
   // ------------------------------------------------------

   int store(const UString& key, const UString& data, int flag = RDB_INSERT);

   // ---------------------------------------------------------
   // Mark a key/value as deleted.
   // ---------------------------------------------------------
   // RETURN VALUE
   // ---------------------------------------------------------
   //  0: Everything was OK.
   // -1: The entry was not in the database
   // -2: The entry was already marked deleted in the hash-tree
   // -3: disk full writing to the journal file
   // ---------------------------------------------------------

   int remove(const UString& key);

   // ----------------------------------------------------------
   // Substitute a key/value with a new key/value (remove+store)
   // ----------------------------------------------------------
   // RETURN VALUE
   // ----------------------------------------------------------
   //  0: Everything was OK
   // -1: The entry was not in the database
   // -2: The entry was marked deleted in the hash-tree
   // -3: disk full writing to the journal file
   // -4: flag was RDB_INSERT and the new key already existed
   // ----------------------------------------------------------

   int substitute(const UString& key, const UString& new_key, const UString& data, int flag = RDB_INSERT);

   // operator []

   UString operator[](const UString& key);

   // TRANSACTION

   bool  beginTransaction();
   bool  abortTransaction();
   bool commitTransaction();

   // Call function for all entry

   void callForAllEntry(      vPFprpr function) { _callForAllEntry(function, false); }
   void callForAllEntrySorted(vPFprpr function) { _callForAllEntry(function, true); }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   int nResponseCode;

   // Costruttori

   URDBClient_Base(UFileConfig* cfg) : UClient_Base(cfg)
      {
      U_TRACE_REGISTER_OBJECT(0, URDBClient_Base, "%p", cfg)

      nResponseCode = 0;

      URPC::allocate();

      request.setBuffer(U_CAPACITY);
      }

   ~URDBClient_Base()
      {
      U_TRACE_UNREGISTER_OBJECT(0, URDBClient_Base)

      delete URPC::rpc_info;
      }

   // Call function for all entry

   void _callForAllEntry(vPFprpr function, bool sorted);

private:
   const char* status() U_NO_EXPORT;

   URDBClient_Base(const URDBClient_Base&) : UClient_Base(0) {}
   URDBClient_Base& operator=(const URDBClient_Base&)        { return *this; }
};

template <class Socket> class U_EXPORT URDBClient : public URDBClient_Base,  public UClient<Socket> {
public:

   // COSTRUTTORI

   URDBClient(UFileConfig* cfg) : UClient_Base(cfg), URDBClient_Base(cfg), UClient<Socket>(cfg)
      {
      U_TRACE_REGISTER_OBJECT(0, URDBClient, "%p", cfg)
      }

   ~URDBClient()
      {
      U_TRACE_UNREGISTER_OBJECT(0, URDBClient)
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool _reset) const { return URDBClient_Base::dump(_reset); }
#endif

private:
   URDBClient(const URDBClient&) : URDBClient_Base(0) {}
   URDBClient& operator=(const URDBClient&)           { return *this; }
};

#endif
