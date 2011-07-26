// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    client_image_rdb.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/db/rdb.h>
#include <ulib/net/rpc/rpc.h>
#include <ulib/net/server/server.h>
#include <ulib/net/server/client_image_rdb.h>

// 2xx indicates success of some kind
#define STR_200 "200 "  // OK

// 4xx try again: not found, EOF, no database open, permission denied
#define STR_400 "400 "  // Lookup failed: the entry was not in the database
#define STR_401 "401 "  // Store failed: flag was insertion of new entries only and the key already existed
#define STR_402 "402 "  // Remove failed: the entry was already marked deleted

// 5xx go away: parse error, catastrophic error, ...
#define STR_500 "500 "  // Server error: requested action not taken

URDB* URDBClientImage::rdb;

int URDBClientImage::handlerRead()
{
   U_TRACE(0, "URDBClientImage::handlerRead()")

   U_INTERNAL_ASSERT_POINTER(UClientImage_Base::rbuffer)
   U_INTERNAL_ASSERT_POINTER(UClientImage_Base::wbuffer)

   int result = genericRead();

   if (result == U_PLUGIN_HANDLER_AGAIN) U_RETURN(U_NOTIFIER_OK); // NONBLOCKING...
   if (result == U_PLUGIN_HANDLER_ERROR) U_RETURN(U_NOTIFIER_DELETE);

   U_INTERNAL_ASSERT_EQUALS(result, U_PLUGIN_HANDLER_GO_ON)

   UClientImage_Base::initAfterGenericRead();

   if (UServer_Base::isLog()) UClientImage_Base::logRequest();

   // check for RPC request

   if (URPC::readRPCRequest(UClientImage_Base::socket, true) == false) U_RETURN(U_NOTIFIER_DELETE);

   result = U_NOTIFIER_OK;

   // Process the RPC message

   const char* ptr = UClientImage_Base::request->data();
   const char* res = 0;

   (void) wbuffer->reserve(U_TOKEN_LN);

   if (U_STRNEQ(ptr, "FIND")) // FIND
      {
      if (rdb->find((*URPC::rpc_info)[0]))
         {
         // Build the response: 200

         uint32_t size = rdb->data.dsize;

         (void) wbuffer->reserve(U_TOKEN_LN + size);

         UStringExt::buildTokenInt(res = STR_200, size, *wbuffer);

         (void) wbuffer->append((const char*)rdb->data.dptr, size);
         }
      else
         {
         // Build the response: 400

         UStringExt::buildTokenInt(res = STR_400, 0, *wbuffer);
         }
      }
   else if (U_STRNEQ(ptr, "STR")) // STORE
      {
      // ------------------------------------------------------
      // Write a key/value pair to a reliable database
      // ------------------------------------------------------
      // RETURN VALUE
      // ------------------------------------------------------
      //  0: Everything was OK
      // -1: flag was RDB_INSERT and this key already existed
      // -3: disk full writing to the journal file
      // ------------------------------------------------------

      // #define RDB_INSERT  0 // Insertion of new entries only
      // #define RDB_REPLACE 1 // Allow replacing existing entries

      result = rdb->store((*URPC::rpc_info)[0], (*URPC::rpc_info)[1], ptr[3] == '0' ? RDB_INSERT : RDB_REPLACE);

      switch (result)
         {
         case  0: res = STR_200; break; //  0: Everything was OK
         case -1: res = STR_401; break; // -1: flag was RDB_INSERT and this key already existed
         case -3: res = STR_500; break; // -3: disk full writing to the journal file
         }

      UStringExt::buildTokenInt(res, 0, *wbuffer);
      }
   else if (U_STRNEQ(ptr, "REMV")) // REMOVE
      {
      // ---------------------------------------------------------
      // Mark a key/value as deleted
      // ---------------------------------------------------------
      // RETURN VALUE
      // ---------------------------------------------------------
      //  0: Everything was OK
      // -1: The entry was not in the database
      // -2: The entry was already marked deleted in the hash-tree
      // -3: disk full writing to the journal file
      // ---------------------------------------------------------

      result = rdb->remove((*URPC::rpc_info)[0]);

      switch (result)
         {
         case  0: res = STR_200; break; //  0: Everything was OK
         case -1: res = STR_400; break; // -1: The entry was not in the database
         case -2: res = STR_402; break; // -2: The entry was already marked deleted in the hash-tree
         case -3: res = STR_500; break; // -3: disk full writing to the journal file
         }

      UStringExt::buildTokenInt(res, 0, *wbuffer);
      }
   else if (U_STRNEQ(ptr, "SUB")) // SUBSTITUTE
      {
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

      // #define RDB_INSERT  0 // Insertion of new entries only
      // #define RDB_REPLACE 1 // Allow replacing existing entries

      result = rdb->substitute((*URPC::rpc_info)[0], (*URPC::rpc_info)[1], (*URPC::rpc_info)[2], ptr[3] == '0' ? RDB_INSERT : RDB_REPLACE);

      switch (result)
         {
         case  0: res = STR_200; break; //  0: Everything was OK
         case -1: res = STR_400; break; // -1: The entry was not in the database
         case -2: res = STR_402; break; // -2: The entry was marked deleted in the hash-tree
         case -3: res = STR_500; break; // -3: disk full writing to the journal file
         case -4: res = STR_401; break; // -4: flag was RDB_INSERT and the new key already existed
         }

      UStringExt::buildTokenInt(res, 0, *wbuffer);
      }
   else if (U_STRNEQ(ptr, "PRT"))   // PRINT
      {
      // Build the response: 200

      UString tmp = (ptr[3] == '0' ? rdb->print() : rdb->printSorted());

      UStringExt::buildTokenInt(res = STR_200, tmp.size(), *wbuffer);

      wbuffer->append(tmp);
      }
   else if (U_STRNEQ(ptr, "RORG")) // REORGANIZE
      {
      res = (rdb->reorganize() ? STR_200 : STR_500);

      UStringExt::buildTokenInt(res, 0, *wbuffer);
      }
   else if (U_STRNEQ(ptr, "BTRN")) // beginTransaction
      {
      res = (rdb->beginTransaction() ? STR_200 : STR_500);

      UStringExt::buildTokenInt(res, 0, *wbuffer);
      }
   else if (U_STRNEQ(ptr, "ATRN")) // abortTransaction
      {
      rdb->abortTransaction();

      UStringExt::buildTokenInt(res = STR_200, 0, *wbuffer);
      }
   else if (U_STRNEQ(ptr, "CTRN")) // commitTransaction
      {
      rdb->commitTransaction();

      UStringExt::buildTokenInt(res = STR_200, 0, *wbuffer);
      }
   else
      {
      UStringExt::buildTokenInt(res = STR_500, 0, *wbuffer);
      }

   U_SRV_LOG_WITH_ADDR("method %.4S return %s for", ptr, res);

   result = handlerWrite();

   U_RETURN(result);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* URDBClientImage::dump(bool _reset) const
{
   UClientImage<UTCPSocket>::dump(false);

   *UObjectIO::os << '\n'
                  << "rdb             (URDB              " << (void*)rdb << ')';

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
