/* ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    hash.c
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================*/

#include <ulib/base/hash.h>
#include <ulib/base/utility.h>

/* Quick 4byte hashing function

References: http://mia.ece.uic.edu/cgi-bin/lxr/http/ident?v=ipband-0.7.2;i=makehash
-----------------------------------------------------------------------------------
Performs a one to one mapping between input integer and output integers, in other words, two different
input integers i, j will ALWAYS result in two different output hash(i) and hash(j). This hash function
is designed so that a changing just one bit in input 'i' will potentially affect the every bit in hash(i),
and the correlation between succesive hashes is (hopefully) extremely small (if not zero). It also can be
used as a quick, dirty, portable and open source random number generator that generates randomness on all 32 bits
*/

uint32_t u_random(uint32_t a)
{
   /* Random sequence table - source of random numbers.
      The random() routine 'amplifies' this 2**8 long sequence into a 2**32 long sequence.
   */

   static const unsigned char rseq[259] = {
    79, 181,  35, 147,  68, 177,  63, 134, 103,   0,  34,  88,  69, 221, 231,  13,
    91,  49, 220,  90,  58, 112,  72, 145,   7,   4,  93, 176, 129, 192,   5, 132,
    86, 142,  21, 148,  37, 139,  39, 169, 143, 224, 251,  64, 223,   1,   9, 152,
    51,  66,  98, 155, 180, 109, 149, 135, 229, 137, 215,  42,  62, 115, 246, 242,
   118, 160,  94, 249, 123, 144, 122, 213, 252, 171,  60, 167, 253, 198,  77,   2,
   154, 174, 168,  52,  27,  92, 226, 233, 205,  10, 208, 247, 209, 113, 211, 106,
   163, 116,  65, 196,  73, 201,  23,  15,  31, 140, 189,  53, 207,  83,  87, 202,
   101, 173,  28,  46,   6, 255, 237,  47, 227,  36, 218,  70, 114,  22, 100,  96,
   182, 117,  43, 228, 210,  19, 191, 108, 128,  89,  97, 153, 212, 203,  99, 236,
   238, 141,   3,  95,  29, 232,   8,  75,  57,  25, 159,  24, 131, 162,  67, 119,
    74,  30, 138, 214, 240,  12, 187, 127, 133,  18,  81, 222, 188, 239,  82, 199,
   186, 166, 197, 230, 126, 161, 200,  40,  59, 165, 136, 234, 250,  44, 170, 157,
   190, 150, 105,  84,  55, 204,  56, 244, 219, 151, 178, 195, 194, 110, 184,  14,
    48, 146, 235, 216, 120, 175, 254,  50, 102, 107,  41, 130,  54,  26, 248, 225,
   111, 124,  33, 193,  76, 121, 125, 158, 185, 245,  16, 206,  71,  45,  20, 179,
    32,  38, 241,  80,  85, 243,  11, 217,  61,  17,  78, 172, 156, 183, 104, 164,
    79, 181,  35 };

   union uuintchar {
      uint32_t i;
      unsigned char d[4];
   };

   int i = 0;
   union uuintchar u = { 0U };
   unsigned char* restrict c = (unsigned char* restrict) &a;

   U_INTERNAL_TRACE("u_random(%u)", a)

   for (; i < 4; ++i)
      {
      u.d[3] = rseq[c[0]] + rseq[c[1] + 1] + rseq[c[2] + 2] + rseq[c[3] + 3];
      u.d[2] = rseq[c[1]] + rseq[c[2] + 1] + rseq[c[3] + 2];
      u.d[1] = rseq[c[2]] + rseq[c[3] + 1];
      u.d[0] = rseq[c[3]];

      a = u.i;
      }

   return a;
}

/* Map hash value into number  0.0 <= n < 1.0

double u_randomd(uint32_t a)
{
   U_INTERNAL_TRACE("u_randomd(%u)", a)

   static double f = 1.0/4294967296.0;

   return f * u_random(a);
}   

'Folds' n-byte key into 4 byte key

uint32_t u_foldkey(unsigned char* k, uint32_t length)
{
   uint32_t ikey = 0, tkey, ishift = 0, fkey = 0;

   U_INTERNAL_TRACE("u_foldkey(%.*s,%u)", U_min(length,128), k, length)

   for (; ikey < length; ++ikey)
      {
      tkey  = (uint32_t) k[ikey];
      fkey ^= (tkey << ishift);

      ishift += 8;

      if (ishift >= 32) ishift = 0;
      }

   return fkey;
}

References: http://burtleburtle.net/bob/hash/doobs.html
--------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.
For every delta with one or two bits set, and the deltas of all three
  high bits or all three low bits, whether the original value of a,b,c
  is almost all zero or is uniformly distributed,
* If mix() is run forward or backward, at least 32 bits in a,b,c
  have at least 1/4 probability of changing.
* If mix() is run forward, every bit of c will change between 1/3 and
  2/3 of the time.  (Well, 22/100 and 78/100 for some 2-bit deltas.)
mix() was built out of 36 single-cycle latency instructions in a 
  structure that could supported 2x parallelism, like so:
      a -= b; 
      a -= c; x = (c>>13);
      b -= c; a ^= x;
      b -= a; x = (a<<8);
      c -= a; b ^= x;
      c -= b; x = (b>>13);
      ...
  Unfortunately, superscalar Pentiums and Sparcs can't take advantage 
  of that parallelism.  They've also turned some of those single-cycle
  latency instructions into multi-cycle latency instructions.  Still,
  this is the fastest good hash I could find.  There were about 2^^68
  to choose from.  I only looked at a billion or so.
--------------------------------------------------------------------

#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

--------------------------------------------------------------------
u_hash() -- hash a variable-length key into a 32-bit value
  k       : the key (the unaligned variable-length array of bytes)
  len     : the length of the key, counting by bytes
  initval : can be any 4-byte value
Returns a 32-bit value.  Every bit of the key affects every bit of
the return value.  Every 1-bit and 2-bit delta achieves avalanche.
About 6*len+35 instructions.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 32 bits,
use a bitmask.  For example, if you need only 10 bits, do
  h = (h & hashmask(10));
In which case, the hash table should have hashsize(10) elements.

If you are hashing n strings (ub1 **)k, do it like this:
  for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);

By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  You may use this
code any way you wish, private, educational, or commercial.  It's free.

See http://burtleburtle.net/bob/hash/evahash.html
Use for hash table lookup, or anything where one collision in 2^^32 is
acceptable.  Do NOT use for cryptographic purposes.
--------------------------------------------------------------------

uint32_t u_hash(unsigned char* k, uint32_t length, bool ignore_case)
{
   uint32_t a,b,c,len;

   U_INTERNAL_TRACE("u_hash(%.*s,%u,%b)", U_min(length,128), k, length, ignore_case)

   // Set up the internal state

   len = length;
   a = b = c = 0x9e3779b9;  // the golden ratio; an arbitrary value

   //---------------------------------------- handle most of the key

   if (ignore_case)
      {
      while (len >= 12)
         {
         a += (u_tolower(k[0]) + ((uint32_t)u_tolower(k[1]) << 8) + 
                                 ((uint32_t)u_tolower(k[2]) <<16) + ((uint32_t)u_tolower(k[3])<<24));
         b += (u_tolower(k[4]) + ((uint32_t)u_tolower(k[5]) << 8) +
                                 ((uint32_t)u_tolower(k[6]) <<16) + ((uint32_t)u_tolower(k[7])<<24));
         c += (u_tolower(k[8]) + ((uint32_t)u_tolower(k[9]) << 8) +
                                 ((uint32_t)u_tolower(k[10])<<16) + ((uint32_t)u_tolower(k[11])<<24));

         mix(a,b,c);

         k += 12; len -= 12;
         }

      //------------------------------------- handle the last 11 bytes
      c += length;

      switch (len)              // all the case statements fall through
         {
         case 11: c += ((uint32_t)u_tolower(k[10])<<24);
         case 10: c += ((uint32_t)u_tolower(k[9])<<16);
         case 9 : c += ((uint32_t)u_tolower(k[8])<<8);
         // the first byte of c is reserved for the length
         case 8 : b += ((uint32_t)u_tolower(k[7])<<24);
         case 7 : b += ((uint32_t)u_tolower(k[6])<<16);
         case 6 : b += ((uint32_t)u_tolower(k[5])<<8);
         case 5 : b += u_tolower(k[4]);
         case 4 : a += ((uint32_t)u_tolower(k[3])<<24);
         case 3 : a += ((uint32_t)u_tolower(k[2])<<16);
         case 2 : a += ((uint32_t)u_tolower(k[1])<<8);
         case 1 : a += u_tolower(k[0]);
         // case 0: nothing left to add
         }
      }
   else
      {
      while (len >= 12)
         {
         a += ((k[0]) + ((uint32_t)(k[1])<<8) + ((uint32_t)(k[2])<<16) + ((uint32_t)(k[3])<<24));
         b += ((k[4]) + ((uint32_t)(k[5])<<8) + ((uint32_t)(k[6])<<16) + ((uint32_t)(k[7])<<24));
         c += ((k[8]) + ((uint32_t)(k[9])<<8) + ((uint32_t)(k[10])<<16)+ ((uint32_t)(k[11])<<24));
         mix(a,b,c);
         k += 12; len -= 12;
         }

      //------------------------------------- handle the last 11 bytes
      c += length;

      switch (len)              // all the case statements fall through
         {
         case 11: c += ((uint32_t)(k[10])<<24);
         case 10: c += ((uint32_t)(k[9])<<16);
         case 9 : c += ((uint32_t)(k[8])<<8);
         // the first byte of c is reserved for the length
         case 8 : b += ((uint32_t)(k[7])<<24);
         case 7 : b += ((uint32_t)(k[6])<<16);
         case 6 : b += ((uint32_t)(k[5])<<8);
         case 5 : b += (k[4]);
         case 4 : a += ((uint32_t)(k[3])<<24);
         case 3 : a += ((uint32_t)(k[2])<<16);
         case 2 : a += ((uint32_t)(k[1])<<8);
         case 1 : a += (k[0]);
         // case 0: nothing left to add
         }
      }

   mix(a,b,c);
   //-------------------------------------------- report the result

   return c;
}

// the famous DJB hash function for strings

uint32_t u_hash(unsigned char* t, uint32_t tlen, bool ignore_case)
{
   uint32_t h = 5381;

   U_INTERNAL_TRACE("u_hash(%.*s,%u,%d)", U_min(tlen,128), t, tlen, ignore_case)

   if (ignore_case) while (tlen--) h = ((h << 5) + h) + u_tolower(*t++);
   else             while (tlen--) h = ((h << 5) + h) +           *t++;

   h &= ~(1 << 31); // strip the highest bit

   return h;
}
*/

/* 64 bit Fowler/Noll/Vo-0 hash code
 *
 * Fowler/Noll/Vo hash
 *
 * The basis of this hash algorithm was taken from an idea sent
 * as reviewer comments to the IEEE POSIX P1003.2 committee by:
 *
 *      Phong Vo (http://www.research.att.com/info/kpv/)
 *      Glenn Fowler (http://www.research.att.com/~gsf/)
 *
 * In a subsequent ballot round:
 *
 *      Landon Curt Noll (http://www.isthe.com/chongo/)
 *
 * improved on their algorithm.  Some people tried this hash
 * and found that it worked rather well.  In an EMail message
 * to Landon, they named it the ``Fowler/Noll/Vo'' or FNV hash.
 *
 * FNV hashes are designed to be fast while maintaining a low
 * collision rate. The FNV speed allows one to quickly hash lots
 * of data while maintaining a reasonable collision rate.  See:
 *
 *      http://www.isthe.com/chongo/tech/comp/fnv/index.html
 *
 * for more details as well as other forms of the FNV hash.
 *
 ***
 * Please do not copyright this code. This code is in the public domain.
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * By:
 * chongo <Landon Curt Noll> /\oo/\
 *      http://www.isthe.com/chongo/
 *
 * Share and Enjoy! :-)
*/

#define FNV_32_INIT  ((uint32_t)0x811c9dc5)
#define FNV_64_INIT  ((uint64_t)0xcbf29ce484222325ULL)

#define FNV_32_PRIME ((uint32_t)0x01000193)
#define FNV_64_PRIME ((uint64_t)0x100000001b3ULL)

uint32_t u_hash(unsigned char* restrict bp, uint32_t len, bool ignore_case)
{
   uint32_t hval              = FNV_32_INIT;
   unsigned char* restrict be = bp + len; /* beyond end of buffer */

   U_INTERNAL_TRACE("u_hash(%.*s,%u)", U_min(len,128), bp, len)

   /* FNV-1 hash each octet of the buffer */

   if (ignore_case)
      {
      while (bp < be)
         {
         /* xor the bottom with the current octet */

         hval ^= (uint32_t) u_tolower(*bp++);

         /* multiply by the 32 bit FNV magic prime mod 2^32 */

#     ifdef NO_FNV_GCC_OPTIMIZATION
         hval *= FNV_32_PRIME;
#     else
         hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#     endif
         }
      }
   else
      {
      while (bp < be)
         {
         /* xor the bottom with the current octet */

         hval ^= (uint32_t) *bp++;

         /* multiply by the 32 bit FNV magic prime mod 2^32 */

#     ifdef NO_FNV_GCC_OPTIMIZATION
         hval *= FNV_32_PRIME;
#     else
         hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#     endif
         }
      }

   return hval; /* return our new hash value */
}

/*
uint64_t u_hash64(unsigned char* bp, uint32_t len)
{
   uint64_t hval     = FNV_64_INIT;
   unsigned char* be = bp + len; // beyond end of buffer

   U_INTERNAL_TRACE("u_hash64(%.*s,%u)", U_min(len,128), bp, len)

   // FNV-1 hash each octet of the buffer

   while (bp < be)
      {
      // multiply by the 64 bit FNV magic prime mod 2^64

#  ifdef NO_FNV_GCC_OPTIMIZATION
      hval *= FNV_64_PRIME;
#  else
      hval += (hval<<1) + (hval<<4) + (hval<<5) + (hval<<7) + (hval<<8) + (hval<<40);
#  endif

      hval ^= (uint64_t)*bp++; // xor the bottom with the current octet
      }

   return hval; // return our new hash value
}
*/

#ifdef HAVE_ARCH64

uint32_t u_random64(uint64_t ptr)
{
   uint32_t a = (uint32_t)((ptr & 0xffffffff00000000) >> 32L),
            b = (uint32_t)((ptr & 0x00000000ffffffff));

   U_INTERNAL_TRACE("u_random64(%llu)", ptr)

   return u_random(a) + u_random(b);
}

#endif
