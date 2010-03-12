// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    zip.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/zip/zip.h>

void UZIP::clear()
{
   U_TRACE(0, "UZIP::clear()")

   if (file)
      {
      U_ASSERT(tmpdir.empty() == false)

      delete file;
             file = 0;

      (void) UFile::rmdir(tmpdir.c_str(), true);
      }

   if (zippartname)
      {
      delete zippartname;
             zippartname = 0;

      U_INTERNAL_ASSERT_MAJOR(npart,0)

      for (uint32_t i = 0; i < npart; ++i)
         {
         U_SYSCALL_VOID(free, "%p", filenames[i]);
         }

      U_SYSCALL_VOID(free, "%p", filenames);
      U_SYSCALL_VOID(free, "%p", filenames_len);

      filenames = 0;
      filenames_len = 0;
      }

   if (zippartcontent)
      {
      delete zippartcontent;
             zippartcontent = 0;

      U_INTERNAL_ASSERT_MAJOR(npart,0)

      for (uint32_t i = 0; i < npart; ++i)
         {
         U_SYSCALL_VOID(free, "%p", filecontents[i]);
         }

      U_SYSCALL_VOID(free, "%p", filecontents);
      U_SYSCALL_VOID(free, "%p", filecontents_len);

      filecontents = 0;
      filecontents_len = 0;
      }
}

U_NO_EXPORT void UZIP::assignFilenames()
{
   U_TRACE(0, "UZIP::assignFilenames()")

   U_INTERNAL_ASSERT(valid)
   U_INTERNAL_ASSERT_MAJOR(npart,0)
   U_INTERNAL_ASSERT_EQUALS(zippartname,0)
   U_INTERNAL_ASSERT_POINTER(filenames)
   U_INTERNAL_ASSERT_POINTER(filenames_len)

   zippartname = U_NEW(UVector<UString>(npart));

   for (uint32_t i = 0; i < npart; ++i) zippartname->push_back(UString(filenames[i], filenames_len[i]));
}

bool UZIP::extract(const UString* _tmpdir)
{
   U_TRACE(0, "UZIP::extract(%p)", _tmpdir)

   U_INTERNAL_ASSERT(valid)
   U_ASSERT(tmpdir.empty())
   U_ASSERT_EQUALS(content.empty(),false)

   if (_tmpdir) tmpdir = *_tmpdir;
   else
      {
      static uint32_t index;

      tmpdir.snprintf("/tmp/UZIP_TMP_%P_%u", index++);
      }

   const char* dir = tmpdir.c_str();

   if (UFile::mkdirs(dir) &&
       UFile::chdir(dir, true))
      {
      file = U_NEW(UFile);

      file->setPath(U_STRING_FROM_CONSTANT("tmp.zip"));

      if (file->creat() &&
          file->write(content))
         {
         file->close();

         npart = zip_extract("tmp.zip", 0, &filenames, &filenames_len);
         }

      (void) UFile::chdir(0, true);
      }

   if (npart > 0)
      {
      assignFilenames();

      U_RETURN(true);
      }

   clear();

   U_RETURN(false);
}

bool UZIP::readContent()
{
   U_TRACE(0, "UZIP::readContent()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(valid)
   U_INTERNAL_ASSERT_EQUALS(zippartname,0)
   U_ASSERT_EQUALS(content.empty(),false)

   npart = zip_get_content(U_STRING_TO_PARAM(content), &filenames, &filenames_len, &filecontents, &filecontents_len);

   if (npart > 0)
      {
      assignFilenames();

      zippartcontent = U_NEW(UVector<UString>(npart));

      for (uint32_t i = 0; i < npart; ++i)
         {
         if (filecontents_len[i]) zippartcontent->push_back(UString(filecontents[i], filecontents_len[i]));
         else                     zippartcontent->push_back(UString::getStringNull());
         }

      U_RETURN(true);
      }

   U_RETURN(false);
}

UString UZIP::getFileContentAt(int index)
{
   U_TRACE(0, "UZIP::getFileContentAt(%d)", index)

   U_CHECK_MEMORY

   UString dati;

   if (zippartcontent) dati = zippartcontent->at(index);
   else
      {
      U_INTERNAL_ASSERT_POINTER(file)

      UString filename = zippartname->at(index), buffer(U_CAPACITY);

      buffer.snprintf("%.*s/%.*s", U_STRING_TO_TRACE(tmpdir), U_STRING_TO_TRACE(filename));

      dati = (file->setPath(buffer), file->getContent());
      }

   U_RETURN_STRING(dati);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UZIP::dump(bool reset) const
{
   *UObjectIO::os << "npart                            " << npart                      << '\n'
                  << "valid                            " << valid                      << '\n'
                  << "filenames                        " << (void*)filenames           << '\n'
                  << "filecontents                     " << (void*)filecontents        << '\n'
                  << "filenames_len                    " << (void*)filenames_len       << '\n'
                  << "filecontents_len                 " << (void*)filecontents_len    << '\n'
                  << "file           (UFile            " << (void*)file                << ")\n"
                  << "tmpdir         (UString          " << (void*)&tmpdir             << ")\n"
                  << "content        (UString          " << (void*)&content            << ")\n"
                  << "zippartname    (UVector<UString> " << (void*)zippartname         << ")\n"
                  << "zippartcontent (UVector<UString> " << (void*)zippartcontent      << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
