// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    dir_walk.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file.h>
#include <ulib/container/tree.h>
#include <ulib/utility/dir_walk.h>

/* FTW - walks through the directory tree starting from the indicated directory.
 *       For each found entry in the tree, it calls foundFile()
 *
 * struct dirent {
 *    ino_t          d_ino;       // inode number
 *    off_t          d_off;       // offset to the next dirent
 *    unsigned short d_reclen;    // length of this record
 *    unsigned char  d_type;      // type of file; not supported by all file system types
 *    char           d_name[256]; // filename
 * };
 */

#ifndef DT_DIR
#define DT_DIR      4
#endif
#ifndef DT_REG
#define DT_REG      8
#endif
#ifndef DT_LNK
#define DT_LNK     10
#endif
#ifndef DT_UNKNOWN
#define DT_UNKNOWN  0
#endif

#ifdef _DIRENT_HAVE_D_TYPE
#define U_DT_TYPE dp->d_type
#else
#define U_DT_TYPE DT_UNKNOWN
#endif

vPF               UDirWalk::call_if_up;
vPF               UDirWalk::call_internal;
char              UDirWalk::filetype;
bool              UDirWalk::brecurse; // recurse subdirectories ?
bool              UDirWalk::tree_root;
bool              UDirWalk::call_if_directory;
qcompare          UDirWalk::sort_by;
uint32_t          UDirWalk::max;
uint32_t          UDirWalk::filter_len;
UDirWalk*         UDirWalk::pthis;
const char*       UDirWalk::filter;
UTree<UString>*   UDirWalk::ptree;
UVector<UString>* UDirWalk::pvector;

UDirWalk::UDirWalk(const char* dir, const char* _filter, uint32_t _filter_len)
{
   U_TRACE_REGISTER_OBJECT(0, UDirWalk, "%S,%.*S,%u", dir, _filter_len, _filter, _filter_len)

   max               = 128 * 1024;
   depth             = -1; // starting recursion depth
   pthis             = this;
   sort_by           = 0;
   filetype          = '.';
   call_if_up        = 0;
   call_internal     = 0;
   call_if_directory = brecurse = is_directory = false;

   if (dir) (void) setDirectory(dir, _filter, _filter_len);
   else
      {
      pathname[0]             = '.';
      pathname[(pathlen = 1)] = '\0';

      if ((filter_len = _filter_len))
         {
         filter      = _filter;
         u_pfn_flags = 0;
         u_pfn_match = u_dosmatch_with_OR;
         }
      }
}

bool UDirWalk::setDirectory(const char* dir, const char* _filter, uint32_t _filter_len)
{
   U_TRACE(0, "UDirWalk::setDirectory(%S,%.*S,%u", dir, _filter_len, _filter, _filter_len)

   pthis->pathlen = u__strlen(dir);

   dir = u_getPathRelativ(dir, &(pthis->pathlen));

   U_INTERNAL_ASSERT_MAJOR(pthis->pathlen, 0)

   if (UFile::access(dir) == false)
      {
      pthis->pathlen = 0;

      U_RETURN(false);
      }

   U__MEMCPY(pthis->pathname, dir, pthis->pathlen);

   pthis->pathname[pthis->pathlen] = '\0';

   if (_filter_len)
      {
      filter     = _filter;
      filter_len = _filter_len;

      u_pfn_flags = 0;
      u_pfn_match = u_dosmatch_with_OR;
      }

   U_RETURN(true);
}

U_NO_EXPORT void UDirWalk::prepareForCallingRecurse(char* d_name, uint32_t d_namlen, unsigned char d_type)
{
   U_TRACE(0, "UDirWalk::prepareForCallingRecurse(%.*S,%u,%d)", d_namlen, d_name, d_namlen, d_type)

   U_INTERNAL_ASSERT_EQUALS(pthis, this)

   if (d_type == DT_REG ||
       d_type == DT_DIR ||
       d_type == DT_LNK ||
       d_type == DT_UNKNOWN)
      {
      u__memcpy(pathname + pathlen, d_name, d_namlen, __PRETTY_FUNCTION__);

      pathlen += d_namlen;

      U_INTERNAL_ASSERT_MINOR(pathlen, sizeof(pathname))

      pathname[pathlen] = '\0';

      is_directory = (d_type == DT_DIR);

      if (brecurse &&
          (is_directory     ||
           d_type == DT_LNK ||
           d_type == DT_UNKNOWN))
         {
         recurse();

         goto end;
         }

      foundFile();

end:
      pathlen -= d_namlen;
      }
}

void UDirWalk::recurse()
{
   U_TRACE(1+256, "UDirWalk::recurse()")

   U_INTERNAL_ASSERT_EQUALS(pthis, this)

   DIR* dirp;

   ++depth; // if this has been called, then we're one level lower

   U_INTERNAL_DUMP("depth = %d pathlen = %u pathname(%u) = %S", depth, pathlen, u__strlen(pathname), pathname)

   U_INTERNAL_ASSERT_EQUALS(u__strlen(pathname), pathlen)

   if (depth == 0) dirp = (DIR*) U_SYSCALL(opendir, "%S", "."); // NB: if pathname it is not '.' we have already make chdir()... 
   else
      {
      // NB: if it is present the char 'filetype' we check if this
      //     item isn't a directory so we don't need to try opendir()...

      if (filetype)
         {
         const char* ptr = (const char*) memrchr(pathname+1, filetype, pathlen); // NB: we don't use the macro U_SYSCALL to avoid warning on stderr...

         if (ptr++                &&
             (u_get_mimetype(ptr) ||
              U_STRNEQ(ptr, U_LIB_SUFFIX)))
            {
            is_directory = false;

            foundFile();

            goto end;
            }
         }

      dirp = (DIR*) U_SYSCALL(opendir, "%S", U_PATH_CONV(pathname));
      }

   is_directory = (dirp != 0);

   if (is_directory == false ||
       call_if_directory)
      {
      foundFile();
      }

   if (is_directory)
      {
      dir_s qdir;
      dirent_s* ds;
      uint32_t d_namlen;
      struct dirent* dp;

      qdir.num            = 0;
      pathname[pathlen++] = '/';

      if (sort_by)
         {
         U_INTERNAL_ASSERT_MAJOR(max, 0)

         qdir.max   = max;
         qdir.dp    = (dirent_s*) UMemoryPool::_malloc(&qdir.max, sizeof(dirent_s));

         qdir.szfree = qdir.max * 128;
         qdir.free   = (char*) UMemoryPool::_malloc(&qdir.szfree);
         qdir.nfree  =                               qdir.szfree;
         qdir.pfree  = 0;
         }

      // -----------------------------------------
      // NB: NON sono sempre le prime due entry !!
      // -----------------------------------------
      // (void) readdir(dirp); // skip '.'
      // (void) readdir(dirp); // skip '..'
      // -----------------------------------------

      while ((dp = readdir(dirp))) // NB: we don't use the macro U_SYSCALL to avoid warning on stderr...
         {
         d_namlen = NAMLEN(dp);

         U_INTERNAL_DUMP("d_namlen = %u d_name = %.*s", d_namlen, d_namlen, dp->d_name)

         if (U_ISDOTS(dp->d_name)) continue;

         if (filter_len == 0 ||
             u_pfn_match(dp->d_name, d_namlen, filter, filter_len, u_pfn_flags))
            {
            if (sort_by == 0) prepareForCallingRecurse(dp->d_name, d_namlen, U_DT_TYPE);
            else
               {
               // NB: check if we must do reallocation...

               if (qdir.num >= qdir.max)
                  {
                  uint32_t  old_max   = qdir.max;
                  dirent_s* old_block = qdir.dp;

                  qdir.max <<= 1;

                  U_INTERNAL_DUMP("Reallocating dirent (%u => %u)", old_max, qdir.max)

                  qdir.dp = (dirent_s*) UMemoryPool::_malloc(&qdir.max, sizeof(dirent_s));

                  U__MEMCPY(qdir.dp, old_block, old_max * sizeof(dirent_s));

                  UMemoryPool::_free(old_block, old_max, sizeof(dirent_s));
                  }

               if (d_namlen > qdir.nfree)
                  {
                  char*    old_block = qdir.free;
                  uint32_t old_free  = qdir.szfree;

                  qdir.szfree <<= 1;

                  qdir.free  = (char*) UMemoryPool::_malloc(&qdir.szfree);
                  qdir.nfree = (qdir.szfree - qdir.pfree);

                  U_INTERNAL_DUMP("Reallocating dirname (%u => %u) nfree = %u", old_free, qdir.szfree, qdir.nfree)

                  U__MEMCPY(qdir.free, old_block, qdir.pfree);

                  UMemoryPool::_free(old_block, old_free);
                  }

               ds         = qdir.dp + qdir.num++;
               ds->d_ino  = dp->d_ino;
               ds->d_type = U_DT_TYPE;

               u__memcpy(qdir.free + (ds->d_name = qdir.pfree), dp->d_name, (ds->d_namlen = d_namlen), __PRETTY_FUNCTION__);

               qdir.pfree += d_namlen;
               qdir.nfree -= d_namlen;

               U_INTERNAL_DUMP("readdir: %lu %.*s %d", ds->d_ino, ds->d_namlen, qdir.free + ds->d_name, ds->d_type);
               }
            }
         }

      (void) U_SYSCALL(closedir, "%p", dirp);

      U_INTERNAL_DUMP("qdir.num = %u", qdir.num)

      if (qdir.num)
         {
         U_SYSCALL_VOID(qsort, "%p,%u,%d,%p", qdir.dp, qdir.num, sizeof(dirent_s), sort_by);

         for (uint32_t i = 0; i < qdir.num; ++i)
            {
            ds = qdir.dp + i;

            prepareForCallingRecurse(qdir.free + ds->d_name, ds->d_namlen, ds->d_type);
            }
         }

      if (sort_by)
         {
         UMemoryPool::_free(qdir.free, qdir.szfree);
         UMemoryPool::_free(qdir.dp,   qdir.max, sizeof(dirent_s));
         }

      --pathlen;

      if (call_if_up) call_if_up(); // for UTree<UString>::load() ...
      }

end:
   --depth; // we're returning to the parent's depth now

   U_INTERNAL_DUMP("depth = %d", depth)
}

void UDirWalk::walk()
{
   U_TRACE(0, "UDirWalk::walk()")

   U_INTERNAL_DUMP("u_cwd    = %S", u_cwd)
   U_INTERNAL_DUMP("pathname = %S", pathname)

   U_INTERNAL_ASSERT_EQUALS(pthis, this)

   if (pathlen     == 1 &&
       pathname[0] == '.')
      {
      recurse();

      pathname[0]             = '.';
      pathname[(pathlen = 1)] = '\0';
      }
   else
      {
      // NB: we need our own backup of current directory (see IR)...

      char cwd_save[U_PATH_MAX];

      (void) u__strcpy(cwd_save, u_cwd);

      if (UFile::chdir(pathname, false))
         {
         char pathname_save[U_PATH_MAX];
         uint32_t pathlen_save = pathlen;

         (void) u__strcpy(pathname_save, pathname);

         recurse();

         (void) UFile::chdir(cwd_save, false);

         pathlen = pathlen_save;

         (void) u__strcpy(pathname, pathname_save);
         }
      }

   U_INTERNAL_DUMP("u_cwd    = %S", u_cwd)
   U_INTERNAL_DUMP("pathname = %S", pathname)
}

U_NO_EXPORT void UDirWalk::vectorPush()
{
   U_TRACE(0, "UDirWalk::vectorPush()")

   U_INTERNAL_ASSERT_POINTER(pthis)
   U_INTERNAL_ASSERT_POINTER(pvector)

   U_INTERNAL_DUMP("depth = %d pathlen = %u pathname(%u) = %S", pthis->depth, pthis->pathlen, u__strlen(pthis->pathname), pthis->pathname)

   uint32_t len    = pthis->pathlen;
   const char* ptr = pthis->pathname;

   if (ptr[0] == '.')
      {
      if (len == 1) return;

      if (IS_DIR_SEPARATOR(ptr[1]))
         {
         U_INTERNAL_ASSERT_MAJOR(len, 2)

         ptr += 2;
         len -= 2;
         }
      }

   UString str((void*)ptr, len);

   pvector->push(str);
}

uint32_t UDirWalk::walk(UVector<UString>& vec)
{
   U_TRACE(0, "UDirWalk::walk(%p)", &vec)

   U_INTERNAL_ASSERT_EQUALS(pthis, this)

   pvector = &vec;

   call_internal = vectorPush;

   walk();

   uint32_t result = vec.size();

   U_RETURN(result);
}

U_NO_EXPORT void UDirWalk::treePush()
{
   U_TRACE(0, "UDirWalk::treePush()")

   U_INTERNAL_DUMP("is_directory = %b", pthis->is_directory)

   U_INTERNAL_ASSERT_POINTER(ptree)

   if (tree_root) tree_root = false;
   else
      {
      UString str((void*)pthis->pathname, pthis->pathlen);

      UTree<UString>* _ptree = ptree->push(str);

      if (pthis->is_directory) ptree = _ptree;
      }
}

U_NO_EXPORT void UDirWalk::treeUp()
{
   U_TRACE(0, "UDirWalk::treeUp()")

   U_INTERNAL_ASSERT_POINTER(ptree)

   ptree = ptree->parent();
}

uint32_t UDirWalk::walk(UTree<UString>& tree)
{
   U_TRACE(0, "UDirWalk::walk(%p)", &tree)

   U_INTERNAL_ASSERT_EQUALS(pthis, this)

   ptree     = &tree;
   tree_root = true;

   setRecurseSubDirs();

   call_if_up    = treeUp;
   call_internal = treePush;

   UString str((void*)pathname, pathlen);

   tree.setRoot(str);

   walk();

   uint32_t result = tree.size();

   U_RETURN(result);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UDirWalk::dump(bool reset) const
{
   *UObjectIO::os << "max               " << max                  << '\n'
                  << "pathlen           " << pathlen              << '\n'
                  << "sort_by           " << (void*)sort_by       << '\n'
                  << "brecurse          " << brecurse             << '\n'
                  << "filetype          " << filetype             << '\n'
                  << "call_if_up        " << (void*)call_if_up    << '\n'
                  << "filter_len        " << filter_len           << '\n'
                  << "is_directory      " << is_directory         << '\n'
                  << "call_if_directory " << call_if_directory;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
