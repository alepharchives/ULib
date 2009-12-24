/* Public domain. */

#include "cdb.h"

uint32 cdb_hashadd(uint32 h,unsigned char c)
{
	h ^= (uint32) c;

	// multiply by the 32 bit FNV magic prime mod 2^32

#ifdef NO_FNV_GCC_OPTIMIZATION
	h *= FNV_32_PRIME;
#else
	h += (h<<1) + (h<<4) + (h<<7) + (h<<8) + (h<<24);
#endif

	return h;
}

uint32 cdb_hash(char *buf,unsigned int len)
{
  uint32 h;

  h = CDB_HASHSTART;
  while (len) {
	 h = cdb_hashadd(h,*buf++);
	 --len;
  }
  return h;
}
