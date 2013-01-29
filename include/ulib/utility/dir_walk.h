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

/* FTW - walks through the directory tree starting from the indicated directory dir. For each found entry in
 *       the tree, it calls fn() with the full pathname of the entry, his length and if it is a directory
 */

class U_EXPORT UDirWalk {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // Costruttori

    UDirWalk(const UString* dir = 0, const char* filter = 0, uint32_t filter_len = 0);
   ~UDirWalk()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UDirWalk)
      }

   // SERVICES

   // STREAM

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   qcompare sort_by;
   vPF call, call_if_up;
   uint32_t filter_len;
   const char* filter;
   bool depth, is_directory, call_if_directory;

   /* NB: we use u_buffer...
    * -----------------------
    * uint32_t u_ftw_pathlen;
    * char*    u_ftw_fullpath;
    * -----------------------
    */

private:
   UDirWalk(const UDirWalk&)            {}
   UDirWalk& operator=(const UDirWalk&) { return *this; }
};
#endif

/*
private:
 // Member variables that affect the object as a whole
    BOOL mRecurse;                        // Recurse subdirectories?
    BOOL mListSubDirs; //Include Subdirectory names while recursing?
    char mSearchSpec[_MAX_PATH]; // Include only files matching this
    char mStartingDirectory[_MAX_PATH];  //The beginning of our walk
#if defined(_MT)
 char mCurrentDirectory[_MAX_PATH];
 char mCurrentDirectorySearch[_MAX_PATH];
 char mFullPathName[_MAX_PATH];
#endif
 // Member variables that deal only with the current file
    BOOL mFoundAnother;   // Found another in the current directory?
    BOOL mIsDir;                          // Is this one a directory
    DWORD mSize;                                // Current file size
    DWORD mSizeHigh;                // High double word of file size
    int mDepth;               // Recursion level of the current file
    WIN32_FIND_DATA mFindData;//Lots of stuff about the current file
 enum { ShortFileNameLength=13 };
    char mShortFileName[ShortFileNameLength];  // Holds the 8.3 name
 
 // Performs some initializations common to all the constructors
 void ConstructorHelper(const BOOL RecurseSubDirs,
                     const BOOL ListSubDirs) throw();
    void Recurse() throw(runtime_error);//Performs the actual work
 BOOL IsChildDir() const throw();
 HANDLE FindFirstChildDir() throw();
 BOOL FindNextChildDir(HANDLE hFindFile) throw();

protected:
 // Recursion level of the current file
 inline int Depth()   const throw() { return mDepth; }
 // Size of current file. Use SizeHigh() if over 4.2GB.
 inline DWORD Size()   const throw() { return mSize;  }
 inline DWORD SizeHigh()  const throw() { return mSizeHigh; }
 FileAttributes mFA;
 FileTime mCreationTime;
 FileTime mLastAccessTime;
 FileTime mLastWriteTime;

 inline const char* const Filename() const throw() {
  return mFindData.cFileName; }
 const char* const ShortFilename() throw();
#if defined(_MT)
 const char* const FullPathName() throw();
#endif
 // FoundFile() is called whenever another file or directory is
 // found that meets the criteria in effect for the object. This
 // must be overridden in derived classes.
 virtual void FoundFile()=0;

public:
 // Constructors. RecurseSubDirs specifies whether recursion down
 // into subdirectories takes place. 
    // ListSubDirs specifies whether the subdirectories themselves
    // are reported to FoundFile(). This is useful if you're, say,
 // generating a checksum for a tree of files--you can't generate
    // a checksum on a directory, so there's no point in having it
 // listed.
    // StartingDirectory is the directory in which to begin walking.
    // SearchSpec is the Windows wildcard-compatible file
 // specification.
#if defined(_MSC_VER)
 DirWalk(const BOOL RecurseSubDirs=TRUE,
      const BOOL ListSubDirs=TRUE);
 DirWalk(const char* StartingDirectory,
      const BOOL RecurseSubDirs=TRUE,
   const BOOL ListSubDirs=TRUE);
 DirWalk(const char* StartingDirectory,
      const char* SearchSpec,
   const BOOL RecurseSubDirs=TRUE,
   const BOOL ListSubDirs=TRUE);
#else
 DirWalk(const BOOL RecurseSubDirs=TRUE,
      const BOOL ListSubDirs=TRUE) throw();
 DirWalk(const char* StartingDirectory,
      const BOOL RecurseSubDirs=TRUE,
   const BOOL ListSubDirs=TRUE) throw();
    DirWalk(const char* StartingDirectory,const char* SearchSpec,
   const BOOL RecurseSubDirs=TRUE,
   const BOOL ListSubDirs=TRUE) throw();
#endif
 void Walk() throw(runtime_error); // Begin the journey.
};
*/
