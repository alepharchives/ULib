// wi_auth.cpp
   
#include <ulib/net/server/usp_macro.h>
   
#include <ulib/net/server/plugin/mod_ssi.h>
   
   #ifdef DEBUG
   #include <ulib/internal/objectIO.h>
   #endif
   
   class WiAuthUser;
   class WiAuthNodog;
   
   static int          ap_port;
   static int          priority;
   static URDB*        db_ap;
   static URDB*        db_user;
   static UString*     ap;
   static UString*     uid;
   static UString*     label;
   static UString*     address;
   static UString*     portal_name;
   static UString*     request_uri;
   static UString*     client_address;
   static WiAuthUser*  user_rec;
   static WiAuthNodog* nodog_rec;
   
   #define NUM_ACCESS_POINT_ATTRIBUTE 4
   
   class WiAuthAccessPoint {
   public:
      // Check for memory error
      U_MEMORY_TEST
   
      // Allocator e Deallocator
      U_MEMORY_ALLOCATOR
      U_MEMORY_DEALLOCATOR
   
      UString label;
      UVector<UString> mac_white_list, group_accounts;
      bool noconsume;
   
      // COSTRUTTORE
   
      WiAuthAccessPoint()
         {
         U_TRACE_REGISTER_OBJECT(5, WiAuthAccessPoint, "")
   
         noconsume = false;
         }
   
      ~WiAuthAccessPoint()
         {
         U_TRACE_UNREGISTER_OBJECT(5, WiAuthAccessPoint)
         }
   
      // SERVICES
   
      void reset()
         {
         U_TRACE(5, "WiAuthAccessPoint::reset()")
   
         label.clear();
   
         noconsume = false;
   
         mac_white_list.clear();
         group_accounts.clear();
         }
   
      UString toString()
         {
         U_TRACE(5, "WiAuthAccessPoint::toString()")
   
         UString x(U_CAPACITY),
                 vec1 = mac_white_list.join(" "),
                 vec2 = group_accounts.join(" ");
   
         x.snprintf("%u %.*s \"%.*s\" \"%.*s\"", noconsume, U_STRING_TO_TRACE(label), U_STRING_TO_TRACE(vec1), U_STRING_TO_TRACE(vec2));
   
         U_RETURN_STRING(x);
         }
   
      void fromString(const UString& data)
         {
         U_TRACE(5, "WiAuthAccessPoint::fromString(%.*S)", U_STRING_TO_TRACE(data))
   
         U_ASSERT_EQUALS(data.empty(), false)
   
         istrstream is(data.data(), data.size());
         UString vec1(U_CAPACITY), vec2(U_CAPACITY);
   
         is >> noconsume;
   
         is.get(); // skip ' '
   
         label.get(is);
   
         is.get(); // skip ' '
   
         vec1.get(is);
   
         is.get(); // skip ' '
   
         vec2.get(is);
   
                                           mac_white_list.clear();
         if (vec1.empty() == false) (void) mac_white_list.split(U_STRING_TO_PARAM(vec1));
   
                                           group_accounts.clear();
         if (vec2.empty() == false) (void) group_accounts.split(U_STRING_TO_PARAM(vec2));
         }
   
   #ifdef DEBUG
      const char* dump(bool breset) const
         {
         *UObjectIO::os << "noconsume                        " << noconsume              << '\n'
                        << "label          (UString>         " << (void*)&label          << ")\n"
                        << "group_accounts (UVector<UString> " << (void*)&group_accounts << ")\n"
                        << "mac_white_list (UVector<UString> " << (void*)&mac_white_list << ')';
   
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
   
   static bool  user_exist;
   static bool nodog_exist;
   
   class WiAuthNodog {
   public:
      // Check for memory error
      U_MEMORY_TEST
   
      // Allocator e Deallocator
      U_MEMORY_ALLOCATOR
      U_MEMORY_DEALLOCATOR
   
      int port;
      UString hostname;
      UVector<UString> access_point;
      bool down;
   
      // COSTRUTTORE
   
      WiAuthNodog()
         {
         U_TRACE_REGISTER_OBJECT(5, WiAuthNodog, "")
         }
   
      ~WiAuthNodog()
         {
         U_TRACE_UNREGISTER_OBJECT(5, WiAuthNodog)
         }
   
      // SERVICES
   
      UString toString()
         {
         U_TRACE(5, "WiAuthNodog::toString()")
   
         UString x(U_CAPACITY), ap_list = access_point.join(" ");
   
         x.snprintf("%u %u %.*s [%.*s]", down, port, U_STRING_TO_TRACE(hostname), U_STRING_TO_TRACE(ap_list));
   
         U_RETURN_STRING(x);
         }
   
      void fromString(const UString& data)
         {
         U_TRACE(5, "WiAuthNodog::fromString(%.*S)", U_STRING_TO_TRACE(data))
   
         U_ASSERT_EQUALS(data.empty(), false)
   
         istrstream is(data.data(), data.size());
   
         is >> down
            >> port;
   
         is.get(); // skip ' '
   
         hostname.get(is);
   
         is.get(); // skip ' '
         is.get(); // skip '['
   
         access_point.clear();
         access_point.readVector(is, ']');
         }
   
      bool setRecord()
         {
         U_TRACE(5, "WiAuthNodog::setRecord()")
   
         U_ASSERT_EQUALS(address->empty(), false)
   
         int op        = -1;
         UString value = (*db_ap)[*address], ap_new(100U);
         nodog_exist   = (value.empty() == false);
   
         if (nodog_exist)
            {
            fromString(value);
   
            if ( ap->empty() == false &&
                *ap != hostname)
               {
               ULog::logger(portal_name->data(), priority,
                              "%.*s: %.*s *** AP HOSTNAME NOT EQUAL ***",
                              U_STRING_TO_TRACE(*request_uri), U_STRING_TO_TRACE(*ap));
               }
   
            if (down)
               {
               op = RDB_REPLACE;
   
               char* ptr = value.data();
   
               *ptr = '0';
               down = false;
               }
   
            if (label->empty()                   == false &&
                access_point.isContained(*label) == false)
               {
               op = RDB_REPLACE;
   
               goto add_ap;
               }
            }
         else
            {
            U_ASSERT_EQUALS(ap->empty(), false)
            U_INTERNAL_ASSERT(u_isHostName(U_STRING_TO_PARAM(*ap)))
   
            op       = RDB_INSERT;
            port     = ap_port;
            down     = false;
            hostname = *ap;
   
            access_point.clear();
   add_ap:
            U_ASSERT_EQUALS(label->empty(), false)
   
            ap_new.snprintf("0 %.*s \"\" \"\"", U_STRING_TO_TRACE(*label));
   
            access_point.push_back(ap_new);
   
            value = toString();
            }
   
         if (op != -1)
            {
            int result = db_ap->store(*address, value, op);
   
            if (result) U_SRV_LOG("store with flag %d on db WiAuthNodog failed with error %d", op, result);
            }
   
         U_RETURN(nodog_exist);
         }
   
   #ifdef DEBUG
      const char* dump(bool breset) const
         {
         *UObjectIO::os << "down                           " << down                 << '\n'
                        << "port                           " << port                 << '\n'
                        << "hostname     (UString>         " << (void*)&hostname     << ")\n"
                        << "access_point (UVector<UString> " << (void*)&access_point << ')';
   
         if (breset)
            {
            UObjectIO::output();
   
            return UObjectIO::buffer_output;
            }
   
         return 0;
         }
   #endif
   private:
      WiAuthNodog(const WiAuthNodog&)            {}
      WiAuthNodog& operator=(const WiAuthNodog&) { return *this; }
   };
   
   class WiAuthUser {
   public:
      // Check for memory error
      U_MEMORY_TEST
   
      // Allocator e Deallocator
      U_MEMORY_ALLOCATOR
      U_MEMORY_DEALLOCATOR
   
      UString user, auth_domain, ip, mac, gateway, policy, token, nodog;
      uint64_t traffic_done, traffic_available;
      time_t time_done, time_available, login_time, last_modified;
      uint32_t access_point_index;
      bool connected, consume, group_account;
   
      // COSTRUTTORE
   
      WiAuthUser()
         {
         U_TRACE_REGISTER_OBJECT(5, WiAuthUser, "")
   
         consume            = true;
         connected          = group_account = false;
         time_available     = time_done = login_time = last_modified = 0;
         traffic_available  = traffic_done = 0;
         access_point_index = 0;
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
   
         x.snprintf("%u %u %u %u "
                    "%ld %llu %ld %llu "
                    "%ld %ld "
                    "\"%.*s\" %.*s %.*s %.*s %.*s %.*s %.*s %.*s",
                    connected, access_point_index, consume, group_account,
                       time_done,
                    traffic_done,
                       time_available,
                    traffic_available,
                    login_time, last_modified,
                    U_STRING_TO_TRACE(user), U_STRING_TO_TRACE(auth_domain), U_STRING_TO_TRACE(ip),
                    U_STRING_TO_TRACE(mac), U_STRING_TO_TRACE(gateway), U_STRING_TO_TRACE(policy), U_STRING_TO_TRACE(token), U_STRING_TO_TRACE(nodog));
   
         U_RETURN_STRING(x);
         }
   
      void fromString(const UString& _data)
         {
         U_TRACE(5, "WiAuthUser::fromString(%.*S)", U_STRING_TO_TRACE(_data))
   
         U_ASSERT_EQUALS(_data.empty(), false)
   
         istrstream is(_data.data(), _data.size());
   
         is >> connected
            >> access_point_index
            >> consume
            >> group_account
            >>    time_done
            >> traffic_done
            >>    time_available
            >> traffic_available
            >> login_time
            >> last_modified;
   
         is.get(); // skip ' '
   
         user.get(is);
   
         is.get(); // skip ' '
   
         auth_domain.get(is);
   
         is.get(); // skip ' '
   
         ip.get(is);
   
         is.get(); // skip ' '
   
         mac.get(is);
   
         is.get(); // skip ' '
   
         gateway.get(is);
   
         is.get(); // skip ' '
   
         policy.get(is);
   
         is.get(); // skip ' '
   
         token.get(is);
   
         is.get(); // skip ' '
   
         nodog.get(is);
         }
   
      UString getAP()
         {
         U_TRACE(5, "WiAuthUser::getAP()")
   
         *label = nodog_rec->access_point[(access_point_index * NUM_ACCESS_POINT_ATTRIBUTE) + 1];
   
         UString x(100U);
   
         x.snprintf("%.*s@%.*s", U_STRING_TO_TRACE(*label), U_STRING_TO_TRACE(nodog));
   
         U_RETURN_STRING(x);
         }
   
      bool setRecord()
         {
         U_TRACE(5, "WiAuthUser::setRecord()")
   
         U_ASSERT_EQUALS(uid->empty(), false)
   
         int op        = -1;
         UString value = (*db_user)[*uid], user_new(100U);
         user_exist    = (value.empty() == false);
   
         U_RETURN(false);
         }
   
   #ifdef DEBUG
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
                        << "access_point_index   " << access_point_index   << '\n'
                        << "ip          (UString " << (void*)&ip           << ")\n"
                        << "mac         (UString " << (void*)&mac          << ")\n"
                        << "user        (UString " << (void*)&user         << ")\n"
                        << "nodog       (UString " << (void*)&nodog        << ")\n"
                        << "token       (UString " << (void*)&token        << ")\n"
                        << "policy      (UString " << (void*)&policy       << ")\n"
                        << "gateway     (UString " << (void*)&gateway      << ")\n"
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
   
   static void check_if_user_exist(UStringRep* key, UStringRep* data)
   {
      U_TRACE(5, "::check_if_user_exist(%.*S,%.*S)", U_STRING_TO_TRACE(*key), U_STRING_TO_TRACE(*data))
   
      user_rec->fromString(UString(data));
   
      if (user_rec->ip == *client_address)
         {
         user_exist = true;
   
         db_user->stopCallForAllEntry();
         }
   }
   
   static bool check_if_user_is_connected()
   {
      U_TRACE(5, "::check_if_user_is_connected()")
   
      user_exist = false;
   
      if (uid->empty()) db_user->callForAllEntry(check_if_user_exist);
      else
         {
         UString data = (*db_user)[*uid];
         user_exist   = (data.empty() == false);
   
         if (user_exist) user_rec->fromString(data);
         }
   
      if (user_exist)
         {
         ap->clear();
   
         *address = user_rec->nodog;
   
         if (nodog_rec->setRecord() == false)
            {
            ULog::logger(portal_name->data(), priority,
                           "%.*s: *** USER %.*s CONNECTED ON AP %.*s NOT EXISTENT ***",
                           U_STRING_TO_TRACE(*request_uri), U_STRING_TO_TRACE(*client_address), U_STRING_TO_TRACE(*address));
            }
   
         U_RETURN(user_rec->connected);
         }
   
      U_RETURN(false);
   }
   
   #define GET_admin                 0
   #define GET_card_activation       1
   #define GET_error_ap              2
   #define GET_get_config            3
   #define GET_get_users_info        4
   #define GET_info                  5
   #define GET_logged                6
   #define GET_login                 7
   #define GET_login_request         8
   #define GET_login_validate        9
   #define GET_logout               10
   #define GET_logout_page          11
   #define GET_polling_attivazione  12
   #define GET_postlogin            13
   #define GET_recovery             14
   #define GET_registrazione        15
   #define GET_reset_policy         16
   #define GET_start_ap             17
   #define GET_stato_utente         18
   #define GET_status_ap            19
   #define GET_status_network       20
   #define GET_unifi                21
   #define GET_unifi_login_request  22
   #define GET_view_user            23
   #define GET_webif_ap             24
   
   static UVector<UString>* GET_request;
   static         UString*  GET_request_str;
   
   #define POST_login_request        0
   #define POST_logout               1
   #define POST_registrazione        2
   #define POST_uploader             3
   
   static UVector<UString>* POST_request;
   static         UString*  POST_request_str;
   
   #define VIRTUAL_HOST "wifi-aaa.comune.fi.it"
   
   static UCache*  cache;
   static uint32_t num_ap;
   static UString* body;
   static UString* ap_ref;
   static UString* dir_root;
   static UString* ip_server;
   static UString* login_url;
   static UString* caller_id;
   static UString* tutela_dati;
   static UString* virtual_name;
   static UString* dir_template;
   static UString* registrazione_url;
   static UString* allowed_web_hosts;
   static UString* historical_log_dir;
   
   static UFile*   url_banner_ap_default;
   static UFile*   url_banner_comune_default;
   static UString* help_url;
   static UString* wallet_url;
   static UString* url_banner_ap;
   static UString* url_banner_comune;
   
   static void count_num_ap(UStringRep* key, UStringRep* data)
   {
      U_TRACE(5, "::count_num_ap(%.*S,%.*S)", U_STRING_TO_TRACE(*key), U_STRING_TO_TRACE(*data))
   
      nodog_rec->fromString(UString(data));
   
      num_ap += nodog_rec->access_point.size() / NUM_ACCESS_POINT_ATTRIBUTE;
   
      U_INTERNAL_DUMP("num_ap = %u", num_ap)
   }
   
   static void usp_init()
   {
      U_TRACE(5, "::usp_init()")
   
      U_INTERNAL_ASSERT_POINTER(U_LOCK_USER1)
      U_INTERNAL_ASSERT_POINTER(U_LOCK_USER2)
   
      UString pathdb_ap   = U_STRING_FROM_CONSTANT(U_LIBEXECDIR "/WiAuthAccessPoint.cdb"),
              pathdb_user = U_STRING_FROM_CONSTANT(U_LIBEXECDIR "/WiAuthUser");
   
      db_ap   = U_NEW(URDB(pathdb_ap,   false));
      db_user = U_NEW(URDB(pathdb_user, false));
   
      db_ap->setShared(U_LOCK_USER1);
      db_user->setShared(U_LOCK_USER2);
   
      bool result;
   
      db_ap->lock();
   
      result = db_ap->open(4 * 1024, false);
   
      db_ap->unlock();
   
      nodog_rec = U_NEW(WiAuthNodog);
   
      if (UServer_Base::bssl == false)
         {
         db_ap->callForAllEntry(count_num_ap);
   
         U_SRV_LOG("db initialization of wi-auth access point WiAuthAccessPoint.cdb %s: num_ap %u", result ? "success" : "FAILED", num_ap);
   
         UFile::writeToTmpl("/tmp/WiAuthAccessPoint.init", db_ap->print());
         }
   
      db_user->lock();
   
      result = db_user->open(1024 * 1024, false);
   
      db_user->unlock();
   
      user_rec = U_NEW(WiAuthUser);
   
      if (UServer_Base::bssl == false)
         {
         U_SRV_LOG("db initialization of wi-auth users WiAuthUser %s", result ? "success" : "FAILED");
         }
   
      ap                 = U_NEW(UString);
      uid                = U_NEW(UString);
      body               = U_NEW(UString);
      label              = U_NEW(UString);
      ap_ref             = U_NEW(UString(100U));
      address            = U_NEW(UString);
      caller_id          = U_NEW(UString(100U));
      ip_server          = U_NEW(UString(UServer_Base::getIPAddress()));
      tutela_dati        = U_NEW(UString);
      request_uri        = U_NEW(UString);
      client_address     = U_NEW(UString);
      allowed_web_hosts  = U_NEW(UString);
   
      UString environment = *USSIPlugIn::environment + "VIRTUAL_HOST=" + VIRTUAL_HOST;
   
      dir_root           = U_NEW(UString(UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("DIR_ROOT"),           &environment)));
      portal_name        = U_NEW(UString(UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("PORTAL_NAME"),        &environment)));
      virtual_name       = U_NEW(UString(UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("VIRTUAL_NAME"),       &environment)));
      historical_log_dir = U_NEW(UString(UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("HISTORICAL_LOG_DIR"), &environment)));
   
      U_INTERNAL_ASSERT(portal_name->isNullTerminated())
   
      priority = ULog::getPriorityForLogger(UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("LOCAL_SYSLOG_SELECTOR"), &environment).data());
   
      GET_request_str = U_NEW(U_STRING_FROM_CONSTANT(
                                 "/admin "
                                 "/card_activation "
                                 "/error_ap "
                                 "/get_config "
                                 "/get_users_info "
                                 "/info "
                                 "/logged "
                                 "/login "
                                 "/login_request "
                                 "/login_validate "
                                 "/logout "
                                 "/logout_page "
                                 "/polling_attivazione "
                                 "/postlogin "
                                 "/recovery "
                                 "/registrazione "
                                 "/reset_policy "
                                 "/start_ap "
                                 "/stato_utente "
                                 "/status_ap "
                                 "/status_network "
                                 "/unifi "
                                 "/unifi_login_request "
                                 "/view_user "
                                 "/webif_ap"));
   
      GET_request = U_NEW(UVector<UString>(*GET_request_str));
   
      POST_request_str = U_NEW(U_STRING_FROM_CONSTANT(
                                 "/login_request "
                                 "/logout "
                                 "/registrazione "
                                 "/uploader"));
   
      POST_request = U_NEW(UVector<UString>(*POST_request_str));
   
      UString content = UFile::contentOf("$DIR_ROOT/etc/AllowedWebHosts.txt", O_RDONLY, false, &environment);
   
      UVector<UString> vec(content);
   
      if (vec.empty() == false)
         {
         *allowed_web_hosts = vec.join(" ") + ' ';
   
         vec.clear();
         }
   
      cache   = U_NEW(UCache);
      content = U_STRING_FROM_CONSTANT("$DIR_ROOT/etc/" VIRTUAL_HOST "/cache.tmpl");
   
      dir_template = U_NEW(UString(UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("DIR_TEMPLATE"), &environment)));
   
      if (cache->open(content, 96 * 1024, &environment) == false) cache->loadContentOf(*dir_template);
   
      dir_template->push_back('/');
   
      *tutela_dati = cache->contentOf(*dir_template + "tutela_dati.txt");
   
      UHashMap<UString> table;
   
      table.allocate();
   
      content = UFile::contentOf("$DIR_ROOT/etc/" VIRTUAL_HOST "/script.conf", O_RDONLY, false, &environment);
   
      if (UFileConfig::loadProperties(table, content.data(), content.end()))
         {
         url_banner_ap     = U_NEW(UString(UStringExt::expandPath(table["URL_BANNER_AP"],               &environment)));
         url_banner_comune = U_NEW(UString(UStringExt::expandPath(table["URL_BANNER_COMUNE"],           &environment)));
   
         help_url          = U_NEW(UString(UStringExt::expandEnvironmentVar(table["HELP_URL"],          &environment)));
         login_url         = U_NEW(UString(UStringExt::expandEnvironmentVar(table["LOGIN_URL"],         &environment)));
         wallet_url        = U_NEW(UString(UStringExt::expandEnvironmentVar(table["WALLET_URL"],        &environment)));
         registrazione_url = U_NEW(UString(UStringExt::expandEnvironmentVar(table["REGISTRAZIONE_URL"], &environment)));
   
         UString x(U_CAPACITY);
   
         x.snprintf("$DIR_WEB/" VIRTUAL_HOST "%s/default", url_banner_ap->data());
   
         url_banner_ap_default = U_NEW(UFile(x, &environment));
   
         x.snprintf("$DIR_WEB/" VIRTUAL_HOST "%s/default", url_banner_comune->data());
   
         url_banner_comune_default = U_NEW(UFile(x, &environment));
   
         if (url_banner_ap_default->stat() == false)
            {
            delete url_banner_ap_default;
                   url_banner_ap_default = 0;
            }
   
         if (url_banner_comune_default->stat() == false)
            {
            delete url_banner_comune_default;
                   url_banner_comune_default = 0;
            }
         }
   
      table.clear();
      table.deallocate();
   }
   
   static void usp_end()
   {
      U_TRACE(5, "::usp_end()")
   
      if (db_ap)
         {
         if (UServer_Base::bssl == false)
            {
            UFile::writeToTmpl("/tmp/WiAuthAccessPoint.end", db_ap->print());
   
            (void) db_ap->closeReorganize();
            }
   
   #  ifdef DEBUG
         delete ap;
         delete uid;
         delete body;
         delete db_ap;
         delete cache;
         delete label;
         delete ap_ref;
         delete address;
         delete db_user;
         delete dir_root;
         delete user_rec;
         delete nodog_rec;
         delete ip_server;
         delete caller_id;
         delete tutela_dati;
         delete request_uri;
         delete portal_name;
         delete GET_request;
         delete POST_request;
         delete dir_template;
         delete virtual_name;
         delete client_address;
         delete GET_request_str;
         delete POST_request_str;
         delete allowed_web_hosts;
   
         if (help_url)
            {
            delete help_url;
            delete login_url;
            delete wallet_url;
            delete url_banner_ap;
            delete url_banner_comune;
            delete registrazione_url;
   
            if (url_banner_ap_default)     delete url_banner_ap_default;
            if (url_banner_comune_default) delete url_banner_comune_default;
            }
   #  endif
         }
   }
   
   static void quitUsersLogged()
   {
      U_TRACE(5, "::quitUsersLogged()")
   
      // TODO
   }
   
   static void setAccessPointReference(bool localization)
   {
      U_TRACE(5, "::setAccessPointReference(%b)", localization)
   
      UHTTP::processHTTPForm();
   
      if (localization == false)
         {
         // ------------------------------------------------
         // $2 -> key
         // ------------------------------------------------
   
         UString key;
   
         UHTTP::getFormValue(key, 3);
   
         ap_ref->snprintf("%.*s", U_STRING_TO_TRACE(key));
         }
      else
         {
         // ------------------------------------------------
         // $7 -> ap (with localization => '@')
         // ------------------------------------------------
   
         UHTTP::getFormValue(*ap, 13);
   
         uint32_t pos = ap->find('@');
   
         U_INTERNAL_ASSERT_DIFFERS(pos, U_NOT_FOUND)
   
         uint32_t certid = 0;
         const char* ptr = ap->c_pointer(pos+1);
   
         // 10.8.1.2
   
         for (uint32_t i = 0, dot_count = 0; dot_count < 3; ++i)
            {
            if (ptr[i++] == '.')
               {
               ++dot_count;
   
                    if (dot_count == 2) certid  = 254 * strtol(ptr+i, 0, 10);
               else if (dot_count == 3) certid +=       strtol(ptr+i, 0, 10);
               }
            }
   
         ap_ref->snprintf("X%04dR%.*s", certid, pos, ap->data());
         }
   }
   
   static bool setAccessPoint(bool localization, bool start)
   {
      U_TRACE(5, "::setAccessPoint(%b,%b)", localization, start)
   
      // $1 -> ap
      // $2 -> public address to contact the access point
      // $3 -> pid (0 => start)
   
      UHTTP::processHTTPForm();
   
      uint32_t pos;
      UString hostname, public_address;
   
      UHTTP::getFormValue(hostname,       1);
      UHTTP::getFormValue(public_address, 3);
   
      if (public_address.empty()) address->clear();
      else
         {
         pos = public_address.find(':');
   
         U_INTERNAL_ASSERT_DIFFERS(pos, U_NOT_FOUND)
   
         *address = public_address.substr(0U, pos).copy();
   
         if (u_isIPv4Addr(U_STRING_TO_PARAM(*address)) == false)
            {
            ULog::logger(portal_name->data(), priority,
                              "%.*s: *** ADDRESS AP %.*s NOT VALID ***",
                              U_STRING_TO_TRACE(*request_uri), U_STRING_TO_TRACE(*address));
   
            U_RETURN(false);
            }
   
         ap_port = public_address.substr(pos+1).strtol();
         }
   
      if (localization)
         {
         pos = hostname.find('@');
   
         U_ASSERT_DIFFERS(pos, U_NOT_FOUND)
   
         *label = hostname.substr(0U, pos).copy();
         *ap    = hostname.substr(pos + 1).copy();
         }
      else
         {
         U_ASSERT_EQUALS(hostname.find('@'), U_NOT_FOUND)
   
         label->clear();
   
         *ap = hostname;
         }
   
      if (u_isHostName(U_STRING_TO_PARAM(*ap)) == false)
         {
         ULog::logger(portal_name->data(), priority,
                           "%.*s: *** AP HOSTNAME %.*s NOT VALID ***",
                           U_STRING_TO_TRACE(*request_uri), U_STRING_TO_TRACE(*ap));
   
         U_RETURN(false);
         }
   
      bool restart = nodog_rec->setRecord();
   
      if (start)
         {
         UString pid(100U);
   
         UHTTP::getFormValue(pid, 5);
   
         ULog::logger(portal_name->data(), priority, "%.*s: %.*s %s",
                      U_STRING_TO_TRACE(*request_uri),
                      U_STRING_TO_TRACE(*ap), (pid.strtol() ? "*** AP CRASHED ***" : "started"));
   
         if (restart) quitUsersLogged();
         }
   
      U_RETURN(true);
   }
   
   static void setAlternativeResponse()
   {
      U_TRACE(5, "::setAlternativeResponse()")
   
      U_http_is_connection_close = U_YES;
   
      USSIPlugIn::alternative_response = 1;
   
      U_INTERNAL_DUMP("body = %.*S", U_STRING_TO_TRACE(*body))
   
      if (body->empty())
         {
         u_http_info.nResponseCode = HTTP_NO_CONTENT;
   
         UHTTP::setHTTPResponse(0, 0);
         }
      else
         {
         u_http_info.nResponseCode = HTTP_OK;
   
         UHTTP::setHTTPResponse(UHTTP::str_ctype_html, body);
         }
   }
   
   static void setAlternativeInclude(const char* title_txt, const char* head_html, const char* body_style, const char* name_template, ...)
   {
      U_TRACE(5, "::setAlternativeInclude(%S,%S,%S,%S)", title_txt, head_html, body_style, name_template)
   
      UString _template = cache->contentOf(*dir_template + name_template);
   
      U_INTERNAL_ASSERT(_template.isNullTerminated())
   
      USSIPlugIn::alternative_include->setBuffer(_template.size() + 256 * 1024);
   
      va_list argp;
      va_start(argp, name);
   
      USSIPlugIn::alternative_include->vsnprintf(_template.data(), argp);
   
      va_end(argp);
   
      u_http_info.nResponseCode = HTTP_NO_CONTENT;
   
      if (title_txt == 0) title_txt = "Firenze WiFi";
   
      UString buffer(U_CAPACITY);
   
                      buffer.snprintf(    "'TITLE_TXT=%s'\n", title_txt ? title_txt : "Firenze WiFi");
      if (head_html)  buffer.snprintf_add("'HEAD_HTML=%s'\n", head_html);
      if (body_style) buffer.snprintf_add("'BODY_STYLE=%s'\n", body_style);
   
      (void) UClientImage_Base::environment->append(buffer);
   }
   
   static void setMessagePage(const char* title_txt, const char* message)
   {
      U_TRACE(5, "::setMessagePage(%S,%S)", title_txt, message)
   
      setAlternativeInclude(title_txt, 0, 0, "message_page.tmpl", title_txt, message);
   }
   
   static void setAlternativeRedirect(const char* fmt, ...)
   {
      U_TRACE(5, "::setAlternativeRedirect(%S)", fmt)
   
      UString format(U_CAPACITY), buffer(U_CAPACITY);
   
      format.snprintf(U_CTYPE_HTML "\r\nRefresh: 0; url=%s\r\n", fmt);
   
      va_list argp;
      va_start(argp, fmt);
   
      buffer.vsnprintf(format.data(), argp);
   
      va_end(argp);
   
      U_http_is_connection_close = U_YES;
      u_http_info.nResponseCode  = HTTP_MOVED_TEMP;
   
      UHTTP::setHTTPResponse(&buffer, 0);
   
      USSIPlugIn::alternative_response = 1;
   }
   
   static void checkBannerDefault()
   {
      U_TRACE(5, "::checkBannerDefault()")
   
      U_INTERNAL_ASSERT_POINTER(help_url)
      U_INTERNAL_ASSERT_POINTER(wallet_url)
      U_INTERNAL_ASSERT_POINTER(url_banner_ap)
      U_INTERNAL_ASSERT_POINTER(url_banner_comune)
   
      const char* ptr;
      UString banner(U_CAPACITY), x(U_CAPACITY);
   
      if (url_banner_ap_default)
         {
         U_ASSERT(url_banner_ap_default->dir())
   
         x.snprintf("$DIR_WEB/" VIRTUAL_HOST "%s/%s", url_banner_ap->data(), ap_ref->data());
   
         banner = UStringExt::expandPath(x, USSIPlugIn::environment);
         ptr    = banner.data();
   
         if (UFile::access(ptr) ||
             UFile::_mkdir(ptr))
            {
            (void) banner.append(U_CONSTANT_TO_PARAM("/default"));
   
            ptr = banner.data();
   
            if (UFile::access(ptr) == false) (void) url_banner_ap_default->symlink(ptr);
            }
         }
   
      if (url_banner_comune_default)
         {
         U_ASSERT(url_banner_comune_default->dir())
   
         x.snprintf("$DIR_WEB/" VIRTUAL_HOST "%s/%s", url_banner_comune->data(), ap_ref->data());
   
         banner = UStringExt::expandPath(x, USSIPlugIn::environment);
         ptr    = banner.data();
   
         if (UFile::access(ptr) ||
             UFile::_mkdir(ptr))
            {
            (void) banner.append(U_CONSTANT_TO_PARAM("/default"));
   
            ptr = banner.data();
   
            if (UFile::access(ptr) == false) (void) url_banner_comune_default->symlink(ptr);
            }
         }
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
   
   *request_uri    = UHTTP::getRequestURI(false);
   *client_address = UServer_Base::getClientAddress();
   
   if (UHTTP::isHttpGET())
      {
      switch (GET_request->findSorted(*request_uri))
         {
         case GET_admin:
            {
            setAlternativeRedirect("https://%.*s/admin.html", U_STRING_TO_TRACE(*ip_server));
            }
         break;
   
         case GET_card_activation:
            {
            if (*ip_server == *client_address)
               {
               }
            }
         break;
   
         case GET_error_ap:
            {
            // $1 -> ap (with localization => '@')
            // $2 -> public address to contact the access point
   
            if (setAccessPoint(true, false))
               {
               ULog::logger(portal_name->data(), priority,
                              "%.*s: *** ON AP %.*s FIREWALL IS NOT ALIGNED ***",
                              U_STRING_TO_TRACE(*request_uri), U_STRING_TO_TRACE(*ap));
               }
   
            body->clear();
   
            setAlternativeResponse();
            }
         break;
   
         case GET_get_config:
            {
            // $1 -> ap (without localization => '@')
            // $2 -> key
   
            setAccessPointReference(false);
   
            UFileConfig cfg;
            UString pathname(U_CAPACITY);
   
            pathname.snprintf("%w/ap/%.*s/nodog.conf", U_STRING_TO_TRACE(*ap_ref));
   
            if (cfg.open(pathname)) *body = cfg.getData();
            else
               {
               pathname.snprintf("%w/ap/default/nodog.conf", 0);
   
               *body = UFile::contentOf(pathname);
               }
   
            setAlternativeResponse();
            }
         break;
   
         case GET_get_users_info:
            {
            if (*ip_server == *client_address)
               {
               }
            }
         break;
   
         case GET_info:
         break;
   
         case GET_logged:
            {
            uid->clear();
   
            if (check_if_user_is_connected())
               {
               *ap = user_rec->getAP();
   
               setAlternativeInclude(0, 0, 0, "logged.tmpl",
                                       url_banner_ap->data(),
                                       help_url->data(), wallet_url->data(),
                                       ap->data(), "/logged_login_request",
                                       url_banner_comune->data());
               }
            else
               {
               setAlternativeRedirect("http://www.google.com", 0);
               }
            }
         break;
   
         case GET_login:
            {
            // -----------------------------------------------------------------------------------------------------------------------------------------------
            // GET /login?mac=00%3A14%3AA5%3A6E%3A9C%3ACB&ip=192.168.226.2&redirect=http%3A%2F%2Fgoogle&gateway=192.168.226.1%3A5280&timeout=0&token=x&ap=lab2
            // -----------------------------------------------------------------------------------------------------------------------------------------------
            // $1 -> mac
            // $2 -> ip
            // $3 -> redirect
            // $4 -> gateway
            // $5 -> timeout
            // $6 -> token
            // $7 -> ap (with localization => '@')
            // -----------------------------------------------------------------------------
            // 00:e0:4c:d4:63:f5 10.30.1.105 http://google 10.30.1.131:5280 0 x ap@lab2
            // -----------------------------------------------------------------------------
   
            setAccessPointReference(true);
   
            checkBannerDefault();
   
            UString request(U_CAPACITY);
   
            request.snprintf("/login_request?%.*s", U_HTTP_QUERY_TO_TRACE);
   
            setAlternativeInclude(0, 0, 0, "login.tmpl",
                                    url_banner_ap->data(), ap_ref->data(),
                                    help_url->data(), wallet_url->data(),
                                    ap->data(), request.data(),
                                    url_banner_comune->data(), ap_ref->data());
            }
         break;
   
         case GET_login_request:
            {
            // $1 -> mac
            // $2 -> ip
            // $3 -> redirect
            // $4 -> gateway
            // $5 -> timeout
            // $6 -> token
            // $7 -> ap (with localization => '@')
   
            UHTTP::processHTTPForm();
   
            UString mac, ip, redirect, gateway, timeout, token;
   
            UHTTP::getFormValue(mac,      1);
            UHTTP::getFormValue(ip,       3);
            UHTTP::getFormValue(redirect, 5);
            UHTTP::getFormValue(gateway,  7);
            UHTTP::getFormValue(timeout,  9);
            UHTTP::getFormValue(token,   11);
            UHTTP::getFormValue(*ap,     13);
   
            setAlternativeInclude(0, 0, 0, "login_request.tmpl", login_url->data(),
                                  mac.c_str(), ip.c_str(), redirect.c_str(), gateway.c_str(), timeout.c_str(), token.c_str(), ap->c_str());
            }
         break;
   
         case GET_login_validate:
         break;
   
         case GET_logout:
            {
            uid->clear();
   
            if (check_if_user_is_connected() == false) setMessagePage("Utente non connesso", "Utente non connesso");
            else
               {
               // TODO: logout_user
   
               setAlternativeInclude(0, 0, 0, "ringraziamenti.tmpl", uid->data(), user_rec->time_available / 60, user_rec->traffic_available / (1024 * 1024));
               }
            }
         break;
   
         case GET_logout_page:
            {
            setAlternativeInclude(0, 0, 0, "logout_page.tmpl", 0);
            }
         break;
   
         case GET_polling_attivazione:
         break;
   
         case GET_postlogin:
            {
            UHTTP::processHTTPForm();
   
            uint32_t num_args = UHTTP::form_name_value->size() / 2;
   
            U_INTERNAL_DUMP("num_args = %u", num_args)
   
            switch (num_args)
               {
               case 9:
                  {
                  // $1 -> uid
                  // $2 -> gateway
                  // $3 -> redirect
                  // $4 -> ap
                  // $5 -> ip
                  // $6 -> mac
                  // $7 -> timeout
                  // $8 -> traffic
                  // $9 -> auth_domain
                  }
               break;
   
               case 2:
                  {
                  // ----------------------------
                  // $1 -> uid
                  // $2 -> gateway
                  // ----------------------------
   
                  UHTTP::getFormValue(*uid, 1);
   
                  U_http_is_connection_close = U_YES;
   
                  setAlternativeInclude("Logout popup", 0, 0, "logout_popup.tmpl", uid->data(), uid->data());
                  }
               break;
               }
            }
         break;
   
         case GET_recovery:
            {
            if (*ip_server == *client_address)
               {
               }
            }
         break;
   
         case GET_registrazione:
            {
            // $1 -> ap (with localization => '@')
   
            U_http_is_connection_close = U_YES;
   
            setAlternativeInclude("Registrazione utente",
                                  "<link type=\"text/css\" href=\"css/livevalidation.css\" rel=\"stylesheet\">"
                                  "<script type=\"text/javascript\" src=\"js/livevalidation_standalone.compressed.js\"></script>",
                                  0, "registrazione.tmpl", registrazione_url->data(), tutela_dati->data());
            }
         break;
   
         case GET_reset_policy:
            {
            if (*ip_server == *client_address)
               {
               }
            }
         break;
   
         case GET_start_ap:
            {
            // $1 -> ap (with localization => '@')
            // $2 -> public address to contact the access point
            // $3 -> pid (0 => start)
   
            (void) setAccessPoint(true, true);
   
            *body = *allowed_web_hosts;
   
            setAlternativeResponse();
            }
         break;
   
         case GET_stato_utente:
         break;
   
         case GET_status_ap:
            {
            // $1 -> ap (with localization => '@')
            // $2 -> public address to contact the access point
   
            if (virtual_name->equal(U_HTTP_VHOST_TO_PARAM) &&
                setAccessPoint(true, false))
               {
               }
            }
         break;
   
         case GET_status_network:
            {
            if (*ip_server == *client_address)
               {
               }
            }
         break;
   
         case GET_unifi:
            {
            setAlternativeInclude(0, 0, 0, "unifi_page.tmpl",
                                       url_banner_ap->data(),
                                       help_url->data(), wallet_url->data(),
                                       "unifi", "/unifi_login_request",
                                       url_banner_comune->data());
            }
         break;
   
         case GET_unifi_login_request:
            {
            setAlternativeInclude(0, 0, 0, "unifi_login_request.tmpl", 0);
            }
         break;
   
         case GET_view_user:
            {
            if (*ip_server == *client_address)
               {
               }
            }
         break;
   
         case GET_webif_ap:
            {
            // $1 -> ap (with localization => '@')
            // $2 -> public address to contact the access point
   
            if (virtual_name->equal(U_HTTP_VHOST_TO_PARAM) &&
                setAccessPoint(true, false))
               {
               UString dest(U_CAPACITY);
   
               dest.snprintf("%.*s/client/%s:%u.srv", U_STRING_TO_TRACE(*dir_root), client_address->data(), UHTTP::getUserAgent());
   
               (void) UFile::writeTo(dest, *address);
   
               setAlternativeRedirect("http://" VIRTUAL_HOST "/cgi-bin/webif/status-basic.sh?cat=Status", 0);
               }
            }
         break;
   
         default:
            u_http_info.nResponseCode = HTTP_BAD_REQUEST;
         break;
         }
      }
   else if (UHTTP::isHttpPOST())
      {
      switch (POST_request->contains(*request_uri))
         {
         case POST_login_request:
            {
            // $1  -> mac
            // $2  -> ip
            // $3  -> redirect
            // $4  -> gateway
            // $5  -> timeout
            // $6  -> token
            // $7  -> ap (with localization => '@')
            // $8  -> realm
            // $9  -> uid
            // $10 -> password
            // $11 -> bottone
   
            UHTTP::processHTTPForm();
   
            UString mac, ip, redirect, gateway, timeout, token, realm, password;
   
            UHTTP::getFormValue(mac,       1);
            UHTTP::getFormValue(ip,        3);
            UHTTP::getFormValue(redirect,  5);
            UHTTP::getFormValue(gateway,   7);
            UHTTP::getFormValue(timeout,   9);
            UHTTP::getFormValue(token,    11);
            UHTTP::getFormValue(*ap,      13);
            UHTTP::getFormValue(realm,    15);
            UHTTP::getFormValue(*uid,     17);
            UHTTP::getFormValue(password, 19);
   
                 if (realm != "auth_service")          setMessagePage("Errore", "Errore Autorizzazione - dominio sconosciuto");
            else if (uid->empty() || password.empty()) setMessagePage("Impostare utente e/o password" "Impostare utente e/o password"); 
            else
               {
               // TODO: login
               }
            }
         break;
   
         case POST_logout:
            {
            // ----------------------------
            // $1 -> uid
            // ----------------------------
   
            UHTTP::processHTTPForm();
   
            UHTTP::getFormValue(*uid, 1);
   
            if (check_if_user_is_connected() == false) setMessagePage("Utente non connesso", "Utente non connesso");
            else
               {
               // TODO: logout_user
   
               U_http_is_connection_close = U_YES;
   
               setAlternativeInclude(0, "<script type=\"text/javascript\" src=\"js/logout_popup.js\"></script>",
                                     "'onload=\"CloseItAfterSomeTime()\"'", "logout_notify.tmpl", uid->data());
               }
            }
         break;
   
         case POST_registrazione:
            {
            // ----------------------------
            // $1  -> nome
            // $2  -> cognome
            // $3  -> luogo_di_nascita
            // $4  -> data_di_nascita
            // $5  -> email
            // $6  -> cellulare_prefisso
            // $7  -> telefono_cellulare
            // $8  -> password
            // $9  -> password_conferma
            // $10 -> submit
            // ----------------------------
   
            UHTTP::processHTTPForm();
   
            UString nome, cognome, luogo_di_nascita, data_di_nascita, email,
                    cellulare_prefisso, telefono_cellulare, password, password_conferma;
   
            UHTTP::getFormValue(nome,                 1);
            UHTTP::getFormValue(cognome,              3);
            UHTTP::getFormValue(luogo_di_nascita,     5);
            UHTTP::getFormValue(data_di_nascita,      7);
            UHTTP::getFormValue(email,                9);
            UHTTP::getFormValue(cellulare_prefisso,  11);
            UHTTP::getFormValue(telefono_cellulare,  13);
            UHTTP::getFormValue(password,            15);
            UHTTP::getFormValue(password_conferma,   17);
   
            if (password != password_conferma) setMessagePage("Conferma Password errata", "Conferma Password errata");
            else
               {
               // TODO: registrazione
   
               setAlternativeInclude("Registrazione effettuata", 0, 0, "post_registrazione.tmpl",
                                     caller_id->data(), password.data(), "polling_attivazione", caller_id->data(), password.data());
               }
            }
         break;
   
         case POST_uploader:
            {
            // $1 -> path file uploaded
   
            UHTTP::processHTTPForm();
   
            UString content, source(100U);
   
            UHTTP::getFormValue(source, 1);
   
            content = UFile::contentOf(source);
   
            U_INTERNAL_ASSERT(source.isNullTerminated())
   
            (void) UFile::_unlink(source.data());
   
            if (content.size() > (2 * 1024))
               {
               UString dest(U_CAPACITY), basename = UStringExt::basename(source);
   
               dest.snprintf("%.*s/%.*s", U_STRING_TO_TRACE(*historical_log_dir), U_STRING_TO_TRACE(basename));
   
               (void) UFile::writeTo(dest, content);
               }
   
            body->clear();
   
            setAlternativeResponse();
            }
         break;
   
         default:
            u_http_info.nResponseCode = HTTP_BAD_REQUEST;
         break;
         }
      }
   else
      {
      u_http_info.nResponseCode = HTTP_BAD_METHOD;
      }
   
   U_INTERNAL_DUMP("u_http_info.nResponseCode = %d", u_http_info.nResponseCode)
   
   if (u_http_info.nResponseCode == 0) (void) UClientImage_Base::environment->append(U_CONSTANT_TO_PARAM("HTTP_RESPONSE_CODE=0\n"));
   
   return 0;
   
   U_RETURN(usp_as_service ? 0 : 200);
} }