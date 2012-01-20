// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    data_session.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/utility/data_session.h>

void UDataSession::fromString(const UString& _data)
{
   U_TRACE(0, "UDataSession::fromString(%.*S)", U_STRING_TO_TRACE(_data))

   U_ASSERT_DIFFERS(_data.empty(), true)

   data = _data;

   istrstream is(_data.data(), _data.size());

   fromStream(is);
}

UString UDataSession::toString()
{
   U_TRACE(0, "UDataSession::toString()")

   char buffer[64 * 1024];

   ostrstream os(buffer, sizeof(buffer));

   toStream(os);

   UString x((void*)buffer, os.pcount());

   U_RETURN_STRING(x);
}

bool UDataSession::getValue(const char* key, uint32_t keylen, UString& value)
{
   U_TRACE(0, "UDataSession::getValue(%.*S,%u,%p)", keylen, key, keylen, &value)

   if (tbl)
      {
      value = tbl->at(key, keylen);

      if (value.empty() == false) U_RETURN(true);
      }

   U_RETURN(false);
}

void UDataSession::putValue(const UString& key, const UString& value)
{
   U_TRACE(0, "UDataSession::putValue(%.*S,%.*S)", U_STRING_TO_TRACE(key), U_STRING_TO_TRACE(value))

   if (tbl == 0)
      {
      tbl = U_NEW(UHashMap<UString>);

      tbl->allocate();
      }

   tbl->insert(key, value);
}

// method VIRTUAL to define

void UDataSession::clear()
{
   U_TRACE(0, "UDataSession::clear()")

   if (tbl)
      {
      tbl->clear();
      tbl->deallocate();

      delete tbl;
             tbl = 0;
      }
}

void UDataSession::fromStream(istream& is)
{
   U_TRACE(0, "UDataSession::fromStream(%p)", &is)

   is >> creation;

   is.get(); // skip ' '

   if (is.peek() == '[')
      {
      U_INTERNAL_ASSERT_EQUALS(tbl,0)

      tbl = UHashMap<UString>::fromStream(is);

      is.get(); // skip ' '
      }

   last_access = u_now->tv_sec;
}

void UDataSession::toStream(ostream& os)
{
   U_TRACE(0, "UDataSession::toStream(%p)", &os)

   os.put(' ');
   os << creation;
   os.put(' ');

   if (tbl)
      {
      os << *tbl;

      os.put(' ');
      }
}

void UDataSession::fromDataSession(UDataSession& data_session)
{
   U_TRACE(0, "UDataSession::fromDataSession(%p)", &data_session)

   U_INTERNAL_ASSERT_EQUALS(tbl,0)

   tbl      = UHashMap<UString>::duplicate(data_session.tbl);
   creation = data_session.creation;

   last_access = u_now->tv_sec;
}

UDataSession* UDataSession::toDataSession()
{
   U_TRACE(0, "UDataSession::toDataSession()")

   UDataSession* ptr = U_NEW(UDataSession);

   ptr->tbl      = UHashMap<UString>::duplicate(tbl);
   ptr->creation = ptr->last_access = creation;

   U_RETURN_POINTER(ptr,UDataSession);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UDataSession::dump(bool reset) const
{
   *UObjectIO::os << "creation                 " << creation     << '\n'
                  << "last_access              " << last_access  << '\n'
                  << "data (UString            " << (void*)&data << ")\n"
                  << "tbl  (UHashMap<UString*> " << (void*)tbl   << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
