// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_nocat.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_NOCAT_H
#define U_MOD_NOCAT_H 1

#include <ulib/url.h>
#include <ulib/timer.h>
#include <ulib/command.h>
#include <ulib/net/ping.h>
#include <ulib/container/vector.h>
#include <ulib/container/hash_map.h>
#include <ulib/net/server/server_plugin.h>

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
3) handlerREAD:
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

class UIptAccount;
class UNoCatPlugIn;

class UModNoCatPeer : public UIPAddress, UEventTime {
public:

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   enum Status { PEER_DENY, PEER_ACCEPT };

   // COSTRUTTORI

            UModNoCatPeer(const UString& peer_ip);
   virtual ~UModNoCatPeer()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UModNoCatPeer)
      }

   // VARIE

   void setCommand(const UString& script)
      {
      U_TRACE(0, "UModNoCatPeer::setCommand(%.*S)", U_STRING_TO_TRACE(script))

      // NB: request(arp|permit|deny|clear|reset|initialize) mac ip class(Owner|Member|Public) rulenum]

      UString command(100U);

      command.snprintf("/bin/sh %.*s request %.*s %.*s Member %u", U_STRING_TO_TRACE(script), U_STRING_TO_TRACE(mac), U_STRING_TO_TRACE(ip), rulenum);

      cmd.set(command, (char**)0);
      }

   // define method VIRTUAL of class UEventTime

   virtual int handlerTime();

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   int status;
   uint64_t traffic, ltraffic;
   time_t connected, expire, logout, ctime;
   uint32_t ifindex, ctraffic, rulenum, index_AUTH;
   UString ip, mac, token, user, ifname, label;
   UCommand cmd;

private:
   UModNoCatPeer(const UModNoCatPeer&) : UIPAddress(), UEventTime() {}
   UModNoCatPeer& operator=(const UModNoCatPeer&)                   { return *this; }

   friend class UNoCatPlugIn;
};

// override the default...
template <> inline void u_destroy(UIPAddress** ptr, uint32_t n) { U_TRACE(0,"u_destroy<UIPAddress*>(%p,%u)", ptr, n) }

class U_EXPORT UNoCatPlugIn : public UServerPlugIn, UEventTime {
public:

   static const UString* str_ROUTE_ONLY;
   static const UString* str_DNS_ADDR;
   static const UString* str_INCLUDE_PORTS;
   static const UString* str_EXCLUDE_PORTS;
   static const UString* str_ALLOWED_WEB_HOSTS;
   static const UString* str_EXTERNAL_DEVICE;
   static const UString* str_INTERNAL_DEVICE;
   static const UString* str_INTERNAL_DEVICE_LABEL;
   static const UString* str_LOCAL_NETWORK;
   static const UString* str_AUTH_SERVICE_URL;
   static const UString* str_LOGOUT_URL;
   static const UString* str_LOGIN_TIMEOUT;
   static const UString* str_FW_CMD;
   static const UString* str_DECRYPT_CMD;
   static const UString* str_DECRYPT_KEY;
   static const UString* str_CHECK_BY_ARPING;
   static const UString* str_Action;
   static const UString* str_Permit;
   static const UString* str_Deny;
   static const UString* str_Mode;
   static const UString* str_Redirect;
   static const UString* str_renew;
   static const UString* str_Mac;
   static const UString* str_Timeout;
   static const UString* str_Token;
   static const UString* str_User;
   static const UString* str_anonymous;
   static const UString* str_Traffic;
   static const UString* str_GATEWAY_PORT;
   static const UString* str_FW_ENV;
   static const UString* str_IPHONE_SUCCESS;
   static const UString* str_CHECK_EXPIRE_INTERVAL;

   static void str_allocate();

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

            UNoCatPlugIn();
   virtual ~UNoCatPlugIn();

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerInit();

   // Connection-wide hooks

   virtual int handlerRequest();

   // define method VIRTUAL of class UEventTime

   virtual int handlerTime();

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UCommand cmd, pgp;
   UVector<UIPAllow*> vLocalNetworkMask;
   UVector<Url*> vauth_service_url, vlogout_url, vinfo_url;
   UVector<UString> vInternalDevice, vInternalDeviceLabel, vLocalNetwork, vauth_login, vauth_logout, vauth_ip;
   UString input, output, location, fw_cmd, decrypt_cmd, decrypt_key, mode, gateway, access_point, extdev, route_only,
           dns_addr, include_ports, exclude_ports, allowed_web_hosts, intdev, localnet, auth_login;

   static vPF unatexit;
   static int fd_stderr;
   static UPing** sockp;
   static fd_set* addrmask;
   static UIptAccount* ipt;
   static UNoCatPlugIn* pthis;
   static UString* status_content;
   static time_t last_request_check;
   static UVector<UIPAddress*>** vaddr;
   static UHashMap<UModNoCatPeer*>* peers;
   static bool arping, flag_check_peers_for_info;

   static char pcStrAddress[INET6_ADDRSTRLEN];
   static uint32_t total_connections, login_timeout, nfds, num_radio, index_AUTH;

   // VARIE

   void getTraffic();
   void checkPeersForInfo();
   void setStatusContent(UModNoCatPeer* peer);
   bool checkAuthMessage(UModNoCatPeer* peer);
   bool checkSignedData(const char* ptr, uint32_t len);
   void addPeerInfo(UModNoCatPeer* peer, time_t logout);
   void setRedirectLocation(UModNoCatPeer* peer, const UString& redirect, const Url& auth);

   static UModNoCatPeer* getPeer(uint32_t i) __pure;
          UModNoCatPeer* getPeerFromMAC(const UString& mac);
          UModNoCatPeer* creatNewPeer(const UString& peer_ip);

   static void notifyAuthOfUsersInfo();
   static void setHTTPResponse(const UString& content);
   static void getPeerStatus(UStringRep* key, void* value);
   static void checkPeerInfo(UStringRep* key, void* value);
   static void executeCommand(UModNoCatPeer* peer, int type);

   static void permit(UModNoCatPeer* peer, time_t timeout);
   static void   deny(UModNoCatPeer* peer, bool alarm, bool disconnected);

   static bool isPingAsyncPending()
      {
      U_TRACE(0, "UNoCatPlugIn::isPingAsyncPending()")

      U_INTERNAL_DUMP("nfds = %u addrmask = %p", nfds, addrmask)

      bool result = (nfds && addrmask == 0);

      U_RETURN(result);
      }

   static const char* getIPAddress(const char* ptr, uint32_t len);

private:
   UNoCatPlugIn(const UNoCatPlugIn&) : UServerPlugIn(), UEventTime() {}
   UNoCatPlugIn& operator=(const UNoCatPlugIn&)                      { return *this; }

   friend class UModNoCatPeer;
};

#endif
