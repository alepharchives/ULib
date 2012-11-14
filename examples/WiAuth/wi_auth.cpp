// wi_auth.cpp
   
#include <ulib/net/server/usp_macro.h>
   
#include <ulib/net/server/plugin/mod_ssi.h>
   
   static UString* ap;
   static UString* ip;
   static UString* uid;
   static UString* mac;
   static UString* token;
   static UString* label;
   static UString* buffer;
   static UString* policy;
   static UString* ap_ref;
   static UString* output;
   static UString* address;
   static UString* gateway;
   static UString* dir_reg;
   static UString* account;
   static UString* telefono;
   static UString* dir_root;
   static UString* mac_auth;
   static UString* hostname;
   static UString* padding1;
   static UString* padding2;
   static UString* cert_auth;
   static UString* ip_server;
   static UString* login_url;
   static UString* empty_str;
   static UString* empty_list;
   static UString* nodog_conf;
   static UString* policy_flat;
   static UString* environment;
   static UString* auth_domain;
   static UString* account_auth;
   static UString* request_uri;
   static UString* client_address;
   static UString* virtual_name;
   static UString* ldap_card_param;
   static UString* ldap_user_param;
   static UString* redirect_default;
   static UString* registrazione_url;
   static UString* allowed_web_hosts;
   static UString* historical_log_dir;
   static UString* wiauth_card_basedn;
   static UString* wiauth_user_basedn;
   static UString* message_page_template;
   static UString* status_nodog_template;
   static UString* status_network_template;
   
   static UString* help_url;
   static UString* wallet_url;
   static UString* fmt_auth_cmd;
   static UString* url_banner_ap;
   static UString* url_banner_comune;
   
   static UString* _time_chunk;
   static UString* _time_consumed;
   static UString* _time_available;
   static UString* _traffic_chunk;
   static UString* _traffic_consumed;
   static UString* _traffic_available;
   
   static UString* user_UploadRate;
   static UString* user_DownloadRate;
   
   static UFile* file_LOG;
   static UFile* file_WARNING;
   static UFile* file_RECOVERY;
   static UFile* url_banner_ap_default;
   static UFile* url_banner_comune_default;
   
   static const char* cptr;
   static const char* ptr1;
   static const char* ptr2;
   static const char* ptr3;
   static const char* ptr4;
   
   static UCache*                  cache;
   static URDB*                    db_ap;
   static URDB*                    db_user;
   static UVector<UString>*        vec;
   static UHashMap<UString>*       table;
   static UHttpClient<UTCPSocket>* client;
   
   static bool     user_exist, check_user_agent;
   static uint32_t csize, lindex, num_ap, _num_ap, num_user, _num_user, users_connected;
   
   class WiAuthUser;
   class WiAuthNodog;
   class WiAuthAccessPoint;
   
   static WiAuthUser*         user_rec;
   static WiAuthNodog*       nodog_rec;
   static WiAuthAccessPoint*    ap_rec;
   
   #define U_LOGGER(fmt,args...) u_printf2(file_WARNING->getFd(), "%6D %.*s: " fmt "\n", U_STRING_TO_TRACE(*request_uri) , ##args)
   
   #define NUM_ACCESS_POINT_ATTRIBUTE 4
   
   static bool askNodogToLogoutUser();
   static void writeToLOG(const char* op);
   static void loadPolicy(const UString& policy);
   
   class WiAuthAccessPoint {
   public:
      // Check for memory error
      U_MEMORY_TEST
   
      // Allocator e Deallocator
      U_MEMORY_ALLOCATOR
      U_MEMORY_DEALLOCATOR
   
      UString value, _label, mac_mask, group_account;
      bool noconsume;
   
      // COSTRUTTORE
   
      WiAuthAccessPoint() : value(U_CAPACITY)
         {
         U_TRACE_REGISTER_OBJECT(5, WiAuthAccessPoint, "")
         }
   
      ~WiAuthAccessPoint()
         {
         U_TRACE_UNREGISTER_OBJECT(5, WiAuthAccessPoint)
         }
   
      // SERVICES
   
      void clear()
         {
         U_TRACE(5, "WiAuthAccessPoint::clear()")
   
                 value.clear();
                _label.clear();
              mac_mask.clear();
         group_account.clear();
         }
   
      UString toString()
         {
         U_TRACE(5, "WiAuthAccessPoint::toString()")
   
         U_ASSERT_EQUALS(_label.empty(), false)
   
         value.snprintf(" %u %.*s \"%.*s\" \"%.*s\"",
                        noconsume,
                        U_STRING_TO_TRACE(_label),
                        U_STRING_TO_TRACE(group_account),
                        U_STRING_TO_TRACE(mac_mask));
   
         U_RETURN_STRING(value);
         }
   
      void fromVector(const UVector<UString>& _vec, uint32_t _index)
         {
         U_TRACE(5, "WiAuthAccessPoint::fromVector(%p,%u)", &_vec, _index)
   
         U_ASSERT_EQUALS(_vec.empty(), false)
   
         noconsume         = (_vec[_index].strtol() == 1);
         _label            =  _vec[_index+1];
   
         UString _account  =  _vec[_index+2],
                 _mac_mask =  _vec[_index+3];
   
              mac_mask.clear();
         group_account.clear();
   
         if (_account  != *empty_str) group_account = _account;
         if (_mac_mask != *empty_str) mac_mask      = _mac_mask;
         }
   
   #ifdef DEBUG
      const char* dump(bool breset) const
         {
         *UObjectIO::os << "noconsume              " << noconsume             << '\n'
                        << "value         (UString " << (void*)&value         << ")\n"
                        << "_label        (UString " << (void*)&_label        << ")\n"
                        << "mac_mask      (UString " << (void*)&mac_mask      << ")\n"
                        << "group_account (UString " << (void*)&group_account << ')';
   
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
   
   static bool db_store(URDB* db, int op, const UString& key, const UString& new_record, const UString& old_record, const UString& padding)
   {
      U_TRACE(5, "::db_store(%p,%d,%.*S,%.*S)", db, op, U_STRING_TO_TRACE(key), U_STRING_TO_TRACE(new_record),
                                                        U_STRING_TO_TRACE(old_record), U_STRING_TO_TRACE(padding))
   
      int result = db->store(key, new_record, old_record, padding, op);
   
      if (result)
         {
         U_SRV_LOG("store with flag %d on db %.*S failed with error %d", op, U_FILE_TO_TRACE(*db), result);
   
         U_RETURN(false);
         }
   
      U_RETURN(true);
   }
   
   class WiAuthNodog {
   public:
      // Check for memory error
      U_MEMORY_TEST
   
      // Allocator e Deallocator
      U_MEMORY_ALLOCATOR
      U_MEMORY_DEALLOCATOR
   
      time_t last_info;
      UString value, _hostname;
      UVector<UString> access_point;
      int port;
      bool down;
   
   /*
   interface Coordinates {
     readonly attribute double latitude;
     readonly attribute double longitude;
     readonly attribute double? altitude;
     readonly attribute double accuracy;
     readonly attribute double? altitudeAccuracy;
     readonly attribute double? heading;
     readonly attribute double? speed;
   
   The latitude and longitude attributes are geographic coordinates specified in decimal degrees.
   
   The altitude attribute denotes the height of the position, specified in meters above the [WGS84] ellipsoid. If the implementation cannot provide altitude information, the value of this attribute must be null.
   
   The accuracy attribute denotes the accuracy level of the latitude and longitude coordinates. It is specified in meters and must be supported by all implementations. The value of the accuracy attribute must be a non-negative real number.
   
   The altitudeAccuracy attribute is specified in meters. If the implementation cannot provide altitude information, the value of this attribute must be null. Otherwise, the value of the altitudeAccuracy attribute must be a non-negative real number.
   
   The accuracy and altitudeAccuracy values returned by an implementation should correspond to a 95% confidence level.
   
   The heading attribute denotes the direction of travel of the hosting device and is specified in degrees, where 0° ≤ heading < 360°, counting clockwise relative to the true north. If the implementation cannot provide heading information, the value of this attribute must be null. If the hosting device is stationary (i.e. the value of the speed attribute is 0), then the value of the heading attribute must be NaN.
   
   The speed attribute denotes the magnitude of the horizontal component of the hosting device's current velocity and is specified in meters per second. If the implementation cannot provide speed information, the value of this attribute must be null. Otherwise, the value of the speed attribute must be a non-negative real number.
   };
   */
   
      // COSTRUTTORE
   
      WiAuthNodog() : value(U_CAPACITY)
         {
         U_TRACE_REGISTER_OBJECT(5, WiAuthNodog, "")
         }
   
      ~WiAuthNodog()
         {
         U_TRACE_UNREGISTER_OBJECT(5, WiAuthNodog)
         }
   
      // SERVICES
   
      void clear()
         {
         U_TRACE(5, "WiAuthNodog::clear()")
   
                value.clear();
            _hostname.clear();
         access_point.clear();
         }
   
      void callForAllAccessPoint(bPF func)
         {
         U_TRACE(5, "WiAuthNodog::callForAllAccessPoint(%p)", func)
   
         U_ASSERT_EQUALS(access_point.empty(), false)
   
         lindex = U_NOT_FOUND;
   
         for (int32_t i = 0, n = access_point.size(); i < n; i += NUM_ACCESS_POINT_ATTRIBUTE)
            {
            U_INTERNAL_ASSERT_MAJOR(n, 0)
   
            ap_rec->fromVector(access_point, i);
   
            if (func() == false)
               {
               lindex = i;
   
               break;
               }
            }
         }
   
      static bool checkLabel()
         {
         U_TRACE(5, "WiAuthNodog::checkLabel()")
   
         if (*label == ap_rec->_label) U_RETURN(false);
   
         U_RETURN(true);
         }
   
      uint32_t findLabel()
         {
         U_TRACE(5, "WiAuthNodog::findLabel()")
   
         U_INTERNAL_DUMP("label = %.*S", U_STRING_TO_TRACE(*label))
   
         U_ASSERT_EQUALS(label->empty(), false)
   
         callForAllAccessPoint(checkLabel);
   
         if (lindex == U_NOT_FOUND)
            {
            U_LOGGER("*** LABEL(%.*s) NOT EXISTENT ON AP(%.*s) ***", U_STRING_TO_TRACE(*label), U_STRING_TO_TRACE(_hostname));
            }
   
         U_RETURN(lindex);
         }
   
      static bool checkMAC()
         {
         U_TRACE(5, "WiAuthNodog::checkMAC()")
   
         if (ap_rec->mac_mask.empty() == false &&
             UServices::dosMatchWithOR(*mac, ap_rec->mac_mask, FNM_IGNORECASE))
            {
            U_RETURN(false);
            }
   
         U_RETURN(true);
         }
   
      bool findMAC()
         {
         U_TRACE(5, "WiAuthNodog::findMAC()")
   
         U_INTERNAL_DUMP("mac = %.*S", U_STRING_TO_TRACE(*mac))
   
         U_ASSERT_EQUALS(mac->empty(), false)
   
         if (findLabel() != U_NOT_FOUND)
            {
            callForAllAccessPoint(checkMAC);
   
            if (lindex != U_NOT_FOUND) U_RETURN(true);
            }
   
         U_RETURN(false);
         }
   
      static bool getAccessPointToString()
         {
         U_TRACE(5, "WiAuthNodog::getAccessPointToString()")
   
         (void) buffer->append(ap_rec->toString());
   
         U_RETURN(true);
         }
   
      UString toString()
         {
         U_TRACE(5, "WiAuthNodog::toString()")
   
         buffer->setBuffer(U_CAPACITY);
   
         buffer->snprintf("%u %10ld %u %.*s [",
                           down,
                           last_info,
                           port,
                           U_STRING_TO_TRACE(_hostname));
   
         callForAllAccessPoint(getAccessPointToString);
   
         (void) buffer->append(U_CONSTANT_TO_PARAM(" ]"));
   
         U_RETURN_STRING(*buffer);
         }
   
      void fromString()
         {
         U_TRACE(5, "WiAuthNodog::fromString()")
   
         U_ASSERT_EQUALS(value.empty(), false)
   
         istrstream is(value.data(), value.size());
   
         is >> down
            >> last_info
            >> port;
   
         is.get(); // skip ' '
   
         _hostname.get(is);
   
         U_INTERNAL_ASSERT_EQUALS(_hostname.empty(), false)
   
         is.get(); // skip ' '
         is.get(); // skip '['
   
         access_point.clear();
         access_point.readVector(is, ']');
   
         U_INTERNAL_ASSERT_EQUALS(access_point.empty(), false)
         }
   
      void setLastInfo()
         {
         U_TRACE(5, "WiAuthNodog::setLastInfo()")
   
         char* _ptr = value.c_pointer(2);
   
         U_INTERNAL_DUMP("_ptr = %.40s", _ptr)
   
         (void) u__snprintf(_ptr, 10, "%10ld", u_now->tv_sec); // last_info
   
         U_INTERNAL_ASSERT(u__isspace(_ptr[10]))
         }
   
      void setDown(bool bdown)
         {
         U_TRACE(5, "WiAuthNodog::setDown(%b)", bdown)
   
         char c     = (bdown ? '1' : '0');
         char* _ptr = value.c_pointer(0);
   
         U_INTERNAL_DUMP("_ptr = %.40s", _ptr)
   
         U_INTERNAL_ASSERT(u__isspace(_ptr[1]))
   
      // char old = _ptr[0];
                    _ptr[0] = c;
   
         /*
         if ((down          && old != '1') ||
             (down == false && old != '0'))
            {
            U_LOGGER("*** setDown(%b) AP(%.*s@%.*s) WITH DIFFERENT STATE ***", bdown, U_STRING_TO_TRACE(*label), U_STRING_TO_TRACE(*address));
            }
         */
   
         down = bdown;
   
         setLastInfo();
         }
   
      bool setValue(const UString& _address)
         {
         U_TRACE(5, "WiAuthNodog::setValue(%.*S)", U_STRING_TO_TRACE(_address))
   
         U_ASSERT_EQUALS(_address.empty(), false)
   
         value = (*db_ap)[_address];
   
         if (value.empty() == false)
            {
            U_INTERNAL_ASSERT(u__isspace(value.c_char(1)))
   
            fromString();
   
            U_RETURN(true);
            }
   
         U_RETURN(false);
         }
   
      bool setRecord(uint32_t* pindex, int _port = 5280, bool bdown = false, bool bnoconsume = false, bool bgroup_account = false, UString* mac_mask = 0)
         {
         U_TRACE(5, "WiAuthNodog::setRecord(%p,%d,%b,%b,%b,%p)", pindex, _port, bdown, bnoconsume, bgroup_account, mac_mask)
   
         int op      = RDB_REPLACE;
         bool add_ap = false;
   
         if (setValue(*address) == false)
            {
            U_ASSERT((*db_ap)[*address].empty())
   
            down   = bdown;
            port   = _port;
   
            op     = RDB_INSERT;
            add_ap = true;
            lindex = 0;
   
            access_point.clear();
   
            if (   label->empty()) *label    = U_STRING_FROM_CONSTANT("ap");
            if (hostname->empty()) *hostname = U_STRING_FROM_CONSTANT("empty");
            }
         else
            {
                 if ( hostname->empty()) *hostname = _hostname;
            else if (*hostname != _hostname)
               {
               U_LOGGER("*** AP HOSTNAME (%.*s) NOT EQUAL (%.*s) ***", U_STRING_TO_TRACE(_hostname), U_STRING_TO_TRACE(*hostname));
               }
   
            if (label->empty()) lindex = U_NOT_FOUND;
            else
               {
               callForAllAccessPoint(checkLabel);
   
               if (lindex == U_NOT_FOUND) add_ap = true;
               }
   
            U_INTERNAL_DUMP("lindex = %u", lindex)
            }
   
         _hostname = *hostname;
         last_info = u_now->tv_sec;
   
         if (add_ap == false)
            {
            down = bdown;
   
            if (pindex) *pindex = lindex;
            else
               {
               // edit_ap
   
               if (lindex != U_NOT_FOUND)
                  {
                  U_INTERNAL_ASSERT_EQUALS(pindex, 0)
   
                  access_point.replace(lindex, bnoconsume ? U_STRING_FROM_CONSTANT("1")
                                                          : U_STRING_FROM_CONSTANT("0"));
   
                  access_point.replace(lindex+2, bgroup_account ? *account  : *empty_str);
                  access_point.replace(lindex+3, mac_mask       ? *mac_mask : *empty_str);
                  }
               }
            }
         else
            {
            // add ap
   
            U_ASSERT_EQUALS(label->empty(), false)
   
            ++num_ap;
   
            if (pindex) *pindex = access_point.size();
   
            access_point.push_back(bnoconsume ? U_STRING_FROM_CONSTANT("1")
                                              : U_STRING_FROM_CONSTANT("0"));
   
            access_point.push_back(*label);
   
            access_point.push_back(bgroup_account ? *account  : *empty_str);
            access_point.push_back(mac_mask       ? *mac_mask : *empty_str);
            }
   
         U_INTERNAL_DUMP("pindex = %u", (pindex ? *pindex : U_NOT_FOUND))
   
         if (db_store(db_ap, op, *address, toString(), value, *padding1))
            {
            if (op == RDB_INSERT) value = (*db_ap)[*address];
   
            U_RETURN(true);
            }
   
         U_RETURN(false);
         }
   
   #ifdef DEBUG
      const char* dump(bool breset) const
         {
         *UObjectIO::os << "down                           " << down                 << '\n'
                        << "port                           " << port                 << '\n'
                        << "last_info                      " << last_info            << '\n'
                        << "value        (UString          " << (void*)&value        << ")\n"
                        << "_hostname    (UString>         " << (void*)&_hostname    << ")\n"
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
   
   static UString get_UserName()
   {
      U_TRACE(5, "::get_UserName()")
   
      U_ASSERT_EQUALS(uid->empty(), false)
   
      UString pathname(U_CAPACITY), content, user;
   
      pathname.snprintf("%.*s/%.*s.reg", U_STRING_TO_TRACE(*dir_reg), U_STRING_TO_TRACE(*uid));
   
      content = UFile::contentOf(pathname);
   
      if (content.empty()) user = U_STRING_FROM_CONSTANT("anonymous");
      else
         {
         if (vec->split(content) > 2) user = (*vec)[0] + ' ' + (*vec)[1];
             vec->clear();
         }
   
      U_RETURN_STRING(user);
   }
   
   class WiAuthUser {
   public:
      // Check for memory error
      U_MEMORY_TEST
   
      // Allocator e Deallocator
      U_MEMORY_ALLOCATOR
      U_MEMORY_DEALLOCATOR
   
      uint64_t traffic_done, traffic_available, traffic_consumed;
      time_t login_time, last_modified, time_done, time_available, time_consumed;
      UString value, _ip, _auth_domain, _mac, _policy, nodog, _user;
      uint32_t index_access_point, agent, DownloadRate, UploadRate;
      bool connected, consume;
   
      // COSTRUTTORE
   
      WiAuthUser() : value(U_CAPACITY)
         {
         U_TRACE_REGISTER_OBJECT(5, WiAuthUser, "")
   
         consume            = true;
         connected          = false;
         time_done          = time_available = time_consumed = login_time = last_modified = 0;
         traffic_done       = traffic_available = traffic_consumed = 0;
         index_access_point = agent = DownloadRate = UploadRate = 0;
         }
   
      ~WiAuthUser()
         {
         U_TRACE_UNREGISTER_OBJECT(5, WiAuthUser)
         }
   
      // SERVICES
   
      void clear()
         {
         U_TRACE(5, "WiAuthUser::clear()")
   
                  _ip.clear();
                 _mac.clear();
                _user.clear();
                value.clear();
                nodog.clear();
              _policy.clear();
         _auth_domain.clear();
         }
   
      UString toString()
         {
         U_TRACE(5, "WiAuthUser::toString()")
   
         U_INTERNAL_ASSERT_EQUALS(nodog.empty(), false)
   
         UString x(U_CAPACITY);
   
         x.snprintf("%.*s %u "
                    "%10ld %10ld "
                    "%8ld %15llu "
                    "%10ld %10ld %12llu %12llu "
                    "%u %u %u %u %u "
                    "%.*s %.*s %.*s %.*s \"%.*s\"",
                    U_STRING_TO_TRACE(_ip), connected,
                    last_modified, login_time,
                    time_consumed, traffic_consumed,
                    time_done, time_available, traffic_done, traffic_available,
                    index_access_point, consume, agent, DownloadRate, UploadRate,
                    U_STRING_TO_TRACE(_auth_domain),
                    U_STRING_TO_TRACE(_mac),
                    U_STRING_TO_TRACE(_policy),
                    U_STRING_TO_TRACE(nodog),
                    U_STRING_TO_TRACE(_user));
   
         U_RETURN_STRING(x);
         }
   
      void setConnected(bool bconnected)
         {
         U_TRACE(5, "WiAuthUser::setConnected(%b)", bconnected)
   
         char c     = (bconnected ? '1' : '0');
         char* _ptr = value.c_pointer(_ip.size());
   
         U_INTERNAL_DUMP("_ptr = %.40s", _ptr)
   
         U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
   
         char old = _ptr[1];
                    _ptr[1] = c;
   
         if ((connected          && old != '1') ||
             (connected == false && old != '0'))
            {
            U_LOGGER("*** setConnected(%b) UID(%.*s) IP(%.*s) MAC(%.*s) AP(%.*s@%.*s) WITH DIFFERENT STATE ***",
                           bconnected, U_STRING_TO_TRACE(*uid), U_STRING_TO_TRACE(_ip), U_STRING_TO_TRACE(_mac),
                           U_STRING_TO_TRACE(*label), U_STRING_TO_TRACE(*address));
            }
   
         _ptr += 2;
   
         U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
   
         (void) u__snprintf(++_ptr, 10, "%10ld", u_now->tv_sec); // last_modified
   
         if (bconnected)
            {
         // if (connected == false) addConnection();
   
            connected = true;
   
            _ptr += 10;
   
            U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
   
            (void) u__snprintf(++_ptr, 10, "%10ld", u_now->tv_sec); // login_time
            }
         else
            {
         // if (connected) delConnection();
   
            connected = false;
            }
   
         U_INTERNAL_DUMP("num_user = %u connected = %u", num_user, users_connected)
         }
   
      void resetCounter()
         {
         U_TRACE(5, "WiAuthUser::resetCounter()")
   
         // 172.16.1.172 1 1346679913 1346679913        8            12736          4      7200         12736   314572800 
   
         char* _ptr = value.c_pointer(_ip.size() + 2 + 11 + 11 + 9 + 16); // connected + last_modified + login_time + time_consumed + traffic_consumed
   
         U_INTERNAL_DUMP("_ptr = %.40s", _ptr)
   
         (void) u__snprintf(++_ptr,   10, "%10ld", (time_done = 0));
                              _ptr += 10;
   
         U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
   
         _ptr += 11; // time_available
   
         U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
   
         (void) u__snprintf(++_ptr,   12, "%12llu", (traffic_done = 0));
                              _ptr += 12;
   
         U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
         }
   
      void updateCounter(time_t _logout, time_t _connected, uint64_t _traffic)
         {
         U_TRACE(5, "WiAuthUser::updateCounter(%ld,%ld,%llu)", _logout, _connected, _traffic)
   
         U_INTERNAL_ASSERT(connected)
         U_INTERNAL_ASSERT(user_exist)
         U_INTERNAL_ASSERT_MAJOR(last_modified, 0)
   
         char* _ptr = value.c_pointer(_ip.size());
   
         // 172.16.1.172 1 1346679913 1346679913        8            12736          4      7200         12736   314572800 
         // 0 1 3821975508 PASS_AUTH 00:14:a5:6e:9c:cb DAILY 10.8.1.2
   
         U_INTERNAL_DUMP("_ptr = %.40s", _ptr)
   
         U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
         U_INTERNAL_ASSERT_EQUALS(_ptr[1], '1')
   
         _ptr += 2;
   
         U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
   
         char* ptr_last_modified = ++_ptr; // last_modified
                                     _ptr += 10;
   
         U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
   
         _ptr += 11; // login_time
   
         U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
   
         if (_connected == 0) _ptr += 9; // time_consumed
         else
            {
            time_consumed += _connected;
   
            (void) u__snprintf(++_ptr,   8, "%8ld", time_consumed);
                                 _ptr += 8;
            }
   
         U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
   
         if (_traffic == 0) _ptr += 16; // traffic_consumed
         else
            {
            traffic_consumed += _traffic;
   
            (void) u__snprintf(++_ptr,   15, "%15llu", traffic_consumed);
                                 _ptr += 15;
            }
   
         U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
   
         bool ask_logout = false;
   
         if (consume)
            {
            if (_connected == 0) _ptr += 11; // time_done
            else
               {
               time_done += _connected;
   
               if (time_done > time_available)
                  {
                  ask_logout = true;
   
                  time_t time_diff = time_done - time_available;
   
                  U_LOGGER("*** updateCounter() UID(%.*s) IP(%.*s) MAC(%.*s) AP(%.*s@%.*s) EXCEED TIME AVAILABLE (%ld sec) ***",
                           U_STRING_TO_TRACE(*uid), U_STRING_TO_TRACE(_ip), U_STRING_TO_TRACE(_mac),
                           U_STRING_TO_TRACE(*label), U_STRING_TO_TRACE(*address), time_diff);
                  }
   
               (void) u__snprintf(++_ptr,   10, "%10ld", time_done);
                                    _ptr += 10;
               }
   
            U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
   
            _ptr += 11; // time_available
   
            U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
   
            if (_traffic == 0) _ptr += 13; // traffic_done
            else
               {
               traffic_done += _traffic;
   
               if (traffic_done > traffic_available)
                  {
                  ask_logout = true;
   
                  uint64_t traffic_diff = traffic_done - traffic_available;
   
                  U_LOGGER("*** updateCounter() UID(%.*s) IP(%.*s) MAC(%.*s) AP(%.*s@%.*s) EXCEED TRAFFIC AVAILABLE (%llu bytes) ***",
                           U_STRING_TO_TRACE(*uid), U_STRING_TO_TRACE(_ip), U_STRING_TO_TRACE(_mac),
                           U_STRING_TO_TRACE(*label), U_STRING_TO_TRACE(*address), traffic_diff);
                  }
   
               (void) u__snprintf(++_ptr,   12, "%12llu", traffic_done);
                                    _ptr += 12;
               }
   
            U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
   
            _ptr += 13; // traffic_available
   
            U_INTERNAL_DUMP("_ptr = %.40s", _ptr)
   
            U_INTERNAL_ASSERT(u__isspace(_ptr[0]))
            }
   
         if (_logout == 0) // NB: _logout == 0 mean NOT logout (only info)...
            {
            if (_traffic == 0)
               {
               U_LOGGER("*** INFO PARAM: UID(%.*s) IP(%.*s) MAC(%.*s) AP(%.*s@%.*s) NO TRAFFIC (%ld secs) ***",
                        U_STRING_TO_TRACE(*uid), U_STRING_TO_TRACE(_ip), U_STRING_TO_TRACE(_mac),
                        U_STRING_TO_TRACE(*label), U_STRING_TO_TRACE(*address), _connected);
   
               if (_connected >= 360 &&
                   _policy != *policy_flat)
                  {
                  ask_logout = true;
                  }
               }
            }
         else
            {
            setConnected(false);
   
            writeToLOG(_logout == -1 || _traffic == 0 ? "EXIT" : "LOGOUT"); // LOGOUT (-1 => implicito)
   
            return;
            }
   
         if (ask_logout) (void) askNodogToLogoutUser();
         else            (void) u__snprintf(ptr_last_modified, 10, "%10ld", u_now->tv_sec);
         }
   
      void fromString()
         {
         U_TRACE(5, "WiAuthUser::fromString()")
   
         U_ASSERT_EQUALS(value.empty(), false)
   
         istrstream is(value.data(), value.size());
   
         _ip.get(is);
   
         is.get(); // skip ' '
   
         is >> connected
            >> last_modified >> login_time
            >>    time_consumed
            >> traffic_consumed
            >>    time_done >>    time_available
            >> traffic_done >> traffic_available
            >> index_access_point >> consume >> agent >> DownloadRate >> UploadRate;
   
         is.get(); // skip ' '
   
         _auth_domain.get(is);
   
         is.get(); // skip ' '
   
         _mac.get(is);
   
         is.get(); // skip ' '
   
         _policy.get(is);
   
         is.get(); // skip ' '
   
         nodog.get(is);
   
         is.get(); // skip ' '
   
         _user.get(is);
   
         U_INTERNAL_ASSERT_EQUALS(_user.empty(), false)
         }
   
      void setValue()
         {
         U_TRACE(5, "WiAuthUser::setValue()")
   
         U_ASSERT_EQUALS(uid->empty(), false)
   
         value      = (*db_user)[*uid];
         user_exist = (value.empty() == false);
   
         if (user_exist)
            {
            U_INTERNAL_ASSERT(u__isdigit(value.c_char(0)))
   
            fromString();
            }
         }
   
      UString getLabelAP()
         {
         U_TRACE(5, "WiAuthUser::getLabelAP()")
   
         UString x = nodog_rec->access_point[index_access_point + 1];
   
         U_RETURN_STRING(x);
         }
   
      UString getAP()
         {
         U_TRACE(5, "WiAuthUser::getAP()")
   
         UString x(100U), _label = getLabelAP();
   
         x.snprintf("%.*s@%.*s:%u/%.*s", U_STRING_TO_TRACE(_label), U_STRING_TO_TRACE(nodog), nodog_rec->port, U_STRING_TO_TRACE(nodog_rec->_hostname));
   
         U_RETURN_STRING(x);
         }
   
      void setChunkValue()
         {
         U_TRACE(5, "WiAuthUser::setChunkValue()")
   
            _time_chunk->snprintf("%ld",     time_available -    time_done);
         _traffic_chunk->snprintf("%llu", traffic_available - traffic_done);
         }
   
      void getCounter()
         {
         U_TRACE(5, "WiAuthUser::getCounter()")
   
            _time_chunk->snprintf("%ld",  (   time_available -    time_done) /              60L);
         _traffic_chunk->snprintf("%llu", (traffic_available - traffic_done) / (1024ULL * 1024ULL));
         }
   
      void getConsumed()
         {
         U_TRACE(5, "WiAuthUser::getConsumed()")
   
            _time_consumed->snprintf("%ld",     time_consumed /              60L);
         _traffic_consumed->snprintf("%llu", traffic_consumed / (1024ULL * 1024ULL));
         }
   
      bool setNodogReference()
         {
         U_TRACE(5, "WiAuthUser::setNodogReference()")
   
         U_ASSERT_EQUALS( uid->empty(), false)
         U_ASSERT_EQUALS(nodog.empty(), false)
   
         UString x = (connected ? nodog : *address);
   
         U_ASSERT_EQUALS(x.empty(), false)
   
         if (check_user_agent &&
                        agent != UHTTP::getUserAgent())
            {
            U_ASSERT_DIFFERS(*client_address, x)
   
            U_LOGGER("*** UID(%.*s) BIND WITH AP(%.*s) HAS DIFFERENT AGENT ***", U_STRING_TO_TRACE(*uid), U_STRING_TO_TRACE(x));
            }
   
         if (nodog_rec->setValue(x))
            {
            if (connected) *address = nodog;
   
            U_RETURN(true);
            }
   
         setConnected(false);
   
         U_LOGGER("*** UID(%.*s) BIND WITH AP(%.*s) NOT REGISTERED ***", U_STRING_TO_TRACE(*uid), U_STRING_TO_TRACE(x));
   
         U_RETURN(false);
         }
   
      bool setRecord() // NB: it is called only from the context of login validation...
         {
         U_TRACE(5, "WiAuthUser::setRecord()")
   
         U_ASSERT_EQUALS(uid->empty(), false)
   
         if (nodog_rec->setRecord(&index_access_point) == false ||
                                   index_access_point  == U_NOT_FOUND)
            {
            U_RETURN(false);
            }
   
         nodog = *address;
   
         int op = RDB_REPLACE;
   
         U_INTERNAL_DUMP("user_exist = %b index_access_point = %u nodog = %.*S", user_exist, index_access_point, U_STRING_TO_TRACE(nodog))
   
         if (user_exist == false)
            {
            U_ASSERT((*db_user)[*uid].empty())
   
            op        = RDB_INSERT;
            connected = false;
   
               time_consumed = 0;
            traffic_consumed = 0;
   
            _user = get_UserName();
            }
   
         _ip           = *ip;
         _mac          = *mac;
         agent         = UHTTP::getUserAgent();
         consume       = (ap_rec->noconsume == false);
         login_time    = 0;
         last_modified = u_now->tv_sec;
   
           UploadRate  = user_UploadRate->strtol();
         DownloadRate  = user_DownloadRate->strtol();
   
         bool bflat;
   
         if (ap_rec->group_account.empty() == false &&
             UServices::dosMatchWithOR(*account, ap_rec->group_account))
            {
            bflat        = true;
            consume      = false;
            _auth_domain = *account_auth;
   
            loadPolicy(_policy = *policy_flat);
            }
         else
            {
            U_ASSERT_EQUALS(     policy->empty(), false)
            U_ASSERT_EQUALS(auth_domain->empty(), false)
   
                      _auth_domain = *auth_domain;
            bflat = ((_policy      = *policy) == *policy_flat);
            }
   
         if (bflat ||
             user_exist == false)
            {
            U_ASSERT_EQUALS(   _time_available->empty(), false)
            U_ASSERT_EQUALS(_traffic_available->empty(), false)
   
               time_available =    _time_available->strtol();
            traffic_available = _traffic_available->strtoll();
   
            // NB: reset counter for every login event...
   
            if ((bflat && consume) ||
                user_exist == false)
               {
                  time_done = 0;
               traffic_done = 0;
               }
            }
   
         if (db_store(db_user, op, *uid, toString(), value, *padding2))
            {
            if (op == RDB_INSERT)
               {
               user_exist = true;
   
               value = (*db_user)[*uid];
               }
   
            U_RETURN(true);
            }
   
         U_RETURN(false);
         }
   
   #ifdef DEBUG
      const char* dump(bool breset) const
         {
         *UObjectIO::os << "agent                 " << agent                << '\n'
                        << "consume               " << consume              << '\n'
                        << "connected             " << connected            << '\n'
                        << "time_done             " << time_done            << '\n'
                        << "login_time            " << login_time           << '\n'
                        << "UploadRate            " << UploadRate           << '\n'
                        << "DownloadRate          " << DownloadRate         << '\n'
                        << "time_consumed         " << time_consumed        << '\n'
                        << "traffic_done          " << traffic_done         << '\n'
                        << "last_modified         " << last_modified        << '\n'
                        << "time_available        " << time_available       << '\n'
                        << "traffic_consumed      " << traffic_consumed     << '\n'
                        << "traffic_available     " << traffic_available    << '\n'
                        << "index_access_point    " << index_access_point   << '\n'
                        << "_ip          (UString " << (void*)&_ip          << ")\n"
                        << "_mac         (UString " << (void*)&_mac         << ")\n"
                        << "nodog        (UString " << (void*)&nodog        << ")\n"
                        << "value        (UString " << (void*)&value        << ")\n"
                        << "_policy      (UString " << (void*)&_policy      << ")\n"
                        << "_auth_domain (UString " << (void*)&_auth_domain << ')';
   
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
   
   static void countAP(UStringRep* key, UStringRep* data)
   {
      U_TRACE(5, "::countAP(%.*S,%.*S)", U_STRING_TO_TRACE(*key), U_STRING_TO_TRACE(*data))
   
      nodog_rec->value._assign(data);
   
      nodog_rec->fromString();
   
      _num_ap += nodog_rec->access_point.size() / NUM_ACCESS_POINT_ATTRIBUTE;
   }
   
   static void countUsers(UStringRep* key, UStringRep* data)
   {
      U_TRACE(5, "::countUsers(%.*S,%.*S)", U_STRING_TO_TRACE(*key), U_STRING_TO_TRACE(*data))
   
      ++_num_user;
   
      user_rec->value._assign(data);
   
      user_rec->fromString();
   
      if (user_rec->connected) ++users_connected;
   }
   
   static void callForAllAP(vPFprpr function, UVector<UString>* pvec)
   {
      U_TRACE(5, "::callForAllAP(%p,%p)", function, pvec)
   
      _num_ap = 0;
   
      db_ap->callForAllEntry(function, pvec);
   
      num_ap = _num_ap;
   }
   
   static void callForAllAPSorted(vPFprpr function, UVector<UString>* pvec)
   {
      U_TRACE(5, "::callForAllAPSorted(%p)", function, pvec)
   
      _num_ap = 0;
   
      db_ap->callForAllEntrySorted(function, pvec, UStringExt::qscompver);
   
      num_ap = _num_ap;
   }
   
   static void callForAllUsers(vPFprpr function)
   {
      U_TRACE(5, "::callForAllUsers(%p)", function)
   
      _num_user  = 0;
      user_exist = true;
   
      db_user->callForAllEntry(function);
   
      num_user = _num_user;
   }
   
   #define VIRTUAL_HOST "wifi-aaa.comune.fi.it"
   
   static void usp_init()
   {
      U_TRACE(5, "::usp_init()")
   
      U_INTERNAL_ASSERT_POINTER(U_LOCK_USER1)
      U_INTERNAL_ASSERT_POINTER(U_LOCK_USER2)
   
      UString pathdb_ap   = U_STRING_FROM_CONSTANT(U_LIBEXECDIR "/WiAuthAccessPoint.cdb"),
              pathdb_user = U_STRING_FROM_CONSTANT(U_LIBEXECDIR "/WiAuthUser.cdb");
   
      db_ap   = U_NEW(URDB(pathdb_ap,   false));
      db_user = U_NEW(URDB(pathdb_user, false));
   
      db_ap->setShared(  U_LOCK_USER1);
      db_user->setShared(U_LOCK_USER2);
   
      bool result;
   
      db_ap->lock();
   
      result = db_ap->open(4 * 1024, false, false);
   
      db_ap->unlock();
   
         ap_rec = U_NEW(WiAuthAccessPoint);
       user_rec = U_NEW(WiAuthUser);
      nodog_rec = U_NEW(WiAuthNodog);
   
      if (UServer_Base::bssl == false)
         {
         callForAllAP(countAP, 0);
   
         U_SRV_LOG("db initialization of wi-auth access point WiAuthAccessPoint.cdb %s: num_ap %u", result ? "success" : "FAILED", num_ap);
   
         UFile::writeToTmpl("/tmp/WiAuthAccessPoint.init", db_ap->print());
   
         (void) UFile::_unlink("/tmp/WiAuthAccessPoint.end");
         }
   
      db_user->lock();
   
      result = db_user->open(1024 * 1024, false, false);
   
      db_user->unlock();
   
      if (UServer_Base::bssl == false)
         {
         users_connected = 0;
   
         callForAllUsers(countUsers);
   
         U_SRV_LOG("db initialization of wi-auth users WiAuthUser.cdb %s: num_user %u connected  %u", result ? "success" : "FAILED", num_user, users_connected);
   
         UFile::writeToTmpl("/tmp/WiAuthUser.init", db_user->print());
   
         (void) UFile::_unlink("/tmp/WiAuthUser.end");
         }
   
      ap                 = U_NEW(UString);
      ip                 = U_NEW(UString);
      uid                = U_NEW(UString);
      mac                = U_NEW(UString);
      label              = U_NEW(UString);
      token              = U_NEW(UString);
      ap_ref             = U_NEW(UString(100U));
      output             = U_NEW(UString);
      buffer             = U_NEW(UString);
      policy             = U_NEW(UString);
      address            = U_NEW(UString);
      gateway            = U_NEW(UString);
      account            = U_NEW(UString(100U));
      hostname           = U_NEW(UString);
      padding1           = U_NEW(UString(12, ' '));
      padding2           = U_NEW(UString(36, ' '));
      mac_auth           = U_NEW(U_STRING_FROM_CONSTANT("MAC_AUTH"));
      ip_server          = U_NEW(UString(UServer_Base::getIPAddress()));
      empty_str          = U_NEW(U_STRING_FROM_CONSTANT("\"\""));
      cert_auth          = U_NEW(U_STRING_FROM_CONSTANT("CERT_AUTH"));
      nodog_conf         = U_NEW(UString(UFile::contentOf("ap/nodog.conf.template")));
      empty_list         = U_NEW(U_STRING_FROM_CONSTANT("()"));
      policy_flat        = U_NEW(U_STRING_FROM_CONSTANT("FLAT"));
      request_uri        = U_NEW(UString);
      auth_domain        = U_NEW(UString);
      account_auth       = U_NEW(U_STRING_FROM_CONSTANT("ACCOUNT_AUTH"));
      client_address     = U_NEW(UString);
      allowed_web_hosts  = U_NEW(UString);
   
      _time_chunk        = U_NEW(UString(20U));
      _time_consumed     = U_NEW(UString(20U));
      _time_available    = U_NEW(UString);
      _traffic_chunk     = U_NEW(UString(20U));
      _traffic_consumed  = U_NEW(UString(20U));
      _traffic_available = U_NEW(UString);
   
      user_UploadRate    = U_NEW(UString);
      user_DownloadRate  = U_NEW(UString);
   
      environment        = U_NEW(UString(*USSIPlugIn::environment + "VIRTUAL_HOST=" + VIRTUAL_HOST));
   
      dir_reg            = U_NEW(UString(UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("DIR_REG"),            environment)));
      dir_root           = U_NEW(UString(UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("DIR_ROOT"),           environment)));
      virtual_name       = U_NEW(UString(UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("VIRTUAL_NAME"),       environment)));
      historical_log_dir = U_NEW(UString(UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("HISTORICAL_LOG_DIR"), environment)));
   
      UString tmp1 = UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("LDAP_CARD_PARAM"),    environment),
              tmp2 = UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("WIAUTH_CARD_BASEDN"), environment);
   
        ldap_card_param  = U_NEW(UString(UStringExt::expandEnvironmentVar(tmp1, environment)));
      wiauth_card_basedn = U_NEW(UString(UStringExt::expandEnvironmentVar(tmp2, environment)));
   
      tmp1 = UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("LDAP_USER_PARAM"),    environment),
      tmp2 = UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("WIAUTH_USER_BASEDN"), environment);
   
        ldap_user_param  = U_NEW(UString(UStringExt::expandEnvironmentVar(tmp1, environment)));
      wiauth_user_basedn = U_NEW(UString(UStringExt::expandEnvironmentVar(tmp2, environment)));
   
      UString content = UFile::contentOf("$DIR_ROOT/etc/AllowedWebHosts.txt", O_RDONLY, false, environment);
   
      vec = U_NEW(UVector<UString>);
   
      if (content.empty() == false &&
          vec->split(content))
         {
         *allowed_web_hosts = vec->join(" ") + ' ';
                              vec->clear();
         }
   
      cache   = U_NEW(UCache);
      content = U_STRING_FROM_CONSTANT("$DIR_ROOT/etc/" VIRTUAL_HOST "/cache.tmpl");
   
      (void) cache->open(content, U_STRING_FROM_CONSTANT("$DIR_TEMPLATE"), environment);
   
      message_page_template   = U_NEW(UString(cache->getContent(U_CONSTANT_TO_PARAM("message_page.tmpl"))));
      status_nodog_template   = U_NEW(UString(cache->getContent(U_CONSTANT_TO_PARAM("status_nodog_body.tmpl"))));
      status_network_template = U_NEW(UString(cache->getContent(U_CONSTANT_TO_PARAM("status_network_body.tmpl"))));
   
      content = UFile::contentOf("$DIR_ROOT/etc/" VIRTUAL_HOST "/script.conf", O_RDONLY, false, environment);
   
      table = U_NEW(UHashMap<UString>);
   
      table->allocate();
   
      if (UFileConfig::loadProperties(*table, content.data(), content.end()))
         {
         telefono          = U_NEW(UString((*table)["TELEFONO"]));
         fmt_auth_cmd      = U_NEW(UString((*table)["FMT_AUTH_CMD"]));
         redirect_default  = U_NEW(UString((*table)["REDIRECT_DEFAULT"]));
   
         url_banner_ap     = U_NEW(UString(UStringExt::expandPath((*table)["URL_BANNER_AP"],               environment)));
         url_banner_comune = U_NEW(UString(UStringExt::expandPath((*table)["URL_BANNER_COMUNE"],           environment)));
   
         help_url          = U_NEW(UString(UStringExt::expandEnvironmentVar((*table)["HELP_URL"],          environment)));
         login_url         = U_NEW(UString(UStringExt::expandEnvironmentVar((*table)["LOGIN_URL"],         environment)));
         wallet_url        = U_NEW(UString(UStringExt::expandEnvironmentVar((*table)["WALLET_URL"],        environment)));
         registrazione_url = U_NEW(UString(UStringExt::expandEnvironmentVar((*table)["REGISTRAZIONE_URL"], environment)));
   
         UString x(U_CAPACITY);
   
         x.snprintf("$DIR_WEB/" VIRTUAL_HOST "%s/default", url_banner_ap->data());
   
         url_banner_ap_default = U_NEW(UFile(x, environment));
   
         x.snprintf("$DIR_WEB/" VIRTUAL_HOST "%s/default", url_banner_comune->data());
   
         url_banner_comune_default = U_NEW(UFile(x, environment));
   
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
   
      table->clear();
   
      UDES3::setPassword("vivalatopa");
   
      client = U_NEW(UHttpClient<UTCPSocket>(0));
   
      client->setSaveHttpInfo(true);
   
      if (UServer_Base::isLog() &&
          client->UClient_Base::isLogSharedWithServer() == false)
         {
         client->UClient_Base::setLogShared();
         }
   
      file_LOG = U_NEW(UFile(U_STRING_FROM_CONSTANT("$FILE_LOG"), environment));
   
      UString dir = UStringExt::dirname(file_LOG->getPath());
   
      file_WARNING  = U_NEW(UFile(dir + U_STRING_FROM_CONSTANT("/wifi-warning")));
      file_RECOVERY = U_NEW(UFile(dir + U_STRING_FROM_CONSTANT("/wifi-recovery")));
   
      (void) UServer_Base::addLog(file_LOG);
      (void) UServer_Base::addLog(file_WARNING);
      (void) UServer_Base::addLog(file_RECOVERY, O_CREAT | O_RDWR | O_APPEND);
   }
   
   static void usp_end()
   {
      U_TRACE(5, "::usp_end()")
   
      if (db_ap)
         {
   #  ifdef DEBUG
         delete ip;
         delete ap;
         delete uid;
         delete mac;
         delete label;
         delete token;
         delete ap_ref;
         delete buffer;
         delete policy;
         delete output;
         delete account;
         delete address;
         delete dir_reg;
         delete gateway;
         delete dir_root;
         delete hostname;
         delete padding1;
         delete padding2;
         delete mac_auth;
         delete ip_server;
         delete empty_str;
         delete cert_auth;
         delete nodog_conf;
         delete empty_list;
         delete auth_domain;
         delete environment;
         delete policy_flat;
         delete request_uri;
         delete account_auth;
         delete virtual_name;
         delete client_address;
         delete ldap_user_param;
         delete ldap_card_param;
         delete allowed_web_hosts;
         delete wiauth_card_basedn;
         delete wiauth_user_basedn;
         delete historical_log_dir;
         delete message_page_template;
         delete status_nodog_template;
         delete status_network_template;
   
         delete _time_chunk;
         delete _time_consumed;
         delete _time_available;
         delete _traffic_chunk;
         delete _traffic_consumed;
         delete _traffic_available;
   
         delete user_UploadRate;
         delete user_DownloadRate;
   
         if (help_url)
            {
            delete telefono;
            delete help_url;
            delete login_url;
            delete wallet_url;
            delete fmt_auth_cmd;
            delete url_banner_ap;
            delete redirect_default;
            delete url_banner_comune;
            delete registrazione_url;
   
            if (url_banner_ap_default)     delete url_banner_ap_default;
            if (url_banner_comune_default) delete url_banner_comune_default;
            }
   
            ap_rec->clear();
          user_rec->clear();
         nodog_rec->clear();
   
         table->clear();
         table->deallocate();
   
         delete vec;
         delete table;
         delete cache;
         delete client;
   
         delete ap_rec;
         delete user_rec;
         delete nodog_rec;
   #  endif
   
         if (UServer_Base::bssl)
            {
            (void) db_ap->close();
            (void) db_user->close();
            }
         else
            {
            UFile::writeToTmpl("/tmp/WiAuthAccessPoint.end", db_ap->print());
   
            (void) db_ap->closeReorganize();
   
            UFile::writeToTmpl("/tmp/WiAuthUser.end", db_user->print());
   
            (void) db_user->closeReorganize();
   
            (void) UFile::_unlink("/tmp/WiAuthUser.init");
            (void) UFile::_unlink("/tmp/WiAuthAccessPoint.init");
            }
   
   #  ifdef DEBUG
         delete db_ap;
         delete db_user;
   #  endif
         }
   }
   
   static void writeToLOG(const char* op)
   {
      U_TRACE(5, "::writeToLOG(%S)", op)
   
      char _buffer[4096];
   
      /* Example
      --------------------------------------------------------------------------------------------------------------------------------------------------------- 
      2012/08/08 14:56:00 op: PASS_AUTH, uid: 33437934, ap: 00@10.8.1.2, ip: 172.16.1.172, mac: 00:14:a5:6e:9c:cb, timeout: 233, traffic: 342, policy: DAILY
      --------------------------------------------------------------------------------------------------------------------------------------------------------- 
      */
   
      U_INTERNAL_ASSERT(user_exist)
      U_INTERNAL_ASSERT_POINTER(op)
   
      UString x = user_rec->getAP();
                  user_rec->getCounter();
   
      (void) U_SYSCALL(write, "%d,%p,%u", file_LOG->getFd(), _buffer,
                           u__snprintf(_buffer, sizeof(_buffer),
                              "%6D op: %s, uid: %.*s, ap: %s, ip: %.*s, mac: %.*s, timeout: %.*s, traffic: %.*s, policy: %.*s\n",
                              op,
                              U_STRING_TO_TRACE(*uid),
                              x.data(),
                              U_STRING_TO_TRACE(user_rec->_ip),
                              U_STRING_TO_TRACE(user_rec->_mac),
                              U_STRING_TO_TRACE(*_time_chunk),
                              U_STRING_TO_TRACE(*_traffic_chunk),
                              U_STRING_TO_TRACE(user_rec->_policy)));
   }
   
   static void checkIfUserExist(UStringRep* key, UStringRep* data)
   {
      U_TRACE(5, "::checkIfUserExist(%.*S,%.*S)", U_STRING_TO_TRACE(*key), U_STRING_TO_TRACE(*data))
   
      if (memcmp(cptr, data->data(), csize) == 0)
         {
         istrstream is(data->data(), data->size());
   
         is >> *ip;
   
         if (ip->size() == csize)
            {
            U_ASSERT_EQUALS(*ip, *client_address)
   
            user_exist = true;
   
            (void) uid->assign(U_STRING_TO_PARAM(*key));
   
            user_rec->value._assign(data);
   
            user_rec->fromString();
   
            U_ASSERT_EQUALS(user_rec->_ip, *client_address)
   
            // NB: db can have different users with the same ip...
   
            if (user_rec->connected) db_user->stopCallForAllEntry();
            }
         }
   }
   
   static bool checkIfUserConnected()
   {
      U_TRACE(5, "::checkIfUserConnected()")
   
      user_exist = false;
   
      if (uid->empty() == false) user_rec->setValue();
      else
         {
         cptr  = client_address->data();
         csize = client_address->size();
   
         db_user->callForAllEntry(checkIfUserExist);
         }
   
      if (user_exist)
         {
         // NB: db can have different users for the same ip...
   
         U_INTERNAL_ASSERT_EQUALS(user_rec->nodog.empty(), false)
   
         if (user_rec->connected) U_RETURN(true);
         }
   
      U_RETURN(false);
   }
   
   static bool isUserConnected(bool validation)
   {
      U_TRACE(5, "::isUserConnected(%b)", validation)
   
      UHTTP::getFormValue(*uid, U_CONSTANT_TO_PARAM("redirect"), 0, 5, 14);
   
      U_INTERNAL_DUMP("uid = %.*S", U_STRING_TO_TRACE(*uid))
   
      U_ASSERT_EQUALS(uid->empty(), false)
   
      if (uid->size() > 32)
         {
         UString _buffer(U_CAPACITY);
   
         if (UBase64::decode(*uid, _buffer))
            {
            output->setBuffer(U_CAPACITY);
   
            UDES3::decode(_buffer, *output);
   
            // ========================
            // => uid
            // => policy
            // => auth_domain
            // => account
            // => max_time
            // => max_traffic
            // => UserDownloadRate 
            // => UserUploadRate 
            // ========================
   
            ptr1 = output->data() + U_CONSTANT_SIZE("uid=");
            ptr2 = ptr1;
   
            do ++ptr2; while (*ptr2 != '&');
   
            uint32_t pos = ptr2 - ptr1;
   
            (void) uid->assign(ptr1, pos);
   
            if (validation)
               {
               UHTTP::getFormValue(*gateway, U_CONSTANT_TO_PARAM("gateway"),  0,  7, 14);
               UHTTP::getFormValue(*token,   U_CONSTANT_TO_PARAM("token"),    0, 11, 14);
   
               UVector<UString> name_value(10);
   
               (void) UStringExt::getNameValueFromData(output->substr(pos+1), name_value, U_CONSTANT_TO_PARAM("&"));
   
               *policy              = name_value[ 1];  
               *auth_domain         = name_value[ 3];
               *account             = name_value[ 5]; 
               *_time_available     = name_value[ 7];
               *_traffic_available  = name_value[ 9];
               *user_DownloadRate   = name_value[11];
               *user_UploadRate     = name_value[13];
               }
            }
         }
   
      if (checkIfUserConnected() || user_exist) (void) user_rec->setNodogReference();
   
      if (user_exist &&
          user_rec->connected)
         {
         U_RETURN(true);
         }
   
      U_RETURN(false);
   }
   
   static bool isUserConnected(UStringRep* data)
   {
      U_TRACE(5, "::isUserConnected(%.*S)", U_STRING_TO_TRACE(*data))
   
      const char* _ptr = data->data() + 7; // "1.1.1.1"
   
      while (u__isspace(*_ptr) == false) ++_ptr;
   
      U_INTERNAL_DUMP("_ptr = %.40s", _ptr)
   
      U_RETURN(_ptr[1] == '1');
   }
   
   static UString getUserName()
   {
      U_TRACE(5, "::getUserName()")
   
      UString user;
   
      if (user_exist == false) user = get_UserName();
         {
         user = user_rec->_user;
   
         UString x = get_UserName();
   
         if (user != x)
            {
            U_LOGGER("*** getUserName() USER(%.*s=>%.*s) WITH DIFFERENT VALUE ***", U_STRING_TO_TRACE(user), U_STRING_TO_TRACE(x));
            }
         }
   
      U_RETURN_STRING(user);
   }
   
   static void quitUserConnected(UStringRep* key, UStringRep* data)
   {
      U_TRACE(5, "::quitUserConnected(%.*S,%.*S)", U_STRING_TO_TRACE(*key), U_STRING_TO_TRACE(*data))
   
      ++_num_user;
   
      if (isUserConnected(data))
         {
         user_rec->value._assign(data);
   
         user_rec->fromString();
   
         U_INTERNAL_ASSERT(user_rec->connected)
   
         if (user_rec->nodog == *address)
            {
            uid->assign(U_STRING_TO_PARAM(*key));
   
            user_rec->setConnected(false);
   
            writeToLOG("QUIT");
            }
         }
   }
   
   static void resetUserPolicy(UStringRep* key, UStringRep* data)
   {
      U_TRACE(5, "::resetUserPolicy(%.*S,%.*S)", U_STRING_TO_TRACE(*key), U_STRING_TO_TRACE(*data))
   
      ++_num_user;
   
      user_rec->value._assign(data);
   
      user_rec->fromString();
   
      if (user_rec->_policy == *policy) user_rec->resetCounter();
   }
   
   static void setStatusUser(UStringRep* key, UStringRep* data)
   {
      U_TRACE(5, "::setStatusUser(%.*S,%.*S)", U_STRING_TO_TRACE(*key), U_STRING_TO_TRACE(*data))
   
      ++_num_user;
   
      if (isUserConnected(data))
         {
         user_rec->value._assign(data);
   
         user_rec->fromString();
   
         U_INTERNAL_ASSERT(user_rec->connected)
   
         (void) uid->assign(U_STRING_TO_PARAM(*key));
   
         if (user_rec->setNodogReference())
            {
            ++users_connected;
   
            if (user_rec->consume)
               {
               user_rec->getCounter();
   
               ptr1 = "green";
               ptr2 = "yes";
               ptr3 =    _time_chunk->data();
               ptr4 = _traffic_chunk->data();
               }
            else
               {
               user_rec->getConsumed();
   
               ptr1 = "orange";
               ptr2 = "no";
               ptr3 =    _time_consumed->data();
               ptr4 = _traffic_consumed->data();
               }
   
            UString riga(U_CAPACITY),
                    x      = user_rec->getAP(),
                    user   = getUserName(),
                    _label = user_rec->getLabelAP();
   
            riga.snprintf(status_network_template->data(),
                          U_STRING_TO_TRACE(user),
                          U_STRING_TO_TRACE(*uid),
                          U_STRING_TO_TRACE(user_rec->_auth_domain),
                          U_STRING_TO_TRACE(user_rec->_ip),
                          U_STRING_TO_TRACE(user_rec->_mac),
                          user_rec->login_time + u_now_adjust,
                          U_STRING_TO_TRACE(user_rec->_policy),
                          ptr1, ptr2,
                          ptr3, ptr4,
                          VIRTUAL_HOST,
                          U_STRING_TO_TRACE(_label), U_STRING_TO_TRACE(nodog_rec->_hostname),
                          U_STRING_TO_TRACE(user_rec->nodog), nodog_rec->port, U_STRING_TO_TRACE(x));
   
            (void) output->append(riga);
            }
         }
   }
   
   static bool writeStatusAccessPoint()
   {
      U_TRACE(5, "::writeStatusAccessPoint()")
   
      if (ap_rec->noconsume)
         {
         ptr3 = "orange";
         ptr4 = "no";
         }
      else
         {
         ptr3 = "green";
         ptr4 = "yes";
         }
   
      const char* ptr  = ap_rec->_label.data();
      uint32_t    size = ap_rec->_label.size();
   
      UString riga(U_CAPACITY);
   
      riga.snprintf(status_nodog_template->data(),
                    size, ptr,
                    VIRTUAL_HOST, size, ptr, U_STRING_TO_TRACE(nodog_rec->_hostname), U_STRING_TO_TRACE(*address), nodog_rec->port,
                    U_STRING_TO_TRACE(*address),
                    U_STRING_TO_TRACE(nodog_rec->_hostname),
                    ptr1, ptr2,
                    nodog_rec->last_info + u_now_adjust,
                    ptr3, ptr4,
                    U_STRING_TO_TRACE(ap_rec->mac_mask),
                    U_STRING_TO_TRACE(ap_rec->group_account),
                    VIRTUAL_HOST, size, ptr, U_STRING_TO_TRACE(nodog_rec->_hostname), U_STRING_TO_TRACE(*address), nodog_rec->port);
   
      (void) output->append(riga);
   
      U_RETURN(true);
   }
   
   static void setStatusNodog(UStringRep* key, UStringRep* data)
   {
      U_TRACE(5, "::setStatusNodog(%.*S,%.*S)", U_STRING_TO_TRACE(*key), U_STRING_TO_TRACE(*data))
   
      countAP(key, data);
   
      (void) address->assign(U_STRING_TO_PARAM(*key));
   
      if (nodog_rec->down)
         {
         ptr1 = "red";
         ptr2 = "NO";
         }
      else
         {
         ptr1 = "green";
         ptr2 = "yes";
         }
   
      nodog_rec->callForAllAccessPoint(writeStatusAccessPoint);
   }
   
   static UString sendRequestToNodog(const UString& url)
   {
      U_TRACE(5, "::sendRequestToNodog(%.*S)", U_STRING_TO_TRACE(url))
   
      // NB: we need PREFORK_CHILD > 2
   
      UString result;
   
      if (client->connectServer(url) &&
          client->sendRequest(result))
         {
         result = client->getContent();
   
         if (nodog_rec->down) nodog_rec->setDown(false);
         else                 nodog_rec->setLastInfo();
         }
      else
         {
         if (nodog_rec->down) nodog_rec->setLastInfo();
         else
            {
            nodog_rec->setDown(true);
   
            callForAllUsers(quitUserConnected);
            }
         }
   
      client->reset(); // NB: url is referenced by UClient::url...
   
      U_RETURN_STRING(result);
   }
   
   static UString sendRequestToNodog(const char* fmt, ...)
   {
      U_TRACE(5, "::sendRequestToNodog(%S)", fmt)
   
      U_ASSERT_EQUALS(address->empty(), false)
   
      UString _buffer(U_CAPACITY), url(U_CAPACITY), result;
   
      va_list argp;
      va_start(argp, fmt);
   
      _buffer.vsnprintf(fmt, argp);
   
      va_end(argp);
   
      url.snprintf("http://%.*s:%u/%.*s", U_STRING_TO_TRACE(*address), nodog_rec->port, U_STRING_TO_TRACE(_buffer));
   
      result = sendRequestToNodog(url);
   
      U_RETURN_STRING(result);
   }
   
   static void checkAccessPoint(UStringRep* key, UStringRep* data)
   {
      U_TRACE(5, "::checkAccessPoint(%.*S,%.*S)", U_STRING_TO_TRACE(*key), U_STRING_TO_TRACE(*data))
   
      countAP(key, data);
   
      if ((u_now->tv_sec - nodog_rec->last_info) > (16L * 60L)) UCDB::addEntryToVector();
   }
   
   static bool setAccessPointAddress()
   {
      U_TRACE(5, "::setAccessPointAddress()")
   
        label->clear();
      address->clear();
   
      if (ap->empty()) U_RETURN(false);
   
      uint32_t pos = ap->find('@');
   
      if (pos == U_NOT_FOUND) U_RETURN(false);
   
      *label   = ap->substr(0U, pos).copy();
      *address = ap->substr(pos + 1).copy();
   
      U_RETURN(true);
   }
   
   static void setAccessPointReference()
   {
      U_TRACE(5, "::setAccessPointReference()")
   
      U_INTERNAL_ASSERT_POINTER(help_url)
      U_INTERNAL_ASSERT_POINTER(wallet_url)
      U_INTERNAL_ASSERT_POINTER(url_banner_ap)
      U_INTERNAL_ASSERT_POINTER(url_banner_comune)
   
      const char* _ptr;
   
      if (ap->empty()) ap_ref->snprintf("default", 0);
      else
         {
         uint32_t certid = 0;
   
         if (address->empty() == false)
            {
            _ptr = address->data();
   
            // Ex: 10.8.1.2
   
            for (uint32_t i = 0, dot_count = 0; dot_count < 3; ++i)
               {
               if (_ptr[i++] == '.')
                  {
                  ++dot_count;
   
                       if (dot_count == 2) certid  = 254 * strtol(_ptr+i, 0, 10);
                  else if (dot_count == 3) certid +=       strtol(_ptr+i, 0, 10);
                  }
               }
            }
   
         ap_ref->snprintf("X%04dR%.*s", certid, U_STRING_TO_TRACE(*label));
         }
   
      UString banner(U_CAPACITY), x(U_CAPACITY);
   
      if (url_banner_ap_default)
         {
         U_ASSERT(url_banner_ap_default->dir())
   
         x.snprintf("$DIR_WEB/" VIRTUAL_HOST "%s/%s", url_banner_ap->data(), ap_ref->data());
   
         banner = UStringExt::expandPath(x, environment);
         _ptr   = banner.data();
   
         if (UFile::access(_ptr) ||
             UFile::_mkdir(_ptr))
            {
            (void) banner.append(U_CONSTANT_TO_PARAM("/default"));
   
            _ptr = banner.data();
   
            if (UFile::access(_ptr) == false) (void) url_banner_ap_default->symlink(_ptr);
            }
         }
   
      if (url_banner_comune_default)
         {
         U_ASSERT(url_banner_comune_default->dir())
   
         x.snprintf("$DIR_WEB/" VIRTUAL_HOST "%s/%s", url_banner_comune->data(), ap_ref->data());
   
         banner = UStringExt::expandPath(x, environment);
         _ptr   = banner.data();
   
         if (UFile::access(_ptr) ||
             UFile::_mkdir(_ptr))
            {
            (void) banner.append(U_CONSTANT_TO_PARAM("/default"));
   
            _ptr = banner.data();
   
            if (UFile::access(_ptr) == false) (void) url_banner_comune_default->symlink(_ptr);
            }
         }
   }
   
   static bool setAccessPoint(bool localization)
   {
      U_TRACE(5, "::setAccessPoint(%b)", localization)
   
      // $1 -> ap
      // $2 -> public address to contact the access point
      // $3 -> pid (0 => start)
   
      int ap_port = 5280;
      uint32_t index_access_point, end = UHTTP::processHTTPForm();
   
            ap->clear();
         label->clear();
       address->clear();
       account->clear();
      hostname->clear();
   
      if (end)
         {
         uint32_t pos;
         UString _hostname, _address;
   
         UHTTP::getFormValue(_hostname, U_CONSTANT_TO_PARAM("ap"),     0, 1, end);
         UHTTP::getFormValue(_address,  U_CONSTANT_TO_PARAM("public"), 0, 3, end);
   
         if (_hostname.empty() == false)
            {
            pos = _hostname.find('@');
   
            if (pos == U_NOT_FOUND) *hostname = _hostname;
            else
               {
               *label    = _hostname.substr(0U, pos).copy();
               *hostname = _hostname.substr(pos + 1).copy();
               }
   
            // NB: bad case generated by MAIN_BASH...
   
            if (u_isIPv4Addr(U_STRING_TO_PARAM(*hostname)))
               {
               *address = *hostname;
                           hostname->clear();
   
               goto next;
               }
   
            if (u_isHostName(U_STRING_TO_PARAM(*hostname)) == false)
               {
               U_LOGGER("*** AP HOSTNAME(%.*s) NOT VALID ***", U_STRING_TO_TRACE(*hostname));
   
               U_RETURN(false);
               }
            }
   
         if (_address.empty() == false)
            {
            pos = _address.find(':');
   
            if (pos == U_NOT_FOUND) *address = _address;
            else
               {
               *address = _address.substr(0U, pos).copy();
               ap_port  = _address.substr(pos+1).strtol();
               }
   
            if (u_isIPv4Addr(U_STRING_TO_PARAM(*address)) == false)
               {
               U_LOGGER("*** ADDRESS AP(%.*s) NOT VALID ***", U_STRING_TO_TRACE(*address));
   
               U_RETURN(false);
               }
            }
         }
   
      U_INTERNAL_DUMP("address = %.*S hostname = %.*S label = %.*S", U_STRING_TO_TRACE(*address), U_STRING_TO_TRACE(*hostname), U_STRING_TO_TRACE(*label))
   
      if (address->empty()   ||
          (localization      &&
          (   label->empty() ||
           hostname->empty())))
         {
         U_RETURN(false);
         }
   
   next:
      if (nodog_rec->setRecord(&index_access_point, ap_port)) U_RETURN(true);
   
      U_RETURN(false);
   }
   
   static void loginWithProblem()
   {
      U_TRACE(5, "::loginWithProblem()")
   
      if (uid->empty() == false)
         {
         U_LOGGER("*** FAILURE: UID(%.*s) IP(%.*s) MAC(%.*s) AP(%.*s) ***",
                     U_STRING_TO_TRACE(*uid), U_STRING_TO_TRACE(*ip), U_STRING_TO_TRACE(*mac), U_STRING_TO_TRACE(*ap));
         }
   
      USSIPlugIn::setMessagePageWithVar(*message_page_template, "Login",
                                        "Problema in fase di autenticazione. "
                                        "Si prega di riprovare, se il problema persiste contattare: %.*s", U_STRING_TO_TRACE(*telefono));
   }
   
   static bool askToLDAP(UString* pinput, const char* title_txt, const char* message, const char* fmt, ...)
   {
      U_TRACE(5, "::askToLDAP(%p,%S,%S,%S)",  pinput, title_txt, message, fmt)
   
      /*
      ldapsearch -LLL -b ou=cards,o=unwired-portal -x -D cn=admin,o=unwired-portal -w programmer -H ldap://127.0.0.1 waLogin=3386453924
      ---------------------------------------------------------------------------------------------------------------------------------
      dn: waCid=6bc07bf3-a09f-4815-8029-db68f32f4189,ou=cards,o=unwired-portal
      objectClass: top
      objectClass: waCard
      waCid: 6bc07bf3-a09f-4815-8029-db68f32f4189
      waPin: 3386453924
      waCardId: db68f32f4189
      waLogin: 3386453924
      waPassword: {MD5}ciwjVccK0u68vqupEXFukQ==
      waRevoked: FALSE
      waValidity: 0
      waPolicy: DAILY
      waTime: 7200
      waTraffic: 314572800
      ---------------------------------------------------------------------------------------------------------------------------------
      */
   
      va_list argp;
      va_start(argp, fmt);
   
      int result = UServices::askToLDAP(pinput, table, fmt, argp);
   
      va_end(argp);
   
      if (result <= 0)
         {
         if (result == -1) // Can't contact LDAP server (-1)
            {
            U_LOGGER("*** LDAP NON DISPONIBILE (anomalia 008) ***", 0);
   
            title_txt = "Servizio LDAP non disponibile";
            message   = "Servizio LDAP non disponibile (anomalia 008)";
            }
   
         if (title_txt && message) USSIPlugIn::setMessagePage(*message_page_template, title_txt, message);
   
         U_RETURN(false);
         }
   
      U_RETURN(true);
   }
   
   static bool runAuthCmd(const char* _uid, const char* password)
   {
      U_TRACE(5, "::runAuthCmd(%S,%S)", _uid, password)
   
      static int fd_stderr = UServices::getDevNull("/tmp/auth_cmd.err");
   
      UString cmd(U_CAPACITY);
   
      cmd.snprintf(fmt_auth_cmd->data(), _uid, password);
   
      *output = UCommand::outputCommand(cmd, 0, -1, fd_stderr);
   
      UServer_Base::logCommandMsgError(cmd.data(), true);
   
      if (UCommand::exit_value ||
          output->empty())
         {
         U_LOGGER("*** AUTH_CMD fail EXIT_VALUE=%d RESPONSE=%.*S ***", UCommand::exit_value, U_STRING_TO_TRACE(*output));
   
         if (UCommand::exit_value == 1) USSIPlugIn::setMessagePage(*message_page_template, "Utente e/o Password errato/i", "Credenziali errate!");
         else                           USSIPlugIn::setMessagePage(*message_page_template, "Errore", "Richiesta autorizzazione ha esito errato");
   
         U_RETURN(false);
         }
   
      U_RETURN(true);
   }
   
   static bool askNodogToLogoutUser(const UString& signed_data)
   {
      U_TRACE(5, "::askNodogToLogoutUser(%.*S)", U_STRING_TO_TRACE(signed_data))
   
      UString result = sendRequestToNodog("logout?%.*s", U_STRING_TO_TRACE(signed_data));
   
      if (U_IS_HTTP_ERROR(client->responseCode())) U_RETURN(false);
   
      // --------------------------------------------------
      // NB: we can have two possibility:
      // --------------------------------------------------
      // 1) response No Content       (204) => OK
      // 2) response No Empty (status user) => must be DENY
      // --------------------------------------------------
   
      if (result.empty() == false)
         {
         if (U_STRING_FIND(result, 0, "DENY") == U_NOT_FOUND)
            {
            U_LOGGER("*** user status NOT DENY: AP(%.*s) CLIENT(%.*s) IP(%.*s) MAC(%.*s) ***", U_STRING_TO_TRACE(nodog_rec->_hostname),
                           U_STRING_TO_TRACE(*uid), U_STRING_TO_TRACE(user_rec->_ip), U_STRING_TO_TRACE(user_rec->_mac));
   
            U_RETURN(false);
            }
         }
   
      // NB: we must have serviced a info request from nodog by another process istance (PREFORK_CHILD > 2)...
   
      U_ASSERT_EQUALS(uid->empty(), false)
   
      if (checkIfUserConnected())
         {
         user_rec->setConnected(false);
   
         U_RETURN(false);
         }
   
      U_INTERNAL_ASSERT_EQUALS(user_rec->connected, false)
   
      U_RETURN(true);
   }
   
   static bool askNodogToLogoutUser()
   {
      U_TRACE(5, "::askNodogToLogoutUser()")
   
      UString signed_data = UDES3::signData("ip=%.*s&mac=%.*s", U_STRING_TO_TRACE(user_rec->_ip), U_STRING_TO_TRACE(user_rec->_mac));
   
      bool result = askNodogToLogoutUser(signed_data);  
   
      U_RETURN(result);
   }
   
   static bool checkLoginRequest(uint32_t end)
   {
      U_TRACE(5, "::checkLoginRequest(%u)", end)
   
           ap->clear();
           ip->clear();
          uid->clear();
          mac->clear();
      gateway->clear();
     hostname->clear();
   
      user_exist = false;
   
      if (end == (UHTTP::form_name_value->empty() ? UHTTP::processHTTPForm()
                                                  : UHTTP::form_name_value->size()))
         {
         UHTTP::getFormValue(*ip, U_CONSTANT_TO_PARAM("ip"), 0,  3, end);
         UHTTP::getFormValue(*ap, U_CONSTANT_TO_PARAM("ap"), 0, 13, end);
   
         if (end == 22) U_RETURN(true);
   
         if (setAccessPointAddress())
            {
            // ----------------------------------------------------------------------------------------
            // NB: *** the params CAN be empty ***
            // ----------------------------------------------------------------------------------------
            // $1 -> mac
            // $2 -> ip
            // $3 -> redirect
            // $4 -> gateway
            // $5 -> timeout
            // $6 -> token
            // $7 -> ap (with localization => '@')
            // ----------------------------------------------------------------------------------------
   
            U_INTERNAL_ASSERT_EQUALS(end, 14)
   
            UHTTP::getFormValue(*mac, U_CONSTANT_TO_PARAM("mac"), 0, 1, 14);
   
            U_RETURN(true);
            }
         }
   
      U_RETURN(false);
   }
   
   static void checkLogoutRequest(bool bget)
   {
      U_TRACE(5, "::checkLogoutRequest(%b)", bget)
   
      if (bget) uid->clear();
      else
         {
         // ----------------------------
         // $1 -> uid
         // ----------------------------
   
         UHTTP::getFormValue(*uid, U_CONSTANT_TO_PARAM("uid"), 0, 1, UHTTP::processHTTPForm());
         }
   
      UString signed_data;
   
      if (checkIfUserConnected() &&
          user_rec->setNodogReference())
         {
         signed_data = UDES3::signData("ip=%.*s&mac=%.*s", U_STRING_TO_TRACE(user_rec->_ip), U_STRING_TO_TRACE(user_rec->_mac));
         }
      else if (user_exist)
         {
         *address = user_rec->nodog;
   
         signed_data = UDES3::signData("ip=%.*s", U_STRING_TO_TRACE(*client_address));
         }
   
      if (signed_data.empty() ||
          askNodogToLogoutUser(signed_data) == false)
         {
         USSIPlugIn::setMessagePage(*message_page_template, "Utente non connesso", "Utente non connesso");
         }
      else
         {
         if (bget)
            {
            user_rec->getCounter();
   
            USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("ringraziamenti.tmpl")),
                                              "Firenze WiFi", 0, 0,
                                              U_STRING_TO_TRACE(*uid),
                                              _time_chunk->data(), _traffic_chunk->data());
            }
         else
            {
            U_http_is_connection_close = U_YES;
   
            USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("logout_notify.tmpl")),
                                              "Firenze WiFi", "<script type=\"text/javascript\" src=\"js/logout_popup.js\"></script>",
                                              "'onload=\"CloseItAfterSomeTime()\"'",
                                              U_STRING_TO_TRACE(*uid));
            }
         }
   }
   
   static void isLogged()
   {
      U_TRACE(5, "::isLogged()")
   
      (void) user_rec->setNodogReference();
   
      UString x = user_rec->getAP();
   
      USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("logged.tmpl")),
                                        "Firenze WiFi", 0, 0,
                                        url_banner_ap->data(), help_url->data(), wallet_url->data(),
                                        x.data(), "/logged_login_request", url_banner_comune->data());
   }
   
   static void loadPolicy(const UString& _policy)
   {
      U_TRACE(5, "::loadPolicy(%.*S)", U_STRING_TO_TRACE(_policy))
   
      U_ASSERT_EQUALS(_policy.empty(), false)
   
      const char* key_time    = 0;
      const char* key_traffic = 0;
   
      if (table->empty() == false)
         {
         key_time    = "waTime";
         key_traffic = "waTraffic";
         }
      else
         {
         UString pathname(U_CAPACITY), content;
   
         pathname.snprintf("%.*s/policy/%.*s", U_STRING_TO_TRACE(*dir_root), U_STRING_TO_TRACE(_policy));
   
         content = UFile::contentOf(pathname);
   
         if (UFileConfig::loadProperties(*table, content.data(), content.end()))
            {
            key_time    = "MAX_TIME";
            key_traffic = "MAX_TRAFFIC";
            }
         }
   
      if (key_time &&
          key_traffic)
         {
         *_time_available    = (*table)[key_time];
         *_traffic_available = (*table)[key_traffic];
         }
   
      table->clear();
   }
   
   /*******************************
   #define U_MANAGED_BY_MAIN_BASH 1 
   ********************************/
   
   static void GET_admin()
   {
      U_TRACE(5, "::GET_admin()")
   
      USSIPlugIn::setAlternativeRedirect("https://%.*s/admin.html", U_STRING_TO_TRACE(*ip_server));
   }
   
   static void GET_card_activation()
   {
      U_TRACE(5, "::GET_card_activation()")
   
      if (*client_address != *ip_server) u_http_info.nResponseCode = HTTP_BAD_REQUEST;
   #ifndef U_MANAGED_BY_MAIN_BASH
      /*
      ---------------------------------------------------------------------------------------------------------------
      NB: we can manage this request with the main.bash script...
      ---------------------------------------------------------------------------------------------------------------
      */
   #endif
   }
   
   static void GET_edit_ap()
   {
      U_TRACE(5, "::GET_edit_ap()")
   
      // $1 -> ap (with localization => '@')
      // $2 -> public address to contact the access point
   
      if (setAccessPoint(true) == false)
         {
         if (UHTTP::form_name_value->empty() == false) u_http_info.nResponseCode = HTTP_BAD_REQUEST;
         else
            {
            USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("edit_ap.tmpl")),
                                              "Firenze WiFi", 0, 0,
                                              U_CONSTANT_TO_TRACE("ap"),             "",
                                              U_CONSTANT_TO_TRACE("10.8.0.xxx"),     "",
                                              U_CONSTANT_TO_TRACE("aaa-r29587_bbb"), "",
                                              0, "",
                                              0, "",
                                              "checked",
                                              "checked");
            }
         }
      else
         {
         USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("edit_ap.tmpl")),
                                           "Firenze WiFi", 0, 0,
                                           U_STRING_TO_TRACE(ap_rec->_label),       "readonly",
                                           U_STRING_TO_TRACE(*address),             "readonly",
                                           U_STRING_TO_TRACE(nodog_rec->_hostname), "readonly",
                                           U_STRING_TO_TRACE(ap_rec->mac_mask),
                                           U_STRING_TO_TRACE(ap_rec->group_account),
                                           nodog_rec->down   ? "" : "checked",
                                           ap_rec->noconsume ? "" : "checked");
         }
   }
   
   static void GET_error_ap()
   {
      U_TRACE(5, "::GET_error_ap()")
   
      // $1 -> ap (without localization => '@')
      // $2 -> public address to contact the access point
   
      if (setAccessPoint(false))
         {
         U_LOGGER("*** ON AP(%.*s:%.*s) THE FIREWALL IS NOT ALIGNED ***", U_STRING_TO_TRACE(*address), U_STRING_TO_TRACE(*hostname));
         }
   
   #ifndef U_MANAGED_BY_MAIN_BASH
      USSIPlugIn::setAlternativeResponse(UString::getStringNull());
   #endif
   }
   
   static void GET_get_config()
   {
      U_TRACE(5, "::GET_get_config()")
   
      // $1 -> ap (without localization => '@')
      // $2 -> key
   
      UString body;
      uint32_t end = UHTTP::processHTTPForm();
   
      if (end)
         {
         UString key;
   
         UHTTP::getFormValue(key, U_CONSTANT_TO_PARAM("key"), 0, 3, end);
   
         if (key.empty() == false)
            {
            UString pathname(U_CAPACITY);
   
            pathname.snprintf("%w/ap/%.*s/nodog.conf", U_STRING_TO_TRACE(key));
   
            body = UFile::contentOf(pathname);
   
            if (body.empty())
               {
               body = *nodog_conf;
   
               *ip = (u_isIPv4Addr(U_STRING_TO_PARAM(key)) ? key : *client_address);
   
               U_ASSERT(vec->empty())
   
               vec->split(*ip, '.');
   
               key = (*vec)[3]; // 10.8.0.54
   
               vec->clear();
   
               pathname.snprintf("%w/ap/%.*s/nodog.conf.local", U_STRING_TO_TRACE(*ip));
   
               UString local = UFile::contentOf(pathname);
   
               uint32_t    len = U_CONSTANT_SIZE("???");
               const char* lan =                 "???";
   
               UHTTP::getFormValue(*ap, U_CONSTANT_TO_PARAM("ap"), 0, 1, end);
   
               if (ap->empty() == false)
                  {
                  if (UStringExt::endsWith(*ap, U_CONSTANT_TO_PARAM("_picoM2")) ||
                      UStringExt::endsWith(*ap, U_CONSTANT_TO_PARAM("_locoM2")))
                     {
                     lan =                 "wlan0";
                     len = U_CONSTANT_SIZE("wlan0");
                     }
                  else if (UStringExt::endsWith(*ap, U_CONSTANT_TO_PARAM("_rspro")))
                     {
                     if (UStringExt::startsWith(*ap, U_CONSTANT_TO_PARAM("wimo")))
                        {
                        lan =                 "br-lan";
                        len = U_CONSTANT_SIZE("br-lan");
                        }
                     else
                        {
                        lan =                 "eth1";
                        len = U_CONSTANT_SIZE("eth1");
                        }
                     }
                  else if (UStringExt::endsWith(*ap, U_CONSTANT_TO_PARAM("_x86")))
                     {
                     lan =                 "eth1";
                     len = U_CONSTANT_SIZE("eth1");
                     }
                  }
   
               body = UStringExt::substitute(body, U_CONSTANT_TO_PARAM("<LAN>"),                                              lan, len);
               body = UStringExt::substitute(body, U_CONSTANT_TO_PARAM("<DDD>"),                                              U_STRING_TO_PARAM(key));
               body = UStringExt::substitute(body, U_CONSTANT_TO_PARAM("#include \"ap/<AAA.BBB.CCC.DDD>/nodog.conf.local\""), U_STRING_TO_PARAM(local));
               body = UStringExt::substitute(body, U_CONSTANT_TO_PARAM("<AAA.BBB.CCC.DDD>"),                                  U_STRING_TO_PARAM(*ip));
   
               UFileConfig cfg(body, true);
   
               if (cfg.processData()) body = cfg.getData();
               }
            }
         }
   
      USSIPlugIn::setAlternativeResponse(body);
   }
   
   static void GET_get_users_info()
   {
      U_TRACE(5, "::GET_get_users_info()")
   
      if (*client_address != *ip_server) u_http_info.nResponseCode = HTTP_BAD_REQUEST;
      else
         {
         // $1 -> ap (without localization => '@')
         // $2 -> public address to contact the access point
   
         if (setAccessPoint(false))
            {
            UTimeVal to_sleep(U_TIME_FOR_ARPING_ASYNC_COMPLETION + 2, 0L);
   loop:
            (void) sendRequestToNodog("check", 0);
   
            if (client->responseCode() == HTTP_NO_CONTENT)
               {
               to_sleep.nanosleep();
   
               goto loop;
               }
            }
         else
            {
            U_ASSERT(vec->empty())
   
            callForAllAP(checkAccessPoint, vec);
   
            UString url(U_CAPACITY);
   
            for (int32_t i = 0, n = vec->size(); i < n; i += 2)
               {
               *address = (*vec)[i];
   
               nodog_rec->value._assign(vec->UVector<UStringRep*>::at(i+1));
   
               nodog_rec->fromString();
   
               url.snprintf("http://%.*s:%u/check", U_STRING_TO_TRACE(*address), nodog_rec->port);
   
               (void) sendRequestToNodog(url);
               }
   
            vec->clear();
            }
   
         USSIPlugIn::setAlternativeResponse(UString::getStringNull());
         }
   }
   
   static void GET_info()
   {
      U_TRACE(5, "::GET_info()")
   
   #ifndef U_MANAGED_BY_MAIN_BASH
      UString logout, connected, traffic;
      uint32_t end, num_args = UHTTP::processHTTPForm() / 2;
   
      U_INTERNAL_DUMP("num_args = %u", num_args)
   
      for (int32_t i = 0, n = num_args / 2; i < n; i += 8)
         {
         // ----------------------------------------------------------------------------------------------------------------------------------
         // $1 -> mac
         // $2 -> ip
         // $3 -> gateway
         // $4 -> ap (with localization => '@')
         // $5 -> => UID <=
         // $6 -> logout
         // $7 -> connected
         // $8 -> traffic
         // ----------------------------------------------------------------------------------------------------------------------------------
         // /info?Mac=00%3A14%3Aa5%3A6e%3A9c%3Acb&ip=172.16.1.172&gateway=172.16.1.254%3A5280&ap=00%4010.8.1.2&User=3343793489&logout=0&conn...
         // ----------------------------------------------------------------------------------------------------------------------------------
   
         end = i+16;
   
         UHTTP::getFormValue(*mac, U_CONSTANT_TO_PARAM("Mac"),  i, i+1, end);
         UHTTP::getFormValue(*ip,  U_CONSTANT_TO_PARAM("ip"),   i, i+3, end);
         UHTTP::getFormValue(*ap,  U_CONSTANT_TO_PARAM("ap"),   i, i+7, end);
         UHTTP::getFormValue(*uid, U_CONSTANT_TO_PARAM("User"), i, i+9, end);
   
         if (setAccessPointAddress()       == false ||
             checkIfUserConnected()        == false ||
             user_rec->setNodogReference() == false)
            {
            U_LOGGER("*** INFO ERROR: UID(%.*s) IP(%.*s) MAC(%.*s) AP(%.*s@%.*s) ***",
                     U_STRING_TO_TRACE(*uid), U_STRING_TO_TRACE(*ip), U_STRING_TO_TRACE(*mac),
                     U_STRING_TO_TRACE(*label), U_STRING_TO_TRACE(*address));
   
            continue;
            }
   
         if ((*ip  != user_rec->_ip) ||
             (*mac != user_rec->_mac))
            {
            U_LOGGER("*** INFO DIFFERENCE: AP(%.*s) CLIENT(%.*s) IP(%.*s=>%.*s) MAC(%.*s=>%.*s) ***",
                        U_STRING_TO_TRACE(*ap),  U_STRING_TO_TRACE(*uid),
                        U_STRING_TO_TRACE(*ip),  U_STRING_TO_TRACE(user_rec->_ip),
                        U_STRING_TO_TRACE(*mac), U_STRING_TO_TRACE(user_rec->_mac));
   
            continue;
            }
   
         UHTTP::getFormValue(logout,    U_CONSTANT_TO_PARAM("logout"),    i, i+11, end);
         UHTTP::getFormValue(connected, U_CONSTANT_TO_PARAM("connected"), i, i+13, end);
         UHTTP::getFormValue(traffic,   U_CONSTANT_TO_PARAM("traffic"),   i, i+15, end);
   
         user_rec->updateCounter(logout.strtol(), connected.strtol(), traffic.strtol());
         }
   
      USSIPlugIn::setAlternativeResponse(UString::getStringNull());
   #endif
   }
   
   static void GET_logged()
   {
      U_TRACE(5, "::GET_logged()")
   
   #ifndef U_MANAGED_BY_MAIN_BASH
      uid->clear();
   
      if (checkIfUserConnected()) isLogged();
      else                        USSIPlugIn::setAlternativeRedirect("http://www.google.com", 0);
   #endif
   }
   
   static void GET_logged_login_request()
   {
      U_TRACE(5, "::GET_logged_login_request()")
   
      USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("logged_login_request.tmpl")),
                                        "Firenze WiFi", 0, 0,
                                        0);
   }
   
   static void GET_login() // MAIN PAGE
   {
      U_TRACE(5, "::GET_login()")
   
      // -----------------------------------------------------------------------------------------------------------------------------------------------
      // NB: *** the params CAN be empty ***
      // -----------------------------------------------------------------------------------------------------------------------------------------------
      // $1 -> mac
      // $2 -> ip
      // $3 -> redirect
      // $4 -> gateway
      // $5 -> timeout
      // $6 -> token
      // $7 -> ap (with localization => '@')
      // -----------------------------------------------------------------------------------------------------------------------------------------------
      // GET /login?mac=00%3A14%3AA5%3A6E%3A9C%3ACB&ip=192.168.226.2&redirect=http%3A%2F%2Fgoogle&gateway=192.168.226.1%3A5280&timeout=0&token=x&ap=lab2
      // -----------------------------------------------------------------------------------------------------------------------------------------------
   
   #ifndef U_MANAGED_BY_MAIN_BASH
      if (UServer_Base::bssl)
         {
         X509* x509 = ((USSLSocket*)UServer_Base::pClientImage->socket)->getPeerCertificate();
   
         if (x509)
            {
            long serial    = UCertificate::getSerialNumber(x509);
            UString issuer = UCertificate::getIssuer(x509);
   
            if (askToLDAP(0, 0, 0,
                           "ldapsearch -LLL -b %.*s %.*s (&(objectClass=waUser)(&(waIssuer=%.*s)(waSerial=%ld)(waActive=TRUE)))",
                           U_STRING_TO_TRACE(*wiauth_user_basedn),
                           U_STRING_TO_TRACE(*ldap_user_param),
                           U_STRING_TO_TRACE(issuer), serial))
               {
               *uid         = (*table)["waUid"];
               *auth_domain = *cert_auth;
   
               table->clear();
   
               goto next;
               }
            }
         }
   
      if (checkLoginRequest(14)         &&
          mac->empty() == false         &&
          nodog_rec->setValue(*address) &&
          nodog_rec->findMAC())
         {
         *uid         = *mac;
         *auth_domain = *mac_auth;
   next:
         loadPolicy(*policy_flat);
   
         UString signed_data = UDES3::signData("uid=%.*s&policy=FLAT&auth_domain=%.*s&account=null&max_time=%.*s&max_traffic=%.*s",
                                          U_STRING_TO_TRACE(*uid),
                                          U_STRING_TO_TRACE(*auth_domain),
                                          U_STRING_TO_TRACE(*_time_available),
                                          U_STRING_TO_TRACE(*_traffic_available));
   
         // NB: in questo modo l'utente ripassa dal firewall e NoDog lo rimanda da noi (login_validate) con i dati rinnovati...
   
         USSIPlugIn::setAlternativeRedirect("http://www.google.com/login_validate?%.*s", U_STRING_TO_TRACE(signed_data));
   
         return;
         }
   
      setAccessPointReference();
   
      UString request(U_CAPACITY);
   
      request.snprintf("/login_request?%.*s", U_HTTP_QUERY_TO_TRACE);
   
      USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("login.tmpl")),
                                        "Firenze WiFi", 0, 0,
                                        url_banner_ap->data(), ap_ref->data(), help_url->data(), wallet_url->data(),
                                        ap->c_str(), request.data(), url_banner_comune->data(), ap_ref->data());
   #endif
   }
   
   static void GET_login_request()
   {
      U_TRACE(5, "::GET_login_request()")
   
      // -----------------------------------------------------------------------------
      // *** the params CAN be empty ***
      // -----------------------------------------------------------------------------
      // $1 -> mac
      // $2 -> ip
      // $3 -> redirect
      // $4 -> gateway
      // $5 -> timeout
      // $6 -> token
      // $7 -> ap (with localization => '@')
      // -----------------------------------------------------------------------------
   
      UString redirect, timeout;
      uint32_t end = UHTTP::processHTTPForm();
   
      if (end)
         {
         UHTTP::getFormValue(*mac,     U_CONSTANT_TO_PARAM("mac"),      0,  1, end);
         UHTTP::getFormValue(*ip,      U_CONSTANT_TO_PARAM("ip"),       0,  3, end);
         UHTTP::getFormValue(redirect, U_CONSTANT_TO_PARAM("redirect"), 0,  5, end);
         UHTTP::getFormValue(*gateway, U_CONSTANT_TO_PARAM("gateway"),  0,  7, end);
         UHTTP::getFormValue(timeout,  U_CONSTANT_TO_PARAM("timeout"),  0,  9, end);
         UHTTP::getFormValue(*token,   U_CONSTANT_TO_PARAM("token"),    0, 11, end);
         UHTTP::getFormValue(*ap,      U_CONSTANT_TO_PARAM("ap"),       0, 13, end);
         }
   
      USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("login_request.tmpl")),
                                        "Firenze WiFi", 0, 0,
                                        login_url->data(), mac->c_str(), ip->c_str(), redirect.c_str(),
                                        gateway->c_str(), timeout.c_str(), token->c_str(), ap->c_str());
   }
   
   static void GET_login_validate()
   {
      U_TRACE(5, "::GET_login_validate()")
   
      // ---------------------------------------------------------------------------------------------------
      // NB: come back from the gateway (NoDog) after the POST of login_request, the params CANNOT be empty
      // ---------------------------------------------------------------------------------------------------
      // $1 -> mac
      // $2 -> ip
      // $3 -> redirect
      // ========================
      // => uid
      // => policy
      // => auth_domain
      // => account
      // => max_time
      // => max_traffic
      // => UserDownloadRate 
      // => UserUploadRate 
      // ========================
      // $4 -> gateway
      // $5 -> timeout
      // $6 -> token
      // $7 -> ap (with localization => '@')
      // ---------------------------------------------------------------------------------------------------
   
   #ifndef U_MANAGED_BY_MAIN_BASH
      if (checkLoginRequest(14) == false)
         {
   error:
         loginWithProblem();
   
         return;
         }
   
      if (isUserConnected(true))
         {
         // Check if change of connection context for user id (RENEW)
   
         if ((*ip      == user_rec->_ip)  &&
             (*mac     == user_rec->_mac) &&
             (*address == user_rec->nodog))
            {
            USSIPlugIn::setMessagePage(*message_page_template, "Login", "Sei già loggato! (login_request)");
   
            return;
            }
   
         U_LOGGER("*** RENEW: UID(%.*s) IP(%.*s=>%.*s) MAC(%.*s=>%.*s) ADDRESS(%.*s=>%.*s) ***",
                     U_STRING_TO_TRACE(*uid),
                     U_STRING_TO_TRACE(*ip),      U_STRING_TO_TRACE(user_rec->_ip),
                     U_STRING_TO_TRACE(*mac),     U_STRING_TO_TRACE(user_rec->_mac),
                     U_STRING_TO_TRACE(*address), U_STRING_TO_TRACE(user_rec->nodog));
   
         if (askNodogToLogoutUser() == false) goto error;
         }
   
      if (user_rec->setRecord() == false) goto error;
   
      if (user_rec->consume)
         {
         if (user_rec->time_done >= user_rec->time_available)
            {
            USSIPlugIn::setMessagePage(*message_page_template, "Tempo consumato", "Hai consumato il tempo disponibile del servizio!");
   
            return;
            }
   
         if (user_rec->traffic_done >= user_rec->traffic_available)
            {
            USSIPlugIn::setMessagePage(*message_page_template, "Traffico consumato", "Hai consumato il traffico disponibile del servizio!");
   
            return;
            }
         }
   
      writeToLOG(auth_domain->c_str());
   
      // redirect back to the gateway appending a signed ticket that will signal NoDog to unlock the firewall...
   
      user_rec->setChunkValue();
   
      UString signed_data = UDES3::signData("\n"
         "Action Permit\n"
         "Mode Login\n"
         "Redirect http://" VIRTUAL_HOST "/postlogin?%.*s\n"
         "Mac %.*s\n"
         "Timeout %.*s\n"
         "Traffic %.*s\n"
         "Token %.*s\n"
         "User %.*s\n"
         "UserDownloadRate %.*s\n"
         "UserUploadRate %.*s\n",
         U_HTTP_QUERY_TO_TRACE,
         U_STRING_TO_TRACE(*mac),
         U_STRING_TO_TRACE(*_time_chunk),
         U_STRING_TO_TRACE(*_traffic_chunk),
         U_STRING_TO_TRACE(*token), U_STRING_TO_TRACE(*uid),
         U_STRING_TO_TRACE(*user_DownloadRate), U_STRING_TO_TRACE(*user_UploadRate));
   
      USSIPlugIn::setAlternativeRedirect("http://%.*s/ticket?ticket=%.*s", U_STRING_TO_TRACE(*gateway), U_STRING_TO_TRACE(signed_data));
   #endif
   }
   
   static void GET_logout()
   {
      U_TRACE(5, "::GET_logout()")
   
   #ifndef U_MANAGED_BY_MAIN_BASH
      checkLogoutRequest(true);
   #endif
   }
   
   static void GET_logout_page()
   {
      U_TRACE(5, "::GET_logout_page()")
   
      USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("logout_page.tmpl")),
                                        "Firenze WiFi", 0, 0,
                                        0);
   }
   
   static void GET_polling_attivazione()
   {
      U_TRACE(5, "::GET_polling_attivazione()")
   
      /*
      ---------------------------------------------------------------------------------------------------------------
      NB: we can manage this request with the main.bash script...
      ---------------------------------------------------------------------------------------------------------------
      */
   }
   
   static void GET_postlogin()
   {
      U_TRACE(5, "::GET_postlogin()")
   
   #ifndef U_MANAGED_BY_MAIN_BASH
      uint32_t num_args = UHTTP::processHTTPForm() / 2;
   
      U_INTERNAL_DUMP("num_args = %u", num_args)
   
      if (num_args == 1)
         {
         // ----------------------------
         // $1 -> uid
         // ----------------------------
   
         UHTTP::getFormValue(*uid, U_CONSTANT_TO_PARAM("uid"), 0, 1, 2);
   
         U_http_is_connection_close = U_YES;
   
         const char* _ptr = uid->c_str();
   
         USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("logout_popup.tmpl")),
                                           "Logout popup", 0, 0,
                                           _ptr, _ptr);
   
         return;
         }
   
      if (num_args != 7)
         {
   error:
         loginWithProblem();
   
         return;
         }
   
      if (checkLoginRequest(14) == false) goto error;
   
      if (isUserConnected(false)) goto error;
   
      U_ASSERT_EQUALS(*address, user_rec->nodog)
   
      if ((*ip  != user_rec->_ip) ||
          (*mac != user_rec->_mac))
         {
         U_LOGGER("*** POSTLOGIN DIFFERENCE: IP(%.*s=>%.*s) MAC(%.*s=>%.*s) ***",
                        U_STRING_TO_TRACE(*ip),  U_STRING_TO_TRACE(user_rec->_ip),
                        U_STRING_TO_TRACE(*mac), U_STRING_TO_TRACE(user_rec->_mac));
   
         goto error;
         }
   
      user_rec->setConnected(true);
   
      writeToLOG("LOGIN");
   
      // NB: send as response the message of waiting to redirect to google site...
   
      ptr1 = uid->c_str();
      ptr2 = redirect_default->c_str();
   
      UString _buffer(300U);
   
      _buffer.snprintf("onload=\"doOnLoad('postlogin?uid=%s','%s')\"", ptr1, ptr2); 
   
      U_http_is_connection_close = U_YES;
   
      USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("postlogin.tmpl")),
                                        "Firenze WiFi", "<script type=\"text/javascript\" src=\"js/logout_popup.js\"></script>", _buffer.data(),
                                        ptr1, ptr2, ptr2);
   #endif
   }
   
   static void GET_recovery()
   {
      U_TRACE(5, "::GET_recovery()")
   
   #ifndef U_MANAGED_BY_MAIN_BASH
      if (*ip_server == *client_address)
         {
         // ----------------------------
         // $1 -> uid
         // ----------------------------
   
         UHTTP::getFormValue(*uid, U_CONSTANT_TO_PARAM("user"), 0, 1, UHTTP::processHTTPForm());
   
         if (checkIfUserConnected()) (void) askNodogToLogoutUser();
   
         UString user = getUserName();
   
         u_printf2(file_RECOVERY->getFd(), "%6D %.*s \"%.*s\"\n", U_STRING_TO_TRACE(*uid), U_STRING_TO_TRACE(user));
   
         int result = db_user->remove(*uid);
   
         if (result) U_SRV_LOG("remove of user %.*s on db WiAuthUser failed with error %d", U_STRING_TO_TRACE(*uid), result);
   
         USSIPlugIn::setAlternativeResponse(UString::getStringNull());
         }
   #endif
   }
   
   static void GET_registrazione()
   {
      U_TRACE(5, "::GET_registrazione()")
   
      // $1 -> ap (with localization => '@')
   
      U_http_is_connection_close = U_YES;
   
      UString tutela_dati = cache->getContent(U_CONSTANT_TO_PARAM("tutela_dati.txt"));
   
      U_INTERNAL_ASSERT(tutela_dati.isNullTerminated())
   
      USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("registrazione.tmpl")),
                            "Registrazione utente",
                            "<link type=\"text/css\" href=\"css/livevalidation.css\" rel=\"stylesheet\">"
                            "<script type=\"text/javascript\" src=\"js/livevalidation_standalone.compressed.js\"></script>", 0,
                            registrazione_url->data(), tutela_dati.data());
   }
   
   static void GET_reset_policy()
   {
      U_TRACE(5, "::GET_reset_policy()")
   
      if (*client_address != *ip_server) u_http_info.nResponseCode = HTTP_BAD_REQUEST;
   #ifndef U_MANAGED_BY_MAIN_BASH
      else
         {
         *policy = U_STRING_FROM_CONSTANT("DAILY");
   
         callForAllUsers(resetUserPolicy);
   
         USSIPlugIn::setAlternativeResponse(UString::getStringNull());
         }
   #endif
   }
   
   static void GET_start_ap()
   {
      U_TRACE(5, "::GET_start_ap()")
   
      // $1 -> ap (with localization => '@')
      // $2 -> public address to contact the access point
      // $3 -> pid (0 => start)
   
      if (setAccessPoint(true))
         {
         UString pid(100U);
   
         UHTTP::getFormValue(pid, U_CONSTANT_TO_PARAM("pid"), 0, 5, UHTTP::form_name_value->size());
   
         U_LOGGER("%.*s:%.*s %s", U_STRING_TO_TRACE(*address), U_STRING_TO_TRACE(*hostname), (pid.strtol() ? "*** AP CRASHED ***" : "started"));
   
         callForAllUsers(quitUserConnected);
         }
   
   #ifndef U_MANAGED_BY_MAIN_BASH
      UString body = *allowed_web_hosts;
   
      USSIPlugIn::setAlternativeResponse(body);
   #endif
   }
   
   static void GET_stato_utente()
   {
      U_TRACE(5, "::GET_stato_utente()")
   
   #ifndef U_MANAGED_BY_MAIN_BASH
      uid->clear();
   
      if (checkIfUserConnected()        == false ||
          user_rec->setNodogReference() == false)
         {
   error:
         USSIPlugIn::setMessagePage(*message_page_template, "Utente non connesso", "Utente non connesso");
   
         return;
         }
   
      UString result = sendRequestToNodog("status?ip=%.*s", U_STRING_TO_TRACE(*client_address));
   
      if (result.empty() ||
          U_IS_HTTP_ERROR(client->responseCode()))
         {
         user_rec->setConnected(false);
   
         goto error;
         }
   
      UString x = user_rec->getAP(), user = getUserName();
   
      USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("stato_utente.tmpl")),
                                        "Stato utente", 0, 0,
                                        U_STRING_TO_TRACE(user),
                                        U_STRING_TO_TRACE(*uid), x.data(),
                                        U_STRING_TO_TRACE(result));
   #endif
   }
   
   static void GET_status_ap()
   {
      U_TRACE(5, "::GET_status_ap()")
   
      // $1 -> ap (with localization => '@')
      // $2 -> public address to contact the access point
   
      if (virtual_name->equal(U_HTTP_VHOST_TO_PARAM) == false) u_http_info.nResponseCode = HTTP_BAD_REQUEST;
      else
         {
         UString result;
   
         if (setAccessPoint(true)) result = sendRequestToNodog("status?label=%.*s", U_STRING_TO_TRACE(*label));
   
         if (result.empty() ||
             U_IS_HTTP_ERROR(client->responseCode()))
            {
            USSIPlugIn::setMessagePage(*message_page_template, "Servizio non disponibile",
                                                               "Servizio non disponibile (access point non contattabile). "
                                                               "Riprovare piu' tardi");
            }
         else
            {
            USSIPlugIn::setAlternativeResponse(result);
            }
         }
   }
   
   static void GET_status_network()
   {
      U_TRACE(5, "::GET_status_network()")
   
      if (*client_address != *ip_server) u_http_info.nResponseCode = HTTP_BAD_REQUEST;
   #ifndef U_MANAGED_BY_MAIN_BASH
      else
         {
         // $1 -> outfile
   
         UString outfile;
   
         UHTTP::getFormValue(outfile, U_CONSTANT_TO_PARAM("outfile"), 0, 1, UHTTP::processHTTPForm());
   
         UString _buffer(U_CAPACITY),
                 _template = cache->getContent(U_CONSTANT_TO_PARAM("status_network_head.tmpl"));
   
         U_INTERNAL_ASSERT(_template.isNullTerminated())
   
         users_connected = 0;
   
         output->setBuffer(U_CAPACITY);
   
         callForAllUsers(setStatusUser);
   
         _buffer.snprintf(_template.data(), num_ap, users_connected);
   
         (void) output->insert(0, _buffer);
   
         (void) UFile::writeTo(outfile, *output);
   
         USSIPlugIn::setAlternativeResponse(UString::getStringNull());
         }
   #endif
   }
   
   static void GET_status_nodog()
   {
      U_TRACE(5, "::GET_status_nodog()")
   
      if (*client_address != *ip_server) u_http_info.nResponseCode = HTTP_BAD_REQUEST;
      else
         {
         // $1 -> outfile
   
         UString outfile;
   
         UHTTP::getFormValue(outfile, U_CONSTANT_TO_PARAM("outfile"), 0, 1, UHTTP::processHTTPForm());
   
         UString _buffer(U_CAPACITY),
                 _template = cache->getContent(U_CONSTANT_TO_PARAM("status_nodog_head.tmpl"));
   
         U_INTERNAL_ASSERT(_template.isNullTerminated())
   
         output->setBuffer(U_CAPACITY);
   
         callForAllAPSorted(setStatusNodog, 0);
   
         _buffer.snprintf(_template.data(), num_ap, VIRTUAL_HOST);
   
         (void) output->insert(0, _buffer);
   
         (void) UFile::writeTo(outfile, *output);
   
         USSIPlugIn::setAlternativeResponse(UString::getStringNull());
         }
   }
   
   static void GET_unifi()
   {
      U_TRACE(5, "::GET_unifi()")
   
      USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("unifi_page.tmpl")),
                                        "Firenze WiFi", 0, 0,
                                        url_banner_ap->data(), help_url->data(), wallet_url->data(),
                                        "unifi", "/unifi_login_request", url_banner_comune->data());
   }
   
   static void GET_unifi_login_request()
   {
      U_TRACE(5, "::GET_unifi_login_request()")
   
      USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("unifi_login_request.tmpl")),
                                        "Firenze WiFi", 0, 0,
                                        0);
   }
   
   static void GET_view_user()
   {
      U_TRACE(5, "::GET_view_user()")
   
   #ifndef U_MANAGED_BY_MAIN_BASH
      if (*ip_server == *client_address)
         {
         // $1 -> uid
         // $2 -> outfile
   
         uint32_t end = UHTTP::processHTTPForm();
   
         if (end)
            {
            UString outfile;
   
            UHTTP::getFormValue(*uid,    U_CONSTANT_TO_PARAM("uid"),     0, 1, end);
            UHTTP::getFormValue(outfile, U_CONSTANT_TO_PARAM("outfile"), 0, 3, end);
   
            if (askToLDAP(0, "utente non registrato", "utente non registrato",
                          "ldapsearch -LLL -b %.*s %.*s waLogin=%.*s",
                          U_STRING_TO_TRACE(*wiauth_card_basedn),
                          U_STRING_TO_TRACE(*ldap_card_param),
                          U_STRING_TO_TRACE(*uid)) == false)
               {
               return;
               }
   
            UString WA_POLICY   = (*table)["waPolicy"],   // waPolicy: DAILY
                    WA_USEDBY   = (*table)["waUsedBy"],   // waUsedBy: 3343793489
                    WA_REVOKED  = (*table)["waRevoked"],  // waRevoked: FALSE
                    WA_VALIDITY = (*table)["waValidity"], // waValidity: 0
                    WA_PASSWORD = (*table)["waPassword"], // waPassword: {MD5}M7Bt9PlxMhHxcVd2HCVGcg==
                    WA_NOTAFTER = (*table)["waNotAfter"]; // waNotAfter: 20371231235959Z
   
            table->clear();
   
            const char* ptr;
            UString not_after(100U);
   
            if (WA_NOTAFTER.empty()) (void) not_after.assign(U_CONSTANT_TO_PARAM("Non disponibile"));
            else
               {
               ptr = WA_NOTAFTER.data();
   
               not_after.snprintf("%.2s/%.2s/%.4s - %.2s:%.2s", ptr+6, ptr+4, ptr, ptr+8, ptr+10);
               }
   
            const char* ptr5;
            const char* ptr6;
            const char* ptr7;
   
            if (WA_USEDBY.empty())
               {
               ptr1 = "red";
               ptr2 = "NO";
               }
            else
               {
               ptr1 = "green";
               ptr2 = "yes";
               }
   
            UString _buffer(U_CAPACITY), user,
                    _template = cache->getContent(U_CONSTANT_TO_PARAM("view_user.tmpl"));
   
            U_INTERNAL_ASSERT(_template.isNullTerminated())
   
            time_t last_modified;
            bool connected = checkIfUserConnected();
   
            user = getUserName(); // NB: must be after checkIfUserConnected()...
   
            if (user_exist)
               {
               last_modified = user_rec->last_modified + u_now_adjust;
   
               ptr = user.c_str();
   
               user_rec->getCounter();
   
               ptr3 =    _time_chunk->data();
               ptr4 = _traffic_chunk->data();
   
               user_rec->getConsumed();
   
               ptr5 =    _time_consumed->data();
               ptr6 = _traffic_consumed->data();
               ptr7 = (connected ? "yes" : "no");
               }
            else
               {
               last_modified = 0;
   
               ptr  = ptr7 = "";
               ptr3 = ptr4 = ptr5 = ptr6 = "Non disponibile";
               }
   
            _buffer.snprintf(_template.data(),
                            ptr, uid->c_str(),
                            ptr3, ptr4,
                            WA_PASSWORD.c_str(),
                            not_after.data(),
                            WA_VALIDITY.c_str(),
                            WA_REVOKED.c_str(),
                            WA_POLICY.c_str(),
                            ptr1, ptr2,
                            UStringExt::substr_count(file_RECOVERY->_getContent(), *uid),
                            ptr7, ptr5, ptr6, last_modified);
   
            (void) UFile::writeTo(outfile, _buffer);
   
            USSIPlugIn::setAlternativeResponse(UString::getStringNull());
            }
         }
   #endif
   }
   
   static void GET_webif_ap()
   {
      U_TRACE(5, "::GET_webif_ap()")
   
      // $1 -> ap (with localization => '@')
      // $2 -> public address to contact the access point
   
      if (virtual_name->equal(U_HTTP_VHOST_TO_PARAM) == false) u_http_info.nResponseCode = HTTP_BAD_REQUEST;
      else
         {
         if (setAccessPoint(true))
            {
            UString dest(U_CAPACITY);
   
            dest.snprintf("%.*s/client/%s:%u.srv", U_STRING_TO_TRACE(*dir_root), client_address->data(), UHTTP::getUserAgent());
   
            (void) UFile::writeTo(dest, *address);
   
            USSIPlugIn::setAlternativeRedirect("http://" VIRTUAL_HOST "/cgi-bin/webif/status-basic.sh?cat=Status", 0);
            }
         else
            {
            USSIPlugIn::setMessagePage(*message_page_template, "Servizio non disponibile",
                                                               "Servizio non disponibile (access point non contattabile). "
                                                               "Riprovare piu' tardi");
            }
         }
   }
   
   static const struct UHTTP::service_info GET_table[] = {
      GET_ENTRY(admin),
      GET_ENTRY(card_activation),
      GET_ENTRY(edit_ap),
      GET_ENTRY(error_ap),
      GET_ENTRY(get_config),
      GET_ENTRY(get_users_info),
      GET_ENTRY(info),
      GET_ENTRY(logged),
      GET_ENTRY(logged_login_request),
      GET_ENTRY(login),
      GET_ENTRY(login_request),
      GET_ENTRY(login_validate),
      GET_ENTRY(logout),
      GET_ENTRY(logout_page),
      GET_ENTRY(polling_attivazione),
      GET_ENTRY(postlogin),
      GET_ENTRY(recovery),
      GET_ENTRY(registrazione),
      GET_ENTRY(reset_policy),
      GET_ENTRY(start_ap),
      GET_ENTRY(stato_utente),
      GET_ENTRY(status_ap),
      GET_ENTRY(status_network),
      GET_ENTRY(status_nodog),
      GET_ENTRY(unifi),
      GET_ENTRY(unifi_login_request),
      GET_ENTRY(view_user),
      GET_ENTRY(webif_ap)
   };
   
   static void POST_edit_ap()
   {
      U_TRACE(5, "::POST_edit_ap()")
   
      // $1 -> ap_label
      // $2 -> ap_address
      // $3 -> ap_hostname
      // $4 -> ap_mac_mask
      // $5 -> ap_group_account
      // $6 -> ap_up
      // $7 -> ap_consume
      // $8 -> submit
   
      uint32_t end = UHTTP::processHTTPForm();
   
      if (end)
         {
         int ap_port = 5280;
         UString up, consume, _mac_mask;
   
         UHTTP::getFormValue(*label,    U_CONSTANT_TO_PARAM("ap_label"),         0, 1, end);
         UHTTP::getFormValue(*address,  U_CONSTANT_TO_PARAM("ap_address"),       0, 3, end);
         UHTTP::getFormValue(*hostname, U_CONSTANT_TO_PARAM("ap_hostname"),      0, 5, end);
         UHTTP::getFormValue(_mac_mask, U_CONSTANT_TO_PARAM("ap_mac_mask"),      0, 7, end);
         UHTTP::getFormValue(*account,  U_CONSTANT_TO_PARAM("ap_group_account"), 0, 9, end);
   
         UHTTP::getFormValue(up,        U_CONSTANT_TO_PARAM("ap_up"));
         UHTTP::getFormValue(consume,   U_CONSTANT_TO_PARAM("ap_consume"));
   
         U_INTERNAL_DUMP("address = %.*S hostname = %.*S label = %.*S", U_STRING_TO_TRACE(*address), U_STRING_TO_TRACE(*hostname), U_STRING_TO_TRACE(*label))
   
         if (u_isIPv4Addr(U_STRING_TO_PARAM(*address)) == false)
            {
            U_LOGGER("*** ADDRESS AP(%.*s) NOT VALID ***", U_STRING_TO_TRACE(*address));
   
            u_http_info.nResponseCode = HTTP_BAD_REQUEST;
   
            return;
            }
   
         if (u_isHostName(U_STRING_TO_PARAM(*hostname)) == false)
            {
            U_LOGGER("*** AP HOSTNAME(%.*s) NOT VALID ***", U_STRING_TO_TRACE(*hostname));
   
            u_http_info.nResponseCode = HTTP_BAD_REQUEST;
   
            return;
            }
   
         (void) nodog_rec->setRecord(0, ap_port, up.empty(), consume.empty(), account->empty() == false, (_mac_mask.empty() ? 0 : &_mac_mask));
         }
   
      USSIPlugIn::setAlternativeRedirect("https://%.*s/admin_status_nodog", U_STRING_TO_TRACE(*ip_server));
   }
   
   static void POST_login_request()
   {
      U_TRACE(5, "::POST_login_request()")
   
      // ---------------------------------------------------------------------------------------------------
      // *** this params CAN be empty ***
      // ---------------------------------------------------------------------------------------------------
      // $1  -> mac
      // $2  -> ip
      // $3  -> redirect
      // $4  -> gateway
      // $5  -> timeout
      // $6  -> token
      // $7  -> ap (with localization => '@')
      // ---------------------------------------------------------------------------------------------------
      // *** this params CANNOT be empty ***
      // ---------------------------------------------------------------------------------------------------
      // $8  -> realm
      // $9  -> uid
      // $10 -> password
      // $11 -> bottone
      // ---------------------------------------------------------------------------------------------------
    
   #ifndef U_MANAGED_BY_MAIN_BASH
      if (checkLoginRequest(22) == false)
         {
         loginWithProblem();
   
         return;
         }
   
      UString realm;
   
      UHTTP::getFormValue(realm, U_CONSTANT_TO_PARAM("realm"), 0, 15, 22);
   
      if (realm != "10_piazze" &&
          realm != "auth_service")
         {
         USSIPlugIn::setMessagePage(*message_page_template, "Errore", "Errore Autorizzazione - dominio sconosciuto");
   
         return;
         }
   
      UString password;
   
      UHTTP::getFormValue(*uid,     U_CONSTANT_TO_PARAM("uid"),  0, 17, 22);
      UHTTP::getFormValue(password, U_CONSTANT_TO_PARAM("pass"), 0, 19, 22);
   
      if (    uid->empty() ||
          password.empty())
         {
         USSIPlugIn::setMessagePage(*message_page_template, "Impostare utente e/o password", "Impostare utente e/o password"); 
   
         return;
         }
   
      if (askToLDAP(0, "Utente e/o Password errato/i", "Credenziali errate!",
                    "ldapsearch -LLL -b %.*s %.*s waLogin=%.*s",
                    U_STRING_TO_TRACE(*wiauth_card_basedn),
                    U_STRING_TO_TRACE(*ldap_card_param),
                    U_STRING_TO_TRACE(*uid)) == false)
         {
         return;
         }
   
      UString password_on_ldap = (*table)["waPassword"]; // waPassword: {MD5}ciwjVccK0u68vqupEXFukQ==
   
      if (U_STRNEQ(password_on_ldap.data(), "{MD5}") == false)
         {
         // if realm is 'auth_service' and not MD5 password check credential by AUTH command...
   
         if (realm == "10_piazze")
            {
            USSIPlugIn::setMessagePage(*message_page_template, "Utente e/o Password errato/i", "Credenziali errate!");
   
            return;
            }
   
         if (runAuthCmd(uid->c_str(), password.c_str()) == false) return;
   
         *policy      = U_STRING_FROM_CONSTANT("DAILY");
         *auth_domain = U_STRING_FROM_CONSTANT("AUTH_") + UStringExt::trim(*output);
         }
      else
         {
         UString passwd(33U);
   
         // Check 1: Wrong user and/or password
   
         UServices::generateDigest(U_HASH_MD5, 0, (unsigned char*)U_STRING_TO_PARAM(password), passwd, true);
   
         if (strncmp(password_on_ldap.c_pointer(U_CONSTANT_SIZE("{MD5}")), U_STRING_TO_PARAM(passwd)))
            {
            USSIPlugIn::setMessagePage(*message_page_template, "Utente e/o Password errato/i", "Credenziali errate!");
   
            return;
            }
   
         // Check 2: Activation required
   
         if ((*table)["waUsedBy"].empty()) // waUsedBy: 3343793489
            {
            USSIPlugIn::setMessagePage(*message_page_template, "Attivazione non effettuata", "Per utilizzare il servizio e' richiesta l'attivazione");
   
            return;
            }
   
         // Check 3: Card revoked
   
         if ((*table)["waRevoked"] != "FALSE") // waRevoked: FALSE
            {
            USSIPlugIn::setMessagePage(*message_page_template, "Carta revocata", "La tua carta e' revocata!");
   
            return;
            }
   
         UString NOT_AFTER = (*table)["waNotAfter"]; // waNotAfter: 20371231235959Z
   
         if (NOT_AFTER.empty() == false)
            {
            // Check 4: Expired validity
   
            if (UTimeDate::getSecondFromTime(NOT_AFTER.data(), true, "%4u%2u%2u%2u%2u%2uZ") <= u_now->tv_sec)
               {
               USSIPlugIn::setMessagePage(*message_page_template, "Validita' scaduta", "La tua validita' e' scaduta!");
   
               return;
               }
   
            *auth_domain = U_STRING_FROM_CONSTANT("PASS_AUTH");
            }
         else
            {
            *auth_domain = U_STRING_FROM_CONSTANT("FIRST_PASS_AUTH");
   
            // Update card with a new generated waNotAfter
   
            UString DN       = (*table)["dn"],         // dn: waCid=80e415bc-4be0-4385-85ee-970aa1f52ef6,ou=cards,o=unwired-portal
                    VALIDITY = (*table)["waValidity"]; // waValidity: 0
   
            if (VALIDITY == "0") NOT_AFTER = U_STRING_FROM_CONSTANT("20371231235959Z");
            else
               {
               UTimeDate t;
   
               t.addDays(VALIDITY.strtol());
   
               NOT_AFTER = t.strftime("%Y%m%d%H%M%SZ");
               }
   
            UString input(U_CAPACITY);
   
            input.snprintf("dn: %.*s\n"
                           "changetype: modify\n"
                           "add: waNotAfter\n"
                           "waNotAfter: %.*s\n"
                           "-",
                           U_STRING_TO_TRACE(DN),
                           U_STRING_TO_TRACE(NOT_AFTER));
   
            if (askToLDAP(&input, "Errore", "LDAP error", "ldapmodify -c %.*s", U_STRING_TO_TRACE(*ldap_card_param)) == false) return;
            }
   
         *policy = (*table)["waPolicy"]; // waPolicy: DAILY
   
         *user_DownloadRate = U_STRING_FROM_CONSTANT("0");
         *user_UploadRate   = U_STRING_FROM_CONSTANT("0");
         }
   
      if ( ip->empty() == false &&
          *ip != *client_address)
         {
         U_LOGGER("*** PARAM IP(%.*s) FROM AP(%.*s) IS DIFFERENT FROM CLIENT ADDRESS(%.*s) ***",
                     U_STRING_TO_TRACE(*ip), U_STRING_TO_TRACE(*ap), U_STRING_TO_TRACE(*client_address));
         }
   
      loadPolicy(*policy);
   
      account->snprintf("%.*s:%.*s", U_STRING_TO_TRACE(*uid), U_STRING_TO_TRACE(password));
   
      UString signed_data = UDES3::signData("uid=%.*s&policy=%.*s&auth_domain=%.*s&account=%.*s&"
                                            "max_time=%.*s&max_traffic=%.*s&UserDownloadRate=%.*s&UserUploadRate=%.*s",
                                             U_STRING_TO_TRACE(*uid),
                                             U_STRING_TO_TRACE(*policy),
                                             U_STRING_TO_TRACE(*auth_domain),
                                             U_STRING_TO_TRACE(*account),
                                             U_STRING_TO_TRACE(*_time_available),
                                             U_STRING_TO_TRACE(*_traffic_available),
                                             U_STRING_TO_TRACE(*user_DownloadRate),
                                             U_STRING_TO_TRACE(*user_UploadRate));
   
      // NB: in questo modo l'utente ripassa dal firewall e NoDog lo rimanda da noi (login_validate) con i dati rinnovati...
   
      USSIPlugIn::setAlternativeRedirect("http://www.google.com/login_validate?%.*s", U_STRING_TO_TRACE(signed_data));
   #endif
   }
   
   static void POST_logout()
   {
      U_TRACE(5, "::POST_logout()")
   
   #ifndef U_MANAGED_BY_MAIN_BASH
      checkLogoutRequest(false);
   #endif
   }
   
   static void POST_registrazione()
   {
      U_TRACE(5, "::POST_registrazione()")
   
      /*
      ---------------------------------------------------------------------------------------------------------------
      NB: we can manage this request with the main.bash script...
      ---------------------------------------------------------------------------------------------------------------
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
   
      uint32_t end = UHTTP::processHTTPForm();
   
      if (end)
         {
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
   
         if (password != password_conferma) USSIPlugIn::setMessagePage(*message_page_template, "Conferma Password errata", "Conferma Password errata");
         else
            {
            ....
   
            USSIPlugIn::setAlternativeInclude(cache->getContent(U_CONSTANT_TO_PARAM("post_registrazione.tmpl")),
                                              "Registrazione effettuata", 0, 0,
                                              caller_id->data(), password.c_str(), "polling_attivazione", caller_id->data(), password.c_str());
            }
         }
      ---------------------------------------------------------------------------------------------------------------
      */
   }
   
   static void POST_uploader()
   {
      U_TRACE(5, "::POST_uploader()")
   
      // $1 -> path file uploaded
   
      if (UHTTP::processHTTPForm())
         {
         UString tmpfile(100U);
   
         UHTTP::getFormValue(tmpfile, 1);
   
         if (tmpfile.empty() == false)
            {
            UString content = UFile::contentOf(tmpfile);
   
            U_INTERNAL_ASSERT(tmpfile.isNullTerminated())
   
            (void) UFile::_unlink(tmpfile.data());
   
            if (content.size() > (2 * 1024))
               {
               UString dest(U_CAPACITY), basename = UStringExt::basename(tmpfile);
   
               dest.snprintf("%.*s/%.*s", U_STRING_TO_TRACE(*historical_log_dir), U_STRING_TO_TRACE(basename));
   
               (void) UFile::writeTo(dest, content);
               }
            }
         }
   
      USSIPlugIn::setAlternativeResponse(UString::getStringNull());
   }
   
   static const struct UHTTP::service_info POST_table[] = {
      POST_ENTRY(edit_ap),
      POST_ENTRY(login_request),
      POST_ENTRY(logout),
      POST_ENTRY(registrazione),
      POST_ENTRY(uploader)
   }; 
   
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
   
   UHTTP::manageRequest(request_uri,
                        client_address,
                         GET_table, sizeof( GET_table) / sizeof( GET_table[0]),
                        POST_table, sizeof(POST_table) / sizeof(POST_table[0]));
   
   return 0;
   
   U_RETURN(usp_as_service ? 0 : 200);
} }