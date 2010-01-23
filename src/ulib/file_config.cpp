// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    file_config.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/command.h>
#include <ulib/file_config.h>
#include <ulib/utility/escape.h>
#include <ulib/utility/services.h>
#include <ulib/container/vector.h>
#include <ulib/utility/string_ext.h>

#ifdef HAVE_STRSTREAM_H
#  include <strstream.h>
#else
#  include <ulib/replace/strstream.h>
#endif

UString* UFileConfig::str_yes;
UString* UFileConfig::str_file;
UString* UFileConfig::str_string;

void UFileConfig::str_allocate()
{
   U_TRACE(0, "UFileConfig::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_yes,0)
   U_INTERNAL_ASSERT_EQUALS(str_file,0)
   U_INTERNAL_ASSERT_EQUALS(str_string,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("yes") },
      { U_STRINGREP_FROM_CONSTANT("FILE") },
      { U_STRINGREP_FROM_CONSTANT("STRING") }
   };

   U_NEW_ULIB_OBJECT(str_yes,    U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_file,   U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_string, U_STRING_FROM_STRINGREP_STORAGE(2));
}

bool UFileConfig::open()
{
   U_TRACE(0, "UFileConfig::open()")

   U_CHECK_MEMORY

   bool result =  UFile::open()      &&
                 (UFile::size() > 0) &&
                  UFile::memmap(PROT_READ, &data);

   if (result)
      {
      // manage if we need preprocessing...

      if (U_STRING_FIND(data, 0, "\n#include ") != U_NOT_FOUND)
         {
         (void) UFile::lseek(U_SEEK_BEGIN, SEEK_SET);

         UString command(200U), dir = UStringExt::dirname(pathname);

         command.snprintf("cpp -undef -nostdinc -w -P -C -I%.*s -", U_STRING_TO_TRACE(dir));

         data = UCommand::outputCommand(command, 0, UFile::getFd(), UServices::getDevNull());
         }

      if (data.empty()) result = false;
      else
         {
         _end   = data.end();
         _start = data.data();
         _size  = data.size();

         result = load(0, 0);
         }
      }

   if (result)
      {
      if (UFile::isOpen()) UFile::close();

      U_RETURN(true);
      }

   U_ERROR("configuration file %S not valid...", UFile::getPath().data());

   U_RETURN(false);
}

// Perform search of first caracter '{' and check section name before...

bool UFileConfig::searchForObjectStream(const char* section, uint32_t len)
{
   U_TRACE(0, "UFileConfig::searchForObjectStream(%.*S,%u)", len, section, len)

   U_INTERNAL_DUMP("_size = %u _start = %.*S", _size, 10, _start)

   if (len == 0) section = "{";

   const char* save_start = _start;

loop:
   U_SKIP(_start,_end,loop,done)

   if (_start[0] == '#') U_SKIP_LINE_COMMENT(_start,_end,loop)

   U_INTERNAL_ASSERT_EQUALS(u_isspace(_start[0]),false)

   U_INTERNAL_DUMP("_start = %.*S", 10, _start)

   if (_start[0] != section[0]                                      ||
       (len && U_SYSCALL(memcmp, "%S,%S,%u", _start, section, len)) ||
       (u_isspace(_start[(len ? len : 1)]) == false)) // check for partial match of the name section...
      {
      while (u_isspace(*_start) == false) ++_start;

      goto loop;
      }

   _start += (len ? len : 1);

   U_INTERNAL_ASSERT_EQUALS(u_isspace(_start[0]),true)

   if (len)
      {
      // check the caracter after the name of the section...

      while (u_isspace(*_start)) ++_start;

      if (*_start != '{') goto loop;

      // find the end of the section and consider that as EOF... (call reset() when done with this section)

      _end = u_strpend(_start, data.remain(_start), U_CONSTANT_TO_PARAM("{}"), '#');

      U_INTERNAL_DUMP("_end = %p", _end)

      if (_end == 0) goto done;

      _size = (_end - ++_start); // NB: we advance one char (to call skip() after...)

      U_INTERNAL_DUMP("_size = %u", _size)
      }

   // FOUND

   U_RETURN(true);

done:
   _start = save_start;

   U_INTERNAL_DUMP("_size = %u _start = %.*S", _size, 10, _start)

   U_RETURN(false);
}

// skip space and line comment...

bool UFileConfig::skip()
{
   U_TRACE(0, "UFileConfig::skip()")

   U_INTERNAL_DUMP("_size = %u", _size)

loop:
   U_SKIP(_start,_end,loop,done)

   if (_start[0] == '#') U_SKIP_LINE_COMMENT(_start,_end,loop)

   U_INTERNAL_ASSERT_EQUALS(u_isspace(_start[0]),false)

   U_INTERNAL_DUMP("_start = %.*S", 10, _start)

   U_RETURN(true);

done:
   U_RETURN(false);
}

bool UFileConfig::loadTable(UHashMap<UString>& tbl)
{
   U_TRACE(0, "UFileConfig::loadTable(%p)", &tbl)

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("_size = %u _start = %.*S", _size, 10, _start)

   if (_size)
      {
      istrstream is(_start, _size);

      if (tbl.capacity() == 0) tbl.allocate();

      if (is >> tbl)
         {
         uint32_t pos = is.rdbuf()->pubseekoff(0, ios::cur, ios::in);

         U_INTERNAL_DUMP("pos = %u", pos)

         _start += pos;
         _size  -= pos;

         U_RETURN(true);
         }
      }

   U_RETURN(false);
}

bool UFileConfig::loadVector(UVector<UString>& vec)
{
   U_TRACE(0, "UFileConfig::loadVector(%p)", &vec)

   U_CHECK_MEMORY

   if (_size && skip() &&
       (_start[0] == '[' ||
        _start[0] == '('))
      {
      UVector<UString> vtmp;
      istrstream is(_start, _size);

      if (is >> vtmp)
         {
         uint32_t pos = is.rdbuf()->pubseekoff(0, ios::cur, ios::in);

         U_INTERNAL_DUMP("pos = %u", pos)

         _start += pos;
         _size  -= pos;

         UFile file;
         UString type, value, str(U_CAPACITY);

         // gcc: cannot optimize loop, the loop counter may overflow ???

         for (uint32_t i = 0, n = vtmp.size(); i < n; ++i)
            {
            type = vtmp[i];

            if (type == *str_file)
               {
               value = vtmp[++i];

               if (file.open(value)) str = file.getContent();
               else
                  {
                  U_WARNING("error on open file '%.*s' specified in configuration", U_STRING_TO_TRACE(value));
                  }
               }
            else if (type == *str_string)
               {
               value = vtmp[++i];

               str.setBuffer(value.size() * 4);

               (void) UEscape::decode(value, str);
               }
            else
               {
               str = type;
               }

            vec.push_back(str);

            str.clear();
            }

         U_RETURN(true);
         }
      }

   U_RETURN(false);
}

bool UFileConfig::load(const char* section, uint32_t len)
{
   U_TRACE(0, "UFileConfig::load(%.*S,%u)", len, section, len)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_MAJOR(_size,0)

   bool result = searchForObjectStream(section, len) && (table.clear(), loadTable(table));

   U_RETURN(result);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UFileConfig::dump(bool reset) const
{
   UFile::dump(false);

   *UObjectIO::os << '\n'
                  << "_end                      " << (void*)_end   << '\n'
                  << "_size                     " << _size         << '\n'
                  << "_start                    " << (void*)_start << '\n'
                  << "data  (UString            " << (void*)&data  << ")\n"
                  << "table (UHashMap<UString>  " << (void*)&table << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
