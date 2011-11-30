// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    file_config.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_FILECONFIG_H
#define ULIB_FILECONFIG_H 1

#include <ulib/file.h>
#include <ulib/container/hash_map.h>

class U_EXPORT UFileConfig : public UFile {
public:

   static const UString* str_FILE;
   static const UString* str_string;

   static void str_allocate();

   // COSTRUTTORI

    UFileConfig();
   ~UFileConfig()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UFileConfig)

      destroy();
      }

   // SERVICES

   bool loadTable(UHashMap<UString>& table);
   bool loadVector(UVector<UString>& vec, const char* name = 0);

   bool loadTable() { return loadTable(table); }

   bool load(const char* section = 0, uint32_t len = 0);

   void destroy()
      {
      U_TRACE(0, "UFileConfig::destroy()")

      if (table.capacity())
         {
         table.clear();
         table.deallocate();
         }

      data.clear();
      }

   // section management

   char peek() { return _start[0]; }

   void reset()
      {
      U_TRACE(0, "UFileConfig::reset()")

      _end  = data.end();
      _size = (_end - _start);
      }

   bool searchForObjectStream(const char* section = 0, uint32_t len = 0);

   // Wrapper for table

   UHashMap<UString> table;

   void clear()       {        table.clear(); }
   void deallocate()  {        table.deallocate(); }
   bool empty() const { return table.empty(); }

   UString      erase(const UString& key) { return table.erase(key); }
   UString operator[](const UString& key) { return table[key]; }

   // Facilities

   bool readBoolean(const UString& key);

   long readLong(const UString& key, long default_value = 0)
      {
      U_TRACE(0, "UFileConfig::readLong(%.*S,%ld)", U_STRING_TO_TRACE(key), default_value)

      UString value = table[key];

      if (value.empty() == false) default_value = value.strtol();

      U_RETURN(default_value);
      }

   // Open a configuration file

   bool open();
   bool open(const UString& path)
      {
      U_TRACE(0, "UFileConfig::open(%.*S)", U_STRING_TO_TRACE(path))

      UFile::setPath(path);

      return UFileConfig::open();
      }

   // EXT

   // This implementation of a Configuration reads properties
   // from a legacy Windows initialization (.ini) file.
   //
   // The file syntax is implemented as follows.
   //   - a line starting with a semicolon is treated as a comment and ignored
   //   - a line starting with a square bracket denotes a section key [<key>]
   //   - every other line denotes a property assignment in the form
   //     <value key> = <value>
   //
   // The name of a property is composed of the section key and the value key,
   // separated by a period (<section key>.<value key>).
   //
   // Property names are not case sensitive. Leading and trailing whitespace is
   // removed from both keys and values.

   bool loadINI();

   // This implementation of a Configuration reads properties
   // from a Java-style properties file.
   //
   // The file syntax is implemented as follows.
   //   - a line starting with a hash '#' or exclamation mark '!' is treated as a comment and ignored
   //   - every other line denotes a property assignment in the form
   //     <key> = <value> or
   //     <key> : <value>
   //
   // Property names are case sensitive. Leading and trailing whitespace is
   // removed from both keys and values. A property name can neither contain
   // a colon ':' nor an equal sign '=' character.

          bool loadProperties();
   static bool loadProperties(UHashMap<UString>& table, const char* start, const char* end);

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UString data;
   const char* _end;
   const char* _start;
   uint32_t _size;

private:
   UFileConfig(const UFileConfig&) : UFile()  {}
   UFileConfig& operator=(const UFileConfig&) { return *this; }
};

#endif
