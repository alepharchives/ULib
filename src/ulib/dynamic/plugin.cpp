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

static pvPF creator;

const char*     UPlugIn<void*>::plugin_dir = "/usr/libexec/ulib";
UPlugIn<void*>* UPlugIn<void*>::first;

#ifdef __MINGW32__
#  define U_EXT ".dll"
#else
#  define U_EXT ".so"
#endif

void UPlugIn<void*>::_create(const char* n, uint32_t len)
{
   U_TRACE(0, "UPlugIn<void*>::_create(%.*S,%u)", len, n, len)

   name     = strndup(n, len);
   name_len = len;

   char libname[U_PATH_MAX];
   (void) sprintf(libname, "%s/%.*s" U_EXT, U_PATH_CONV(plugin_dir), len, n);

   creator = (pvPF) (UDynamic::load(libname) ? operator[]("u_creat") : 0);

   if (creator)
      {
      obj = U_SYSCALL_NO_PARAM(creator);

      U_INTERNAL_DUMP("obj = %p", obj)
      }
}

void* UPlugIn<void*>::create(const char* name, uint32_t name_len)
{
   U_TRACE(0, "UPlugIn<void*>::create(%.*S,%u)", name_len, name, name_len)

   UPlugIn<void*>* item = U_NEW(UPlugIn<void*>);

   item->next = first;
   first      = item;

   item->_create(name, name_len);

   U_RETURN(item->obj);
}

UPlugIn<void*>* UPlugIn<void*>::getObjWrapper(void* obj)
{
   U_TRACE(0, "UPlugIn<void*>::getObjWrapper(%p)", obj)

   for (UPlugIn<void*>* item = first; item; item = item->next)
      {
      if (item->obj == obj) U_RETURN_POINTER(item,UPlugIn<void*>);
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
