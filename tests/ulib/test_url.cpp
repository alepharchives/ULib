// test_url.cpp

#include <ulib/url.h>
#include <ulib/file.h>

#include <iostream>

static void check(const UString& dati, const UString& file)
{
   U_TRACE(5,"check(%p,%p)", &dati, &file)

   UString buffer1(dati.size() * 4), buffer2(dati.size());

   Url::encode(dati,    buffer1);
   Url::decode(buffer1, buffer2);

   U_INTERNAL_DUMP("buffer1 = %#.*S", U_STRING_TO_TRACE(buffer1))
   U_INTERNAL_DUMP("dati    = %#.*S", U_STRING_TO_TRACE(dati))
   U_INTERNAL_DUMP("buffer2 = %#.*S", U_STRING_TO_TRACE(buffer2))

// (void) UFile::writeToTmpl("url.encode", buffer1);
// (void) UFile::writeToTmpl("url.decode", buffer2);

   U_ASSERT( dati == buffer2 )
}

int U_EXPORT main(int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)", argc)

   UString filename;

   while (cin >> filename)
      {
      UString dati = UFile::contentOf(filename);

      check(dati, filename);
      }

    Url url[] = {
       Url(U_STRING_FROM_CONSTANT("http://www.cs.wustl.edu/")),
       Url(U_STRING_FROM_CONSTANT("http://www.cs.wustl.edu/index.html")),
       Url(U_STRING_FROM_CONSTANT("http://www.cs.wustl.edu/form?var=foo")),
       Url(U_STRING_FROM_CONSTANT("http://www.notexist.com:8080/index.html")),
       Url(U_STRING_FROM_CONSTANT("http://www.notexist.com:80/index.html")),
       Url(U_STRING_FROM_CONSTANT("ftp://foo")),
       Url(U_STRING_FROM_CONSTANT("http://www/?kkk//")),
       Url(U_STRING_FROM_CONSTANT("ftp://www.cs.wustl.edu/")),
       Url(U_STRING_FROM_CONSTANT("ftp://user@www.cs.wustl.edu/")),
       Url(U_STRING_FROM_CONSTANT("ftp://user:pass@www.cs.wustl.edu/")),
       Url(U_STRING_FROM_CONSTANT("ftp://user:pass@www.cs.wustl.edu/path")),
       Url(U_STRING_FROM_CONSTANT("ftp://www.cs.wustl.edu")),
       Url(U_STRING_FROM_CONSTANT("http://www.cs.wustl.edu/index.html")),
       Url(U_STRING_FROM_CONSTANT("mailto:ace-users@cs.wustl.edu")),
       Url(U_STRING_FROM_CONSTANT("mailto:majordomo@cs.wustl.edu?Subject: subscribe ace-users")),
       Url(U_STRING_FROM_CONSTANT("mailto:nobody")),
       Url(U_STRING_FROM_CONSTANT("http://www.cs.wustl.edu")),
       Url(U_STRING_FROM_CONSTANT("file:/etc/passwd")),
       Url(U_STRING_FROM_CONSTANT("http://www.cs.wustl.edu/form?var=foo&url=http%3a//www/%3fkkk//")) };

   int i, nurl = sizeof(url)/sizeof(url[0]);

   for (i = 0; i < nurl; ++i)
      {
      cout  << '"' << url[i]              << "\" "
            << '"' << url[i].getService() << "\" "
            << '"' << url[i].getUser()    << "\" "
            << '"' << url[i].getHost()    << "\" "
            << '"' << url[i].getPort()    << "\" " 
            << '"' << url[i].getPath()    << "\" "
            << '"' << url[i].getQuery()   << "\"\n";
      }

   for (i = 0; i < nurl; ++i)
      {
      url[i].eraseUser();
      url[i].eraseQuery();
      }

   Url u = url[2];

   u.setService(U_CONSTANT_TO_PARAM("http"));
   u.setUser(U_CONSTANT_TO_PARAM("pippo"));
   u.setHost(U_CONSTANT_TO_PARAM("www.unirel.com"));
   u.setPort(8080);
   u.setPath(U_CONSTANT_TO_PARAM("/usr/src"));
   u.setQuery(U_CONSTANT_TO_PARAM("var1=foo&var2=ba"));

   cout  << '"' << u              << "\" "
         << '"' << u.getService() << "\" "
         << '"' << u.getUser()    << "\" "
         << '"' << u.getHost()    << "\" "
         << '"' << u.getPort()    << "\" " 
         << '"' << u.getPath()    << "\" "
         << '"' << u.getQuery()   << "\"\n";

   U_ASSERT( u.isQuery() ) 

   /*
   U_ASSERT( u.getService() == UString( u.getService(buffer, sizeof(buffer)) ) )
   U_ASSERT( u.getUser()    == UString( u.getUser(buffer, sizeof(buffer)) ) )
   U_ASSERT( u.getHost()    == UString( u.getHost(buffer, sizeof(buffer)) ) )
   U_ASSERT( u.getPort()    == 8080 )
   U_ASSERT( u.getPath()    == UString( u.getPath(buffer, sizeof(buffer)) ) )
   U_ASSERT( u.getQuery()   == UString( u.getQuery(buffer, sizeof(buffer)) ) )
   */

   UString entry(1000), value(1000);

   /*
   U_ASSERT( u.firstQuery(entry, value) == u.firstQuery(buffer1, sizeof(buffer1), buffer2, sizeof(buffer2)) )
   U_ASSERT( entry == UString(buffer1) )
   U_ASSERT( value == UString(buffer2) )
   U_ASSERT( entry == UString(U_CONSTANT_TO_PARAM("var1")) )
   U_ASSERT( value == UString(U_CONSTANT_TO_PARAM("foo")) )
   */

   u.firstQuery(entry, value);
   u.nextQuery(entry, value);

   /*
   u.firstQuery(buffer1, sizeof(buffer1), buffer2, sizeof(buffer2));
   u.nextQuery(buffer1, sizeof(buffer1), buffer2, sizeof(buffer2));
   U_ASSERT( entry == UString(buffer1) )
   U_ASSERT( value == UString(buffer2) )
   */
   U_ASSERT( entry == U_STRING_FROM_CONSTANT("var2") )
   U_ASSERT( value == U_STRING_FROM_CONSTANT("ba") )

   u.addQuery(U_CONSTANT_TO_PARAM("var3"), U_CONSTANT_TO_PARAM("co"));

   u.firstQuery(entry, value);
   u.nextQuery(entry, value);
   u.nextQuery(entry, value);

   /*
   U_ASSERT( u.firstQuery(entry, value) == u.firstQuery(buffer1, sizeof(buffer1), buffer2, sizeof(buffer2)) )
   u.nextQuery(entry, value);
   u.nextQuery(entry, value);
   u.firstQuery(buffer1, sizeof(buffer1), buffer2, sizeof(buffer2));
   u.nextQuery(buffer1, sizeof(buffer1), buffer2, sizeof(buffer2));
   u.nextQuery(buffer1, sizeof(buffer1), buffer2, sizeof(buffer2));

   U_ASSERT( entry == UString(buffer1) )
   U_ASSERT( value == UString(buffer2) )
   */
   U_ASSERT( entry == U_STRING_FROM_CONSTANT("var3") )
   U_ASSERT( value == U_STRING_FROM_CONSTANT("co") )

   entry = U_STRING_FROM_CONSTANT("var2");

   U_ASSERT( u.findQuery(entry, value) == true )
   U_ASSERT( entry == U_STRING_FROM_CONSTANT("var2") )
   U_ASSERT( value == U_STRING_FROM_CONSTANT("ba") )

   entry.clear();
   value = U_STRING_FROM_CONSTANT("foo");

   U_ASSERT( u.findQuery(entry, value) == true )
   U_ASSERT( entry == U_STRING_FROM_CONSTANT("var1") )
   U_ASSERT( value == U_STRING_FROM_CONSTANT("foo") )
}
