// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    schema.cpp - wrapping of libxml2
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/xml/libxml2/schema.h>

bool UXML2Schema::validate(const UString& docfile)
{
   U_TRACE(1, "UXML2Schema::validate(%.*S)", U_STRING_TO_TRACE(docfile))

   U_INTERNAL_ASSERT_POINTER(impl_)

   bool result = false; // (U_SYSCALL(xmlSaveFormatFileEnc, "%S,%p,%S,%d", filename, impl_, encoding, formatted) != -1);

   U_RETURN(result);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UXML2Schema::dump(bool reset) const
{
   *UObjectIO::os << "impl_ " << (void*)impl_;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
