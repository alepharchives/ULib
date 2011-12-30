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

// method VIRTUAL to define

void UDataSession::fromStream(istream& is)
{
   U_TRACE(0, "UDataSession::fromStream(%p)", &is)

   is >> creation;

   is.get(); // skip ' '

   last_access = u_now->tv_sec;
}

void UDataSession::toStream(ostream& os)
{
   U_TRACE(0, "UDataSession::toStream(%p)", &os)

   os.put(' ');
   os << creation;
   os.put(' ');
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UDataSession::dump(bool reset) const
{
   *UObjectIO::os << "creation      " << creation     << '\n'
                  << "last_access   " << last_access  << '\n'
                  << "data (UString " << (void*)&data << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
