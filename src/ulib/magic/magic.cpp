// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    magic.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/magic/magic.h>

magic_t        UMagic::magic;
const UString* UMagic::str_default;

void UMagic::str_allocate()
{
   U_TRACE(0, "UMagic::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_default,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("application/octet-stream") }
   };

   U_NEW_ULIB_OBJECT(str_default, U_STRING_FROM_STRINGREP_STORAGE(0));
}

bool UMagic::init(int flags)
{
   U_TRACE(1, "UMagic::init(%d)", flags)

   U_INTERNAL_ASSERT_EQUALS(magic, 0)

   magic = (magic_t) U_SYSCALL(magic_open, "%d", flags);

   bool ok = (magic && U_SYSCALL(magic_load, "%p", magic, 0) != -1);

   U_DUMP("ok = %b status = %.*S", ok, 512, getError())

   str_allocate();

   U_RETURN(ok);
}

UString UMagic::getType(const char* buffer, uint32_t buffer_len)
{
   U_TRACE(1, "UMagic::getType(%.*S,%u)", buffer_len, buffer, buffer_len)

   U_INTERNAL_ASSERT_POINTER(magic)

   const char* result = (const char*) U_SYSCALL(magic_buffer, "%p,%p,%u", magic, buffer, buffer_len);

   if (result)
      {
      UString str(result);

      U_RETURN_STRING(str);
      }

   if (str_default == 0) str_allocate();

   U_RETURN_STRING(*str_default);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UMagic::dump(bool reset) const
{
   U_CHECK_MEMORY

   *UObjectIO::os << "magic " << magic;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
