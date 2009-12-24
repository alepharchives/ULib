// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_usp.cpp - this is a plugin usp (ULib Servlet Page) for UServer
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file_config.h>
#include <ulib/plugin/mod_usp.h>
#include <ulib/container/vector.h>
#include <ulib/net/server/server.h>

U_CREAT_FUNC(UUspPlugIn)

void* UUspPlugIn::argument;
vPFpv UUspPlugIn::runDynamicPage;

void UUspPlugIn::callRunDynamicPage(UStringRep* key, void* value)
{
   U_TRACE(0, "UUspPlugIn::callRunDynamicPage(%.*S,%p)", U_STRING_TO_TRACE(*key), value)

   UDynamic* page = (UDynamic*)value;

   U_INTERNAL_ASSERT_POINTER(page)

   runDynamicPage = (vPFpv)(*page)["runDynamicPage"];

   // ------------------
   // argument value:
   // ------------------
   //  0 -> init
   // -1 -> reset
   // -2 -> destroy
   // ------------------

   runDynamicPage(argument);
}

UUspPlugIn::~UUspPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, UUspPlugIn)

   // call end for all modules...

   argument = (void*)-2;

   pages.callForAllEntry(callRunDynamicPage);

   pages.clear();
   pages.deallocate();
}

// Server-wide hooks

#ifdef __MINGW32__
#define U_LEN_SUFFIX 4 // .dll
#else
#define U_LEN_SUFFIX 3 // .so
#endif

int UUspPlugIn::handlerInit()
{
   U_TRACE(0, "UUspPlugIn::handlerInit()")

   if (UFile::chdir("usp", true))
      {
      UString name;
      UDynamic* page;
      const char* ptr;
      UVector<UString> vec;
      uint32_t n = UFile::listContentOf(vec);

      for (uint32_t i = 0; i < n; ++i)
         {
         name = vec[i];
         ptr  = name.c_str();
         page = U_NEW(UDynamic);

         if (page->load(ptr) == false) delete page;
         else
            {
            UString key(100U);

            key.snprintf("%.*s.usp", name.size() - U_LEN_SUFFIX, ptr);

            u_canonicalize_pathname(key.data());

            key.size_adjust();

            pages.insert(key, page);

            U_SRV_LOG_VAR("found: usp/%s, USP service registered (URI): /usp/%.*s", ptr+2, U_STRING_TO_TRACE(key));
            }
         }

      (void) UFile::chdir(0, true);

      // call init for all modules...

      argument = 0;

      pages.callForAllEntry(callRunDynamicPage);
      }

   if (pages.empty())
      {
      U_SRV_LOG_MSG("initialization of plugin FAILED");

      U_RETURN(U_PLUGIN_HANDLER_ERROR);
      }

   U_SRV_LOG_MSG("initialization of plugin success");

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int UUspPlugIn::handlerRequest()
{
   U_TRACE(0, "UUspPlugIn::handlerRequest()")

   U_INTERNAL_DUMP("method = %.*S uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_HTTP_URI_TO_TRACE)

   if (UHTTP::isUSPRequest() == false) U_RETURN(U_PLUGIN_HANDLER_GO_ON);

   // check if dynamic page (ULib Servlet Page)

   UDynamic* page = pages[UString(U_HTTP_URI_TO_PARAM_SHIFT(U_CONSTANT_SIZE("/usp/")))];

   if (page == 0)
      {
      U_SRV_LOG_VAR("USP request '%.*s' NOT available...", U_HTTP_URI_TO_TRACE);

      UHTTP::setHTTPServiceUnavailable(); // set Service Unavailable error response...

      U_RETURN(U_PLUGIN_HANDLER_FINISHED);
      }

   runDynamicPage = (vPFpv)(*page)["runDynamicPage"];

   // retrieve information on specific HTML form elements
   // (such as checkboxes, radio buttons, and text fields), or uploaded files

   uint32_t n = UHTTP::processHTTPForm();

   UClientImage_Base::wbuffer->setBuffer(U_CAPACITY);

   runDynamicPage(UClientImage_Base::pClientImage);

   if (n) UHTTP::resetForm();

   if (UHTTP::processCGIOutput() == false)
      {
      U_SRV_LOG_MSG("runDynamicPage(): call UHTTP::processCGIOutput() return false...");

      UHTTP::setHTTPInternalError(); // set internal error response...

      U_RETURN(U_PLUGIN_HANDLER_FINISHED);
      }

   // check for "Connection: close" in headers...

   int result = UHTTP::checkForHTTPConnectionClose();

   U_RETURN(result);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UUspPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "pages (UHashMap<UDynamic*> " << (void*)&pages  << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
