// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    uhttp_session.h - HTTP session utility
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_HTTP_SESSION_H
#define ULIB_HTTP_SESSION_H 1

#include <ulib/utility/uhttp.h>
#include <ulib/utility/data_session.h>

class U_EXPORT UHTTPSession {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

    UHTTPSession(const char* location = "http_session", uint32_t size = 1024 * 1024, UDataSession* ptr = 0);

   ~UHTTPSession()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UHTTPSession)
      }

   // SERVICES

   UString getKeyID() const { return keyID; }
   UString getCreationTime() const;
   UString getLastAccessedTime() const;

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UString keyID;
   UDataSession* data_session;

   static void* db_session;
   static uint32_t counter;

   bool getDataSession();
   bool putDataSession();

   static void  endDB();
   static bool initDB(const char* location, uint32_t size);

private:
   UHTTPSession(const UHTTPSession&)            {}
   UHTTPSession& operator=(const UHTTPSession&) { return *this; }

   friend class UHTTP;
};

#endif
