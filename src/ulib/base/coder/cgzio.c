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

#ifdef DEBUG_DEBUG
static const char* get_error_string(int err)
{
   switch (err)
      {
      case Z_NEED_DICT:     return "Need dict.";
      case Z_ERRNO:         return "Errno";
      case Z_STREAM_ERROR:  return "Stream error";
      case Z_DATA_ERROR:    return "Data error";
      case Z_MEM_ERROR:     return "Memory error";
      case Z_BUF_ERROR:     return "Buffer error";
      case Z_VERSION_ERROR: return "Version error";
      }

   return "unknown";
}
#endif

#ifdef U_ZLIB_DEFLATE_WORKSPACESIZE
static int workspacesize = zlib_deflate_workspacesize();
static char workspace[workspacesize];
#else
#define zlib_inflate(a,b)  inflate(a,b)
#define zlib_deflate(a,b)  deflate(a,b)
#define zlib_inflateEnd(a) inflateEnd(a)
#define zlib_deflateEnd(a) deflateEnd(a)
#endif

uint32_t u_gz_deflate(const char* input, uint32_t len, char* result)
{
   int err;
   z_stream stream;

   U_INTERNAL_TRACE("u_gz_deflate(%p,%u,%p)", input, len, result)

   U_INTERNAL_ASSERT_POINTER(input)

   (void) memset(&stream, 0, sizeof(z_stream));

#ifdef U_ZLIB_DEFLATE_WORKSPACESIZE
   (void) memset((void*)workspace, 0, workspacesize);

   stream.workspace = workspace; /* Set the workspace */
#endif

   /* -MAX_WBITS: suppresses zlib header & trailer */

   err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);

   if (err != Z_OK)
      {
      U_INTERNAL_PRINT("deflateInit2() = (%d, %s)", err, get_error_string(err))

      return 0;
      }

   stream.next_in  = (unsigned char*)input;
   stream.avail_in = len;

   do {
      /* Set up output buffer */

      stream.next_out  = (unsigned char*)result + stream.total_out;
      stream.avail_out = 0xffff0000;

      err = zlib_deflate(&stream, Z_FINISH);

      switch (err)
         {
         case Z_OK: break;

         case Z_STREAM_END:
            {
            err = zlib_deflateEnd(&stream);

            if (err != Z_OK)
               {
               U_INTERNAL_PRINT("zlib_deflateEnd() = (%d, %s)", err, get_error_string(err))

               return 0;
               }

            U_INTERNAL_PRINT("stream.total_in = %u len = %u", stream.total_in, len)

            U_INTERNAL_ASSERT_EQUALS(stream.total_in, len)

            return stream.total_out;
            }

         default:
            {
            U_INTERNAL_PRINT("zlib_deflate() = (%d, %s)", err, get_error_string(err))

            return 0;
            }
         }
   } while (stream.avail_out == 0);

   return 0;
}

/* gzip flag byte */

#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define CONTINUATION 0x02 /* bit 1 set: continuation of multi-part gzip file */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define ENCRYPTED    0x20 /* bit 5 set: file is encrypted */
#define RESERVED     0xC0 /* bit 6,7:   reserved */

uint32_t u_gz_inflate(const char* input, uint32_t len, char* result)
{
   int err;
   z_stream stream;

   U_INTERNAL_TRACE("u_gz_inflate(%p,%u,%p)", input, len, result)

   U_INTERNAL_ASSERT_POINTER(input)

   (void) memset(&stream, 0, sizeof(z_stream));

#ifdef U_ZLIB_DEFLATE_WORKSPACESIZE
   (void) memset((void*)workspace, 0, workspacesize);

   stream.workspace = workspace;
#endif

   /* !! ZLIB UNDOCUMENTED FEATURE !!
    * windowBits is passed < 0 to tell that there is no zlib header.
    * Note that in this case inflate *requires* an extra "dummy" byte after the compressed stream in order
    * to complete decompression and return Z_STREAM_END
    */

   err = inflateInit2(&stream, -MAX_WBITS);

   if (err != Z_OK)
      {
      U_INTERNAL_PRINT("inflateInit2() = (%d, %s)", err, get_error_string(err))

      return 0;
      }

   if (memcmp(input, U_CONSTANT_TO_PARAM(GZIP_MAGIC)) == 0)
      {
      int header_size;
      const char* ptr = input + U_CONSTANT_SIZE(GZIP_MAGIC);
      char flags, method = *ptr++; /* method */

      if (method != Z_DEFLATED) /* 8 */
         {
         U_WARNING("u_gz_inflate(): unknown method %d -- not supported", method);

         return 0;
         }

      flags = *ptr++; /* compression flags */

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

      /* timestamp:   4
       * extra flags: 1
       * OS type:     1
       */

      ptr += 4 + 1 + 1;

      if ((flags & EXTRA_FIELD) != 0)
         {
         unsigned len  =  (unsigned)*ptr++;
                  len |= ((unsigned)*ptr++) << 8;

         input += len;
         }

      if ((flags & ORIG_NAME) != 0) while (*ptr++ != '\0') {} /* Discard file name if any */
      if ((flags & COMMENT)   != 0) while (*ptr++ != '\0') {} /* Discard file comment if any */

      header_size = (ptr - input);

      U_INTERNAL_PRINT("header_size = %d *ptr = '%o'", header_size, *(unsigned char*)ptr)

      len  -= header_size + 8; /* crc + size_original */
      input = ptr;
      }

   stream.next_in  = (unsigned char*)input;
   stream.avail_in = len;

   do {
      /* Set up output buffer */

      stream.next_out  = (unsigned char*)result + stream.total_out;
      stream.avail_out = 0xffff0000;

      err = zlib_inflate(&stream, Z_SYNC_FLUSH);

      switch (err)
         {
         case Z_OK:
            {
#        if !defined(ZLIB_VERNUM) || ZLIB_VERNUM < 0x1200
            /* zlib < 1.2.0 workaround: push a dummy byte at the end of the stream when inflating (see zlib ChangeLog)
            * The zlib code effectively READ the dummy byte, this imply that the pointer MUST point to a valid data region.
            * The dummy byte is not always needed, only if inflate return Z_OK instead of Z_STREAM_END
            */
            unsigned char dummy = 0; /* dummy byte */

            stream.next_in  = &dummy;
            stream.avail_in = 1;

            err = inflate(&stream, Z_SYNC_FLUSH);

            U_INTERNAL_PRINT("inflate() = (%d, %s)", err, get_error_string(err))
#        endif
            }
         break;

         case Z_STREAM_END:
            {
            err = zlib_inflateEnd(&stream);

            if (err != Z_OK)
               {
               U_INTERNAL_PRINT("zlib_inflateEnd() = (%d, %s)", err, get_error_string(err))

               return 0;
               }

            U_INTERNAL_PRINT("stream.total_in = %u len = %u", stream.total_in, len)

            U_INTERNAL_ASSERT_EQUALS(stream.total_in, len)

            return stream.total_out;
            }

         default:
            {
            U_INTERNAL_PRINT("zlib_inflate() = (%d, %s)", err, get_error_string(err))

            return 0;
            }
         }
   } while (stream.avail_out == 0);

   return 0;
}
