// ============================================================================
//
// = LIBRARY
//    ulibase - c++ library
//
// = FILENAME
//    cgzio.c
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

// #define DEBUG_DEBUG

#include <ulib/base/coder/gzio.h>

#include <zlib.h>

/*
Synopsis: Compress and Decompresses the source buffer into the destination buffer
*/

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define CONTINUATION 0x02 /* bit 1 set: continuation of multi-part gzip file */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define ENCRYPTED    0x20 /* bit 5 set: file is encrypted */
#define RESERVED     0xC0 /* bit 6,7:   reserved */

/*
#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO         (-1)
#define Z_STREAM_ERROR  (-2)
#define Z_DATA_ERROR    (-3)
#define Z_MEM_ERROR     (-4)
#define Z_BUF_ERROR     (-5)
#define Z_VERSION_ERROR (-6)
*/

uint32_t u_gz_deflate(const char* input, uint32_t len, char* result)
{
   z_stream stream;
   int r, compression_window;
   unsigned required_window = len;

   U_INTERNAL_TRACE("u_gz_deflate(%p,%u,%p)", input, len, result)

   U_INTERNAL_ASSERT_POINTER(input)

   stream.next_in   = (unsigned char*)input;
   stream.avail_in  = len;
   stream.next_out  = (unsigned char*)result;
   stream.avail_out = 0xffff0000;

   stream.zalloc    = (alloc_func)Z_NULL;
   stream.zfree     = (free_func)Z_NULL;
   stream.opaque    = Z_NULL;

   // don't use the 8 bit window due a bug in the zlib 1.1.3 and previous

   if      (required_window <=   512) compression_window =  9;
   else if (required_window <=  1024) compression_window = 10;
   else if (required_window <=  2048) compression_window = 11;
   else if (required_window <=  4096) compression_window = 12;
   else if (required_window <=  8192) compression_window = 13;
   else if (required_window <= 16384) compression_window = 14;
   else                               compression_window = 15;

   if (compression_window > MAX_WBITS) compression_window = MAX_WBITS;

   /* windowBits is passed < 0 to suppress the zlib header */

   r = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -compression_window, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);

   U_INTERNAL_PRINT("deflateInit2() = (%d, %s)", r, stream.msg)

   if (r != Z_OK) return 0;

   r = deflate(&stream, Z_FINISH);

   U_INTERNAL_PRINT("deflate() = (%d, %s)", r, stream.msg)

   if (r != Z_STREAM_END) stream.total_out = 0;

   deflateEnd(&stream);

   return stream.total_out;
}

uint32_t u_gz_inflate(const char* input, uint32_t len, char* result)
{
   int r;
   z_stream stream;

   U_INTERNAL_TRACE("u_gz_inflate(%p,%u,%p)", input, len, result)

   U_INTERNAL_ASSERT_POINTER(input)

   if (memcmp(input, U_CONSTANT_TO_PARAM(GZIP_MAGIC)) == 0)
      {
      int header_size;
      const char* ptr = input + U_CONSTANT_SIZE(GZIP_MAGIC);
      char flags, method = *ptr++; // method

      if (method != Z_DEFLATED) // 8
         {
         U_WARNING("u_gz_inflate(): unknown method %d -- not supported", method);

         return 0;
         }

      flags = *ptr++; // compression flags

      if ((flags & ENCRYPTED) != 0)
         {
         U_WARNING("u_gz_inflate(): file is encrypted -- not supported", 0);

         return 0;
         }

      if ((flags & CONTINUATION) != 0)
         {
         U_WARNING("u_gz_inflate(): file is a a multi-part gzip file -- not supported", 0);

         return 0;
         }

      if ((flags & RESERVED) != 0)
         {
         U_WARNING("u_gz_inflate(): has flags 0x%x -- not supported", flags);

         return 0;
         }

      // timestamp: 4 
      // extra flags: 1
      // OS type: 1

      ptr += 4 + 1 + 1;

      if ((flags & EXTRA_FIELD) != 0)
         {
         unsigned len  =  (unsigned)*ptr++;
                  len |= ((unsigned)*ptr++) << 8;

         input += len;
         }

      if ((flags & ORIG_NAME) != 0) while (*ptr++ != '\0'); // Discard file name if any
      if ((flags & COMMENT)   != 0) while (*ptr++ != '\0'); // Discard file comment if any

      header_size = (ptr - input);

      U_INTERNAL_PRINT("header_size = %d *ptr = '%o'", header_size, *(unsigned char*)ptr)

      len  -= header_size + 8; // crc + size_original
      input = ptr;
      }

   stream.next_in   = (unsigned char*)input;
   stream.avail_in  = len;
   stream.next_out  = (unsigned char*)result;
   stream.avail_out = 0xffff0000;

   stream.zalloc    = (alloc_func)Z_NULL;
   stream.zfree     = (free_func)Z_NULL;
   stream.opaque    = Z_NULL;

   /* !! ZLIB UNDOCUMENTED FEATURE !!
    * windowBits is passed < 0 to tell that there is no zlib header.
    * Note that in this case inflate *requires* an extra "dummy" byte after the compressed stream in order
    * to complete decompression and return Z_STREAM_END
    */

   r = inflateInit2(&stream, -15);

   U_INTERNAL_PRINT("inflateInit2() = (%d, %s)", r, stream.msg)

   if (r != Z_OK) return 0;

   r = inflate(&stream, Z_SYNC_FLUSH);

   U_INTERNAL_PRINT("inflate() = (%d, %s)", r, stream.msg)

#if !defined(ZLIB_VERNUM) || ZLIB_VERNUM < 0x1200
   /* zlib < 1.2.0 workaround: push a dummy byte at the end of the stream when inflating (see zlib ChangeLog)
    * The zlib code effectively READ the dummy byte, this imply that the pointer MUST point to a valid data region.
    * The dummy byte is not always needed, only if inflate return Z_OK instead of Z_STREAM_END
    */
   if (r == Z_OK)
      {
      /* dummy byte */

      unsigned char dummy = 0;

      stream.next_in  = &dummy;
      stream.avail_in = 1;

      r = inflate(&stream, Z_SYNC_FLUSH);

      U_INTERNAL_PRINT("inflate() = (%d, %s)", r, stream.msg)
      }
#endif

   if (r != Z_STREAM_END)
      {
      inflateEnd(&stream);

      return 0;
      }

   r = inflateEnd(&stream);

   U_INTERNAL_PRINT("inflateEnd() = (%d, %s)", r, stream.msg)

   if (r != Z_OK) return 0;

   if (stream.total_in != len) // || stream.total_out != out_size)
      {
      U_INTERNAL_PRINT("stream.total_in = %u len = %u", stream.total_in, len)

      return 0;
      }

   return stream.total_out;
}
