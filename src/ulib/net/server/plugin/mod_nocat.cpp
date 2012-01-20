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
#include <ulib/utility/base64.h>
#include <ulib/utility/escape.h>
#include <ulib/net/ipt_ACCOUNT.h>
#include <ulib/utility/services.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/string_ext.h>
#include <ulib/net/server/client_image.h>
#include <ulib/net/server/plugin/mod_nocat.h>

#ifdef HAVE_STRSTREAM_H
#  include <strstream.h>
#else
#  include <ulib/replace/strstream.h>
#endif

#ifndef   ULLONG_MAX
#  define ULLONG_MAX 0xffffffffffffffffULL
#endif

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_nocat, UNoCatPlugIn)
#endif

const UString* UNoCatPlugIn::str_ROUTE_ONLY;
const UString* UNoCatPlugIn::str_DNS_ADDR;
const UString* UNoCatPlugIn::str_INCLUDE_PORTS;
const UString* UNoCatPlugIn::str_EXCLUDE_PORTS;
const UString* UNoCatPlugIn::str_ALLOWED_WEB_HOSTS;
const UString* UNoCatPlugIn::str_EXTERNAL_DEVICE;
const UString* UNoCatPlugIn::str_INTERNAL_DEVICE;
const UString* UNoCatPlugIn::str_INTERNAL_DEVICE_LABEL;
const UString* UNoCatPlugIn::str_LOCAL_NETWORK;
const UString* UNoCatPlugIn::str_AUTH_SERVICE_URL;
const UString* UNoCatPlugIn::str_LOGOUT_URL;
const UString* UNoCatPlugIn::str_LOGIN_TIMEOUT;
const UString* UNoCatPlugIn::str_DECRYPT_CMD;
const UString* UNoCatPlugIn::str_DECRYPT_KEY;
const UString* UNoCatPlugIn::str_FW_CMD;
const UString* UNoCatPlugIn::str_CHECK_BY_ARPING;
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
const UString* UNoCatPlugIn::str_anonymous;
const UString* UNoCatPlugIn::str_Traffic;
const UString* UNoCatPlugIn::str_GATEWAY_PORT;
const UString* UNoCatPlugIn::str_FW_ENV;
const UString* UNoCatPlugIn::str_IPHONE_SUCCESS;
const UString* UNoCatPlugIn::str_CHECK_EXPIRE_INTERVAL;

int                       UNoCatPlugIn::fd_stderr;
vPF                       UNoCatPlugIn::unatexit;
char                      UNoCatPlugIn::pcStrAddress[INET6_ADDRSTRLEN];
bool                      UNoCatPlugIn::arping;
bool                      UNoCatPlugIn::flag_check_peers_for_info;
time_t                    UNoCatPlugIn::last_request_check;
UPing**                   UNoCatPlugIn::sockp;
fd_set*                   UNoCatPlugIn::addrmask;
uint32_t                  UNoCatPlugIn::nfds;
uint32_t                  UNoCatPlugIn::num_radio;
uint32_t                  UNoCatPlugIn::index_AUTH;
uint32_t                  UNoCatPlugIn::total_connections;
uint32_t                  UNoCatPlugIn::login_timeout;
UString*                  UNoCatPlugIn::status_content;
UIptAccount*              UNoCatPlugIn::ipt;
UNoCatPlugIn*             UNoCatPlugIn::pthis;
UVector<UIPAddress*>**    UNoCatPlugIn::vaddr;
UHashMap<UModNoCatPeer*>* UNoCatPlugIn::peers;

#define U_NO_MORE_TIME       10
#define U_NOCAT_MAX_TIMEOUT (30 * U_ONE_DAY_IN_SECOND)

#define U_NOCAT_STATUS \
"<html>\n" \
"<head><title>Access Point: %s</title></head>\n" \
"<body bgcolor=\"#FFFFFF\" text=\"#000000\">\n" \
"<h1>Access Point: %s</h1>\n" \
"<hr noshade=\"1\"/>\n" \
"<table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">\n" \
   "<tr><td>Current Time</td><td>%7D</td></tr>\n" \
   "<tr><td>Gateway Up Since</td><td>%#7D</td></tr>\n" \
   "<tr><td>GatewayVersion</td><td>" ULIB_VERSION "</td></tr>\n" \
   "<tr><td>RouteOnly</td><td>%.*s</td></tr>\n" \
   "<tr><td>DNSAddr</td><td>%.*s</td></tr>\n" \
   "<tr><td>IncludePorts</td><td>%.*s</td></tr>\n" \
   "<tr><td>ExcludePorts</td><td>%.*s</td></tr>\n" \
   "<tr><td>AllowedWebHosts</td><td>%.*s</td></tr>\n" \
   "<tr><td>ExternalDevice</td><td>%.*s</td></tr>\n" \
   "<tr><td>InternalDevice</td><td>%.*s</td></tr>\n" \
   "<tr><td>LocalNetwork</td><td>%.*s</td></tr>\n" \
   "<tr><td>GatewayPort</td><td>%u</td></tr>\n" \
   "<tr><td>AuthServiceAddr</td><td>%.*s</td></tr>\n" \
   "<tr><td>LoginTimeout</td><td>%u</td></tr>\n" \
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

// status

void UNoCatPlugIn::getPeerStatus(UStringRep* key, void* value)
{
   U_TRACE(0, "UNoCatPlugIn::getPeerStatus(%.*S,%p)", U_STRING_TO_TRACE(*key), value)

   UModNoCatPeer* peer = (UModNoCatPeer*) value;

   U_INTERNAL_ASSERT(peer->mac.isNullTerminated())

   char c;
   const char* color;
   const char* status;
   uint64_t how_much_traffic;
   UString buffer(U_CAPACITY);
   const char* mac = peer->mac.data();
   time_t how_much_connected, how_much_remain;

   U_INTERNAL_DUMP("now    = %#4D", u_now->tv_sec)
   U_INTERNAL_DUMP("logout = %#4D", peer->logout)
   U_INTERNAL_DUMP("expire = %#4D", peer->expire)

   if (login_timeout                 &&
       peer->expire <= u_now->tv_sec &&
       peer->status == UModNoCatPeer::PEER_ACCEPT) deny(peer, false, false);

   if (peer->status == UModNoCatPeer::PEER_ACCEPT)
      {
      color  = "green";
      status = "PERMIT";

      how_much_connected = u_now->tv_sec - peer->connected;
      how_much_remain    = (peer->expire > u_now->tv_sec ? (peer->expire - u_now->tv_sec) : 0);
      }
   else
      {
      color  = "red";
      status = "DENY";

      how_much_connected = peer->logout - peer->connected;
      how_much_remain    = peer->expire - peer->logout;
      }

   U_INTERNAL_DUMP("ltraffic = %llu traffic = %llu", peer->ltraffic, peer->traffic)

   how_much_traffic = (peer->ltraffic > peer->traffic ? (peer->ltraffic - peer->traffic) : 0);

   U_INTERNAL_DUMP("how_much_traffic = %llu", how_much_traffic)

   how_much_traffic /= 1024;

   if (how_much_traffic < 1024) c = 'K';
      {
      c = 'M';

      how_much_traffic /= 1024;
      }

   buffer.snprintf("<tr>\n"
                     "<td>%.*s</td>\n"
                     "<td>%.*s</td>\n"
                     "<td>%#7D</td>\n"
                     "<td>%#3D</td>\n"
                     "<td>%#3D</td>\n"
                     "<td>%llu KBytes</td>\n"
                     "<td>%llu %cBytes</td>\n"
                     "<td><a href=\"http://standards.ieee.org/cgi-bin/ouisearch?%c%c%c%c%c%c\">%s</a></td>\n"
                     "<td style=\"color:%s\">%s</td>\n"
                   "</tr>\n",
                   U_STRING_TO_TRACE(peer->user),
                   U_STRING_TO_TRACE(peer->ip),
                   peer->connected + u_now_adjust,
                   how_much_connected,
                   how_much_remain,
                   peer->traffic / 1024, how_much_traffic, c,
                   mac[0], mac[1], mac[3], mac[4], mac[6], mac[7], mac,
                   color, status);

   (void) status_content->append(buffer);
}

void UNoCatPlugIn::setStatusContent(UModNoCatPeer* peer)
{
   U_TRACE(0, "UNoCatPlugIn::setStatusContent(%p)", peer)

   status_content->setEmpty();

   if (peer) getPeerStatus(peer->ip.rep, peer);
   else
      {
      peers->callForAllEntry(getPeerStatus);

      UString buffer(800U + sizeof(U_NOCAT_STATUS) + status_content->size());

      U_INTERNAL_ASSERT(access_point.isNullTerminated())

      const char* name = access_point.data();

      buffer.snprintf(U_NOCAT_STATUS, name, name, u_start_time,
                      U_STRING_TO_TRACE(route_only), U_STRING_TO_TRACE(dns_addr), U_STRING_TO_TRACE(include_ports),
                      U_STRING_TO_TRACE(exclude_ports), U_STRING_TO_TRACE(allowed_web_hosts), U_STRING_TO_TRACE(extdev),
                      U_STRING_TO_TRACE(intdev), U_STRING_TO_TRACE(localnet), UServer_Base::port,
                      U_STRING_TO_TRACE(auth_login),
                      login_timeout, peers->size(), total_connections,
                      U_STRING_TO_TRACE(*status_content), "/images/auth_logo.png");

      *status_content = buffer;
      }

   setHTTPResponse(*status_content);
}

// VARIE

UModNoCatPeer::UModNoCatPeer(const UString& peer_ip) : ip(peer_ip)
{
   U_TRACE_REGISTER_OBJECT(0, UModNoCatPeer, "%.*S", U_STRING_TO_TRACE(peer_ip))

   ip.duplicate();

   ifname = USocketExt::getNetworkInterfaceName(ip);

   U_INTERNAL_DUMP("ifname = %.*S", U_STRING_TO_TRACE(ifname))

   ifindex = (ifname.empty() ? 0 : UNoCatPlugIn::pthis->vInternalDevice.find(ifname));

   U_INTERNAL_DUMP("ifindex = %u", ifindex)

   U_INTERNAL_ASSERT(ifindex <= UNoCatPlugIn::num_radio)

   label = (UNoCatPlugIn::pthis->vInternalDeviceLabel.empty() ? UNoCatPlugIn::pthis->vInternalDevice[ifindex]
                                                              : UNoCatPlugIn::pthis->vInternalDeviceLabel[ifindex]);

   status = UModNoCatPeer::PEER_DENY;

   // set MAC address

   U_INTERNAL_ASSERT(ip.isNullTerminated())

   mac = UServer_Base::getMacAddress(ip.data());

   // user name

   user = *UNoCatPlugIn::str_anonymous;

   // set connection time

   ctime = connected = expire = logout = u_now->tv_sec;

   // set traffic

   ctraffic = traffic = 0;
   ltraffic = ULLONG_MAX;
}

// define method VIRTUAL of class UEventTime

int UModNoCatPeer::handlerTime()
{
   U_TRACE(0, "UModNoCatPeer::handlerTime()")

   UNoCatPlugIn::deny(this, true, false);

   // return value:
   // ---------------
   // -1 - normal
   //  0 - monitoring
   // ---------------

   U_RETURN(-1);
}

int UNoCatPlugIn::handlerTime()
{
   U_TRACE(0, "UNoCatPlugIn::handlerTime()")

   // NB: check if there is now a http request processing...

   if (u_http_info.method == 0) checkPeersForInfo();
   else                         flag_check_peers_for_info = true;

   // return value:
   // ---------------
   // -1 - normal
   //  0 - monitoring
   // ---------------

   U_RETURN(0);
}

UNoCatPlugIn::UNoCatPlugIn() : vauth_service_url(4U), vlogout_url(4U), vinfo_url(4U),
                               vInternalDevice(64U), vInternalDeviceLabel(64U), vLocalNetwork(64U),
                               vauth_login(4U), vauth_logout(4U), vauth_ip(4U),
                               input(U_CAPACITY), output(U_CAPACITY), location(U_CAPACITY)
{
   U_TRACE_REGISTER_OBJECT(0, UNoCatPlugIn, "")

   if (str_AUTH_SERVICE_URL == 0) str_allocate();
}

UNoCatPlugIn::~UNoCatPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, UNoCatPlugIn)

   if (UEventTime::notZero() || login_timeout)
      {
      UTimer::stop();
      UTimer::clear(true);
      }

   if (ipt)
      {
      delete ipt;

      for (uint32_t i = 0; i < num_radio; ++i)
         {
         delete vaddr[i];
         delete sockp[i];
         }

      U_FREE_VECTOR(sockp, num_radio, UPing);
      U_FREE_VECTOR(vaddr, num_radio, UVector<UIPAddress*>);
      }

   if (peers)
      {
      peers->clear();
      peers->deallocate();

      delete peers;
      }

   if (status_content) delete status_content;
}

void UNoCatPlugIn::executeCommand(UModNoCatPeer* peer, int type)
{
   U_TRACE(0, "UNoCatPlugIn::executeCommand(%p,%d)", peer, type)

   peer->cmd.setArgument(3, (type == UModNoCatPeer::PEER_ACCEPT ? "permit" : "deny"));

   if (peer->cmd.execute(0, 0, -1, fd_stderr))
      {
      int pid, status;
retry:
      pid = UProcess::waitpid(-1, &status, WNOHANG); // NB: to avoid too much zombie...

      if (pid > 0)
         {
         U_SRV_LOG("child (pid %d) exited with value %d (%s)", pid, status, UProcess::exitInfo(status));

         goto retry;
         }

      peer->status = type;
      }

   UServer_Base::logCommandMsgError(peer->cmd.getCommand());
}

void UNoCatPlugIn::deny(UModNoCatPeer* peer, bool alarm, bool disconnected)
{
   U_TRACE(0, "UNoCatPlugIn::deny(%p,%b,%b)", peer, alarm, disconnected)

   if (peer->status == UModNoCatPeer::PEER_DENY)
      {
      U_SRV_LOG("Peer %.*s already deny", U_STRING_TO_TRACE(peer->ip));
      }
   else
      {
      --total_connections;

      U_SRV_LOG("Removing peer %.*s", U_STRING_TO_TRACE(peer->ip));

      bool bdelete;
      time_t t         = peer->expire - u_now->tv_sec;
      uint64_t traffic = (peer->ltraffic > peer->traffic ? (peer->ltraffic - peer->traffic) : 0);

      if (traffic < 1024 ||
          t <= U_NO_MORE_TIME)
         {
         // user with no more time or no more traffic...

         bdelete      = true;
         peer->logout = peer->expire;

         (void) peers->erase(peer->ip);
         }
      else
         {
         // request of logout or user disconnected...

         bdelete      = false;
         peer->logout = u_now->tv_sec;
         }

      pthis->addPeerInfo(peer, (disconnected ? -1 : peer->logout));

      executeCommand(peer, UModNoCatPeer::PEER_DENY);

      if (alarm == false)
         {
         if (login_timeout) UTimer::erase(peer, bdelete, true);

         peer->UEventTime::reset();
         }
      }
}

void UNoCatPlugIn::permit(UModNoCatPeer* peer, time_t timeout)
{
   U_TRACE(0, "UNoCatPlugIn::permit(%p,%ld)", peer, timeout)

   if (peer->status == UModNoCatPeer::PEER_ACCEPT)
      {
      U_SRV_LOG("Peer %.*s already permit", U_STRING_TO_TRACE(peer->ip));
      }
   else
      {
      U_SRV_LOG("Accepting peer %.*s", U_STRING_TO_TRACE(peer->ip));

      executeCommand(peer, UModNoCatPeer::PEER_ACCEPT);

      // set connection time

      peer->expire = (peer->ctime = peer->connected = peer->logout = u_now->tv_sec) + timeout;

      if (timeout > U_NOCAT_MAX_TIMEOUT) timeout = U_NOCAT_MAX_TIMEOUT; // check for safe timeout...

      peer->UEventTime::setSecond(timeout);

      if (login_timeout) UTimer::insert(peer, true);

      ++total_connections;
      }
}

void UNoCatPlugIn::setRedirectLocation(UModNoCatPeer* peer, const UString& redirect, const Url& auth)
{
   U_TRACE(0, "UNoCatPlugIn::setRedirectLocation(%p,%.*S,%.*S)", peer, U_STRING_TO_TRACE(redirect), U_URL_TO_TRACE(auth))

   UString value_encoded(U_CAPACITY);

   Url::encode(peer->mac, value_encoded);

   location.snprintf("%.*s?mac=%.*s&ip=", U_URL_TO_TRACE(auth), U_STRING_TO_TRACE(value_encoded));

   Url::encode(peer->ip, value_encoded);

   location.snprintf_add("%.*s&redirect=", U_STRING_TO_TRACE(value_encoded));

   Url::encode(redirect, value_encoded);

   location.snprintf_add("%.*s&gateway=", U_STRING_TO_TRACE(value_encoded));

   Url::encode(gateway, value_encoded);

   location.snprintf_add("%.*s&timeout=%ld&token=", U_STRING_TO_TRACE(value_encoded), login_timeout);

   if (peer->token.empty())
      {
      // ---------------------------------------------------------------------------------------------------------------------
      // set crypto token (valid for 30 minutes)...
      // ---------------------------------------------------------------------------------------------------------------------
      // NB: tutto il traffico viene rediretto sulla 80 (CAPTIVE PORTAL) e quindi windows update, antivrus, etc...
      // questo introduce la possibilita' che durante la fase di autorizzazione il token generato per il ticket autorizzativo
      // non corrisponda piu' a quello inviato dal portale per l'autorizzazione...
      // ---------------------------------------------------------------------------------------------------------------------

      time_t _expire = u_now->tv_sec + (30L * 60L);
      peer->token    = UServices::generateToken(peer->mac, _expire);
      }

   location.snprintf_add("%.*s&ap=%.*s@%.*s", U_STRING_TO_TRACE(value_encoded), U_STRING_TO_TRACE(peer->label), U_STRING_TO_TRACE(access_point));
}

bool UNoCatPlugIn::checkSignedData(const char* ptr, uint32_t len)
{
   U_TRACE(0, "UNoCatPlugIn::checkSignedData(%.*S,%u)", len, ptr, len)

   UString buffer(U_CAPACITY);

   if (decrypt_key.empty() == false)
      {
      if (UBase64::decode(ptr, len, buffer)) UDES3::decode(buffer, output);
      else                                   output.setEmpty();
      }
   else
      {
      Url::decode(ptr, len, buffer);

      input.snprintf("-----BEGIN PGP MESSAGE-----\n\n"
                     "%.*s"
                     "\n-----END PGP MESSAGE-----", U_STRING_TO_TRACE(buffer));

      (void) pgp.execute(&input, &output, -1, fd_stderr);

      UServer_Base::logCommandMsgError(pgp.getCommand());
      }

   bool result = (output.empty() == false);

   U_RETURN(result);
}

bool UNoCatPlugIn::checkAuthMessage(UModNoCatPeer* peer)
{
   U_TRACE(0, "UNoCatPlugIn::checkAuthMessage(%p)", peer)

   U_INTERNAL_ASSERT_POINTER(peer)

   time_t _expire;
   Url destination;
   bool result = true;
   UHashMap<UString> args;
   UVector<UString> name_value;
   UString token, action, peer_mac(100U);

   args.allocate();

   istrstream is(output.data(), output.size());

   is >> args;

   /*
   Action   Permit
   Mode     Login
   Redirect http://wi-auth:443/postlogin?uid=s.casazza&gateway=10.30.1.131&redirect=http%3A//stefano%3A5280/pippo
   Mac      00:e0:4c:d4:63:f5
   Timeout  3000
   Token    10.30.1.105&1237907630&05608a4cbd42c9f72d2bd3a0e19ed23f
   User     s.casazza
   */

   // check crypto token (valid for 30 minutes)...

   token = args[*str_Token];

   if (peer->token != token                                       ||
       UServices::getTokenData(peer_mac, token, _expire) == false ||
       peer->mac   != peer_mac)
      {
      U_INTERNAL_DUMP("expire   = %ld", _expire)
      U_INTERNAL_DUMP("token    = %.*S", U_STRING_TO_TRACE(token))
      U_INTERNAL_DUMP("peer     = %.*S", U_STRING_TO_TRACE(peer->token))
      U_INTERNAL_DUMP("peer_mac = %.*S", U_STRING_TO_TRACE(peer_mac))

      U_SRV_LOG("Tampered token from peer %.*s: MAC %.*s", U_STRING_TO_TRACE(peer->ip), U_STRING_TO_TRACE(peer_mac));

      goto error;
      }

   // check mac address

   if (peer->mac != args[*str_Mac])
      {
      U_SRV_LOG("Different MAC %.*S in ticket from peer %.*s", U_STRING_TO_TRACE(peer->mac), U_STRING_TO_TRACE(peer->ip));

      goto error;
      }

   // get action

   action = args[*str_Action];

        if (action == *str_Permit) permit(peer, args[*str_Timeout].strtol());
   else if (action == *str_Deny)     deny(peer, false, false);
   else
      {
      U_SRV_LOG("Can't make sense of action %.*S in ticket from peer %.*s", U_STRING_TO_TRACE(action), U_STRING_TO_TRACE(peer->ip));

      goto error;
      }

   // get redirect (destination)

   destination.set(args[*str_Redirect]);

   if (destination.getQuery(name_value) == 0)
      {
      U_SRV_LOG("Can't make sense of Redirect %.*S in ticket from peer %.*s", U_URL_TO_TRACE(destination), U_STRING_TO_TRACE(peer->ip));

      goto error;
      }

   destination.eraseQuery();

   if (destination.setQuery(name_value) == false)
      {
      U_SRV_LOG("Error on setting Redirect in ticket from peer %.*s", U_STRING_TO_TRACE(peer->ip));

      goto error;
      }

   (void) location.replace(destination.get());

   // set user name

   peer->user = args[*str_User].copy();

   // get mode

   mode = args[*str_Mode].copy();

   // get traffic available

   peer->ltraffic = args[*str_Traffic].strtoll();

   U_INTERNAL_DUMP("ltraffic = %llu", peer->ltraffic)

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

   peer->token.clear();

   U_RETURN(result);
}

UModNoCatPeer* UNoCatPlugIn::creatNewPeer(const UString& peer_ip)
{
   U_TRACE(0, "UNoCatPlugIn::creatNewPeer(%.*S)", U_STRING_TO_TRACE(peer_ip))

   UModNoCatPeer* peer = U_NEW(UModNoCatPeer(peer_ip));

   *((UIPAddress*)peer) = UServer_Base::pClientImage->socket->remoteIPAddress();

   if (peer->mac.empty() && peer_ip == gateway) peer->mac = UServer_Base::getMacAddress(extdev.data());
   if (peer->mac.empty())                       peer->mac = U_STRING_FROM_CONSTANT("00:00:00:00:00:00");

   peer->rulenum    = total_connections + 1; // iptables FORWARD
   peer->index_AUTH = index_AUTH;

   U_INTERNAL_DUMP("peer->rulenum = %u peer->index_AUTH = %u", peer->rulenum, peer->index_AUTH)

   peer->setCommand(fw_cmd);

   peers->insertAfterFind(peer->ip, peer);

   U_RETURN_POINTER(peer, UModNoCatPeer);
}

void UNoCatPlugIn::getTraffic()
{
   U_TRACE(0, "UNoCatPlugIn::getTraffic()")

#ifdef HAVE_LINUX_NETFILTER_IPV4_IPT_ACCOUNT_H
   const char* ip;
   const char* table;
   UModNoCatPeer* peer;
   struct ipt_acc_handle_ip* entry;

   for (uint32_t i = 0, n = vLocalNetwork.size(); i < n; ++i)
      {
      U_INTERNAL_ASSERT(vLocalNetwork[i].isNullTerminated())

      table = vLocalNetwork[i].data();

      if (ipt->readEntries(table, false))
         {
         while ((entry = ipt->getNextEntry()))
            {
            ip = u_inet_nltoa(entry->ip);

            U_INTERNAL_DUMP("IP: %s SRC packets: %u bytes: %u DST packets: %u bytes: %u",
                             ip, entry->src_packets, entry->src_bytes, entry->dst_packets, entry->dst_bytes)

            peer = (*peers)[ip];

            U_INTERNAL_DUMP("peer = %p", peer)

            if (peer)
               {
               peer->ctraffic = entry->src_bytes + entry->dst_bytes;
               peer->traffic += peer->ctraffic;

               U_INTERNAL_DUMP("peer->traffic = %llu peer->ctraffic = %u", peer->traffic, peer->ctraffic)
               }
            }
         }
      }
#endif
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

   U_INTERNAL_ASSERT(     gateway.isNullTerminated())
   U_INTERNAL_ASSERT(    peer->ip.isNullTerminated())
   U_INTERNAL_ASSERT(   peer->mac.isNullTerminated())
   U_INTERNAL_ASSERT(  peer->user.isNullTerminated())
   U_INTERNAL_ASSERT(access_point.isNullTerminated())

   U_INTERNAL_DUMP("u_http_info.method = %p", u_http_info.method)

   if (u_http_info.method || logout > 0)
      {
      Url* info_url = pthis->vinfo_url[peer->index_AUTH];

      U_INTERNAL_DUMP("peer->index_AUTH = %u info_url = %p", peer->index_AUTH, info_url)

      UString buffer(100U);

      buffer.snprintf("%.*s@%.*s", U_STRING_TO_TRACE(peer->label), U_STRING_TO_TRACE(access_point));

      info_url->addQuery(U_STRING_TO_PARAM(*str_Mac),    U_STRING_TO_PARAM(peer->mac));
      info_url->addQuery(U_CONSTANT_TO_PARAM("ip"),      U_STRING_TO_PARAM(peer->ip));
      info_url->addQuery(U_CONSTANT_TO_PARAM("gateway"), U_STRING_TO_PARAM(gateway));
      info_url->addQuery(U_CONSTANT_TO_PARAM("ap"),      U_STRING_TO_PARAM(buffer));
      info_url->addQuery(U_STRING_TO_PARAM(*str_User),   U_STRING_TO_PARAM(peer->user));

      buffer = UStringExt::numberToString(logout); // NB: (-1|0) mean NOT logout (only info)...

      info_url->addQuery(U_CONSTANT_TO_PARAM("logout"), U_STRING_TO_PARAM(buffer));

      buffer = UStringExt::numberToString(u_now->tv_sec - peer->ctime);
                                                          peer->ctime = u_now->tv_sec;

      info_url->addQuery(U_CONSTANT_TO_PARAM("connected"), U_STRING_TO_PARAM(buffer));

      getTraffic();

      buffer.snprintf("%u", peer->ctraffic);
                            peer->ctraffic = 0;

      info_url->addQuery(U_CONSTANT_TO_PARAM("traffic"), U_STRING_TO_PARAM(buffer));
      }
}

void UNoCatPlugIn::checkPeerInfo(UStringRep* key, void* value)
{
   U_TRACE(0, "UNoCatPlugIn::checkPeerInfo(%.*S,%p)", U_STRING_TO_TRACE(*key), value)

   UModNoCatPeer* peer = (UModNoCatPeer*) value;

   if (peer->status == UModNoCatPeer::PEER_ACCEPT)
      {
      time_t t         = peer->expire - u_now->tv_sec;
      uint64_t traffic = (peer->ltraffic > peer->traffic ? (peer->ltraffic - peer->traffic) : 0);

      U_SRV_LOG("Checking peer %.*s for info, remain: %ld secs %llu bytes", U_STRING_TO_TRACE(peer->ip), U_max(0,t), traffic);

      if (login_timeout &&
          (traffic < 1024 || t <= U_NO_MORE_TIME))
         {
         deny(peer, false, false);

         return;
         }

      UString ifname = USocketExt::getNetworkInterfaceName(peer->ip);

      U_INTERNAL_DUMP("ifname = %.*S peer->ifname = %.*S", U_STRING_TO_TRACE(ifname), U_STRING_TO_TRACE(peer->ifname))

      if (ifname.empty())
         {
         U_SRV_LOG("Peer %.*s not present in ARP cache, I assume he is disconnected...", U_STRING_TO_TRACE(peer->ip));

         deny(peer, false, true);

         return;
         }

      uint32_t ifindex = pthis->vInternalDevice.find(ifname);

      U_INTERNAL_DUMP("ifindex = %u peer->ifindex = %u", ifindex, peer->ifindex)

      U_INTERNAL_ASSERT(ifindex <= num_radio)

      if (ifindex != U_NOT_FOUND)
         {
         ++nfds;

         vaddr[ifindex]->push(peer);
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

void UNoCatPlugIn::checkPeersForInfo()
{
   U_TRACE(0, "UNoCatPlugIn::checkPeersForInfo()")

   uint32_t i;
   UModNoCatPeer* peer;
   time_t last_request = u_now->tv_sec - last_request_check;

   U_INTERNAL_DUMP("last_request = %ld", last_request)

   // NB: check for pending arping...

   if (isPingAsyncPending())
      {
      U_SRV_LOG("Pending arping in process (%u), waiting for completion...", nfds);

      addrmask = UPing::checkForPingAsyncCompletion(nfds);

      if (addrmask || last_request < 15) goto result; // NB: check for something wrong (must complete within 15 secs)...
      }

   if (last_request >= 15) // NB: protection from DoS...
      {
      last_request_check = u_now->tv_sec;

      U_SRV_LOG("Checking peers for info");

      for (nfds = i = 0; i < num_radio; ++i) vaddr[i]->clear();

      peers->callForAllEntry(checkPeerInfo);

#  ifdef HAVE_NETPACKET_PACKET_H
      addrmask = (nfds ? (arping ? UPing::arping(sockp, vaddr, num_radio, true, unatexit, vInternalDevice) // parallel ARPING (async)...
                                 : UPing::arpcache(     vaddr, num_radio))                                 // ARP cache...
                       : 0);
#  else
      addrmask = (nfds ? (UPing::arpcache(vaddr, num_radio)) : 0);
#  endif
      }

result:
   if (addrmask)
      {
      U_INTERNAL_ASSERT_MAJOR(nfds,0)

      for (i = 0; i < nfds; ++i)
         {
         peer = getPeer(i);

         U_INTERNAL_DUMP("peer = %p", peer)

         if (peer)
            {
            if (FD_ISSET(i, addrmask)) addPeerInfo(peer, 0);
            else
               {
               U_SRV_LOG("Peer %.*s don't return ARP reply, I assume he is disconnected...", U_STRING_TO_TRACE(peer->ip));

               deny(peer, false, true);
               }
            }
         }

      nfds     = 0;
      addrmask = 0;
      }

   flag_check_peers_for_info = false;
}

void UNoCatPlugIn::notifyAuthOfUsersInfo()
{
   U_TRACE(0, "UNoCatPlugIn::notifyAuthOfUsersInfo()")

   Url* info_url = pthis->vinfo_url[index_AUTH];

   U_INTERNAL_DUMP("index_AUTH = %u info_url = %p", index_AUTH, info_url)

   // NB: if there are some data to transmit we need redirect...

   if (info_url->isQuery())
      {
      UHTTP::setHTTPRedirectResponse(false, UString::getStringNull(), U_URL_TO_PARAM(*info_url));

      *info_url = *(pthis->vlogout_url[index_AUTH]);
      }
   else
      {
      U_http_is_connection_close = U_YES;
      u_http_info.nResponseCode  = (isPingAsyncPending() ? HTTP_NO_CONTENT : HTTP_NOT_MODIFIED);

      UHTTP::setHTTPResponse(0, 0);
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

const char* UNoCatPlugIn::getIPAddress(const char* ptr, uint32_t len)
{
   U_TRACE(0, "UNoCatPlugIn::getIPAddress(%.*S,%u)", len, ptr, len)

   char c;
   uint32_t i;

   for (i = 0; i < len; ++i)
      {
      c = ptr[i];

      if (u_isdigit(c) == 0 && c != '.' && c != ':') break;

      pcStrAddress[i] = c;
      }

   pcStrAddress[i] = '\0';

   U_RETURN(pcStrAddress);
}

void UNoCatPlugIn::setHTTPResponse(const UString& content)
{
   U_TRACE(0, "UNoCatPlugIn::setHTTPResponse(%.*S)", U_STRING_TO_TRACE(content))

   u_http_info.clength         = content.size();
   u_http_info.nResponseCode   = HTTP_OK;
   *UClientImage_Base::wbuffer = content;

   UHTTP::setHTTPCgiResponse(false, UHTTP::isCompressable(), false);
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
   U_INTERNAL_ASSERT_EQUALS(str_AUTH_SERVICE_URL,0)
   U_INTERNAL_ASSERT_EQUALS(str_LOGOUT_URL,0)
   U_INTERNAL_ASSERT_EQUALS(str_LOGIN_TIMEOUT,0)
   U_INTERNAL_ASSERT_EQUALS(str_FW_CMD,0)
   U_INTERNAL_ASSERT_EQUALS(str_DECRYPT_CMD,0)
   U_INTERNAL_ASSERT_EQUALS(str_DECRYPT_KEY,0)
   U_INTERNAL_ASSERT_EQUALS(str_CHECK_BY_ARPING,0)
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
   U_INTERNAL_ASSERT_EQUALS(str_anonymous,0)
   U_INTERNAL_ASSERT_EQUALS(str_Traffic,0)
   U_INTERNAL_ASSERT_EQUALS(str_GATEWAY_PORT,0)
   U_INTERNAL_ASSERT_EQUALS(str_FW_ENV,0)
   U_INTERNAL_ASSERT_EQUALS(str_IPHONE_SUCCESS,0)
   U_INTERNAL_ASSERT_EQUALS(str_CHECK_EXPIRE_INTERVAL,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("RouteOnly") },
      { U_STRINGREP_FROM_CONSTANT("DNSAddr") },
      { U_STRINGREP_FROM_CONSTANT("IncludePorts") },
      { U_STRINGREP_FROM_CONSTANT("ExcludePorts") },
      { U_STRINGREP_FROM_CONSTANT("AllowedWebHosts") },
      { U_STRINGREP_FROM_CONSTANT("ExternalDevice") },
      { U_STRINGREP_FROM_CONSTANT("InternalDevice") },
      { U_STRINGREP_FROM_CONSTANT("INTERNAL_DEVICE_LABEL") },
      { U_STRINGREP_FROM_CONSTANT("LocalNetwork") },
      { U_STRINGREP_FROM_CONSTANT("AuthServiceAddr") },
      { U_STRINGREP_FROM_CONSTANT("LOGOUT_URL") },
      { U_STRINGREP_FROM_CONSTANT("LOGIN_TIMEOUT") },
      { U_STRINGREP_FROM_CONSTANT("FW_CMD") },
      { U_STRINGREP_FROM_CONSTANT("DECRYPT_CMD") },
      { U_STRINGREP_FROM_CONSTANT("DECRYPT_KEY") },
      { U_STRINGREP_FROM_CONSTANT("CHECK_BY_ARPING") },
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
      { U_STRINGREP_FROM_CONSTANT("anonymous") },
      { U_STRINGREP_FROM_CONSTANT("Traffic") },
      { U_STRINGREP_FROM_CONSTANT("GatewayPort") },
      { U_STRINGREP_FROM_CONSTANT("CHECK_EXPIRE_INTERVAL") },
      { U_STRINGREP_FROM_CONSTANT("FW_ENV") },
      { U_STRINGREP_FROM_CONSTANT("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">\n"
                                  "<HTML>\n"
                                  "<HEAD>\n"
                                  "   <TITLE>Success</TITLE>\n"
                                  "</HEAD>\n"
                                  "<BODY>\n"
                                  "Success\n"
                                  "</BODY>\n"
                                  "</HTML>") }
   };

   U_NEW_ULIB_OBJECT(str_ROUTE_ONLY,            U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_DNS_ADDR,              U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_INCLUDE_PORTS,         U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_EXCLUDE_PORTS,         U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_ALLOWED_WEB_HOSTS,     U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_EXTERNAL_DEVICE,       U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_INTERNAL_DEVICE,       U_STRING_FROM_STRINGREP_STORAGE(6));
   U_NEW_ULIB_OBJECT(str_INTERNAL_DEVICE_LABEL, U_STRING_FROM_STRINGREP_STORAGE(7));
   U_NEW_ULIB_OBJECT(str_LOCAL_NETWORK,         U_STRING_FROM_STRINGREP_STORAGE(8));
   U_NEW_ULIB_OBJECT(str_AUTH_SERVICE_URL,      U_STRING_FROM_STRINGREP_STORAGE(9));
   U_NEW_ULIB_OBJECT(str_LOGOUT_URL,            U_STRING_FROM_STRINGREP_STORAGE(10));
   U_NEW_ULIB_OBJECT(str_LOGIN_TIMEOUT,         U_STRING_FROM_STRINGREP_STORAGE(11));
   U_NEW_ULIB_OBJECT(str_FW_CMD,                U_STRING_FROM_STRINGREP_STORAGE(12));
   U_NEW_ULIB_OBJECT(str_DECRYPT_CMD,           U_STRING_FROM_STRINGREP_STORAGE(13));
   U_NEW_ULIB_OBJECT(str_DECRYPT_KEY,           U_STRING_FROM_STRINGREP_STORAGE(14));
   U_NEW_ULIB_OBJECT(str_CHECK_BY_ARPING,       U_STRING_FROM_STRINGREP_STORAGE(15));
   U_NEW_ULIB_OBJECT(str_Action,                U_STRING_FROM_STRINGREP_STORAGE(16));
   U_NEW_ULIB_OBJECT(str_Permit,                U_STRING_FROM_STRINGREP_STORAGE(17));
   U_NEW_ULIB_OBJECT(str_Deny,                  U_STRING_FROM_STRINGREP_STORAGE(18));
   U_NEW_ULIB_OBJECT(str_Mode,                  U_STRING_FROM_STRINGREP_STORAGE(19));
   U_NEW_ULIB_OBJECT(str_Redirect,              U_STRING_FROM_STRINGREP_STORAGE(20));
   U_NEW_ULIB_OBJECT(str_renew,                 U_STRING_FROM_STRINGREP_STORAGE(21));
   U_NEW_ULIB_OBJECT(str_Mac,                   U_STRING_FROM_STRINGREP_STORAGE(22));
   U_NEW_ULIB_OBJECT(str_Timeout,               U_STRING_FROM_STRINGREP_STORAGE(23));
   U_NEW_ULIB_OBJECT(str_Token,                 U_STRING_FROM_STRINGREP_STORAGE(24));
   U_NEW_ULIB_OBJECT(str_User,                  U_STRING_FROM_STRINGREP_STORAGE(25));
   U_NEW_ULIB_OBJECT(str_anonymous,             U_STRING_FROM_STRINGREP_STORAGE(26));
   U_NEW_ULIB_OBJECT(str_Traffic,               U_STRING_FROM_STRINGREP_STORAGE(27));
   U_NEW_ULIB_OBJECT(str_GATEWAY_PORT,          U_STRING_FROM_STRINGREP_STORAGE(28));
   U_NEW_ULIB_OBJECT(str_CHECK_EXPIRE_INTERVAL, U_STRING_FROM_STRINGREP_STORAGE(29));
   U_NEW_ULIB_OBJECT(str_FW_ENV,                U_STRING_FROM_STRINGREP_STORAGE(30));
   U_NEW_ULIB_OBJECT(str_IPHONE_SUCCESS,        U_STRING_FROM_STRINGREP_STORAGE(31));
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
   //
   // LOGOUT_URL            URL to redirect user after logout
   // PUBLIC_ADDRESS        address to contact access point from portal
   // INTERNAL_DEVICE_LABEL radio tag to be used from portal
   //
   // DECRYPT_CMD           PGP command stuff
   // DECRYPT_KEY           DES3 password stuff
   //
   // LOGIN_TIMEOUT         Number of seconds after a client last login/renewal to terminate their connection
   // CHECK_BY_ARPING       metodo aggiuntivo per verificare la presenza di un peer nella tabella ARP (yes -> abilitato)
   // CHECK_EXPIRE_INTERVAL Number of seconds to check if some client has terminate their connection
   // -----------------------------------------------------------------------------------------------------------------------------------

   if (cfg.loadTable())
      {
      int port = 0;
      long check_expire;
      UString pathconf, fw_env, command(100U), intdevlbl, sport, tmp;

      fw_env = cfg[*str_FW_ENV];

      if (fw_env.empty()) goto err;

      fw_cmd        = cfg[*str_FW_CMD];
      intdevlbl     = cfg[*str_INTERNAL_DEVICE_LABEL];
      decrypt_cmd   = cfg[*str_DECRYPT_CMD];
      decrypt_key   = cfg[*str_DECRYPT_KEY];

      arping        = cfg.readBoolean(*str_CHECK_BY_ARPING);
      check_expire  = cfg.readLong(*str_CHECK_EXPIRE_INTERVAL);
      login_timeout = cfg.readLong(*str_LOGIN_TIMEOUT);

      tmp           = cfg[*str_LOGOUT_URL];

      (void) vauth_logout.split(U_STRING_TO_PARAM(tmp));

      U_INTERNAL_DUMP("check_expire = %ld login_timeout = %ld", check_expire, login_timeout)

      if (check_expire) UEventTime::setSecond(check_expire);

      if (login_timeout > U_NOCAT_MAX_TIMEOUT) login_timeout = U_NOCAT_MAX_TIMEOUT; // check for safe timeout...

      tmp = UStringExt::getEnvironmentVar(*str_INTERNAL_DEVICE, &fw_env);

      if (tmp.empty() == false) intdev = tmp;

      tmp = UStringExt::getEnvironmentVar(*str_EXTERNAL_DEVICE, &fw_env);

      if (tmp.empty() == false) extdev = tmp;

      tmp = UStringExt::getEnvironmentVar(*str_LOCAL_NETWORK, &fw_env);

      if (tmp.empty() == false) localnet = tmp;

      tmp = UStringExt::getEnvironmentVar(*str_AUTH_SERVICE_URL, &fw_env);

      if (tmp.empty() == false) auth_login = tmp;

      tmp = UStringExt::getEnvironmentVar(*str_GATEWAY_PORT, &fw_env);

      if (tmp.empty() == false) port = tmp.strtol();

      pathconf = UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("FW_CONF"), &fw_env);

      if (pathconf.empty() == false)
         {
         UString data      = UFile::contentOf(pathconf);
         const char* end   = data.end();
         const char* start = data.data();

         cfg.clear();

         if (UFileConfig::loadProperties(cfg.table, start, end) == false) goto err;

         intdev     = cfg[*str_INTERNAL_DEVICE];
         extdev     = cfg[*str_EXTERNAL_DEVICE];
         localnet   = cfg[*str_LOCAL_NETWORK];
         auth_login = cfg[*str_AUTH_SERVICE_URL];
         port       = cfg.readLong(*str_GATEWAY_PORT);
         }

      command.snprintf("/bin/sh %.*s request", U_STRING_TO_TRACE(fw_cmd));

      cmd.set(command, (char**)0);
      cmd.setEnvironment(&fw_env);

      (void) vauth_login.split(U_STRING_TO_PARAM(auth_login));

      U_INTERNAL_ASSERT_EQUALS(vauth_login.size(), vauth_logout.size())

      if (extdev.empty())
         {
         extdev = UServer_Base::getNetworkDevice(0);

         if (extdev.empty()) U_ERROR("No ExternalDevice detected!");

         U_SRV_LOG("Autodetected ExternalDevice %S", extdev.data());
         }

      if (intdev.empty())
         {
         intdev = UServer_Base::getNetworkDevice(extdev.data());

         if (intdev.empty()) U_ERROR("No InternalDevice detected!");

         U_SRV_LOG("Autodetected InternalDevice %S", intdev.data());
         }

      (void) vInternalDeviceLabel.split(U_STRING_TO_PARAM(intdevlbl));
      num_radio = vInternalDevice.split(U_STRING_TO_PARAM(intdev));

      U_INTERNAL_DUMP("num_radio = %u", num_radio)

      if (localnet.empty())
         {
         localnet = UServer_Base::getNetworkAddress(intdev.data());

         if (localnet.empty()) U_ERROR("No LocalNetwork detected!");

         U_SRV_LOG("Autodetected LocalNetwork %S", localnet.data());
         }

      (void) vLocalNetwork.split(U_STRING_TO_PARAM(localnet));
      (void) UIPAllow::parseMask(localnet, vLocalNetworkMask);

      UServer_Base::port = (port ? port : 5280);

      sport = UStringExt::numberToString(UServer_Base::port);

      if (UServer_Base::port != 80) gateway = ':' + sport;

      U_INTERNAL_DUMP("gateway = %.*S", U_STRING_TO_TRACE(gateway))
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);

err:
   U_SRV_LOG("configuration of plugin FAILED");

   U_RETURN(U_PLUGIN_HANDLER_ERROR);
}

int UNoCatPlugIn::handlerInit()
{
   U_TRACE(0, "UNoCatPlugIn::handlerInit()")

   if (fw_cmd.empty() == false)
      {
      Url* url;
      UString auth_ip;
      UIPAddress addr;

      // NB: get IP address of AUTH hosts...

      for (uint32_t i = 0, n = vauth_login.size(); i < n; ++i)
         {
         url = U_NEW(Url(vauth_login[i]));

         vauth_service_url.push(url);

         auth_ip = url->getHost();

         if (addr.setHostName(auth_ip, UClientImage_Base::bIPv6) == false)
            {
            U_SRV_LOG("unknown AUTH host %.*S", U_STRING_TO_TRACE(auth_ip));
            }
         else
            {
            (void) auth_ip.replace(addr.getAddressString());

            U_SRV_LOG("AUTH host registered: %.*s", U_STRING_TO_TRACE(auth_ip));

            vauth_ip.push(auth_ip);
            }

         url = U_NEW(Url(vauth_logout[i]));

         vlogout_url.push(url);

         url = U_NEW(Url(vauth_logout[i]));

         vinfo_url.push(url);
         }

      U_INTERNAL_ASSERT_EQUALS(vauth_ip.size(), vauth_login.size())

      // crypto cmd

      if (decrypt_cmd.empty() == false) pgp.set(decrypt_cmd, (char**)0);

      U_INTERNAL_ASSERT(decrypt_key.isNullTerminated())

      if (decrypt_key.empty() == false) UDES3::setPassword(decrypt_key.data());

      // initialize the firewall: direct all port 80 traffic to us...

      fd_stderr = UServices::getDevNull();

      cmd.setArgument(3, "initialize");

      (void) cmd.executeAndWait(0, -1, fd_stderr);

      UServer_Base::logCommandMsgError(cmd.getCommand());

      if (UEventTime::notZero() || login_timeout)
         {
         UTimer::init(true); // async...

         if (UEventTime::notZero()) UTimer::insert(this, true);
         }

      peers = U_NEW(UHashMap<UModNoCatPeer*>);

      peers->allocate();

      access_point   = USocketExt::getNodeName();
      status_content = U_NEW(UString(U_CAPACITY));

      gateway.insert(0, UServer_Base::getIPAddress());

      U_INTERNAL_DUMP("gateway = %.*S access_point = %.*S", U_STRING_TO_TRACE(gateway), U_STRING_TO_TRACE(access_point))

      // users traffic

      ipt = U_NEW(UIptAccount(UClientImage_Base::bIPv6));

      // NB: needed because all instance try to close the log... (inherits from its parent)

      unatexit = (UServer_Base::isLog() ? &ULog::close : 0);

      // manage internal device...

      sockp = U_MALLOC_VECTOR(num_radio, UPing);
      vaddr = U_MALLOC_VECTOR(num_radio, UVector<UIPAddress*>);

      for (uint32_t i = 0; i < num_radio; ++i)
         {
         vaddr[i] = U_NEW(UVector<UIPAddress*>);
         sockp[i] = U_NEW(UPing(3000, UClientImage_Base::bIPv6));

         if (arping)
            {
            sockp[i]->setLocal(UServer_Base::socket->localIPAddress());

#        ifdef HAVE_NETPACKET_PACKET_H
            U_INTERNAL_ASSERT(vInternalDevice[i].isNullTerminated())

            sockp[i]->initArpPing(vInternalDevice[i].data());
#        endif
            }
         }

      pthis = this;

      U_SRV_LOG("initialization of plugin success");

      goto end;
      }

   U_SRV_LOG("initialization of plugin FAILED");

end:
   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int UNoCatPlugIn::handlerRequest()
{
   U_TRACE(0, "UNoCatPlugIn::handlerRequest()")

   if (UHTTP::isHTTPRequestNotFound() &&
       fw_cmd.empty() == false)
      {
      Url url;
      bool refresh = false;
      UModNoCatPeer* peer = 0;
      UString host(U_HTTP_HOST_TO_PARAM), buffer(U_CAPACITY), ip_client = UHTTP::getRemoteIP();

      U_INTERNAL_DUMP("host = %.*S ip_client = %.*S", U_STRING_TO_TRACE(host), U_STRING_TO_TRACE(ip_client))

      // NB: check for request from AUTHs

      index_AUTH = vauth_ip.contains(ip_client);

      U_INTERNAL_DUMP("index_AUTH = %u", index_AUTH)

      if (index_AUTH != U_NOT_FOUND)
         {
         U_SRV_LOG("request from AUTH: %.*s", U_HTTP_URI_TO_TRACE);

         if (U_HTTP_URI_STRNEQ("/check"))
            {
            // NB: request from AUTH to check and notify logout e/o disconnected users and info...

            checkPeersForInfo();

            notifyAuthOfUsersInfo();

            goto end;
            }

         if (U_HTTP_URI_STRNEQ("/status"))
            {
            if (U_HTTP_QUERY_STRNEQ("ip="))
               {
               // NB: request from AUTH to get status user

               uint32_t len    = u_http_info.query_len - U_CONSTANT_SIZE("ip=");
               const char* ptr = u_http_info.query     + U_CONSTANT_SIZE("ip=");

               const char* peer_ip = getIPAddress(ptr, len);

               U_SRV_LOG("request from AUTH to get status for user with ip address: %s", peer_ip);

               peer = (*peers)[peer_ip];

               U_INTERNAL_DUMP("peer = %p", peer)

               if (peer == 0)
                  {
                  UHTTP::setHTTPBadRequest();

                  goto end;
                  }
               }

            // status

            setStatusContent(peer); // NB: peer == 0 -> request from AUTH to get status access point...

            goto end;
            }

         if (U_HTTP_URI_STRNEQ("/logout") &&
             u_http_info.query_len)
            {
            // NB: request from AUTH to logout user (ip=192.168.301.223&mac=00:e0:4c:d4:63:f5)

            if (checkSignedData(U_HTTP_QUERY_TO_PARAM) == false)
               {
               U_SRV_LOG("Tampered request of logout user");

               goto set_redirection_url;
               }

            uint32_t len    = output.size() - U_CONSTANT_SIZE("ip=");
            const char* ptr = output.data() + U_CONSTANT_SIZE("ip=");

            const char* peer_ip = getIPAddress(ptr, len);

            U_SRV_LOG("request from AUTH to logout user with ip address: %s", peer_ip);

            peer = (*peers)[peer_ip];

            U_INTERNAL_DUMP("peer = %p", peer)

            if (peer == 0 ||
                peer->status != UModNoCatPeer::PEER_ACCEPT)
               {
               uint32_t pos = output.find_first_of('&', 3);

               if (pos != U_NOT_FOUND &&
                   U_STRNEQ(output.c_pointer(pos+1), "mac="))
                  {
                  UString mac = output.substr(pos + 5, U_CONSTANT_SIZE("00:00:00:00:00:00"));

                  U_SRV_LOG("request from AUTH to logout user with MAC: %.*s", U_STRING_TO_TRACE(mac));

                  peer = getPeerFromMAC(mac);
                  }
               }

            U_INTERNAL_DUMP("peer = %p", peer)

            if (peer == 0 ||
                peer->status != UModNoCatPeer::PEER_ACCEPT)
               {
               UHTTP::setHTTPBadRequest();
               }
            else
               {
               deny(peer, false, false);

               notifyAuthOfUsersInfo();
               }

            goto end;
            }
         }

      index_AUTH = UIPAllow::contains(ip_client, vLocalNetworkMask);

      U_INTERNAL_DUMP("index_AUTH = %u", index_AUTH)

   // U_INTERNAL_ASSERT_DIFFERS(index_AUTH, U_NOT_FOUND)

      if (index_AUTH >= vauth_ip.size()) index_AUTH = 0;

      url  = *(vauth_service_url[index_AUTH]);
      peer = (*peers)[ip_client];

      U_INTERNAL_DUMP("peer = %p", peer)

      if (U_HTTP_URI_STRNEQ("/cpe"))
         {
         (void) buffer.assign(U_CONSTANT_TO_PARAM("http://www.google.com"));

         url.setPath(U_CONSTANT_TO_PARAM("/cpe"));
         url.setService(U_CONSTANT_TO_PARAM("https"));

         goto set_redirect_to_AUTH;
         }

      if (U_HTTP_URI_STRNEQ("/test"))
         {
         (void) buffer.assign(U_CONSTANT_TO_PARAM("http://www.google.com"));

         goto set_redirect_to_AUTH;
         }

      if (U_HTTP_URI_STRNEQ( "/ticket") &&
          U_HTTP_QUERY_STRNEQ("ticket="))
         {
         // user with a ticket

#     ifdef DEBUG
         if (U_http_version == '1') U_INTERNAL_ASSERT_EQUALS(host, gateway)
#     endif

         uint32_t len    = u_http_info.query_len - U_CONSTANT_SIZE("ticket=");
         const char* ptr = u_http_info.query     + U_CONSTANT_SIZE("ticket=");

         if (checkSignedData(ptr, len) == false)
            {
            U_SRV_LOG("Invalid ticket from peer %.*S", U_STRING_TO_TRACE(ip_client));

            goto set_redirection_url;
            }

         if (UServer_Base::isLog())
            {
            UString printable(output.size() * 4);

            UEscape::encode(output, printable, false);

            UServer_Base::log->log("%sauth message: %.*s\n", UServer_Base::mod_name, U_STRING_TO_TRACE(printable));
            }

         if (peer == 0 ||
             checkAuthMessage(peer) == false)
            {
            U_SRV_LOG("Tampered ticket from peer %.*S", U_STRING_TO_TRACE(ip_client));

            goto set_redirection_url;
            }

         U_INTERNAL_DUMP("mode = %.*S", U_STRING_TO_TRACE(mode))

         // OK: go to the destination (with Location: ...)

         goto redirect;
         }

      // NB: check for strange initial WiFi behaviour of the iPhone...

      if (U_HTTP_URI_STRNEQ("/library/test/success.html")  &&
          host.equal(U_CONSTANT_TO_PARAM("www.apple.com")) &&
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

         U_SRV_LOG("detected strange initial WiFi request from peer %.*S", U_STRING_TO_TRACE(ip_client));

         setHTTPResponse(*str_IPHONE_SUCCESS);

         goto end;
         }

      if (U_http_version == '1' &&
          host           == gateway)
         {
         U_SRV_LOG("Missing ticket from peer %.*S", U_STRING_TO_TRACE(ip_client));
         }

set_redirection_url:
      buffer.snprintf("http://%.*s%.*s", U_STRING_TO_TRACE(host), U_HTTP_URI_TO_TRACE);

set_redirect_to_AUTH:
      if (peer == 0) peer = creatNewPeer(ip_client);

      setRedirectLocation(peer, buffer, url);

      refresh = true;

redirect:
      UHTTP::setHTTPRedirectResponse(refresh, UString::getStringNull(), U_STRING_TO_PARAM(location));

end:
      UHTTP::setHTTPRequestProcessed();

      // NB: maybe we have a check delayed because of http request processing...

      if (flag_check_peers_for_info) checkPeersForInfo();
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UModNoCatPeer::dump(bool _reset) const
{
   *UObjectIO::os << "status                " << status         <<  '\n'
                  << "expire                " << expire         <<  '\n'
                  << "logout                " << logout         <<  '\n'
                  << "ifindex               " << ifindex        <<  '\n'
                  << "traffic               " << traffic        <<  '\n'
                  << "ctraffic              " << ctraffic       <<  '\n'
                  << "ltraffic              " << ltraffic       <<  '\n'
                  << "connected             " << connected      <<  '\n'
                  << "index_AUTH            " << index_AUTH     <<  '\n'
                  << "ip        (UString    " << (void*)&ip     << ")\n"
                  << "mac       (UString    " << (void*)&mac    << ")\n"
                  << "token     (UString    " << (void*)&token  << ")\n"
                  << "label     (UString    " << (void*)&label  << ")\n"
                  << "ifname    (UString    " << (void*)&ifname << ")\n"
                  << "cmd       (UCommand   " << (void*)&cmd    << ')';

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

const char* UNoCatPlugIn::dump(bool _reset) const
{
   *UObjectIO::os << "nfds                                           " << nfds                         <<  '\n'
                  << "vaddr                                          " << (void*)vaddr                 <<  '\n' 
                  << "sockp                                          " << (void*)sockp                 <<  '\n'
                  << "addrmask                                       " << (void*)addrmask              <<  '\n'
                  << "unatexit                                       " << (void*)unatexit              <<  '\n'
                  << "fd_stderr                                      " << fd_stderr                    <<  '\n'
                  << "num_radio                                      " << num_radio                    <<  '\n'
                  << "index_AUTH                                     " << index_AUTH                   <<  '\n'
                  << "login_timeout                                  " << login_timeout                <<  '\n'
                  << "total_connections                              " << total_connections            <<  '\n'
                  << "last_request_check                             " << last_request_check           <<  '\n'
                  << "mode                 (UString                  " << (void*)&mode                 << ")\n"
                  << "input                (UString                  " << (void*)&input                << ")\n"
                  << "output               (UString                  " << (void*)&output               << ")\n"
                  << "extdev               (UString                  " << (void*)&extdev               << ")\n"
                  << "fw_cmd               (UString                  " << (void*)&fw_cmd               << ")\n"
                  << "gateway              (UString                  " << (void*)&gateway              << ")\n"
                  << "location             (UString                  " << (void*)&location             << ")\n"
                  << "dns_addr             (UString                  " << (void*)&dns_addr             << ")\n"
                  << "route_only           (UString                  " << (void*)&route_only           << ")\n"
                  << "auth_login           (UString                  " << (void*)&auth_login           << ")\n"
                  << "decrypt_cmd          (UString                  " << (void*)&decrypt_cmd          << ")\n"
                  << "decrypt_key          (UString                  " << (void*)&decrypt_key          << ")\n"
                  << "access_point         (UString                  " << (void*)&access_point         << ")\n"
                  << "include_ports        (UString                  " << (void*)&include_ports        << ")\n"
                  << "exclude_ports        (UString                  " << (void*)&exclude_ports        << ")\n"
                  << "status_content       (UString                  " << (void*)status_content        << ")\n"
                  << "allowed_web_hosts    (UString                  " << (void*)&allowed_web_hosts    << ")\n"
                  << "cmd                  (UCommand                 " << (void*)&cmd                  << ")\n"
                  << "pgp                  (UCommand                 " << (void*)&pgp                  << ")\n"
                  << "ipt                  (UIptAccount              " << (void*)ipt                   << ")\n"
                  << "vinfo_url            (UVector<Url*>            " << (void*)&vinfo_url            << ")\n"
                  << "vlogout_url          (UVector<Url*>            " << (void*)&vlogout_url          << ")\n"
                  << "vauth_service_url    (UVector<Url*>            " << (void*)&vauth_service_url    << ")\n"
                  << "vauth_ip             (UVector<UString>         " << (void*)&vauth_ip             << ")\n"
                  << "vauth_login          (UVector<UString>         " << (void*)&vauth_login          << ")\n"
                  << "vauth_logout         (UVector<UString>         " << (void*)&vauth_logout         << ")\n"
                  << "vLocalNetwork        (UVector<UString>         " << (void*)&vLocalNetwork        << ")\n"
                  << "vInternalDevice      (UVector<UString>         " << (void*)&vInternalDevice      << ")\n"
                  << "vInternalDeviceLabel (UVector<UString>         " << (void*)&vInternalDeviceLabel << ")\n"
                  << "peers                (UHashMap<UModNoCatPeer*> " << (void*)peers                 << ')';

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
