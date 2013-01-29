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
#include <ulib/utility/dir_walk.h>

/* FTW - walks through the directory tree starting from the indicated directory.
 *       For each found entry in the tree, it calls fn() with the full pathname of the entry, his length and if it is a directory
 *
 * struct dirent {
 *    ino_t          d_ino;       // inode number
 *    off_t          d_off;       // offset to the next dirent
 *    unsigned short d_reclen;    // length of this record
 *    unsigned char  d_type;      // type of file; not supported by all file system types
 *    char           d_name[256]; // filename
 * };
 */

struct u_dirent_s {
   ino_t         d_ino;
   uint32_t      d_name, d_namlen;
   unsigned char d_type;
};

struct u_dir_s {
   char* restrict free;
   struct u_dirent_s* dp;
   uint32_t num, max, pfree, nfree, szfree;
};

#define U_DIRENT_ALLOCATE   (128U * 1024U)
#define U_FILENAME_ALLOCATE ( 32U * U_DIRENT_ALLOCATE)

UDirWalk::UDirWalk(const UString* dir, const char* _filter, uint32_t _filter_len)
{
   U_TRACE_REGISTER_OBJECT(0, UDirWalk, "%p,%.*S,%u", dir, _filter_len, _filter, _filter_len)

   depth = true;

   if (dir == 0)
      {
      u_buffer[0]  = '.';
      u_buffer_len = 1;
      }
   else
      {
      u_buffer_len    = dir->size();
      const char* ptr = dir->c_str();

      U_INTERNAL_DUMP("dir      = %S", ptr)

      ptr = u_getPathRelativ(ptr, &u_buffer_len);

      U_INTERNAL_ASSERT_MAJOR(u_buffer_len, 0)

      if (UFile::access(ptr) == false)
         {
         u_buffer_len = 0;

         return;
         }

      U__MEMCPY(u_buffer, ptr, u_buffer_len);
      }

   u_buffer[u_buffer_len] = '\0';

   filter     = _filter;
   filter_len = _filter_len;

   if (_filter_len)
      {
      u_pfn_flags = 0;
      u_pfn_match = u_dosmatch_with_OR;
      }

   U_INTERNAL_DUMP("u_cwd    = %S", u_cwd)
   U_INTERNAL_DUMP("u_buffer = %S", u_buffer)
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UDirWalk::dump(bool reset) const
{
   U_CHECK_MEMORY

   *UObjectIO::os << "depth             " << depth                << '\n'
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

/*
static inline void u_ftw_allocate(struct u_dir_s* restrict u_dir)
{
   U_INTERNAL_TRACE("u_ftw_allocate(%p)", u_dir)

   if (u_ftw_ctx.sort_by)
      {
      u_dir->free  = (char* restrict)     malloc((u_dir->nfree = u_dir->szfree = U_FILENAME_ALLOCATE));
      u_dir->dp    = (struct u_dirent_s*) malloc((u_dir->max                   = U_DIRENT_ALLOCATE) * sizeof(struct u_dirent_s));
      u_dir->pfree = 0U;
      }
   else
      {
      (void) memset(u_dir, 0, sizeof(struct u_dir_s));
      }

   u_dir->num = 0U;
}

static inline void u_ftw_deallocate(struct u_dir_s* restrict u_dir)
{
   U_INTERNAL_TRACE("u_ftw_deallocate(%p)", u_dir)

   if (u_ftw_ctx.sort_by)
      {
      free(u_dir->free);
      free(u_dir->dp);
      }
}

static inline void u_ftw_reallocate(struct u_dir_s* restrict u_dir, uint32_t d_namlen)
{
   U_INTERNAL_TRACE("u_ftw_reallocate(%p,%u)", u_dir, d_namlen)

   U_INTERNAL_ASSERT_DIFFERS(u_ftw_ctx.sort_by,0)

   if (u_dir->num >= u_dir->max)
      {
      u_dir->max += U_DIRENT_ALLOCATE;

      U_INTERNAL_PRINT("Reallocating u_dir->dp to size %u", u_dir->max)

      u_dir->dp = (struct u_dirent_s* restrict) realloc(u_dir->dp, u_dir->max * sizeof(struct u_dirent_s));

      U_INTERNAL_ASSERT_POINTER(u_dir->dp)
      }

   if (d_namlen > u_dir->nfree)
      {
      u_dir->nfree  += U_FILENAME_ALLOCATE;
      u_dir->szfree += U_FILENAME_ALLOCATE;

      U_INTERNAL_PRINT("Reallocating u_dir->free to size %u", u_dir->szfree)

      u_dir->free = (char* restrict) realloc(u_dir->free, u_dir->szfree);

      U_INTERNAL_ASSERT_POINTER(u_dir->free)
      }
}

static void u_ftw_call(char* restrict d_name, uint32_t d_namlen, unsigned char d_type)
{
   U_INTERNAL_TRACE("u_ftw_call(%.*s,%u,%d)", d_namlen, d_name, d_namlen, d_type)

   if (d_type != DT_REG &&
       d_type != DT_DIR &&
       d_type != DT_LNK &&
       d_type != DT_UNKNOWN) return;

   u__memcpy(u_buffer + u_buffer_len, d_name, d_namlen, __PRETTY_FUNCTION__);

   u_buffer_len += d_namlen;

   U_INTERNAL_ASSERT_MINOR(u_buffer_len,4096)

   u_buffer[u_buffer_len] = '\0';

   u_ftw_ctx.is_directory = (d_type == DT_DIR);

   if ( u_ftw_ctx.depth &&
       (u_ftw_ctx.is_directory ||
        d_type == DT_LNK       ||
        d_type == DT_UNKNOWN))
      {
      u_ftw();

      goto end;
      }

   u_ftw_ctx.call();

end:
   u_buffer_len -= d_namlen;
}

__pure int u_ftw_ino_cmp(const void* restrict a, const void* restrict b)
{ return (((const struct u_dirent_s* restrict)a)->d_ino - ((const struct u_dirent_s* restrict)b)->d_ino); }

static void u_ftw_readdir(DIR* restrict dirp)
{
   uint32_t i, d_namlen;
   struct u_dir_s u_dir;
   struct dirent* restrict dp;
   struct u_dirent_s* restrict ds;

   U_INTERNAL_TRACE("u_ftw_readdir(%p)", dirp)

   u_ftw_allocate(&u_dir);

   // -----------------------------------------
   // NB: NON sono sempre le prime due entry !!
   // -----------------------------------------
   // (void) readdir(dirp);         // skip '.'
   // (void) readdir(dirp);         // skip '..'
   // -----------------------------------------
   //

   while ((dp = (struct dirent* restrict) readdir(dirp)))
      {
      d_namlen = NAMLEN(dp);

      U_INTERNAL_PRINT("d_namlen = %u d_name = %.*s", d_namlen, d_namlen, dp->d_name)

      if (U_ISDOTS(dp->d_name)) continue;

      if (u_ftw_ctx.filter_len == 0 ||
          u_pfn_match(dp->d_name, d_namlen, u_ftw_ctx.filter, u_ftw_ctx.filter_len, u_pfn_flags))
         {
         if (u_ftw_ctx.sort_by)
            {
            u_ftw_reallocate(&u_dir, d_namlen); // check se necessarie nuove allocazioni

            ds = u_dir.dp + u_dir.num++;

            ds->d_ino  = dp->d_ino;
            ds->d_type = U_DT_TYPE;

            u__memcpy(u_dir.free + (ds->d_name = u_dir.pfree), dp->d_name, (ds->d_namlen = d_namlen), __PRETTY_FUNCTION__);

            u_dir.pfree += d_namlen;
            u_dir.nfree -= d_namlen;

            U_INTERNAL_PRINT("readdir: %lu %.*s %d", ds->d_ino, ds->d_namlen, u_dir.free + ds->d_name, ds->d_type);
            }
         else
            {
            u_ftw_call(dp->d_name, d_namlen, U_DT_TYPE);
            }
         }
      }

   (void) closedir(dirp);

   U_INTERNAL_PRINT("u_dir.num = %u", u_dir.num)

   if (u_dir.num)
      {
      qsort(u_dir.dp, u_dir.num, sizeof(struct u_dirent_s), u_ftw_ctx.sort_by);

      for (i = 0; i < u_dir.num; ++i)
         {
         ds = u_dir.dp + i;

         u_ftw_call(u_dir.free + ds->d_name, ds->d_namlen, ds->d_type);
         }
      }

   u_ftw_deallocate(&u_dir);
}

void u_ftw(void)
{
   DIR* restrict dirp;

   U_INTERNAL_TRACE("u_ftw()")

   U_INTERNAL_ASSERT_EQUALS(u__strlen(u_buffer), u_buffer_len)

   // NB: if is present the char 'filetype' this item isn't a directory and we don't need to try opendir()...
   //
   // dirp = (DIR*) (u_ftw_ctx.filetype && strchr(u_buffer+1, u_ftw_ctx.filetype) ? 0 : opendir(u_buffer));

   u_ftw_ctx.is_directory = ((dirp = (DIR* restrict) opendir(U_PATH_CONV(u_buffer))) != 0);

   if (u_ftw_ctx.is_directory == false ||
       u_ftw_ctx.call_if_directory)
      {
      u_ftw_ctx.call();
      }

   if (u_ftw_ctx.is_directory)
      {
      u_buffer[u_buffer_len++] = '/';

      u_ftw_readdir(dirp);

      --u_buffer_len;

      if (u_ftw_ctx.call_if_up) u_ftw_ctx.call_if_up(); // for UTree<UString>::load() ...
      }
}
*/
