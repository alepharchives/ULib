// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    document.cpp - wrapping of libxml2
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/xml/libxml2/document.h>

bool UXML2Document::writeToFile(const char* filename, const char* encoding, bool formatted)
{
   U_TRACE(1, "UXML2Document::writeToFile(%S,%S,%b)", filename, encoding, formatted)

   U_INTERNAL_ASSERT_POINTER(impl_)

   int oldIndentTreeOutput, oldKeepBlanksDefault;

   if (formatted)
      {
      oldIndentTreeOutput  = xmlIndentTreeOutput;
      xmlIndentTreeOutput  = 1;
      oldKeepBlanksDefault = U_SYSCALL(xmlKeepBlanksDefault, "%d", 1);
      }

   bool result = (U_SYSCALL(xmlSaveFormatFileEnc, "%S,%p,%S,%d", filename, impl_, encoding, formatted) != -1);

   if (formatted)
      {
      xmlIndentTreeOutput = oldIndentTreeOutput;

      (void) U_SYSCALL(xmlKeepBlanksDefault, "%d", oldKeepBlanksDefault);
      }

   U_RETURN(result);
}

xmlChar* UXML2Document::writeToString(int& length, const char* encoding, bool formatted)
{
   U_TRACE(1, "UXML2Document::writeToString(%S,%b)", encoding, formatted)

   U_INTERNAL_ASSERT_POINTER(impl_)

   int oldIndentTreeOutput, oldKeepBlanksDefault;

   if (formatted)
      {
      oldIndentTreeOutput  = xmlIndentTreeOutput;
      xmlIndentTreeOutput  = 1;
      oldKeepBlanksDefault = U_SYSCALL(xmlKeepBlanksDefault, "%d", 1);
      }

   xmlChar* buffer = 0;

   U_SYSCALL_VOID(xmlDocDumpFormatMemoryEnc, "%p,%p,%p,%S,%d", impl_, &buffer, &length, encoding, formatted);

   if (formatted)
      {
      xmlIndentTreeOutput = oldIndentTreeOutput;

      (void) U_SYSCALL(xmlKeepBlanksDefault, "%d", oldKeepBlanksDefault);
      }

   U_RETURN_POINTER(buffer, xmlChar);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UXML2Document::dump(bool reset) const
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
