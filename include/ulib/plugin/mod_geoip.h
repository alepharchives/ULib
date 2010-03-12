// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_geoip.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_GEOIP_H
#define U_MOD_GEOIP_H 1

#include <ulib/string.h>
#include <ulib/net/server/server_plugin.h>

#include <GeoIP.h>
#include <GeoIPCity.h>

/*
The plugin interface is an integral part of UServer which provides a flexible way to add specific functionality to UServer.
Plugins allow you to enhance the functionality of UServer without changing the core of the server. They can be loaded at
startup time and can change virtually some aspect of the behaviour of the server.

UServer has 5 hooks which are used in different states of the execution of the request:
--------------------------------------------------------------------------------------------
* Server-wide hooks:
````````````````````
1) handlerConfig: called when the server finished to process its configuration
2) handlerInit:   called when the server finished its init, and before start to run

* Connection-wide hooks:
````````````````````````
3) handlerRead:
4) handlerRequest:
5) handlerReset:
  called in `UClientImage_Base::handlerRead()`
--------------------------------------------------------------------------------------------

RETURNS:
  U_PLUGIN_HANDLER_GO_ON    if ok
  U_PLUGIN_HANDLER_FINISHED if the final output is prepared
  U_PLUGIN_HANDLER_AGAIN    if the request is empty (NONBLOCKING)

  U_PLUGIN_HANDLER_ERROR    on error
*/

class U_EXPORT UGeoIPPlugIn : public UServerPlugIn {
public:

   static UString* str_COUNTRY_FORBIDDEN_MASK;

   static void str_allocate();

   // COSTRUTTORI

   UGeoIPPlugIn()
      {
      U_TRACE_REGISTER_OBJECT(0, UGeoIPPlugIn, "", 0)

      if (str_COUNTRY_FORBIDDEN_MASK == 0) str_allocate();
      }

   virtual ~UGeoIPPlugIn();

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerInit();

   // Connection-wide hooks

   virtual int handlerRead();
   virtual int handlerRequest();
   virtual int handlerReset()
      {
      U_TRACE(0, "UGeoIPPlugIn::handlerReset()")

      U_RETURN(U_PLUGIN_HANDLER_GO_ON);
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   uint32_t ipnum;
   const char* org;
   GeoIPRecord* gir;
   char* domain_name;
   GeoIPRegion* region;
   GeoIP* gi[NUM_DB_TYPES];
   const char* country_code;
   const char* country_name;
   int netspeed, country_id;
   UString country_forbidden_mask;

   static bool bGEOIP_CITY_EDITION_REV1;

   bool checkCountryForbidden();
   bool setCountryCode(const char* ipaddress);

private:
   UGeoIPPlugIn(const UGeoIPPlugIn&) : UServerPlugIn() {}
   UGeoIPPlugIn& operator=(const UGeoIPPlugIn&)        { return *this; }
};

#endif
