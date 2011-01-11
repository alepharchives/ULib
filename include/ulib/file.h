// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    file.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_FILE_H
#define ULIB_FILE_H

#include <ulib/string.h>

#ifndef __MINGW32__
#  include <sys/uio.h>
#endif

#include <errno.h>

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#  define MAP_ANONYMOUS MAP_ANON /* Don't use a file */
#endif

#ifdef HAVE_LFS
#  define U_SEEK_BEGIN 0LL
#else
#  define U_SEEK_BEGIN 0L
#endif

// struct stat {
//    dev_t     st_dev;      /* device */
//    ino_t     st_ino;      /* inode */
//    mode_t    st_mode;     /* protection */
//    nlink_t   st_nlink;    /* number of hard links */
//    uid_t     st_uid;      /* user ID of owner */
//    gid_t     st_gid;      /* group ID of owner */
//    dev_t     st_rdev;     /* device type (if inode device) */
//    off_t     st_size;     /* total size, in bytes */
//    blksize_t st_blksize;  /* blocksize for filesystem I/O */
//    blkcnt_t  st_blocks;   /* number of blocks allocated */
//    time_t    st_atime;    /* time of last access */
//    time_t    st_mtime;    /* time of last modification */
//    time_t    st_ctime;    /* time of last change */
// };

#ifdef __MINGW32__
#define st_ino u_inode
#endif

/* File-permission-bit symbols */
#define PERM_FILE       (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)
#define PERM_DIRECTORY  S_IRWXU

// NB: la stringa pathname potrebbe non essere scrivibile e quindi path_relativ[path_relativ_len] potrebbe non essere '\0'...
#define U_FILE_TO_PARAM(file) (file).getPathRelativ(),(file).getPathRelativLen()
#define U_FILE_TO_TRACE(file) (file).getPathRelativLen(),(file).getPathRelativ()

#define U_css 'c' // text/css
#define U_js  'j' // text/javascript

class URDB;
class UHTTP;

template <class T> class UTree;
template <class T> class UVector;

class U_EXPORT UFile : public stat {
public:

   // NB: l'oggetto puo' essere usato come (struct stat) in quanto UMemoryError viene allocato dopo...

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // check for op chdir()

#ifdef DEBUG
   static int num_file_object;
   static void inc_num_file_object();
   static void dec_num_file_object();
   static void chk_num_file_object();
#else
#  define inc_num_file_object()
#  define dec_num_file_object()
#  define chk_num_file_object()
#endif

   void reset()
      {
      U_TRACE(0, "UFile::reset()")

      fd       = -1;
      map      = (char*)MAP_FAILED;
      st_size  = 0;
      map_size = 0;
      }

   // COSTRUTTORI

   UFile()
      {
      U_TRACE_REGISTER_OBJECT(0, UFile, "")

      fd = -1;

      path_relativ = 0;

      inc_num_file_object();
      }

   UFile(const UString& path) : pathname(path) 
      {
      U_TRACE_REGISTER_OBJECT(0, UFile, "%.*S", U_STRING_TO_TRACE(path))

      setPathRelativ();

      inc_num_file_object();
      }

   ~UFile()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UFile)

      dec_num_file_object();

#  ifdef DEBUG
      if (fd != -1) U_WARNING("file descriptor %d not closed...", fd);
#  endif
      }

   // PATH

   void setRoot()
      {
      U_TRACE(0, "UFile::setRoot()")

      reset();

      pathname.setConstant(U_CONSTANT_TO_PARAM("/")); 

      path_relativ_len = 1;
      path_relativ     = pathname.data();

      U_INTERNAL_DUMP("u_cwd            = %S", u_cwd)
      U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)
      }

   void setPath(const UString& path);

   // NB: la stringa potrebbe non essere scrivibile e quindi path_relativ[path_relativ_len] potrebbe non essere '\0'...

   const UString& getPath() const           { return pathname; }
   const char*    getPathRelativ() const    { return path_relativ; }
   int32_t        getPathRelativLen() const { return path_relativ_len; }

   bool isPathRelativ(const UString& name) const
      {
      U_TRACE(0, "UFile::isPathRelativ(%.*S)", U_STRING_TO_TRACE(name))

      U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

      U_INTERNAL_ASSERT_POINTER(path_relativ)
      U_INTERNAL_ASSERT_MAJOR(path_relativ_len,0)

      bool result = name.equal(path_relativ, path_relativ_len);

      U_RETURN(result);
      }

   const char* getSuffix() const
      {
      U_TRACE(0, "UFile::getSuffix()")

      U_INTERNAL_ASSERT_POINTER(path_relativ)

      const char* ptr = (const char*) memrchr(path_relativ, '.', path_relativ_len);

      U_RETURN(ptr);
      }

   uint32_t getSuffixLen(const char* ptr) const
      {
      U_TRACE(0, "UFile::getSuffixLen(%S)", ptr)

      U_INTERNAL_ASSERT_POINTER(path_relativ)

      uint32_t len = (path_relativ  + path_relativ_len) - ptr;

      U_RETURN(len);
      }

   static uint32_t setPathFromFile(const UFile& file, char* buffer_path, const char* suffix, uint32_t len);

   // OPEN - CLOSE

   static int  open(const char*  pathname, int flags,                    mode_t mode);
   static int creat(const char* _pathname, int flags = O_TRUNC | O_RDWR, mode_t mode = PERM_FILE)
      {
      U_TRACE(0, "UFile::creat(%S,%d,%d)", _pathname, flags, mode)

      return open(_pathname, flags | O_CREAT, mode);
      }

   static void close(int _fd)
      {
      U_TRACE(1, "UFile::close(%d)", _fd)

      U_INTERNAL_ASSERT_DIFFERS(_fd, -1)

      (void) U_SYSCALL(close, "%d", _fd);
      }

   bool open(                       int flags = O_RDONLY);
   bool open(const char* _pathname, int flags = O_RDONLY)
      {
      U_TRACE(0, "UFile::open(%S,%d)", _pathname, flags)

      UString path(_pathname);

      setPath(path);

      return open(flags);
      }

   bool open(const UString& path, int flags = O_RDONLY)
      {
      U_TRACE(0, "UFile::open(%.*S,%d)", U_STRING_TO_TRACE(path), flags)

      setPath(path);

      return open(flags);
      }

   bool creat(                     int flags = O_TRUNC | O_RDWR, mode_t mode = PERM_FILE);
   bool creat(const UString& path, int flags = O_TRUNC | O_RDWR, mode_t mode = PERM_FILE);

   bool isOpen()
      {
      U_TRACE(0, "UFile::isOpen()")

      U_CHECK_MEMORY

      bool result = (fd != -1);

      U_RETURN(result);
      }

   void close()
      {
      U_TRACE(0, "UFile::close()")

      U_CHECK_MEMORY

      UFile::close(fd);

      fd = -1;
      }

   void setFd(int _fd)
      {
      U_TRACE(0, "UFile::setFd(%d)", _fd)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_EQUALS(fd, -1)

      fd = _fd;
      }

   int getFd() const
      {
      U_TRACE(0, "UFile::getFd()")

      U_CHECK_MEMORY

      U_RETURN(fd);
      }

   // ACCESS

   bool access(int mode = R_OK | X_OK)
      {
      U_TRACE(1, "UFile::access(%d)", mode)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_POINTER(path_relativ)

      U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

      bool result = (U_SYSCALL(access, "%S,%d", U_PATH_CONV(path_relativ), mode) == 0);

      U_RETURN(result);
      }

   bool stat();

#ifndef __MINGW32__
   void lstat()
      {
      U_TRACE(1, "UFile::lstat()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_POINTER(path_relativ)

      U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

      (void) U_SYSCALL(lstat, "%S,%p", path_relativ, (struct stat*)this);
      }
#endif

   void fstat()
      {
      U_TRACE(1, "UFile::fstat()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_DIFFERS(fd, -1)

      U_INTERNAL_ASSERT_POINTER(path_relativ)

      U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

      (void) U_SYSCALL(fstat, "%d,%p", fd, (struct stat*)this);
      }

   off_t lseek(uint32_t offset, int whence)
      {
      U_TRACE(1, "UFile::lseek(%u,%d)", offset, whence)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_DIFFERS(fd, -1)

      off_t result = U_SYSCALL(lseek, "%d,%u,%d", fd, offset, whence);

      U_RETURN(result);
      }

   void readSize()
      {
      U_TRACE(1, "UFile::readSize()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_DIFFERS(fd, -1)
      U_INTERNAL_ASSERT_EQUALS(st_size,0)
      U_INTERNAL_ASSERT_POINTER(path_relativ)

      U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

      st_size = lseek(U_SEEK_BEGIN, SEEK_END);

      U_INTERNAL_ASSERT(st_size >= U_SEEK_BEGIN)
      }

   off_t size(bool bstat = false);

   off_t getSize() const
      {
      U_TRACE(0, "UFile::getSize()")

      U_CHECK_MEMORY

      U_RETURN(st_size);
      }

   bool empty() const
      {
      U_TRACE(0, "UFile::empty()")

      U_CHECK_MEMORY

      U_RETURN(st_size == 0);
      }

   bool regular() const
      {
      U_TRACE(0, "UFile::regular()")

      U_CHECK_MEMORY

      bool result = S_ISREG(st_mode);

      U_RETURN(result);
      }

   bool dir() const
      {
      U_TRACE(0, "UFile::dir()")

      U_CHECK_MEMORY

      bool result = S_ISDIR(st_mode);

      U_RETURN(result);
      }

   bool socket() const
      {
      U_TRACE(0, "UFile::socket()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_DIFFERS(fd, -1)

      bool result = S_ISSOCK(st_mode);

      U_RETURN(result);
      }

   // Etag (HTTP/1.1)

   UString etag() const
      {
      U_TRACE(0, "UFile::etag()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(st_size,0)
      U_INTERNAL_ASSERT_DIFFERS(st_ino,0)

      UString _etag(100U);

      // NB: The only format constraints are that the string be quoted...

      _etag.snprintf("\"%x-%x-%x\"", st_ino, st_size, st_mtime);

      U_RETURN_STRING(_etag);
      }

   uint64_t inode()
      {
      U_TRACE(0, "UFile::inode()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(st_size,0)
      U_INTERNAL_ASSERT_DIFFERS(st_ino,0)

      U_RETURN(st_ino);
      }

   // MODIFIER

   bool modified()
      {
      U_TRACE(1, "UFile::modified()")

      time_t mtime = st_mtime;

      fstat();

      U_RETURN(mtime != st_mtime);
      }

   static bool isBlocking(int _fd, int& flags) // actual state is blocking...?
      {
      U_TRACE(1, "UFile::isBlocking(%d,%d)", _fd, flags)

      U_INTERNAL_ASSERT_DIFFERS(_fd, -1)

      if (flags == -1) flags = U_SYSCALL(fcntl, "%d,%d,%d", _fd, F_GETFL, 0);

      U_INTERNAL_DUMP("O_NONBLOCK = %B, flags = %B", O_NONBLOCK, flags)

      bool blocking = ((flags & O_NONBLOCK) != O_NONBLOCK);

      U_RETURN(blocking);
      }

   static void setBlocking(int fd, int& flags, bool block);

#ifdef __MINGW32__
#undef mkdir
#undef unlink
#undef rename
#endif

   // mkdir

   static bool mkdir(const char* path, mode_t mode);

   // unlink

   static bool unlink(const char* _pathname)
      {
      U_TRACE(1, "UFile::unlink(%S)", _pathname)

#  ifdef __MINGW32__
      bool result = (U_SYSCALL(unlink_w32, "%S", U_PATH_CONV(_pathname)) == 0);
#  else
      bool result = (U_SYSCALL(unlink,     "%S",             _pathname)  == 0);
#  endif

      U_RETURN(result);
      }

   bool unlink()
      {
      U_TRACE(0, "UFile::unlink()")

      U_CHECK_MEMORY
      U_INTERNAL_ASSERT_POINTER(path_relativ)

      U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

      bool result = UFile::unlink(path_relativ);

      U_RETURN(result);
      }

   // rename

          bool rename(                     const char* newpath);
   static bool rename(const char* oldpath, const char* newpath)
      {
      U_TRACE(1, "UFile::rename(%S,%S)", oldpath, newpath)

#  ifdef __MINGW32__
      bool result = (U_SYSCALL(rename_w32, "%S,%S", oldpath, newpath) != -1);
#  else
      bool result = (U_SYSCALL(rename,     "%S,%S", oldpath, newpath) != -1);
#  endif

      U_RETURN(result);
      }

   void fsync()
      {
      U_TRACE(1, "UFile::fsync()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_DIFFERS(fd, -1)

      U_INTERNAL_ASSERT_POINTER(path_relativ)

      U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

      (void) U_SYSCALL(fsync, "%d", fd);
      }

   bool fallocate(uint32_t n);
   bool ftruncate(uint32_t n);

   static bool chdir(const char* path, bool flag_save = false);

   // LOCKING

   bool lock(short l_type = F_WRLCK, uint32_t start = 0, uint32_t len = 0) const; /* set the lock, waiting if necessary */

   bool unlock(uint32_t start = 0, uint32_t len = 0) const { return lock(F_UNLCK, start, len); }

   // MEMORY MAPPED I/O

   static char* mmap(uint32_t length,
                     int _fd = -1,
                     int prot = PROT_READ | PROT_WRITE,
                     int flags = MAP_SHARED | MAP_ANONYMOUS,
                     uint32_t offset = 0)
      {
      U_TRACE(1, "UFile::mmap(%u,%d,%d,%d,%u)", length, _fd, prot, flags, offset)

#  ifndef HAVE_ARCH64
      U_INTERNAL_ASSERT_RANGE(1U,length,3U*1024U*1024U*1024U) // limit of linux system
#  endif

      char* _map = (char*) U_SYSCALL(mmap, "%d,%u,%d,%d,%d,%u", 0, length, prot, flags, _fd, offset);

      return _map;
      }

   bool isMapped() const
      {
      U_TRACE(0, "UFile::isMapped()")

      U_CHECK_MEMORY

      U_RETURN(map != MAP_FAILED);
      }

   char* getMap() const { return map; }

          void munmap();
   static void munmap(void* _map, uint32_t length)
      {
      U_TRACE(1, "UFile::munmap(%p,%u)", _map, length)

      U_INTERNAL_ASSERT_DIFFERS(_map, MAP_FAILED)

      (void) U_SYSCALL(munmap, "%p,%u", _map, length);
      }

   static void msync(char* ptr, char* page, int flags = MS_SYNC); // flushes changes made to memory mapped file back to disk

   // mremap expands (or shrinks) an existing memory  mapping, potentially moving it at the same time
   // (controlled by the flags argument and the available virtual address space)

   static void* mremap(void* old_address, uint32_t old_size, uint32_t new_size, int flags = 0) // MREMAP_MAYMOVE == 1
      {
      U_TRACE(1, "UFile::mremap(%p,%u,%u,%d)", old_address, old_size, new_size, flags)

      void* result = U_SYSCALL(mremap, "%p,%u,%u,%d", old_address, old_size, new_size, flags);

      U_RETURN(result);
      }

   bool memmap(int prot = PROT_READ, UString* str = 0, uint32_t offset = 0, uint32_t length = 0);

   char* eof() const
      {
      U_TRACE(0, "UFile::eof()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_DIFFERS(map, MAP_FAILED)

      U_INTERNAL_DUMP("st_size = %I map_size = %u", st_size, map_size)

      char* result = map + (st_size ? (uint32_t)st_size : map_size);

      U_RETURN(result);
      }

          UString getContent(                        bool brdonly = true,  bool bstat = false, bool bmap = true);
   static UString contentOf(const char*    pathname, int flags = O_RDONLY, bool bstat = false, bool bmap = true);
   static UString contentOf(const UString& pathname, int flags = O_RDONLY, bool bstat = false, bool bmap = true);

   static void mkSharedMemory(UString& buffer, uint32_t size)
      {
      U_TRACE(0, "UFile::mkSharedMemory(%.*S,%u)", U_STRING_TO_TRACE(buffer), size)

      buffer.mmap(mmap(size), size);
      }  

   // MIME TYPE

   static int mime_index;

          const char* getMimeType();
   static const char* getMimeType(const char* suffix);

   // PREAD - PWRITE

   static bool pread(int _fd, void* buf, uint32_t count, uint32_t offset)
      {
      U_TRACE(1, "UFile::pread(%d,%p,%u,%u)", _fd, buf, count, offset)

      bool result = (U_SYSCALL(pread, "%d,%p,%u,%u", _fd, buf, count, offset) == (ssize_t)count);

      U_RETURN(result);
      }

   static bool pwrite(int _fd, const void* buf, uint32_t count, uint32_t offset)
      {
      U_TRACE(1, "UFile::pwrite(%d,%p,%u,%u)", _fd, buf, count, offset)

      bool result = (U_SYSCALL(pwrite, "%d,%p,%u,%u", _fd, buf, count, offset) == (ssize_t)count);

      U_RETURN(result);
      }

   bool pread(       void* buf, uint32_t count, uint32_t offset);
   bool pwrite(const void* buf, uint32_t count, uint32_t offset);

   // SERVICES

   static bool access(const char* path, int mode = R_OK | X_OK)
      {
      U_TRACE(1, "UFile::access(%S,%d)", path, mode)

      bool result = (U_SYSCALL(access, "%S,%d", U_PATH_CONV(path), mode) == 0);

      U_RETURN(result);
      }

   static bool stat(const char* path, struct stat* buf)
      {
      U_TRACE(1, "UFile::stat(%S,%p)", path, buf)

      bool result = (U_SYSCALL(stat, "%S,%p", U_PATH_CONV(path), buf) == 0);

      U_RETURN(result);
      }

   static bool write(int _fd, const void* buf, uint32_t count)
      {
      U_TRACE(1, "UFile::write(%d,%p,%u)", _fd, buf, count)

      bool result = (U_SYSCALL(write, "%d,%p,%u", _fd, buf, count) == (ssize_t)count);

      U_RETURN(result);
      }

   static int writev(int _fd, const struct iovec* iov, int n)
      {
      U_TRACE(1, "UFile::writev(%d,%p,%d)", _fd, iov, n)

      int result = U_SYSCALL(writev, "%d,%p,%d", _fd, iov, n);

      U_RETURN(result);
      }

   int writev(const struct iovec* iov, int n) const
      {
      U_TRACE(0, "UFile::writev(%p,%d)", iov, n)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_DIFFERS(fd, -1)
      U_INTERNAL_ASSERT_POINTER(path_relativ)

      U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

      int result = UFile::writev(fd, iov, n);

      U_RETURN(result);
      }

          bool write(                        const UString& data, bool append = false, bool bmkdirs = false);
   static bool writeTo(const UString& path,  const UString& data, bool append = false, bool bmkdirs = false);
   static bool writeToTmpl(const char* tmpl, const UString& data, bool append = false, bool bmkdirs = false);

   // symlink

#ifndef __MINGW32__

   static bool symlink(const char* oldpath, const char* newpath)
      {
      U_TRACE(1, "UFile::symlink(%S,%S)", oldpath, newpath)

      bool result = (U_SYSCALL(symlink, "%S,%S", oldpath, newpath) != -1);

      U_RETURN(result);
      }

   bool symlink(const char* newpath)
      {
      U_TRACE(0, "UFile::symlink(%S)", newpath)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_POINTER(path_relativ)

      U_INTERNAL_DUMP("path_relativ(%u) = %.*S", path_relativ_len, path_relativ_len, path_relativ)

      bool result = symlink(path_relativ, newpath);

      U_RETURN(result);
      }

#endif

   /*
   Expands all symbolic links and resolves references to '/./', '/../' and extra '/' characters in the null
   terminated string named by path and stores the canonicalized absolute pathname in the return string.
   The resulting string will have no symbolic link, '/./' or '/../' components
   */

   static UString getRealPath(const char* path);

   // TEMP FILES

          bool mkTemp(const char* _tmpl = "lockXXXXXX");     // temporary  file for locking...
   static bool mkTempStorage(UString& space, uint32_t size); // temporary space for upload file...

   // ----------------------------------------------------------------------------------------------------------------------
   // create a unique temporary file
   // ----------------------------------------------------------------------------------------------------------------------
   // char pathname[] = "/tmp/dataXXXXXX"
   // The last six characters of template must be XXXXXX and these are replaced with a string that makes the filename unique
   // ----------------------------------------------------------------------------------------------------------------------

   static int mkstemp(char* _template);

   // ----------------------------------------------------------------------------------------------------------------------
   // mkdtemp - create a unique temporary directory
   // ------------------------------------------------------------------------------------------------------------
   // The mkdtemp() function generates a uniquely-named temporary directory from template. The last six characters
   // of template must be XXXXXX and these are replaced with a string that makes the directory name unique.
   // The directory is then created with permissions 0700. Since it will be modified, template must not be a string
   // constant, but should be declared as a character array.
   // ------------------------------------------------------------------------------------------------------------

   static bool mkdtemp(UString& _template);

   // DIR OP

   static bool rmdir(const char* path, bool remove_all = false);

   // Creates all directories in this path. This method returns true if all directories in this path are created

   static bool mkdirs(const char* path, mode_t mode = PERM_DIRECTORY);

   // If path includes more than one pathname component, remove it, then strip the last component
   // and remove the resulting directory, etc., until all components have been removed.
   // The pathname component must be empty...

   static bool rmdirs(const char* path, bool remove_all = false);

   // LS

   static uint32_t listContentOf(UVector<UString>& vec,         const UString* dir = 0, const UString* filter = 0);
   static void     listRecursiveContentOf(UTree<UString>& tree, const UString* dir = 0, const UString* filter = 0);
   static uint32_t buildFilenameListFrom(UVector<UString>& vec, const UString& arg,     char sep = ',');

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   char* map;
   UString pathname;
   const char* path_relativ;            // la stringa potrebbe non essere scrivibile...
   uint32_t path_relativ_len, map_size; // size to mmap(), may be larger than the size of the file...
   int fd;

   static bool _root;
   static UTree<UString>* tree;
   static UVector<UString>* vector;

   static char     cwd_save[U_PATH_MAX];
   static uint32_t cwd_len_save;

   void setPathRelativ();
   void substitute(UFile& file);
   void setPath(const UFile& file, char* buffer_path, const char* suffix, uint32_t len);

   static void ftw_tree_up();
   static void ftw_tree_push();
   static void ftw_vector_push();

private:
#ifdef __MINGW32__
   uint64_t u_inode;
#endif

   UFile(const UFile&)            {}
   UFile& operator=(const UFile&) { return *this; }

   friend class URDB;
   friend class UHTTP;
};

#endif
