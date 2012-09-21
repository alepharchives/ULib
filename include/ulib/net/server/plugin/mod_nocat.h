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

class UIptAccount;
class UNoCatPlugIn;

class UModNoCatPeer : public UIPAddress, UEventTime {
public:

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   enum Status { PEER_DENY, PEER_ACCEPT };

   // COSTRUTTORI

            UModNoCatPeer();
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

      command.snprintf("/bin/sh %.*s request %.*s %s Member %u", U_STRING_TO_TRACE(script), U_STRING_TO_TRACE(mac), UIPAddress::pcStrAddress, rulenum);

      fw.set(command, (char**)0);
      }

   // define method VIRTUAL of class UEventTime

   virtual int handlerTime();

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   uint64_t traffic_done, traffic_available;
   UString ip, mac, token, user, ifname, label, gateway;
   UCommand fw;
   time_t connected, expire, logout, ctime;
   uint32_t ctraffic, rulenum, index_AUTH, index_device, index_network;
   int status;
   bool allowed;

private:
   UModNoCatPeer(const UModNoCatPeer&) : UIPAddress(), UEventTime() {}
   UModNoCatPeer& operator=(const UModNoCatPeer&)                   { return *this; }

   friend class UNoCatPlugIn;
};

// override the default...
template <> inline void u_destroy(UIPAddress** ptr, uint32_t n) { U_TRACE(0,"u_destroy<UIPAddress*>(%p,%u)", ptr, n) }

class U_EXPORT UNoCatPlugIn : public UServerPlugIn, UEventTime {
public:

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   static const UString* str_ROUTE_ONLY;
   static const UString* str_DNS_ADDR;
   static const UString* str_INCLUDE_PORTS;
   static const UString* str_EXCLUDE_PORTS;
   static const UString* str_ALLOWED_WEB_HOSTS;
   static const UString* str_EXTERNAL_DEVICE;
   static const UString* str_INTERNAL_DEVICE;
   static const UString* str_LOCAL_NETWORK;
   static const UString* str_LOCAL_NETWORK_LABEL;
   static const UString* str_AUTH_SERVICE_URL;
   static const UString* str_LOGIN_TIMEOUT;
   static const UString* str_FW_CMD;
   static const UString* str_DECRYPT_CMD;
   static const UString* str_DECRYPT_KEY;
   static const UString* str_CHECK_TYPE;
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
   static const UString* str_allowed;
   static const UString* str_anonymous;
   static const UString* str_Traffic;
   static const UString* str_GATEWAY_PORT;
   static const UString* str_FW_ENV;
   static const UString* str_IPHONE_SUCCESS;
   static const UString* str_CHECK_EXPIRE_INTERVAL;
   static const UString* str_ALLOWED_MEMBERS;
   static const UString* str_without_mac;
   static const UString* str_without_label;
   static const UString* str_allowed_members_default;

   static void str_allocate();

   enum CheckType {
      U_CHECK_NONE      = 0x000,
      U_CHECK_ARP_CACHE = 0x001,
      U_CHECK_ARP_PING  = 0x002,
      U_CHECK_TRAFFIC   = 0x004
   };

   // COSTRUTTORI

            UNoCatPlugIn();
   virtual ~UNoCatPlugIn();

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerInit();
   virtual int handlerFork();

   // Connection-wide hooks

   virtual int handlerRequest();

   // define method VIRTUAL of class UEventTime

   virtual int handlerTime();

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UCommand fw, uclient, uploader;
   UVector<Url*> vauth_url, vinfo_url;
   UVector<UIPAllow*> vLocalNetworkMask;
   UVector<UString> vInternalDevice, vLocalNetwork, vLocalNetworkLabel, vLocalNetworkGateway, vauth, vauth_ip, vLoginValidate;
   UString input, output, location, fw_cmd, decrypt_key, mode, host, hostname, ip, extdev, intdev, localnet, auth_login, fw_env, allowed_members, label;

   static vPF unatexit;
   static UPing** sockp;
   static fd_set addrmask;
   static fd_set* paddrmask;
   static UIptAccount* ipt;
   static UNoCatPlugIn* pthis;
   static UString* status_content;
   static int fd_stderr, check_type;
   static UVector<UIPAddress*>** vaddr;
   static const UString* label_to_match;
   static bool flag_check_peers_for_info;
   static UHashMap<UModNoCatPeer*>* peers;
   static time_t last_request, last_request_check, check_expire;
   static uint32_t total_connections, login_timeout, nfds, num_radio;

   // VARIE

   uint32_t getIndexAUTH(const char* ip_address) __pure;

   void           checkOldPeer(UModNoCatPeer* peer);
   void             setNewPeer(UModNoCatPeer* peer, uint32_t index_AUTH);
   UModNoCatPeer* creatNewPeer(uint32_t index_AUTH);

   void setPeerListInfo();
   void checkPeersForInfo();
   bool checkAuthMessage(UModNoCatPeer* peer);
   bool checkSignedData(const char* ptr, uint32_t len);
   void addPeerInfo(UModNoCatPeer* peer, time_t logout);
   void setStatusContent(UModNoCatPeer* peer, const UString& label);
   void setRedirectLocation(UModNoCatPeer* peer, const UString& redirect, const Url& auth);
   bool sendMsgToPortal(UCommand& cmd, uint32_t index_AUTH, const UString& msg, UString* poutput = 0);

   void sendMsgToPortal(UCommand& cmd, const UString& msg)
      {
      U_TRACE(0, "UNoCatPlugIn::sendMsgToPortal(%p,%.*S)", &cmd, U_STRING_TO_TRACE(msg))

      for (uint32_t i = 0, n = pthis->vinfo_url.size(); i < n; ++i) (void) sendMsgToPortal(cmd, i, msg, 0);
      }

   static UModNoCatPeer* getPeer(uint32_t i) __pure;
          UModNoCatPeer* getPeerFromMAC(const UString& mac);

   static UString getIPAddress(const char* ptr, uint32_t len);

   static void getTraffic();
   static void setHTTPResponse(const UString& content);
   static void notifyAuthOfUsersInfo(uint32_t index_AUTH);
   static void getPeerStatus(UStringRep* key, void* value);
   static void checkPeerInfo(UStringRep* key, void* value);
   static void getPeerListInfo(UStringRep* key, void* value);
   static void executeCommand(UModNoCatPeer* peer, int type);

   static void permit(UModNoCatPeer* peer, time_t timeout);
   static void   deny(UModNoCatPeer* peer, bool alarm, bool disconnected);

   static bool isPingAsyncPending()
      {
      U_TRACE(0, "UNoCatPlugIn::isPingAsyncPending()")

      U_INTERNAL_DUMP("check_type = %B nfds = %u paddrmask = %p", check_type, nfds, paddrmask)

      bool result = (((check_type & U_CHECK_ARP_PING) != 0) && nfds && paddrmask == 0);

      U_RETURN(result);
      }

private:
   UNoCatPlugIn(const UNoCatPlugIn&) : UServerPlugIn(), UEventTime() {}
   UNoCatPlugIn& operator=(const UNoCatPlugIn&)                      { return *this; }

   friend class UModNoCatPeer;
};

#endif
