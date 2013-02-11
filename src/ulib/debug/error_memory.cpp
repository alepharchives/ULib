// ============================================================================
//
// = LIBRARY
//    ulibdbg - c++ library
//
// = FILENAME
//    error_memory.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/base/utility.h>
#include <ulib/internal/common.h>

const char* UMemoryError::getErrorType(const void* pobj) const
{
   // (ABW) Array Beyond Write | (FMR) Free Memory Read...

   if (invariant() == false)
      {
      char* pbuffer = (u_buffer_len == 0 ? u_buffer : (char*)malloc(sizeof(u_buffer)));

      (void) sprintf(pbuffer, "[pobj = %p _this = %p - %s]", pobj, _this, (_this ? "ABW" : "FMR"));

      if (UObjectDB::fd > 0)
         {
         uint32_t n = u__strlen(pbuffer),
                  l = UObjectDB::dumpObject(pbuffer+n+1, sizeof(u_buffer)-n-1, pobj);

         if (l)
            {
            pbuffer[n  +1] = '\n';
            pbuffer[n+l+1] = '\n';
            pbuffer[n+l+2] = '\0';
            }
         }

      return pbuffer;
      }

   return "";
}
