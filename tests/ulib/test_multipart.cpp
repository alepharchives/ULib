// test_multipart.cpp

#include <ulib/file.h>
#include <ulib/mime/multipart.h>

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   UMimeMultipartMsg msg;

   UString filename;

   while (cin >> filename)
      {
      UString dati = UFile::contentOf(filename);

      msg.add(UMimeMultipartMsg::section(dati, "", UMimeMultipartMsg::AUTO, "iso-8859-1", filename.c_str()));
      }

   msg.add(UMimeMultipartMsg::section(U_STRING_FROM_CONSTANT("File di testo\n"),
                                      "text/plain",
                                      UMimeMultipartMsg::BIT7,
                                      "iso-8859-1",
                                      "",
                                      "Content-Disposition: inline"));

   cout << msg;

// exit(0);
}
