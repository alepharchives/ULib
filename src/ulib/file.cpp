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
#include <ulib/container/vector.h>
#include <ulib/utility/services.h>
#include <ulib/utility/dir_walk.h>
#include <ulib/utility/string_ext.h>

#ifdef USE_LIBMAGIC
#  include <ulib/magic/magic.h>
#endif

char*    UFile::cwd_save;
char*    UFile::pfree;
uint32_t UFile::nfree;
uint32_t UFile::cwd_save_len;
#ifdef DEBUG
int      UFile::num_file_object;

void UFile::inc_num_file_object(UFile* pthis)
{
   U_TRACE(0+256, "UFile::inc_num_file_object(%p)", pthis)

   ++num_file_object;

   U_INTERNAL_DUMP("this         = %p", pthis)
   U_INTERNAL_DUMP("&st_dev      = %p", &(pthis->st_dev))
   U_INTERNAL_DUMP("&st_ctime    = %p", &(pthis->st_ctime))
   U_INTERNAL_DUMP("memory._this = %p", pthis->memory._this)

   U_INTERNAL_ASSERT_EQUALS((void*)pthis, (void*)&(pthis->st_dev))
}

void UFile::dec_num_file_object(UFile* pthis, int fd)
{
   --num_file_object;

   if (fd != -1) U_WARNING("file descriptor %d not closed...", fd);
}

void UFile::chk_num_file_object()
{
   if (num_file_object) U_WARNING("UFile::chdir() with num file object = %d", num_file_object);
}
#endif

#ifndef MAP_POPULATE // (since Linux 2.5.46)
#define MAP_POPULATE 0
#endif

#ifndef MREMAP_MAYMOVE
#define MREMAP_MAYMOVE 1
#endif

void UFile::setPathRelativ(const UString* environment)
{
   U_TRACE(0, "UFile::setPathRelativ(%p)", environment)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(pathname)

   reset();

   char c = pathname.c_char(0);

   if (c == '~' ||
       c == '$')
      {
      UString x = UStringExt::expandPath(pathname, environment);

      if (x.empty() == false) pathname = x;
      }

   // NB: la stringa potrebbe non essere scrivibile...!!!!

   path_relativ_len = pathname.size();
   path_relativ     = u_getPathRelativ(pathname.c_str(), &path_relativ_len);

   U_INTERNAL_ASSERT_MAJOR(path_relativ_len, 0)

   // we don't need this... (I think)

   /*
   if (pathname.writeable() &&
       pathname.size() != path_relativ_len)
      {
      path_relativ[path_relativ_len] = '\0';
      }
   */

   U_INTERNAL_DUMP("u_cwd            = %S", u_cwd)
   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)
}

void UFile::setRoot()
{
   U_TRACE(0, "UFile::setRoot()")

   reset();

   pathname.setConstant(U_CONSTANT_TO_PARAM("/"));

   st_mode          = S_IFDIR|0755;
   path_relativ     = pathname.data();
   path_relativ_len = 1;

   U_INTERNAL_DUMP("u_cwd           = %S", u_cwd)
   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)
}

// gcc - call is unlikely and code size would grow

void UFile::setPath(const UString& path, const UString* environment)
{
   U_TRACE(0, "UFile::setPath(%.*S,%p)", U_STRING_TO_TRACE(path), environment)

   pathname = path;

   setPathRelativ(environment);
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
   U_INTERNAL_ASSERT_MAJOR(u__strlen(_pathname, __PRETTY_FUNCTION__), 0)

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

   st_ino = 0;

   bool result = (U_SYSCALL(stat, "%S,%p", U_PATH_CONV(path_relativ), (struct stat*)this) == 0);

   U_RETURN(result);
}

bool UFile::chdir(const char* path, bool flag_save)
{
   U_TRACE(1, "UFile::chdir(%S,%b)", path, flag_save)

   chk_num_file_object();

   U_INTERNAL_ASSERT_POINTER(cwd_save)

   U_INTERNAL_DUMP("u_cwd    = %S", u_cwd)
   U_INTERNAL_DUMP("cwd_save = %S", cwd_save)

   if (path)
      {
      if (strcmp(path, u_cwd) == 0) U_RETURN(true);

#  ifndef __MINGW32__
      U_INTERNAL_ASSERT(IS_DIR_SEPARATOR(u_cwd[0]))
#  endif

      if (flag_save)
         {
         cwd_save_len = u_cwd_len;

         (void) u__strcpy(cwd_save, u_cwd);
         }
      }
   else
      {
      U_INTERNAL_ASSERT(flag_save)
      U_INTERNAL_ASSERT_MAJOR(cwd_save_len, 0)

      path = cwd_save;
      }

   bool result = (U_SYSCALL(chdir, "%S", U_PATH_CONV(path)) != -1);

   if (result)
      {
      if (path == cwd_save) // NB: => chdir(0, true)...
         {
         U_INTERNAL_ASSERT(flag_save)

         u_cwd_len = cwd_save_len;

         (void) u__strcpy(u_cwd, cwd_save);

         cwd_save_len = 0;
         }
      else if (IS_DIR_SEPARATOR(path[0]) == false) u_getcwd();
      else
         {
         u_cwd_len = u__strlen(path, __PRETTY_FUNCTION__);

         U_INTERNAL_ASSERT_MINOR(u_cwd_len, U_PATH_MAX)

         (void) u__strcpy(u_cwd, path);
         }
      }

   U_INTERNAL_DUMP("u_cwd    = %S", u_cwd)
   U_INTERNAL_DUMP("cwd_save = %S", cwd_save)

   U_RETURN(result);
}

uint32_t UFile::setPathFromFile(const UFile& file, char* buffer_path, const char* suffix, uint32_t len)
{
   U_TRACE(1, "UFile::setPathFromFile(%p,%p,%.*S,%u)", &file, buffer_path, len, suffix, len)

   U_INTERNAL_DUMP("file.path_relativ(%u) = %.*S", file.path_relativ_len, file.path_relativ_len, file.path_relativ)

   U__MEMCPY(buffer_path,                         file.path_relativ, file.path_relativ_len);
   U__MEMCPY(buffer_path + file.path_relativ_len,            suffix,                   len);

   uint32_t new_path_relativ_len = file.path_relativ_len + len;

   U_INTERNAL_ASSERT_MINOR(new_path_relativ_len,(int32_t)MAX_FILENAME_LEN)

   buffer_path[new_path_relativ_len] = '\0';

   U_INTERNAL_DUMP("buffer_path(%u)       = %S", new_path_relativ_len, buffer_path)

   U_RETURN(new_path_relativ_len);
}

void UFile::setPath(const UFile& file, char* buffer_path, const char* suffix, uint32_t len)
{
   U_TRACE(1, "UFile::setPath(%p,%p,%.*S,%u)", &file, buffer_path, len, suffix, len)

   U_INTERNAL_DUMP("u_cwd    = %S", u_cwd)
   U_INTERNAL_DUMP("cwd_save = %.*S", cwd_save_len, cwd_save)

   reset();

   if (IS_DIR_SEPARATOR(file.path_relativ[0]) == false && cwd_save_len) (void) U_SYSCALL(chdir, "%S", U_PATH_CONV(cwd_save)); // for IR...

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

UString UFile::getName() const
{
   U_TRACE(0, "UFile::getName()")

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   U_INTERNAL_ASSERT_POINTER(path_relativ)
   U_INTERNAL_ASSERT_MAJOR(path_relativ_len, 0)

   uint32_t pos;
   UString result;
   const char* base = 0;
   ptrdiff_t name_len = path_relativ_len;

   for (const char* ptr = path_relativ, *end = path_relativ + path_relativ_len; ptr < end; ++ptr) if (*ptr == '/') base = ptr + 1;

   if (base) name_len -= (base - path_relativ);

   pos = pathname.size() - name_len;

   U_INTERNAL_DUMP("name = %.*S", name_len, pathname.c_pointer(pos))

   U_ASSERT(UStringExt::endsWith(pathname, pathname.c_pointer(pos), name_len))

   result = pathname.substr(pos);

   U_RETURN_STRING(result);
}

bool UFile::isNameDosMatch(const char* mask, uint32_t mask_len) const
{
   U_TRACE(0, "UFile::isNameDosMatch(%.*S,%u)", mask_len, mask, mask_len)

   UString basename = getName();

   bool result = u_dosmatch_with_OR(U_STRING_TO_PARAM(basename), mask, mask_len, 0);

   U_RETURN(result);
}

off_t UFile::size(bool bstat)
{
   U_TRACE(0, "UFile::size(%b)", bstat)

   U_INTERNAL_ASSERT_EQUALS(st_size, 0)

   if (bstat == false) readSize();
   else
      {
      fstat();

#  ifdef __MINGW32__
      st_ino = u_get_inode(fd);
#  endif

      U_INTERNAL_DUMP("st_ino = %llu", st_ino)

      if (S_ISDIR(st_mode)) U_RETURN(0);
      }

   U_RETURN(st_size);
}

// MEMORY MAPPED I/O

char* UFile::mmap(uint32_t* plength, int _fd, int prot, int flags, uint32_t offset)
{
   U_TRACE(1, "UFile::mmap(%p,%d,%d,%d,%u)", plength, _fd, prot, flags, offset)

   U_INTERNAL_ASSERT_POINTER(plength)

#ifndef __MINGW32__
#  ifndef HAVE_ARCH64
   U_INTERNAL_ASSERT_RANGE(1U, *plength, 3U * 1024U * 1024U * 1024U) // limit of linux system on 32bit
#  endif
   if (_fd != -1)
#endif
   return (char*) U_SYSCALL(mmap, "%d,%u,%d,%d,%d,%u", 0, *plength, prot, flags, _fd, offset);

   *plength = (*plength + U_PAGEMASK) & ~U_PAGEMASK;

   U_INTERNAL_ASSERT_EQUALS(*plength & U_PAGEMASK, 0)

   if ((flags & MAP_SHARED) != 0) return (char*) U_SYSCALL(mmap, "%d,%u,%d,%d,%d,%u", 0, *plength, prot, flags, -1, 0);

   char* _ptr;
   bool _abort = false;

   if (*plength >= (256U * 1024U * 1024U)) // NB: to save some swap pressure...
      {
#if defined(__linux__) && defined(ENABLE_THREAD)
try_from_file_system:
#endif
      UFile tmp;
      char _template[32];

#  ifdef DEBUG
      U_WARNING("we are going to allocate from file system %u bytes (%u KB) (pid %P)", *plength, *plength / 1024);
#  endif

      // By default, /tmp on Fedora 18 will be on a tmpfs. Storage of large temporary files should be done in /var/tmp.
      // This will reduce the I/O generated on disks, increase SSD lifetime, save power, and improve performance of the /tmp filesystem. 

      (void) strcpy(_template, "/var/tmp/mapXXXXXX");

      if (tmp.mkTemp(_template) &&
          tmp.fallocate(*plength))
         {
         _ptr = (char*) U_SYSCALL(mmap, "%d,%u,%d,%d,%d,%u", 0, tmp.st_size, prot, MAP_PRIVATE | MAP_NORESERVE, tmp.fd, 0);
         }
      else
         {
         _ptr = (char*)MAP_FAILED;
         }

      tmp.close();

      if (_ptr != (char*)MAP_FAILED) return _ptr;

      if (_abort)
         {
         unsigned long vsz, rss;

         u_get_memusage(&vsz, &rss);

         U_ERROR("cannot allocate %u bytes (%u KB) of memory. Going down - ",
                  "address space usage: %.2f MBytes - "
                            "rss usage: %.2f MBytes",
                  *plength, *plength / 1024, (double)vsz / (1024.0 * 1024.0),
                                             (double)rss / (1024.0 * 1024.0));
         }
      }

#if defined(__linux__) && defined(ENABLE_THREAD)
   U_INTERNAL_DUMP("plength = %u nfree = %u pfree = %p", *plength, nfree, pfree)

   if (pfree == 0)
      {
#  ifdef DEBUG
      unsigned long vsz, rss;

      u_get_memusage(&vsz, &rss);

      U_WARNING("we are going to allocate 256 MB (pid %P) - "
                 "address space usage: %.2f MBytes - "
                           "rss usage: %.2f MBytes",
                        (double)vsz / (1024.0 * 1024.0),
                        (double)rss / (1024.0 * 1024.0));
#  endif

      nfree = (256U * 1024U * 1024U);
      pfree = (char*) U_SYSCALL(mmap, "%d,%u,%d,%d,%d,%u", 0, nfree, prot, U_MAP_ANON, -1, 0);

      if (pfree == (char*)MAP_FAILED)
         {
         nfree = 0;
         pfree = 0;
         }
      }

   if (*plength > nfree)
      {
#  ifdef DEBUG
      U_WARNING("we are going to allocate %u bytes (%u KB) (pid %P) - nfree = %u", *plength, *plength / 1024, nfree);
#  endif

      _ptr = (char*) U_SYSCALL(mmap, "%d,%u,%d,%d,%d,%u", 0, *plength, prot, U_MAP_ANON, -1, 0);

      if (_ptr == (char*)MAP_FAILED)
         {
         _abort = true;

         goto try_from_file_system;
         }
      }

   _ptr   = pfree;
   nfree -= *plength;

   if (nfree > (16U * 1024U)) pfree += *plength;
   else
      {
      pfree     = 0;
      *plength += nfree;
      }

   U_INTERNAL_DUMP("plength = %u nfree = %u pfree = %p", *plength, nfree, pfree)
#else
#  ifdef DEBUG
   U_WARNING("we are going to malloc %u bytes (%u KB) (pid %P)", *plength, *plength / 1024);
#  endif

   _ptr = (char*) U_SYSCALL(malloc, "%u", *plength);
#endif

   return _ptr;
}

bool UFile::memmap(int prot, UString* str, uint32_t offset, uint32_t length)
{
   U_TRACE(0, "UFile::memmap(%d,%p,%u,%u)", prot, str, offset, length)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   U_INTERNAL_ASSERT_DIFFERS(fd,-1)
   U_INTERNAL_ASSERT_MAJOR(st_size,0)

#ifdef __MINGW32__
   U_INTERNAL_ASSERT((off_t)length <= st_size) // NB: don't allow mappings beyond EOF since Windows can't handle that POSIX like...
#endif

   if (length == 0) length = st_size;

   uint32_t resto = 0;

   if (offset)
      {
      resto = offset % PAGESIZE;

      offset -= resto;
      length += resto;
      }

   U_INTERNAL_DUMP("resto = %u", resto)

#ifdef HAVE_ARCH64
   U_INTERNAL_ASSERT_MINOR_MSG(length, U_STRING_LIMIT, "we can't manage file bigger than 4G...") // limit of UString
#endif

   U_INTERNAL_ASSERT_EQUALS((offset % PAGESIZE),0) // offset should be a multiple of the page size as returned by getpagesize(2)

   if (map != (char*)MAP_FAILED)
      {
      munmap(map, map_size);

      map_size = 0;
      }

   map = (char*) U_SYSCALL(mmap, "%d,%u,%d,%d,%d,%u", 0, length, prot, MAP_SHARED | MAP_POPULATE, fd, offset);

   if (map != (char*)MAP_FAILED)
      {
      map_size = length;

      if (str) str->mmap(map + resto, length - resto);

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
   U_INTERNAL_ASSERT_DIFFERS(map,(char*)MAP_FAILED)

   UFile::munmap(map, map_size);

   map      = (char*)MAP_FAILED;
   map_size = 0;
}

void UFile::msync(char* ptr, char* page, int flags)
{
   U_TRACE(1, "UFile::msync(%p,%p,%d)", ptr, page, flags)

   U_INTERNAL_ASSERT(ptr > page)

   uint32_t resto = (long)page & U_PAGEMASK;

   U_INTERNAL_DUMP("resto = %u", resto)

   char* addr      = page - resto;
   uint32_t length =  ptr - addr;

   U_INTERNAL_ASSERT_EQUALS((long)addr & U_PAGEMASK, 0) // addr should be a multiple of the page size as returned by getpagesize(2)

   (void) U_SYSCALL(msync, "%p,%u,%d", addr, length, flags);
}

UString UFile::_getContent(bool bsize, bool brdonly, bool bmap)
{
   U_TRACE(0, "UFile::_getContent(%b,%b,%b)", bsize, brdonly, bmap)

   U_INTERNAL_DUMP("fd = %d map = %p map_size = %u st_size = %I", fd, map, map_size, st_size)

   U_INTERNAL_ASSERT_DIFFERS(fd, -1)
   U_INTERNAL_ASSERT_POINTER(path_relativ)

   UString fileContent;

   if (bsize) readSize();

   if (st_size)
      {
      if (bmap ||
          st_size > (off_t)(4L * PAGESIZE))
         {
         int                   prot  = PROT_READ;
         if (brdonly == false) prot |= PROT_WRITE;

         (void) memmap(prot, &fileContent, 0, st_size);
         }
      else
         {
         UString tmp(st_size);

         char* ptr = tmp.data();

         ssize_t value = U_SYSCALL(pread, "%d,%p,%u,%u", fd, ptr, st_size, 0);

         if (value < 0L) value = 0L;

         ptr[value] = '\0'; // NB: in this way we can use the UString method data()...

         tmp.size_adjust(value);

         fileContent = tmp;
         }
      }

   U_RETURN_STRING(fileContent);
}

UString UFile::getContent(bool brdonly, bool bstat, bool bmap)
{
   U_TRACE(0, "UFile::getContent(%b,%b,%b)", brdonly, bstat, bmap)

   if (isOpen()                          == false &&
       open(brdonly ? O_RDONLY : O_RDWR) == false)
      {
      U_RETURN_STRING(UString::getStringNull());
      }

   UString fileContent;

   if (st_size ||
       size(bstat))
      {
      fileContent = _getContent(false, brdonly, bmap);
      }

   UFile::close();

   U_RETURN_STRING(fileContent);
}

UString UFile::contentOf(const UString& _pathname, int flags, bool bstat)
{
   U_TRACE(0, "UFile::contentOf(%.*S,%d,%b)", U_STRING_TO_TRACE(_pathname), flags, bstat)

   UFile file;
   UString content;

   file.reset();

   if (file.open(_pathname, flags)) content = file.getContent((((flags & O_RDWR) | (flags & O_WRONLY)) == 0), bstat);

   U_RETURN_STRING(content);
}

UString UFile::contentOf(const char* _pathname, int flags, bool bstat, const UString* environment)
{
   U_TRACE(0, "UFile::contentOf(%S,%d,%b,%p)", _pathname, flags, bstat, environment)

   UFile file;
   UString path(_pathname), content;

   file.reset();

   file.setPath(path, environment);

   if (file.open(flags)) content = file.getContent((((flags & O_RDWR) | (flags & O_WRONLY)) == 0), bstat);

   U_RETURN_STRING(content);
}

bool UFile::write(const char* data, uint32_t sz, bool append, bool bmkdirs)
{
   U_TRACE(1, "UFile::write(%.*S,%u,%b,%b)", sz, data, sz, append, bmkdirs)

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

            U__MEMCPY(buffer, path_relativ, len);

            buffer[len] = '\0';

            if (mkdirs(buffer)) esito = creat(flags, PERM_FILE);
            }
         }
      }

   if (sz &&
       esito)
      {
      if (sz <= PAGESIZE) esito = UFile::write(fd, data, sz);
      else
         {
         uint32_t offset = (append ? size() : 0);

         esito = fallocate(offset + sz);

         if (esito == false)
            {
            readSize();

            U_WARNING("no more space on disk for requested size %u - acquired only %u bytes", offset + sz, st_size);

            sz = (st_size > offset ? st_size - offset : 0);
            }

         if (sz &&
             memmap(PROT_READ | PROT_WRITE, 0, offset, st_size))
            {
            U__MEMCPY(map + offset, data, sz);

            munmap();
            }
         }
      }

   U_RETURN(esito);
}

bool UFile::writeTo(const UString& path, const char* data, uint32_t sz, bool append, bool bmkdirs)
{
   U_TRACE(0, "UFile::writeTo(%.*S,%.*S,%u,%b,%b)", U_STRING_TO_TRACE(path), sz, data, sz, append, bmkdirs)

   UFile tmp(path);

   bool result = tmp.write(data, sz, append, bmkdirs);

   if (tmp.isOpen()) tmp.close();

   U_RETURN(result);
}

bool UFile::writeToTmpl(const char* tmpl, const char* data, uint32_t sz, bool append, bool bmkdirs)
{
   U_TRACE(0+256, "UFile::writeToTmpl(%S,%.*S,%u,%b,%b)", tmpl, sz, data, sz, append, bmkdirs)

   bool result = false;

   if (sz)
      {
      UString path(U_PATH_MAX);

      path.snprintf(tmpl, 0);

      result = UFile::writeTo(path, data, sz, append, bmkdirs);
      }

   U_RETURN(result);
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
   U_INTERNAL_ASSERT_EQUALS(map,(char*)MAP_FAILED)
#endif

   if (map != (char*)MAP_FAILED &&
       map_size < (uint32_t)n)
      {
      uint32_t _map_size = n * 2;
      char* _map         = (char*) mremap(map, map_size, _map_size, MREMAP_MAYMOVE);

      if (_map == (char*)MAP_FAILED) U_RETURN(false);

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
   if (U_SYSCALL(fallocate, "%d,%d,%u,%u", fd, 0, 0, n) == 0) goto next;

   U_INTERNAL_DUMP("errno = %d", errno)

   if (errno != EOPNOTSUPP) U_RETURN(false);
#endif

   if (U_SYSCALL(ftruncate, "%d,%u", fd, n) == 0)
      {
#ifdef FALLOCATE_IS_SUPPORTED
   next:
#endif
      st_size = n;

      U_RETURN(true);
      }

   U_RETURN(false);
}

int UFile::getSysParam(const char* name)
{
   U_TRACE(0, "UFile::getSysParam(%S)", name)

   int value = -1, fd = open(name, O_RDWR, PERM_FILE);

   if (fd != -1)
      {
      char buffer[32];

      buffer[U_SYSCALL(read, "%d,%p,%u", fd, buffer, sizeof(buffer)-1)] = '\0';

      value = atoi(buffer);

      close(fd);
      }

   U_RETURN(value);
}

int UFile::setSysParam(const char* name, int value, bool force)
{
   U_TRACE(0, "UFile::setSysParam(%S,%u,%b)", name, value, force)

   int old_value = -1, fd = open(name, O_RDWR, PERM_FILE);

   if (fd != -1)
      {
      char buffer[32];

      buffer[U_SYSCALL(read, "%d,%p,%u", fd, buffer, sizeof(buffer)-1)] = '\0';

      old_value = atoi(buffer);

      if (force ||
          old_value < value)
         {
         UString data = UStringExt::numberToString(value);

         (void) write(fd, U_STRING_TO_PARAM(data));
         }

      close(fd);
      }

   U_RETURN(old_value);
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

int UFile::setBlocking(int _fd, int flags, bool block)
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

   U_RETURN(flags);
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

bool UFile::mkTemp(char* _template)
{
   U_TRACE(1, "UFile::mkTemp(%S)", _template)

   if (_template) setPath(_template);
   else
      {
      UString path(U_PATH_MAX);

      path.snprintf("%s/lockXXXXXX", u_tmpdir);

      setPath(path);
      }

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   mode_t old_mode = U_SYSCALL(umask, "%d", 077);  // Create file with restrictive permissions

   errno = 0; // mkstemp may not set it on error

   fd = U_SYSCALL(mkstemp, "%S", U_PATH_CONV((char*)path_relativ));

   (void) U_SYSCALL(umask, "%d", old_mode);

   bool result = (isOpen() && _unlink());

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
      // NB: c_str() in replace use a new string...

      if (modified != _template.data()) (void) _template.assign(modified);

      U_RETURN(true);
      }

   U_RETURN(false);
}

// Make any missing parent directories for each directory argument

bool UFile::mkdirs(const char* path, mode_t mode)
{
   U_TRACE(1, "UFile::mkdirs(%S,%d)", path, mode)

   if (_mkdir(path, mode)) U_RETURN(true);

   U_INTERNAL_DUMP("errno = %d", errno)

   if (errno == ENOENT)
      {
      char* ptr = (char*) strrchr(path, '/');

      U_INTERNAL_DUMP("ptr = %S", ptr)

      if (ptr)
         {
         char buffer[U_PATH_MAX];

         uint32_t len = ptr - path;

         U__MEMCPY(buffer, path, len);

         buffer[len] = '\0';

         bool result =  mkdirs(buffer, mode) &&
                       _mkdir(   path, mode);

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
         UDirWalk dirwalk(path);
         UVector<UString> vec(256);

         for (uint32_t i = 0, n = dirwalk.walk(vec); i < n; ++i)
            {
            U_INTERNAL_ASSERT(vec[i].isNullTerminated())

            file = vec[i].data();

            U_ASSERT_DIFFERS(path, file)

            if (UFile::_unlink(file) == false &&
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

         U__MEMCPY(newpath, path, length);

         newpath[length] = '\0';

         result = rmdirs(newpath);
         }
      }

   U_RETURN(result);
}

bool UFile::_rename(const char* newpath)
{
   U_TRACE(0, "UFile::_rename(%S)", newpath)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(path_relativ)

   U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

   bool result = UFile::_rename(path_relativ, newpath);

   if (result)
      {
      path_relativ     =  (char*) newpath;
      path_relativ_len = u__strlen(newpath, __PRETTY_FUNCTION__);

      U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

      U_RETURN(true);
      }

   U_RETURN(false);
}

void UFile::substitute(UFile& file)
{
   U_TRACE(1, "UFile::substitute(%p)", &file)

   U_INTERNAL_ASSERT_POINTER(cwd_save)

   U_INTERNAL_DUMP("u_cwd             = %S",   u_cwd)
   U_INTERNAL_DUMP("cwd_save          = %.*S", cwd_save_len, cwd_save)
   U_INTERNAL_DUMP("path_relativ(%u)  = %.*S", path_relativ_len, path_relativ_len, path_relativ)
   U_INTERNAL_DUMP("file.path_relativ = %.*S", file.path_relativ_len, file.path_relativ)

   U_INTERNAL_ASSERT_EQUALS(strcmp(path_relativ, file.path_relativ), 0)

   if (cwd_save_len) (void) U_SYSCALL(chdir, "%S", U_PATH_CONV(u_cwd));

   if (fd != -1)
      {
      UFile::fsync();
      UFile::close();
      }

   if (map != (char*)MAP_FAILED) munmap();

   fd       = file.fd;
   map      = file.map;
   st_size  = file.st_size;
   map_size = file.map_size;

   file.reset();

   U_INTERNAL_DUMP("fd = %d map = %p map_size = %u st_size = %I", fd, map, map_size, st_size)

   if (fd != -1) UFile::fsync();
}

// MIME TYPE

const char* UFile::getMimeType(bool bmagic)
{
   U_TRACE(0, "UFile::getMimeType(%b)", bmagic)

   const char* suffix       = getSuffix();
   const char* content_type = 0;

   if (suffix)
      {
      U_INTERNAL_ASSERT_EQUALS(suffix[0], '.')
      U_INTERNAL_ASSERT_EQUALS(strchr(suffix, '/'), 0)

      content_type = u_get_mimetype(suffix+1);

      U_INTERNAL_DUMP("u_mime_index = %C", u_mime_index)
      }

#ifdef USE_LIBMAGIC
   if (bmagic              &&
       u_is_css() == false &&
       u_is_js () == false &&
       u_is_ssi() == false &&
       u_is_usp() == false)
      {
      U_INTERNAL_ASSERT_DIFFERS(map, (char*)MAP_FAILED)

      const char* ctype = UMagic::getType(map, map_size).data();

      if (ctype) content_type = ctype;
      }
#endif

   if (content_type == 0)
      {
#  ifdef USE_LIBMAGIC
      if (map != (char*)MAP_FAILED) content_type = UMagic::getType(map, map_size).data();

      if (content_type == 0)
#  endif

      content_type = "application/octet-stream";
      }

   U_INTERNAL_ASSERT_POINTER(content_type)

   U_RETURN(content_type);
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
