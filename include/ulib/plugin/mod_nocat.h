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

class UIptAccount;
class UNoCatPlugIn;

class UModNoCatPeer : public UIPAddress, UEventTime {
public:

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   enum Status { PEER_DENY, PEER_ACCEPT };

   // COSTRUTTORI

            UModNoCatPeer(const char* ip);
   virtual ~UModNoCatPeer()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UModNoCatPeer)
      }

   // VARIE

   void setCommand(const UString& access_cmd)
      {
      U_TRACE(0, "UModNoCatPeer::setCommand(%.*S)", U_STRING_TO_TRACE(access_cmd))

      // NB: AUTOMATIC PARAMS(5) ADDED BY THIS PLUGIN: [action(permit|deny) mac ip class(Owner|Member|Public) rulenum]

      command.snprintf("%.*s (permit|deny) %.*s %.*s Member %u", U_STRING_TO_TRACE(access_cmd), U_STRING_TO_TRACE(mac), U_STRING_TO_TRACE(ip), rulenum);

      cmd.set(command, (char**)0);
      }

   // define method VIRTUAL of class UEventTime

   virtual int handlerTime();

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   uint32_t rulenum;
   int status, ifindex;
   uint32_t traffic, ctraffic, ltraffic;
   time_t connected, expire, logout, ctime;
   UString ip, mac, token, command, user, ifname;
   UCommand cmd;

private:
   UModNoCatPeer(const UModNoCatPeer&) : UIPAddress(), UEventTime() {}
   UModNoCatPeer& operator=(const UModNoCatPeer&)                   { return *this; }

   friend class UNoCatPlugIn;
};

// override the default...
template <> inline void u_destroy(UIPAddress** ptr, uint32_t n) { U_TRACE(0,"u_destroy<UIPAddress*>(%p,%u)", ptr, n) }

class U_EXPORT UNoCatPlugIn : public UServerPlugIn {
public:

   static UString* str_AUTH_SERVICE_URL;
   static UString* str_AUTH_SERVICE_IP;
   static UString* str_LOGOUT_URL;
   static UString* str_LOGIN_TIMEOUT;
   static UString* str_INIT_CMD;
   static UString* str_ACCESS_CMD;
   static UString* str_RESET_CMD;
   static UString* str_DECRYPT_CMD;
   static UString* str_DECRYPT_KEY;
   static UString* str_CHECK_BY_ARPING;
   static UString* str_Action;
   static UString* str_Permit;
   static UString* str_Deny;
   static UString* str_Mode;
   static UString* str_Redirect;
   static UString* str_renew;
   static UString* str_Mac;
   static UString* str_Timeout;
   static UString* str_Token;
   static UString* str_User;
   static UString* str_anonymous;
   static UString* str_Traffic;

   static void str_allocate();

   // COSTRUTTORI

            UNoCatPlugIn();
   virtual ~UNoCatPlugIn();

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerInit();

   // Connection-wide hooks

   virtual int handlerRead()
      {
      U_TRACE(0, "UNoCatPlugIn::handlerRead()")

      U_RETURN(U_PLUGIN_HANDLER_GO_ON);
      }

   virtual int handlerRequest();
   virtual int handlerReset()
      {
      U_TRACE(0, "UNoCatPlugIn::handlerReset()")

      U_RETURN(U_PLUGIN_HANDLER_GO_ON);
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UCommand cmd, pgp;
   Url auth_service_url, logout_url; // NB: we need *_url before vfwopt to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...
   UVector<UString> vfwopt, vInternalDevice, vLocalNetwork;
   UString input, output, location, init_cmd, reset_cmd, access_cmd, decrypt_cmd,
           decrypt_key, mode, gateway, access_point, auth_ip, auth_ip_mask;

   static Url* info;
   static bool arping;
   static vPF unatexit;
   static int fd_stderr;
   static UPing** sockp;
   static fd_set* addrmask;
   static UIptAccount* ipt;
   static UNoCatPlugIn* pthis;
   static UString* status_content;
   static time_t last_request_check;
   static UVector<UIPAllow*>* vauth_ip;
   static UVector<UIPAddress*>** vaddr;
   static UHashMap<UModNoCatPeer*>* peers;

   static char pcStrAddress[INET6_ADDRSTRLEN];
   static uint32_t total_connections, login_timeout, nfds, num_radio;

   // VARIE

   void getTraffic();
   void checkPeersForInfo();
   void setStatusContent(UModNoCatPeer* peer);
   bool checkAuthMessage(UModNoCatPeer* peer);
   bool checkSignedData(const char* ptr, uint32_t len);
   void addPeerInfo(UModNoCatPeer* peer, time_t logout);
   void setRedirectLocation(UModNoCatPeer* peer, const UString& url);

   static UModNoCatPeer* getPeer(uint32_t i);
          UModNoCatPeer* creatNewPeer(const char* peer_ip);
          UModNoCatPeer* getPeerFromMAC(const UString& mac);

   static void notifyAuthOfUsersInfo();
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
   UNoCatPlugIn(const UNoCatPlugIn&) : UServerPlugIn() {}
   UNoCatPlugIn& operator=(const UNoCatPlugIn&)        { return *this; }

   friend class UModNoCatPeer;
};

#endif
