// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    objectIO.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/string.h>
#include <ulib/internal/objectIO.h>

#ifdef HAVE_STRSTREAM_H
#  include <streambuf.h>
#else
#  include <streambuf>
#endif

#ifdef HAVE_OLD_IOSTREAM
#  define U_openmode ios::out
#else
#  define U_openmode std::ios_base::out
#endif

ostrstream* UObjectIO::os;
istrstream* UObjectIO::is;
uint32_t    UObjectIO::buffer_output_len;
char        UObjectIO::buffer_output[U_BUFSZ_OBJ_IO];

// gestione conversione oggetti in const char*...

void UObjectIO::init()
{
   U_INTERNAL_TRACE("UObjectIO::init()")

#ifdef HAVE_OLD_IOSTREAM
   os = new ostrstream(buffer_output, U_BUFSZ_OBJ_IO);
#else
   static char place[sizeof(ostrstream)];

   os = (ostrstream*) new(place) ostrstream(buffer_output, U_BUFSZ_OBJ_IO);
#endif
}

void UObjectIO::input(char* t, uint32_t tlen)
{
   U_INTERNAL_TRACE("UObjectIO::input(%s,%u)", t, tlen)

   if (is)
      {
#  ifdef HAVE_OLD_IOSTREAM
      delete is;
#  else
      is->~istrstream();
#  endif
      }

#ifdef HAVE_OLD_IOSTREAM
   is = new istrstream(t, tlen);
#else
   static char place[sizeof(istrstream)];

   is = new(place) istrstream(t, tlen);
#endif
}

void UObjectIO::output()
{
   U_INTERNAL_TRACE("UObjectIO::output()")

   U_INTERNAL_ASSERT_POINTER(os)

   buffer_output_len = os->pcount();

   U_INTERNAL_PRINT("os->pcount() = %d", buffer_output_len)

   U_INTERNAL_ASSERT_MINOR(buffer_output_len,U_BUFSZ_OBJ_IO)

   buffer_output[buffer_output_len] = '\0';

   U_INTERNAL_PRINT("buffer_output = %.*s", U_min(buffer_output_len,128), buffer_output)

#ifdef DEBUG_DEBUG
   off_t pos = os->rdbuf()->pubseekpos(0, U_openmode);
#else
        (void) os->rdbuf()->pubseekpos(0, U_openmode);
#endif

   U_INTERNAL_PRINT("pos = %ld, os->pcount() = %d", pos, os->pcount())
}

UStringRep* UObjectIO::create()
{
   U_TRACE(0, "UObjectIO::create()")

   UObjectIO::output();

   UStringRep* rep = U_NEW(UStringRep(buffer_output, buffer_output_len));

   U_INTERNAL_PRINT("rep = %.*S", U_STRING_TO_TRACE(*rep))

   U_RETURN_POINTER(rep, UStringRep);
}
