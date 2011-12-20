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

      U_SRV_LOG("db initialization of http session %S success", location);
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

bool UHTTPSession::putDataSession(uint32_t lifetime) // lifetime of the cookie in HOURS (0 -> valid until browser exit)
{
   U_TRACE(0, "UHTTPSession::putDataSession(%u)", lifetime)

   // set session cookie

   if (db_session == 0 &&
       initDB()   == false)
      {
      U_SRV_LOG("db initialization of http session failed...");

      U_RETURN(false);
      }

   U_INTERNAL_ASSERT_POINTER(db_session)

   if (data_session == 0) data_session = U_NEW(UDataSession);

   UString param(100U), ip_client = UHTTP::getRemoteIP();

   U_ASSERT(key_id.empty())

   key_id.setBuffer(100U);
   key_id.snprintf("%.*s_%P_%u", U_STRING_TO_TRACE(ip_client), ++counter);

   // REQ: [ data expire path domain secure HttpOnly ]
   // -------------------------------------------------------------------------------------------------------------------------------
   // string -- key_id or data to put in cookie    -- must
   // int    -- lifetime of the cookie in HOURS    -- must (0 -> valid until browser exit)
   // string -- path where the cookie can be used  --  opt
   // string -- domain which can read the cookie   --  opt
   // bool   -- secure mode                        --  opt
   // bool   -- only allow HTTP usage              --  opt
   // -------------------------------------------------------------------------------------------------------------------------------
   // RET: Set-Cookie: ulib.s<counter>=data&expire&HMAC-MD5(data&expire); expires=expire(GMT); path=path; domain=domain; secure; HttpOnly

   param.snprintf("Set-Cookie: TODO[ %.*s %u ]\r\n", U_STRING_TO_TRACE(key_id), lifetime); // like as shell script...

   *UClientImage_Base::_set_cookie = UHTTP::setHTTPCookie(param, this);

   if (UServer_Base::preforked_num_kids == 0) ((UHashMap<UDataSession*>*)db_session)->insert(key_id, data_session);
   else
      {
      UString data = data_session->toString();

      if (data.empty() == false)
         {
         int result = ((URDB*)db_session)->store(key_id, data, RDB_REPLACE);

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

   key_id = UHTTP::getHTTPCookie(this);

   U_INTERNAL_DUMP("key_id = %.*S", U_STRING_TO_TRACE(key_id))

   if (key_id.empty() == false)
      {
      if (db_session == 0 &&
          initDB()   == false)
         {
         U_SRV_LOG("db initialization of http session failed...");

         U_RETURN(false);
         }

      if (UServer_Base::preforked_num_kids == 0)
         {
         if ((data_session = (*(UHashMap<UDataSession*>*)db_session)[key_id]))
            {
            data_session->last_access = u_now->tv_sec;

            U_RETURN(true);
            }
         }
      else
         {
         UString data = (*(URDB*)db_session)[key_id];

         if (data.empty() == false)
            {
            if (data_session == 0) data_session = U_NEW(UDataSession);

            data_session->fromString(data);

            U_ASSERT_EQUALS(data, data_session->toString())

            U_RETURN(true);
            }
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
                  << "key_id       (UString      " << (void*)&key_id      << ")\n"
                  << "value_id     (UString      " << (void*)&value_id    << ")\n"
                  << "data_session (UDataSession " << (void*)data_session << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
