// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    zip.h - interface to the ziplib library
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_ZIP_H
#define U_ZIP_H 1

#include <ulib/file.h>
#include <ulib/zip/ziptool.h>
#include <ulib/container/vector.h>

#define U_ZIP_ARCHIVE "PK\003\004"

class U_EXPORT UZIP {
public:
   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UZIP(const UString& _content) : content(_content), tmpdir(U_CAPACITY)
      {
      U_TRACE_REGISTER_OBJECT(0, UZIP, "%.*S", U_STRING_TO_TRACE(_content))

      npart         = 0;
      file          = 0;
      valid         = (memcmp(_content.data(), U_CONSTANT_TO_PARAM(U_ZIP_ARCHIVE)) == 0);
      filenames     = filecontents = 0;
      zippartname   = zippartcontent = 0;
      filenames_len = filecontents_len = 0;
      }

   /**
   * Deletes this object.
   */

   ~UZIP()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UZIP)

      clear();
      }

   /**
   * Returns bool value to indicate the correctness of the zip data.
   */

   bool isValid() const
      {
      U_TRACE(0, "UZIP::isValid()")

      U_RETURN(valid);
      }

   bool readContent();
   bool extract(const UString* tmpdir = 0);

   // VARIE

   void clear();

   uint32_t getFilesCount() const          { return zippartname->size(); }
   UString  getFilenameAt(int index) const { return zippartname->at(index); }
   UString  getFileContentAt(int index);

   // OPERATOR

   UString operator[](uint32_t pos) const { return getFilenameAt(pos); }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UFile* file;
   char** filenames;
   char** filecontents;
   uint32_t* filenames_len;
   uint32_t* filecontents_len;
   UVector<UString>* zippartname;
   UVector<UString>* zippartcontent;
   UString content, tmpdir;
   uint32_t npart;
   bool valid;

private:
   void assignFilenames() U_NO_EXPORT;

   UZIP(const UZIP&)            {}
   UZIP& operator=(const UZIP&) { return *this; }
};

#endif
