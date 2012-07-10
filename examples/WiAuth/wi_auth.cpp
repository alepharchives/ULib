// wi_auth.cpp
   
#include <ulib/net/server/usp_macro.h>
   
class WiAuthAccessPoint {
   public:
      // Check for memory error
      U_MEMORY_TEST
   
      // Allocator e Deallocator
      U_MEMORY_ALLOCATOR
      U_MEMORY_DEALLOCATOR
   
      UString public_address;
      UVector<UString> web_hosts_allowed, mac_white_list, group_accounts;
      bool down, consume;
   
      // COSTRUTTORE
   
      WiAuthAccessPoint()
         {
         U_TRACE_REGISTER_OBJECT(5, WiAuthAccessPoint, "")
   
         down    = false;
         consume = true;
         }
   
      ~WiAuthAccessPoint()
         {
         U_TRACE_UNREGISTER_OBJECT(5, WiAuthAccessPoint)
         }
   
      // SERVICES
   
      UString toString()
         {
         U_TRACE(5, "WiAuthAccessPoint::toString()")
   
         UString x(U_CAPACITY),
                 vec1 = web_hosts_allowed.join(" "),
                 vec2 = mac_white_list.join(" "),
                 vec3 = group_accounts.join(" ");
   
         x.snprintf("%u %u "
                    "%*.s \"%.*s\" \"%.*s\" \"%.*s\"",
                    down, consume,
                    U_STRING_TO_TRACE(public_address), U_STRING_TO_TRACE(vec1), U_STRING_TO_TRACE(vec2), U_STRING_TO_TRACE(vec3));
   
         U_RETURN_STRING(x);
         }
   
      void fromString(const UString& _data)
         {
         U_TRACE(5, "WiAuthAccessPoint::fromString(%.*S)", U_STRING_TO_TRACE(_data))
   
         U_ASSERT_EQUALS(_data.empty(), false)
   
         istrstream is(_data.data(), _data.size());
         UString vec1(U_CAPACITY), vec2(U_CAPACITY), vec3(U_CAPACITY);
   
         is >> down
            >> consume;
   
         is.get(); // skip ' '
   
         public_address.get(is);
   
         is.get(); // skip ' '
   
         vec1.get(is);
   
         is.get(); // skip ' '
   
         vec2.get(is);
   
         is.get(); // skip ' '
   
         vec3.get(is);
   
                                           web_hosts_allowed.clear();
         if (vec1.empty() == false) (void) web_hosts_allowed.split(U_STRING_TO_PARAM(vec1));
   
                                           mac_white_list.clear();
         if (vec2.empty() == false) (void) mac_white_list.split(U_STRING_TO_PARAM(vec2));
   
                                           group_accounts.clear();
         if (vec3.empty() == false) (void) group_accounts.split(U_STRING_TO_PARAM(vec3));
         }
   
   #ifdef DEBUG
   #  include <ulib/internal/objectIO.h>
      const char* dump(bool breset) const
         {
         *UObjectIO::os << "down                                " << down                       << '\n'
                        << "consume                             " << consume                    << '\n'
                        << "public_address    (UString>         " << (void*)&public_address     << ")\n"
                        << "group_accounts    (UVector<UString> " << (void*)&group_accounts     << ")\n"
                        << "mac_white_list    (UVector<UString> " << (void*)&mac_white_list     << ")\n"
                        << "web_hosts_allowed (UVector<UString> " << (void*)&web_hosts_allowed  << ')';
   
         if (breset)
            {
            UObjectIO::output();
   
            return UObjectIO::buffer_output;
            }
   
         return 0;
         }
   #endif
   private:
      WiAuthAccessPoint(const WiAuthAccessPoint&)            {}
      WiAuthAccessPoint& operator=(const WiAuthAccessPoint&) { return *this; }
   };
   
   class WiAuthUser {
   public:
      // Check for memory error
      U_MEMORY_TEST
   
      // Allocator e Deallocator
      U_MEMORY_ALLOCATOR
      U_MEMORY_DEALLOCATOR
   
      UString user, auth_domain, ip, mac, ap, policy, token;
      uint64_t traffic_done, traffic_available;
      time_t time_done, time_available, login_time, last_modified;
      bool connected, consume, group_account;
   
      // COSTRUTTORE
   
      WiAuthUser()
         {
         U_TRACE_REGISTER_OBJECT(5, WiAuthUser, "")
   
         traffic_done = traffic_available = 0;
         time_done    = time_available = login_time = last_modified = 0;
         connected    = consume = group_account = false;
         }
   
      ~WiAuthUser()
         {
         U_TRACE_UNREGISTER_OBJECT(5, WiAuthUser)
         }
   
      // SERVICES
   
      UString toString()
         {
         U_TRACE(5, "WiAuthUser::toString()")
   
         UString x(U_CAPACITY);
   
         x.snprintf("%u %ld %llu %ld %llu "
                    "%ld %ld %u %u "
                    "\"%.*s\" %.*s %.*s %.*s %.*s %.*s %.*s",
                    connected,
                       time_done,
                    traffic_done,
                       time_available,
                    traffic_available,
                    login_time, last_modified, consume, group_account,
                    U_STRING_TO_TRACE(user), U_STRING_TO_TRACE(auth_domain), U_STRING_TO_TRACE(ip),
                    U_STRING_TO_TRACE(mac), U_STRING_TO_TRACE(ap), U_STRING_TO_TRACE(policy), U_STRING_TO_TRACE(token));
   
         U_RETURN_STRING(x);
         }
   
      void fromString(const UString& _data)
         {
         U_TRACE(5, "WiAuthUser::fromString(%.*S)", U_STRING_TO_TRACE(_data))
   
         U_ASSERT_EQUALS(_data.empty(), false)
   
         istrstream is(_data.data(), _data.size());
   
         is >> connected
            >>    time_done
            >> traffic_done
            >>    time_available
            >> traffic_available
            >> login_time
            >> last_modified
            >> consume
            >> group_account;
   
         is.get(); // skip ' '
   
         user.get(is);
   
         is.get(); // skip ' '
   
         auth_domain.get(is);
   
         is.get(); // skip ' '
   
         ip.get(is);
   
         is.get(); // skip ' '
   
         mac.get(is);
   
         is.get(); // skip ' '
   
         ap.get(is);
   
         is.get(); // skip ' '
   
         policy.get(is);
   
         is.get(); // skip ' '
   
         token.get(is);
         }
   
   #ifdef DEBUG
   #  include <ulib/internal/objectIO.h>
      const char* dump(bool breset) const
         {
         *UObjectIO::os << "consume              " << consume              << '\n'
                        << "connected            " << connected            << '\n'
                        << "time_done            " << time_done            << '\n'
                        << "login_time           " << login_time           << '\n'
                        << "group_account        " << group_account        << '\n'
                        << "traffic_done         " << traffic_done         << '\n'
                        << "last_modified        " << last_modified        << '\n'
                        << "time_available       " << time_available       << '\n'
                        << "traffic_available    " << traffic_available    << '\n'
                        << "ip          (UString " << (void*)&ip           << ")\n"
                        << "ap          (UString " << (void*)&ap           << ")\n"
                        << "mac         (UString " << (void*)&mac          << ")\n"
                        << "user        (UString " << (void*)&user         << ")\n"
                        << "token       (UString " << (void*)&token        << ")\n"
                        << "policy      (UString " << (void*)&policy       << ")\n"
                        << "auth_domain (UString " << (void*)&auth_domain  << ')';
   
         if (breset)
            {
            UObjectIO::output();
   
            return UObjectIO::buffer_output;
            }
   
         return 0;
         }
   #endif
   private:
      WiAuthUser(const WiAuthUser&)            {}
      WiAuthUser& operator=(const WiAuthUser&) { return *this; }
   };
   
   static URDB* db_ap;
   static URDB* db_user;
   
   static void usp_init()
   {
      U_TRACE(5, "::usp_init()")
   
      if (UServer_Base::bssl == false)
         {
         U_INTERNAL_ASSERT_POINTER(U_LOCK_USER1)
         U_INTERNAL_ASSERT_POINTER(U_LOCK_USER2)
   
         UString pathdb_ap   = U_STRING_FROM_CONSTANT(U_LIBEXECDIR "/WiAuthAccessPoint.cdb"),
                 pathdb_user = U_STRING_FROM_CONSTANT(U_LIBEXECDIR "/WiAuthUser");
   
         db_ap   = U_NEW(URDB(pathdb_ap,   false));
         db_user = U_NEW(URDB(pathdb_user, false));
   
         if (db_ap->open(1024 * 1024, false) == false)
            {
            U_SRV_LOG("db initialization of wi-auth access point \"WiAuthAccessPoint.cdb\" failed...");
   
            delete db_ap;
                   db_ap = 0;
   
            return;
            }
   
         ((URDB*)db_ap)->setShared(U_LOCK_USER1);
   
         U_SRV_LOG("db initialization of wi-auth access point \"WiAuthAccessPoint.cdb\" success");
   
         if (db_user->open(1024 * 1024, false) == false)
            {
            U_SRV_LOG("db initialization of wi-auth users \"WiAuthUser\" failed...");
   
            delete db_user;
                   db_user = 0;
   
            return;
            }
   
         ((URDB*)db_user)->setShared(U_LOCK_USER2);
   
         U_SRV_LOG("db initialization of wi-auth users \"WiAuthUser\" success");
         }
   }
   
   static void usp_end()
   {
      U_TRACE(5, "::usp_end()")
   
   #ifdef DEBUG
      if (UServer_Base::bssl == false)
         {
         if (db_ap)   delete db_ap;
         if (db_user) delete db_user;
         }
   #endif
   }
   
   static void write_SSI()
   {
      U_TRACE(5, "::write_SSI()")
   
      /*
      if [ -n "$CONNECTION_CLOSE" ]; then
      HTTP_RESPONSE_HEADER="Connection: close\r\n$HTTP_RESPONSE_HEADER"
      fi
   
      if [ -n "$FILE_RESPONSE_HTML" ]; then
      echo \"FILE_RESPONSE_HTML=$FILE_RESPONSE_HTML\"
      else
      if [ -n  "$HTTP_RESPONSE_HEADER" ]; then
      echo \"HTTP_RESPONSE_HEADER=$HTTP_RESPONSE_HEADER\"
      fi
   
      if [ -n "$HTTP_RESPONSE_BODY" ]; then
      echo  \"HTTP_RESPONSE_BODY=$HTTP_RESPONSE_BODY\"
      else
      if [ -z "$BODY_SHTML" ]; then
   
      TITLE_TXT="400 Bad Request"
      BODY_SHTML="<h1>Bad Request</h1><p>Your browser sent a request that this server could not understand<br></p>"
   
      EXIT_VALUE=400
      fi
   
      if [ -z "$TITLE_TXT" ]; then
      TITLE_TXT="Firenze WiFi"
      fi
   
      echo -e "'TITLE_TXT=$TITLE_TXT'\nBODY_STYLE=$BODY_STYLE"
   
      if [ -n "$HEAD_HTML" ]; then
      echo "'HEAD_HTML=$HEAD_HTML'"
      fi
   
      echo -n -e "$BODY_SHTML" > $FILE_BODY_SHTML
      fi
      fi
   
      uscita
      */
   }  
   
extern "C" {
extern U_EXPORT int runDynamicPage(UClientImage_Base* client_image);
       U_EXPORT int runDynamicPage(UClientImage_Base* client_image)
{
   U_TRACE(0, "::runDynamicPage(%p)", client_image)
   
   if (client_image == 0)         { usp_init();  U_RETURN(0); }
   
   if (client_image == (void*)-1) {              U_RETURN(0); }
   
   if (client_image == (void*)-2) { usp_end();   U_RETURN(0); }
   
   uint32_t usp_sz;
   char usp_buffer[10 * 4096];
   bool usp_as_service = (client_image == (UClientImage_Base*)-3);
   
   if (UHTTP::isHttpGET())
      {
      }
   else if (UHTTP::isHttpPOST())
      {
      }
   
   return HTTP_BAD_METHOD;
   
   U_RETURN(usp_as_service ? U_NOT_FOUND : 200);
} }