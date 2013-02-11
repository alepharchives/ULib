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

#include <ulib/net/server/server.h>

/**
   @class URDBServer

   @brief Handles incoming TCP/IP connections from URDBClient.
*/

class URDB;

class U_EXPORT URDBServer : public UServer<UTCPSocket> {
public:

            URDBServer(UFileConfig* cfg, bool ignore_case = false);
   virtual ~URDBServer();

   // Open a reliable database

   static bool open(const UString& pathdb, uint32_t log_size = 1024 * 1024);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   static URDB* rdb;

   // method VIRTUAL to redefine

   virtual void preallocate();
#ifdef DEBUG
   virtual void  deallocate();
   virtual bool check_memory();
#endif

private:
   URDBServer(const URDBServer& s) : UServer<UTCPSocket>(0) {}
   URDBServer& operator=(const URDBServer&)                 { return *this; }
};

#endif
