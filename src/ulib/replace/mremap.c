/* mremap.c */

#if !defined(U_ALL_C)
#  include <ulib/base/top.h>
#  ifdef HAVE_CONFIG_H
#     include <ulib/internal/config.h>
#  endif
#  include <ulib/base/bottom.h>
#endif

#include <errno.h>
#ifndef __MINGW32__
#  include <sys/mman.h>
#endif

#ifndef PAGE_MASK
#define PAGE_MASK      0xFFFFF000
#endif
#ifndef MREMAP_MAYMOVE
#define MREMAP_MAYMOVE 1
#endif

/* 
 * Expand (or shrink) an existing mapping, potentially moving it at the 
 * same time (controlled by the MREMAP_MAYMOVE flag and available VM space) 
 */ 

extern U_EXPORT void* mremap(void* addr, size_t old_len , size_t new_len, unsigned long flags);

U_EXPORT void* mremap(void* addr, size_t old_len , size_t new_len, unsigned long flags)
{
   if (((unsigned long)addr & (~PAGE_MASK)))
      {
      errno = EINVAL;

      return (void*)-1;
      }

   if (!(flags & MREMAP_MAYMOVE))
      {
      errno = ENOSYS;

      return (void*)-1;
      }

   /*
   old_len = PAGE_ALIGN(old_len);
   new_len = PAGE_ALIGN(new_len);
   */

   /* Always allow a shrinking remap: that just unmaps the unnecessary pages.. */ 

   if (old_len > new_len)
      {
      munmap(addr+new_len, old_len - new_len); 

      return addr;
      }

   errno = ENOSYS;

   return (void*)-1;
}
