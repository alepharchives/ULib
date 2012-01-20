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

#include <ulib/command.h>
#include <ulib/notifier.h>
#include <ulib/file_config.h>
#include <ulib/net/udpsocket.h>
#include <ulib/utility/uhttp.h>
#include <ulib/utility/services.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/string_ext.h>

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
#  include <ulib/net/unixsocket.h>
#  define U_TCP_SETTING yes
#endif

#define U_DEFAULT_PORT   80
#define U_TOT_CONNECTION ptr_shared_data->tot_connection

int                        UServer_Base::sfd;
int                        UServer_Base::port;
int                        UServer_Base::bclose;
int                        UServer_Base::iBackLog = SOMAXCONN;
int                        UServer_Base::timeoutMS = -1;
int                        UServer_Base::cgi_timeout;
int                        UServer_Base::verify_mode;
int                        UServer_Base::preforked_num_kids;
bool                       UServer_Base::bssl;
bool                       UServer_Base::flag_loop;
bool                       UServer_Base::bpluginsHandlerReset;
bool                       UServer_Base::accept_edge_triggered;
bool                       UServer_Base::set_realtime_priority;
bool                       UServer_Base::flag_use_tcp_optimization;
char                       UServer_Base::mod_name[20];
ULog*                      UServer_Base::log;
time_t                     UServer_Base::expire;
uint32_t                   UServer_Base::start;
uint32_t                   UServer_Base::count;
uint32_t                   UServer_Base::shared_data_add;
UString*                   UServer_Base::host;
UString*                   UServer_Base::senvironment;
USocket*                   UServer_Base::socket;
UProcess*                  UServer_Base::proc;
UEventFd*                  UServer_Base::handler_inotify;
UEventTime*                UServer_Base::ptime;
UServer_Base*              UServer_Base::pthis;
UVector<UString>*          UServer_Base::vplugin_name;
UClientImage_Base*         UServer_Base::pindex;
UClientImage_Base*         UServer_Base::vClientImage;
UClientImage_Base*         UServer_Base::pClientImage;
UClientImage_Base*         UServer_Base::eClientImage;
UVector<UIPAllow*>*        UServer_Base::vallow_IP;
UVector<UServerPlugIn*>*   UServer_Base::vplugin;
UServer_Base::shared_data* UServer_Base::ptr_shared_data;

const UString* UServer_Base::str_USE_IPV6;
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

#ifdef HAVE_PTHREAD_H
#  include "ulib/thread.h"

class UTimeThread : public UThread {
public:

   UTimeThread() : UThread(false, false) {}

   virtual void run()
      {
      int counter        = 1;
      struct timespec ts = {  1L, 0L };

      while (true)
         {
         if (--counter) u_now->tv_sec += 1L;
         else
            {
            counter = 30;

            (void) gettimeofday(u_now, 0);
            }

         (void) nanosleep(&ts, 0);
         }
      }
};

class UClientThread : public UThread {
public:

   UClientThread() : UThread(false, false) {}

   virtual void run()
      {
      U_TRACE(0, "UClientThread::run()")

      while (UServer_Base::flag_loop) (void) UNotifier::waitForEvent(UServer_Base::ptime);
      }
};
#endif

#ifndef __MINGW32__
static int sysctl_somaxconn, tcp_abort_on_overflow, sysctl_max_syn_backlog, tcp_fin_timeout;
#endif

void UServer_Base::str_allocate()
{
   U_TRACE(0, "UServer_Base::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_USE_IPV6,0)
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

   static ustringrep stringrep_storage[] = {
   { U_STRINGREP_FROM_CONSTANT("USE_IPV6") },
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
   { U_STRINGREP_FROM_CONSTANT("SET_REALTIME_PRIORITY") }
   };

   U_NEW_ULIB_OBJECT(str_USE_IPV6,              U_STRING_FROM_STRINGREP_STORAGE(0));
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
   U_NEW_ULIB_OBJECT(str_SOCKET_NAME,           U_STRING_FROM_STRINGREP_STORAGE(18));
   U_NEW_ULIB_OBJECT(str_DOCUMENT_ROOT,         U_STRING_FROM_STRINGREP_STORAGE(19));
   U_NEW_ULIB_OBJECT(str_PLUGIN,                U_STRING_FROM_STRINGREP_STORAGE(20));
   U_NEW_ULIB_OBJECT(str_PLUGIN_DIR,            U_STRING_FROM_STRINGREP_STORAGE(21));
   U_NEW_ULIB_OBJECT(str_REQ_TIMEOUT,           U_STRING_FROM_STRINGREP_STORAGE(22));
   U_NEW_ULIB_OBJECT(str_CGI_TIMEOUT,           U_STRING_FROM_STRINGREP_STORAGE(23));
   U_NEW_ULIB_OBJECT(str_VIRTUAL_HOST,          U_STRING_FROM_STRINGREP_STORAGE(24));
   U_NEW_ULIB_OBJECT(str_DIGEST_AUTHENTICATION, U_STRING_FROM_STRINGREP_STORAGE(25));
   U_NEW_ULIB_OBJECT(str_URI,                   U_STRING_FROM_STRINGREP_STORAGE(26));
   U_NEW_ULIB_OBJECT(str_HOST,                  U_STRING_FROM_STRINGREP_STORAGE(27));
   U_NEW_ULIB_OBJECT(str_USER,                  U_STRING_FROM_STRINGREP_STORAGE(28));
   U_NEW_ULIB_OBJECT(str_SERVER,                U_STRING_FROM_STRINGREP_STORAGE(29));
   U_NEW_ULIB_OBJECT(str_METHOD_NAME,           U_STRING_FROM_STRINGREP_STORAGE(30));
   U_NEW_ULIB_OBJECT(str_RESPONSE_TYPE,         U_STRING_FROM_STRINGREP_STORAGE(31));
   U_NEW_ULIB_OBJECT(str_IP_ADDRESS,            U_STRING_FROM_STRINGREP_STORAGE(32));
   U_NEW_ULIB_OBJECT(str_MAX_KEEP_ALIVE,        U_STRING_FROM_STRINGREP_STORAGE(33));
   U_NEW_ULIB_OBJECT(str_PID_FILE,              U_STRING_FROM_STRINGREP_STORAGE(34));
   U_NEW_ULIB_OBJECT(str_USE_TCP_OPTIMIZATION,  U_STRING_FROM_STRINGREP_STORAGE(35));
   U_NEW_ULIB_OBJECT(str_LISTEN_BACKLOG,        U_STRING_FROM_STRINGREP_STORAGE(36));
   U_NEW_ULIB_OBJECT(str_SET_REALTIME_PRIORITY, U_STRING_FROM_STRINGREP_STORAGE(37));
}

UServer_Base::UServer_Base(UFileConfig* cfg)
{
   U_TRACE_REGISTER_OBJECT(0, UServer_Base, "%p", cfg)

   U_INTERNAL_ASSERT_EQUALS(pthis,0)
   U_INTERNAL_ASSERT_EQUALS(senvironment,0)

   if (str_USE_IPV6 == 0) str_allocate();

   port         = U_DEFAULT_PORT;
   pthis        = this;
   senvironment = U_NEW(UString(U_CAPACITY));

   U_INTERNAL_DUMP("u_seed_hash = %u", u_seed_hash)

   u_init_ulib_hostname();

   U_INTERNAL_DUMP("u_hostname(%u) = %.*S", u_hostname_len, u_hostname_len, u_hostname)

   u_init_ulib_username();

   U_INTERNAL_DUMP("u_user_name(%u) = %.*S", u_user_name_len, u_user_name_len, u_user_name)

   u_init_security();

#if !defined(HAVE_EPOLL_WAIT) && !defined(HAVE_LIBEVENT) && defined(DEBUG)
   U_INTERNAL_DUMP("SOMAXCONN = %d FD_SETSIZE = %d", SOMAXCONN, FD_SETSIZE)
#endif

   if (cfg) loadConfigParam(*cfg);
}

UServer_Base::~UServer_Base()
{
   U_TRACE_UNREGISTER_OBJECT(0, UServer_Base)

   UClientImage_Base::pipeline = false;

   UClientImage_Base::initAfterGenericRead();

   if (vplugin)
      {
      delete vplugin;
      delete vplugin_name;

#  ifdef HAVE_MODULES
      UPlugIn<void*>::clear();
#  endif
      }

   U_INTERNAL_ASSERT_EQUALS(handler_inotify,0)

   UClientImage_Base::clear(); // NB: must be here, after UHTTP::ctor()...

   UEventFd::fd = 0; // NB: to avoid delete itself...

   UNotifier::clear(preforked_num_kids == -1);

   if (vClientImage)
      {
      if (isClassic()) UNotifier::num_connection = UNotifier::min_connection;

      delete[] vClientImage;
      }

   U_INTERNAL_ASSERT_POINTER(senvironment)

   delete senvironment;

   if (log)        delete log;
   if (host)       delete host;
   if (ptime)      delete ptime;
   if (vallow_IP)  delete vallow_IP;

   U_INTERNAL_ASSERT_POINTER(socket)

   delete socket;

#ifndef __MINGW32__
   if (as_user.empty() &&
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

   if (ptr_shared_data) UFile::munmap(ptr_shared_data, sizeof(shared_data) + shared_data_add);

#ifdef HAVE_SSL
   if (UServices::CApath) delete UServices::CApath;
#endif
}

void UServer_Base::loadConfigParam(UFileConfig& cfg)
{
   U_TRACE(0, "UServer_Base::loadConfigParam(%p)", &cfg)

   U_ASSERT_EQUALS(cfg.empty(), false)

   // --------------------------------------------------------------------------------------------------------------------------------------
   // userver - configuration parameters
   // --------------------------------------------------------------------------------------------------------------------------------------
   // USE_IPV6      flag indicating the use of ipv6
   // SERVER        host name or ip address for the listening socket
   // PORT          port number             for the listening socket
   // SOCKET_NAME   file name               for the listening socket
   // IP_ADDRESS    public ip address of host for the interface connected to the Internet (autodetected if not specified)
   // ALLOWED_IP    list of comma separated client address for IP-based access control (IPADDR[/MASK])
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

   server                     = cfg[*str_SERVER];
   as_user                    = cfg[*str_RUN_AS_USER],
   log_file                   = cfg[*str_LOG_FILE];
   allow_IP                   = cfg[*str_ALLOWED_IP];
   name_sock                  = cfg[*str_SOCKET_NAME];
   IP_address                 = cfg[*str_IP_ADDRESS];

   port                       = cfg.readLong(*str_PORT, U_DEFAULT_PORT);
   iBackLog                   = cfg.readLong(*str_LISTEN_BACKLOG, SOMAXCONN);
   timeoutMS                  = cfg.readLong(*str_REQ_TIMEOUT);
   cgi_timeout                = cfg.readLong(*str_CGI_TIMEOUT);
   set_realtime_priority      = cfg.readBoolean(*str_SET_REALTIME_PRIORITY);
   flag_use_tcp_optimization  = cfg.readBoolean(*str_USE_TCP_OPTIMIZATION);
   UNotifier::max_connection  = cfg.readLong(*str_MAX_KEEP_ALIVE);
   u_printf_string_max_length = cfg.readLong(*str_LOG_MSG_SIZE);

#ifdef HAVE_IPV6
   UClientImage_Base::bIPv6   = cfg.readBoolean(*str_USE_IPV6);
#endif

   if (timeoutMS) timeoutMS *= 1000;
   else           timeoutMS  = -1;

   if (cgi_timeout) UCommand::setTimeout(cgi_timeout);

   UClientImage_Base::setMsgWelcome(cfg[*str_MSG_WELCOME]);

#ifdef HAVE_SSL
   dh_file     = cfg[*str_DH_FILE];
   ca_file     = cfg[*str_CA_FILE];
   ca_path     = cfg[*str_CA_PATH];
   key_file    = cfg[*str_KEY_FILE];
   password    = cfg[*str_PASSWORD];
   cert_file   = cfg[*str_CERT_FILE];

   verify_mode = cfg.readLong(*str_VERIFY_MODE);
#endif

#ifndef __MINGW32__
   UString x = cfg[*str_PREFORK_CHILD];

   if (x.empty() == false) preforked_num_kids = x.strtol();
   else
      {
      preforked_num_kids = u_get_num_cpu();

      U_INTERNAL_DUMP("num_cpu = %d", preforked_num_kids)

      if (preforked_num_kids < 2) preforked_num_kids = 2;
      }

   if (isPreForked())
      {
      // manage shared data...

      U_INTERNAL_ASSERT_EQUALS(ptr_shared_data,0)

      U_INTERNAL_DUMP("shared_data_add = %u", shared_data_add)

      ptr_shared_data = (shared_data*) UFile::mmap(sizeof(shared_data) + shared_data_add);

      U_INTERNAL_ASSERT_EQUALS(U_TOT_CONNECTION,0)
      U_INTERNAL_ASSERT_DIFFERS(ptr_shared_data, MAP_FAILED)

      u_now = &(ptr_shared_data->_timeval);

#  if defined(HAVE_PTHREAD_H) && defined(U_HTTP_CACHE_REQUEST)
      ((UTimeThread*)(u_pthread_time = U_NEW(UTimeThread)))->start();
#  endif
      }
#endif

   // write pid on file...

   UString pid_file = cfg[*str_PID_FILE];

   if (pid_file.empty() == false) (void) UFile::writeTo(pid_file, UString(u_pid_str, u_pid_str_len));

   // open log

   if (log_file.empty() == false)
      {
#  if defined(HAVE_PTHREAD_H)
      if (u_pthread_time == 0) ((UTimeThread*)(u_pthread_time = U_NEW(UTimeThread)))->start();
#  endif

      openLog(log_file, cfg.readLong(*str_LOG_FILE_SZ));
      }

   // DOCUMENT_ROOT: The directory out of which you will serve your documents

   document_root = cfg[*str_DOCUMENT_ROOT];

   if (document_root.empty() ||
       document_root.equal(U_CONSTANT_TO_PARAM(".")))
      {
      (void) document_root.replace(u_cwd, u_cwd_len);
      }
   else
      {
      U_INTERNAL_ASSERT(document_root.isNullTerminated())

      (void) u_canonicalize_pathname(document_root.data());

      U_INTERNAL_DUMP("document_root = %S", document_root.data())
      }

   if (document_root.first_char() == '.') document_root = UFile::getRealPath(document_root.data());

   if (UFile::chdir(document_root.data(), false) == false)
      {
      U_ERROR("chdir to working directory (DOCUMENT_ROOT) %S FAILED. Going down...", document_root.data());
      }

   U_SRV_LOG("working directory (DOCUMENT_ROOT) changed to %S", u_cwd);

   // load plugin modules and call server-wide hooks handlerConfig()...

   UString plugin_dir = cfg[*str_PLUGIN_DIR];

   if (loadPlugins(plugin_dir, cfg[*str_PLUGIN], &cfg) == U_PLUGIN_HANDLER_ERROR)
      {
      U_ERROR("plugins load FAILED. Going down...");
      }

   cfg.clear();
   cfg.deallocate();
}

// load plugin modules and call server-wide hooks handlerConfig()...

U_NO_EXPORT void UServer_Base::loadStaticLinkedModules(const char* name)
{
   U_TRACE(0, "UServer_Base::loadStaticLinkedModules(%S)", name)

   UString x(name);

   vplugin_name->push_back(x);

   UServerPlugIn* _plugin = 0;

#ifdef U_STATIC_HANDLER_RPC
   if (x.equal(U_CONSTANT_TO_PARAM("mod_rpc")))    { _plugin = U_NEW(URpcPlugIn); goto next; }
#endif
#ifdef U_STATIC_HANDLER_SHIB
   if (x.equal(U_CONSTANT_TO_PARAM("mod_shib")))   { _plugin = U_NEW(UShibPlugIn); goto next; }
#endif
#ifdef U_STATIC_HANDLER_ECHO
   if (x.equal(U_CONSTANT_TO_PARAM("mod_echo")))   { _plugin = U_NEW(UEchoPlugIn); goto next; }
#endif
#ifdef U_STATIC_HANDLER_STREAM
   if (x.equal(U_CONSTANT_TO_PARAM("mod_stream"))) { _plugin = U_NEW(UStreamPlugIn); goto next; }
#endif
#ifdef U_STATIC_HANDLER_NOCAT
   if (x.equal(U_CONSTANT_TO_PARAM("mod_nocat")))  { _plugin = U_NEW(UNoCatPlugIn); goto next; }
#endif
#ifdef U_STATIC_HANDLER_SOCKET
   if (x.equal(U_CONSTANT_TO_PARAM("mod_socket"))) { _plugin = U_NEW(UWebSocketPlugIn); goto next; }
#endif
#ifdef U_STATIC_HANDLER_SCGI
   if (x.equal(U_CONSTANT_TO_PARAM("mod_scgi")))   { _plugin = U_NEW(USCGIPlugIn); goto next; }
#endif
#ifdef U_STATIC_HANDLER_FCGI
   if (x.equal(U_CONSTANT_TO_PARAM("mod_fcgi")))   { _plugin = U_NEW(UFCGIPlugIn); goto next; }
#endif
#ifdef U_STATIC_HANDLER_GEOIP
   if (x.equal(U_CONSTANT_TO_PARAM("mod_geoip")))  { _plugin = U_NEW(UGeoIPPlugIn); goto next; }
#endif
#ifdef U_STATIC_HANDLER_PROXY
   if (x.equal(U_CONSTANT_TO_PARAM("mod_proxy")))  { _plugin = U_NEW(UProxyPlugIn); goto next; }
#endif
#ifdef U_STATIC_HANDLER_SOAP
   if (x.equal(U_CONSTANT_TO_PARAM("mod_soap")))   { _plugin = U_NEW(USoapPlugIn); goto next; }
#endif
#ifdef U_STATIC_HANDLER_SSI
   if (x.equal(U_CONSTANT_TO_PARAM("mod_ssi")))    { _plugin = U_NEW(USSIPlugIn); goto next; }
#endif
#ifdef U_STATIC_HANDLER_TSA
   if (x.equal(U_CONSTANT_TO_PARAM("mod_tsa")))    { _plugin = U_NEW(UTsaPlugIn); goto next; }
#endif
#ifdef U_STATIC_HANDLER_HTTP
   if (x.equal(U_CONSTANT_TO_PARAM("mod_http")))   { _plugin = U_NEW(UHttpPlugIn); goto next; }
#endif

   goto next;

next:
   U_INTERNAL_ASSERT_POINTER(_plugin)

   vplugin->push_back(_plugin);

   if (isLog()) ULog::log("[%s] link of static plugin ok\n", name);
}

int UServer_Base::loadPlugins(UString& plugin_dir, const UString& plugin_list, UFileConfig* cfg)
{
   U_TRACE(0, "UServer_Base::loadPlugins(%.*S,%.*S,%p)", U_STRING_TO_TRACE(plugin_dir), U_STRING_TO_TRACE(plugin_list), cfg)

   UString name;
   bool bnostatic;
   uint32_t i, length;
   UServerPlugIn* _plugin = 0;
   UVector<UString> vec(plugin_list);
   int result = U_PLUGIN_HANDLER_ERROR;

   vplugin_name = U_NEW(UVector<UString>(10U));
   vplugin      = U_NEW(UVector<UServerPlugIn*>(10U));

   /* I do know that to include code in the middle of a function is hacky and dirty,
    * but this is the best solution that I could figure out. If you have some idea to
    * clean it up, please, don't hesitate and let me know.
    */

#include "plugin/loader.autoconf.cpp"

   if (plugin_list.empty()) goto next;

   bnostatic = vplugin->empty();

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

   for (i = 0, length = vec.size(); i < length; ++i)
      {
      name = vec[i];

      if (vplugin_name->find(name) != U_NOT_FOUND) continue;

#  ifdef HAVE_MODULES
      _plugin = UPlugIn<UServerPlugIn*>::create(U_STRING_TO_PARAM(name));
#  endif

      if (_plugin == 0)
         {
         U_SRV_LOG("load of plugin '%.*s' FAILED", U_STRING_TO_TRACE(name));

         goto end;
         }

      name.duplicate();

      if (isLog()) ULog::log("[%.*s] load of plugin success\n", U_STRING_TO_TRACE(name));

      if (bnostatic)
         {
         vplugin->push_back(_plugin);
         vplugin_name->push_back(name);
         }
      else
         {
         vplugin->insert(0, _plugin);
         vplugin_name->insert(0, name);
         }
      }

next:
   U_INTERNAL_ASSERT_EQUALS(vplugin->size(), vplugin_name->size())

   if (cfg == 0) goto ok;

   for (i = 0, length = vplugin->size(); i < length; ++i)
      {
      name    = (*vplugin_name)[i];
      _plugin = (*vplugin)[i];

      if (cfg->searchForObjectStream(U_STRING_TO_PARAM(name)))
         {
         cfg->clear();

         if (isLog()) (void) snprintf(mod_name, sizeof(mod_name), "[%.*s] ", U_STRING_TO_TRACE(name));

         result = _plugin->handlerConfig(*cfg);

         cfg->reset();

         if (result != U_PLUGIN_HANDLER_GO_ON) goto end;
         }
      }

ok:
   result = U_PLUGIN_HANDLER_FINISHED;

end:
   mod_name[0] = '\0';

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
                                                                                          \
   int result;                                                                            \
   UString name;                                                                          \
   UServerPlugIn* _plugin;                                                                \
                                                                                          \
   for (uint32_t i = 0, length = vplugin->size(); i < length; ++i)                        \
      {                                                                                   \
      _plugin = (*vplugin)[i];                                                            \
                                                                                          \
      if (isLog())                                                                        \
         {                                                                                \
         name = (*vplugin_name)[i];                                                       \
                                                                                          \
         (void) snprintf(mod_name, sizeof(mod_name), "[%.*s] ", U_STRING_TO_TRACE(name)); \
         }                                                                                \
                                                                                          \
      result = _plugin->handler##xxx();                                                   \
                                                                                          \
      if (result != U_PLUGIN_HANDLER_GO_ON) goto end;                                     \
      }                                                                                   \
                                                                                          \
   result = U_PLUGIN_HANDLER_FINISHED;                                                    \
                                                                                          \
end:                                                                                      \
   mod_name[0] = '\0';                                                                    \
                                                                                          \
   U_RETURN(result);                                                                      \
}

U_PLUGIN_HANDLER(Init)
U_PLUGIN_HANDLER(READ)
U_PLUGIN_HANDLER(Request)
U_PLUGIN_HANDLER(Reset)

void UServer_Base::runAsUser(bool is_child)
{
   U_TRACE(0, "UServer_Base::runAsUser(%b)", is_child)

   if (pthis->as_user.empty())
      {
#  ifndef __MINGW32__
      if (is_child       ||
          (iBackLog != 1 &&
           flag_use_tcp_optimization == false))
         {
         /* don't need these anymore. Good security policy says we get rid of them */

         u_never_need_root();
         u_never_need_group();
         }
#  endif
      }
   else
      {
      /* If you want the webserver to run as a process of a defined user, you can call it.
       * For the change of user to work, it's necessary to execute the server with root privileges.
       * If it's started by a user that that doesn't have root privileges, this step will be omitted.
       */

      if (UServices::isSetuidRoot() == false)
         {
         U_SRV_LOG("the \"RUN_AS_USER\" directive makes sense only if the master process runs with super-user privileges, ignored");
         }
      else
         {
         U_INTERNAL_ASSERT(pthis->as_user.isNullTerminated())

         const char* user = pthis->as_user.data();

         if (u_runAsUser(user, false))
            {
            U_INTERNAL_DUMP("$HOME = %S", getenv("HOME"))

            U_SRV_LOG("server run with user %S permission", user);
            }
         else
            {
#        ifndef __MINGW32__
            U_ERROR("set user %S context failed...", user);
#        endif
            }
         }
      }
}

void UServer_Base::init()
{
   U_TRACE(1, "UServer_Base::init()")

   U_INTERNAL_ASSERT_POINTER(socket)

#ifndef __MINGW32__
   if (socket->isIPC())
      {
      if (name_sock.empty() == false) UUnixSocket::setPath(name_sock.data());

      if (UUnixSocket::path == 0) U_ERROR("UNIX domain socket is not bound to a file system pathname...");
      }
#endif

   if (socket->setServer(server, port, iBackLog) == false)
      {
      if (server.empty()) server = U_STRING_FROM_CONSTANT("*");

      U_ERROR("run as server with local address '%.*s:%d' FAILED...", U_STRING_TO_TRACE(server), port);
      }

   UEventFd::fd = socket->iSockDesc;

   // get name host

   host = U_NEW(UString(server.empty() ? USocketExt::getNodeName() : server));

   if (port != U_DEFAULT_PORT)
      {
      host->push_back(':');

      (void) host->append(UStringExt::numberToString(port));
      }

   U_SRV_LOG("HOST registered as: %.*s", U_STRING_TO_TRACE(*host));

   // get IP address host (default source)

   if (IP_address.empty()      &&
       server.empty() == false &&
       u_isIPAddr(UClientImage_Base::bIPv6, U_STRING_TO_PARAM(server)))
      {
      IP_address = server;
      }

   /* The above code does NOT make a connection or send any packets (to 64.233.187.99 which is google).
    * Since UDP is a stateless protocol connect() merely makes a system call which figures out how to
    * route the packets based on the address and what interface (and therefore IP address) it should
    * bind to. Returns an array containing the family (AF_INET), local port, and local address (which
    * is what we want) of the socket.
    */

   UUDPSocket cClientSocket(UClientImage_Base::bIPv6);

   if (cClientSocket.connectServer(U_STRING_FROM_CONSTANT("64.233.187.99"), 1001))
      {
      socket->cLocalAddress = cClientSocket.cLocalAddress;

      if (IP_address.empty()) IP_address = UString(socket->getLocalInfo());
      }

   U_SRV_LOG("SERVER IP ADDRESS registered as: %.*s", U_STRING_TO_TRACE(IP_address));

   // Instructs server to accept connections from the IP address IPADDR. A CIDR mask length can be
   // supplied optionally after a trailing slash, e.g. 192.168.0.0/24, in which case addresses that
   // match in the most significant MASK bits will be allowed. If no options are specified, all clients
   // are allowed. Unauthorized connections are rejected by closing the TCP connection immediately. A
   // warning is logged on the server but nothing is sent to the client.

   if (allow_IP.empty() == false)
      {
      vallow_IP = U_NEW(UVector<UIPAllow*>);

      if (UIPAllow::parseMask(allow_IP, *vallow_IP) == 0)
         {
         delete vallow_IP;
                vallow_IP = 0;
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

#ifndef __MINGW32__
   if (flag_use_tcp_optimization)
      {
      u_need_root(false);

      U_ASSERT_EQUALS(socket->isIPC(),false) // no unix socket...

   // socket->setBufferRCV(128 * 1024);
   // socket->setBufferSND(128 * 1024);

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

#  ifdef U_TCP_SETTING 
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
#  endif
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
#endif

   U_INTERNAL_ASSERT_EQUALS(proc,0)

   proc = U_NEW(UProcess);

   U_INTERNAL_ASSERT_POINTER(proc)

   proc->setProcessGroup();

   UClientImage_Base::init();

   USocket::accept4_flags = SOCK_CLOEXEC | SOCK_NONBLOCK;

   // init plugin modules...

   if (vplugin &&
       pluginsHandlerInit() != U_PLUGIN_HANDLER_FINISHED)
      {
      U_ERROR("plugins initialization FAILED. Going down...");
      }

   // init notifier event manager...

   USocket* csocket;
   UIPAddress* addr;
   UClientImage_Base* ptr;

   UNotifier::min_connection = (isClassic() == false) + (handler_inotify != 0);

   if (UNotifier::max_connection == 0) UNotifier::max_connection  = 1020;
                                       UNotifier::max_connection += (UNotifier::num_connection = UNotifier::min_connection);

   UNotifier::init();

   preallocate();

   U_INTERNAL_ASSERT_POINTER(vClientImage)

   pindex = pClientImage = vClientImage;

   eClientImage = vClientImage + UNotifier::max_connection;

   U_INTERNAL_DUMP("vClientImage = %p eClientImage = %p UNotifier::max_connection = %u", vClientImage, eClientImage, UNotifier::max_connection)

#ifdef DEBUG
   UTrace::suspend();
#endif

   for (ptr = pindex, addr = (socket->isLocalSet() ? &(socket->cLocalAddress) : 0); ptr < eClientImage; ++ptr)
      {
      csocket = ptr->socket;

                                                  csocket->flags |= O_CLOEXEC;
      if (USocket::accept4_flags & SOCK_NONBLOCK) csocket->flags |= O_NONBLOCK;

   // U_INTERNAL_DUMP("vClientImage[%d].socket->flags = %d %B", (ptr - vClientImage), csocket->flags, csocket->flags)

      if (addr) csocket->cLocalAddress.set(*addr);

      ptr->last_response = u_now->tv_sec;
      }

#ifdef DEBUG
   UTrace::resume();
#endif

   socket->flags |= O_CLOEXEC;

   // NB: in the classic model we don't need to notify for request of connection
   // (loop: accept-fork) and the forked child don't accept new client, but we need
   // event manager for the forked child to feel the eventually timeout of request from the new client...

   if (isClassic())              goto next;
#if defined(HAVE_PTHREAD_H) && defined(HAVE_EPOLL_WAIT) && !defined(HAVE_LIBEVENT)
   if (preforked_num_kids == -1) goto next; 
#endif

   UNotifier::insert(pthis); // NB: we ask to notify for request of connection (=> accept)

   /* There may not always be a connection waiting after a SIGIO is delivered or select(2) or poll(2) return
    * a readability event because the connection might have been removed by an asynchronous network error or
    * another thread before accept() is called. If this happens then the call will block waiting for the next
    * connection to arrive. To ensure that accept() never blocks, the passed socket sockfd needs to have the
    * O_NONBLOCK flag set (see socket(7))
    */

   socket->flags |= O_NONBLOCK;

#ifdef __MINGW32__
   goto next; 
#endif

#ifdef HAVE_SSL
   if (bssl == false)
#endif
      {
      UEventFd::op_mask     |= EPOLLET;
      accept_edge_triggered  = true;
      }
next:
   (void) U_SYSCALL(fcntl, "%d,%d,%d", socket->iSockDesc, F_SETFL, socket->flags);

   if (handler_inotify) UNotifier::insert(handler_inotify); // NB: we ask to be notified for change of file system (inotify)

   if (timeoutMS != -1) ptime = U_NEW(UTimeoutConnection);
}

RETSIGTYPE UServer_Base::handlerForSigHUP(int signo)
{
   U_TRACE(0, "[SIGHUP] UServer_Base::handlerForSigHUP(%d)", signo)

   U_INTERNAL_ASSERT_POINTER(pthis)

   U_INTERNAL_ASSERT(proc->parent())

   U_SRV_LOG("--- SIGHUP (Interrupt) ---");

   pthis->handlerSignal(); // manage before regenering preforked pool of children...

   // NB: we can't use UInterrupt::erase() because it restore the old action (UInterrupt::init)...
   UInterrupt::setHandlerForSignal(SIGTERM, (sighandler_t)SIG_IGN);

   // NB: for logrotate...
   if (isLog()) ULog::reopen();

   sendSigTERM();

#ifdef HAVE_LIBEVENT
   UInterrupt::setHandlerForSignal(SIGTERM, (sighandler_t)UServer_Base::handlerForSigTERM); //  sync signal
#else
   UInterrupt::insert(             SIGTERM, (sighandler_t)UServer_Base::handlerForSigTERM); // async signal
#endif
}

RETSIGTYPE UServer_Base::handlerForSigTERM(int signo)
{
   U_TRACE(0, "[SIGTERM] UServer_Base::handlerForSigTERM(%d)", signo)

   U_SRV_LOG("--- SIGTERM (Interrupt) ---");

   flag_loop = false;

   if (proc->parent())
      {
      // NB: we can't use UInterrupt::erase() because it restore the old action (UInterrupt::init)...
      UInterrupt::setHandlerForSignal(SIGTERM, (sighandler_t)SIG_IGN);

      sendSigTERM();
      }
   else
      {
#  ifdef HAVE_LIBEVENT
      (void) UDispatcher::exit(0);
#  else
      UInterrupt::erase(SIGTERM); // async signal
#  endif

      U_EXIT(0);
      }
}

int UServer_Base::handlerRead() // This method is called to accept a new connection on the server socket
{
   U_TRACE(1, "UServer_Base::handlerRead()")

   time_t idle = 0;
   uint32_t counter = 0;
   UClientImage_Base* ptr; // NB: we can't use directly the variable pClientImage cause of the thread approch...

start:
   ptr = pindex;

   U_INTERNAL_ASSERT_MINOR(pindex, eClientImage)

   U_INTERNAL_DUMP("vClientImage[%u].sfd           = %d",    (ptr - vClientImage), ptr->sfd)
   U_INTERNAL_DUMP("vClientImage[%u].UEventFd::fd  = %d",    (ptr - vClientImage), ptr->UEventFd::fd)
   U_INTERNAL_DUMP("vClientImage[%d].socket->flags = %d %B", (ptr - vClientImage), ptr->socket->flags, ptr->socket->flags)
   U_INTERNAL_DUMP("vClientImage[%d].last_response = %ld",   (ptr - vClientImage), ptr->last_response)

   if (ptr->UEventFd::fd)
      {
      if (timeoutMS != -1 &&
          u_pthread_time)
         {
         // check for idle connection

         if (idle == 0)
            {
            idle = (u_now->tv_sec - (timeoutMS / 1000));

            U_INTERNAL_DUMP("idle          = %ld", idle)
            U_INTERNAL_DUMP("u_now->tv_sec = %ld", u_now->tv_sec)
            }

         if (ptr->last_response <= idle)
            {
            if (handlerTimeoutConnection(ptr)) UNotifier::erase(ptr);

            goto next;
            }
         }
back:
      if (++pindex >= eClientImage)
         {
         U_INTERNAL_ASSERT_POINTER(vClientImage)

         pindex = vClientImage;
         }

      U_INTERNAL_ASSERT_DIFFERS(ptr, pindex)

      goto start;
      }

next:
   U_INTERNAL_ASSERT_EQUALS(ptr->UEventFd::fd,0)

   USocket* csocket = ptr->socket;

   if (socket->acceptClient(csocket) == false)
      {
      U_INTERNAL_DUMP("flag_loop = %b", flag_loop)

      if (isLog()   &&
          flag_loop &&                // check for SIGTERM event...
          csocket->iState != -EAGAIN) // NB: to avoid log spurious EAGAIN on accept() with epoll()...
         {
         char buffer[4096];

         const char* msg_error = csocket->getMsgError(buffer, sizeof(buffer));

         if (msg_error) ULog::log("accept new client failed %S\n", msg_error);
         }

      goto end;
      }

   U_INTERNAL_ASSERT(csocket->isConnected())

   // Instructs server to accept connections from the IP address IPADDR. A CIDR mask length can be supplied optionally after
   // a trailing slash, e.g. 192.168.0.0/24, in which case addresses that match in the most significant MASK bits will be allowed.
   // If no options are specified, all clients are allowed. Unauthorized connections are rejected by closing the TCP connection
   // immediately. A warning is logged on the server but nothing is sent to the client.

   if (vallow_IP &&
       ptr->isAllowed(*vallow_IP) == false)
      {
      U_SRV_LOG("new client connected from %S, connection denied by Access Control List", csocket->remoteIPAddress().getAddressString());

      csocket->close();

      goto end;
      }

   if (++UNotifier::num_connection >= UNotifier::max_connection)
      {
       --UNotifier::num_connection;

      U_SRV_LOG("new client connected from %S, connection denied by MAX_KEEP_ALIVE (%d)",
                  csocket->remoteIPAddress().getAddressString(), UNotifier::max_connection - UNotifier::min_connection);

      csocket->close();

      goto end;
      }

#if !defined(HAVE_EPOLL_WAIT) && !defined(HAVE_LIBEVENT)
   if (csocket->iSockDesc >= FD_SETSIZE)
      {
       --UNotifier::num_connection;

      U_SRV_LOG("new client connected from %S, connection denied by FD_SETSIZE (%d)", csocket->remoteIPAddress().getAddressString(), FD_SETSIZE);

      csocket->close();

      goto end;
      }
#endif

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
         int pid, status;

         csocket->close();

         U_INTERNAL_DUMP("UNotifier::num_connection = %d", UNotifier::num_connection)

         U_SRV_LOG("started new child (pid %d), up to %u children", proc->pid(), UNotifier::num_connection - UNotifier::min_connection);

retry:
         pid = UProcess::waitpid(-1, &status, WNOHANG); // NB: to avoid too much zombie...

         if (pid > 0)
            {
            --UNotifier::num_connection;

            U_SRV_LOG("child (pid %d) exited with value %d (%s), down to %u children",
                           pid, status, UProcess::exitInfo(status), UNotifier::num_connection - UNotifier::min_connection);

            goto retry;
            }

         if (isLog()) ULog::log("waiting for connection\n");

         U_RETURN(U_NOTIFIER_OK);
         }

      if (proc->child())
         {
         if (isLog()) u_unatexit(&ULog::close); // NB: needed because all instance try to close the log... (inherits from its parent)

         // NB: in the classic model we don't need to notify for request of connection
         // (loop: accept-fork) and the forked child don't accept new client, but we need
         // event manager for the forked child to feel the eventually timeout of request from the new client...

         socket->close();
         }
      }

   if (ptr->newConnection() == false)  goto check;

#ifdef HAVE_SSL
   if (bssl)                           goto insert;
#endif
#ifdef HAVE_PTHREAD_H
   if (UNotifier::pthread)             goto insert;
#endif

   if (ptr->handlerRead() == U_NOTIFIER_DELETE)
      {
      ptr->handlerDelete();

      goto check;
      }

#ifndef __MINGW32__
   // -----------------------------------------------------------------------------------------------------------------------
   // NB: for non-keepalive connection we have chance to drop small last part of large file while sending to a slow client...
   // -----------------------------------------------------------------------------------------------------------------------
   // if (ptr->bclose != U_YES)
   //    {
   //    struct linger lng = { 1, 0 };
   //
   //    (void) csocket->setSockOpt(SOL_SOCKET, SO_LINGER, (const void*)&lng, sizeof(struct linger)); // send RST - ECONNRESET
   //    }
   // -----------------------------------------------------------------------------------------------------------------------
#endif

insert:
   UNotifier::insert(ptr);

check:
   if (accept_edge_triggered && ++counter < 100) goto back;

end:

   U_RETURN(U_NOTIFIER_OK);
}

const char* UServer_Base::getNumConnection()
{
   U_TRACE(0, "UServer_Base::getNumConnection()")

   static char buffer[32];

   if (isPreForked()) (void) snprintf(buffer, sizeof(buffer), "(%d/%d)", UNotifier::num_connection - UNotifier::min_connection, U_TOT_CONNECTION);
   else               (void) snprintf(buffer, sizeof(buffer),  "%d",     UNotifier::num_connection - UNotifier::min_connection);

   U_RETURN(buffer);
}

void UServer_Base::handlerCloseConnection(UClientImage_Base* ptr)
{
   U_TRACE(0, "UServer_Base::handlerCloseConnection(%p)", ptr)

   U_INTERNAL_ASSERT_POINTER(ptr)

   if (isPreForked())
      {
      U_TOT_CONNECTION--;

      U_INTERNAL_DUMP("tot_connection = %d", U_TOT_CONNECTION)
      }

   if (isLog())
      {
      U_INTERNAL_DUMP("UEventFd::fd = %d logbuf = %.*S", ptr->UEventFd::fd, U_STRING_TO_TRACE(*(ptr->logbuf)))

      U_INTERNAL_ASSERT_MAJOR(ptr->UEventFd::fd,0)
      U_ASSERT_EQUALS(ptr->UEventFd::fd, ptr->logbuf->strtol())

      U_SRV_LOG("client closed connection from %.*s, %s clients still connected", U_STRING_TO_TRACE(*(ptr->logbuf)), getNumConnection());

      ptr->logbuf->setEmpty();
      }

   if (isClassic()) U_EXIT(0);
}

bool UServer_Base::handlerTimeoutConnection(void* cimg)
{
   U_TRACE(0, "UServer_Base::handlerTimeoutConnection(%p)", cimg)

   U_INTERNAL_ASSERT_POINTER(cimg)
   U_INTERNAL_ASSERT_POINTER(pthis)

   U_INTERNAL_DUMP("pthis = %p handler_inotify = %p ", pthis, handler_inotify)

   if (cimg != pthis &&
       cimg != handler_inotify)
      {
      ((UClientImage_Base*)cimg)->handlerError(USocket::TIMEOUT | USocket::BROKEN);

      U_SRV_LOG_WITH_ADDR("client connected didn't send any request in %u secs (timeout), close connection", getReqTimeout());

      U_RETURN(true);
      }

   U_RETURN(false);
}

// define method VIRTUAL of class UEventTime

int UServer_Base::UTimeoutConnection::handlerTime()
{
   U_TRACE(0, "UTimeoutConnection::handlerTime()")

   // there are idle connection... (timeout)

   UNotifier::callForAllEntryDynamic(handlerTimeoutConnection);

   if (USocket::accept4_flags & SOCK_NONBLOCK) UServer_Base::pthis->handlerRead();

   // return value:
   // ---------------
   // -1 - normal
   //  0 - monitoring
   // ---------------

   U_RETURN(0);
}

void UServer_Base::run()
{
   U_TRACE(0, "UServer_Base::run()")

   U_INTERNAL_ASSERT_POINTER(pthis)

   pthis->init();

   UInterrupt::syscall_restart = false;

   flag_loop = UInterrupt::exit_loop_wait_event_for_signal = true;

#ifdef HAVE_LIBEVENT
   UInterrupt::setHandlerForSignal( SIGHUP, (sighandler_t)UServer_Base::handlerForSigHUP);  //  sync signal
   UInterrupt::setHandlerForSignal(SIGTERM, (sighandler_t)UServer_Base::handlerForSigTERM); //  sync signal
#else
   UInterrupt::insert(              SIGHUP, (sighandler_t)UServer_Base::handlerForSigHUP);  // async signal
   UInterrupt::insert(             SIGTERM, (sighandler_t)UServer_Base::handlerForSigTERM); // async signal
#endif

   // -------------------------------------------------------------------------------------------------------------------------
   // PREFORK_CHILD number of child server processes created at startup: -1 - thread approach (experimental)
   //                                                                     0 - serialize, no forking
   //                                                                     1 - classic, forking after accept client
   //                                                                    >1 - pool of process serialize plus monitoring process
   // -------------------------------------------------------------------------------------------------------------------------

   if (isPreForked())
      {
      U_INTERNAL_ASSERT_MAJOR(preforked_num_kids,1)

      /**
      * Main loop for the parent process with the new preforked implementation.
      * The parent is just responsible for keeping a pool of children and they accept connections themselves...
      **/

      int pid, status, nkids = 0;
      UTimeVal to_sleep(0L, 500 * 1000L);
      bool baffinity = (preforked_num_kids <= u_get_num_cpu());

      U_INTERNAL_DUMP("baffinity = %b", baffinity)

      while (flag_loop)
         {
         u_need_root(false);

         while (nkids < preforked_num_kids)
            {
            if (proc->fork() &&
                proc->parent())
               {
               ++nkids;

               U_INTERNAL_DUMP("up to %u children", nkids)
               U_INTERNAL_DUMP("UNotifier::num_connection = %d tot_connection = %d", UNotifier::num_connection, U_TOT_CONNECTION)

               pid = proc->pid();

               cpu_set_t cpuset;

               if (baffinity) u_bind2cpu(pid, nkids); // Pin the process to a particular core...

               CPU_ZERO(&cpuset);

#           ifdef HAVE_SCHED_GETAFFINITY
               (void) U_SYSCALL(sched_getaffinity, "%d,%d,%p", pid, sizeof(cpuset), &cpuset);
#           endif

               U_INTERNAL_DUMP("cpuset = %ld %B", CPUSET_BITS(&cpuset)[0], CPUSET_BITS(&cpuset)[0])

               U_SRV_LOG("started new child (pid %d), up to %u children, affinity mask: %x", pid, nkids, CPUSET_BITS(&cpuset)[0]);

               if (set_realtime_priority &&
                   u_switch_to_realtime_priority(pid) == false)
                  {
                  U_WARNING("cannot set posix realtime scheduling policy");
                  }
               }

            if (proc->child())
               {
               U_INTERNAL_DUMP("UNotifier::num_connection = %d tot_connection = %d", UNotifier::num_connection, U_TOT_CONNECTION)

               runAsUser(true);

               UNotifier::init();

               if (isLog()) u_unatexit(&ULog::close); // NB: needed because all instance try to close the log... (inherits from its parent)

               // NB: we can't use UInterrupt::erase() because it restore the old action (UInterrupt::init)...
               UInterrupt::setHandlerForSignal(SIGHUP, (sighandler_t)SIG_IGN);

               if (isLog()) ULog::log("waiting for connection\n");

               while (flag_loop)
                  {
                  if (UInterrupt::event_signal_pending)
                     {
                     UInterrupt::callHandlerSignal();

                     continue;
                     }

                  if (UNotifier::waitForEvent(ptime) == false) break; // no more events registered...
                  }

               // NB: it is needed because only the parent process must close the log...

               if (isLog()) log = 0;

               return;
               }

            /* Don't start them too quickly, or we might overwhelm a machine that's having trouble. */

            to_sleep.nanosleep();
            }

         /* wait for any children to exit, and then start some more */

         u_dont_need_root();

         pid = UProcess::waitpid(-1, &status, 0);

         if (pid > 0 &&
             flag_loop) // check for SIGTERM event...
            {
            --nkids;

            baffinity = false;

            U_INTERNAL_DUMP("down to %u children", nkids)

            U_SRV_LOG("child (pid %d) exited with value %d (%s), down to %u children", pid, status, UProcess::exitInfo(status), nkids);

            U_INTERNAL_DUMP("tot_connection = %d", U_TOT_CONNECTION)
            }

         /* Another little safety brake here: since children should not exit
          * too quickly, pausing before starting them should be harmless. */

         to_sleep.nanosleep();
         }

      U_INTERNAL_ASSERT(proc->parent())

      (void) proc->waitAll();
      }
   else
      {
      runAsUser(false);

      if (isLog()) ULog::log("waiting for connection\n");

      U_INTERNAL_ASSERT(preforked_num_kids <= 1)

#  if defined(HAVE_PTHREAD_H) && defined(HAVE_EPOLL_WAIT) && !defined(HAVE_LIBEVENT)
      if (preforked_num_kids == -1) ((UThread*)(UNotifier::pthread = U_NEW(UClientThread)))->start();
#  endif

      while (flag_loop)
         {
         if (UInterrupt::event_signal_pending)
            {
            UInterrupt::callHandlerSignal();

            continue;
            }

         // NB: in the classic model we don't need to notify for request of connection
         // (loop: accept-fork) and the forked child don't accept new client, but we need
         // event manager for the forked child to feel the eventually timeout of request from the new client...

         if (socket->isClosed() ||
             preforked_num_kids == 0)
            {
            (void) UNotifier::waitForEvent(ptime);

            continue;
            }

         U_INTERNAL_ASSERT((socket->flags & O_NONBLOCK) == 0)

         (void) pthis->handlerRead();
         }
      }
}

// it creates a copy of itself, return true if parent...

bool UServer_Base::parallelization()
{
   U_TRACE(0, "UServer_Base::parallelization()")

   U_INTERNAL_ASSERT_POINTER(proc)

   U_INTERNAL_ASSERT_EQUALS(UClientImage_Base::isPipeline(),false)

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
            // NB: we need locking to write log...

            log = 0;

            delete pClientImage->logbuf;
                   pClientImage->logbuf = 0;
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

      if (environment.empty() == false) cmd->setEnvironment(&environment);
      }

   U_RETURN_POINTER(cmd,UCommand);
}

void UServer_Base::logCommandMsgError(const char* cmd)
{
   U_TRACE(0, "UServer_Base::logCommandMsgError(%S)", cmd)

   if (isLog())
      {
      UCommand::setMsgError(cmd);

      ULog::log("%s%.*s\n", mod_name, u_buffer_len, u_buffer);

      u_buffer_len = 0;
      }
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UServer_Base::dump(bool reset) const
{
   *UObjectIO::os << "port                      " << port                        << '\n'
                  << "iBackLog                  " << iBackLog                    << '\n'
                  << "flag_loop                 " << flag_loop                   << '\n'
                  << "timeoutMS                 " << timeoutMS                   << '\n'
                  << "verify_mode               " << verify_mode                 << '\n'
                  << "cgi_timeout               " << cgi_timeout                 << '\n'
                  << "verify_mode               " << verify_mode                 << '\n'
                  << "shared_data_add           " << shared_data_add             << '\n'
                  << "ptr_shared_data           " << (void*)ptr_shared_data      << '\n'
                  << "preforked_num_kids        " << preforked_num_kids          << '\n'
                  << "flag_use_tcp_optimization " << flag_use_tcp_optimization   << '\n'
                  << "log           (ULog       " << (void*)log                  << ")\n"
                  << "socket        (USocket    " << (void*)socket               << ")\n"
                  << "host          (UString    " << (void*)host                 << ")\n"
                  << "server        (UString    " << (void*)&server              << ")\n"
                  << "log_file      (UString    " << (void*)&log_file            << ")\n"
                  << "dh_file       (UString    " << (void*)&dh_file             << ")\n"
                  << "ca_file       (UString    " << (void*)&ca_file             << ")\n"
                  << "ca_path       (UString    " << (void*)&ca_path             << ")\n"
                  << "allow_IP      (UString    " << (void*)&allow_IP            << ")\n"
                  << "key_file      (UString    " << (void*)&key_file            << ")\n"
                  << "password      (UString    " << (void*)&password            << ")\n"
                  << "cert_file     (UString    " << (void*)&cert_file           << ")\n"
                  << "name_sock     (UString    " << (void*)&name_sock           << ")\n"
                  << "IP_address    (UString    " << (void*)&IP_address          << ")\n"
                  << "document_root (UString    " << (void*)&document_root       << ")\n"
                  << "proc          (UProcess   " << (void*)proc                 << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
