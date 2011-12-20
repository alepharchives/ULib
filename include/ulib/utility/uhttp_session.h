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

   UHTTPSession(UDataSession* ptr = 0) : data_session(ptr)
      {
      U_TRACE_REGISTER_OBJECT(0, UHTTPSession, "%p", ptr)
      }

   ~UHTTPSession()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UHTTPSession)
      }

   // SERVICES

   UString getId() const                  { return value_id; }
   UString getCreationTime() const;
   UString getLastAccessedTime() const;

   static void  endDB();
   static bool initDB(const char* location = "http_session", uint32_t size = 1024 * 1024);

   // GET/PUT

   bool getDataSession();
   bool putDataSession(uint32_t lifetime = 0); // lifetime of the cookie in HOURS (0 -> valid until browser exit)

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UString key_id, value_id;
   UDataSession* data_session;

   static void* db_session;
   static uint32_t counter;

private:
   UHTTPSession(const UHTTPSession&)            {}
   UHTTPSession& operator=(const UHTTPSession&) { return *this; }

   friend class UHTTP;
};

#endif
