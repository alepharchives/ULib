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

   static UString* str_yes;
   static UString* str_file;
   static UString* str_string;

   static void str_allocate();

   // COSTRUTTORI

   UFileConfig()
      {
      U_TRACE_REGISTER_OBJECT(0, UFileConfig, "", 0)

      UFile::map      = (char*)MAP_FAILED;
      UFile::st_size  = 0;
      UFile::map_size = 0;

      if (str_yes == 0) str_allocate();
      }

   ~UFileConfig()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UFileConfig)

      if (table.capacity())
         {
         table.clear();
         table.deallocate();
         }
      }

   // SERVICES

   bool loadVector(UVector<UString>& vec);
   bool loadTable(UHashMap<UString>& table);

   bool loadTable() { return loadTable(table); }

   bool load(const char* section = 0, uint32_t len = 0);

   // section management

   bool skip();                        // skip space and line comment...
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

   long readLong(const UString& key, long default_value = 0)
      {
      U_TRACE(0, "UFileConfig::readLong(%.*S,%ld)", U_STRING_TO_TRACE(key), default_value)

      UString value = table[key];

      if (value.empty() == false) default_value = value.strtol();

      U_RETURN(default_value);
      }

   bool readBoolean(const UString& key)
      {
      U_TRACE(0, "UFileConfig::readBoolean(%.*S)", U_STRING_TO_TRACE(key))

      bool result = (table[key] == *str_yes);

      U_RETURN(result);
      }

   // Open a configuration file

   bool open();
   bool open(const UString& path)
      {
      U_TRACE(0, "UFileConfig::open(%.*S)", U_STRING_TO_TRACE(path))

      UFile::setPath(path);

      return UFileConfig::open();
      }

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
