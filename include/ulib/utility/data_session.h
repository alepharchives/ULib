// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    data_session.h - data session utility
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_DATA_SESSION_H
#define ULIB_DATA_SESSION_H 1

#include <ulib/string.h>

#ifdef HAVE_STRSTREAM_H
#  include <strstream.h>
#else
#  include <ulib/replace/strstream.h>
#endif

class UHTTP;
class UHTTPSession;

class U_EXPORT UDataSession {
public:

   // Check Memory
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORE

   time_t creation, last_access;

   UDataSession()
      {
      U_TRACE_REGISTER_OBJECT(0, UDataSession, "")

      creation = last_access = u_now->tv_sec;
      }

   UDataSession(const UString& _data)
      {
      U_TRACE_REGISTER_OBJECT(0, UDataSession, "%.*S", U_STRING_TO_TRACE(_data))

      creation = u_now->tv_sec;

      fromString(_data);
      }

   virtual ~UDataSession()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UDataSession)

      clear();
      }

   // SERVICES

   UString   toString();
   void    fromString(const UString& data);

   // method VIRTUAL to define

   virtual void clear() {}

   virtual void   toStream(ostream& os);
   virtual void fromStream(istream& is);

   virtual UDataSession*   toDataSession();
   virtual void          fromDataSession(UDataSession& data_session);

   // STREAM

   friend istream& operator>>(istream& is, UDataSession& d) { d.fromStream(is); return is; }
   friend ostream& operator<<(ostream& os, UDataSession& d) {   d.toStream(os); return os; }

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UString data;

private:
   UDataSession(const UDataSession&)            {}
   UDataSession& operator=(const UDataSession&) { return *this; }

   friend class UHTTP;
   friend class UHTTPSession;
};

#endif
