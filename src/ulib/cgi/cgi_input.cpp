// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    cgi_input.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/notifier.h>
#include <ulib/cgi/input.h>

uint32_t UCgiInput::_read(char* data, uint32_t length)
{
   U_TRACE(0, "UCgiInput::_read(%p,%u)", data, length)

   uint32_t result = UNotifier::read(STDIN_FILENO, data, length, U_TIMEOUT);

   U_RETURN(result);
}

UString UCgiInput::_getenv(const char* varName)
{
   U_TRACE(1, "UCgiInput::_getenv(%S)", varName)

   char* var = U_SYSCALL(getenv, "%S", varName);

   if (var)
      {
      UString result(var);

      U_RETURN_STRING(result);
      }

   U_RETURN_STRING(UString::getStringNull());
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UCgiInput::dump(bool reset) const
{
// *UObjectIO::os << "err     " << err << '\n'
//                << "handle  " << (void*)handle;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
