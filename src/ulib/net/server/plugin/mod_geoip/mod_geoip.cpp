// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_geoip.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file_config.h>
#include <ulib/utility/uhttp.h>
#include <ulib/net/server/server.h>
#include <ulib/net/server/plugin/mod_geoip.h>

extern "C" {
   extern void          _GeoIP_setup_dbfilename();
   extern unsigned long _GeoIP_lookupaddress(const char* host);
}

U_CREAT_FUNC(mod_geoip, UGeoIPPlugIn)

int          UGeoIPPlugIn::netspeed;
int          UGeoIPPlugIn::country_id;
bool         UGeoIPPlugIn::bGEOIP_CITY_EDITION_REV1;
char*        UGeoIPPlugIn::domain_name;
GeoIP*       UGeoIPPlugIn::gi[NUM_DB_TYPES];
uint32_t     UGeoIPPlugIn::ipnum;
UString*     UGeoIPPlugIn::country_forbidden_mask;
const char*  UGeoIPPlugIn::org;
const char*  UGeoIPPlugIn::country_code;
const char*  UGeoIPPlugIn::country_name;
GeoIPRecord* UGeoIPPlugIn::gir;
GeoIPRegion* UGeoIPPlugIn::region;

const UString* UGeoIPPlugIn::str_COUNTRY_FORBIDDEN_MASK;

void UGeoIPPlugIn::str_allocate()
{
   U_TRACE(0, "UGeoIPPlugIn::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_COUNTRY_FORBIDDEN_MASK,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("COUNTRY_FORBIDDEN_MASK") }
   };

   U_NEW_ULIB_OBJECT(str_COUNTRY_FORBIDDEN_MASK, U_STRING_FROM_STRINGREP_STORAGE(0));
}

bool UGeoIPPlugIn::setCountryCode()
{
   U_TRACE(1, "UGeoIPPlugIn::setCountryCode()")

   gir = 0;
   region = 0;
   domain_name = 0;
   netspeed = country_id = 0;
   bGEOIP_CITY_EDITION_REV1 = false;
   country_code = country_name = org = 0;

   ipnum = U_SYSCALL(_GeoIP_lookupaddress, "%s", UServer_Base::getClientAddress());

   if (ipnum)
      {
      // iterate through different database types

      for (uint32_t i = 0; i < NUM_DB_TYPES; ++i)
         {
         if (gi[i])
            {
            if (GEOIP_DOMAIN_EDITION == i)
               {
               domain_name = U_SYSCALL(GeoIP_name_by_ipnum, "%p,%lu", gi[i], ipnum);
               }
            else if (GEOIP_COUNTRY_EDITION == i)
               {
               country_id = U_SYSCALL(GeoIP_id_by_ipnum, "%p,%lu", gi[i], ipnum);

               if (country_id)
                  {
                  country_code = GeoIP_country_code[country_id];
                  country_name = GeoIP_country_name[country_id];

                  U_SRV_LOG_WITH_ADDR("%s: IP %S is from %s, %s for", GeoIPDBDescription[i],
                                          UServer_Base::getClientAddress(), country_code, country_name);
                  }
               }
            else if (GEOIP_REGION_EDITION_REV0 == i || GEOIP_REGION_EDITION_REV1 == i)
               {
               region = (GeoIPRegion*) U_SYSCALL(GeoIP_region_by_ipnum, "%p,%lu", gi[i], ipnum);

               if (region)
                  {
                  U_SRV_LOG_WITH_ADDR("%s: IP %S is from %s, %s for", GeoIPDBDescription[i],
                                          UServer_Base::getClientAddress(), region->country_code, region->region);

                  U_SYSCALL_VOID(GeoIPRegion_delete, "%p", region);
                  }
               }
            else if (GEOIP_CITY_EDITION_REV0 == i || GEOIP_CITY_EDITION_REV1 == i)
               {
               gir = (GeoIPRecord*) U_SYSCALL(GeoIP_record_by_ipnum, "%p,%lu", gi[i], ipnum);

               bGEOIP_CITY_EDITION_REV1 = (GEOIP_CITY_EDITION_REV1 == i);
               }
            else if (GEOIP_ORG_EDITION == i || GEOIP_ISP_EDITION == i)
               {
               org = U_SYSCALL(GeoIP_org_by_ipnum, "%p,%lu", gi[i], ipnum);
               }
            else if (GEOIP_NETSPEED_EDITION == i)
               {
               netspeed = U_SYSCALL(GeoIP_id_by_ipnum, "%p,%lu", gi[i], ipnum);

               /*
                    if (netspeed == GEOIP_UNKNOWN_SPEED)   {}
               else if (netspeed == GEOIP_DIALUP_SPEED)    {}
               else if (netspeed == GEOIP_CABLEDSL_SPEED)  {}
               else if (netspeed == GEOIP_CORPORATE_SPEED) {}
               */
               }
            }
         }
      }

   if (country_code) U_RETURN(true);

   U_RETURN(false);
}

bool UGeoIPPlugIn::checkCountryForbidden()
{
   U_TRACE(0, "UGeoIPPlugIn::checkCountryForbidden()")

   if (u_dosmatch_with_OR(country_code, 2, U_STRING_TO_PARAM(*country_forbidden_mask), 0))
      {
      U_SRV_LOG("COUNTRY_FORBIDDEN: request from %s denied by access list", country_name);

      UHTTP::setHTTPForbidden();

      U_RETURN(false);
      }

   U_RETURN(true);
}

UGeoIPPlugIn::UGeoIPPlugIn()
{
   U_TRACE_REGISTER_OBJECT(0, UGeoIPPlugIn, "")

   country_forbidden_mask = U_NEW(UString);

   if (str_COUNTRY_FORBIDDEN_MASK == 0) str_allocate();
}

UGeoIPPlugIn::~UGeoIPPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, UGeoIPPlugIn)

   delete country_forbidden_mask;

   for (uint32_t i = 0; i < NUM_DB_TYPES; ++i) if (gi[i]) U_SYSCALL_VOID(GeoIP_delete, "%p", gi[i]);
}

// Server-wide hooks

int UGeoIPPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UGeoIPPlugIn::handlerConfig(%p)", &cfg)

   // ------------------------------------------------------------------------------------------
   // COUNTRY_FORBIDDEN_MASK  mask (DOS regexp) of GEOIP country code that give forbidden access
   // ------------------------------------------------------------------------------------------

   if (cfg.loadTable()) *country_forbidden_mask = cfg[*str_COUNTRY_FORBIDDEN_MASK];

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UGeoIPPlugIn::handlerInit()
{
   U_TRACE(1, "UGeoIPPlugIn::handlerInit()")

   U_SYSCALL_VOID_NO_PARAM(_GeoIP_setup_dbfilename);

   // iterate through different database types

   char* db_info;
   bool result = false;

   for (uint32_t i = 0; i < NUM_DB_TYPES; ++i)
      {
      if (U_SYSCALL(GeoIP_db_avail, "%d", i))
         {
         gi[i] = (GeoIP*) U_SYSCALL(GeoIP_open_type, "%d,%d", i, GEOIP_STANDARD);

         if (gi[i] == 0)
            {
            U_SRV_LOG("%s not available, skipping...", GeoIPDBDescription[i]);
            }
         else
            {
            result = true;

            db_info = U_SYSCALL(GeoIP_database_info, "%p", gi[i]);

            U_SRV_LOG("%s available, %s", GeoIPDBDescription[i], db_info);

            U_SYSCALL_VOID(free, "%p", (void*)db_info);
            }
         }
      }

   U_SRV_LOG("initialization of plugin %s", (result ? "success" : "FAILED"));

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int UGeoIPPlugIn::handlerREAD()
{
   U_TRACE(0, "UGeoIPPlugIn::handlerREAD()")

   if (country_id || gir) UHTTP::geoip->setEmpty();

   if (country_forbidden_mask->empty() == false &&
       setCountryCode()                         &&
       checkCountryForbidden() == false)
      {
      U_RETURN(U_PLUGIN_HANDLER_FINISHED);
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UGeoIPPlugIn::handlerRequest()
{
   U_TRACE(0, "UGeoIPPlugIn::handlerRequest()")

   if (country_forbidden_mask->empty() == false &&
       setCountryCode()                         &&
       checkCountryForbidden() == false)
      {
      U_RETURN(U_PLUGIN_HANDLER_FINISHED);
      }

   if (country_id)
      {
      UHTTP::geoip->snprintf_add("GEOIP_COUNTRY_CODE=%s\n"  // code international du pays (suivant la norme ISO 3166)
                                 "GEOIP_COUNTRY_NAME=%s\n", // nom complet du pays (en anglais)
                                 country_code, country_name);
      }

   if (gir)
      {
      UHTTP::geoip->snprintf_add(
         "GEOIP_REGION=%s\n"        // un code indiquant la région
         "GEOIP_CITY=%s\n",         // la ville
         "GEOIP_DMA_CODE=%s\n",     // code DMA (Designated Market Area) attribué à une zone où les fréquences (télévision, radio, etc)
                                    // sont identiques (concerne les Etats Unis, valeur par défaut : 0)
         "GEOIP_AREA_CODE=%s\n",    // indice téléphonique représentant une zone précise (concerne les Etats Unis, valeur par défaut : 0)
         "GEOIP_LATITUDE=%f\n",     // la latitude
         "GEOIP_LONGITUDE=%f\n",    // la longitude
         "GEOIP_POSTAL_CODE=%s\n",  // le code postal (concerne uniquement les Etats Unis, ne sera pas définie pour tout autre pays)
         gir->region,
         gir->city,
         (bGEOIP_CITY_EDITION_REV1 ? gir->metro_code : 0),
         (bGEOIP_CITY_EDITION_REV1 ? gir->area_code  : 0),
         gir->area_code,
         gir->latitude,
         gir->longitude,
         gir->postal_code);
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UGeoIPPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "bGEOIP_CITY_EDITION_REV1        " << bGEOIP_CITY_EDITION_REV1      << '\n'
                  << "country_forbidden_mask (UString " << (void*)country_forbidden_mask << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
