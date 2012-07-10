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

   U_ASSERT_EQUALS(_data.empty(), false)

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

bool UDataSession::getValue(uint32_t index, UString& value)
{
   U_TRACE(0, "UDataSession::getValue(%u,%p)", index, &value)

   if (vec)
      {
      value = vec->at(index);

      U_INTERNAL_DUMP("value = %.*S", U_STRING_TO_TRACE(value))

      U_RETURN(true);
      }

   U_RETURN(false);
}

void UDataSession::putValue(uint32_t index, const UString& value)
{
   U_TRACE(0, "UDataSession::putValue(%u,%.*S)", index, U_STRING_TO_TRACE(value))

   if (vec == 0)
      {
      U_INTERNAL_ASSERT_EQUALS(index,0)

      vec = U_NEW(UVector<UString>);
      }

   if (index < vec->size()) vec->replace(index, value);
   else
      {
      U_INTERNAL_ASSERT_EQUALS(index,vec->size())

      vec->push_back(value);
      }
}

// method VIRTUAL to define

void UDataSession::clear()
{
   U_TRACE(0, "UDataSession::clear()")

   if (vec)
      {
      vec->clear();

      delete vec;
             vec = 0;
      }
}

void UDataSession::fromStream(istream& is)
{
   U_TRACE(0, "UDataSession::fromStream(%p)", &is)

   is >> creation;

   is.get(); // skip ' '

   if (is.peek() == '(')
      {
      U_INTERNAL_ASSERT_EQUALS(vec,0)

      vec = UVector<UString>::fromStream(is);

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

   if (vec)
      {
      os << *vec;

      os.put(' ');
      }
}

void UDataSession::fromDataSession(UDataSession& data_session)
{
   U_TRACE(0, "UDataSession::fromDataSession(%p)", &data_session)

   U_INTERNAL_ASSERT_EQUALS(vec,0)

   vec      = UVector<UString>::duplicate(data_session.vec);
   creation = data_session.creation;

   last_access = u_now->tv_sec;
}

UDataSession* UDataSession::toDataSession()
{
   U_TRACE(0, "UDataSession::toDataSession()")

   UDataSession* ptr = U_NEW(UDataSession);

   ptr->vec      = UVector<UString>::duplicate(vec);
   ptr->creation = ptr->last_access = creation;

   U_RETURN_POINTER(ptr,UDataSession);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UDataSession::dump(bool reset) const
{
   *UObjectIO::os << "creation                " << creation     << '\n'
                  << "last_access             " << last_access  << '\n'
                  << "data (UString           " << (void*)&data << ")\n"
                  << "vec  (UVector<UString*> " << (void*)vec   << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
