// =================================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    magic.h - interface to the libmagic library (Magic Number Recognition Library)
//
// = AUTHOR
//    Stefano Casazza
//
// =================================================================================

#ifndef U_MAGIC_H
#define U_MAGIC_H 1

#include <ulib/string.h>

#include <magic.h>

class UHttpClient_Base;
class UMimeMultipartMsg;

class U_EXPORT UMagic {
public:

   static const UString* str_default;

   static void str_allocate();

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UMagic(int flags)
      {
      U_TRACE_REGISTER_OBJECT(0, UMagic, "%d", flags)

      if (magic == 0) (void) init();

      U_INTERNAL_ASSERT_POINTER(magic)

      (void) setFlags(flags);
      }

   /**
   * Deletes this object
   */

   ~UMagic()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UMagic)
      }

   // VARIE

   static void clear()
      {
      U_TRACE(1, "UMagic::clear()")

      if (magic)
         {
         U_SYSCALL_VOID(magic_close, "%p", magic);

         magic = 0;
         }
      }

   static const char* getError()
      {
      U_TRACE(1, "UMagic::getError()")

      U_INTERNAL_ASSERT_POINTER(magic)

      const char* result = (const char*) U_SYSCALL(magic_error, "%p", magic);

      U_RETURN(result);
      }

   static bool setFlags(int flags = MAGIC_NONE)
      {
      U_TRACE(1, "UMagic::setFlags(%d)", flags)

      U_INTERNAL_ASSERT_POINTER(magic)

      bool result = (U_SYSCALL(magic_setflags, "%p,%d", magic, flags) != -1);

      U_RETURN(result);
      }

   static bool init(int flags = MAGIC_MIME);

   static UString getType(const char* buffer, uint32_t buffer_len);

   static UString getType(const UString& buffer) { return getType(U_STRING_TO_PARAM(buffer)); }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   static magic_t magic; /* pointer to magic :-) */

private:
   UMagic(const UMagic&)            {}
   UMagic& operator=(const UMagic&) { return *this; }

   friend class UHttpClient_Base;
   friend class UMimeMultipartMsg;
};

#endif
