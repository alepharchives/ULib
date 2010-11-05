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
#include <ulib/notifier.h>
#include <ulib/tokenizer.h>
#include <ulib/container/tree.h>
#include <ulib/utility/services.h>
#include <ulib/utility/string_ext.h>
#include <ulib/net/server/client_image.h>

#ifdef HAVE_MAGIC
#  include <ulib/magic/magic.h>
#endif

int               UFile::mime_index;
bool              UFile::_root;
uint32_t          UFile::cwd_len_save;
const char*       UFile::cwd_save;
UTree<UString>*   UFile::tree;
UVector<UString>* UFile::vector;

#ifdef DEBUG
int         UFile::num_file_object;

void UFile::inc_num_file_object() { ++num_file_object; }
void UFile::dec_num_file_object() { --num_file_object; }
void UFile::chk_num_file_object() { if (num_file_object) U_WARNING("UFile::chdir() with num file object = %d", num_file_object); }
#endif

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

   // we don't need this... (I think)

   /*
   if (pathname.writeable() &&
       pathname.size() != path_relativ_len)
      {
      path_relativ[path_relativ_len] = '\0';
      }
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

int UFile::open(const char* _pathname, int flags, mode_t mode)
{
   U_TRACE(1, "UFile::open(%S,%d,%d)", _pathname, flags, mode)

   U_INTERNAL_ASSERT_POINTER(_pathname)
   U_INTERNAL_ASSERT_MAJOR(u_strlen(_pathname), 0)

   // NB: we centralize here O_BINARY...

   int _fd = U_SYSCALL(open, "%S,%d,%d", U_PATH_CONV(_pathname), flags | O_CLOEXEC | O_BINARY, mode);

   U_RETURN(_fd);
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

bool UFile::stat()
{
   U_TRACE(1, "UFile::stat()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   bool result = (U_SYSCALL(stat, "%S,%p", U_PATH_CONV(path_relativ), (struct stat*)this) == 0);

   U_RETURN(result);
}

bool UFile::chdir(const char* path, bool flag_save)
{
   U_TRACE(1, "UFile::chdir(%S,%b)", path, flag_save)

   chk_num_file_object();

   U_INTERNAL_DUMP("u_cwd(%u)    = %.*S", u_cwd_len, u_cwd_len, u_cwd)

   if (path)
      {
      if (strcmp(path, u_cwd) == 0) U_RETURN(true);

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

bool UFile::memmap(int prot, UString* str, uint32_t offset, uint32_t length)
{
   U_TRACE(0, "UFile::memmap(%d,%p,%u,%u)", prot, str, offset, length)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   U_INTERNAL_ASSERT_DIFFERS(fd,-1)
   U_INTERNAL_ASSERT_MAJOR(st_size,0)
   U_INTERNAL_ASSERT_EQUALS((offset % PAGESIZE),0) // offset should be a multiple of the page size as returned by getpagesize(2)
#ifdef __MINGW32__
   U_INTERNAL_ASSERT((off_t)length <= st_size)     // Don't allow mappings beyond EOF since Windows can't handle that POSIX like
#endif

   map_size = (length ? length : (uint32_t)(st_size - offset));

#ifndef HAVE_ARCH64
   U_INTERNAL_ASSERT_RANGE(1,map_size,3U*1024U*1024U*1024U) // limit of linux system
#endif

   map = UFile::mmap(map_size, fd, prot, MAP_SHARED | MAP_POPULATE, offset);

   if (map != MAP_FAILED)
      {
#  if defined(MADV_SEQUENTIAL)
      if (map_size > (128 * PAGESIZE)) (void) U_SYSCALL(madvise, "%p,%u,%d", map, map_size, MADV_SEQUENTIAL);
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

   U_INTERNAL_DUMP("fd = %d map = %p map_size = %u st_size = %I", fd, map, map_size, st_size)

   if (bmap ||
       st_size > (off_t)(16L * 1024L))
      {
      int                   prot  = PROT_READ;
      if (brdonly == false) prot |= PROT_WRITE;

      (void) memmap(prot, &fileContent);
      }
   else
      {
      UString tmp(st_size);

      char* ptr = tmp.data();

      ssize_t value = U_SYSCALL(pread, "%d,%p,%u,%u", fd, ptr, st_size, 0);

      if (value < 0L) value = 0L;

      ptr[value] = '\0'; // NB: in this way we can use string function data()...

      tmp.size_adjust(value);

      fileContent = tmp;
      }

end:
   UFile::close();

   U_RETURN_STRING(fileContent);
}

UString UFile::contentOf(const char* _pathname, int flags, bool bstat, bool bmap)
{
   U_TRACE(0, "UFile::contentOf(%S,%d,%b,%b)", _pathname, flags, bstat, bmap)

   UFile file;
   UString content;

   file.reset();

   if (file.open(_pathname, flags)) content = file.getContent((((flags & O_RDWR) | (flags & O_WRONLY)) == 0), bstat, bmap);

   U_RETURN_STRING(content);
}

UString UFile::contentOf(const UString& _pathname, int flags, bool bstat, bool bmap)
{
   U_TRACE(0, "UFile::contentOf(%.*S,%d,%b,%b)", U_STRING_TO_TRACE(_pathname), flags, bstat, bmap)

   UFile file;
   UString content;

   file.reset();

   if (file.open(_pathname, flags)) content = file.getContent((((flags & O_RDWR) | (flags & O_WRONLY)) == 0), bstat, bmap);

   U_RETURN_STRING(content);
}

bool UFile::write(const UString& data, bool append, bool bmkdirs)
{
   U_TRACE(1, "UFile::write(%.*S,%b,%b)", U_STRING_TO_TRACE(data), append, bmkdirs)

   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   bool esito = isOpen();

   if (esito == false)
      {
      int flags = O_RDWR | (append ? O_APPEND : O_TRUNC);

      esito = creat(flags, PERM_FILE);

      if (esito == false && bmkdirs)
         {
         // Make any missing parent directories for each directory argument

         char* ptr = (char*) strrchr(path_relativ, '/');

         U_INTERNAL_DUMP("ptr = %S", ptr)

         if (ptr)
            {
            char buffer[U_PATH_MAX];

            uint32_t len = ptr - path_relativ;

            (void) U_SYSCALL(memcpy, "%p,%p,%u", buffer, path_relativ, len);

            buffer[len] = '\0';

            if (mkdirs(buffer)) esito = creat(flags, PERM_FILE);
            }
         }
      }

   if (esito)
      {
      uint32_t sz = data.size();

      if (sz > PAGESIZE)
         {
         uint32_t offset = (append ? size() : 0), resto = 0;

         esito = fallocate(offset + sz);

         if (esito == false) U_WARNING("no more space on disk for size %u", offset + sz);
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

bool UFile::writeTo(const UString& path, const UString& data, bool append, bool bmkdirs)
{
   U_TRACE(0, "UFile::writeTo(%.*S,%.*S,%b,%b)", U_STRING_TO_TRACE(path), U_STRING_TO_TRACE(data), append, bmkdirs)

   UFile tmp(path);

   bool result = tmp.write(data, append, bmkdirs);

   if (tmp.isOpen()) tmp.close();

   U_RETURN(result);
}

bool UFile::writeToTmpl(const char* tmpl, const UString& data, bool append, bool bmkdirs)
{
   U_TRACE(0+256, "UFile::writeToTmpl(%S,%.*S,%b,%b)", tmpl, U_STRING_TO_TRACE(data), append, bmkdirs)

   bool result = false;

   if (data.empty() == false)
      {
      UString path(U_PATH_MAX);

      path.snprintf(tmpl, 0);

      result = UFile::writeTo(path, data, append, bmkdirs);
      }

   U_RETURN(result);
}

/*
sendfile() copies data between one file descriptor and another. Either or both of these file descriptors may refer to a socket.
OUT_FD should be a descriptor opened for writing. POFFSET is a pointer to a variable holding the input file pointer position from
which sendfile() will start reading data. When sendfile() returns, this variable will be set to the offset of the byte following
the last byte that was read. COUNT is the number of bytes to copy between file descriptors. Because this copying is done within
the kernel, sendfile() does not need to spend time transferring data to and from user space.
*/

bool UFile::sendfile(int out_fd, off_t* poffset, uint32_t count)
{
   U_TRACE(1, "UFile::sendfile(%d,%p,%u)", out_fd, poffset, count)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_DIFFERS(fd, -1)
   U_INTERNAL_ASSERT_MAJOR(st_size, 0)
   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   if (count == 0) count = st_size;

   ssize_t value;
   bool write_again;

   do {
      U_INTERNAL_DUMP("count = %u", count)

      value = U_SYSCALL(sendfile, "%d,%d,%p,%u", out_fd, fd, poffset, count);

      if (value < 0L)
         {
         U_INTERNAL_DUMP("errno = %d", errno)

         if (errno == EAGAIN &&
             UNotifier::waitForWrite(out_fd, 3 * 1000) == 1)
            {
            setBlocking(out_fd, UClientImage_Base::socket->flags, true);

            continue;
            }

         U_RETURN(false);
         }

      write_again = (value != (ssize_t)count);

      count -= value;
      }
   while (write_again);

   U_INTERNAL_ASSERT_EQUALS(count,0)

   U_RETURN(true);
}

bool UFile::lock(short l_type, uint32_t start, uint32_t len) const
{
   U_TRACE(1, "UFile::lock(%d,%u,%u)", l_type, start, len)

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

bool UFile::ftruncate(uint32_t n)
{
   U_TRACE(1, "UFile::ftruncate(%u)", n)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   U_INTERNAL_ASSERT_DIFFERS(fd,-1)
#if defined(__CYGWIN__) || defined(__MINGW32__)
   U_INTERNAL_ASSERT_EQUALS(map,MAP_FAILED)
#endif

   if (map != MAP_FAILED &&
       map_size < (uint32_t)n)
      {
#  ifndef HAVE_ARCH64
      if (n >= 512UL*1024UL*1024UL) U_RETURN(false); // NB: on 32 bit, i can't trust mremap beyond this limit for linux system...
#  endif

      uint32_t _map_size = n * 2;
      char* _map         = (char*) mremap(map, map_size, _map_size, MREMAP_MAYMOVE);

      if (_map == MAP_FAILED) U_RETURN(false);

      map      = _map;
      map_size = _map_size;
      }

   bool result = (U_SYSCALL(ftruncate, "%d,%u", fd, n) == 0);

   if (result)
      {
      st_size = n;

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool UFile::fallocate(uint32_t n)
{
   U_TRACE(1, "UFile::fallocate(%u)", n)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_DIFFERS(fd, -1)

   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

#ifdef FALLOCATE_IS_SUPPORTED
   if (U_SYSCALL(fallocate, "%d,%d,%u,%u", fd, 0, 0, n) == 0)
#else
   if (U_SYSCALL(ftruncate, "%d,%u", fd, n) == 0)
#endif
      {
      st_size = n;

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool UFile::pread(void* buf, uint32_t count, uint32_t offset)
{
   U_TRACE(0, "UFile::pread(%p,%u,%u)", buf, count, offset)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_DIFFERS(fd, -1)
   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   bool result = UFile::pread(fd, buf, count, offset);

   U_RETURN(result);
}

bool UFile::pwrite(const void* _buf, uint32_t count, uint32_t offset)
{
   U_TRACE(0, "UFile::pwrite(%p,%u,%u)", _buf, count, offset)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_DIFFERS(fd, -1)
   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   bool result = UFile::pwrite(fd, _buf, count, offset);

   U_RETURN(result);
}

void UFile::setBlocking(int _fd, int& flags, bool block)
{
   U_TRACE(1, "UFile::setBlocking(%d,%d,%b)", _fd, flags, block)

   U_INTERNAL_ASSERT_DIFFERS(_fd, -1)

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

   bool blocking = isBlocking(_fd, flags); // actual state is blocking...?

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

      (void) U_SYSCALL(fcntl, "%d,%d,%d", _fd, F_SETFL, flags);
      }
}

// risolve link simbolici e/o riferimenti a "./" and "../"

UString UFile::getRealPath(const char* path)
{
   U_TRACE(1, "UFile::getRealPath(%S)", path)

   UString buffer(U_PATH_MAX);

   char* result = U_SYSCALL(realpath, "%S,%p", path, buffer.data());

   if (result)
      {
      buffer.size_adjust();

      U_RETURN_STRING(buffer);
      }

   U_RETURN_STRING(UString::getStringNull());
}

// ----------------------------------------------------------------------------------------------------------------------
// create a unique temporary file
// ----------------------------------------------------------------------------------------------------------------------
// char pathname[] = "/tmp/dataXXXXXX"
// The last six characters of template must be XXXXXX and these are replaced with a string that makes the filename unique
// ----------------------------------------------------------------------------------------------------------------------

int UFile::mkstemp(char* _template)
{
   U_TRACE(1, "UFile::mkstemp(%S)", _template)

   errno = 0; // mkstemp may not set it on error

   mode_t old_mode = U_SYSCALL(umask, "%d", 077);  // Create file with restrictive permissions

   int _fd = U_SYSCALL(mkstemp, "%S", U_PATH_CONV(_template));

   U_INTERNAL_DUMP("_template = %S", _template)

   (void) U_SYSCALL(umask, "%d", old_mode);

   U_RETURN(_fd);
}

// temporary file for locking...

bool UFile::mkTemp(const char* name)
{
   U_TRACE(0, "UFile::mkTemp(%S)", name)

   UString path(U_PATH_MAX);

   path.snprintf("%s/%s", u_tmpdir, name);

   setPath(path);

   fd = UFile::mkstemp(pathname.data());

   bool result = (isOpen() && unlink());

   U_RETURN(result);
}

// temporary space for upload file...

bool UFile::mkTempStorage(UString& space, uint32_t size)
{
   U_TRACE(0, "UFile::mkTempStorage(%.*S,%u)", U_STRING_TO_TRACE(space), size)

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
#  ifdef __MINGW32__
          (errno == ENOTEMPTY || errno == EACCES))
#  else
          (errno == ENOTEMPTY))
#  endif
         {
         const char* file;
         UString tmp(path);
         UVector<UString> vec;
         uint32_t n = listContentOf(vec, &tmp);

         for (uint32_t i = 0; i < n; ++i)
            {
            U_INTERNAL_ASSERT(vec[i].isNullTerminated())

            file = vec[i].data();

            if (UFile::unlink(file) == false &&
#        ifdef __MINGW32__
                (errno == EISDIR || errno == EPERM || errno == EACCES))
#        else
                (errno == EISDIR || errno == EPERM))
#        endif
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
      path_relativ     =  (char*) newpath;
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

   U_INTERNAL_DUMP("fd = %d map = %p map_size = %u st_size = %I", fd, map, map_size, st_size)

   if (fd != -1) UFile::fsync();

   file.reset();
}

// LS

void UFile::ftw_vector_push()
{
   U_TRACE(0, "UFile::ftw_vector_push()")

   uint32_t len;
   const char* ptr;

   if (                 u_buffer[0] == '.' &&
       IS_DIR_SEPARATOR(u_buffer[1]))
      {
      U_INTERNAL_ASSERT_MAJOR(u_buffer_len,2)

      ptr = u_buffer    +2;
      len = u_buffer_len-2;
      }
   else
      {
      ptr = u_buffer;
      len = u_buffer_len;
      }

   UString str((void*)ptr, len);

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

void UFile::ftw_tree_push()
{
   U_TRACE(0, "UFile::ftw_tree_push()")

   U_INTERNAL_DUMP("u_ftw_ctx.is_directory = %b", u_ftw_ctx.is_directory)

   if (_root) _root = false;
   else
      {
      UString str((void*)u_buffer, u_buffer_len);

      UTree<UString>* ptree = tree->push(str);

      if (u_ftw_ctx.is_directory) tree = ptree;
      }
}

void UFile::ftw_tree_up()
{
   U_TRACE(0, "UFile::ftw_tree_up()")

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

// MIME TYPE

const char* UFile::getMimeType()
{
   U_TRACE(0, "UFile::getMimeType()")

   const char* content_type = 0;

#ifdef HAVE_MAGIC
   if (map               == MAP_FAILED &&
       memmap(PROT_READ) == false)
      {
      U_RETURN((const char*)0);
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

   U_RETURN(content_type);
}

typedef struct mimeentry {
   const char* type;
   const char* name;
   uint32_t name_len;
} mimeentry;

#define MIME_ENTRY(name,type) { type, name+1, U_CONSTANT_SIZE(name)-1 }

static struct mimeentry mimetab_a[] = {
   MIME_ENTRY( "ai",   "application/postscript" ),
   MIME_ENTRY( "arj",  "application/x-arj-compressed" ),
   MIME_ENTRY( "asx",  "video/x-ms-asf" ),
   MIME_ENTRY( "atom", "application/atom+xml" ),
   MIME_ENTRY( "avi",  "video/x-msvideo" ),
   { 0, 0, 0 }
};

static struct mimeentry mimetab_b[] = {
   MIME_ENTRY( "bat",  "application/x-msdos-program" ),
   MIME_ENTRY( "bmp",  "image/x-ms-bmp" ),
   MIME_ENTRY( "bild", "image/jpeg" ),
   MIME_ENTRY( "boz",  "application/x-bzip2" ),

   /*
   MIME_ENTRY( "bin",  "application/octet-stream" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_c[] = {
   // -------------------------------------------------------------------------------
   // The certificate being downloaded represents a Certificate Authority.
   // When it is downloaded the user will be shown a sequence of dialogs that
   // will guide them through the process of accepting the Certificate Authority
   // and deciding if they wish to trust sites certified by the CA. If a certificate
   // chain is being imported then the first certificate in the chain must be the CA
   // certificate, and any subsequent certificates will be added as untrusted CA
   // certificates to the local database.
   // -------------------------------------------------------------------------------
   MIME_ENTRY( "css", "text/css" ),                   // U_css
   MIME_ENTRY( "crt", "application/x-x509-ca-cert" ),
   MIME_ENTRY( "cer", "application/x-x509-ca-cert" ),

   /*
   MIME_ENTRY( "c",        "text/x-c" ),
   MIME_ENTRY( "c++",      "text/x-c++src" ),
   MIME_ENTRY( "c4d",      "application/vnd.clonk.c4group" ),
   MIME_ENTRY( "cac",      "chemical/x-cache" ),
   MIME_ENTRY( "cascii",   "chemical/x-cactvs-binary" ),
   MIME_ENTRY( "cct",      "application/x-director" ),
   MIME_ENTRY( "cdf",      "application/x-netcdf" ),
   MIME_ENTRY( "cef",      "chemical/x-cxf" ),
   MIME_ENTRY( "cls",      "text/x-tex" ),
   MIME_ENTRY( "csm",      "chemical/x-csml" ),
   MIME_ENTRY( "cdf",      "application/x-netcdf" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_d[] = {
   // -------------------------------------------------------------------------------
   // The certificate being downloaded represents a Certificate Authority.
   // When it is downloaded the user will be shown a sequence of dialogs that
   // will guide them through the process of accepting the Certificate Authority
   // and deciding if they wish to trust sites certified by the CA. If a certificate
   // chain is being imported then the first certificate in the chain must be the CA
   // certificate, and any subsequent certificates will be added as untrusted CA
   // certificates to the local database.
   // -------------------------------------------------------------------------------
   MIME_ENTRY( "der", "application/x-x509-ca-cert" ),
   MIME_ENTRY( "doc", "application/msword" ),
   MIME_ENTRY( "dtd", "application/xml" ),

   /*
   MIME_ENTRY( "deb", "application/x-debian-package" ),
   MIME_ENTRY( "dll", "application/octet-stream" ),
   MIME_ENTRY( "dvi", "application/x-dvi" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_e[] = {
   MIME_ENTRY( "eps", "application/postscript" ),

   /*
   MIME_ENTRY( "exe", "application/octet-stream" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_f[] = {
   MIME_ENTRY( "flv", "video/x-flv" ),
   { 0, 0, 0 }
};

static struct mimeentry mimetab_g[] = {
   MIME_ENTRY( "gif", "image/gif" ),
   MIME_ENTRY( "gz",  "application/x-gunzip" ),
   { 0, 0, 0 }
};

static struct mimeentry mimetab_h[] = {
   MIME_ENTRY( "htm",  "text/html" ),

   /*
   MIME_ENTRY( "html", "text/html" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_i[] = {
   MIME_ENTRY( "ico", "image/x-icon" ),
   { 0, 0, 0 }
};

static struct mimeentry mimetab_j[] = {
   MIME_ENTRY( "js",   "text/javascript" ),  // U_js
   MIME_ENTRY( "jpg",  "image/jpeg" ),
   MIME_ENTRY( "jpeg", "image/jpeg" ),
   MIME_ENTRY( "json", "application/json" ),
   { 0, 0, 0 }
};

static struct mimeentry mimetab_m[] = {
   MIME_ENTRY( "mng",  "video/x-mng" ),
   MIME_ENTRY( "mp3",  "audio/mpeg" ),
   MIME_ENTRY( "mpg",  "video/mpeg" ),
   MIME_ENTRY( "mp4",  "video/mp4" ),
   MIME_ENTRY( "m4a",  "audio/mp4" ),
   MIME_ENTRY( "mpeg", "video/mpeg" ),
   MIME_ENTRY( "md5",  "text/plain" ),
   MIME_ENTRY( "mov",  "video/quicktime" ),

   /*
   MIME_ENTRY( "m3u", "audio/x-mpegurl" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_o[] = {
   MIME_ENTRY( "ogg", "application/ogg" ),
   { 0, 0, 0 }
};

static struct mimeentry mimetab_p[] = {
   MIME_ENTRY( "pem", "application/x-x509-ca-cert" ),
   MIME_ENTRY( "php", "application/x-httpd-php" ),
   MIME_ENTRY( "png", "image/png" ),
   MIME_ENTRY( "pdf", "application/pdf" ),
   MIME_ENTRY( "ps",  "application/postscript" ),
   MIME_ENTRY( "p7b", "application/x-pkcs7-certificates" ),
   MIME_ENTRY( "p7c", "application/x-pkcs7-mime" ),
   MIME_ENTRY( "p12", "application/x-pkcs12" ),
   MIME_ENTRY( "pot", "application/vnd.ms-powerpoint" ),

   /*
   MIME_ENTRY( "pac", "application/x-ns-proxy-autoconfig" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_q[] = {
   MIME_ENTRY( "qt", "video/quicktime" ),
   { 0, 0, 0 }
};

static struct mimeentry mimetab_r[] = {
   MIME_ENTRY( "rar", "application/x-rar-compressed" ),
   MIME_ENTRY( "rtf", "text/richtext" ),
   MIME_ENTRY( "rss", "application/rss+xml" ),

   { 0, 0, 0 }
};

static struct mimeentry mimetab_s[] = {
   MIME_ENTRY( "shtm", "text/html" ),
   MIME_ENTRY( "svg",  "image/svg+xml" ),
   MIME_ENTRY( "swf",  "application/x-shockwave-flash" ),

   /*
   MIME_ENTRY( "sig",   "application/pgp-signature" ),
   MIME_ENTRY( "shtml", "text/html" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_t[] = {
   MIME_ENTRY( "tar",  "application/x-tar" ),
   MIME_ENTRY( "tif",  "image/tiff" ),
   MIME_ENTRY( "text", "text/plain" ),
   MIME_ENTRY( "txt",  "text/plain" ),
   MIME_ENTRY( "tgz",  "application/x-tar-gz" ),

   /*
   MIME_ENTRY( "torrent",  "application/x-bittorrent" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_w[] = {
   MIME_ENTRY( "wav", "audio/x-wav" ),
   MIME_ENTRY( "wmv", "video/x-ms-wmv" ),
   { 0, 0, 0 }
};

static struct mimeentry mimetab_x[] = {
   MIME_ENTRY( "xls", "application/excel" ),
   MIME_ENTRY( "xml", "application/xml" ),
   MIME_ENTRY( "xpm", "image/x-xpixmap" ),
   MIME_ENTRY( "xbm", "image/x-xbitmap" ),

   /*
   MIME_ENTRY( "xwd", "image/x-xwindowdump" ),
   */

   { 0, 0, 0 }
};

static struct mimeentry mimetab_z[] = {
   MIME_ENTRY( "zip", "application/zip" ),
   { 0, 0, 0 }
};

static struct mimeentry mimetab_null[] = {
   { 0, 0, 0 }
};

const char* UFile::getMimeType(const char* suffix)
{
   U_TRACE(0, "UFile::getMimeType(%S)", suffix)

   U_INTERNAL_ASSERT_POINTER(suffix)

   char c;
   struct mimeentry* ptr;
   struct mimeentry* mimetab;

   switch ((c = *suffix++))
      {
      case 'a': mimetab = mimetab_a; break;
      case 'b': mimetab = mimetab_b; break;
      case 'c': mimetab = mimetab_c; break;
      case 'd': mimetab = mimetab_d; break;
      case 'e': mimetab = mimetab_e; break;
      case 'f': mimetab = mimetab_f; break;
      case 'g': mimetab = mimetab_g; break;
      case 'h': mimetab = mimetab_h; break;
      case 'i': mimetab = mimetab_i; break;
      case 'j': mimetab = mimetab_j; break;
   // case 'k': mimetab = mimetab_null; break;
   // case 'l': mimetab = mimetab_null; break;
      case 'm': mimetab = mimetab_m; break;
   // case 'n': mimetab = mimetab_null; break;
      case 'o': mimetab = mimetab_o; break;
      case 'p': mimetab = mimetab_p; break;
      case 'q': mimetab = mimetab_q; break;
      case 'r': mimetab = mimetab_r; break;
      case 's': mimetab = mimetab_s; break;
      case 't': mimetab = mimetab_t; break;
   // case 'u': mimetab = mimetab_null; break;
   // case 'v': mimetab = mimetab_null; break;
      case 'w': mimetab = mimetab_w; break;
      case 'x': mimetab = mimetab_x; break;
      case 'z': mimetab = mimetab_z; break;
      default:  mimetab = mimetab_null;
      }

   U_INTERNAL_DUMP("c = %C", c)

   ptr = mimetab;

loop:
   while (ptr->name)
      {
      U_INTERNAL_DUMP("mimetab = %p (%S,%u,%S)", ptr, ptr->name, ptr->name_len, ptr->type)

      for (uint32_t j = 0; j < ptr->name_len; ++j)
         {
         if (suffix[j] != ptr->name[j])
            {
            ++ptr;

            goto loop;
            }
         }

      // NB: first entry: 'c' (U_css) - 'j' (U_js)

      mime_index = (ptr == mimetab && (c == 'c' || c == 'j') ? c : -1);

      U_INTERNAL_DUMP("mime_index = %C", mime_index)

      U_RETURN(ptr->type);
      }

   U_RETURN((const char*)0);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UFile::dump(bool _reset) const
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

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
