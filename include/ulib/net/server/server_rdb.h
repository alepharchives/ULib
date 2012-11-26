// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    server_rdb.h - RDB Server
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_SERVER_RDB_H
#define U_SERVER_RDB_H 1

#include <ulib/db/rdb.h>
#include <ulib/net/rpc/rpc.h>
#include <ulib/net/server/server.h>
#include <ulib/net/server/client_image_rdb.h>

/**
   @class URDBServer

   @brief Handles incoming TCP/IP connections from URDBClient.
*/

class U_EXPORT URDBServer : public UServer<UTCPSocket> {
public:

   URDBServer(UFileConfig* cfg, bool ignore_case = false) : UServer<UTCPSocket>(cfg)
      {
      U_TRACE_REGISTER_OBJECT(0, URDBServer, "%p,%b", cfg, ignore_case)

      rdb = U_NEW(URDB(ignore_case));

      URPC::allocate();
      }

   virtual ~URDBServer()
      {
      U_TRACE_UNREGISTER_OBJECT(0, URDBServer)

      delete rdb;
      delete URPC::rpc_info;
      }

   // Open a reliable database

   static bool open(const UString& pathdb, uint32_t log_size = 1024 * 1024)
      {
      U_TRACE(0, "URDBServer::open(%.*S,%u)", U_STRING_TO_TRACE(pathdb), log_size)

      URDBClientImage::rdb = rdb;

      bool result = rdb->open(pathdb, log_size);

      U_RETURN(result);
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   static URDB* rdb;

   // method VIRTUAL to redefine

   virtual void preallocate()
      {
      U_TRACE(0, "URDBServer::preallocate()")

      if (UServer_Base::isPreForked())
         {
         U_INTERNAL_ASSERT_POINTER(U_LOCK_RDB_SERVER)

         rdb->setShared(U_LOCK_RDB_SERVER);
         }

      vClientImage = U_NEW_VEC(UNotifier::max_connection, URDBClientImage);
      }

private:
   URDBServer(const URDBServer& s) : UServer<UTCPSocket>(0) {}
   URDBServer& operator=(const URDBServer&)                 { return *this; }
};

#endif
