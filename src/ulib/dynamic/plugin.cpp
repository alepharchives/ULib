// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    plugin.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/dynamic/plugin.h>

const char*     UPlugIn<void*>::plugin_dir = "/usr/libexec/ulib";
UPlugIn<void*>* UPlugIn<void*>::first;

#ifdef __MINGW32__
#  define U_FMT_LIBPATH "%s/%.*s.dll"
#else
#  define U_FMT_LIBPATH "%s/%.*s.so"
#endif

void* UPlugIn<void*>::create(const char* _name, uint32_t _name_len)
{
   U_TRACE(0, "UPlugIn<void*>::create(%.*S,%u)", _name_len, _name, _name_len)

   UPlugIn<void*>* item = U_NEW(UPlugIn<void*>);

   item->next = first;
   first      = item;

   item->name     = strndup(_name, _name_len);
   item->name_len = _name_len;

   char buffer[U_PATH_MAX];

   (void) snprintf(buffer, U_PATH_MAX, U_FMT_LIBPATH, U_PATH_CONV(plugin_dir), _name_len, _name);

   if (item->UDynamic::load(buffer))
      {
      (void) sprintf(buffer, "u_creat_%.*s", _name_len, _name);

      pvPF creator = (pvPF) item->operator[](buffer);

      if (creator)
         {
         item->obj = creator();

         U_INTERNAL_DUMP("obj = %p", item->obj)
         }
      }

   U_RETURN(item->obj);
}

UPlugIn<void*>* UPlugIn<void*>::getObjWrapper(void* _obj)
{
   U_TRACE(0, "UPlugIn<void*>::getObjWrapper(%p)", _obj)

   for (UPlugIn<void*>* item = first; item; item = item->next)
      {
      if (item->obj == _obj) U_RETURN_POINTER(item,UPlugIn<void*>);
      }

   U_RETURN_POINTER(0,UPlugIn<void*>);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UPlugIn<void*>::dump(bool reset) const
{
   *UObjectIO::os << "obj      " << obj          << '\n'
                  << "first    " << (void*)first << '\n'
                  << "next     " << (void*)next  << '\n'
                  << "name     ";

   char buffer[32];

   UObjectIO::os->write(buffer, u_snprintf(buffer, sizeof(buffer), "%#.*S", name_len, name));

   *UObjectIO::os << '\n'
                  << "name_len " << name_len;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
