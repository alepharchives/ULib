// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    cgi.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/cgi/cgi.h>
#include <ulib/utility/services.h>

UString UCGI::getPostData() const
{
   U_TRACE(0, "UCGI::getPostData()")

   uint32_t size = (uint32_t) getContentLength();

   if (size)
      {
      UString result(size);
      char* ptr = result.data();

      uint32_t n = (input ? input->read(ptr, size) : UCgiInput::_read(ptr, size));

      result.size_adjust(n);

      U_RETURN_STRING(result);
      }
   else
      {
      UString result(U_CAPACITY);

      (void) UServices::read(STDIN_FILENO, result);

      U_RETURN_STRING(result);
      }
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UCGI::dump(bool reset) const
{
   *UObjectIO::os << "input " << (void*)input;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
