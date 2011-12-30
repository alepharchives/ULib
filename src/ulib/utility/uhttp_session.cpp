// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    uhttp_session.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/db/rdb.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/uhttp_session.h>

void*    UHTTPSession::db_session;
uint32_t UHTTPSession::counter;

UHTTPSession::UHTTPSession(const char* location, uint32_t size, UDataSession* ptr) : data_session(ptr)
{
   U_TRACE_REGISTER_OBJECT(0, UHTTPSession, "%S,%u,%p", location, size, ptr)

   if (db_session             == 0 &&
       initDB(location, size) == false)
      {
      U_SRV_LOG("db initialization of http session failed...");
      }
   else
      {
      U_SRV_LOG("db initialization of http session %S success", location);
      }
}

bool UHTTPSession::initDB(const char* location, uint32_t size)
{
   U_TRACE(0, "UHTTPSession::initDB(%S,%u)", location, size)

   U_INTERNAL_ASSERT_EQUALS(db_session,0)

   if (UServer_Base::preforked_num_kids == 0)
      {
      db_session = U_NEW(UHashMap<UDataSession*>);

      ((UHashMap<UDataSession*>*)db_session)->allocate(U_GET_NEXT_PRIME_NUMBER(size));
      }
   else
      {
      // NB: the old sessions are automatically invalid because UServer generate the crypto key at startup...

      UString pathdb(U_CAPACITY);

      pathdb.snprintf("%s%s", (location[0] == '/' ? "" : U_LIBEXECDIR "/"), location);

      db_session = U_NEW(URDB(pathdb, false));

      if (((URDB*)db_session)->open(size, true) == false)
         {
         delete (URDB*)db_session;
                       db_session = 0;

         U_RETURN(false);
         }
      }

   U_RETURN(true);
}

void UHTTPSession::endDB()
{
   U_TRACE(0, "UHTTPSession::endDB()")

   if (db_session)
      {
      if (UServer_Base::preforked_num_kids == 0)
         {
         ((UHashMap<UDataSession*>*)db_session)->clear();
         ((UHashMap<UDataSession*>*)db_session)->deallocate();

         delete (UHashMap<UDataSession*>*)db_session;
         }
      else
         {
         ((URDB*)db_session)->close();

         delete (URDB*)db_session;
         }

      db_session = 0;
      }
}

bool UHTTPSession::putDataSession()
{
   U_TRACE(0, "UHTTPSession::putDataSession()")

   U_ASSERT_DIFFERS(keyID.empty(), true)

   if (db_session == 0) U_RETURN(false);

   if (data_session == 0) data_session = U_NEW(UDataSession);

   if (UServer_Base::preforked_num_kids == 0) ((UHashMap<UDataSession*>*)db_session)->insert(keyID, data_session);
   else
      {
      UString data = data_session->toString();

      if (data.empty() == false)
         {
         int result = ((URDB*)db_session)->store(keyID, data, RDB_REPLACE);

         if (result)
            {
            U_SRV_LOG("store of session data on db failed with error %d", result);

            U_RETURN(false);
            }
         }
      }

   U_RETURN(true);
}

bool UHTTPSession::getDataSession()
{
   U_TRACE(0, "UHTTPSession::getDataSession()")

   U_ASSERT_DIFFERS(keyID.empty(), true)

   if (db_session == 0) U_RETURN(false);

   if (UServer_Base::preforked_num_kids == 0)
      {
      if ((data_session = (*(UHashMap<UDataSession*>*)db_session)[keyID]))
         {
         data_session->last_access = u_now->tv_sec;

         U_RETURN(true);
         }
      }
   else
      {
      UString data = (*(URDB*)db_session)[keyID];

      if (data.empty() == false)
         {
         if (data_session == 0) data_session = U_NEW(UDataSession);

         data_session->fromString(data);

         U_ASSERT_EQUALS(data, data_session->toString())

         U_RETURN(true);
         }
      }

   U_RETURN(false);
}

UString UHTTPSession::getCreationTime() const
{
   U_TRACE(0, "UHTTPSession::getCreationTime()")

   UString x(40U);

   if (data_session) x.snprintf("%#7D", data_session->creation);

   U_RETURN_STRING(x);
}

UString UHTTPSession::getLastAccessedTime() const
{
   U_TRACE(0, "UHTTPSession::getLastAccessedTime()")

   UString x(40U);

   if (data_session) x.snprintf("%#7D", data_session->last_access);

   U_RETURN_STRING(x);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UHTTPSession::dump(bool reset) const
{
   *UObjectIO::os << "counter                    " << counter             << '\n'
                  << "keyID        (UString      " << (void*)&keyID       << ")\n"
                  << "data_session (UDataSession " << (void*)data_session << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
