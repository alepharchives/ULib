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
#include <ulib/net/server/server.h>
#include <ulib/utility/string_ext.h>

#ifdef HAVE_STRSTREAM_H
#  include <strstream.h>
#else
#  include <ulib/replace/strstream.h>
#endif

const UString* UFileConfig::str_FILE;
const UString* UFileConfig::str_string;

void UFileConfig::str_allocate()
{
   U_TRACE(0, "UFileConfig::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_FILE,0)
   U_INTERNAL_ASSERT_EQUALS(str_string,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("FILE") },
      { U_STRINGREP_FROM_CONSTANT("STRING") }
   };

   U_NEW_ULIB_OBJECT(str_FILE,   U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_string, U_STRING_FROM_STRINGREP_STORAGE(1));
}

UFileConfig::UFileConfig()
{
   U_TRACE_REGISTER_OBJECT(0, UFileConfig, "")

   _size  = 0;
   _start = _end = data.data();

   UFile::map      = (char*)MAP_FAILED;
   UFile::st_size  = 0;
   UFile::map_size = 0;

   if (str_FILE == 0) str_allocate();
}

bool UFileConfig::processData()
{
   U_TRACE(0, "UFileConfig::processData()")

   U_CHECK_MEMORY

   bool result = false;

   // manage if we need preprocessing...

#if defined(HAVE_CPP) || defined(HAVE_MCPP)
   if (u_dosmatch(U_STRING_TO_PARAM(data), U_CONSTANT_TO_PARAM("*#*include *\n*"), 0))
      {
      static int fd_stderr = UServices::getDevNull("/tmp/cpp.err");

      UString command(200U), _dir = UStringExt::dirname(pathname);

#  ifdef HAVE_MCPP
      command.snprintf("mcpp                    -P -C -I%.*s -", U_STRING_TO_TRACE(_dir));
#  else
      command.snprintf("cpp -undef -nostdinc -w -P -C -I%.*s -", U_STRING_TO_TRACE(_dir));
#  endif

      if (UFile::isOpen())
         {
         (void) UFile::lseek(U_SEEK_BEGIN, SEEK_SET);

         data = UCommand::outputCommand(command, environ, UFile::getFd(), fd_stderr);
         }
      else
         {
         UCommand cmd(command);
         UString output(U_CAPACITY);

         bool esito = cmd.execute(&data, &output, -1, fd_stderr);

         UServer_Base::logCommandMsgError(cmd.getCommand());
         
         if (esito == false) U_RETURN(false);

         data = output;
         }
      }
#endif

   if (data.empty()) U_RETURN(false);

   _end   = data.end();
   _start = data.data();
   _size  = data.size();

   if (table.capacity() == 0) table.allocate();

   if (UFile::isPath())
      {
      // Loads configuration information from the file.
      // The file type is determined by the file extension.
      // The following extensions are supported:
      // -------------------------------------------------------------
      // .properties - properties file (JAVA Properties)
      // .ini        - initialization file (Windows INI)

      const char* suffix = UFile::getSuffix();

      U_INTERNAL_ASSERT_EQUALS(suffix[0], '.')

      ++suffix;

           if (U_STRNEQ(suffix, "ini"))        { result = loadINI();        goto end; }
      else if (U_STRNEQ(suffix, "properties")) { result = loadProperties(); goto end; }
      }

   result = load(0, 0);

end:
   U_RETURN(result);
}

bool UFileConfig::open()
{
   U_TRACE(0, "UFileConfig::open()")

   U_CHECK_MEMORY

   bool result = (UFile::open()                   &&
                  UFile::size() > 0               &&
                  UFile::memmap(PROT_READ, &data) &&
                  processData());

   if (UFile::isOpen()) UFile::close();

   if (result) U_RETURN(true);

   U_ERROR("configuration file %S not valid...", UFile::getPath().data());

   U_RETURN(false);
}

// Perform search of first caracter '{' and check section name before...

bool UFileConfig::searchForObjectStream(const char* section, uint32_t len)
{
   U_TRACE(0, "UFileConfig::searchForObjectStream(%.*S,%u)", len, section, len)

   U_INTERNAL_DUMP("_size = %u _start = %.*S", _size, 10, _start)

   if (len == 0) section = "{";

   bool bretry            = len && (_start != data.data());
   const char* save_start = _start;

retry:

   while (_start < _end)
      {
      _start = u_skip(_start, _end, 0, '#');

      if (_start == _end) break;

      U_INTERNAL_ASSERT_EQUALS(u__isspace(_start[0]), false)

      U_INTERNAL_DUMP("_start = %.*S", 10, _start)

      if (_start[0] != section[0]               ||
          (len && memcmp(_start, section, len)) ||
          (u__isspace(_start[(len ? len : 1)]) == false)) // check for partial match of the name section...
         {
         while (u__isspace(*_start) == false) ++_start;

         continue;
         }

      _start += (len ? len : 1);

      U_INTERNAL_ASSERT(u__isspace(_start[0]))

      if (len)
         {
         // check the caracter after the name of the section...

         while (u__isspace(*_start)) ++_start;

         if (*_start != '{') continue;

         // find the end of the section and consider that as EOF... (call reset() when done with this section)

         _end = u_strpend(_start, data.remain(_start), U_CONSTANT_TO_PARAM("{}"), '#');

         U_INTERNAL_DUMP("_end = %p", _end)

         if (_end == 0) break;

         _size = (_end - ++_start); // NB: we advance one char (to call u_skip() after...)

         U_INTERNAL_DUMP("_size = %u", _size)
         }

      // FOUND

      U_RETURN(true);
      }

   if (bretry)
      {
      bretry = false;
      _start = data.data();

      goto retry;
      }

   _start = save_start;

   U_INTERNAL_DUMP("_size = %u _start = %.*S", _size, 10, _start)

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

bool UFileConfig::loadVector(UVector<UString>& vec, const char* name)
{
   U_TRACE(0, "UFileConfig::loadVector(%p,%S)", &vec, name)

   U_CHECK_MEMORY

   _start = u_skip(_start, _end, 0, '#');

   if (_start == _end) U_RETURN(false);

   U_INTERNAL_ASSERT_EQUALS(u__isspace(_start[0]), false)

   U_INTERNAL_DUMP("_start = %.*S", 10, _start)

   uint32_t len = (name ? u__strlen(name) : 0);

   if (len)
      {
      if (_start[0] != name[0] ||
          memcmp(_start, name, len))
         {
         U_RETURN(false);
         }

      _start += len;

      U_INTERNAL_ASSERT(u__isspace(_start[0]))

      while (u__isspace(*_start)) ++_start;
      }

   if (_start[0] == '[' ||
       _start[0] == '(')
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

            if (type == *str_FILE)
               {
               value = vtmp[++i];

               if (file.open(value)) str = file.getContent();
               else
                  {
                  U_WARNING("error on open file %.*S specified in configuration", U_STRING_TO_TRACE(value));
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

bool UFileConfig::loadINI()
{
   U_TRACE(0, "UFileConfig::loadINI()")

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("_size = %u _start = %.*S", _size, 10, _start)

   uint32_t len;
   const char* ptr;
   UString sectionKey, fullKey(U_CAPACITY), key, value;

   while (_start < _end)
      {
      _start = u_skip(_start, _end, 0, ';');

      if (_start == _end) break;

      U_INTERNAL_ASSERT_EQUALS(u__isspace(_start[0]),false)

      U_INTERNAL_DUMP("_start = %.*S", 10, _start)

      if (_start[0] == '[') // a line starting with a square bracket denotes a section key [<key>]
         {
         ++_start;

         ptr = u_strpbrk(_start, _end - _start, "]\n");

         if (ptr == 0)
            {
            _start = _end;

            break;
            }

         sectionKey = UStringExt::trim(_start, ptr - _start);

         _start = ptr;
         }
      else // every other line denotes a property assignment in the form <value key> = <value>
         {
         ptr = u_strpbrk(_start, _end - _start, "=\n");

         if (ptr == 0)
            {
            _start = _end;

            break;
            }

         key = UStringExt::trim(_start, ptr - _start);

         _start = ptr;

         if (*_start == '=')
            {
            ++_start;

            ptr = (const char*) memchr(_start, '\n', _end - _start);

            if (ptr == 0)
               {
               _start = _end;

               break;
               }

            value = UStringExt::trim(_start, ptr - _start);

            _start = ptr;
            }

         // The name of a property is composed of the section key and the value key,
         // separated by a period (<section key>.<value key>).

         len = sectionKey.size();

         fullKey.setBuffer(len + 1 + key.size());

         if (len == 0) fullKey.snprintf(     "%.*s",                         U_STRING_TO_TRACE(key));
         else          fullKey.snprintf("%.*s.%.*s", len, sectionKey.data(), U_STRING_TO_TRACE(key));

         table.insert(fullKey, value);
         }

      if (_start >= _end) break;

      ++_start;
      }

   _size = (_end - _start);

   U_INTERNAL_DUMP("_size = %u", _size)

   bool result = (table.empty() == false);

   U_RETURN(result);
}

bool UFileConfig::loadProperties(UHashMap<UString>& table, const char* _start, const char* _end)
{
   U_TRACE(0, "UFileConfig::loadProperties(%p,%p,%p)", &table, _start, _end)

   U_INTERNAL_DUMP("_start = %.*S", 10, _start)

   char c;
   const char* ptr;
   UString key, value;

   while (_start < _end)
      {
      // skip white space

      if (u__isspace(*_start))
         {
         ++_start;

         continue;
         }

      // a line starting with a hash '#' or exclamation mark '!' is treated as a comment and ignored

      c = *_start;

      if (c == '#' ||
          c == '!')
         {
         // skip line comment

         _start = (const char*) memchr(_start, '\n', _end - _start);

         if (_start == 0) _start = _end;

         continue;
         }

      U_INTERNAL_ASSERT_EQUALS(u__isspace(_start[0]),false)

      U_INTERNAL_DUMP("_start = %.*S", 10, _start)

      // every other line denotes a property assignment in the form <key> = <value>

      ptr = u_strpbrk(_start, _end - _start, "=:\r\n");

      if (ptr == 0)
         {
         _start = _end;

         break;
         }

      key = UStringExt::trim(_start, ptr - _start);

      _start = ptr;

      c = *_start;

      if (c == '=' ||
          c == ':')
         {
         ++_start;

         ptr = (const char*) memchr(_start, '\n', _end - _start);

         if (ptr == 0)
            {
            _start = _end;

            break;
            }

         value = UStringExt::trim(_start, ptr - _start);

         // NB: var shell often need to be quoted...

         if (c == '=' && value.isQuoted()) value.unQuote();

         _start = ptr;
         }

      table.insert(key, value);

      if (_start >= _end) break;

      ++_start;
      }

   bool result = (table.empty() == false);

   U_RETURN(result);
}

bool UFileConfig::loadProperties()
{
   U_TRACE(0, "UFileConfig::loadProperties()")

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("_size = %u _start = %.*S", _size, 10, _start)

   bool result = loadProperties(table, _start, _end);

   if (result)
      {
      _size = (_end - _start);

      U_INTERNAL_DUMP("_size = %u", _size)

      U_RETURN(true);
      }

   U_RETURN(false);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UFileConfig::dump(bool _reset) const
{
   UFile::dump(false);

   *UObjectIO::os << '\n'
                  << "_end                      " << (void*)_end   << '\n'
                  << "_size                     " << _size         << '\n'
                  << "_start                    " << (void*)_start << '\n'
                  << "data  (UString            " << (void*)&data  << ")\n"
                  << "table (UHashMap<UString>  " << (void*)&table << ')';

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
