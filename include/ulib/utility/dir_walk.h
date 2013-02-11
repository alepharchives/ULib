// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    dir_walk.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_DIR_WALK_H
#define ULIB_DIR_WALK_H 1

#include <ulib/string.h>

/* FTW - walks through the directory tree starting from the indicated directory.
 *       For each found entry in the tree, it calls foundFile()
 */

                   class IR;
                   class UHTTP;
template <class T> class UTree;
template <class T> class UVector;
                   class PEC_report;

class U_EXPORT UDirWalk {
public:

   typedef struct dirent_s {
      ino_t         d_ino;
      uint32_t      d_name, d_namlen;
      unsigned char d_type;
   } dirent_s;

   typedef struct dir_s {
      char* free;
      dirent_s* dp;
      uint32_t num, max, pfree, nfree, szfree;
   } dir_s;

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // Costruttori

            UDirWalk(const UString* dir = 0, const char* filter = 0, uint32_t filter_len = 0);
   virtual ~UDirWalk()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UDirWalk)
      }

   // Begin the journey

   void     walk();
   uint32_t walk(UTree<UString>& tree);
   uint32_t walk(UVector<UString>& vec);

   // SERVICES

   static bool isDirectory()
      {
      U_TRACE(0, "UDirWalk::isDirectory()")

      U_INTERNAL_ASSERT_POINTER(pthis)

      U_RETURN(pthis->is_directory);
      }

   static void setRecurseSubDirs(bool bcall_if_directory = true)
      {
      U_TRACE(0, "UDirWalk::setRecurseSubDirs(%b)", bcall_if_directory)

      brecurse          = true;
      call_if_directory = bcall_if_directory;
      }

   static void setSortingForInode()
      {
      U_TRACE(0, "UDirWalk::setSortingForInode()")

      sort_by = cmp_inode;
      }

   static void resetCheckForFileType()
      {
      U_TRACE(0, "UDirWalk::resetCheckForFileType()")

      filetype = '\0';
      }

   static void setFoundFile(UString& path)
      {
      U_TRACE(0, "UDirWalk::setFoundFile(%.*S)", U_STRING_TO_TRACE(path))

      U_INTERNAL_ASSERT_POINTER(pthis)

      U_INTERNAL_DUMP("depth = %d pathlen = %u pathname(%u) = %S", pthis->depth, pthis->pathlen, u__strlen(pthis->pathname), pthis->pathname)

      path.replace(pthis->pathname+2, pthis->pathlen-2);
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   int depth;
   uint32_t pathlen;
   bool is_directory;
   char pathname[4000];

   static char filetype;
   static UDirWalk* pthis;
   static qcompare sort_by;
   static const char* filter;
   static UTree<UString>* ptree;
   static uint32_t max, filter_len;
   static UVector<UString>* pvector;
   static vPF call_if_up, call_internal;
   static bool tree_root, call_if_directory, brecurse; // recurse subdirectories?

   // foundFile() is called whenever another file or directory is
   // found that meets the criteria in effect for the object. This
   // can be overridden in derived classes

   virtual void foundFile()
      {
      U_TRACE(0, "UDirWalk::foundFile()")

      if (call_internal) call_internal();
      }

   static int cmp_inode(const void* a, const void* b) { return (((const dirent_s*)a)->d_ino - ((const dirent_s*)b)->d_ino); }

private:
   void recurse() U_NO_EXPORT; // performs the actual work
   void prepareForCallingRecurse(char* d_name, uint32_t d_namlen, unsigned char d_type) U_NO_EXPORT;

   static void treeUp() U_NO_EXPORT;
   static void treePush() U_NO_EXPORT;
   static void vectorPush() U_NO_EXPORT;

   UDirWalk(const UDirWalk&)            {}
   UDirWalk& operator=(const UDirWalk&) { return *this; }

   friend class IR;
   friend class UHTTP;
   friend class PEC_report;
};
#endif
