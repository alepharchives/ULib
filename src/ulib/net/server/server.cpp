// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    server.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/db/rdb.h>
#include <ulib/command.h>
#include <ulib/notifier.h>
#include <ulib/file_config.h>
#include <ulib/net/udpsocket.h>
#include <ulib/utility/uhttp.h>
#include <ulib/utility/services.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/string_ext.h>

#ifdef USE_LIBSSL
#  include <ulib/ssl/net/ssl_session.h>
#endif

#ifdef U_STATIC_HANDLER_RPC
#  include <ulib/net/server/plugin/mod_rpc.h>
#endif
#ifdef U_STATIC_HANDLER_SHIB
#  include <ulib/net/server/plugin/mod_shib.h>
#endif
#ifdef U_STATIC_HANDLER_ECHO
#  include <ulib/net/server/plugin/mod_echo.h>
#endif
#ifdef U_STATIC_HANDLER_STREAM
#  include <ulib/net/server/plugin/mod_stream.h>
#endif
#ifdef U_STATIC_HANDLER_NOCAT
#  include <ulib/net/server/plugin/mod_nocat.h>
#endif
#ifdef U_STATIC_HANDLER_SOCKET
#  include <ulib/net/server/plugin/mod_socket.h>
#endif
#ifdef U_STATIC_HANDLER_SCGI
#  include <ulib/net/server/plugin/mod_scgi.h>
#endif
#ifdef U_STATIC_HANDLER_FCGI
#  include <ulib/net/server/plugin/mod_fcgi.h>
#endif
#ifdef U_STATIC_HANDLER_GEOIP
#  include <ulib/net/server/plugin/mod_geoip.h>
#endif
#ifdef U_STATIC_HANDLER_PROXY
#  include <ulib/net/server/plugin/mod_proxy.h>
#endif
#ifdef U_STATIC_HANDLER_SOAP
#  include <ulib/net/server/plugin/mod_soap.h>
#endif
#ifdef U_STATIC_HANDLER_SSI
#  include <ulib/net/server/plugin/mod_ssi.h>
#endif
#ifdef U_STATIC_HANDLER_TSA
#  include <ulib/net/server/plugin/mod_tsa.h>
#endif
#ifdef U_STATIC_HANDLER_HTTP
#  include <ulib/net/server/plugin/mod_http.h>
#endif

#ifndef __MINGW32__
#  include <pwd.h>
#  include <ulib/net/unixsocket.h>
#  define U_TCP_SETTING yes
#endif

#define U_DEFAULT_PORT 80

int                               UServer_Base::sfd;
int                               UServer_Base::port;
int                               UServer_Base::bclose;
int                               UServer_Base::iBackLog = SOMAXCONN;
int                               UServer_Base::timeoutMS = -1;
int                               UServer_Base::cgi_timeout;
int                               UServer_Base::verify_mode;
int                               UServer_Base::preforked_num_kids;
bool                              UServer_Base::bssl;
bool                              UServer_Base::bipc;
bool                              UServer_Base::flag_loop;
bool                              UServer_Base::public_address;
bool                              UServer_Base::monitoring_process;
bool                              UServer_Base::bpluginsHandlerReset;
bool                              UServer_Base::bpluginsHandlerRequest;
bool                              UServer_Base::accept_edge_triggered;
bool                              UServer_Base::enable_rfc1918_filter;
bool                              UServer_Base::set_realtime_priority;
bool                              UServer_Base::flag_use_tcp_optimization;
ULog*                             UServer_Base::log;
pid_t                             UServer_Base::pid;
time_t                            UServer_Base::expire;
time_t                            UServer_Base::last_event;
uint32_t                          UServer_Base::start;
uint32_t                          UServer_Base::count;
uint32_t                          UServer_Base::map_size;
uint32_t                          UServer_Base::vplugin_size;
uint32_t                          UServer_Base::oClientImage;
uint32_t                          UServer_Base::shared_data_add;
UString*                          UServer_Base::mod_name;
UString*                          UServer_Base::host;
UString*                          UServer_Base::senvironment;
UString*                          UServer_Base::server;
UString*                          UServer_Base::as_user;
UString*                          UServer_Base::log_file;
UString*                          UServer_Base::dh_file;
UString*                          UServer_Base::cert_file;
UString*                          UServer_Base::key_file;
UString*                          UServer_Base::password;
UString*                          UServer_Base::ca_file;
UString*                          UServer_Base::ca_path;
UString*                          UServer_Base::name_sock;
UString*                          UServer_Base::IP_address;
UString*                          UServer_Base::allow_IP;
UString*                          UServer_Base::allow_IP_prv;
UString*                          UServer_Base::document_root;
USocket*                          UServer_Base::socket;
UProcess*                         UServer_Base::proc;
UEventFd*                         UServer_Base::handler_inotify;
UEventTime*                       UServer_Base::ptime;
UServer_Base*                     UServer_Base::pthis;
UVector<UString>*                 UServer_Base::vplugin_name;
UVector<UString>*                 UServer_Base::vplugin_name_static;
UClientImage_Base*                UServer_Base::pClientIndex;
UClientImage_Base*                UServer_Base::pClientImage;
UClientImage_Base*                UServer_Base::vClientImage;
UClientImage_Base*                UServer_Base::eClientImage;
UVector<UIPAllow*>*               UServer_Base::vallow_IP;
UVector<UIPAllow*>*               UServer_Base::vallow_IP_prv;
UVector<UServerPlugIn*>*          UServer_Base::vplugin;
UServer_Base::shared_data*        UServer_Base::ptr_shared_data;
UVector<UServer_Base::file_LOG*>* UServer_Base::vlog;

const UString* UServer_Base::str_ENABLE_IPV6;
const UString* UServer_Base::str_PORT;
const UString* UServer_Base::str_MSG_WELCOME;
const UString* UServer_Base::str_COMMAND;
const UString* UServer_Base::str_ENVIRONMENT;
const UString* UServer_Base::str_RUN_AS_USER;
const UString* UServer_Base::str_PREFORK_CHILD;
const UString* UServer_Base::str_LOG_FILE;
const UString* UServer_Base::str_LOG_FILE_SZ;
const UString* UServer_Base::str_LOG_MSG_SIZE;
const UString* UServer_Base::str_CERT_FILE;
const UString* UServer_Base::str_KEY_FILE;
const UString* UServer_Base::str_PASSWORD;
const UString* UServer_Base::str_DH_FILE;
const UString* UServer_Base::str_CA_FILE;
const UString* UServer_Base::str_CA_PATH;
const UString* UServer_Base::str_VERIFY_MODE;
const UString* UServer_Base::str_ALLOWED_IP;
const UString* UServer_Base::str_ALLOWED_IP_PRIVATE;
const UString* UServer_Base::str_SOCKET_NAME;
const UString* UServer_Base::str_DOCUMENT_ROOT;
const UString* UServer_Base::str_PLUGIN;
const UString* UServer_Base::str_PLUGIN_DIR;
const UString* UServer_Base::str_REQ_TIMEOUT;
const UString* UServer_Base::str_CGI_TIMEOUT;
const UString* UServer_Base::str_VIRTUAL_HOST;
const UString* UServer_Base::str_DIGEST_AUTHENTICATION;
const UString* UServer_Base::str_URI;
const UString* UServer_Base::str_HOST;
const UString* UServer_Base::str_USER;
const UString* UServer_Base::str_SERVER;
const UString* UServer_Base::str_METHOD_NAME;
const UString* UServer_Base::str_RESPONSE_TYPE;
const UString* UServer_Base::str_IP_ADDRESS;
const UString* UServer_Base::str_MAX_KEEP_ALIVE;
const UString* UServer_Base::str_PID_FILE;
const UString* UServer_Base::str_USE_TCP_OPTIMIZATION;
const UString* UServer_Base::str_LISTEN_BACKLOG;
const UString* UServer_Base::str_SET_REALTIME_PRIORITY;
const UString* UServer_Base::str_ENABLE_RFC1918_FILTER;

#if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD)
#  include <ulib/thread.h>

class UTimeThread : public UThread {
public:

   UTimeThread() : UThread(true, false) {}

#ifdef DEBUG
   UTimeVal before;
#endif
   int watch_counter;

   virtual void run()
      {
      U_TRACE(0, "UTimeThread::run()")

#  ifdef DEBUG
      long delta;
      UTimeVal after;
#  endif

      U_SRV_LOG("UTimeThread optimization for time resolution of one second activated (pid %u)", UThread::getTID());

      watch_counter      = 1;
      struct timespec ts = { 1L, 0L };

      while (UServer_Base::flag_loop)
         {
         (void) nanosleep(&ts, 0);

         U_INTERNAL_DUMP("watch_counter = %d", watch_counter)

         if (--watch_counter > 0) u_now->tv_sec += 1L;
         else
            {
#        ifdef DEBUG
            if (watch_counter == 0)
               {
               u_now->tv_sec += 1L;

               before.set(*u_now);
               }
#        endif

            watch_counter = 30;

            (void) U_SYSCALL(gettimeofday, "%p,%p", u_now, 0);

#        ifdef DEBUG
            after.set(*u_now);

            after -= before;
            delta  = after.getMilliSecond();

            if (delta >=  1000L ||
                delta <= -1000L)
               {
               U_SRV_LOG("UTimeThread delta time exceed 1 sec: diff(%ld ms) counter(%d)", delta, watch_counter);
               }
#        endif
            }
         }
      }
};

class UClientThread : public UThread {
public:

   UClientThread() : UThread(true, false) {}

   virtual void run()
      {
      U_TRACE(0, "UClientThread::run()")

      while (UServer_Base::flag_loop) UNotifier::waitForEvent(UServer_Base::ptime);
      }
};
#endif

#ifndef __MINGW32__
static int sysctl_somaxconn, tcp_abort_on_overflow, sysctl_max_syn_backlog, tcp_fin_timeout;
#endif

void UServer_Base::str_allocate()
{
   U_TRACE(0, "UServer_Base::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_ENABLE_IPV6,0)
   U_INTERNAL_ASSERT_EQUALS(str_PORT,0)
   U_INTERNAL_ASSERT_EQUALS(str_MSG_WELCOME,0)
   U_INTERNAL_ASSERT_EQUALS(str_COMMAND,0)
   U_INTERNAL_ASSERT_EQUALS(str_ENVIRONMENT,0)
   U_INTERNAL_ASSERT_EQUALS(str_RUN_AS_USER,0)
   U_INTERNAL_ASSERT_EQUALS(str_PREFORK_CHILD,0)
   U_INTERNAL_ASSERT_EQUALS(str_LOG_FILE,0)
   U_INTERNAL_ASSERT_EQUALS(str_LOG_FILE_SZ,0)
   U_INTERNAL_ASSERT_EQUALS(str_LOG_MSG_SIZE,0)
   U_INTERNAL_ASSERT_EQUALS(str_CERT_FILE,0)
   U_INTERNAL_ASSERT_EQUALS(str_KEY_FILE,0)
   U_INTERNAL_ASSERT_EQUALS(str_PASSWORD,0)
   U_INTERNAL_ASSERT_EQUALS(str_DH_FILE,0)
   U_INTERNAL_ASSERT_EQUALS(str_CA_FILE,0)
   U_INTERNAL_ASSERT_EQUALS(str_CA_PATH,0)
   U_INTERNAL_ASSERT_EQUALS(str_VERIFY_MODE,0)
   U_INTERNAL_ASSERT_EQUALS(str_ALLOWED_IP,0)
   U_INTERNAL_ASSERT_EQUALS(str_ALLOWED_IP_PRIVATE,0)
   U_INTERNAL_ASSERT_EQUALS(str_SOCKET_NAME,0)
   U_INTERNAL_ASSERT_EQUALS(str_DOCUMENT_ROOT,0)
   U_INTERNAL_ASSERT_EQUALS(str_PLUGIN,0)
   U_INTERNAL_ASSERT_EQUALS(str_PLUGIN_DIR,0)
   U_INTERNAL_ASSERT_EQUALS(str_REQ_TIMEOUT,0)
   U_INTERNAL_ASSERT_EQUALS(str_CGI_TIMEOUT,0)
   U_INTERNAL_ASSERT_EQUALS(str_VIRTUAL_HOST,0)
   U_INTERNAL_ASSERT_EQUALS(str_DIGEST_AUTHENTICATION,0)
   U_INTERNAL_ASSERT_EQUALS(str_URI,0)
   U_INTERNAL_ASSERT_EQUALS(str_HOST,0)
   U_INTERNAL_ASSERT_EQUALS(str_USER,0)
   U_INTERNAL_ASSERT_EQUALS(str_SERVER,0)
   U_INTERNAL_ASSERT_EQUALS(str_METHOD_NAME,0)
   U_INTERNAL_ASSERT_EQUALS(str_RESPONSE_TYPE,0)
   U_INTERNAL_ASSERT_EQUALS(str_IP_ADDRESS,0)
   U_INTERNAL_ASSERT_EQUALS(str_MAX_KEEP_ALIVE,0)
   U_INTERNAL_ASSERT_EQUALS(str_PID_FILE,0)
   U_INTERNAL_ASSERT_EQUALS(str_USE_TCP_OPTIMIZATION,0)
   U_INTERNAL_ASSERT_EQUALS(str_LISTEN_BACKLOG,0)
   U_INTERNAL_ASSERT_EQUALS(str_SET_REALTIME_PRIORITY,0)
   U_INTERNAL_ASSERT_EQUALS(str_ENABLE_RFC1918_FILTER,0)

   static ustringrep stringrep_storage[] = {
   { U_STRINGREP_FROM_CONSTANT("ENABLE_IPV6") },
   { U_STRINGREP_FROM_CONSTANT("PORT") },
   { U_STRINGREP_FROM_CONSTANT("WELCOME_MSG") },
   { U_STRINGREP_FROM_CONSTANT("COMMAND") },
   { U_STRINGREP_FROM_CONSTANT("ENVIRONMENT") },
   { U_STRINGREP_FROM_CONSTANT("RUN_AS_USER") },
   { U_STRINGREP_FROM_CONSTANT("PREFORK_CHILD") },
   { U_STRINGREP_FROM_CONSTANT("LOG_FILE") },
   { U_STRINGREP_FROM_CONSTANT("LOG_FILE_SZ") },
   { U_STRINGREP_FROM_CONSTANT("LOG_MSG_SIZE") },
   { U_STRINGREP_FROM_CONSTANT("CERT_FILE") },
   { U_STRINGREP_FROM_CONSTANT("KEY_FILE") },
   { U_STRINGREP_FROM_CONSTANT("PASSWORD") },
   { U_STRINGREP_FROM_CONSTANT("DH_FILE") },
   { U_STRINGREP_FROM_CONSTANT("CA_FILE") },
   { U_STRINGREP_FROM_CONSTANT("CA_PATH") },
   { U_STRINGREP_FROM_CONSTANT("VERIFY_MODE") },
   { U_STRINGREP_FROM_CONSTANT("ALLOWED_IP") },
   { U_STRINGREP_FROM_CONSTANT("ALLOWED_IP_PRIVATE") },
   { U_STRINGREP_FROM_CONSTANT("SOCKET_NAME") },
   { U_STRINGREP_FROM_CONSTANT("DOCUMENT_ROOT") },
   { U_STRINGREP_FROM_CONSTANT("PLUGIN") },
   { U_STRINGREP_FROM_CONSTANT("PLUGIN_DIR") },
   { U_STRINGREP_FROM_CONSTANT("REQ_TIMEOUT") },
   { U_STRINGREP_FROM_CONSTANT("CGI_TIMEOUT") },
   { U_STRINGREP_FROM_CONSTANT("VIRTUAL_HOST") },
   { U_STRINGREP_FROM_CONSTANT("DIGEST_AUTHENTICATION") },
   { U_STRINGREP_FROM_CONSTANT("URI") },
   { U_STRINGREP_FROM_CONSTANT("HOST") },
   { U_STRINGREP_FROM_CONSTANT("USER") },
   { U_STRINGREP_FROM_CONSTANT("SERVER") },
   { U_STRINGREP_FROM_CONSTANT("METHOD_NAME") },
   { U_STRINGREP_FROM_CONSTANT("RESPONSE_TYPE") },
   { U_STRINGREP_FROM_CONSTANT("IP_ADDRESS") },
   { U_STRINGREP_FROM_CONSTANT("MAX_KEEP_ALIVE") },
   { U_STRINGREP_FROM_CONSTANT("PID_FILE") },
   { U_STRINGREP_FROM_CONSTANT("USE_TCP_OPTIMIZATION") },
   { U_STRINGREP_FROM_CONSTANT("LISTEN_BACKLOG") },
   { U_STRINGREP_FROM_CONSTANT("SET_REALTIME_PRIORITY") },
   { U_STRINGREP_FROM_CONSTANT("ENABLE_RFC1918_FILTER") }
   };

   U_NEW_ULIB_OBJECT(str_ENABLE_IPV6,           U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_PORT,                  U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_MSG_WELCOME,           U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_COMMAND,               U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_ENVIRONMENT,           U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_RUN_AS_USER,           U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_PREFORK_CHILD,         U_STRING_FROM_STRINGREP_STORAGE(6));
   U_NEW_ULIB_OBJECT(str_LOG_FILE,              U_STRING_FROM_STRINGREP_STORAGE(7));
   U_NEW_ULIB_OBJECT(str_LOG_FILE_SZ,           U_STRING_FROM_STRINGREP_STORAGE(8));
   U_NEW_ULIB_OBJECT(str_LOG_MSG_SIZE,          U_STRING_FROM_STRINGREP_STORAGE(9));
   U_NEW_ULIB_OBJECT(str_CERT_FILE,             U_STRING_FROM_STRINGREP_STORAGE(10));
   U_NEW_ULIB_OBJECT(str_KEY_FILE,              U_STRING_FROM_STRINGREP_STORAGE(11));
   U_NEW_ULIB_OBJECT(str_PASSWORD,              U_STRING_FROM_STRINGREP_STORAGE(12));
   U_NEW_ULIB_OBJECT(str_DH_FILE,               U_STRING_FROM_STRINGREP_STORAGE(13));
   U_NEW_ULIB_OBJECT(str_CA_FILE,               U_STRING_FROM_STRINGREP_STORAGE(14));
   U_NEW_ULIB_OBJECT(str_CA_PATH,               U_STRING_FROM_STRINGREP_STORAGE(15));
   U_NEW_ULIB_OBJECT(str_VERIFY_MODE,           U_STRING_FROM_STRINGREP_STORAGE(16));
   U_NEW_ULIB_OBJECT(str_ALLOWED_IP,            U_STRING_FROM_STRINGREP_STORAGE(17));
   U_NEW_ULIB_OBJECT(str_ALLOWED_IP_PRIVATE,    U_STRING_FROM_STRINGREP_STORAGE(18));
   U_NEW_ULIB_OBJECT(str_SOCKET_NAME,           U_STRING_FROM_STRINGREP_STORAGE(19));
   U_NEW_ULIB_OBJECT(str_DOCUMENT_ROOT,         U_STRING_FROM_STRINGREP_STORAGE(20));
   U_NEW_ULIB_OBJECT(str_PLUGIN,                U_STRING_FROM_STRINGREP_STORAGE(21));
   U_NEW_ULIB_OBJECT(str_PLUGIN_DIR,            U_STRING_FROM_STRINGREP_STORAGE(22));
   U_NEW_ULIB_OBJECT(str_REQ_TIMEOUT,           U_STRING_FROM_STRINGREP_STORAGE(23));
   U_NEW_ULIB_OBJECT(str_CGI_TIMEOUT,           U_STRING_FROM_STRINGREP_STORAGE(24));
   U_NEW_ULIB_OBJECT(str_VIRTUAL_HOST,          U_STRING_FROM_STRINGREP_STORAGE(25));
   U_NEW_ULIB_OBJECT(str_DIGEST_AUTHENTICATION, U_STRING_FROM_STRINGREP_STORAGE(26));
   U_NEW_ULIB_OBJECT(str_URI,                   U_STRING_FROM_STRINGREP_STORAGE(27));
   U_NEW_ULIB_OBJECT(str_HOST,                  U_STRING_FROM_STRINGREP_STORAGE(28));
   U_NEW_ULIB_OBJECT(str_USER,                  U_STRING_FROM_STRINGREP_STORAGE(29));
   U_NEW_ULIB_OBJECT(str_SERVER,                U_STRING_FROM_STRINGREP_STORAGE(30));
   U_NEW_ULIB_OBJECT(str_METHOD_NAME,           U_STRING_FROM_STRINGREP_STORAGE(31));
   U_NEW_ULIB_OBJECT(str_RESPONSE_TYPE,         U_STRING_FROM_STRINGREP_STORAGE(32));
   U_NEW_ULIB_OBJECT(str_IP_ADDRESS,            U_STRING_FROM_STRINGREP_STORAGE(33));
   U_NEW_ULIB_OBJECT(str_MAX_KEEP_ALIVE,        U_STRING_FROM_STRINGREP_STORAGE(34));
   U_NEW_ULIB_OBJECT(str_PID_FILE,              U_STRING_FROM_STRINGREP_STORAGE(35));
   U_NEW_ULIB_OBJECT(str_USE_TCP_OPTIMIZATION,  U_STRING_FROM_STRINGREP_STORAGE(36));
   U_NEW_ULIB_OBJECT(str_LISTEN_BACKLOG,        U_STRING_FROM_STRINGREP_STORAGE(37));
   U_NEW_ULIB_OBJECT(str_SET_REALTIME_PRIORITY, U_STRING_FROM_STRINGREP_STORAGE(38));
   U_NEW_ULIB_OBJECT(str_ENABLE_RFC1918_FILTER, U_STRING_FROM_STRINGREP_STORAGE(39));
}

UServer_Base::UServer_Base(UFileConfig* cfg)
{
   U_TRACE_REGISTER_OBJECT(0, UServer_Base, "%p", cfg)

   U_INTERNAL_ASSERT_EQUALS(pthis,0)
   U_INTERNAL_ASSERT_EQUALS(senvironment,0)

   server        = U_NEW(UString);
   as_user       = U_NEW(UString);
   log_file      = U_NEW(UString);
   dh_file       = U_NEW(UString);
   cert_file     = U_NEW(UString);
   key_file      = U_NEW(UString);
   password      = U_NEW(UString);
   ca_file       = U_NEW(UString);
   ca_path       = U_NEW(UString);
   name_sock     = U_NEW(UString);
   IP_address    = U_NEW(UString);
   allow_IP      = U_NEW(UString);
   allow_IP_prv  = U_NEW(UString);
   document_root = U_NEW(UString);

   if (str_ENABLE_IPV6 == 0) str_allocate();

   port         = U_DEFAULT_PORT;
   pthis        = this;

   vlog         = U_NEW(UVector<file_LOG*>);
   mod_name     = U_NEW(UString(32U));
   senvironment = U_NEW(UString(U_CAPACITY));

#ifndef __MINGW32__
   flag_use_tcp_optimization = true;
#endif

   U_INTERNAL_DUMP("u_seed_hash = %u", u_seed_hash)

   u_init_ulib_hostname();

   U_INTERNAL_DUMP("u_hostname(%u) = %.*S", u_hostname_len, u_hostname_len, u_hostname)

   u_init_ulib_username();

   U_INTERNAL_DUMP("u_user_name(%u) = %.*S", u_user_name_len, u_user_name_len, u_user_name)

   u_init_security();

#if !defined(HAVE_EPOLL_WAIT) && !defined(USE_LIBEVENT) && defined(DEBUG)
   U_INTERNAL_DUMP("SOMAXCONN = %d FD_SETSIZE = %d", SOMAXCONN, FD_SETSIZE)
#endif

   if (u_start_time     == 0 &&
       u_setStartTime() == false)
      {
      U_WARNING("System date not updated: %#5D", u_now->tv_sec);
      }

   if (cfg) loadConfigParam(*cfg);
}

UServer_Base::~UServer_Base()
{
   U_TRACE_UNREGISTER_OBJECT(0, UServer_Base)

   UClientImage_Base::pipeline = false;

   UClientImage_Base::initAfterGenericRead();

   U_INTERNAL_ASSERT_POINTER(vplugin)

   delete vplugin;
   delete vplugin_name;

#ifdef HAVE_MODULES
   UPlugIn<void*>::clear();
#endif

   UClientImage_Base::clear();

   U_INTERNAL_ASSERT_EQUALS(handler_inotify,0)

   U_INTERNAL_ASSERT_POINTER(senvironment)

   delete mod_name;
   delete senvironment;

   if (log)           delete log;
   if (host)          delete host;
   if (ptime)         delete ptime;
   if (vallow_IP)     delete vallow_IP;
   if (vallow_IP_prv) delete vallow_IP_prv;

   if (vlog)
      {
      file_LOG* item;

      for (uint32_t i = 0, n = vlog->size(); i < n; ++i)
         {
         item = (*vlog)[i];

                item->LOG->close();
         delete item->LOG;
         }

             vlog->clear();
      delete vlog;
      }

#ifdef USE_LIBSSL
   if (USSLSession::db_ssl_session) USSLSession::deleteSessionCache();
#endif

   U_INTERNAL_ASSERT_POINTER(socket)

   delete socket;

#ifndef __MINGW32__
   if (as_user->empty() &&
       isChild() == false)
      {
      u_need_root(false);

      if (flag_use_tcp_optimization)
         {
#     ifdef U_TCP_SETTING 
         (void) UFile::setSysParam("/proc/sys/net/ipv4/tcp_fin_timeout", tcp_fin_timeout, true);

         if (iBackLog >= SOMAXCONN)
            {
            (void) UFile::setSysParam("/proc/sys/net/core/somaxconn",           sysctl_somaxconn,       true);
            (void) UFile::setSysParam("/proc/sys/net/ipv4/tcp_max_syn_backlog", sysctl_max_syn_backlog, true);
            }
#     endif
         }

      if (iBackLog == 1) (void) UFile::setSysParam("/proc/sys/net/ipv4/tcp_abort_on_overflow", tcp_abort_on_overflow, true);
      }
#endif

   if (proc) delete proc;

   if (ptr_shared_data) UFile::munmap(ptr_shared_data, map_size);

   delete server;
   delete as_user;
   delete log_file;
   delete dh_file;
   delete cert_file;
   delete key_file;
   delete password;
   delete ca_file;
   delete ca_path;
   delete name_sock;
   delete IP_address;
   delete allow_IP;
   delete allow_IP_prv;
   delete document_root;

   UEventFd::fd = 0; // NB: to avoid delete itself...

   UNotifier::num_connection = 0;

   UNotifier::clear();
}

void UServer_Base::loadConfigParam(UFileConfig& cfg)
{
   U_TRACE(0, "UServer_Base::loadConfigParam(%p)", &cfg)

   U_ASSERT_EQUALS(cfg.empty(), false)

   // --------------------------------------------------------------------------------------------------------------------------------------
   // userver - configuration parameters
   // --------------------------------------------------------------------------------------------------------------------------------------
   // ENABLE_IPV6        flag indicating the use of ipv6
   // SERVER             host name or ip address for the listening socket
   // PORT               port number             for the listening socket
   // SOCKET_NAME        file name               for the listening socket
   // IP_ADDRESS         ip address of host for the interface connected to the Internet (autodetected if not specified)
   // ALLOWED_IP         list of comma separated client address for IP-based access control (IPADDR[/MASK])
   //
   // ENABLE_RFC1918_FILTER reject request from private IP to public server address
   // ALLOWED_IP_PRIVATE    list of comma separated client private address for IP-based access control (IPADDR[/MASK]) for public server
   //
   // LISTEN_BACKLOG        max number of ready to be delivered connections to accept()
   // USE_TCP_OPTIMIZATION  flag indicating the use of TCP/IP options to optimize data transmission (DEFER_ACCEPT, QUICKACK)
   // SET_REALTIME_PRIORITY flag indicating that the preforked processes will be scheduled under the real-time policies SCHED_FIFO
   //
   // PID_FILE      write pid on file indicated
   // WELCOME_MSG   message of welcome to send initially to client
   // RUN_AS_USER   downgrade the security to that of user account
   // DOCUMENT_ROOT The directory out of which you will serve your documents
   //
   // LOG_FILE      locations   for file log
   // LOG_FILE_SZ   memory size for file log
   // LOG_MSG_SIZE  limit length of print network message to LOG_MSG_SIZE chars (default 128)
   //
   // PLUGIN        list of plugins to load, a flexible way to add specific functionality to the server
   // PLUGIN_DIR    directory of plugins to load
   //
   // REQ_TIMEOUT   timeout for request from client
   // CGI_TIMEOUT   timeout for cgi execution
   //
   // MAX_KEEP_ALIVE Specifies the maximum number of requests that can be served through a Keep-Alive (Persistent) session.
   //                (Value <= 0 will disable Keep-Alive) (default 1020)
   //
   // DH_FILE       DH param
   // CERT_FILE     server certificate
   // KEY_FILE      server private key
   // PASSWORD      password for server private key
   // CA_FILE       locations of trusted CA certificates used in the verification
   // CA_PATH       locations of trusted CA certificates used in the verification
   // VERIFY_MODE   mode of verification (SSL_VERIFY_NONE=0, SSL_VERIFY_PEER=1, SSL_VERIFY_FAIL_IF_NO_PEER_CERT=2, SSL_VERIFY_CLIENT_ONCE=4)
   //
   // PREFORK_CHILD number of child server processes created at startup: -1 - thread approach (experimental)
   //                                                                     0 - serialize, no forking
   //                                                                     1 - classic, forking after client accept
   //                                                                    >1 - pool of serialized processes plus monitoring process
   // --------------------------------------------------------------------------------------------------------------------------------------

   *server                    = cfg[*str_SERVER];
   *log_file                  = cfg[*str_LOG_FILE];
   *allow_IP                  = cfg[*str_ALLOWED_IP];
   *allow_IP_prv              = cfg[*str_ALLOWED_IP_PRIVATE];
   *name_sock                 = cfg[*str_SOCKET_NAME];
   *IP_address                = cfg[*str_IP_ADDRESS];

   port                       = cfg.readLong(*str_PORT, U_DEFAULT_PORT);
   iBackLog                   = cfg.readLong(*str_LISTEN_BACKLOG, SOMAXCONN);
   timeoutMS                  = cfg.readLong(*str_REQ_TIMEOUT);
   cgi_timeout                = cfg.readLong(*str_CGI_TIMEOUT);
   enable_rfc1918_filter      = cfg.readBoolean(*str_ENABLE_RFC1918_FILTER);
   set_realtime_priority      = cfg.readBoolean(*str_SET_REALTIME_PRIORITY);
   UNotifier::max_connection  = cfg.readLong(*str_MAX_KEEP_ALIVE);
   u_printf_string_max_length = cfg.readLong(*str_LOG_MSG_SIZE);

#ifdef ENABLE_IPV6
   UClientImage_Base::bIPv6   = cfg.readBoolean(*str_ENABLE_IPV6);
#endif

   if (timeoutMS) timeoutMS *= 1000;
   else           timeoutMS  = -1;

   if (cgi_timeout) UCommand::setTimeout(cgi_timeout);

   UClientImage_Base::setMsgWelcome(cfg[*str_MSG_WELCOME]);

#ifdef USE_LIBSSL
   *dh_file    = cfg[*str_DH_FILE];
   *ca_file    = cfg[*str_CA_FILE];
   *ca_path    = cfg[*str_CA_PATH];
   *key_file   = cfg[*str_KEY_FILE];
   *password   = cfg[*str_PASSWORD];
   *cert_file  = cfg[*str_CERT_FILE];

   verify_mode = cfg.readLong(*str_VERIFY_MODE);
#endif

#ifndef __MINGW32__
   UString x = cfg[*str_PREFORK_CHILD];

   if (x.empty() == false)
      {
      preforked_num_kids = x.strtol();

#  if !defined(HAVE_PTHREAD_H) || !defined(ENABLE_THREAD) || !defined(HAVE_EPOLL_WAIT) || defined(USE_LIBEVENT) || !defined(U_SERVER_THREAD_APPROACH_SUPPORT)
      if (preforked_num_kids == -1)
         {
         U_WARNING("Sorry, I was compiled without server thread approach so I can't accept PREFORK_CHILD == -1");

         goto next;
         }
#  endif
      }
   else
      {
#if !defined(HAVE_PTHREAD_H) || !defined(ENABLE_THREAD) || !defined(HAVE_EPOLL_WAIT) || defined(USE_LIBEVENT) || !defined(U_SERVER_THREAD_APPROACH_SUPPORT)
next:
#endif
      preforked_num_kids = u_get_num_cpu();

      U_INTERNAL_DUMP("num_cpu = %d", preforked_num_kids)

      if (preforked_num_kids < 2) preforked_num_kids = 2;
      }

   if (isPreForked()) monitoring_process = true;

   x = cfg[*str_USE_TCP_OPTIMIZATION];

   if (x.empty() == false) flag_use_tcp_optimization = x.strtob();
#endif

   // write pid on file...

   UString pid_file = cfg[*str_PID_FILE];

   if (pid_file.empty() == false) (void) UFile::writeTo(pid_file, UString(u_pid_str, u_pid_str_len));

   // open log

   if (log_file->empty() == false) log = U_NEW(ULog(*log_file, cfg.readLong(*str_LOG_FILE_SZ), "(pid %P) %10D> "));

   // If you want the webserver to run as a process of a defined user, you can do it.
   // For the change of user to work, it's necessary to execute the server with root privileges.
   // If it's started by a user that that doesn't have root privileges, this step will be omitted.

   *as_user = cfg[*str_RUN_AS_USER];

   if (as_user->empty() == false)
      {
      if (UServices::isSetuidRoot() == false)
         {
         U_SRV_LOG("The \"RUN_AS_USER\" directive makes sense only if the master process runs with super-user privileges, ignored");

         as_user->clear();
         }
      else
         {
         U_INTERNAL_ASSERT(as_user->isNullTerminated())

         struct passwd* pw = (struct passwd*) U_SYSCALL(getpwnam, "%S", as_user->data());

         if (pw && pw->pw_dir) u_setHOME(pw->pw_dir);
         }
      }

   // DOCUMENT_ROOT: The directory out of which you will serve your documents

   *document_root = cfg[*str_DOCUMENT_ROOT];

   if (document_root->empty() ||
       document_root->equal(U_CONSTANT_TO_PARAM(".")))
      {
      (void) document_root->replace(u_cwd, u_cwd_len);
      }
   else
      {
      U_INTERNAL_ASSERT(document_root->isNullTerminated())

      char c = document_root->c_char(0);

      if (c == '~' ||
          c == '$')
         {
         *document_root = UStringExt::expandPath(*document_root, 0);

         if (document_root->empty())
            {
            U_ERROR("var DOCUMENT_ROOT %S expansion FAILED. Going down...", document_root->data());
            }
         }

      (void) u_canonicalize_pathname(document_root->data());

      U_INTERNAL_DUMP("document_root = %S", document_root->data())
      }

   if (document_root->first_char() == '.') *document_root = UFile::getRealPath(document_root->data());

   if (UFile::chdir(document_root->data(), false) == false)
      {
      U_ERROR("chdir to working directory (DOCUMENT_ROOT) %S FAILED. Going down...", document_root->data());
      }

   U_SRV_LOG("Working directory (DOCUMENT_ROOT) changed to %S", u_cwd);

   // load plugin modules and call server-wide hooks handlerConfig()...

   UString plugin_dir  = cfg[*str_PLUGIN_DIR],
           plugin_list = cfg[*str_PLUGIN];

   if (loadPlugins(plugin_dir, plugin_list, &cfg) == U_PLUGIN_HANDLER_ERROR)
      {
      U_ERROR("Plugins load FAILED. Going down...");
      }

   cfg.clear();
   cfg.deallocate();
}

// load plugin modules and call server-wide hooks handlerConfig()...

U_NO_EXPORT void UServer_Base::loadStaticLinkedModules(const char* name)
{
   U_TRACE(0, "UServer_Base::loadStaticLinkedModules(%S)", name)

   U_INTERNAL_ASSERT_POINTER(vplugin_name)
   U_INTERNAL_ASSERT_MAJOR(vplugin_size, 0)

   UString x(name);

   if (vplugin_name->find(x) != U_NOT_FOUND) // NB: we load only the plugin that we want from configuration (PLUGIN var)...
      {
      UServerPlugIn* _plugin = 0;

#  ifdef U_STATIC_HANDLER_RPC
      if (x.equal(U_CONSTANT_TO_PARAM("mod_rpc")))    { _plugin = U_NEW(URpcPlugIn);       goto next; }
#  endif
#  ifdef U_STATIC_HANDLER_SHIB
      if (x.equal(U_CONSTANT_TO_PARAM("mod_shib")))   { _plugin = U_NEW(UShibPlugIn);      goto next; }
#  endif
#  ifdef U_STATIC_HANDLER_ECHO
      if (x.equal(U_CONSTANT_TO_PARAM("mod_echo")))   { _plugin = U_NEW(UEchoPlugIn);      goto next; }
#  endif
#  ifdef U_STATIC_HANDLER_STREAM
      if (x.equal(U_CONSTANT_TO_PARAM("mod_stream"))) { _plugin = U_NEW(UStreamPlugIn);    goto next; }
#  endif
#  ifdef U_STATIC_HANDLER_SOCKET
      if (x.equal(U_CONSTANT_TO_PARAM("mod_socket"))) { _plugin = U_NEW(UWebSocketPlugIn); goto next; }
#  endif
#  ifdef U_STATIC_HANDLER_SCGI
      if (x.equal(U_CONSTANT_TO_PARAM("mod_scgi")))   { _plugin = U_NEW(USCGIPlugIn);      goto next; }
#  endif
#  ifdef U_STATIC_HANDLER_FCGI
      if (x.equal(U_CONSTANT_TO_PARAM("mod_fcgi")))   { _plugin = U_NEW(UFCGIPlugIn);      goto next; }
#  endif
#  ifdef U_STATIC_HANDLER_GEOIP
      if (x.equal(U_CONSTANT_TO_PARAM("mod_geoip")))  { _plugin = U_NEW(UGeoIPPlugIn);     goto next; }
#  endif
#  ifdef U_STATIC_HANDLER_PROXY
      if (x.equal(U_CONSTANT_TO_PARAM("mod_proxy")))  { _plugin = U_NEW(UProxyPlugIn);     goto next; }
#  endif
#  ifdef U_STATIC_HANDLER_SOAP
      if (x.equal(U_CONSTANT_TO_PARAM("mod_soap")))   { _plugin = U_NEW(USoapPlugIn);      goto next; }
#  endif
#  ifdef U_STATIC_HANDLER_SSI
      if (x.equal(U_CONSTANT_TO_PARAM("mod_ssi")))    { _plugin = U_NEW(USSIPlugIn);       goto next; }
#  endif
#  ifdef U_STATIC_HANDLER_TSA
      if (x.equal(U_CONSTANT_TO_PARAM("mod_tsa")))    { _plugin = U_NEW(UTsaPlugIn);       goto next; }
#  endif
#  ifdef U_STATIC_HANDLER_NOCAT
      if (x.equal(U_CONSTANT_TO_PARAM("mod_nocat")))  { _plugin = U_NEW(UNoCatPlugIn);     goto next; }
#  endif
#  ifdef U_STATIC_HANDLER_HTTP
      if (x.equal(U_CONSTANT_TO_PARAM("mod_http")))   { _plugin = U_NEW(UHttpPlugIn);      goto next; }
#  endif

#if defined(U_STATIC_HANDLER_RPC)    || defined(U_STATIC_HANDLER_SHIB)   || defined(U_STATIC_HANDLER_ECHO)  || \
    defined(U_STATIC_HANDLER_STREAM) || defined(U_STATIC_HANDLER_SOCKET) || defined(U_STATIC_HANDLER_SCGI)  || \
    defined(U_STATIC_HANDLER_FCGI)   || defined(U_STATIC_HANDLER_GEOIP)  || defined(U_STATIC_HANDLER_PROXY) || \
    defined(U_STATIC_HANDLER_SOAP)   || defined(U_STATIC_HANDLER_SSI)    || defined(U_STATIC_HANDLER_TSA)   || \
    defined(U_STATIC_HANDLER_NOCAT)  || defined(U_STATIC_HANDLER_HTTP)
next:
#endif
      if (_plugin)
         {
         vplugin->push_back(_plugin);
         vplugin_name_static->push_back(x);

         if (isLog()) ULog::log("[%s] Link of static plugin ok\n", name);
         }
      }
}

int UServer_Base::loadPlugins(UString& plugin_dir, const UString& plugin_list, UFileConfig* cfg)
{
   U_TRACE(0, "UServer_Base::loadPlugins(%.*S,%.*S,%p)", U_STRING_TO_TRACE(plugin_dir), U_STRING_TO_TRACE(plugin_list), cfg)

#ifdef HAVE_MODULES
   if (plugin_dir.empty() == false)
      {
      // NB: we can't use relativ path because after we call chdir()...

      if (plugin_dir.first_char() == '.')
         {
         U_INTERNAL_ASSERT(plugin_dir.isNullTerminated())

         plugin_dir = UFile::getRealPath(plugin_dir.data());
         }

      UPlugIn<void*>::setPluginDirectory(plugin_dir.c_strdup());
      }
#endif

   vplugin             = U_NEW(UVector<UServerPlugIn*>(10U));
   vplugin_name        = U_NEW(UVector<UString>(10U));
   vplugin_name_static = U_NEW(UVector<UString>(20U));

   UString name;
   uint32_t i, pos;
   UServerPlugIn* _plugin;
   int result = U_PLUGIN_HANDLER_ERROR;

   // NB: we don't want to use substr() because of dependency from config var PLUGIN...

   if (plugin_list.empty()) vplugin_size = 1, vplugin_name->push(U_CONSTANT_TO_PARAM("mod_http"));
   else                     vplugin_size =    vplugin_name->split( U_STRING_TO_PARAM(plugin_list));

   /* I do know that to include code in the middle of a function is hacky and dirty,
    * but this is the best solution that I could figure out. If you have some idea to
    * clean it up, please, don't hesitate and let me know.
    */

#  include "plugin/loader.autoconf.cpp"

   for (i = 0; i < vplugin_size; ++i)
      {
      name    = vplugin_name->at(i);
      pos     = vplugin_name_static->find(name);
      _plugin = 0;

      U_INTERNAL_DUMP("i = %u pos = %u name = %.*S", i, pos, U_STRING_TO_TRACE(name))

      if (pos == U_NOT_FOUND)
         {
#     ifdef HAVE_MODULES
         _plugin = UPlugIn<UServerPlugIn*>::create(U_STRING_TO_PARAM(name));
#     endif

         if (_plugin == 0)
            {
            U_SRV_LOG("Load of plugin '%.*s' FAILED", U_STRING_TO_TRACE(name));

            goto end;
            }

         vplugin->insert(i, _plugin);
         vplugin_name_static->insert(i, name);

         if (isLog()) ULog::log("[%.*s] Load of plugin success\n", U_STRING_TO_TRACE(name));
         }
      }

   U_INTERNAL_ASSERT_MAJOR(vplugin_size, 0)
   U_INTERNAL_ASSERT_EQUALS(vplugin->size(), vplugin_size)
   U_INTERNAL_ASSERT_EQUALS(*vplugin_name, *vplugin_name_static)

   delete vplugin_name_static;

   if (cfg)
      {
      // NB: we load configuration in reverse order respect to config var PLUGIN...

      i = vplugin_size;

      do {
         name = vplugin_name->at(--i);

         if (cfg->searchForObjectStream(U_STRING_TO_PARAM(name)) == false) continue;

         cfg->clear();

         if (isLog()) mod_name->snprintf("[%.*s] ", U_STRING_TO_TRACE(name));

         _plugin = vplugin->at(i);

         result = _plugin->handlerConfig(*cfg);

         cfg->reset();

         if (result != U_PLUGIN_HANDLER_GO_ON) goto end;
         }
      while (i);
      }

   result = U_PLUGIN_HANDLER_FINISHED;

end:
   mod_name->setEmpty();

   U_RETURN(result);
}

// manage plugin handler hooks...

#define U_PLUGIN_HANDLER(xxx)                                                             \
                                                                                          \
int UServer_Base::pluginsHandler##xxx()                                                   \
{                                                                                         \
   U_TRACE(0, "UServer_Base::pluginsHandler"#xxx"()")                                     \
                                                                                          \
   U_INTERNAL_ASSERT_POINTER(vplugin)                                                     \
   U_INTERNAL_ASSERT_MAJOR(vplugin_size, 0)                                               \
                                                                                          \
   int result;                                                                            \
   uint32_t i = 0;                                                                        \
   UServerPlugIn* _plugin;                                                                \
                                                                                          \
   do {                                                                                   \
      _plugin = vplugin->at(i);                                                           \
                                                                                          \
      if (isLog()) mod_name->snprintf("[%.*s] ", U_STRING_TO_TRACE(vplugin_name->at(i))); \
                                                                                          \
      result = _plugin->handler##xxx();                                                   \
                                                                                          \
      if (result != U_PLUGIN_HANDLER_GO_ON) goto end;                                     \
      }                                                                                   \
   while (++i < vplugin_size);                                                            \
                                                                                          \
   result = U_PLUGIN_HANDLER_FINISHED;                                                    \
                                                                                          \
end:                                                                                      \
   mod_name->setEmpty();                                                                  \
                                                                                          \
   U_RETURN(result);                                                                      \
}

// Connection-wide hooks
U_PLUGIN_HANDLER(Request)
U_PLUGIN_HANDLER(Reset)

// NB: we call the various handlerXXX() in reverse order respect to config var PLUGIN...

#define U_PLUGIN_HANDLER_REVERSE(xxx)                                                     \
                                                                                          \
int UServer_Base::pluginsHandler##xxx()                                                   \
{                                                                                         \
   U_TRACE(0, "UServer_Base::pluginsHandler"#xxx"()")                                     \
                                                                                          \
   U_INTERNAL_ASSERT_POINTER(vplugin)                                                     \
   U_INTERNAL_ASSERT_MAJOR(vplugin_size, 0)                                               \
                                                                                          \
   int result;                                                                            \
   UServerPlugIn* _plugin;                                                                \
   uint32_t i = vplugin_size;                                                             \
                                                                                          \
   do {                                                                                   \
      _plugin = vplugin->at(--i);                                                         \
                                                                                          \
      if (isLog()) mod_name->snprintf("[%.*s] ", U_STRING_TO_TRACE(vplugin_name->at(i))); \
                                                                                          \
      result = _plugin->handler##xxx();                                                   \
                                                                                          \
      if (result != U_PLUGIN_HANDLER_GO_ON) goto end;                                     \
      }                                                                                   \
   while (i);                                                                             \
                                                                                          \
   result = U_PLUGIN_HANDLER_FINISHED;                                                    \
                                                                                          \
end:                                                                                      \
   mod_name->setEmpty();                                                                  \
                                                                                          \
   U_RETURN(result);                                                                      \
}

// Server-wide hooks
U_PLUGIN_HANDLER_REVERSE(Init)
U_PLUGIN_HANDLER_REVERSE(Run)  // NB: we call handlerRun()  in reverse order respect to config var PLUGIN...
U_PLUGIN_HANDLER_REVERSE(Fork) // NB: we call handlerFork() in reverse order respect to config var PLUGIN...
U_PLUGIN_HANDLER_REVERSE(Stop) // NB: we call handlerStop() in reverse order respect to config var PLUGIN...

// Connection-wide hooks
U_PLUGIN_HANDLER_REVERSE(READ) // NB: we call handlerREAD() in reverse order respect to config var PLUGIN...

void UServer_Base::init()
{
   U_TRACE(1, "UServer_Base::init()")

   U_INTERNAL_ASSERT_POINTER(socket)

#ifndef __MINGW32__
#  ifdef USE_LIBSSL
   if (bssl == false)
#  endif
      {
      if (socket->isIPC())
         {
         if (name_sock->empty() == false) UUnixSocket::setPath(name_sock->data());

         if (UUnixSocket::path == 0) U_ERROR("UNIX domain socket is not bound to a file system pathname...");

         bipc = true;
         }
      }
#endif

   if (socket->setServer(*server, port, iBackLog) == false)
      {
      if (server->empty()) *server = U_STRING_FROM_CONSTANT("*");

      U_ERROR("Run as server with local address '%.*s:%d' FAILED...", U_STRING_TO_TRACE(*server), port);
      }

   pthis->UEventFd::fd = socket->iSockDesc;

   // get name host

   host = U_NEW(UString(server->empty() ? USocketExt::getNodeName() : *server));

   if (port != U_DEFAULT_PORT)
      {
      host->push_back(':');

      (void) host->append(UStringExt::numberToString(port));
      }

   U_SRV_LOG("HOST registered as: %.*s", U_STRING_TO_TRACE(*host));

   // get IP address host (default source)

   if (IP_address->empty()          &&
           server->empty() == false &&
       u_isIPAddr(UClientImage_Base::bIPv6, U_STRING_TO_PARAM(*server)))
      {
      *IP_address = *server;
      }

#ifdef __MINGW32__
   if (IP_address->empty())
      {
      U_ERROR("On windows we need to set IP_ADDRESS on configuration file. Going down...");
      }

   socket->cLocalAddress.setHostName(*IP_address, UClientImage_Base::bIPv6);
#else
   /* The above code does NOT make a connection or send any packets (to 64.233.187.99 which is google).
    * Since UDP is a stateless protocol connect() merely makes a system call which figures out how to
    * route the packets based on the address and what interface (and therefore IP address) it should
    * bind to. Returns an array containing the family (AF_INET), local port, and local address (which
    * is what we want) of the socket.
    */

   UUDPSocket cClientSocket(UClientImage_Base::bIPv6);

   if (cClientSocket.connectServer(U_STRING_FROM_CONSTANT("8.8.8.8"), 1001))
      {
      socket->cLocalAddress = cClientSocket.cLocalAddress;

      UString ip = UString(socket->getLocalInfo());

           if ( IP_address->empty()) *IP_address = ip;
      else if (*IP_address != ip)
         {
         U_SRV_LOG("SERVER IP ADDRESS from configuration : %.*S differ from system interface: %.*S", U_STRING_TO_TRACE(*IP_address), U_STRING_TO_TRACE(ip));
         }
      }
#endif

#ifndef __MINGW32__
   if (bipc == false)
#endif
      {
      struct in_addr ia;

      if (inet_aton(IP_address->c_str(), &ia) == 0)
         {
         U_ERROR("IP_ADDRESS conversion fail. Going down...");
         }

      socket->cLocalAddress.setAddress(&ia, UClientImage_Base::bIPv6);

      public_address = (socket->cLocalAddress.isPrivate() == false);

      U_SRV_LOG("SERVER IP ADDRESS registered as: %.*s (%s)", U_STRING_TO_TRACE(*IP_address), (public_address ? "public" : "private"));

      // Instructs server to accept connections from the IP address IPADDR. A CIDR mask length can be
      // supplied optionally after a trailing slash, e.g. 192.168.0.0/24, in which case addresses that
      // match in the most significant MASK bits will be allowed. If no options are specified, all clients
      // are allowed. Unauthorized connections are rejected by closing the TCP connection immediately. A
      // warning is logged on the server but nothing is sent to the client.

      if (allow_IP->empty() == false)
         {
         vallow_IP = U_NEW(UVector<UIPAllow*>);

         if (UIPAllow::parseMask(*allow_IP, *vallow_IP) == 0)
            {
            delete vallow_IP;
                   vallow_IP = 0;
            }
         }

      if (allow_IP_prv->empty() == false)
         {
         vallow_IP_prv = U_NEW(UVector<UIPAllow*>);

         if (UIPAllow::parseMask(*allow_IP_prv, *vallow_IP_prv) == 0)
            {
            delete vallow_IP_prv;
                   vallow_IP_prv = 0;
            }
         }

      /* Let's say an application just issued a request to send a small block of data. Now, we could
       * either send the data immediately or wait for more data. Some interactive and client-server
       * applications will benefit greatly if we send the data right away. For example, when we are
       * sending a short request and awaiting a large response, the relative overhead is low compared
       * to the total amount of data transferred, and the response time could be much better if the
       * request is sent immediately. This is achieved by setting the TCP_NODELAY option on the socket,
       * which disables the Nagle algorithm.
       */

      socket->setTcpNoDelay(1U);

#  ifndef __MINGW32__
      if (flag_use_tcp_optimization)
         {
         u_need_root(false);

         U_ASSERT_EQUALS(bipc, false) // no unix socket...

      // socket->setBufferRCV(128 * 1024);
      // socket->setBufferSND(128 * 1024);

         socket->setTcpFastOpen(5U);

         /* Linux (along with some other OSs) includes a TCP_DEFER_ACCEPT option in its TCP implementation.
          * Set on a server-side listening socket, it instructs the kernel not to wait for the final ACK packet
          * and not to initiate the process until the first packet of real data has arrived. After sending the SYN/ACK,
          * the server will then wait for a data packet from a client. Now, only three packets will be sent over the
          * network, and the connection establishment delay will be significantly reduced, which is typical for HTTP.
          *
          * NB: Takes an integer value (seconds)
          */

         socket->setTcpDeferAccept(1U);

         /* Another way to prevent delays caused by sending useless packets is to use the TCP_QUICKACK option.
          * This option is different from TCP_DEFER_ACCEPT, as it can be used not only to manage the process of
          * connection establishment, but it can be used also during the normal data transfer process. In addition,
          * it can be set on either side of the client-server connection. Delaying sending of the ACK packet could
          * be useful if it is known that the user data will be sent soon, and it is better to set the ACK flag on
          * that data packet to minimize overhead. When the sender is sure that data will be immediately be sent
          * (multiple packets), the TCP_QUICKACK option can be set to 0. The default value of this option is 1 for
          * sockets in the connected state, which will be reset by the kernel to 1 immediately after the first use.
          * (This is a one-time option)
          */

         socket->setTcpQuickAck(0U);

         /* timeout_timewait parameter: Determines the time that must elapse before TCP/IP can release a closed connection
          * and reuse its resources. This interval between closure and release is known as the TIME_WAIT state or twice the
          * maximum segment lifetime (2MSL) state. During this time, reopening the connection to the client and server cost
          * less than establishing a new connection. By reducing the value of this entry, TCP/IP can release closed connections
          * faster, providing more resources for new connections. Adjust this parameter if the running application requires rapid
          * release, the creation of new connections, and a low throughput due to many connections sitting in the TIME_WAIT state.
          */

#     ifdef U_TCP_SETTING 
                                   tcp_fin_timeout = UFile::getSysParam("/proc/sys/net/ipv4/tcp_fin_timeout");
         if (tcp_fin_timeout > 30) tcp_fin_timeout = UFile::setSysParam("/proc/sys/net/ipv4/tcp_fin_timeout", 30, true);

         /* sysctl_somaxconn (SOMAXCONN: 128) specifies the maximum number of sockets in state SYN_RECV per listen socket queue.
          * At listen(2) time the backlog is adjusted to this limit if bigger then that.
          *
          * sysctl_max_syn_backlog on the other hand is dynamically adjusted, depending on the memory characteristic of the system.
          * Default is 256, 128 for small systems and up to 1024 for bigger systems.
          *
          * The system limits (somaxconn & tcp_max_syn_backlog) specify a _maximum_, the user cannot exceed this limit with listen(2).
          * The backlog argument for listen on the other  hand  specify a _minimum_
          */

         if (iBackLog >= SOMAXCONN)
            {
            int value = iBackLog * (flag_use_tcp_optimization ? 2 : 1);

            // NB: take a look at `netstat -s | grep overflowed`

            sysctl_somaxconn       = UFile::setSysParam("/proc/sys/net/core/somaxconn",           value);
            sysctl_max_syn_backlog = UFile::setSysParam("/proc/sys/net/ipv4/tcp_max_syn_backlog", value * 2);
            }
#     endif
         }

      /* sysctl_tcp_abort_on_overflow when its on, new connections are reset once the backlog is exhausted.
       */

      if (iBackLog == 1)
         {
         u_need_root(false);

         tcp_abort_on_overflow = UFile::setSysParam("/proc/sys/net/ipv4/tcp_abort_on_overflow", 1, true);
         }

      U_INTERNAL_DUMP("sysctl_somaxconn = %d tcp_abort_on_overflow = %b sysctl_max_syn_backlog = %d",
                       sysctl_somaxconn,     tcp_abort_on_overflow,     sysctl_max_syn_backlog)
#  endif
      }

   U_INTERNAL_ASSERT_EQUALS(proc,0)

   proc = U_NEW(UProcess);

   U_INTERNAL_ASSERT_POINTER(proc)

   proc->setProcessGroup();

   UClientImage_Base::init();

   USocket::accept4_flags = SOCK_CLOEXEC | SOCK_NONBLOCK;

   // init plugin modules...

   if (pluginsHandlerInit() != U_PLUGIN_HANDLER_FINISHED)
      {
      U_ERROR("Plugins initialization FAILED. Going down...");
      }

   flag_loop = true; // NB: UTimeThread loop depend on this...

   if (isPreForked() ||
       shared_data_add)
      {
      // manage shared data...

      U_INTERNAL_DUMP("shared_data_add = %u", shared_data_add)

      U_INTERNAL_ASSERT_EQUALS(ptr_shared_data, 0)

      map_size        = sizeof(shared_data) + shared_data_add;
      ptr_shared_data = (shared_data*) UFile::mmap(&map_size);

      U_INTERNAL_ASSERT_EQUALS(U_TOT_CONNECTION, 0)
      U_INTERNAL_ASSERT_DIFFERS(ptr_shared_data, MAP_FAILED)

#  if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD)
      /* NB: optimization if it is enough a time resolution of one second...

      typedef struct shared_data {
         ...........
         struct timeval _timeval;
         long last_sec[3];
         char data_1[17]; // 18/06/12 18:45:56
         char  null1[1];  // 123456789012345678901234567890
         char data_2[26]; // 04/Jun/2012:18:18:37 +0200
         char  null2[1];  // 123456789012345678901234567890
         char data_3[29]; // Wed, 20 Jun 2012 11:43:17 GMT
         char  null3[1];  // 123456789012345678901234567890
      };
      */

      U_INTERNAL_ASSERT_EQUALS(u_pthread_time, 0)

      U_NEW_ULIB_OBJECT(u_pthread_time, UTimeThread);

      U_INTERNAL_DUMP("u_pthread_time = %p", u_pthread_time)

      u_now = U_NOW; // &(ptr_shared_data->_timeval);

      (void) U_SYSCALL(gettimeofday, "%p,%p", u_now, 0);

      U_INTERNAL_ASSERT_EQUALS(ptr_shared_data->data_1,(char*)u_now+sizeof(struct timeval)+3*sizeof(long))
      U_INTERNAL_ASSERT_EQUALS(ptr_shared_data->data_2,(char*)u_now+sizeof(struct timeval)+3*sizeof(long)+17+1)
      U_INTERNAL_ASSERT_EQUALS(ptr_shared_data->data_3,(char*)u_now+sizeof(struct timeval)+3*sizeof(long)+17+1+26+1)

      ((UTimeThread*)u_pthread_time)->start();
#  endif
      }

   if (isLog())
      {
      bool shared;

#  if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD)
      shared = true;                     // NB: is always shared cause of possibility of fork() by parallelization...
#  else
      shared = (preforked_num_kids > 1); // NB: for nodog it is not shared...
#  endif

      if (shared)
         {
         ULog::setShared((ptr_shared_data ? U_LOG_DATA_SHARED : 0));

         U_SRV_LOG("Mapped %u bytes (%u KB) of shared memory for %d process", sizeof(shared_data) + shared_data_add, map_size / 1024, preforked_num_kids);
         }
      }

   // init notifier event manager...

   UNotifier::min_connection = (isClassic() == false) + (handler_inotify != 0);
   UNotifier::max_connection = (UNotifier::max_connection ? UNotifier::max_connection : 1020) + (UNotifier::num_connection = UNotifier::min_connection);

   uint32_t n = (((UNotifier::max_connection * sizeof(UClientImage_Base)) + U_PAGEMASK) & ~U_PAGEMASK) / sizeof(UClientImage_Base);

   U_INTERNAL_DUMP("n = %u UNotifier::max_connection = %u", n, UNotifier::max_connection)

   // preallocation object...

   int stack_index[U_NUM_STACK_TYPE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

   if (isLog())
      {
      stack_index[5]                                      = n; // logbuf => UString(200U)
      stack_index[U_SIZE_TO_STACK_INDEX(sizeof(UString))] = n;
      }

   stack_index[U_SIZE_TO_STACK_INDEX(sizeof(UClientImage_Base))] += n;

#if defined(USE_LIBSSL)
   if (bssl) stack_index[U_SIZE_TO_STACK_INDEX(sizeof(USSLSocket))] += n;
   else
#  endif
             stack_index[U_SIZE_TO_STACK_INDEX(sizeof(USocket))]    += n;

   for (uint32_t i = 0; i < U_NUM_STACK_TYPE; ++i)
      {
      if (stack_index[i]) UMemoryPool::allocateMemoryBlocks(i, stack_index[i] + 32);
      }

   pthis->preallocate();

#if defined(USE_LIBSSL)
   if (bssl)
      {
      if (isPreForked() &&
          USSLSession::initSessionCache(UClientImage_Base::ctx, U_STRING_FROM_CONSTANT("../db/session.ssl"), 1024 * 1024))
         {
         ((URDB*)USSLSession::db_ssl_session)->setShared(U_LOCK_SSL_SESSION);
         }

      const char* msg;

#    if !defined(OPENSSL_NO_TLS1) && defined(TLS1_2_VERSION)
      msg = "TLS 1.2";
#  elif !defined(OPENSSL_NO_TLS1) && defined(TLS1_1_VERSION)
      msg = "TLS 1.1";
#  elif !defined(OPENSSL_NO_SSL3)
      msg = "SSL 2.0/3.0";
#  elif !defined(OPENSSL_NO_SSL2)
      msg = "SSL 2.0";
#  else
      msg = "unknow";
#  endif

      U_SRV_LOG("SSL Server use %s protocol", msg);
      }
#elif defined(U_HTTP_CACHE_REQUEST) && !defined(__MINGW32__)
   accept_edge_triggered     = true;
   pthis->UEventFd::op_mask |= EPOLLET;
#endif

   socket->flags |= O_CLOEXEC;

   UNotifier::init(false);

#if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD) && defined(HAVE_EPOLL_WAIT) && !defined(USE_LIBEVENT) && defined(U_SERVER_THREAD_APPROACH_SUPPORT)
   if (preforked_num_kids == -1) goto next; 
#endif

   if (timeoutMS   != -1 &&
       isClassic() == false)
      {
      last_event = u_now->tv_sec;

      ptime = U_NEW(UTimeoutConnection);
      }

   // NB: in the classic model we don't need to be notified for request of connection (loop: accept-fork)
   // and the forked child don't accept new client, but we need anyway the event manager because the forked
   // child feel the possibly timeout for request from the new client...

        if (handler_inotify) UNotifier::insert(handler_inotify); // NB: we ask to be notified for change of file system (inotify)
   else if (isClassic())     goto next;

   /* There may not always be a connection waiting after a SIGIO is delivered or select(2) or poll(2) return
    * a readability event because the connection might have been removed by an asynchronous network error or
    * another thread before accept() is called. If this happens then the call will block waiting for the next
    * connection to arrive. To ensure that accept() never blocks, the passed socket sockfd needs to have the
    * O_NONBLOCK flag set (see socket(7))
    */

   if (preforked_num_kids) socket->flags |= O_NONBLOCK; // NB: for nodog it is blocking...

   UNotifier::insert(pthis); // NB: we ask to be notified for request of connection (=> accept)

next:
   (void) U_SYSCALL(fcntl, "%d,%d,%d", socket->iSockDesc, F_SETFL, socket->flags);
}

bool UServer_Base::addLog(UFile* _log, int flags)
{
   U_TRACE(0, "UServer_Base::addLog(%p,%d)", _log, flags)

   U_INTERNAL_ASSERT_POINTER(vlog)

   if (_log->creat(flags, PERM_FILE))
      {
      file_LOG* item = U_NEW(file_LOG);

      item->LOG   = _log;
      item->flags = flags;

      vlog->push_back(item);

      U_RETURN(true);
      }

   U_RETURN(false);
}

void UServer_Base::reopenLog()
{
   U_TRACE(0, "UServer_Base::reopenLog()")

   file_LOG* item;

   for (uint32_t i = 0, n = vlog->size(); i < n; ++i)
      {
      item = (*vlog)[i];

      item->LOG->reopen(item->flags);
      }
}

U_NO_EXPORT void UServer_Base::logMemUsage(const char* signame)
{
   U_TRACE(0, "UServer_Base::logMemUsage(%S)", signame)

   U_INTERNAL_ASSERT(isLog)

   unsigned long vsz, rss;

   u_get_memusage(&vsz, &rss);

   ULog::log("%s (Interrupt): "
             "address space usage: %.2f MBytes - "
                       "rss usage: %.2f MBytes\n", signame,
             (double)vsz / (1024.0 * 1024.0),
             (double)rss / (1024.0 * 1024.0));
}

RETSIGTYPE UServer_Base::handlerForSigHUP(int signo)
{
   U_TRACE(0, "[SIGHUP] UServer_Base::handlerForSigHUP(%d)", signo)

   U_INTERNAL_ASSERT_POINTER(pthis)

   U_INTERNAL_ASSERT(proc->parent())

   // NB: for logrotate...

   if (isLog())
      {
      logMemUsage("SIGHUP");

      ULog::reopen();
      }

   if (isOtherLog()) reopenLog();

   (void) U_SYSCALL(gettimeofday, "%p,%p", u_now, 0);

#if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD)
   if (u_pthread_time) ((UTimeThread*)u_pthread_time)->suspend();
#endif

   pthis->handlerSignal(); // manage before regenering preforked pool of children...

   // NB: we can't use UInterrupt::erase() because it restore the old action (UInterrupt::init)...
   UInterrupt::setHandlerForSignal(SIGTERM, (sighandler_t)SIG_IGN);

   sendSigTERM();

#ifdef USE_LIBEVENT
   UInterrupt::setHandlerForSignal(SIGTERM, (sighandler_t)UServer_Base::handlerForSigTERM); //  sync signal
#else
   UInterrupt::insert(             SIGTERM, (sighandler_t)UServer_Base::handlerForSigTERM); // async signal
#endif

   if (isPreForked()) U_TOT_CONNECTION = 0;

#if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD)
   if (u_pthread_time)
      {
#  ifdef DEBUG
      (void) U_SYSCALL(gettimeofday, "%p,%p", u_now, 0);

      ((UTimeThread*)u_pthread_time)->before.set(*u_now);
#  endif
      ((UTimeThread*)u_pthread_time)->watch_counter = 0;

      ((UTimeThread*)u_pthread_time)->resume();
      }
#endif
}

RETSIGTYPE UServer_Base::handlerForSigTERM(int signo)
{
   U_TRACE(0, "[SIGTERM] UServer_Base::handlerForSigTERM(%d)", signo)

   flag_loop = false;

   if (isLog()) logMemUsage("SIGTERM");

   U_INTERNAL_ASSERT_POINTER(proc)

   if (proc->parent())
      {
      // NB: we can't use UInterrupt::erase() because it restore the old action (UInterrupt::init)...
      UInterrupt::setHandlerForSignal(SIGTERM, (sighandler_t)SIG_IGN);

      sendSigTERM();

#  if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD)
      if (u_pthread_time) ((UTimeThread*)u_pthread_time)->suspend();
#  endif
      }
   else
      {
#  ifdef USE_LIBEVENT
      (void) UDispatcher::exit(0);
#  else
      UInterrupt::erase(SIGTERM); // async signal

#     if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD) && defined(HAVE_EPOLL_WAIT) && defined(U_SERVER_THREAD_APPROACH_SUPPORT)
      if (preforked_num_kids == -1) ((UThread*)UNotifier::pthread)->suspend();

#     if defined(DEBUG) && defined(HAVE_SYS_SYSCALL_H)
      if (u_plock) (void) pthread_mutex_unlock((pthread_mutex_t*)u_plock);
#     endif
#     endif
#  endif

      U_EXIT(0);
      }
}

int UServer_Base::handlerRead() // This method is called to accept a new connection on the server socket
{
   U_TRACE(1, "UServer_Base::handlerRead()")

#ifdef U_HTTP_CACHE_REQUEST
   uint32_t counter = 0;
#endif

loop:
   U_INTERNAL_ASSERT_MINOR(pClientIndex, eClientImage)

   U_INTERNAL_DUMP("----------------------------------------", 0)
   U_INTERNAL_DUMP("vClientImage[%d].last_event    = %#3D",  (pClientIndex - vClientImage),
                                                              pClientIndex->last_event)
   U_INTERNAL_DUMP("vClientImage[%u].sfd           = %d",    (pClientIndex - vClientImage),
                                                              pClientIndex->sfd)
   U_INTERNAL_DUMP("vClientImage[%u].UEventFd::fd  = %d",    (pClientIndex - vClientImage),
                                                              pClientIndex->UEventFd::fd)
   U_INTERNAL_DUMP("vClientImage[%u].socket        = %p",    (pClientIndex - vClientImage),
                                                              pClientIndex->socket)
   U_INTERNAL_DUMP("vClientImage[%d].socket->flags = %d %B", (pClientIndex - vClientImage),
                                                              pClientIndex->socket->flags,
                                                              pClientIndex->socket->flags)
   U_INTERNAL_DUMP("----------------------------------------", 0)

   if (pClientIndex->UEventFd::fd)
      {
#  ifdef U_HTTP_CACHE_REQUEST
      if (ptime && counter == 0) // NB: we can't check for idle connection if we are looping on accept()...
#  else
      if (ptime)                 // NB:          check for idle connection...
#  endif
         {
         U_gettimeofday; // NB: optimization if it is enough a time resolution of one second...

         if ((u_now->tv_sec - pClientIndex->last_event) >= ptime->UTimeVal::tv_sec)
            {
            (void) handlerTimeoutConnection(0);

            UNotifier::erase((UEventFd*)pClientIndex);

            goto try_accept;
            }
         }

#ifdef U_HTTP_CACHE_REQUEST
try_next:
#endif
      if (++pClientIndex >= eClientImage)
         {
         U_INTERNAL_ASSERT_POINTER(vClientImage)

         pClientIndex = vClientImage;
         }

      goto loop;
      }

try_accept:
   U_INTERNAL_ASSERT_EQUALS(pClientIndex->UEventFd::fd, 0)

   USocket* csocket = pClientIndex->socket;

   if (socket->acceptClient(csocket) == false)
      {
      U_INTERNAL_DUMP("flag_loop = %b", flag_loop)

      if (isLog()                    &&
          flag_loop                  && // check for SIGTERM event...
          csocket->iState != -EAGAIN && // NB: to avoid log spurious EAGAIN on accept() with epoll()...
          csocket->iState != -EINTR)    // NB: to avoid log spurious EINTR  on accept() by timer...
         {
         char buffer[4096];

         const char* msg_error = csocket->getMsgError(buffer, sizeof(buffer));

         if (msg_error) ULog::log("Accept new client failed %S\n", msg_error);
         }

      U_RETURN(U_NOTIFIER_OK);
      }

   U_INTERNAL_ASSERT(csocket->isConnected())

   pClientIndex->client_address = csocket->remoteIPAddress().getAddressString();

   U_INTERNAL_DUMP("client_address = %S", pClientIndex->client_address)

   if (vallow_IP &&
       pClientIndex->isAllowed(*vallow_IP) == false)
      {
      // Instructs server to accept connections from the IP address IPADDR. A CIDR mask length can be supplied optionally after
      // a trailing slash, e.g. 192.168.0.0/24, in which case addresses that match in the most significant MASK bits will be allowed.
      // If no options are specified, all clients are allowed. Unauthorized connections are rejected by closing the TCP connection
      // immediately. A warning is logged on the server but nothing is sent to the client.

      U_SRV_LOG("New client connected from %S, connection denied by Access Control List", pClientIndex->client_address);

      goto error;
      }

   // RFC1918 filtering (DNS rebinding countermeasure)

   if (public_address                         &&
       enable_rfc1918_filter                  &&
       csocket->remoteIPAddress().isPrivate() &&
       (vallow_IP_prv == 0 ||
        pClientIndex->isAllowed(*vallow_IP_prv) == false))
      {
      U_SRV_LOG("New client connected from %S, connection denied by RFC1918 filtering (reject request from private IP to public server address)",
                  pClientIndex->client_address); 

      goto error;
      }

#if !defined(HAVE_EPOLL_WAIT) && !defined(USE_LIBEVENT)
   if (csocket->iSockDesc >= FD_SETSIZE)
      {
      U_SRV_LOG("New client connected from %S, connection denied by FD_SETSIZE (%d)", pClientIndex->client_address, FD_SETSIZE);

      goto error;
      }
#endif

   if (++UNotifier::num_connection >= UNotifier::max_connection)
      {
       --UNotifier::num_connection;

      U_SRV_LOG("New client connected from %S, connection denied by MAX_KEEP_ALIVE (%d)",
                  pClientIndex->client_address, UNotifier::max_connection - UNotifier::min_connection);

      goto error;
      }

   // -------------------------------------------------------------------------------------------------------------------------
   // PREFORK_CHILD number of child server processes created at startup: -1 - thread approach (experimental)
   //                                                                     0 - serialize, no forking
   //                                                                     1 - classic, forking after accept client
   //                                                                    >1 - pool of process serialize plus monitoring process
   // -------------------------------------------------------------------------------------------------------------------------

   if (isPreForked())
      {
      U_TOT_CONNECTION++;

      U_INTERNAL_DUMP("tot_connection = %d", U_TOT_CONNECTION)
      }
   else if (isClassic())
      {
      U_INTERNAL_ASSERT_POINTER(proc)

      if (proc->fork() &&
          proc->parent())
         {
         int status;

         csocket->close();

         U_INTERNAL_DUMP("UNotifier::num_connection = %d", UNotifier::num_connection)

         U_SRV_LOG("Started new child (pid %d), up to %u children", proc->pid(), UNotifier::num_connection - UNotifier::min_connection);

retry:
         pid = UProcess::waitpid(-1, &status, WNOHANG); // NB: to avoid too much zombie...

         if (pid > 0)
            {
            --UNotifier::num_connection;

            U_SRV_LOG("Child (pid %d) exited with value %d (%s), down to %u children",
                              pid, status, UProcess::exitInfo(status), UNotifier::num_connection - UNotifier::min_connection);

            goto retry;
            }

         if (isLog()) ULog::log("Waiting for connection\n");

         U_RETURN(U_NOTIFIER_OK);
         }

      if (proc->child())
         {
         UNotifier::init(false);

         if (isLog()) u_unatexit(&ULog::close); // NB: needed because all instance try to close the log... (inherits from its parent)

         // NB: in the classic model we don't need to be notified for request of connection (loop: accept-fork)
         // and the forked child don't accept new client, but we need anyway the event manager because the forked
         // child feel the possibly timeout for request from the new client...

         socket->USocket::_closesocket();

         if (timeoutMS != -1) ptime = U_NEW(UTimeoutConnection);
         }
      }

   if (pClientIndex->newConnection()) // NB: newConnection() can return false only if fail about sending message welcome...
      {
#  if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD) && defined(HAVE_EPOLL_WAIT) && !defined(USE_LIBEVENT) && defined(U_SERVER_THREAD_APPROACH_SUPPORT)
      if (UNotifier::pthread)
         {
         UNotifier::insert((UEventFd*)pClientIndex);

         U_RETURN(U_NOTIFIER_OK);
         }
#  endif

      if (pClientIndex->handlerRead() == U_NOTIFIER_DELETE) pClientIndex->handlerDelete();
      else
         {
#     ifndef __MINGW32__
         // ---------------------------------------------------------------------------------------------------------------------------
         // NB: for non-keepalive connection we have chance to drop small last part of large file while sending to a slow client...
         // ---------------------------------------------------------------------------------------------------------------------------
         // struct linger lng = { 1, 0 };
         //
         // if (pClientIndex->bclose != U_YES) (void)csocket->setSockOpt(SOL_SOCKET,SO_LINGER,(const void*)&lng,sizeof(struct linger)); //send RST-ECONNRESET
         // ---------------------------------------------------------------------------------------------------------------------------
#     endif

         UNotifier::insert((UEventFd*)pClientIndex);
         }
      }

   goto check;

error:
   csocket->close();

check:
#ifdef U_HTTP_CACHE_REQUEST
   if (accept_edge_triggered && ++counter < 1000) goto try_next; // NB: we try to manage optimally a burst of new connections...
#endif

   U_RETURN(U_NOTIFIER_OK);
}

const char* UServer_Base::getNumConnection()
{
   U_TRACE(0, "UServer_Base::getNumConnection()")

   static char buffer[32];

   char* ptr = buffer;

   if (isPreForked()) *ptr++ = '(';

   uint32_t noutput = u__snprintf(ptr, sizeof(buffer), "%u", UNotifier::num_connection - UNotifier::min_connection - 1);

   if (isPreForked())
      {
      U_INTERNAL_ASSERT_MAJOR(U_TOT_CONNECTION, 0)

      (void) u__snprintf(ptr+noutput, sizeof(buffer), "/%u)", U_TOT_CONNECTION - 1);
      }

   U_RETURN(buffer);
}

void UServer_Base::handlerCloseConnection(UClientImage_Base* ptr)
{
   U_TRACE(0, "UServer_Base::handlerCloseConnection(%p)", ptr)

   U_INTERNAL_ASSERT_POINTER(ptr)

   if (isLog())
      {
      U_INTERNAL_DUMP("UEventFd::fd = %d logbuf = %.*S", ptr->UEventFd::fd, U_STRING_TO_TRACE(*(ptr->logbuf)))

      U_INTERNAL_ASSERT_MAJOR(ptr->UEventFd::fd, 0)
      U_ASSERT_EQUALS(ptr->UEventFd::fd, ptr->logbuf->strtol())

      U_SRV_LOG("%s close connection from %.*s, %s clients still connected",
                     (ptr->socket->isOpen() ? "Server" : "Client"), U_STRING_TO_TRACE(*(ptr->logbuf)), getNumConnection());

      ptr->logbuf->setEmpty();
      }

   if (isPreForked())
      {
      U_TOT_CONNECTION--;

      U_INTERNAL_DUMP("tot_connection = %d", U_TOT_CONNECTION)
      }

   if (isClassic()) U_EXIT(0);
}

bool UServer_Base::handlerTimeoutConnection(void* cimg)
{
   U_TRACE(0, "UServer_Base::handlerTimeoutConnection(%p)", cimg)

   U_INTERNAL_ASSERT_POINTER(pthis)
   U_INTERNAL_ASSERT_POINTER(ptime)
   U_INTERNAL_ASSERT_DIFFERS(timeoutMS, -1)

   bool from_handlerTime;

   if (cimg)
      {
      U_INTERNAL_DUMP("pthis = %p handler_inotify = %p ", pthis, handler_inotify)

      if (cimg == pthis ||
          cimg == handler_inotify)
         {
         U_RETURN(false);
         }

      from_handlerTime = true;
      }
   else
      {
      cimg             = pClientIndex;
      from_handlerTime = false;
      }

   (void) ((UClientImage_Base*)cimg)->handlerError(USocket::TIMEOUT | USocket::BROKEN); // NB: this call set also pClientImage...

   U_INTERNAL_ASSERT_EQUALS(pClientImage, cimg) // NB: U_SRV_LOG_WITH_ADDR macro depend on pClientImage...

   if (isLog())
      {
      if (from_handlerTime)
         {
         U_SRV_LOG_WITH_ADDR("handlerTime: client connected didn't send any request in %u secs (timeout), close connection",
                              ptime->UTimeVal::tv_sec);
         }
      else
         {
         U_SRV_LOG_WITH_ADDR("handlerTimeoutConnection: client connected didn't send any request in %u secs (timeout), close connection",
                              u_now->tv_sec - ((UClientImage_Base*)cimg)->last_event);
         }
      }

   U_RETURN(true);
}

// define method VIRTUAL of class UEventTime

int UServer_Base::UTimeoutConnection::handlerTime()
{
   U_TRACE(0, "UServer_Base::UTimeoutConnection::handlerTime()")

   U_INTERNAL_DUMP("UNotifier::num_connection = %d", UNotifier::num_connection)

   if (UNotifier::num_connection > UNotifier::min_connection)
      {
      // there are idle connection... (timeout)

#  ifdef DEBUG
      if (isLog())
         {
         U_gettimeofday; // NB: optimization if it is enough a time resolution of one second...

         long delta = u_now->tv_sec - last_event - ptime->UTimeVal::tv_sec;

         if (delta >=  1 ||
             delta <= -1)
            {
            U_SRV_LOG("handlerTime: server delta timeout exceed 1 sec: diff %ld sec", delta);
            }
         }
#  endif

      UNotifier::callForAllEntryDynamic(handlerTimeoutConnection);
      }

   // ---------------
   // return value:
   // ---------------
   // -1 - normal
   //  0 - monitoring
   // ---------------

   U_RETURN(0);
}

void UServer_Base::runLoop(const char* user)
{
   U_TRACE(0, "UServer_Base::runLoop(%S)", user)

   if (user)
      {
#  ifndef __MINGW32__
      if (u_runAsUser(user, false) == false)
         {
         U_ERROR("set user %S context failed...", user);
         }

      U_SRV_LOG("Server run with user %S permission", user);
#  endif
      }

   if (isLog()) ULog::log("Waiting for connection\n");

#if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD) && defined(HAVE_EPOLL_WAIT) && !defined(USE_LIBEVENT) && defined(U_SERVER_THREAD_APPROACH_SUPPORT)
   if (preforked_num_kids == -1) ((UThread*)(UNotifier::pthread = U_NEW(UClientThread)))->start();
#endif

   U_INTERNAL_DUMP("UNotifier::min_connection = %d", UNotifier::min_connection)

   while (flag_loop)
      {
      if (UInterrupt::event_signal_pending)
         {
         UInterrupt::callHandlerSignal();

         continue;
         }

#  if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD) && defined(HAVE_EPOLL_WAIT) && !defined(USE_LIBEVENT) && defined(U_SERVER_THREAD_APPROACH_SUPPORT)
      if (preforked_num_kids == -1) goto next;
#  endif

      if (isPreForked())
         {
#     ifdef DEBUG
         if (isLog())
            {
            U_gettimeofday; // NB: optimization if it is enough a time resolution of one second...

            last_event = u_now->tv_sec;
            }
#     endif

         UNotifier::waitForEvent(ptime);

         if (UNotifier::empty()) return; // NB: no more event manager registered, child go to exit...
         }
      else
         {
         U_INTERNAL_ASSERT_RANGE(0,preforked_num_kids,1)

         // NB: in the classic model we don't need to be notified for request of connection (loop: accept-fork)
         // and the forked child don't accept new client, but we need anyway the event manager because the forked
         // child feel the possibly timeout for request from the new client...

         U_INTERNAL_DUMP("UNotifier::num_connection = %d", UNotifier::num_connection)

         if (handler_inotify ||
             UNotifier::num_connection > UNotifier::min_connection)
            {
            // NB: if we have already some client we can't go directly on accept() and block on it...

#        ifdef DEBUG
            if (isLog())
               {
               U_gettimeofday; // NB: optimization if it is enough a time resolution of one second...

               last_event = u_now->tv_sec;
               }
#        endif

            UNotifier::waitForEvent(ptime);

            continue;
            }

#if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD) && defined(HAVE_EPOLL_WAIT) && !defined(USE_LIBEVENT) && defined(U_SERVER_THREAD_APPROACH_SUPPORT)
next:
#endif
         U_INTERNAL_ASSERT((socket->flags & O_NONBLOCK) == 0)

         (void) pthis->handlerRead();
         }
      }
}

void UServer_Base::run()
{
   U_TRACE(1, "UServer_Base::run()")

   U_INTERNAL_ASSERT_POINTER(pthis)

   init();

   if (pluginsHandlerRun() != U_PLUGIN_HANDLER_FINISHED)
      {
      U_ERROR("Plugins running FAILED. Going down...");
      }

   if (u_start_time     == 0 &&
       u_setStartTime() == false)
      {
      U_SRV_LOG("System date not updated. Going down...");
      }

   bpluginsHandlerReset   = false; // default is NOT call...
   bpluginsHandlerRequest = true;  // default is     call...

   UInterrupt::syscall_restart                 = false;
   UInterrupt::exit_loop_wait_event_for_signal = true;

#ifdef USE_LIBEVENT
   UInterrupt::setHandlerForSignal( SIGHUP, (sighandler_t)UServer_Base::handlerForSigHUP);  //  sync signal
   UInterrupt::setHandlerForSignal(SIGTERM, (sighandler_t)UServer_Base::handlerForSigTERM); //  sync signal
#else
   UInterrupt::insert(              SIGHUP, (sighandler_t)UServer_Base::handlerForSigHUP);  // async signal
   UInterrupt::insert(             SIGTERM, (sighandler_t)UServer_Base::handlerForSigTERM); // async signal
#endif

   const char* user = (as_user->empty() ? 0 : as_user->data());

   // -------------------------------------------------------------------------------------------------------------------------
   // PREFORK_CHILD number of child server processes created at startup: -1 - thread approach (experimental)
   //                                                                     0 - serialize, no forking
   //                                                                     1 - classic, forking after accept client
   //                                                                    >1 - pool of process serialize plus monitoring process
   // -------------------------------------------------------------------------------------------------------------------------

   if (monitoring_process == false) runLoop(user);
   else
      {
      /**
      * Main loop for the parent process with the new preforked implementation.
      * The parent is just responsible for keeping a pool of children and they accept connections themselves...
      **/

      pid_t pid_to_wait;
      int status, i = 0, nkids;
      UTimeVal to_sleep(0L, 500 * 1000L);
      bool baffinity = (preforked_num_kids <= u_get_num_cpu() && u_num_cpu > 1);

      if (isPreForked())
         {
         pid_to_wait = -1;
         nkids       = preforked_num_kids;
         }
      else
         {
         nkids       = 1;
         }

      U_INTERNAL_DUMP("nkids = %d baffinity = %b", nkids, baffinity)

      while (flag_loop)
         {
         u_need_root(false);

         while (i < nkids)
            {
            if (proc->fork() &&
                proc->parent())
               {
               ++i;

               U_INTERNAL_DUMP("up to %u children", i)
               U_INTERNAL_DUMP("UNotifier::num_connection = %d", UNotifier::num_connection)

               pid = proc->pid();

               cpu_set_t cpuset;

               if (baffinity) u_bind2cpu(pid, i); // Pin the process to a particular core...

               CPU_ZERO(&cpuset);

#           ifdef HAVE_SCHED_GETAFFINITY
               (void) U_SYSCALL(sched_getaffinity, "%d,%d,%p", pid, sizeof(cpuset), &cpuset);
#           endif

               U_INTERNAL_DUMP("cpuset = %ld %B", CPUSET_BITS(&cpuset)[0], CPUSET_BITS(&cpuset)[0])

               U_SRV_LOG("Started new child (pid %d), up to %u children, affinity mask: %x", pid, i, CPUSET_BITS(&cpuset)[0]);

               if (set_realtime_priority &&
                   u_switch_to_realtime_priority(pid) == false)
                  {
                  U_WARNING("Cannot set posix realtime scheduling policy");
                  }

               if (isPreForked() == false) pid_to_wait = pid;

#           if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD)
               if (u_pthread_time)
                  {
#              ifdef DEBUG
                  (void) U_SYSCALL(gettimeofday, "%p,%p", u_now, 0);

                  ((UTimeThread*)u_pthread_time)->before.set(*u_now);
#              endif
                  ((UTimeThread*)u_pthread_time)->watch_counter = 0;
                  }
#           endif
               }

            if (proc->child())
               {
               U_INTERNAL_DUMP("UNotifier::num_connection = %d", UNotifier::num_connection)

               if (pluginsHandlerFork() != U_PLUGIN_HANDLER_FINISHED)
                  {
                  U_ERROR("Plugins forking FAILED. Going down...");
                  }

               if (user     == 0 &&
                   iBackLog != 1 &&
                   flag_use_tcp_optimization == false)
                  {
                  /* don't need these anymore. Good security policy says we get rid of them */

                  u_never_need_root();
                  u_never_need_group();
                  }

               UNotifier::init(true);

               if (isLog()) u_unatexit(&ULog::close); // NB: needed because all instance try to close the log... (inherits from its parent)

               // NB: we can't use UInterrupt::erase() because it restore the old action (UInterrupt::init)...

               UInterrupt::setHandlerForSignal(SIGHUP, (sighandler_t)SIG_IGN);

               runLoop(user);

               // NB: it is needed because only the parent process must close the log...

               if (isLog()) log = 0;

               return;
               }

            /* Don't start them too quickly, or we might overwhelm a machine that's having trouble. */

            to_sleep.nanosleep();
            }

         /* wait for any children to exit, and then start some more */

         u_dont_need_root();

         pid = UProcess::waitpid(pid_to_wait, &status, 0);

         if (pid > 0 &&
             flag_loop) // check for SIGTERM event...
            {
            --i;

            baffinity = false;

            U_INTERNAL_DUMP("down to %u children", i)

            U_SRV_LOG("Child (pid %d) exited with value %d (%s), down to %u children", pid, status, UProcess::exitInfo(status), i);
            }

         /* Another little safety brake here: since children should not exit
          * too quickly, pausing before starting them should be harmless. */

         if (USemaphore::checkForDeadLock(to_sleep) == false) to_sleep.nanosleep();
         }

      U_INTERNAL_ASSERT(proc->parent())

      (void) proc->waitAll();
      }

   if (pluginsHandlerStop() != U_PLUGIN_HANDLER_FINISHED)
      {
      U_WARNING("Plugins stop FAILED...");
      }

#ifdef DEBUG
   pthis->deallocate();
#endif
}

// it creates a copy of itself, return true if parent...

bool UServer_Base::parallelization()
{
   U_TRACE(0, "UServer_Base::parallelization()")

   U_INTERNAL_ASSERT_POINTER(proc)

   U_INTERNAL_ASSERT_EQUALS(UClientImage_Base::isPipeline(), false)

   if (proc->parent()) proc->wait(); // NB: to avoid fork bomb...

   if (proc->fork() &&
       proc->parent())
      {
      UClientImage_Base::write_off = true;

      U_RETURN(true);
      }

   if (proc->child())
      {
      flag_loop = false;

      if (isLog())
         {
         u_unatexit(&ULog::close); // NB: needed because all instance try to close the log... (inherits from its parent)

         if (ULog::isMemoryMapped() &&
             ULog::isShared() == false)
            {
            log = 0; // NB: we need locking to write on the log...
            }
         }
      }

   U_RETURN(false);
}

UCommand* UServer_Base::loadConfigCommand(UFileConfig& cfg)
{
   U_TRACE(0, "UServer_Base::loadConfigCommand(%p)", &cfg)

   U_ASSERT_EQUALS(cfg.empty(), false)

   UCommand* cmd   = 0;
   UString command = cfg[*str_COMMAND];

   if (command.empty() == false)
      {
      if (U_ENDS_WITH(command, ".sh"))
         {
         uint32_t len = command.size();

         UString buffer(U_CONSTANT_SIZE(U_PATH_SHELL) + 1 + len);

         buffer.snprintf("%s %.*s", U_PATH_SHELL, len, command.data());

         command = buffer;
         }

      cmd = U_NEW(UCommand(command));

      UString environment = cfg[*str_ENVIRONMENT];

      if (environment.empty() == false)
         {
         environment = UStringExt::prepareForEnvironmentVar(environment); 

         cmd->setEnvironment(&environment);
         }
      }

   U_RETURN_POINTER(cmd,UCommand);
}

void UServer_Base::logCommandMsgError(const char* cmd, bool balways)
{
   U_TRACE(0, "UServer_Base::logCommandMsgError(%S,%b)", cmd, balways)

   if (isLog())
      {
      if (UCommand::setMsgError(cmd, !balways) || balways) ULog::log("%.*s%.*s\n", U_STRING_TO_TRACE(*mod_name), u_buffer_len, u_buffer);

      u_buffer_len = 0;
      }
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UServer_Base::dump(bool reset) const
{
   U_CHECK_MEMORY

   *UObjectIO::os << "port                      " << port                       << '\n'
                  << "iBackLog                  " << iBackLog                   << '\n'
                  << "map_size                  " << map_size                   << '\n'
                  << "flag_loop                 " << flag_loop                  << '\n'
                  << "timeoutMS                 " << timeoutMS                  << '\n'
                  << "last_event                " << last_event                 << '\n'
                  << "verify_mode               " << verify_mode                << '\n'
                  << "cgi_timeout               " << cgi_timeout                << '\n'
                  << "verify_mode               " << verify_mode                << '\n'
                  << "shared_data_add           " << shared_data_add            << '\n'
                  << "ptr_shared_data           " << (void*)ptr_shared_data     << '\n'
                  << "preforked_num_kids        " << preforked_num_kids         << '\n'
                  << "flag_use_tcp_optimization " << flag_use_tcp_optimization  << '\n'
                  << "log           (ULog       " << (void*)log                 << ")\n"
                  << "socket        (USocket    " << (void*)socket              << ")\n"
                  << "host          (UString    " << (void*)host                << ")\n"
                  << "server        (UString    " << (void*)server              << ")\n"
                  << "log_file      (UString    " << (void*)log_file            << ")\n"
                  << "dh_file       (UString    " << (void*)dh_file             << ")\n"
                  << "ca_file       (UString    " << (void*)ca_file             << ")\n"
                  << "ca_path       (UString    " << (void*)ca_path             << ")\n"
                  << "mod_name      (UString    " << (void*)mod_name            << ")\n"
                  << "allow_IP      (UString    " << (void*)allow_IP            << ")\n"
                  << "allow_IP_prv  (UString    " << (void*)allow_IP_prv        << ")\n"
                  << "key_file      (UString    " << (void*)key_file            << ")\n"
                  << "password      (UString    " << (void*)password            << ")\n"
                  << "cert_file     (UString    " << (void*)cert_file           << ")\n"
                  << "name_sock     (UString    " << (void*)name_sock           << ")\n"
                  << "IP_address    (UString    " << (void*)IP_address          << ")\n"
                  << "document_root (UString    " << (void*)document_root       << ")\n"
                  << "proc          (UProcess   " << (void*)proc                << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}
#endif
