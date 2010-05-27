// utility.cpp

#include "utility.h"

void UXAdESUtility::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(5, "UXAdESUtility::handlerConfig(%p)", &cfg)

   MSname = cfg[U_STRING_FROM_CONSTANT(    "MS-WORD.SignatureContent")];
   OOname = cfg[U_STRING_FROM_CONSTANT("OPEN-OFFICE.SignatureContent")];

   (void)   MSToBeSigned.split(cfg[U_STRING_FROM_CONSTANT(    "MS-WORD.ToBeSigned")]);
   (void) MSZipStructure.split(cfg[U_STRING_FROM_CONSTANT(    "MS-WORD.ZipStructure")]);
   (void)   OOToBeSigned.split(cfg[U_STRING_FROM_CONSTANT("OPEN-OFFICE.ToBeSigned")]);
   (void) OOZipStructure.split(cfg[U_STRING_FROM_CONSTANT("OPEN-OFFICE.ZipStructure")]);
}

// ---------------------------------------------------------------------------------------------------------------
// check for OOffice or MS-Word document...
// ---------------------------------------------------------------------------------------------------------------

bool UXAdESUtility::checkDocument(const UString& document, const char* pathname)
{
   U_TRACE(5, "UXAdESUtility::checkDocument(%.*S,%S)", U_STRING_TO_TRACE(document), pathname)

   msword = ooffice = false;

   (void) tmpdir.reserve(100U);

   tmpdir.snprintf("%s/%s", u_tmpdir, u_basename(pathname));

   if (zip.extract(document, &tmpdir))
      {
      UString namefile;
      uint32_t i, n, index;

      for (i = 0, n = zip.getFilesCount(); i < n; ++i)
         {
         namefile = zip.getFilenameAt(i);

         ZipStructure.push(namefile);
           ZipContent.push(zip.getFileContentAt(i));

         U_INTERNAL_DUMP("Part %d: Filename=%.*S", i+1, U_STRING_TO_TRACE(namefile));
         }

      U_INTERNAL_DUMP("ZIP: %d parts", n)

      msword = true;

      for (i = 0, n = MSZipStructure.size(); i < n; ++i)
         {
         namefile = MSZipStructure[i];

         if (ZipStructure.isContained(namefile)) continue;

         msword  = false;
         ooffice = true;

         for (i = 0, n = OOZipStructure.size(); i < n; ++i)
            {
            namefile = OOZipStructure[i];

            if (ZipStructure.isContained(namefile)) continue;

            ooffice = false;

            break;
            }

         break;
         }

      if (msword)
         {
         for (i = 0, n = MSToBeSigned.size(); i < n; ++i)
            {
            namefile = MSToBeSigned[i];
            index    = ZipStructure.contains(namefile);

                 vuri.push(namefile);
            vdocument.push(ZipContent[index]);
            }

         (void) docout.reserve(100U);

         docout.snprintf("%.*s/%.*s", U_STRING_TO_TRACE(tmpdir), U_STRING_TO_TRACE(MSname));

         U_RETURN(true);
         }

      if (ooffice)
         {
         for (i = 0, n = OOToBeSigned.size(); i < n; ++i)
            {
            namefile = OOToBeSigned[i];
            index    = ZipStructure.contains(namefile);

                 vuri.push(namefile);
            vdocument.push(ZipContent[index]);
            }

         (void) docout.reserve(100U);

         docout.snprintf("%.*s/%.*s", U_STRING_TO_TRACE(tmpdir), U_STRING_TO_TRACE(OOname));

         U_RETURN(true);
         }
      }

   vdocument.push(document);
        vuri.push(UString(pathname));

   U_RETURN(false);
}

UString UXAdESUtility::getSigned()
{
   U_TRACE(5, "UXAdESUtility::getSigned()")

   UString firma, name = (msword ? MSname : OOname);

   uint32_t index = ZipStructure.contains(name);

   if (index != U_NOT_FOUND) firma = ZipContent[index];

   U_RETURN_STRING(firma);
}

UString UXAdESUtility::outputDocument(const UString& firma)
{
   U_TRACE(5, "UXAdESUtility::outputDocument(%.*S)", U_STRING_TO_TRACE(firma))

   if (msword ||
       ooffice)
      {
      const char* add_to_filenames[] = {
               getSigned().empty() ? msword
                                   ? MSname.c_str() : OOname.c_str()
                                                    : 0, 0 };

      (void) UFile::writeTo(docout, firma);

      UString output = zip.archive(add_to_filenames);

      U_RETURN_STRING(output);
      }

   U_RETURN_STRING(firma);
}
