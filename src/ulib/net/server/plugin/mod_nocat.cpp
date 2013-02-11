// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_nocat.cpp - this is a plugin nocat for userver
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/date.h>
#include <ulib/file_config.h>
#include <ulib/utility/des3.h>
#include <ulib/utility/uhttp.h>
#include <ulib/net/udpsocket.h>
#include <ulib/utility/base64.h>
#include <ulib/utility/escape.h>
#include <ulib/net/ipt_ACCOUNT.h>
#include <ulib/utility/services.h>
#include <ulib/utility/dir_walk.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/string_ext.h>
#include <ulib/net/server/client_image.h>
#include <ulib/net/server/plugin/mod_nocat.h>

#ifdef HAVE_STRSTREAM_H
#  include <strstream.h>
#else
#  include <ulib/replace/strstream.h>
#endif

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_nocat, UNoCatPlugIn)
#endif

int                        UNoCatPlugIn::fd_stderr;
int                        UNoCatPlugIn::check_type;
vPF                        UNoCatPlugIn::unatexit;
bool                       UNoCatPlugIn::flag_check_system;
time_t                     UNoCatPlugIn::check_expire;
time_t                     UNoCatPlugIn::next_event_time;
time_t                     UNoCatPlugIn::last_request_check;
time_t                     UNoCatPlugIn::last_request_firewall;
time_t                     UNoCatPlugIn::time_available;
UPing**                    UNoCatPlugIn::sockp;
fd_set                     UNoCatPlugIn::addrmask;
fd_set*                    UNoCatPlugIn::paddrmask;
uint32_t                   UNoCatPlugIn::nfds;
uint32_t                   UNoCatPlugIn::num_radio;
uint32_t                   UNoCatPlugIn::total_connections;
uint64_t                   UNoCatPlugIn::traffic_available;
UString*                   UNoCatPlugIn::ip;
UString*                   UNoCatPlugIn::host;
UString*                   UNoCatPlugIn::input;
UString*                   UNoCatPlugIn::label;
UString*                   UNoCatPlugIn::ifname;
UString*                   UNoCatPlugIn::fw_env;
UString*                   UNoCatPlugIn::fw_cmd;
UString*                   UNoCatPlugIn::extdev;
UString*                   UNoCatPlugIn::intdev;
UString*                   UNoCatPlugIn::hostname;
UString*                   UNoCatPlugIn::localnet;
UString*                   UNoCatPlugIn::location;
UString*                   UNoCatPlugIn::auth_login;
UString*                   UNoCatPlugIn::decrypt_key;
UString*                   UNoCatPlugIn::login_timeout;
UString*                   UNoCatPlugIn::status_content;
UString*                   UNoCatPlugIn::allowed_members;
UCommand*                  UNoCatPlugIn::fw;
UCommand*                  UNoCatPlugIn::uclient;
UCommand*                  UNoCatPlugIn::uploader;
UIptAccount*               UNoCatPlugIn::ipt;
const UString*             UNoCatPlugIn::label_to_match;
UVector<Url*>*             UNoCatPlugIn::vauth_url;
UVector<Url*>*             UNoCatPlugIn::vinfo_url;
UVector<UString>*          UNoCatPlugIn::openlist;
UVector<UString>*          UNoCatPlugIn::vauth;
UVector<UString>*          UNoCatPlugIn::vauth_ip;
UVector<UString>*          UNoCatPlugIn::vLoginValidate;
UVector<UString>*          UNoCatPlugIn::vInternalDevice;
UVector<UString>*          UNoCatPlugIn::vLocalNetwork;
UVector<UString>*          UNoCatPlugIn::vLocalNetworkLabel;
UVector<UString>*          UNoCatPlugIn::vLocalNetworkGateway;
UVector<UIPAllow*>*        UNoCatPlugIn::vLocalNetworkMask;
UVector<UIPAddress*>**     UNoCatPlugIn::vaddr;
UHashMap<UModNoCatPeer*>*  UNoCatPlugIn::peers;

const UString* UNoCatPlugIn::str_ROUTE_ONLY;
const UString* UNoCatPlugIn::str_DNS_ADDR;
const UString* UNoCatPlugIn::str_INCLUDE_PORTS;
const UString* UNoCatPlugIn::str_EXCLUDE_PORTS;
const UString* UNoCatPlugIn::str_ALLOWED_WEB_HOSTS;
const UString* UNoCatPlugIn::str_EXTERNAL_DEVICE;
const UString* UNoCatPlugIn::str_INTERNAL_DEVICE;
const UString* UNoCatPlugIn::str_LOCAL_NETWORK_LABEL;
const UString* UNoCatPlugIn::str_LOCAL_NETWORK;
const UString* UNoCatPlugIn::str_AUTH_SERVICE_URL;
const UString* UNoCatPlugIn::str_LOGIN_TIMEOUT;
const UString* UNoCatPlugIn::str_DECRYPT_CMD;
const UString* UNoCatPlugIn::str_DECRYPT_KEY;
const UString* UNoCatPlugIn::str_FW_CMD;
const UString* UNoCatPlugIn::str_CHECK_TYPE;
const UString* UNoCatPlugIn::str_Action;
const UString* UNoCatPlugIn::str_Permit;
const UString* UNoCatPlugIn::str_Deny;
const UString* UNoCatPlugIn::str_Mode;
const UString* UNoCatPlugIn::str_Redirect;
const UString* UNoCatPlugIn::str_renew;
const UString* UNoCatPlugIn::str_Mac;
const UString* UNoCatPlugIn::str_Timeout;
const UString* UNoCatPlugIn::str_Token;
const UString* UNoCatPlugIn::str_User;
const UString* UNoCatPlugIn::str_allowed;
const UString* UNoCatPlugIn::str_anonymous;
const UString* UNoCatPlugIn::str_Traffic;
const UString* UNoCatPlugIn::str_Policy;
const UString* UNoCatPlugIn::str_Policy_FLAT;
const UString* UNoCatPlugIn::str_GATEWAY_PORT;
const UString* UNoCatPlugIn::str_FW_ENV;
const UString* UNoCatPlugIn::str_IPHONE_SUCCESS;
const UString* UNoCatPlugIn::str_CHECK_EXPIRE_INTERVAL;
const UString* UNoCatPlugIn::str_ALLOWED_MEMBERS;
const UString* UNoCatPlugIn::str_without_mac;
const UString* UNoCatPlugIn::str_without_label;
const UString* UNoCatPlugIn::str_allowed_members_default;
const UString* UNoCatPlugIn::str_UserDownloadRate;
const UString* UNoCatPlugIn::str_UserUploadRate;
const UString* UNoCatPlugIn::str_NoTraffic;

#define U_NOCAT_STATUS \
"<html>\n" \
"<head>\n" \
"<meta http-equiv=\"Cache Control\" content=\"max-age=0\">\n" \
"<title>Access Point: %s</title>" \
"</head>\n" \
"<body bgcolor=\"#FFFFFF\" text=\"#000000\">\n" \
"<h1>Access Point: %s</h1>\n" \
"<hr noshade=\"1\"/>\n" \
"<table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">\n" \
   "<tr><td>Current Time</td><td>%7D</td></tr>\n" \
   "<tr><td>Gateway Up Since</td><td>%#5D</td></tr>\n" \
   "<tr><td>GatewayVersion</td><td>" ULIB_VERSION "</td></tr>\n" \
   "<tr><td>ExternalDevice</td><td>%.*s</td></tr>\n" \
   "<tr><td>InternalDevice</td><td>%.*s</td></tr>\n" \
   "<tr><td>LocalNetwork</td><td>%.*s</td></tr>\n" \
   "<tr><td>GatewayPort</td><td>%u</td></tr>\n" \
   "<tr><td>AuthServiceAddr</td><td>%.*s</td></tr>\n" \
   "%s" \
   "</table>\n" \
   "<hr noshade=\"1\"/>\n" \
   "<table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">\n" \
   "<tr><td>Users</td><td>%u</td></tr>\n" \
   "<tr><td>Users Connected</td><td>%u</td></tr>\n" \
   "<tr><td></td><td></td></tr>\n" \
   "<tr><td align=\"center\"><h2>Current Users</h2></td>\n" \
   "<table border=\"1\" cellpadding=\"5\">\n" \
      "<tr><th>User UID</th>\n" \
      "<th>IP address</th>\n" \
      "<th>Connection time</th>\n" \
      "<th>Elapsed connection time</th>\n" \
      "<th>Left connection time</th>\n" \
      "<th>Consumed traffic</th>\n" \
      "<th>Left traffic</th>\n" \
      "<th>MAC address</th>\n" \
      "<th>Status</th></tr>\n" \
       "%.*s" \
   "</table></td></tr>\n" \
"</table>\n" \
"<hr noshade=\"1\"/>\n" \
"<img src=%s width=\"112\" height=\"35\">\n" \
"<p style=\"text-align:right\">Powered by ULib</p>\n" \
"</body>\n" \
"</html>"

UModNoCatPeer::UModNoCatPeer() : UEventTime(0L,0L), token(100U), gateway(100U)
{
   U_TRACE_REGISTER_OBJECT(0, UModNoCatPeer, "")

   ctime        = connected = expire = u_now->tv_sec;
   ctraffic     = time_no_traffic   = time_remain = logout = 0L;
   traffic_done = traffic_available = traffic_remain = 0ULL;

   (void) U_SYSCALL(memset,"%p,%d,%u", flag, 0, sizeof(flag));
}

// define method VIRTUAL of class UEventTime

int UModNoCatPeer::handlerTime()
{
   U_TRACE(0, "UModNoCatPeer::handlerTime()")

   if (checkPeerInfo(true) == false)
      {
      UString* tmp =
      UNoCatPlugIn::login_timeout;
      UNoCatPlugIn::login_timeout = 0;

      UNoCatPlugIn::deny(this, false);

      UNoCatPlugIn::login_timeout = tmp;
      }

   // return value:
   // ---------------
   // -1 - normal
   //  0 - monitoring
   // ---------------

   U_RETURN(-1);
}

void UNoCatPlugIn::str_allocate()
{
   U_TRACE(0, "UNoCatPlugIn::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_ROUTE_ONLY,0)
   U_INTERNAL_ASSERT_EQUALS(str_DNS_ADDR,0)
   U_INTERNAL_ASSERT_EQUALS(str_INCLUDE_PORTS,0)
   U_INTERNAL_ASSERT_EQUALS(str_EXCLUDE_PORTS,0)
   U_INTERNAL_ASSERT_EQUALS(str_ALLOWED_WEB_HOSTS,0)
   U_INTERNAL_ASSERT_EQUALS(str_EXTERNAL_DEVICE,0)
   U_INTERNAL_ASSERT_EQUALS(str_INTERNAL_DEVICE,0)
   U_INTERNAL_ASSERT_EQUALS(str_LOCAL_NETWORK,0)
   U_INTERNAL_ASSERT_EQUALS(str_LOCAL_NETWORK_LABEL,0)
   U_INTERNAL_ASSERT_EQUALS(str_AUTH_SERVICE_URL,0)
   U_INTERNAL_ASSERT_EQUALS(str_LOGIN_TIMEOUT,0)
   U_INTERNAL_ASSERT_EQUALS(str_FW_CMD,0)
   U_INTERNAL_ASSERT_EQUALS(str_DECRYPT_CMD,0)
   U_INTERNAL_ASSERT_EQUALS(str_DECRYPT_KEY,0)
   U_INTERNAL_ASSERT_EQUALS(str_CHECK_TYPE,0)
   U_INTERNAL_ASSERT_EQUALS(str_Action,0)
   U_INTERNAL_ASSERT_EQUALS(str_Permit,0)
   U_INTERNAL_ASSERT_EQUALS(str_Deny,0)
   U_INTERNAL_ASSERT_EQUALS(str_Mode,0)
   U_INTERNAL_ASSERT_EQUALS(str_Redirect,0)
   U_INTERNAL_ASSERT_EQUALS(str_renew,0)
   U_INTERNAL_ASSERT_EQUALS(str_Mac,0)
   U_INTERNAL_ASSERT_EQUALS(str_Timeout,0)
   U_INTERNAL_ASSERT_EQUALS(str_Token,0)
   U_INTERNAL_ASSERT_EQUALS(str_User,0)
   U_INTERNAL_ASSERT_EQUALS(str_allowed,0)
   U_INTERNAL_ASSERT_EQUALS(str_anonymous,0)
   U_INTERNAL_ASSERT_EQUALS(str_Traffic,0)
   U_INTERNAL_ASSERT_EQUALS(str_Policy,0)
   U_INTERNAL_ASSERT_EQUALS(str_Policy_FLAT,0)
   U_INTERNAL_ASSERT_EQUALS(str_GATEWAY_PORT,0)
   U_INTERNAL_ASSERT_EQUALS(str_FW_ENV,0)
   U_INTERNAL_ASSERT_EQUALS(str_IPHONE_SUCCESS,0)
   U_INTERNAL_ASSERT_EQUALS(str_CHECK_EXPIRE_INTERVAL,0)
   U_INTERNAL_ASSERT_EQUALS(str_ALLOWED_MEMBERS,0)
   U_INTERNAL_ASSERT_EQUALS(str_without_mac,0)
   U_INTERNAL_ASSERT_EQUALS(str_without_label,0)
   U_INTERNAL_ASSERT_EQUALS(str_allowed_members_default,0)
   U_INTERNAL_ASSERT_EQUALS(str_UserDownloadRate,0)
   U_INTERNAL_ASSERT_EQUALS(str_UserUploadRate,0)
   U_INTERNAL_ASSERT_EQUALS(str_NoTraffic,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("RouteOnly") },
      { U_STRINGREP_FROM_CONSTANT("DNSAddr") },
      { U_STRINGREP_FROM_CONSTANT("IncludePorts") },
      { U_STRINGREP_FROM_CONSTANT("ExcludePorts") },
      { U_STRINGREP_FROM_CONSTANT("AllowedWebHosts") },
      { U_STRINGREP_FROM_CONSTANT("ExternalDevice") },
      { U_STRINGREP_FROM_CONSTANT("InternalDevice") },
      { U_STRINGREP_FROM_CONSTANT("LOCAL_NETWORK_LABEL") },
      { U_STRINGREP_FROM_CONSTANT("LocalNetwork") },
      { U_STRINGREP_FROM_CONSTANT("AuthServiceAddr") },
      { U_STRINGREP_FROM_CONSTANT("LOGIN_TIMEOUT") },
      { U_STRINGREP_FROM_CONSTANT("FW_CMD") },
      { U_STRINGREP_FROM_CONSTANT("DECRYPT_CMD") },
      { U_STRINGREP_FROM_CONSTANT("DECRYPT_KEY") },
      { U_STRINGREP_FROM_CONSTANT("CHECK_TYPE") },
      { U_STRINGREP_FROM_CONSTANT("Action") },
      { U_STRINGREP_FROM_CONSTANT("Permit") },
      { U_STRINGREP_FROM_CONSTANT("Deny") },
      { U_STRINGREP_FROM_CONSTANT("Mode") },
      { U_STRINGREP_FROM_CONSTANT("Redirect") },
      { U_STRINGREP_FROM_CONSTANT("renew") },
      { U_STRINGREP_FROM_CONSTANT("Mac") },
      { U_STRINGREP_FROM_CONSTANT("Timeout") },
      { U_STRINGREP_FROM_CONSTANT("Token") },
      { U_STRINGREP_FROM_CONSTANT("User") },
      { U_STRINGREP_FROM_CONSTANT("allowed") },
      { U_STRINGREP_FROM_CONSTANT("anonymous") },
      { U_STRINGREP_FROM_CONSTANT("Traffic") },
      { U_STRINGREP_FROM_CONSTANT("Policy") },
      { U_STRINGREP_FROM_CONSTANT("FLAT") },
      { U_STRINGREP_FROM_CONSTANT("GatewayPort") },
      { U_STRINGREP_FROM_CONSTANT("CHECK_EXPIRE_INTERVAL") },
      { U_STRINGREP_FROM_CONSTANT("FW_ENV") },
      { U_STRINGREP_FROM_CONSTANT("ALLOWED_MEMBERS") },
      { U_STRINGREP_FROM_CONSTANT("00:00:00:00:00:00") },
      { U_STRINGREP_FROM_CONSTANT("without_label") },
      { U_STRINGREP_FROM_CONSTANT("/etc/nodog.allowed") },
      { U_STRINGREP_FROM_CONSTANT("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">\n"
                                  "<HTML>\n"
                                  "<HEAD>\n"
                                  "   <TITLE>Success</TITLE>\n"
                                  "</HEAD>\n"
                                  "<BODY>\n"
                                  "Success\n"
                                  "</BODY>\n"
                                  "</HTML>") },
      { U_STRINGREP_FROM_CONSTANT("UserDownloadRate") },
      { U_STRINGREP_FROM_CONSTANT("UserUploadRate") },
      { U_STRINGREP_FROM_CONSTANT("NoTraffic") }
   };

   U_NEW_ULIB_OBJECT(str_ROUTE_ONLY,              U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_DNS_ADDR,                U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_INCLUDE_PORTS,           U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_EXCLUDE_PORTS,           U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_ALLOWED_WEB_HOSTS,       U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_EXTERNAL_DEVICE,         U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_INTERNAL_DEVICE,         U_STRING_FROM_STRINGREP_STORAGE(6));
   U_NEW_ULIB_OBJECT(str_LOCAL_NETWORK_LABEL,     U_STRING_FROM_STRINGREP_STORAGE(7));
   U_NEW_ULIB_OBJECT(str_LOCAL_NETWORK,           U_STRING_FROM_STRINGREP_STORAGE(8));
   U_NEW_ULIB_OBJECT(str_AUTH_SERVICE_URL,        U_STRING_FROM_STRINGREP_STORAGE(9));
   U_NEW_ULIB_OBJECT(str_LOGIN_TIMEOUT,           U_STRING_FROM_STRINGREP_STORAGE(10));
   U_NEW_ULIB_OBJECT(str_FW_CMD,                  U_STRING_FROM_STRINGREP_STORAGE(11));
   U_NEW_ULIB_OBJECT(str_DECRYPT_CMD,             U_STRING_FROM_STRINGREP_STORAGE(12));
   U_NEW_ULIB_OBJECT(str_DECRYPT_KEY,             U_STRING_FROM_STRINGREP_STORAGE(13));
   U_NEW_ULIB_OBJECT(str_CHECK_TYPE,              U_STRING_FROM_STRINGREP_STORAGE(14));
   U_NEW_ULIB_OBJECT(str_Action,                  U_STRING_FROM_STRINGREP_STORAGE(15));
   U_NEW_ULIB_OBJECT(str_Permit,                  U_STRING_FROM_STRINGREP_STORAGE(16));
   U_NEW_ULIB_OBJECT(str_Deny,                    U_STRING_FROM_STRINGREP_STORAGE(17));
   U_NEW_ULIB_OBJECT(str_Mode,                    U_STRING_FROM_STRINGREP_STORAGE(18));
   U_NEW_ULIB_OBJECT(str_Redirect,                U_STRING_FROM_STRINGREP_STORAGE(19));
   U_NEW_ULIB_OBJECT(str_renew,                   U_STRING_FROM_STRINGREP_STORAGE(20));
   U_NEW_ULIB_OBJECT(str_Mac,                     U_STRING_FROM_STRINGREP_STORAGE(21));
   U_NEW_ULIB_OBJECT(str_Timeout,                 U_STRING_FROM_STRINGREP_STORAGE(22));
   U_NEW_ULIB_OBJECT(str_Token,                   U_STRING_FROM_STRINGREP_STORAGE(23));
   U_NEW_ULIB_OBJECT(str_User,                    U_STRING_FROM_STRINGREP_STORAGE(24));
   U_NEW_ULIB_OBJECT(str_allowed,                 U_STRING_FROM_STRINGREP_STORAGE(25));
   U_NEW_ULIB_OBJECT(str_anonymous,               U_STRING_FROM_STRINGREP_STORAGE(26));
   U_NEW_ULIB_OBJECT(str_Traffic,                 U_STRING_FROM_STRINGREP_STORAGE(27));
   U_NEW_ULIB_OBJECT(str_Policy,                  U_STRING_FROM_STRINGREP_STORAGE(28));
   U_NEW_ULIB_OBJECT(str_Policy_FLAT,             U_STRING_FROM_STRINGREP_STORAGE(29));
   U_NEW_ULIB_OBJECT(str_GATEWAY_PORT,            U_STRING_FROM_STRINGREP_STORAGE(30));
   U_NEW_ULIB_OBJECT(str_CHECK_EXPIRE_INTERVAL,   U_STRING_FROM_STRINGREP_STORAGE(31));
   U_NEW_ULIB_OBJECT(str_FW_ENV,                  U_STRING_FROM_STRINGREP_STORAGE(32));
   U_NEW_ULIB_OBJECT(str_ALLOWED_MEMBERS,         U_STRING_FROM_STRINGREP_STORAGE(33));
   U_NEW_ULIB_OBJECT(str_without_mac,             U_STRING_FROM_STRINGREP_STORAGE(34));
   U_NEW_ULIB_OBJECT(str_without_label,           U_STRING_FROM_STRINGREP_STORAGE(35));
   U_NEW_ULIB_OBJECT(str_allowed_members_default, U_STRING_FROM_STRINGREP_STORAGE(36));
   U_NEW_ULIB_OBJECT(str_IPHONE_SUCCESS,          U_STRING_FROM_STRINGREP_STORAGE(37));
   U_NEW_ULIB_OBJECT(str_UserDownloadRate,        U_STRING_FROM_STRINGREP_STORAGE(38));
   U_NEW_ULIB_OBJECT(str_UserUploadRate,          U_STRING_FROM_STRINGREP_STORAGE(39));
   U_NEW_ULIB_OBJECT(str_NoTraffic,               U_STRING_FROM_STRINGREP_STORAGE(40));
}

UNoCatPlugIn::UNoCatPlugIn()
{
   U_TRACE_REGISTER_OBJECT(0, UNoCatPlugIn, "")

   fw              = U_NEW(UCommand);
   uclient         = U_NEW(UCommand);
   uploader        = U_NEW(UCommand);

   vauth_url       = U_NEW(UVector<Url*>(4U));
   vinfo_url       = U_NEW(UVector<Url*>(4U));

   ip              = U_NEW(UString);
   host            = U_NEW(UString(100U));
   label           = U_NEW(UString);
   input           = U_NEW(UString(U_CAPACITY));
   ifname          = U_NEW(UString);
   fw_cmd          = U_NEW(UString);
   fw_env          = U_NEW(UString);
   extdev          = U_NEW(UString);
   intdev          = U_NEW(UString);
   hostname        = U_NEW(UString);
   localnet        = U_NEW(UString);
   location        = U_NEW(UString(U_CAPACITY));
   auth_login      = U_NEW(UString);
   decrypt_key     = U_NEW(UString);
   allowed_members = U_NEW(UString);

   vauth                = U_NEW(UVector<UString>(4U));
   vauth_ip             = U_NEW(UVector<UString>(4U));
   vLocalNetwork        = U_NEW(UVector<UString>(64U));
   vLoginValidate       = U_NEW(UVector<UString>);
   vInternalDevice      = U_NEW(UVector<UString>(64U));
   vLocalNetworkMask    = U_NEW(UVector<UIPAllow*>);
   vLocalNetworkLabel   = U_NEW(UVector<UString>(64U));
   vLocalNetworkGateway = U_NEW(UVector<UString>(64U));

   if (str_AUTH_SERVICE_URL == 0) str_allocate();
}

UNoCatPlugIn::~UNoCatPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, UNoCatPlugIn)

   if (login_timeout ||
       UTimeVal::notZero())
      {
      UTimer::stop();
      UTimer::clear(true);
      }

   delete fw;
   delete uclient;
   delete uploader;
   delete vauth_url;
   delete vinfo_url;

   delete ip;
   delete host;
   delete label;
   delete input;
   delete ifname;
   delete fw_cmd;
   delete fw_env;
   delete extdev;
   delete intdev;
   delete localnet;
   delete location;
   delete hostname;
   delete auth_login;
   delete decrypt_key;
   delete allowed_members;

   delete vauth;
   delete vauth_ip;
   delete vLocalNetwork;
   delete vLoginValidate;
   delete vInternalDevice;
   delete vLocalNetworkMask;
   delete vLocalNetworkLabel;
   delete vLocalNetworkGateway;

   if (ipt) delete ipt;

   if (peers)
      {
      peers->clear();
      peers->deallocate();

      delete peers;
      }

   if (status_content)
      {
      delete openlist;
      delete status_content;
      }

   if (vaddr)
      {
      for (uint32_t i = 0; i < num_radio; ++i)
         {
         delete vaddr[i];
         delete sockp[i];
         }

      UMemoryPool::_free(sockp, num_radio, sizeof(UPing*));
      UMemoryPool::_free(vaddr, num_radio, sizeof(UVector<UIPAddress*>*));
      }
}

// status

void UNoCatPlugIn::getPeerStatus(UStringRep* key, void* value)
{
   U_TRACE(0, "UNoCatPlugIn::getPeerStatus(%p,%p)", key, value)

   UModNoCatPeer* peer = (UModNoCatPeer*) value;

   U_INTERNAL_ASSERT(peer->mac.isNullTerminated())

   if ( label_to_match &&
       *label_to_match != peer->label)
      {
      return;
      }

   const char* color;
   const char* status;
   time_t how_much_connected;
   UString buffer(U_CAPACITY);
   const char* mac = peer->mac.data();

   (void) peer->checkPeerInfo((key == 0));

   U_INTERNAL_DUMP("now    = %#5D", u_now->tv_sec)
   U_INTERNAL_DUMP("logout = %#5D", peer->logout)
   U_INTERNAL_DUMP("expire = %#5D", peer->expire)

   if (U_peer_status(peer) == UModNoCatPeer::PEER_PERMIT)
      {
      color  = "green";
      status = "PERMIT";

      how_much_connected = u_now->tv_sec - peer->connected;
      }
   else
      {
      color  = "red";
      status = "DENY";

      how_much_connected = peer->logout - peer->connected;
      }

   char c;
   uint64_t traffic_remain = peer->traffic_remain /1024;

   U_INTERNAL_DUMP("traffic_remain = %llu", traffic_remain)

   if (traffic_remain < 1024) c = 'K';
   else
      {
      c = 'M';

      traffic_remain /= 1024;
      }

   buffer.snprintf("<tr>\n"
                     "<td>%.*s</td>\n"
                     "<td>%s</td>\n"
                     "<td>%#5D</td>\n"
                     "<td>%#2D</td>\n"
                     "<td>%#2D</td>\n"
                     "<td>%llu KBytes</td>\n"
                     "<td>%llu %cBytes</td>\n"
                     "<td><a href=\"http://standards.ieee.org/cgi-bin/ouisearch?%c%c%c%c%c%c\">%s</a></td>\n"
                     "<td style=\"color:%s\">%s</td>\n"
                   "</tr>\n",
                   U_STRING_TO_TRACE(peer->user),
                   peer->UIPAddress::pcStrAddress,
                   peer->connected + u_now_adjust,
                   how_much_connected,
                   peer->time_remain,
                   peer->traffic_done / 1024,
                   traffic_remain, c,
                   mac[0], mac[1], mac[3], mac[4], mac[6], mac[7], mac,
                   color, status);

   (void) status_content->append(buffer);
}

void UNoCatPlugIn::getPeerListInfo(UStringRep* key, void* value)
{
   U_TRACE(0, "UNoCatPlugIn::getPeerListInfo(%p,%p)", key, value)

   UModNoCatPeer* peer = (UModNoCatPeer*) value;

   if (U_peer_allowed(peer) == false &&
       U_peer_status( peer) == UModNoCatPeer::PEER_PERMIT)
      {
      U_INTERNAL_ASSERT(u_now->tv_sec >= peer->ctime)

      UString buffer(U_CAPACITY);

      buffer.snprintf("%.*s %s %.*s %ld %llu\n", U_STRING_TO_TRACE(peer->user), peer->UIPAddress::pcStrAddress,
                                                 U_STRING_TO_TRACE(peer->mac), u_now->tv_sec - peer->ctime, peer->traffic_done);

      (void) status_content->append(buffer);
      }
}

void UNoCatPlugIn::setPeerListInfo()
{
   U_TRACE(0, "UNoCatPlugIn::setPeerListInfo()")

   status_content->setEmpty();

   peers->callForAllEntry(getPeerListInfo);

   setHTTPResponse(*status_content);
}

void UNoCatPlugIn::setStatusContent(UModNoCatPeer* peer, const UString& _label)
{
   U_TRACE(0, "UNoCatPlugIn::setStatusContent(%p,%.*S)", peer, U_STRING_TO_TRACE(_label))

   status_content->setEmpty();

   label_to_match = (_label.empty() ? 0 : &_label);

   if (peer) getPeerStatus(0, peer);
   else
      {
      getTraffic();

      peers->callForAllEntry(getPeerStatus);

      U_INTERNAL_ASSERT(hostname->isNullTerminated())

      const char* name = hostname->data();

      UString label_buffer(100U);

      if (label_to_match)
         {
         uint32_t index = vLocalNetworkLabel->contains(_label);

         if (index < vLocalNetwork->size())
            {
            UString x = (*vLocalNetwork)[index];

            label_buffer.snprintf("<tr><td>Label (Local Network Mask)</td><td>%.*s (%.*s)</td></tr>\n", U_STRING_TO_TRACE(_label), U_STRING_TO_TRACE(x));
            }
         }

      UString buffer(1024U + sizeof(U_NOCAT_STATUS) + intdev->size() + localnet->size() + status_content->size() + label_buffer.size());

      buffer.snprintf(U_NOCAT_STATUS, name, name, u_start_time,
                      U_STRING_TO_TRACE(*extdev),
                      U_STRING_TO_TRACE(*intdev),
                      U_STRING_TO_TRACE(*localnet),
                      UServer_Base::port,
                      U_STRING_TO_TRACE(*auth_login),
                      label_buffer.data(),
                      peers->size(),
                      total_connections,
                      U_STRING_TO_TRACE(*status_content), "/images/auth_logo.png");

      *status_content = buffer;
      }

   setHTTPResponse(*status_content);
}

void UNoCatPlugIn::getTraffic()
{
   U_TRACE(0, "UNoCatPlugIn::getTraffic()")

#ifdef HAVE_LINUX_NETFILTER_IPV4_IPT_ACCOUNT_H
   union uuaddr {
      uint32_t       a;
      struct in_addr addr;
   };

   union uuaddr u;
   const char* _ip;
   uint32_t traffic;
   const char* table;
   UModNoCatPeer* peer;
   struct ipt_acc_handle_ip* entry;

   for (uint32_t i = 0, n = vLocalNetwork->size(); i < n; ++i)
      {
      U_INTERNAL_ASSERT((*vLocalNetwork)[i].isNullTerminated())

      table = (*vLocalNetwork)[i].data();

      if (ipt->readEntries(table, false))
         {
         while ((entry = ipt->getNextEntry()))
            {
            u.a = entry->ip;
            _ip = inet_ntoa(u.addr);

            U_INTERNAL_DUMP("IP: %s SRC packets: %u bytes: %u DST packets: %u bytes: %u",
                             _ip, entry->src_packets, entry->src_bytes, entry->dst_packets, entry->dst_bytes)

            peer = (*peers)[_ip];

            U_INTERNAL_DUMP("peer = %p", peer)

            if (peer)
               {
               traffic = entry->src_bytes +
                         entry->dst_bytes;

               if (traffic)
                  {
                  peer->ctraffic     += traffic;
                  peer->traffic_done += traffic;

                  peer->traffic_remain = (peer->traffic_available > peer->traffic_done ? (peer->traffic_available - peer->traffic_done) : 0);

                  U_INTERNAL_DUMP("traffic = %u traffic_done = %llu ctraffic = %u traffic_remain = %llu",
                                   traffic, peer->traffic_done, peer->ctraffic, peer->traffic_remain)
                  }
               }
            }
         }
      }
#endif
}

void UNoCatPlugIn::setFireWallCommand(UCommand& cmd, const UString& script, const UString& mac, const char* _ip)
{
   U_TRACE(0, "UNoCatPlugIn::setFireWallCommand(%p,%.*S)", &cmd, U_STRING_TO_TRACE(script), U_STRING_TO_TRACE(mac), _ip)

   // NB: request(arp|deny|clear|reset|permit|openlist|initialize) mac ip class(Owner|Member|Public) UserDownloadRate UserUploadRate

   UString command(100U);

   command.snprintf("/bin/sh %.*s deny %.*s %s Member 0 0", U_STRING_TO_TRACE(script), U_STRING_TO_TRACE(mac), _ip);

   cmd.set(command, (char**)0);
   cmd.setEnvironment(fw_env);
}

bool UNoCatPlugIn::sendMsgToPortal(UCommand& cmd, uint32_t index_AUTH, const UString& msg, UString* poutput)
{
   U_TRACE(0, "UNoCatPlugIn::sendMsgToPortal(%p,%u,%.*S,%p)", &cmd, index_AUTH, U_STRING_TO_TRACE(msg), poutput)

   int pid, status;
   bool result = false;
   Url* auth = (*vinfo_url)[index_AUTH];
   UString auth_host    = auth->getHost(),
           auth_service = auth->getService(),
           url(200U + auth_host.size() + auth_service.size() + msg.size());

   url.snprintf("%.*s://%.*s%.*s", U_STRING_TO_TRACE(auth_service), U_STRING_TO_TRACE(auth_host), U_STRING_TO_TRACE(msg));

   cmd.setLastArgument(url.data());

   if (cmd.execute(0, poutput, -1, fd_stderr))
      {
retry:
      pid = UProcess::waitpid(-1, &status, WNOHANG); // NB: to avoid too much zombie...

      if (pid > 0 &&
          status)
         {
         U_SRV_LOG("Child (pid %d) exited with value %d (%s)", pid, status, UProcess::exitInfo(status));

         goto retry;
         }

      result = true;
      }

   UServer_Base::logCommandMsgError(cmd.getCommand(), false);

   U_RETURN(result);
}

int UNoCatPlugIn::handlerTime()
{
   U_TRACE(0, "UNoCatPlugIn::handlerTime()")

   U_INTERNAL_DUMP("flag_check_system = %b", flag_check_system)

   if (flag_check_system == false)
      {
      U_INTERNAL_DUMP("check_expire = %u", check_expire)

      U_INTERNAL_ASSERT_MAJOR(check_expire, 0)

      next_event_time = check_expire;

      U_INTERNAL_DUMP("U_http_method_len = %u", U_http_method_len)

      if (U_http_method_len == 0) checkSystem(UServer_Base::isLog());
      else                        UServer_Base::bpluginsHandlerReset = true; // NB: we put the check after the http request processing...

      U_INTERNAL_ASSERT_MAJOR(next_event_time, 0)

      UTimeVal::setSecond(next_event_time);
      }

   // return value:
   // ---------------
   // -1 - normal
   //  0 - monitoring
   // ---------------

   U_RETURN(0);
}

void UNoCatPlugIn::checkSystem(bool blog)
{
   U_TRACE(1, "UNoCatPlugIn::checkSystem(%b)", blog)

   flag_check_system = true;

   time_t check_interval = u_now->tv_sec - last_request_check;

   U_INTERNAL_DUMP("check_interval = %ld", check_interval)

   U_INTERNAL_ASSERT(u_now->tv_sec >= last_request_check)

   if (check_interval < U_TIME_FOR_ARPING_ASYNC_COMPLETION) // NB: protection from DoS...
      {
      next_event_time = check_expire - check_interval;

      goto end;
      }

   {
   time_t firewall_interval = u_now->tv_sec - last_request_firewall;

   U_INTERNAL_DUMP("firewall_interval = %ld", firewall_interval)

   U_INTERNAL_ASSERT(u_now->tv_sec >= last_request_firewall)

   if (firewall_interval < 3)
      {
      next_event_time = 3 - firewall_interval;

      goto end;
      }
   }

   if (blog) UServer_Base::mod_name->snprintf("[mod_nocat] ", 0);

   {
   UString output;

   last_request_check = u_now->tv_sec;

   // check firewall status

   fw->setArgument(3, "openlist");

   (void) fw->execute(0, &output, -1, fd_stderr);

   UString msg(300U);
   uint32_t i, n = (output.empty() ? 0 : openlist->split(output));

   U_INTERNAL_DUMP("openlist->split() = %u total_connections = %u peers->size() = %u", n, total_connections, peers->size())

   U_SRV_LOG("checkSystem(%b): total_connections %u firewall %u", blog, total_connections, n);

   if (n != total_connections)
      {
      peers->callForAllEntry(checkPeerStatus);

      UCommand _fw;
      const char* ptr;
      UString _ip, _mac;
      uint32_t n1 = openlist->size();

      U_INTERNAL_DUMP("openlist->size() = %u total_connections = %u peers->size() = %u", n1, total_connections, peers->size())

      U_INTERNAL_ASSERT(n1 <= n)

      for (i = 0; i < n1; ++i)
         {
         _ip = (*openlist)[i];

         U_ASSERT_EQUALS((*peers)[_ip], 0)

         if (u_isIPv4Addr(U_STRING_TO_PARAM(_ip)))
            {
            ptr = _ip.c_str();

            U_SRV_LOG("I try to remove: IP %S", ptr);

            _mac = UServer_Base::getMacAddress(ptr);

            if (_mac.empty()) _mac = *str_without_mac;

            // NB: request(arp|deny|clear|reset|permit|openlist|initialize) mac ip class(Owner|Member|Public) UserDownloadRate UserUploadRate

            setFireWallCommand(_fw, *fw_cmd, _mac, ptr);

            (void) _fw.execute(0, 0, -1, fd_stderr);

            UServer_Base::logCommandMsgError(_fw.getCommand(), false);
            }
         }

      // send msg to portal

      msg.snprintf("/error_ap?ap=%.*s@%.*s&public=%.*s:%u",
                     U_STRING_TO_TRACE(*label), U_STRING_TO_TRACE(*hostname), U_STRING_TO_TRACE(*ip), UServer_Base::port);

      (void) sendMsgToPortal(*uclient, msg);

      U_SRV_LOG("*** FIREWALL NOT ALIGNED: %.*S ***", U_STRING_TO_TRACE(output));
      }

   openlist->clear();

   // check if there are some log file to upload

   UFile file;
   const char* ptr;
   UVector<UString> vec(64);
   UString log_file, log_file_basename, log_file_name(U_CAPACITY), log_file_renamed(U_CAPACITY), dir(ULog::dir_log_gz);
   UDirWalk dirwalk(&dir, U_CONSTANT_TO_PARAM("*.gz"));

   for (i = 0, n = dirwalk.walk(vec); i < n; ++i)
      {
      log_file = vec[i];

      file.setPath(log_file);

      if (((void)file.stat(), file.getSize()) < (2 * 1024)) goto remove;

      log_file_basename = UStringExt::basename(log_file);

      if (UStringExt::startsWith(log_file_basename, *hostname)) ptr = log_file.data(); 
      else
         {
         log_file_name.snprintf("%.*s_%.*s", U_STRING_TO_TRACE(*hostname), U_STRING_TO_TRACE(log_file_basename));

         log_file_renamed.snprintf("%s/%.*s", ULog::dir_log_gz, U_STRING_TO_TRACE(log_file_name));

         (void) file._rename((ptr = log_file_renamed.data()));
         }

      uploader->setFileArgument(ptr);

      msg.snprintf("/uploader", 0);

      U_SRV_LOG("Uploading log file: %S", ptr);

      if (sendMsgToPortal(*uploader, 0, msg, &output)) // NB: we ask for output so that we wait for response before delete file...
         {
remove:
         (void) file._unlink();
         }
      }

   U_INTERNAL_ASSERT_EQUALS(paddrmask, 0)

   if (isPingAsyncPending())
      {
      U_SRV_LOG("Pending arping in process (%u), waiting for completion...", nfds);

      paddrmask = UPing::checkForPingAsyncCompletion(nfds);

      if (paddrmask) goto result;

      next_event_time = U_TIME_FOR_ARPING_ASYNC_COMPLETION + 2;

      goto end;
      }

   for (nfds = i = 0; i < num_radio; ++i) vaddr[i]->clear();

   FD_ZERO(&addrmask);

   U_SRV_LOG("Checking peers for info");

   getTraffic();

   peers->callForAllEntry(checkPeerInfo);

   U_INTERNAL_DUMP("nfds = %u addrmask = %B", nfds, __FDS_BITS(&addrmask)[0])

   if (nfds)
      {
           if ((check_type & U_CHECK_ARP_CACHE) != 0) paddrmask = UPing::arpcache(     vaddr, num_radio);
#  ifdef HAVE_NETPACKET_PACKET_H
      else if ((check_type & U_CHECK_ARP_PING)  != 0) paddrmask = UPing::arping(sockp, vaddr, num_radio, true, unatexit, *vInternalDevice);
#  endif
      else                                            paddrmask = &addrmask;
      }

   if (paddrmask)
      {
result:
      U_INTERNAL_ASSERT_MAJOR(nfds,0)

      UModNoCatPeer* peer;

      for (i = 0; i < nfds; ++i)
         {
         peer = getPeer(i);

         U_INTERNAL_DUMP("peer = %p", peer)

         if (peer)
            {
            if (FD_ISSET(i, paddrmask)) addPeerInfo(peer, 0);
            else
               {
#           ifdef HAVE_NETPACKET_PACKET_H
               if ((check_type & U_CHECK_ARP_PING) != 0)
                  {
                  U_SRV_LOG("Peer IP %s MAC %.*s don't return ARP reply, I assume he is disconnected...",
                                 peer->UIPAddress::pcStrAddress, U_STRING_TO_TRACE(peer->mac));
                  }
#           endif

               deny(peer, true);
               }
            }
         }

      nfds      = 0;
      paddrmask = 0;
      }

   UString info;
   Url* info_url;

   for (i = 0, n = vinfo_url->size(); i < n; ++i)
      {
      info_url = (*vinfo_url)[i];

      if (info_url->isQuery())
         {
         info = info_url->getPathAndQuery();

         U_INTERNAL_ASSERT(info)

         // NB: we ask for output so that we wait for response before delete info...

         if (sendMsgToPortal(*uclient, i, info, &output))
            {
            U_SRV_LOG("Info transmitted to AUTH(%u): %.*S", i, U_STRING_TO_TRACE(info));

#        ifdef DEBUG
            info.clear(); // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...
#        endif

            *info_url = *((*vauth_url)[i]);
            }
         }
      }
   }

end:
   if (blog) UServer_Base::mod_name->setEmpty();

   flag_check_system = false;
}

void UNoCatPlugIn::executeCommand(UModNoCatPeer* peer, int type)
{
   U_TRACE(0, "UNoCatPlugIn::executeCommand(%p,%d)", peer, type)

   peer->fw.setArgument(3, (type == UModNoCatPeer::PEER_PERMIT ? "permit" : "deny"));

   if (peer->fw.execute(0, 0, -1, fd_stderr))
      {
      int pid, status;
retry:
      pid = UProcess::waitpid(-1, &status, WNOHANG); // NB: to avoid too much zombie...

      if (pid > 0 &&
          status)
         {
         U_SRV_LOG("Child (pid %d) exited with value %d (%s)", pid, status, UProcess::exitInfo(status));

         goto retry;
         }

      U_peer_status(peer) = type;
      }

   UServer_Base::logCommandMsgError(peer->fw.getCommand(), false);

   last_request_firewall = u_now->tv_sec;
}

void UNoCatPlugIn::deny(UModNoCatPeer* peer, bool disconnected)
{
   U_TRACE(0, "UNoCatPlugIn::deny(%p,%b)", peer, disconnected)

   if (U_peer_status(peer) == UModNoCatPeer::PEER_DENY)
      {
      U_SRV_LOG("Peer already deny: IP %s MAC %.*s", peer->UIPAddress::pcStrAddress, U_STRING_TO_TRACE(peer->mac));

      return;
      }

   if (peer->logout == 0) peer->logout = u_now->tv_sec; // NB: request of logout or user disconnected...
#ifdef DEBUG
   else // NB: user with no more time or no more traffic...
      {
      U_INTERNAL_ASSERT_EQUALS(peer->logout, peer->expire)
      }
#endif

   addPeerInfo(peer, disconnected ? -1 : peer->logout); // -1 => disconnected (logout implicito)

   executeCommand(peer, UModNoCatPeer::PEER_DENY);

   --total_connections;

   U_SRV_LOG("Peer denied: IP %s MAC %.*s remain: %ld secs %llu bytes - total_connections %u",
               peer->UIPAddress::pcStrAddress, U_STRING_TO_TRACE(peer->mac), peer->time_remain, peer->traffic_remain, total_connections);

   if (login_timeout)
      {
      UTimer::erase(peer, false, true);

      peer->UEventTime::reset();
      }
}

void UNoCatPlugIn::permit(UModNoCatPeer* peer, uint32_t UserDownloadRate, uint32_t UserUploadRate)
{
   U_TRACE(0, "UNoCatPlugIn::permit(%p,%u,%u)", peer, UserDownloadRate, UserUploadRate)

   if (U_peer_status(peer) == UModNoCatPeer::PEER_PERMIT)
      {
      U_SRV_LOG("Peer already permit: IP %s MAC %.*s", peer->UIPAddress::pcStrAddress, U_STRING_TO_TRACE(peer->mac));
      }
   else
      {
      if (UserDownloadRate)
         {
         static char buffer[20];

         (void) snprintf(buffer, sizeof(buffer), "%u", UserDownloadRate);

         peer->fw.setArgument(7, buffer);
         }

      if (UserUploadRate)
         {
         static char buffer[20];

         (void) snprintf(buffer, sizeof(buffer), "%u", UserUploadRate);

         peer->fw.setArgument(8, buffer);
         }

      executeCommand(peer, UModNoCatPeer::PEER_PERMIT);

      // set connection time

      peer->logout = 0L;
      peer->expire = (peer->ctime = peer->connected = u_now->tv_sec) + peer->time_remain;

      if (login_timeout)
         {
         peer->UTimeVal::setSecond(peer->time_remain);

         UTimer::insert(peer, true);
         }

      ++total_connections;

      U_SRV_LOG("Peer permitted: IP %s MAC %.*s UserDownloadRate %u UserUploadRate %u - total_connections %u",
                  peer->UIPAddress::pcStrAddress, U_STRING_TO_TRACE(peer->mac), UserDownloadRate, UserUploadRate, total_connections);
      }
}

void UNoCatPlugIn::setRedirectLocation(UModNoCatPeer* peer, const UString& redirect, const Url& auth)
{
   U_TRACE(0, "UNoCatPlugIn::setRedirectLocation(%p,%.*S,%.*S)", peer, U_STRING_TO_TRACE(redirect), U_URL_TO_TRACE(auth))

   UString value_encoded((redirect.size() + 1024U) * 3U);

   Url::encode(peer->mac, value_encoded);

   location->snprintf("%.*s%s?mac=%.*s&ip=", U_URL_TO_TRACE(auth), (auth.isPath() ? "" : "/login"), U_STRING_TO_TRACE(value_encoded));

   Url::encode(U_STRING_TO_PARAM(peer->ip), value_encoded);

   location->snprintf_add("%.*s&redirect=", U_STRING_TO_TRACE(value_encoded));

   uint32_t pos = redirect.find('&');

   if (pos != U_NOT_FOUND) location->snprintf_add("%.*s&gateway=", U_STRING_TO_TRACE(redirect));
   else
      {
      Url::encode(redirect, value_encoded);

      location->snprintf_add("%.*s&gateway=", U_STRING_TO_TRACE(value_encoded));
      }

   Url::encode(peer->gateway, value_encoded);

   location->snprintf_add("%.*s&timeout=%ld&token=", U_STRING_TO_TRACE(value_encoded), time_available);

   if (peer->token.empty())
      {
      /* ---------------------------------------------------------------------------------------------------------------------
      // set crypto token (valid for 30 minutes)...
      // ---------------------------------------------------------------------------------------------------------------------
      // NB: tutto il traffico viene rediretto sulla 80 (CAPTIVE PORTAL) e quindi windows update, antivrus, etc...
      // questo introduce la possibilita' che durante la fase di autorizzazione il token generato per il ticket autorizzativo
      // non corrisponda piu' a quello inviato dal portale per l'autorizzazione...
      // ---------------------------------------------------------------------------------------------------------------------

      peer->token = UServices::generateToken(peer->mac, u_now->tv_sec + (30L * 60L));
      */

      peer->token.snprintf("%u", u_random(u_now->tv_usec));
      }

   location->snprintf_add("%.*s&ap=%.*s@%.*s", U_STRING_TO_TRACE(peer->token), U_STRING_TO_TRACE(peer->label), U_STRING_TO_TRACE(*ip));
}

UString UNoCatPlugIn::getSignedData(const char* ptr, uint32_t len)
{
   U_TRACE(0, "UNoCatPlugIn::getSignedData(%.*S,%u)", len, ptr, len)

   UString output;

   // if (decrypt_key->empty() == false)
      {
      output = UDES3::getSignedData(ptr, len);
      }
   /*
   else
      {
      UString buffer(U_CAPACITY), input(U_CAPACITY);

      Url::decode(ptr, len, buffer);

      input.snprintf("-----BEGIN PGP MESSAGE-----\n\n"
                     "%.*s"
                     "\n-----END PGP MESSAGE-----", U_STRING_TO_TRACE(buffer));

      (void) pgp.execute(&input, &output, -1, fd_stderr);

      UServer_Base::logCommandMsgError(pgp.getCommand(), false);
      }
   */

   U_RETURN_STRING(output);
}

bool UNoCatPlugIn::checkAuthMessage(UModNoCatPeer* peer, const UString& msg)
{
   U_TRACE(0, "UNoCatPlugIn::checkAuthMessage(%p,%.*S)", peer, U_STRING_TO_TRACE(msg))

   U_INTERNAL_ASSERT(msg)
   U_INTERNAL_ASSERT_POINTER(peer)

   uint32_t pos;
   time_t timeout;
   Url destination;
   uint64_t traffic;
   bool result = true;
   UHashMap<UString> args;
   UVector<UString> name_value;
   UString action, policy, notraffic;

   istrstream is(U_STRING_TO_PARAM(msg));

         args.allocate();
   is >> args;

   /*
   Action    Permit
   Mode      Login
   Redirect  http://wi-auth/postlogin?uid=s.casazza&gateway=10.30.1.131:5280&redirect=http%3A//stefano%3A5280/pippo
   Mac       00:e0:4c:d4:63:f5
   Timeout   7200
   Traffic   314572800
   Token     10.30.1.105&1237907630&05608a4cbd42c9f72d2bd3a0e19ed23f
   User      s.casazza
   Policy    FLAT
   NoTraffic 6

   UserDownloadRate 10240
   UserUploadRate    1024
   */

   // check mac address

   if (peer->mac != args[*str_Mac])
      {
      U_SRV_LOG("Different MAC in ticket from peer: IP %s MAC %.*s", peer->UIPAddress::pcStrAddress, U_STRING_TO_TRACE(peer->mac));

      goto error;
      }

   // check token

   if (peer->token != args[*str_Token])
      {
      U_SRV_LOG("Tampered token from peer: IP %s MAC %.*s", peer->UIPAddress::pcStrAddress, U_STRING_TO_TRACE(peer->mac));

      goto error;
      }

   /* get mode

   mode = args[*str_Mode].copy();

   U_INTERNAL_DUMP("mode = %.*S", U_STRING_TO_TRACE(mode))
   */

   // check user id

   peer->user = args[*str_User].copy();

   pos = vLoginValidate->contains(peer->user);

   if (pos != U_NOT_FOUND)
      {
      U_SRV_LOG("Validation of user id %.*S in ticket from peer: IP %s MAC %.*s", U_STRING_TO_TRACE(peer->user), peer->UIPAddress::pcStrAddress,
                                                                                  U_STRING_TO_TRACE(peer->mac));

      vLoginValidate->erase(pos);
      }

   // get redirect (destination)

   destination.set(args[*str_Redirect]);

   if (destination.getQuery(name_value) == 0)
      {
      U_SRV_LOG("Can't make sense of Redirect: %.*S in ticket from peer: IP %s MAC %.*s", U_URL_TO_TRACE(destination), peer->UIPAddress::pcStrAddress,
                                                                                          U_STRING_TO_TRACE(peer->mac));

      goto error;
      }

   destination.eraseQuery();

   if (destination.setQuery(name_value) == false)
      {
      U_SRV_LOG("Error on setting Redirect: %.*S in ticket from peer: IP %s MAC %.*s", U_URL_TO_TRACE(destination), peer->UIPAddress::pcStrAddress,
                                                                                       U_STRING_TO_TRACE(peer->mac));

      goto error;
      }

   (void) location->replace(destination.get());

   // check for user policy FLAT

   policy = args[*str_Policy];

   if (policy == *str_Policy_FLAT) U_peer_policy_flat(peer) = true;

   // check for max time no traffic

   notraffic = args[*str_NoTraffic];

   if (notraffic.empty() == false) U_peer_max_time_no_traffic(peer) = notraffic.strtol();

   if (U_peer_max_time_no_traffic(peer) == 0) U_peer_max_time_no_traffic(peer) = (check_expire / 60) * (U_peer_policy_flat(peer) ? 3 : 1);

   // get time available

   timeout = args[*str_Timeout].strtol();

   if (timeout ||
       peer->time_remain == 0)
      {
      if (timeout) peer->time_remain = timeout;
      else         peer->time_remain = (U_peer_policy_flat(peer) ? 24L : 2L) * 3600L; // default value...
      }

   U_INTERNAL_DUMP("time_remain = %ld", peer->time_remain)

   // get traffic available

   traffic = args[*str_Traffic].strtoll();

   if (traffic ||
       peer->traffic_available == 0)
      {
      if (traffic) peer->traffic_available = traffic;
      else         peer->traffic_available = (U_peer_policy_flat(peer) ? 4 * 1024ULL : 300ULL) * (1024ULL * 1024ULL); // default value...
      }

   U_INTERNAL_DUMP("traffic_available = %llu", peer->traffic_available)

   // get action

   action = args[*str_Action];

   if (action.empty() ||
       action == *str_Permit)
      {
      permit(peer, args[*str_UserDownloadRate].strtol(), args[*str_UserUploadRate].strtol());
      }
   else if (action == *str_Deny)
      {
      (void) peer->checkPeerInfo(true);

      deny(peer, false);
      }
   else
      {
      U_SRV_LOG("Can't make sense of Action: %.*S in ticket from peer: IP %s MAC %.*s", U_STRING_TO_TRACE(action), peer->UIPAddress::pcStrAddress,
                                                                                        U_STRING_TO_TRACE(peer->mac));

      goto error;
      }

   goto end;

error:
   result = false;

end:
   args.clear();
   args.deallocate();

   // ---------------------------------------------------------------------------------------------------------------------
   // NB: tutto il traffico viene rediretto sulla 80 (CAPTIVE PORTAL) e quindi windows update, antivrus, etc...
   // questo introduce la possibilita' che durante la fase di autorizzazione il token generato per il ticket autorizzativo
   // non corrisponda piu' a quello inviato dal portale per l'autorizzazione...
   // ---------------------------------------------------------------------------------------------------------------------
   peer->token.setEmpty();

   U_RETURN(result);
}

void UNoCatPlugIn::setNewPeer(UModNoCatPeer* peer, uint32_t index_AUTH)
{
   U_TRACE(0, "UNoCatPlugIn::setNewPeer(%p,%u)", peer, index_AUTH)

   (void) peer->user.assign(U_peer_allowed(peer) ? *UNoCatPlugIn::str_allowed
                                                 : *UNoCatPlugIn::str_anonymous);

   peer->ifname = USocketExt::getNetworkInterfaceName(peer->UIPAddress::pcStrAddress);

   U_INTERNAL_DUMP("peer->ifname = %.*S", U_STRING_TO_TRACE(peer->ifname))

   U_peer_index_device(peer) = (peer->ifname.empty() ? 0 : vInternalDevice->find(peer->ifname));

   U_INTERNAL_DUMP("U_peer_index_device(peer) = %u", U_peer_index_device(peer))

   /*
   -----------------------------------------------------------------------------------
    OLD METHOD
   -----------------------------------------------------------------------------------
   if (U_peer_index_device(peer) <= num_radio)
      {
      peer->label = (vLocalNetworkLabel.empty() ?    (*vInternalDevice)[U_peer_index_device(peer)]
                                                : (*vLocalNetworkLabel)[U_peer_index_device(peer)]);
      }

   if (peer->label.empty()) peer->label = peer->ifname;
   -----------------------------------------------------------------------------------
   */

   U_peer_index_network(peer) = UIPAllow::contains(peer->UIPAddress::pcStrAddress, *vLocalNetworkMask);

   U_INTERNAL_DUMP("U_peer_index_network(peer) = %u", U_peer_index_network(peer))

   UString x;

   if (peer->ifname.empty() ||
       (uint32_t)U_peer_index_network(peer) >= vLocalNetwork->size())
      {
      x = *ip;
      }
   else
      {
      x = (*vLocalNetwork)[U_peer_index_network(peer)];

      x = USocketExt::getGatewayAddress(x);
      }

   peer->gateway.snprintf("%.*s:%d", U_STRING_TO_TRACE(x), UServer_Base::port);

   peer->label = ((uint32_t)U_peer_index_network(peer) < vLocalNetworkLabel->size()
                                                     ? (*vLocalNetworkLabel)[U_peer_index_network(peer)]
                                                     : *str_without_label);

   U_INTERNAL_DUMP("peer->gateway = %.*S peer->label = %.*S", U_STRING_TO_TRACE(peer->gateway), U_STRING_TO_TRACE(peer->label))

   U_peer_index_AUTH(peer) = index_AUTH;

   U_INTERNAL_DUMP("U_peer_index_AUTH(peer) = %u", U_peer_index_AUTH(peer))

   // NB: request(arp|deny|clear|reset|permit|openlist|initialize) mac ip class(Owner|Member|Public) UserDownloadRate UserUploadRate

   setFireWallCommand(peer->fw, *fw_cmd, peer->mac, peer->UIPAddress::pcStrAddress);

   peers->insertAfterFind(peer->ip, peer);
}

UModNoCatPeer* UNoCatPlugIn::creatNewPeer(uint32_t index_AUTH)
{
   U_TRACE(0, "UNoCatPlugIn::creatNewPeer(%u)", index_AUTH)

   UModNoCatPeer* peer = U_NEW(UModNoCatPeer);

   U_peer_allowed(peer) = false;
   *((UIPAddress*)peer) = UServer_Base::pClientImage->socket->remoteIPAddress();

   if (U_ipaddress_StrAddressUnresolved(peer)) peer->UIPAddress::resolveStrAddress();

   (void) peer->ip.assign(peer->UIPAddress::pcStrAddress);

   U_ASSERT(peer->ip == UServer_Base::getClientAddress())

   peer->mac = UServer_Base::getMacAddress(peer->UIPAddress::pcStrAddress);

   if (peer->mac.empty() && ip->equal(UServer_Base::getClientAddress())) peer->mac = UServer_Base::getMacAddress(extdev->data());
   if (peer->mac.empty())                                                peer->mac = *str_without_mac;

   setNewPeer(peer, index_AUTH);

   U_RETURN_POINTER(peer, UModNoCatPeer);
}

void UNoCatPlugIn::checkOldPeer(UModNoCatPeer* peer)
{
   U_TRACE(0, "UNoCatPlugIn::checkOldPeer(%p)", peer)

   UString mac = UServer_Base::getMacAddress(UServer_Base::getClientAddress()); // NB: get mac from arp cache...

   if (mac != peer->mac      &&
       (mac.empty() == false ||
        peer->mac   != *str_without_mac))
      {
      U_SRV_LOG("Different MAC (%.*s) from arp cache for peer: IP %s MAC %.*s", U_STRING_TO_TRACE(mac),
                                                                                peer->UIPAddress::pcStrAddress,
                                                                                U_STRING_TO_TRACE(peer->mac));

      if (peer->checkPeerInfo(true) == false) deny(peer, true);

      peer->mac = mac;

      peer->fw.setArgument(4, peer->mac.data());

      // ---------------------------------------------------------------------------------------------------------------------
      // NB: tutto il traffico viene rediretto sulla 80 (CAPTIVE PORTAL) e quindi windows update, antivrus, etc...
      // questo introduce la possibilita' che durante la fase di autorizzazione il token generato per il ticket autorizzativo
      // non corrisponda piu' a quello inviato dal portale per l'autorizzazione...
      // ---------------------------------------------------------------------------------------------------------------------
      peer->token.setEmpty();
      }
}

void UNoCatPlugIn::addPeerInfo(UModNoCatPeer* peer, time_t logout)
{
   U_TRACE(0, "UNoCatPlugIn::addPeerInfo(%p,%ld)", peer, logout)

   // $1 -> mac
   // $2 -> ip
   // $3 -> gateway
   // $4 -> ap
   // $5 -> uid
   // $6 -> logout
   // $7 -> connected
   // $8 -> traffic

   Url* info_url = (*vinfo_url)[U_peer_index_AUTH(peer)];

   info_url->setPath(U_CONSTANT_TO_PARAM("/info"));

   U_INTERNAL_DUMP("U_peer_index_AUTH(peer) = %u info_url = %p", U_peer_index_AUTH(peer), info_url)

   U_INTERNAL_ASSERT(peer->user)

   UString buffer(U_CAPACITY);

   buffer.snprintf("%.*s@%.*s", U_STRING_TO_TRACE(peer->label), U_STRING_TO_TRACE(*ip));

   info_url->addQuery(U_STRING_TO_PARAM(*str_Mac),    U_STRING_TO_PARAM(peer->mac));
   info_url->addQuery(U_CONSTANT_TO_PARAM("ip"),      U_STRING_TO_PARAM(peer->ip));
   info_url->addQuery(U_CONSTANT_TO_PARAM("gateway"), U_STRING_TO_PARAM(peer->gateway));
   info_url->addQuery(U_CONSTANT_TO_PARAM("ap"),      U_STRING_TO_PARAM(buffer));
   info_url->addQuery(U_STRING_TO_PARAM(*str_User),   U_STRING_TO_PARAM(peer->user));

   buffer = UStringExt::numberToString(logout); // NB: (-1|0) mean NOT logout (only info)...

   info_url->addQuery(U_CONSTANT_TO_PARAM("logout"), U_STRING_TO_PARAM(buffer));

   U_INTERNAL_ASSERT(u_now->tv_sec >= peer->ctime)

   buffer = UStringExt::numberToString(u_now->tv_sec - peer->ctime);
                                                       peer->ctime = u_now->tv_sec;

   info_url->addQuery(U_CONSTANT_TO_PARAM("connected"), U_STRING_TO_PARAM(buffer));

   if (peer->ctraffic == 0) info_url->addQuery(U_CONSTANT_TO_PARAM("traffic"), U_CONSTANT_TO_PARAM("0"));
   else
      {
      buffer.snprintf("%u", peer->ctraffic);
                            peer->ctraffic = peer->time_no_traffic = 0;

      info_url->addQuery(U_CONSTANT_TO_PARAM("traffic"), U_STRING_TO_PARAM(buffer));
      }
}

void UNoCatPlugIn::checkPeerStatus(UStringRep* key, void* value)
{
   U_TRACE(0, "UNoCatPlugIn::checkPeerStatus(%.*S,%p)", U_STRING_TO_TRACE(*key), value)

   U_INTERNAL_ASSERT_POINTER(openlist)

   UModNoCatPeer* peer = (UModNoCatPeer*) value;
   uint32_t pos        = openlist->contains(peer->UIPAddress::pcStrAddress);

   if (U_peer_status(peer) == UModNoCatPeer::PEER_PERMIT)
      {
      if (pos == U_NOT_FOUND)
         {
         --total_connections;

         U_SRV_LOG("I try to deny user with status permit: IP %s MAC %.*s remain: %ld secs %llu bytes - total_connections %u",
                     peer->UIPAddress::pcStrAddress, U_STRING_TO_TRACE(peer->mac), peer->time_remain, peer->traffic_remain, total_connections);

         executeCommand(peer, UModNoCatPeer::PEER_DENY);

         U_INTERNAL_ASSERT_EQUALS(U_peer_status(peer), UModNoCatPeer::PEER_DENY)
         }
      else
         {
         openlist->erase(pos);

         U_SRV_LOG("User permit: IP %s MAC %.*s", peer->UIPAddress::pcStrAddress, U_STRING_TO_TRACE(peer->mac));
         }
      }
   else
      {
      U_INTERNAL_ASSERT_EQUALS(U_peer_status(peer), UModNoCatPeer::PEER_DENY)

      if (pos == U_NOT_FOUND)
         {
         U_SRV_LOG("User deny: IP %s MAC %.*s", peer->UIPAddress::pcStrAddress, U_STRING_TO_TRACE(peer->mac));
         }
      else
         {
         openlist->erase(pos);

         U_SRV_LOG("I try to deny user with status deny: IP %s MAC %.*s remain: %ld secs %llu bytes - total_connections %u",
                     peer->UIPAddress::pcStrAddress, U_STRING_TO_TRACE(peer->mac), peer->time_remain, peer->traffic_remain, total_connections);

         executeCommand(peer, UModNoCatPeer::PEER_DENY);
         }
      }
}

bool UModNoCatPeer::checkPeerInfo(bool btraffic)
{
   U_TRACE(0, "UModNoCatPeer::checkPeerInfo(%b)", btraffic)

   if (U_peer_allowed(this) == false &&
       U_peer_status( this) == PEER_PERMIT)
      {
      bool result = true;

      if (btraffic) UNoCatPlugIn::getTraffic();

      *UNoCatPlugIn::ifname = USocketExt::getNetworkInterfaceName(UIPAddress::pcStrAddress);

      U_INTERNAL_DUMP("UNoCatPlugIn::ifname = %.*S peer->ifname = %.*S", U_STRING_TO_TRACE(*UNoCatPlugIn::ifname), U_STRING_TO_TRACE(ifname))

      if (ctraffic == 0) time_no_traffic += (u_now->tv_sec - ctime);

      U_INTERNAL_DUMP("time_no_traffic = %ld", time_no_traffic)

      if (time_no_traffic)
         {
         time_t max_time_no_traffic = U_peer_max_time_no_traffic(this) * 60;

         U_SRV_LOG("Peer IP %s MAC %.*s has made no traffic for %ld secs - (max_time_no_traffic: %u secs) (present in ARP cache: %s)",
                     UIPAddress::pcStrAddress, U_STRING_TO_TRACE(mac), time_no_traffic, max_time_no_traffic, UNoCatPlugIn::ifname->empty() ? "no" : "yes");

         if (UNoCatPlugIn::ifname->empty()           ||
             (time_no_traffic >= max_time_no_traffic &&
              (UNoCatPlugIn::check_type & UNoCatPlugIn::U_CHECK_TRAFFIC) != 0))
            {
            U_INTERNAL_ASSERT(u_now->tv_sec >= ctime)

            result = false;
            }
         }

      time_remain = (expire > u_now->tv_sec ? (expire - u_now->tv_sec) : 0);

      if (time_remain <= 15)
         {
         result      = false;
         logout      = expire; // NB: user with no more time or no more traffic...
         time_remain = 0;

         U_SRV_LOG("Peer IP %s MAC %.*s has no more time - remain traffic for %llu bytes", UIPAddress::pcStrAddress, U_STRING_TO_TRACE(mac), traffic_remain);
         }

      if (traffic_remain <= 1024ULL)
         {
         result         = false;
         logout         = expire; // NB: user with no more time or no more traffic...
         traffic_remain = 0;

         U_SRV_LOG("Peer IP %s MAC %.*s has no more traffic - remain time for %lu sec", UIPAddress::pcStrAddress, U_STRING_TO_TRACE(mac), time_remain);
         }

      if (UNoCatPlugIn::ifname->empty() &&
          (UNoCatPlugIn::check_type & UNoCatPlugIn::U_CHECK_ARP_CACHE) != 0)
         {
         result = false;

         U_SRV_LOG("Peer IP %s MAC %.*s not present in ARP cache, I assume he is disconnected", UIPAddress::pcStrAddress, U_STRING_TO_TRACE(mac));
         }

      U_RETURN(result);
      }

   U_RETURN(true);
}

void UNoCatPlugIn::checkPeerInfo(UStringRep* key, void* value)
{
   U_TRACE(0, "UNoCatPlugIn::checkPeerInfo(%.*S,%p)", U_STRING_TO_TRACE(*key), value)

   UModNoCatPeer* peer = (UModNoCatPeer*) value;

   if (peer->checkPeerInfo(false) == false)
      {
      deny(peer, ifname->empty());

      return;
      }

   if (U_peer_status(peer) == UModNoCatPeer::PEER_PERMIT)
      {
      if (U_peer_allowed(peer))
         {
      // addPeerInfo(peer, 0);

         return;
         }

      U_SRV_LOG("Peer IP %s MAC %.*s remain: %ld secs %llu bytes",
                  peer->UIPAddress::pcStrAddress, U_STRING_TO_TRACE(peer->mac), peer->time_remain, peer->traffic_remain);

      uint32_t index_device = U_peer_index_device(peer);

      if ((check_type & U_CHECK_ARP_CACHE) != 0)
         {
         U_INTERNAL_ASSERT(*ifname)

         index_device = vInternalDevice->find(*ifname);
         }

      U_INTERNAL_DUMP("index_device = %u U_peer_index_device(peer) = %u", index_device, U_peer_index_device(peer))

      U_INTERNAL_ASSERT(index_device <= num_radio)

      if (index_device != U_NOT_FOUND)
         {
         FD_SET(nfds, &addrmask);

         ++nfds;

         vaddr[index_device]->push(peer);
         }
      }
}

__pure UModNoCatPeer* UNoCatPlugIn::getPeer(uint32_t n)
{
   U_TRACE(0, "UNoCatPlugIn::getPeer(%u)", n)

   U_INTERNAL_ASSERT_MAJOR(nfds,0)

   int32_t index = -1;
   UModNoCatPeer* peer;

   for (uint32_t k, i = 0; i < num_radio; ++i)
      {
      k = vaddr[i]->size();

      for (uint32_t j = 0; j < k; ++j)
         {
         if (++index == (int32_t)n)
            {
            peer = (UModNoCatPeer*) vaddr[i]->at(j);

            U_RETURN_POINTER(peer, UModNoCatPeer);
            }
         }
      }

   U_RETURN_POINTER(0, UModNoCatPeer);
}

void UNoCatPlugIn::notifyAuthOfUsersInfo(uint32_t index_AUTH)
{
   U_TRACE(0, "UNoCatPlugIn::notifyAuthOfUsersInfo(%u)", index_AUTH)

   Url* info_url = (*vinfo_url)[index_AUTH];

   U_INTERNAL_DUMP("index_AUTH = %u info_url = %p", index_AUTH, info_url)

   // NB: if there are some data to transmit we need redirect...

   if (info_url->isQuery())
      {
      UHTTP::setRedirectResponse(0, UString::getStringNull(), U_URL_TO_PARAM(*info_url));

      // NB: we assume that the redirect always have success...

      *info_url = *((*vauth_url)[index_AUTH]);
      }
   else
      {
      // NB: if there is arping pending AUTH must recall us after ~15sec for completion...

      U_http_is_connection_close = U_YES;
      u_http_info.nResponseCode  = (isPingAsyncPending() ? HTTP_NO_CONTENT : HTTP_NOT_MODIFIED);

      UHTTP::setResponse(0, 0);
      }
}

UModNoCatPeer* UNoCatPlugIn::getPeerFromMAC(const UString& mac)
{
   U_TRACE(0, "UNoCatPlugIn::getPeerFromMAC(%.*S)", U_STRING_TO_TRACE(mac))

   UModNoCatPeer* peer;

   if (peers->first())
      {
      do {
         peer = peers->elem();

         if (mac == peer->mac) U_RETURN_POINTER(peer, UModNoCatPeer);
         }
      while (peers->next());
      }

   U_RETURN_POINTER(0, UModNoCatPeer);
}

UString UNoCatPlugIn::getIPAddress(const char* ptr, uint32_t len)
{
   U_TRACE(0, "UNoCatPlugIn::getIPAddress(%.*S,%u)", len, ptr, len)

   char c;
   uint32_t i;
   UString x(U_INET_ADDRSTRLEN);

   for (i = 0; i < len; ++i)
      {
      c = ptr[i];

      if (u__isdigit(c) == 0 && c != '.' && c != ':') break;

      x._append(c);
      }

   x._append();

   U_RETURN_STRING(x);
}

void UNoCatPlugIn::setHTTPResponse(const UString& content)
{
   U_TRACE(0, "UNoCatPlugIn::setHTTPResponse(%.*S)", U_STRING_TO_TRACE(content))

   u_http_info.clength         = content.size();
   u_http_info.nResponseCode   = HTTP_OK;
   *UClientImage_Base::wbuffer = content;

   UHTTP::setCgiResponse(false, UHTTP::isCompressable(), false);
}

__pure uint32_t UNoCatPlugIn::getIndexAUTH(const char* ip_address)
{
   U_TRACE(0, "UNoCatPlugIn::getIndexAUTH(%S)", ip_address)

   uint32_t index_AUTH, sz_AUTH = vauth_ip->size();

   if (sz_AUTH == 1) index_AUTH = 0;
   else
      {
      // NB: we are multi portal, we must find which portal we indicate to redirect the client...

      index_AUTH = UIPAllow::contains(ip_address, *vLocalNetworkMask);

      if (index_AUTH >= sz_AUTH) index_AUTH = 0;
      }

   U_RETURN(index_AUTH);
}

// Server-wide hooks

int UNoCatPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UNoCatPlugIn::handlerConfig(%p)", &cfg)

   // -----------------------------------------------------------------------------------------------------------------------------------
   // mod_nocat - plugin parameters
   // -----------------------------------------------------------------------------------------------------------------------------------
   // FW_ENV                environment for shell script to execute
   // FW_CMD                shell script to manage the firewall
   // DECRYPT_KEY           DES3 password stuff
   // ALLOWED_MEMBERS       file with list of allowed MAC/IP pairs or NETWORKS (default: /etc/nodog.allowed)
   // LOCAL_NETWORK_LABEL   access point localization tag to be used from portal
   // LOGIN_TIMEOUT         Number of seconds after a client last login/renewal to terminate their connection
   // CHECK_TYPE            mode of verification (U_CHECK_NONE=0, U_CHECK_ARP_CACHE=1, U_CHECK_ARP_PING=2, U_CHECK_TRAFFIC=4)
   // CHECK_EXPIRE_INTERVAL Number of seconds to check if some client has terminate their connection then send info to portal
   // -----------------------------------------------------------------------------------------------------------------------------------

   if (cfg.loadTable())
      {
      *fw_env = cfg[*str_FW_ENV];

      if (fw_env->empty())
         {
         U_SRV_LOG("Configuration of plugin FAILED");

         U_RETURN(U_PLUGIN_HANDLER_ERROR);
         }

      int port = 0;
      UString lnetlbl, tmp;

      tmp           = cfg[*str_LOGIN_TIMEOUT];
      *fw_cmd       = cfg[*str_FW_CMD];
      lnetlbl       = cfg[*str_LOCAL_NETWORK_LABEL];
   // *decrypt_cmd  = cfg[*str_DECRYPT_CMD];
      *decrypt_key  = cfg[*str_DECRYPT_KEY];

      check_type    = cfg.readLong(*str_CHECK_TYPE);
      check_expire  = cfg.readLong(*str_CHECK_EXPIRE_INTERVAL);

      if (tmp.empty() == false)
         {
         char* ptr;

         login_timeout = U_NEW(UString(tmp));

         time_available = strtol(login_timeout->data(), &ptr, 0);

         if (time_available > U_ONE_DAY_IN_SECOND) time_available = U_ONE_DAY_IN_SECOND; // check for safe timeout...

         U_INTERNAL_DUMP("ptr[0] = %C", ptr[0])

         if (ptr[0] == ':') traffic_available = strtoll(ptr+1, 0, 0);

         if (traffic_available == 0) traffic_available = 4ULL * 1024ULL * 1024ULL * 1024ULL; // 4G

         U_INTERNAL_DUMP("time_available = %ld traffic_available = %llu", time_available, traffic_available)
         }

      if (check_expire) UTimeVal::setSecond(check_expire);

      U_INTERNAL_DUMP("check_expire = %ld time_available = %ld check_type = %B", check_expire, time_available, check_type)

      *fw_env = UStringExt::prepareForEnvironmentVar(*fw_env);

      tmp = UStringExt::getEnvironmentVar(*str_INTERNAL_DEVICE, fw_env);

      if (tmp.empty() == false) *intdev = tmp;

      tmp = UStringExt::getEnvironmentVar(*str_EXTERNAL_DEVICE, fw_env);

      if (tmp.empty() == false) *extdev = tmp;

      tmp = UStringExt::getEnvironmentVar(*str_LOCAL_NETWORK, fw_env);

      if (tmp.empty() == false) *localnet = tmp;

      tmp = UStringExt::getEnvironmentVar(*str_AUTH_SERVICE_URL, fw_env);

      if (tmp.empty() == false) *auth_login = tmp;

      tmp = UStringExt::getEnvironmentVar(*str_GATEWAY_PORT, fw_env);

      if (tmp.empty() == false) port = tmp.strtol();

      /* NON piace a Ghivizzani

      UString pathconf = UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("FW_CONF"), fw_env);

      if (pathconf.empty() == false)
         {
         // UploadRate=10240
         // DownloadRate=10240
         // UserUploadRate=1024
         // UserDownloadRate=10240
         // EnableBandwidthLimits=1
         // 
         // DosPrevention=0
         // DosTimeInterval=60
         // DosAllowTries=5

         UString data      = UFile::contentOf(pathconf);
         const char* end   = data.end();
         const char* start = data.data();

         cfg.clear();

         if (UFileConfig::loadProperties(cfg.table, start, end) == false) goto err;
         }
      */

      (void) vauth->split(U_STRING_TO_PARAM(*auth_login));

      if (extdev->empty())
         {
         *extdev = UServer_Base::getNetworkDevice(0);

         if (extdev->empty()) U_ERROR("No ExternalDevice detected!");

         U_SRV_LOG("Autodetected ExternalDevice %S", extdev->data());
         }

      if (intdev->empty())
         {
         *intdev = UServer_Base::getNetworkDevice(extdev->data());

         if (intdev->empty()) U_ERROR("No InternalDevice detected!");

         U_SRV_LOG("Autodetected InternalDevice %S", intdev->data());
         }

      num_radio = vInternalDevice->split(U_STRING_TO_PARAM(*intdev));

      U_INTERNAL_DUMP("num_radio = %u", num_radio)

      if (localnet->empty())
         {
         *localnet = UServer_Base::getNetworkAddress(intdev->data());

         if (localnet->empty()) U_ERROR("No LocalNetwork detected!");

         U_SRV_LOG("Autodetected LocalNetwork %S", localnet->data());
         }

      (void) vLocalNetwork->split(U_STRING_TO_PARAM(*localnet));
      (void) vLocalNetworkLabel->split(U_STRING_TO_PARAM(lnetlbl));

      (void) UIPAllow::parseMask(*localnet, *vLocalNetworkMask);

      U_INTERNAL_DUMP("port = %d", port)

      UServer_Base::port = (port ? port : 5280);

      *allowed_members = cfg[*str_ALLOWED_MEMBERS];
      *allowed_members = UFile::contentOf(allowed_members->empty() ? *str_allowed_members_default : *allowed_members);
      }

   *label = (vLocalNetworkLabel->empty() ? *str_without_label : (*vLocalNetworkLabel)[0]);

   U_INTERNAL_DUMP("label = %.*S", U_STRING_TO_TRACE(*label))

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UNoCatPlugIn::handlerInit()
{
   U_TRACE(0, "UNoCatPlugIn::handlerInit()")

   if (fw_cmd->empty())
      {
      U_SRV_LOG("Initialization of plugin FAILED");

      U_RETURN(U_PLUGIN_HANDLER_GO_ON);
      }

// U_PRINT_SIZEOF(UModNoCatPeer);

   Url* url;
   UIPAddress addr;
   UString auth_ip, _ip;

   // NB: get IP address of AUTH hosts...

   for (uint32_t i = 0, n = vauth->size(); i < n; ++i)
      {
      _ip = (*vauth)[i];
      url = U_NEW(Url(_ip));

      vauth_url->push(url);

      auth_ip = url->getHost();

      if (addr.setHostName(auth_ip, UClientImage_Base::bIPv6) == false)
         {
         U_SRV_LOG("Unknown AUTH host %.*S", U_STRING_TO_TRACE(auth_ip));
         }
      else
         {
         (void) auth_ip.replace(addr.getAddressString());

         U_SRV_LOG("AUTH host registered: %.*s", U_STRING_TO_TRACE(auth_ip));

         vauth_ip->push(auth_ip);
         }

      url = U_NEW(Url(_ip));

      vinfo_url->push(url);
      }

   U_INTERNAL_ASSERT_EQUALS(vauth->size(), vauth_ip->size())
 
   *ip       = UServer_Base::getIPAddress();
   *hostname = USocketExt::getNodeName();

   host->snprintf("%.*s:%d", U_STRING_TO_TRACE(*ip), UServer_Base::port);

   U_INTERNAL_DUMP("host = %.*S hostname = %.*S ip = %.*S", U_STRING_TO_TRACE(*host), U_STRING_TO_TRACE(*hostname), U_STRING_TO_TRACE(*ip))

   UUDPSocket cClientSocket(UClientImage_Base::bIPv6);

   auth_ip = (*vauth_ip)[0];

   if (cClientSocket.connectServer(auth_ip, 1001))
      {
      _ip = UString(cClientSocket.getLocalInfo());

      if (_ip != *ip)
         {
         (void) UFile::writeToTmpl("/tmp/IP_ADDRESS", _ip);

         U_SRV_LOG("SERVER IP ADDRESS differ from IP address: %.*S to connect to AUTH: %.*S", U_STRING_TO_TRACE(_ip), U_STRING_TO_TRACE(auth_ip));
         }
      }

   auth_ip = vauth_ip->join(U_CONSTANT_TO_PARAM(" "));

   fw_env->snprintf_add("'AuthServiceIP=%.*s'\n", U_STRING_TO_TRACE(auth_ip));

   // crypto cmd

// if (decrypt_cmd->empty() == false) pgp.set(decrypt_cmd, (char**)0);
// if (decrypt_key->empty() == false)
      {
      U_INTERNAL_ASSERT(decrypt_key->isNullTerminated())
      
      UDES3::setPassword(decrypt_key->data());
      }

   // firewall cmd

   UString command(500U);

   command.snprintf("/bin/sh %.*s initialize openlist", U_STRING_TO_TRACE(*fw_cmd));

   fw->set(command, (char**)0);
   fw->setEnvironment(fw_env);

   // uclient cmd

   (void) command.assign(U_CONSTANT_TO_PARAM("/usr/sbin/uclient -c /etc/uclient.conf {url}"));

   uclient->set(command, (char**)0);

   // uploader cmd

   (void) command.assign(U_CONSTANT_TO_PARAM("/usr/sbin/uclient -c /etc/uclient.conf -u $FILE {url}"));

   uploader->set(command, (char**)0);
   uploader->setFileArgument();

   fd_stderr = UServices::getDevNull("/tmp/mod_nocat.err");

   // users table

   peers = U_NEW(UHashMap<UModNoCatPeer*>);

   peers->allocate();

   openlist       = U_NEW(UVector<UString>);
   status_content = U_NEW(UString(U_CAPACITY));
 
   // users traffic

   ipt = U_NEW(UIptAccount(UClientImage_Base::bIPv6));

   // NB: needed because all instance try to close the log... (inherits from its parent)

   unatexit = (UServer_Base::isLog() ? &ULog::close : 0);

   UServer_Base::monitoring_process = true;

   U_INTERNAL_ASSERT_EQUALS(UPing::addrmask, 0)

   UPing::addrmask = (fd_set*) UServer_Base::getOffsetToDataShare(sizeof(fd_set) + sizeof(uint32_t));

   U_SRV_LOG("Initialization of plugin success");

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UNoCatPlugIn::handlerFork()
{
   U_TRACE(0, "UNoCatPlugIn::handlerFork()")

   uint32_t i, n;

   // manage internal device...

   U_INTERNAL_DUMP("num_radio = %u", num_radio)

   sockp = (UPing**)                UMemoryPool::_malloc(num_radio, sizeof(UPing*));
   vaddr = (UVector<UIPAddress*>**) UMemoryPool::_malloc(num_radio, sizeof(UVector<UIPAddress*>*));

   UPing::addrmask = (fd_set*) UServer_Base::getPointerToDataShare(UPing::addrmask);

   for (i = 0; i < num_radio; ++i)
      {
      vaddr[i] = U_NEW(UVector<UIPAddress*>);
      sockp[i] = U_NEW(UPing(3000, UClientImage_Base::bIPv6));

      if (((check_type & U_CHECK_ARP_PING) != 0))
         {
         sockp[i]->setLocal(UServer_Base::socket->localIPAddress());

#     ifdef HAVE_NETPACKET_PACKET_H
         U_INTERNAL_ASSERT((*vInternalDevice)[i].isNullTerminated())

         sockp[i]->initArpPing((*vInternalDevice)[i].data());
#     endif
         }
      }

   // manage internal timer...

   if (login_timeout ||
       UTimeVal::notZero())
      {
      UTimer::init(true); // async...

      U_SRV_LOG("Initialization of timer success");

      if (UTimeVal::notZero())
         {
         UTimer::insert(this, true);

         U_SRV_LOG("Monitoring set for every %d secs", UTimeVal::getSecond());
         }
      }

   // send start to portal

   UString msg(300U), output, allowed_web_hosts(U_CAPACITY);

   msg.snprintf("/start_ap?ap=%.*s@%.*s&public=%.*s:%u&pid=%u",
                     U_STRING_TO_TRACE(*label), U_STRING_TO_TRACE(*hostname),
                     U_STRING_TO_TRACE(*ip), UServer_Base::port, UServer_Base::pid);

   for (i = 0, n = (*vinfo_url).size(); i < n; ++i)
      {
      (void) sendMsgToPortal(*uclient, i, msg, &output); // NB: we ask for output so that we wait for the response...

      (void) allowed_web_hosts.append(output);
      }

   // initialize the firewall: direct all port 80 traffic to us...

   fw->setArgument(3, "initialize");
   fw->setArgument(4, allowed_web_hosts.data());

   (void) fw->executeAndWait(0, -1, fd_stderr);

   UServer_Base::logCommandMsgError(fw->getCommand(), false);

   if (allowed_members->empty() == false)
      {
      // 00:27:22:4f:69:f4 172.16.1.1 Member ### routed_ap-locoM2

      UVector<UString> vtmp;

      istrstream is(U_STRING_TO_PARAM(*allowed_members));

      vtmp.readVector(is);

      n = vtmp.size();

      uint32_t UserUploadRate,
               UserDownloadRate,
               increment = ((n % 3)
                              ?                                            5
                              : (UserDownloadRate = 0, UserUploadRate = 0, 3));

      for (i = 0; i < n; i += increment)
         {
         UModNoCatPeer* peer = U_NEW(UModNoCatPeer);

         peer->mac = vtmp[i];
         peer->ip  = vtmp[i+1];

         U_INTERNAL_ASSERT_EQUALS(vtmp[i+2], "Member")

         if (increment == 5)
            {
            UserDownloadRate = vtmp[i+3].strtol();
            UserUploadRate   = vtmp[i+4].strtol();
            }

         U_peer_allowed(peer) = true;

         peer->ip.copy(peer->UIPAddress::pcStrAddress);

         U_ipaddress_StrAddressUnresolved(peer) = false;

         peer->UIPAddress::pcStrAddress[peer->ip.size()] = '\0';

         U_INTERNAL_DUMP("peer->UIPAddress::pcStrAddress = %S", peer->UIPAddress::pcStrAddress)

         setNewPeer(peer, getIndexAUTH(peer->UIPAddress::pcStrAddress));

         permit(peer, UserDownloadRate, UserUploadRate);
         }
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int UNoCatPlugIn::handlerRequest()
{
   U_TRACE(1, "UNoCatPlugIn::handlerRequest()")

   if (UHTTP::isRequestNotFound())
      {
      Url url;
      int refresh = 0;
      UModNoCatPeer* peer = 0;
      UString _host(U_HTTP_HOST_TO_PARAM), buffer(U_CAPACITY), request_uri = UHTTP::getRequestURI();

      U_INTERNAL_DUMP("_host = %.*S UServer_Base::getClientAddress() = %S", U_STRING_TO_TRACE(_host), UServer_Base::getClientAddress())

#  if !defined(HAVE_PTHREAD_H) || !defined(ENABLE_THREAD)
      U_INTERNAL_ASSERT_EQUALS(u_pthread_time, 0)
#  endif

      if (UServer_Base::isLog() == false) (void) U_SYSCALL(gettimeofday, "%p,%p", u_now, 0);

      // ----------------------------------------------
      // NB: check for request from AUTH, which may be:
      // ----------------------------------------------
      // a) /users  - report list ip of peers permitted
      // b) /check  - check system and report info
      // c) /status - report status user
      // d) /logout - logout specific user
      // ----------------------------------------------

      uint32_t index_AUTH = vauth_ip->contains(UServer_Base::getClientAddress());

      if (index_AUTH != U_NOT_FOUND)
         {
         U_SRV_LOG("AUTH(%u) request: %.*S alias %.*S", index_AUTH, U_STRING_TO_TRACE(request_uri), U_HTTP_URI_TO_TRACE);

         if (U_HTTP_URI_OR_ALIAS_STRNEQ(request_uri, "/check"))
            {
            if (flag_check_system == false) checkSystem(false);

            notifyAuthOfUsersInfo(index_AUTH);

            goto end;
            }

         if (U_HTTP_URI_OR_ALIAS_STRNEQ(request_uri, "/status"))
            {
            UString _label;

            if (U_HTTP_QUERY_STRNEQ("ip="))
               {
               // NB: request from AUTH to get status user

               uint32_t len    = u_http_info.query_len - U_CONSTANT_SIZE("ip=");
               const char* ptr = u_http_info.query     + U_CONSTANT_SIZE("ip=");

               UString peer_ip = getIPAddress(ptr, len);

               U_SRV_LOG("AUTH request to get status for user: IP %.*s", U_STRING_TO_TRACE(peer_ip));

               peer = (*peers)[peer_ip];

               U_INTERNAL_DUMP("peer = %p", peer)

               if (peer == 0)
                  {
                  UHTTP::setBadRequest();

                  goto end;
                  }
               }
            else if (U_HTTP_QUERY_STRNEQ("label="))
               {
               // NB: request from AUTH to get status for users of local network...

               uint32_t len    = u_http_info.query_len - U_CONSTANT_SIZE("label=");
               const char* ptr = u_http_info.query     + U_CONSTANT_SIZE("label=");

               (void) _label.assign(ptr, len);

               U_SRV_LOG("AUTH request to get status of local network users: %.*S", U_STRING_TO_TRACE(_label));
               }

            // status

            setStatusContent(peer, _label); // NB: peer == 0 -> request from AUTH to get status access point...

            goto end;
            }

         if (U_HTTP_URI_OR_ALIAS_STRNEQ(request_uri, "/logout") &&
             u_http_info.query_len)
            {
            // NB: request from AUTH to logout user (ip=192.168.301.223&mac=00:e0:4c:d4:63:f5)

            UString data = getSignedData(U_HTTP_QUERY_TO_PARAM);

            if (data.empty())
               {
               U_SRV_LOG("AUTH request to logout user tampered!");

               goto set_redirection_url;
               }

            uint32_t len    = data.size() - U_CONSTANT_SIZE("ip=");
            const char* ptr = data.data() + U_CONSTANT_SIZE("ip=");

            UString peer_ip = getIPAddress(ptr, len);

            U_SRV_LOG("AUTH request to logout user: IP %.*s", U_STRING_TO_TRACE(peer_ip));

            peer = (*peers)[peer_ip];

            if (peer == 0)
               {
               uint32_t pos = data.find_first_of('&', 3);

               if (pos != U_NOT_FOUND &&
                   U_STRNEQ(data.c_pointer(pos+1), "mac="))
                  {
                  UString mac = data.substr(pos + 5, U_CONSTANT_SIZE("00:00:00:00:00:00"));

                  U_SRV_LOG("AUTH request to logout user: MAC %.*s", U_STRING_TO_TRACE(mac));

                  peer = getPeerFromMAC(mac);
                  }
               }

            if (peer == 0) UHTTP::setBadRequest();
            else
               {
               if (U_peer_status(peer) != UModNoCatPeer::PEER_PERMIT)
                  {
                  setStatusContent(peer, UString::getStringNull()); // NB: peer == 0 -> request from AUTH to get status access point...
                  }
               else
                  {
                  (void) peer->checkPeerInfo(true);

                  deny(peer, false);

                  notifyAuthOfUsersInfo(index_AUTH);
                  }
               }

            goto end;
            }

         if (U_HTTP_URI_OR_ALIAS_STRNEQ(request_uri, "/users"))
            {
            // NB: request from AUTH to get list info on peers permitted

            U_SRV_LOG("AUTH request to get list info on peers permitted");

            setPeerListInfo();

            goto end;
            }

         UHTTP::setBadRequest();

         goto end;
         }

      // NB: check for strange initial WiFi behaviour of the iPhone...

      if (U_HTTP_URI_STRNEQ("/library/test/success.html")   &&
          _host.equal(U_CONSTANT_TO_PARAM("www.apple.com")) &&
          u_find(U_HTTP_USER_AGENT_TO_PARAM, U_CONSTANT_TO_PARAM("CaptiveNetworkSupport")) != 0)
         {
         /* When the iPhone automatically assesses the WiFi connection they appear to
          * issue a HTTP GET request to http://www.apple.com/library/test/success.html.
          * This is issued by a User Agent 'CaptiveNetworkSupport' and it does not attempt
          * to use any proxy that may be configured on the iPhone. This attempt will obviously
          * result in a 404 failure and the WLAN text. After this the iPhone attempts to connect
          * to www.apple.com again, this time through Safari to some-kind of login page, again
          * resulting in the WLAN text. The WiFi link on the iPhone is then discarded. It was clear
          * that initial connection needed to succeed. When that connection was attempted a single
          * page of HTML is returned which has the word 'Success' in it. Once this behaviour had
          * been characterised casual Internet searching found that several people had noted this
          * behaviour as well, and speculated upon its meaning. What is clear is that Apple fail
          * to document it. Since the iPhone needs a positive response to its strange little query
          * it was decided to give it one.
          */

         U_SRV_LOG("Detected strange initial WiFi request (iPhone) from peer: IP %s", UServer_Base::getClientAddress());

         setHTTPResponse(*str_IPHONE_SUCCESS);

         goto end;
         }

      // ---------------------------------------------------------------
      // NB: other kind of message, which may be:
      // ---------------------------------------------------------------
      // a) /cpe            - specific request, force redirect via https 
      // b) /test           - force redirect even without a firewall
      // e) /ticket         - authorization ticket with info
      // h) /login_validate - before authorization ticket with info
      // ---------------------------------------------------------------

      index_AUTH = getIndexAUTH(UServer_Base::getClientAddress());
      url        = *((*vauth_url)[index_AUTH]);
      peer       = (*peers)[UServer_Base::getClientAddress()];

      if (U_HTTP_URI_OR_ALIAS_STRNEQ(request_uri, "/cpe"))
         {
         (void) buffer.assign(U_CONSTANT_TO_PARAM("http://www.google.com"));

         url.setPath(U_CONSTANT_TO_PARAM("/cpe"));
         url.setService(U_CONSTANT_TO_PARAM("https"));

         goto set_redirect_to_AUTH;
         }

      if (U_HTTP_URI_OR_ALIAS_STRNEQ(request_uri, "/test"))
         {
         (void) buffer.assign(U_CONSTANT_TO_PARAM("http://www.google.com"));

         goto set_redirect_to_AUTH;
         }

      if (U_HTTP_URI_OR_ALIAS_STRNEQ(request_uri, "/login_validate") &&
          u_http_info.query_len)
         {
         // user has pushed the login button

         UString data = getSignedData(U_HTTP_QUERY_TO_PARAM);

         if (data.empty())
            {
            U_SRV_LOG("AUTH request to validate login tampered!");

            goto set_redirection_url;
            }

         UString uid;
         uint32_t len    = data.size() - U_CONSTANT_SIZE("uid=");
         const char* ptr = data.data() + U_CONSTANT_SIZE("uid=");

         (void) buffer.assign(ptr, len);

         len = buffer.find('&');

         if (len == U_NOT_FOUND) (void) uid.replace(buffer);
         else                    (void) uid.assign(ptr, len);

         U_SRV_LOG("AUTH request to validate login request for uid %.*S from peer: IP %s", U_STRING_TO_TRACE(uid), UServer_Base::getClientAddress());

         if (vLoginValidate->isContained(uid) == false) vLoginValidate->push(uid);

         (void) buffer.assign(U_HTTP_QUERY_TO_PARAM); // NB: we resend the same data to portal... (as redirect field)

         url.setPath(U_CONSTANT_TO_PARAM("/login_validate"));

         refresh = 2; // no body

         goto set_redirect_to_AUTH;
         }

      if (U_HTTP_URI_OR_ALIAS_STRNEQ(request_uri, "/ticket") &&
          U_HTTP_QUERY_STRNEQ("ticket="))
         {
         // user with a ticket

         uint32_t len    = u_http_info.query_len - U_CONSTANT_SIZE("ticket=");
         const char* ptr = u_http_info.query     + U_CONSTANT_SIZE("ticket=");

         UString data = getSignedData(ptr, len);

         if (data.empty())
            {
            U_SRV_LOG("Invalid ticket from peer: IP %s", UServer_Base::getClientAddress());

            goto set_redirection_url;
            }

         if (UServer_Base::isLog())
            {
            UString printable(data.size() * 4);

            UEscape::encode(data, printable, false);

            UServer_Base::log->log("%.*sauth message: %.*s\n", U_STRING_TO_TRACE(*UServer_Base::mod_name), U_STRING_TO_TRACE(printable));
            }

         if (peer == 0 ||
             checkAuthMessage(peer, data) == false)
            {
            goto set_redirection_url;
            }

         // OK: go to the destination (with Location: ...)

         goto redirect;
         }

      if (peer                  &&
          U_http_version == '1' &&
          _host          == peer->gateway)
         {
         U_SRV_LOG("Missing ticket from peer: IP %s", UServer_Base::getClientAddress());
         }

set_redirection_url:
      (void) buffer.reserve(7 + U_http_host_len + u_http_info.uri_len);

      buffer.snprintf("http://%.*s%.*s", U_STRING_TO_TRACE(_host), U_HTTP_URI_TO_TRACE);

set_redirect_to_AUTH:
      if  (peer)  checkOldPeer(peer);
      else peer = creatNewPeer(index_AUTH);

      setRedirectLocation(peer, buffer, url);

      if (refresh == 0) refresh = 1;

redirect:
      UHTTP::setRedirectResponse(refresh, UString::getStringNull(), U_STRING_TO_PARAM(*location));
      
end:
      UHTTP::setRequestProcessed();
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int UNoCatPlugIn::handlerReset()
{
   U_TRACE(0, "UNoCatPlugIn::handlerReset()")

   U_INTERNAL_DUMP("U_http_method_len = %u", U_http_method_len)

   checkSystem(UServer_Base::isLog());

   UServer_Base::bpluginsHandlerReset = false;

   U_http_method_len = 0;

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UModNoCatPeer::dump(bool _reset) const
{
   *UObjectIO::os << "expire                " << expire            <<  '\n'
                  << "logout                " << logout            <<  '\n'
                  << "ctraffic              " << ctraffic          <<  '\n'
                  << "connected             " << connected         <<  '\n'
                  << "time_remain           " << time_remain       <<  '\n'
                  << "traffic_done          " << traffic_done      <<  '\n'
                  << "traffic_remain        " << traffic_remain    <<  '\n'
                  << "time_no_traffic       " << time_no_traffic   <<  '\n'
                  << "traffic_available     " << traffic_available <<  '\n'
                  << "ip        (UString    " << (void*)&ip        << ")\n"
                  << "mac       (UString    " << (void*)&mac       << ")\n"
                  << "token     (UString    " << (void*)&token     << ")\n"
                  << "label     (UString    " << (void*)&label     << ")\n"
                  << "ifname    (UString    " << (void*)&ifname    << ")\n"
                  << "gateway   (UString    " << (void*)&gateway   << ")\n"
                  << "fw        (UCommand   " << (void*)&fw        << ')';

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

const char* UNoCatPlugIn::dump(bool _reset) const
{
   U_CHECK_MEMORY

   *UObjectIO::os << "nfds                                           " << nfds                        <<  '\n'
                  << "vaddr                                          " << (void*)vaddr                <<  '\n' 
                  << "sockp                                          " << (void*)sockp                <<  '\n'
                  << "unatexit                                       " << (void*)unatexit             <<  '\n'
                  << "paddrmask                                      " << (void*)paddrmask            <<  '\n'
                  << "fd_stderr                                      " << fd_stderr                   <<  '\n'
                  << "num_radio                                      " << num_radio                   <<  '\n'
                  << "time_available                                 " << time_available              <<  '\n'
                  << "next_event_time                                " << next_event_time             <<  '\n'
                  << "total_connections                              " << total_connections           <<  '\n'
                  << "traffic_available                              " << traffic_available           <<  '\n'
                  << "flag_check_system                              " << flag_check_system           <<  '\n'
                  << "last_request_check                             " << last_request_check          <<  '\n'
                  << "last_request_firewall                          " << last_request_firewall       <<  '\n'
                  << "ip                   (UString                  " << (void*)ip                   << ")\n"
                  << "host                 (UString                  " << (void*)host                 << ")\n"
                  << "label                (UString                  " << (void*)label                << ")\n"
                  << "input                (UString                  " << (void*)input                << ")\n"
                  << "ifname               (UString                  " << (void*)ifname               << ")\n"
                  << "extdev               (UString                  " << (void*)extdev               << ")\n"
                  << "fw_cmd               (UString                  " << (void*)fw_cmd               << ")\n"
                  << "hostname             (UString                  " << (void*)hostname             << ")\n"
                  << "localnet             (UString                  " << (void*)localnet             << ")\n"
                  << "location             (UString                  " << (void*)location             << ")\n"
                  << "auth_login           (UString                  " << (void*)auth_login           << ")\n"
                  << "decrypt_key          (UString                  " << (void*)decrypt_key          << ")\n"
                  << "login_timeout        (UString                  " << (void*)login_timeout        << ")\n"
                  << "status_content       (UString                  " << (void*)status_content       << ")\n"
                  << "allowed_members      (UString                  " << (void*)allowed_members      << ")\n"
                  << "fw                   (UCommand                 " << (void*)fw                   << ")\n"
                  << "uclient              (UCommand                 " << (void*)uclient              << ")\n"
                  << "uploader             (UCommand                 " << (void*)uploader             << ")\n"
                  << "ipt                  (UIptAccount              " << (void*)ipt                  << ")\n"
                  << "vauth_url            (UVector<Url*>            " << (void*)vauth_url            << ")\n"
                  << "vinfo_url            (UVector<Url*>            " << (void*)vinfo_url            << ")\n"
                  << "vauth                (UVector<UString>         " << (void*)vauth                << ")\n"
                  << "vauth_ip             (UVector<UString>         " << (void*)vauth_ip             << ")\n"
                  << "vLocalNetwork        (UVector<UString>         " << (void*)vLocalNetwork        << ")\n"
                  << "vInternalDevice      (UVector<UString>         " << (void*)vInternalDevice      << ")\n"
                  << "vLocalNetworkLabel   (UVector<UString>         " << (void*)vLocalNetworkLabel   << ")\n"
                  << "vLocalNetworkGateway (UVector<UString>         " << (void*)vLocalNetworkGateway << ")\n"
                  << "peers                (UHashMap<UModNoCatPeer*> " << (void*)peers                << ')';

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
