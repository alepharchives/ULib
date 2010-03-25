// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    file.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file.h>
#include <ulib/tokenizer.h>
#include <ulib/container/tree.h>
#include <ulib/utility/services.h>
#include <ulib/utility/string_ext.h>

#ifdef HAVE_MAGIC
#  include <ulib/magic/magic.h>
#endif

int         UFile::mime_index;
uint32_t    UFile::cwd_len_save;
const char* UFile::cwd_save;

#ifdef DEBUG
int  UFile::num_file_object;

void UFile::inc_num_file_object() { ++num_file_object; }
void UFile::dec_num_file_object() { --num_file_object; }
void UFile::chk_num_file_object() { if (num_file_object) U_WARNING("UFile::chdir() with num file object = %d", num_file_object); }
#endif

typedef struct mimeentry {
   const char* name;
   uint32_t name_len;
   const char* type;
} mimeentry;

#define ENTRY(name,type) { name, U_CONSTANT_SIZE(name), type }

static struct mimeentry mimetab[] = {
   ENTRY( "html",    "text/html" ),
   ENTRY( "htm",     "text/html" ),

   // NB: need to stay here...
   // -----------------------------------
   ENTRY( "css",     "text/css" ),           // 2 U_css
   ENTRY( "js",      "text/javascript" ),    // 3 U_js
   // -----------------------------------

   ENTRY( "ico",     "image/x-icon" ),

   // The certificate being downloaded represents a Certificate Authority.
   // When it is downloaded the user will be shown a sequence of dialogs that
   // will guide them through the process of accepting the Certificate Authority
   // and deciding if they wish to trust sites certified by the CA. If a certificate
   // chain is being imported then the first certificate in the chain must be the CA
   // certificate, and any subsequent certificates will be added as untrusted CA
   // certificates to the local database.
   // -------------------------------------------------------------------------------
   ENTRY( "crt",     "application/x-x509-ca-cert" ),
   ENTRY( "cer",     "application/x-x509-ca-cert" ),
   ENTRY( "der",     "application/x-x509-ca-cert" ),
   // -------------------------------------------------------------------------------

   ENTRY( "xml",     "application/xml" ),
   ENTRY( "dtd",     "application/xml" ),

   ENTRY( "txt",     "text/plain" ),
   ENTRY( "text",    "text/plain" ),
   ENTRY( "md5",     "text/plain" ),

   ENTRY( "ps",      "application/postscript" ),
   ENTRY( "pdf",     "application/pdf" ),

   ENTRY( "gif",     "image/gif" ),
   ENTRY( "png",     "image/png" ),
   ENTRY( "jpg",     "image/jpeg" ),
   ENTRY( "jpeg",    "image/jpeg" ),
   ENTRY( "xpm",     "image/x-xpixmap" ),
   ENTRY( "xbm",     "image/x-xbitmap" ),

   ENTRY( "mp3",     "audio/mpeg" ),
   ENTRY( "wav",     "audio/x-wav" ),

   ENTRY( "mpg",     "video/mpeg" ),
   ENTRY( "mp4",     "video/mp4" ),
   ENTRY( "m4a",     "audio/mp4" ),
   ENTRY( "mpeg",    "video/mpeg" ),
   ENTRY( "ogg",     "application/ogg" ),
   ENTRY( "swf",     "application/x-shockwave-flash" ),
   ENTRY( "wmv",     "video/x-ms-wmv" ),
   ENTRY( "avi",     "video/x-msvideo" ),
   ENTRY( "mov",     "video/quicktime" ),

   ENTRY( "zip",     "application/zip" ),
   ENTRY( "tar",     "application/x-tar" ),
   ENTRY( "rar",     "application/x-rar-compressed" ),

// ENTRY( "dvi",     "application/x-dvi" ),
// ENTRY( "bild",    "image/jpeg" ),
// ENTRY( "qt",      "video/quicktime" ),
// ENTRY( "pac",     "application/x-ns-proxy-autoconfig" ),
// ENTRY( "sig",     "application/pgp-signature" ),
// ENTRY( "torrent", "application/x-bittorrent" ),
// ENTRY( "rss",     "application/rss+xml" ),
// ENTRY( "class",   "application/octet-stream" ),
// ENTRY( "7z",      "application/x-7z-compressed" ),
// ENTRY( "xwd",     "image/x-xwindowdump" ),
// ENTRY( "m3u",     "audio/x-mpegurl" ),
// ENTRY( "nzb",     "application/x-nzb" ),
   { 0, 0, 0 }
};

#ifndef MAP_POPULATE // (since Linux 2.5.46)
#define MAP_POPULATE 0
#endif

#ifndef MREMAP_MAYMOVE
#define MREMAP_MAYMOVE 1
#endif

void UFile::setPathRelativ()
{
   U_TRACE(0, "UFile::setPathRelativ()")

   U_CHECK_MEMORY

   U_ASSERT_EQUALS(pathname.empty(),false)

   char c = pathname.c_char(0);

   if (c == '~' ||
       c == '$')
      {
      pathname = UStringExt::expandPath(pathname);
      }

   // NB: la stringa potrebbe non essere scrivibile...!!!!

   path_relativ_len = pathname.size();
   path_relativ     = u_getPathRelativ(pathname.c_str(), &path_relativ_len);

   U_INTERNAL_ASSERT_MAJOR(path_relativ_len,0)

   /* NB: la stringa potrebbe non essere scrivibile...!!!!

   path_relativ[path_relativ_len] = '\0';
   */

   U_INTERNAL_DUMP("u_cwd(%u)        = %.*S", u_cwd_len, u_cwd_len, u_cwd)
   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   reset();
}

// gcc - call is unlikely and code size would grow

void UFile::setPath(const UString& path)
{
   U_TRACE(0, "UFile::setPath(%.*S)", U_STRING_TO_TRACE(path))

   pathname = path;

   setPathRelativ();
}

bool UFile::open(int flags)
{
   U_TRACE(0, "UFile::open(%d)", flags)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_EQUALS(fd, -1)
   U_INTERNAL_ASSERT_POINTER(path_relativ)
   U_INTERNAL_ASSERT_MAJOR(path_relativ_len, 0)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   fd = UFile::open(path_relativ, flags, PERM_FILE);

   U_RETURN(fd != -1);
}

bool UFile::creat(const UString& path, int flags, mode_t mode)
{
   U_TRACE(0, "UFile::creat(%.*S,%d,%d)", U_STRING_TO_TRACE(path), flags, mode)

   setPath(path);

   return creat(flags, mode);
}

bool UFile::creat(int flags, mode_t mode)
{
   U_TRACE(0, "UFile::creat(%d,%d)", flags, mode)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_EQUALS(fd, -1)
   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   fd = UFile::open(path_relativ, flags | O_CREAT, mode);

   U_RETURN(fd != -1);
}

bool UFile::chdir(const char* path, bool flag_save)
{
   U_TRACE(1, "UFile::chdir(%S,%b)", path, flag_save)

   chk_num_file_object();

   U_INTERNAL_DUMP("u_cwd(%u)    = %.*S", u_cwd_len, u_cwd_len, u_cwd)

   if (path)
      {
      if (flag_save)
         {
         if (IS_DIR_SEPARATOR(u_cwd[0]) == false) u_getcwd();

         cwd_save     = u_cwd;
         cwd_len_save = u_cwd_len;
         }
      }
   else
      {
      U_INTERNAL_ASSERT_POINTER(cwd_save)

      U_INTERNAL_ASSERT_MAJOR(cwd_len_save,0)

     ((char*)cwd_save)[cwd_len_save] = '\0';
      path = cwd_save;

      U_INTERNAL_DUMP("path = %S", path)
      }

   U_INTERNAL_DUMP("cwd_save(%u) = %.*S", cwd_len_save, cwd_len_save, cwd_save)

   bool result = (U_SYSCALL(chdir, "%S", U_PATH_CONV(path)) != -1);

   if (result)
      {
      if (path == cwd_save)
         {
         const char* tmp = u_cwd;

         u_cwd     = cwd_save;
         u_cwd_len = cwd_len_save;

         cwd_save     = tmp;
         cwd_len_save = 0;
         }
      else
         {
         u_cwd     = path;
         u_cwd_len = u_strlen(path);

         if (IS_DIR_SEPARATOR(u_cwd[0]) == false) u_getcwd();
         }
      }

   U_INTERNAL_DUMP("u_cwd(%u)    = %.*S", u_cwd_len, u_cwd_len, u_cwd)

   U_RETURN(result);
}

uint32_t UFile::setPathFromFile(const UFile& file, char* buffer_path, const char* suffix, uint32_t len)
{
   U_TRACE(1, "UFile::setPathFromFile(%p,%p,%.*S,%u)", &file, buffer_path, len, suffix, len)

   U_INTERNAL_DUMP("file.path_relativ(%u) = %.*S", file.path_relativ_len, file.path_relativ_len, file.path_relativ)

   (void) U_SYSCALL(memcpy, "%p,%p,%u", buffer_path,                         file.path_relativ, file.path_relativ_len);
   (void) U_SYSCALL(memcpy, "%p,%p,%u", buffer_path + file.path_relativ_len,            suffix,                   len);

   uint32_t new_path_relativ_len = file.path_relativ_len + len;

   U_INTERNAL_ASSERT_MINOR(new_path_relativ_len,(int32_t)MAX_FILENAME_LEN)

   buffer_path[new_path_relativ_len] = '\0';

   U_INTERNAL_DUMP("buffer_path(%u)       = %S", new_path_relativ_len, buffer_path)

   U_RETURN(new_path_relativ_len);
}

void UFile::setPath(const UFile& file, char* buffer_path, const char* suffix, uint32_t len)
{
   U_TRACE(1, "UFile::setPath(%p,%p,%.*S,%u)", &file, buffer_path, len, suffix, len)

   U_INTERNAL_DUMP("u_cwd             = %S", u_cwd)
   U_INTERNAL_DUMP("cwd_save          = %.*S", cwd_len_save, cwd_save)

   reset();

   if (IS_DIR_SEPARATOR(file.path_relativ[0]) == false && cwd_len_save) (void) U_SYSCALL(chdir, "%S", U_PATH_CONV(cwd_save)); // for IR...

   path_relativ_len = file.path_relativ_len + len;

   if (buffer_path == 0)
      {
      pathname.setBuffer(path_relativ_len);
      pathname.size_adjust(path_relativ_len);

      buffer_path = pathname.data();
      }

   path_relativ = buffer_path;

   (void) setPathFromFile(file, buffer_path, suffix, len);

   U_INTERNAL_DUMP("path_relativ = %.*S", path_relativ_len, path_relativ)
}

off_t UFile::size(bool bstat)
{
   U_TRACE(0, "UFile::size(%b)", bstat)

   U_INTERNAL_ASSERT_EQUALS(st_size, 0)

   if (bstat)
      {
      fstat();

#  ifdef __MINGW32__
      st_ino = u_get_inode(fd);
#  endif

      U_INTERNAL_DUMP("st_ino = %llu", st_ino)
      }
   else
      {
      readSize();
      }

   U_RETURN(st_size);
}

bool UFile::memmap(int prot, UString* str, off_t offset, size_t length)
{
   U_TRACE(0, "UFile::memmap(%d,%p,%I,%lu)", prot, str, offset, length)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   U_INTERNAL_ASSERT_DIFFERS(fd,-1)
   U_INTERNAL_ASSERT_MAJOR(st_size,0)
   U_INTERNAL_ASSERT_EQUALS((offset % PAGESIZE),0) // offset should be a multiple of the page size as returned by getpagesize(2)
#ifdef __MINGW32__
   U_INTERNAL_ASSERT((off_t)length <= st_size)     // Don't allow mappings beyond EOF since Windows can't handle that POSIX like
#endif

   map_size = (length ? length : (st_size - offset));

#ifndef HAVE_ARCH64
   U_INTERNAL_ASSERT_RANGE(1UL,map_size,3UL*1024UL*1024UL*1024UL) // limit of linux system
#endif

   map = UFile::mmap(map_size, fd, prot, MAP_SHARED | MAP_POPULATE, offset);

   if (map != MAP_FAILED)
      {
#  if defined(MADV_SEQUENTIAL)
      if (map_size > (128 * PAGESIZE)) (void) U_SYSCALL(madvise, "%p,%I,%d", map, map_size, MADV_SEQUENTIAL);
#  endif

      if (str) str->mmap(map, map_size);

      U_RETURN(true);
      }

   U_RETURN(false);
}

void UFile::munmap()
{
   U_TRACE(0, "UFile::munmap()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   U_INTERNAL_ASSERT_MAJOR(map_size,0UL)
   U_INTERNAL_ASSERT_DIFFERS(map,MAP_FAILED)

   UFile::munmap(map, map_size);

   map      = (char*)MAP_FAILED;
   map_size = 0;
}

void UFile::msync(char* ptr, char* page, int flags)
{
   U_TRACE(1, "UFile::msync(%p,%p,%d)", ptr, page, flags)

   void* start = page - ((long)page % PAGESIZE);

   int32_t length = (ptr - page) / PAGESIZE;

   if (length <= 0) length = 1;

   (void) U_SYSCALL(msync, "%p,%u,%d", (char*)start, length * PAGESIZE, flags);
}

UString UFile::getContent(bool brdonly, bool bstat, bool bmap)
{
   U_TRACE(1, "UFile::getContent(%b,%b,%b)", brdonly, bstat, bmap)

   if (isOpen()                          == false &&
       open(brdonly ? O_RDONLY : O_RDWR) == false)
      {
      U_RETURN_STRING(UString::getStringNull());
      }

   UString fileContent;

   if (st_size     == 0 &&
       size(bstat) == 0)
      {
      goto end;
      }

   U_INTERNAL_DUMP("fd = %d map = %p map_size = %lu st_size = %I", fd, map, map_size, st_size)

   if (bmap ||
       st_size > (off_t)U_CAPACITY)
      {
      int                   prot  = PROT_READ;
      if (brdonly == false) prot |= PROT_WRITE;

      (void) memmap(prot, &fileContent);
      }
   else
      {
      UString tmp(st_size);

      char* ptr = tmp.data();

      ssize_t value = U_SYSCALL(pread, "%d,%p,%u,%I", fd, ptr, st_size, 0);

      if (value < 0) value = 0;

      ptr[value] = '\0'; // NB: in this way we can use string function data()...

      tmp.size_adjust(value);

      fileContent = tmp;
      }

end:
   UFile::close();

   U_RETURN_STRING(fileContent);
}

UString UFile::contentOf(const char* pathname, int flags, bool bstat, bool bmap)
{
   U_TRACE(0, "UFile::contentOf(%S,%d,%b,%b)", pathname, flags, bstat, bmap)

   UFile file;

   file.reset();

   if (file.open(pathname, flags)) return file.getContent((((flags & O_RDWR) | (flags & O_WRONLY)) == 0), bstat, bmap);

   U_RETURN_STRING(UString::getStringNull());
}

UString UFile::contentOf(const UString& pathname, int flags, bool bstat, bool bmap)
{
   U_TRACE(0, "UFile::contentOf(%.*S,%d,%b,%b)", U_STRING_TO_TRACE(pathname), flags, bstat, bmap)

   return UFile(pathname).getContent((((flags & O_RDWR) | (flags & O_WRONLY)) == 0), bstat, bmap);
}

const char* UFile::getMimeType()
{
   U_TRACE(0, "UFile::getMimeType()")

   const char* content_type = 0;

#ifdef HAVE_MAGIC
   if (map               == MAP_FAILED &&
       memmap(PROT_READ) == false)
      {
      goto end;
      }

   content_type = UMagic::getType(map, map_size).data();
#else
   const char* ptr = getSuffix();

   if (ptr)
      {
      U_INTERNAL_ASSERT_EQUALS(ptr[0], '.')

      content_type = getMimeType(ptr+1);
      }
#endif

end:
   U_RETURN(content_type);
}

const char* UFile::getMimeType(const char* suffix)
{
   U_TRACE(0, "UFile::getMimeType(%S)", suffix)

   U_INTERNAL_ASSERT_POINTER(suffix)

   const char* content_type = 0;

   for (mime_index = 0; mimetab[mime_index].name; ++mime_index)
      {
      if (u_endsWith(suffix, mimetab[mime_index].name_len, mimetab[mime_index].name, mimetab[mime_index].name_len))
         {
         content_type = mimetab[mime_index].type;

         break;
         }
      }

   U_RETURN(content_type);
}

bool UFile::write(const UString& data, bool append)
{
   U_TRACE(1, "UFile::write(%.*S,%b)", U_STRING_TO_TRACE(data), append)

   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   bool esito = isOpen();

   if (esito == false)
      {
      int flags = O_RDWR | (append ? O_APPEND : O_TRUNC);

      esito = creat(flags, PERM_FILE);
      }

   if (esito)
      {
      uint32_t sz = data.size();

      if (sz > PAGESIZE)
         {
         off_t offset   = (append ? size() : 0);
         uint32_t resto = 0;

         esito = fallocate(offset + sz);

         if (esito == false) U_WARNING("no more space on disk for size %I", offset + sz);
         else
            {
            if (offset)
               {
               resto   = offset % PAGESIZE;
               offset -= resto;

               U_INTERNAL_DUMP("resto = %u", resto)
               }

            esito = memmap(PROT_READ | PROT_WRITE, 0, offset);

            if (esito)
               {
               (void) U_SYSCALL(memcpy, "%p,%p,%u", map + resto, data.data(), sz);

               munmap();
               }
            }
         }
      else
         {
         if (sz) esito = UFile::write(fd, data.data(), sz);
         }
      }

   U_RETURN(esito);
}

bool UFile::writeTo(const UString& path, const UString& data, bool append)
{
   U_TRACE(0, "UFile::writeTo(%.*S,%.*S,%b)", U_STRING_TO_TRACE(path), U_STRING_TO_TRACE(data), append)

   UFile tmp(path);

   bool result = tmp.write(data, append);

   if (tmp.isOpen()) tmp.close();

   U_RETURN(result);
}

bool UFile::writeToTmpl(const char* tmpl, const UString& data, bool append)
{
   U_TRACE(0+256, "UFile::writeToTmpl(%S,%.*S,%b)", tmpl, U_STRING_TO_TRACE(data), append)

   bool result = false;

   if (data.empty() == false)
      {
      UString path(U_CAPACITY);

      path.snprintf(tmpl, 0);

      result = UFile::writeTo(path, data, append);
      }

   U_RETURN(result);
}

bool UFile::lock(short l_type, off_t start, off_t len) const
{
   U_TRACE(1, "UFile::lock(%d,%I,%I)", l_type, start, len)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_DIFFERS(fd, -1)
   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   /*
   struct flock {
      short l_type;    // Type of lock: F_RDLCK, F_WRLCK, F_UNLCK
      short l_whence;  // How to interpret l_start: SEEK_SET, SEEK_CUR, SEEK_END
      off_t l_start;   // Starting offset for lock
      off_t l_len;     // Number of bytes to lock
      pid_t l_pid;     // PID of process blocking our lock (F_GETLK only)
   };
   */

   struct flock flock = { l_type, SEEK_SET, start, len, u_pid };

   /*
   F_SETLK: Acquire a lock (when l_type is F_RDLCK or F_WRLCK) or release a lock (when l_type is F_UNLCK) on the
            bytes specified by the l_whence, l_start, and l_len fields of lock. If a conflicting lock is held by another
            process, this call returns -1  and  sets errno to EACCES or EAGAIN.

   F_SETLKW: As for F_SETLK, but if a conflicting lock is held on the file, then wait for that lock to be released.
             If a signal is caught while waiting, then the call is interrupted and (after the signal handler has returned)
             returns immediately (with return value -1 and errno set to EINTR).
   */

   bool result = (U_SYSCALL(fcntl, "%d,%d,%p", fd, F_SETLK, &flock) != -1); // F_SETLKW

   U_RETURN(result);
}

bool UFile::ftruncate(off_t n)
{
   U_TRACE(1, "UFile::ftruncate(%I)", n)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   U_INTERNAL_ASSERT_DIFFERS(fd,-1)
#if defined(__CYGWIN__) || defined(__MINGW32__)
   U_INTERNAL_ASSERT_EQUALS(map,MAP_FAILED)
#endif

   if (map != MAP_FAILED &&
       map_size < (size_t)n)
      {
#  ifndef HAVE_ARCH64
      if (n >= 512UL*1024UL*1024UL) U_RETURN(false); // NB: on 32 bit, i can't trust mremap beyond this limit for linux system...
#  endif

      size_t _map_size = n * 2;
      char* _map       = (char*) mremap(map, map_size, _map_size, MREMAP_MAYMOVE);

      if (_map == MAP_FAILED) U_RETURN(false);

      map      = _map;
      map_size = _map_size;
      }

   bool result = (U_SYSCALL(ftruncate, "%d,%I", fd, n) == 0);

   if (result)
      {
      st_size = n;

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool UFile::fallocate(off_t n)
{
   U_TRACE(1, "UFile::fallocate(%I)", n)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_DIFFERS(fd, -1)

   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

#ifdef FALLOCATE_IS_SUPPORTED
   if (U_SYSCALL(fallocate, "%d,%d,%I,%I", fd, 0, 0, n) == 0)
#else
   if (U_SYSCALL(ftruncate, "%d,%I", fd, n) == 0)
#endif
      {
      st_size = n;

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool UFile::pread(void* buf, size_t count, off_t offset)
{
   U_TRACE(0, "UFile::pread(%p,%lu,%I)", buf, count, offset)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_DIFFERS(fd, -1)
   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   bool result = UFile::pread(fd, buf, count, offset);

   U_RETURN(result);
}

bool UFile::pwrite(const void* buf, size_t count, off_t offset)
{
   U_TRACE(0, "UFile::pwrite(%p,%lu,%I)", buf, count, offset)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_DIFFERS(fd, -1)
   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   bool result = UFile::pwrite(fd, buf, count, offset);

   U_RETURN(result);
}

void UFile::setBlocking(int fd, int& flags, bool block)
{
   U_TRACE(1, "UFile::setBlocking(%d,%d,%b)", fd, flags, block)

   U_INTERNAL_ASSERT_DIFFERS(fd, -1)

   /* ------------------------------------------------------
   * #define O_RDONLY           00
   * #define O_WRONLY           01
   * #define O_RDWR             02
   * #define O_ACCMODE        0003
   * #define O_CREAT          0100 // not fcntl
   * #define O_EXCL           0200 // not fcntl
   * #define O_NOCTTY         0400 // not fcntl
   * #define O_TRUNC         01000 // not fcntl
   * #define O_APPEND        02000
   * #define O_NONBLOCK      04000
   * #define O_SYNC         010000
   * #define O_ASYNC        020000
   * #define O_DIRECT       040000 // Direct disk access
   * #define O_DIRECTORY   0200000 // Must be a directory
   * #define O_NOFOLLOW    0400000 // Do not follow links
   * #define O_NOATIME    01000000 // Do not set atime
   * #define O_CLOEXEC    02000000 // Set close_on_exec
   * ------------------------------------------------------
   * #define O_NDELAY    O_NONBLOCK
   * #define O_FSYNC     O_SYNC
   * ------------------------------------------------------
   */

   bool blocking = isBlocking(fd, flags); // actual state is blocking...?

   // ----------------------------------------------------------------------------------------
   // determina se le operazioni I/O sul descrittore indicato sono di tipo bloccante o meno...
   // ----------------------------------------------------------------------------------------
   // flags & ~O_NONBLOCK: read() e write() bloccanti     (casi normali)
   // flags |  O_NONBLOCK: read() e write() non bloccanti (casi speciali)
   // ----------------------------------------------------------------------------------------

   if (block != blocking)
      {
      flags = (blocking ? (flags |  O_NONBLOCK) 
                        : (flags & ~O_NONBLOCK));

      U_INTERNAL_DUMP("flags = %B", flags)

      (void) U_SYSCALL(fcntl, "%d,%d,%d", fd, F_SETFL, flags);
      }
}

// risolve link simbolici e/o riferimenti a "./" and "../"

UString UFile::getRealPath(const char* path)
{
   U_TRACE(1, "UFile::getRealPath(%S)", path)

   UString buffer(U_CAPACITY);

   char* result = U_SYSCALL(realpath, "%S,%p", path, buffer.data());

   if (result)
      {
      buffer.size_adjust();

      U_RETURN_STRING(buffer);
      }

   U_RETURN_STRING(UString::getStringNull());
}

// temporary file for locking...

bool UFile::mkTemp(const char* name)
{
   U_TRACE(0, "UFile::mkTemp(%S)", name)

   UString path(U_CAPACITY);

   path.snprintf("%s/%s", u_tmpdir, name);

   setPath(path);

   fd = UFile::mkstemp(pathname.data());

   bool result = (isOpen() && unlink());

   U_RETURN(result);
}

// temporary space for upload file...

bool UFile::mkTempStorage(UString& space, off_t size)
{
   U_TRACE(0, "UFile::mkTempStorage(%.*S,%I)", U_STRING_TO_TRACE(space), size)

   U_ASSERT(space.empty())

   UFile tmp;

   bool result = tmp.mkTemp() &&
                 tmp.fallocate(size) &&
                 tmp.memmap(PROT_READ | PROT_WRITE, &space);

   tmp.close();

   U_RETURN(result);
}

// mkdtemp - create a unique temporary directory

bool UFile::mkdtemp(UString& _template)
{
   U_TRACE(1, "UFile::mkdtemp(%.*S)", U_STRING_TO_TRACE(_template))

   errno = 0; // mkdtemp may not set it on error

   char* modified = U_SYSCALL(mkdtemp, "%S", U_PATH_CONV((char*)_template.c_str()));

   if (modified)
      {
      // NB: mkdtemp in replace use new string...

      if (modified != _template.data()) (void) _template.assign(modified);

      U_RETURN(true);
      }

   U_RETURN(false);
}

// Make any missing parent directories for each directory argument

bool UFile::mkdirs(const char* path, mode_t mode)
{
   U_TRACE(1, "UFile::mkdirs(%S,%d)", path, mode)

   if (mkdir(path, mode)) U_RETURN(true);

   U_INTERNAL_DUMP("errno = %d", errno)

   if (errno == ENOENT)
      {
      char* ptr = (char*) strrchr(path, '/');

      U_INTERNAL_DUMP("ptr = %S", ptr)

      if (ptr)
         {
         char buffer[U_PATH_MAX];

         uint32_t len = ptr - path;

         (void) U_SYSCALL(memcpy, "%p,%p,%u", buffer, path, len);

         buffer[len] = '\0';

         bool result = mkdirs(buffer, mode) &&
                       mkdir(   path, mode);

         U_RETURN(result);
         }
      }

   U_RETURN(false);
}

bool UFile::rmdir(const char* path, bool remove_all)
{
   U_TRACE(1, "UFile::rmdir(%S,%b)", path, remove_all)

   if (U_SYSCALL(rmdir, "%S", U_PATH_CONV(path)) == -1)
      {
      if (remove_all &&
          errno == ENOTEMPTY)
         {
         const char* file;
         UString tmp(path);
         UVector<UString> vec;
         uint32_t n = listContentOf(vec, &tmp);

         for (uint32_t i = 0; i < n; ++i)
            {
            file = vec[i].c_str();

            if (UFile::unlink(file) == false &&
                (errno == EISDIR || errno == EPERM))
               {
               if (UFile::rmdir(file, true) == false) U_RETURN(false);
               }
            }

         bool result = UFile::rmdir(path, false);

         U_RETURN(result);
         }

      U_RETURN(false);
      }

   U_RETURN(true);
}

// If path includes more than one pathname component, remove it, then strip the last component
// and remove the resulting directory, etc., until all components have been removed.
// The pathname component must be empty...

bool UFile::rmdirs(const char* path, bool remove_all)
{
   U_TRACE(1, "UFile::rmdirs(%S,%b)", path, remove_all)

   bool result = rmdir(path, remove_all);

   if (result)
      {
      char* ptr = (char*) strrchr(path, '/');

      U_INTERNAL_DUMP("ptr = %S", ptr)

      if (ptr)
         {
         char newpath[U_PATH_MAX];

         while (ptr > path && *ptr == '/') --ptr; /* Remove any trailing slashes from the result. */

         int length = ptr - path + 1;

         (void) U_SYSCALL(memcpy, "%p,%p,%u", newpath, path, length);

         newpath[length] = '\0';

         result = rmdirs(newpath);
         }
      }

   U_RETURN(result);
}

bool UFile::rename(const char* newpath)
{
   U_TRACE(0, "UFile::rename(%S)", newpath)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   bool result = rename(path_relativ, newpath);

   if (result)
      {
      path_relativ     = newpath;
      path_relativ_len = u_strlen(newpath);

      U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

      U_RETURN(true);
      }

   U_RETURN(false);
}

void UFile::substitute(UFile& file)
{
   U_TRACE(1, "UFile::substitute(%p)", &file)

   U_INTERNAL_DUMP("u_cwd             = %S",   u_cwd)
   U_INTERNAL_DUMP("cwd_save          = %.*S", cwd_len_save, cwd_save)
   U_INTERNAL_DUMP("path_relativ(%u)  = %.*S", path_relativ_len, path_relativ_len, path_relativ)
   U_INTERNAL_DUMP("file.path_relativ = %.*S", file.path_relativ_len, file.path_relativ)

   U_INTERNAL_ASSERT_EQUALS(strcmp(path_relativ, file.path_relativ), 0)

   if (cwd_len_save) (void) U_SYSCALL(chdir, "%S", U_PATH_CONV(u_cwd));

   if (fd != -1)
      {
      UFile::fsync();
      UFile::close();
      }

   if (map != MAP_FAILED) UFile::munmap();

   fd       = file.fd;
   map      = file.map;
   st_size  = file.st_size;
   map_size = file.map_size;

   U_INTERNAL_DUMP("fd = %d map = %p map_size = %lu st_size = %I", fd, map, map_size, st_size)

   if (fd != -1) UFile::fsync();

   file.reset();
}

// LS

static UVector<UString>* vector;

static void ftw_vector_push()
{
   U_TRACE(0, "ftw_vector_push()")

   UString str((void*)u_buffer, u_buffer_len);

   vector->push(str);
}

uint32_t UFile::listContentOf(UVector<UString>& vec, const UString* dir, const UString* filter)
{
   U_TRACE(0, "UFile::listContentOf(%p,%p,%p)", &vec, dir, filter)

   if (UServices::setFtw(dir, filter) == false) U_RETURN(0);

   uint32_t n = vec.size();

   vector = &vec;

   u_ftw_ctx.call              = ftw_vector_push;
   u_ftw_ctx.depth             = false;
   u_ftw_ctx.sort_by           = 0;
   u_ftw_ctx.call_if_directory = false;

   u_ftw();

   u_buffer_len    = 0;
   uint32_t result = (vec.size() - n);

   U_RETURN(result);
}

static bool _root;
static UTree<UString>* tree;

static void ftw_tree_push()
{
   U_TRACE(0, "ftw_tree_push()")

   U_INTERNAL_DUMP("u_ftw_ctx.is_directory = %b", u_ftw_ctx.is_directory)

   if (_root) _root = false;
   else
      {
      UString str((void*)u_buffer, u_buffer_len);

      UTree<UString>* ptree = tree->push(str);

      if (u_ftw_ctx.is_directory) tree = ptree;
      }
}

static void ftw_tree_up()
{
   U_TRACE(0, "ftw_tree_up()")

   tree = tree->parent();
}

void UFile::listRecursiveContentOf(UTree<UString>& t, const UString* dir, const UString* filter)
{
   U_TRACE(0, "UFile::listRecursiveContentOf(%p,%p,%p)", &t, dir, filter)

   if (UServices::setFtw(dir, filter) == false) return;

   if (dir == 0)
      {
      static UString* current;

      if (current == 0) U_NEW_ULIB_OBJECT(current, UString("."));

      dir = current;
      }

   tree  = &t;
   _root = true;

   t.setRoot(*dir);

   u_ftw_ctx.call              = ftw_tree_push;
   u_ftw_ctx.call_if_up        = ftw_tree_up;
   u_ftw_ctx.sort_by           = 0;
   u_ftw_ctx.call_if_directory = true;

   u_ftw();

   u_buffer_len         = 0;
   u_ftw_ctx.call_if_up = 0;
}

uint32_t UFile::buildFilenameListFrom(UVector<UString>& vec, const UString& arg, char sep)
{
   U_TRACE(0, "UFile::buildFilenameListFrom(%p,%.*S,%C)", &vec, U_STRING_TO_TRACE(arg), sep)

   UTokenizer t(arg);
   uint32_t pos, n = vec.size();
   UString dir, filename, filter;

   while (t.next(filename, sep))
      {
      if (filename.find_first_of("?*", 0, 2) == U_NOT_FOUND) vec.push(filename);
      else
         {
         pos = filename.find_last_of('/');

         U_INTERNAL_DUMP("pos = %u", pos)

         if (pos == U_NOT_FOUND)
            {
            (void) listContentOf(vec, 0, &filename);
            }
         else
            {
            dir    = filename.substr(0U, pos);
            filter = filename.substr(pos + 1);

            (void) listContentOf(vec, &dir, &filter);
            }
         }
      }

   uint32_t result = (vec.size() - n);

   U_RETURN(result);
}

bool UFile::mkdir(const char* path, mode_t mode)
{
   U_TRACE(1, "UFile::mkdir(%S,%d)", path, mode)

#ifdef __MINGW32__
   bool result = (U_SYSCALL(mkdir, "%S", U_PATH_CONV(path))      != -1 || errno == EEXIST);
#else
   bool result = (U_SYSCALL(mkdir, "%S,%d",          path, mode) != -1 || errno == EEXIST);
#endif

   U_RETURN(result);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UFile::dump(bool reset) const
{
   *UObjectIO::os << "fd                        " << fd                  << '\n'
                  << "map                       " << (void*)map          << '\n'
                  << "st_size                   " << st_size             << '\n'
                  << "path_relativ              " << '"';

   if (path_relativ)
      {
      *UObjectIO::os << path_relativ;
      }

   *UObjectIO::os << "\"\n"
                  << "path_relativ_len          " << path_relativ_len     << '\n'
                  << "pathname (UString         " << (void*)&pathname     << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
