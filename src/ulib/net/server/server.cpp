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

#include <ulib/timeval.h>
#include <ulib/command.h>
#include <ulib/file_config.h>
#include <ulib/net/udpsocket.h>
#include <ulib/utility/services.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/string_ext.h>

#ifndef __MINGW32__
#  include <ulib/net/unixsocket.h>
#endif

#define U_DEFAULT_PORT 80

int                      UServer_Base::port;
int                      UServer_Base::req_timeout;
int                      UServer_Base::cgi_timeout;
int                      UServer_Base::log_file_sz;
int                      UServer_Base::verify_mode;
int                      UServer_Base::num_connection;
int                      UServer_Base::max_Keep_Alive;
int                      UServer_Base::preforked_num_kids;
bool                     UServer_Base::flag_loop;
bool                     UServer_Base::digest_authentication;
char                     UServer_Base::mod_name[20];
ULog*                    UServer_Base::log;
UString*                 UServer_Base::host;
USocket*                 UServer_Base::socket;
UProcess*                UServer_Base::proc;
UServer_Base*            UServer_Base::pthis;
UVector<UIPAllow*>*      UServer_Base::vallow_IP;
#ifdef HAVE_MODULES
UVector<UServerPlugIn*>* UServer_Base::vplugin;
#endif
#ifdef HAVE_LIBEVENT
UEvent<UServer_Base>*    UServer_Base::pevent;
#else
UEventTime*              UServer_Base::ptime;
#endif

const UString* UServer_Base::str_USE_IPV6;
const UString* UServer_Base::str_PORT;
const UString* UServer_Base::str_MSG_WELCOME;
const UString* UServer_Base::str_COMMAND;
const UString* UServer_Base::str_ENVIRONMENT;
const UString* UServer_Base::str_RUN_AS_USER;
const UString* UServer_Base::str_PREFORK_CHILD;
const UString* UServer_Base::str_LOG_FILE;
const UString* UServer_Base::str_LOG_FILE_SZ;
const UString* UServer_Base::str_CERT_FILE;
const UString* UServer_Base::str_KEY_FILE;
const UString* UServer_Base::str_PASSWORD;
const UString* UServer_Base::str_CA_FILE;
const UString* UServer_Base::str_CA_PATH;
const UString* UServer_Base::str_VERIFY_MODE;
const UString* UServer_Base::str_ALLOWED_IP;
const UString* UServer_Base::str_NAME_SOCKET;
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

UString*       UServer_Base::htpasswd;
UString*       UServer_Base::htdigest;
const UString* UServer_Base::str_htpasswd;
const UString* UServer_Base::str_htdigest;

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
   U_INTERNAL_ASSERT_EQUALS(str_CERT_FILE,0)
   U_INTERNAL_ASSERT_EQUALS(str_KEY_FILE,0)
   U_INTERNAL_ASSERT_EQUALS(str_PASSWORD,0)
   U_INTERNAL_ASSERT_EQUALS(str_CA_FILE,0)
   U_INTERNAL_ASSERT_EQUALS(str_CA_PATH,0)
   U_INTERNAL_ASSERT_EQUALS(str_VERIFY_MODE,0)
   U_INTERNAL_ASSERT_EQUALS(str_ALLOWED_IP,0)
   U_INTERNAL_ASSERT_EQUALS(str_NAME_SOCKET,0)
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

   U_INTERNAL_ASSERT_EQUALS(str_htdigest,0)
   U_INTERNAL_ASSERT_EQUALS(str_htpasswd,0)

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
   { U_STRINGREP_FROM_CONSTANT("CERT_FILE") },
   { U_STRINGREP_FROM_CONSTANT("KEY_FILE") },
   { U_STRINGREP_FROM_CONSTANT("PASSWORD") },
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

   { U_STRINGREP_FROM_CONSTANT(".htdigest") },
   { U_STRINGREP_FROM_CONSTANT(".htpasswd") }
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
   U_NEW_ULIB_OBJECT(str_CERT_FILE,             U_STRING_FROM_STRINGREP_STORAGE(9));
   U_NEW_ULIB_OBJECT(str_KEY_FILE,              U_STRING_FROM_STRINGREP_STORAGE(10));
   U_NEW_ULIB_OBJECT(str_PASSWORD,              U_STRING_FROM_STRINGREP_STORAGE(11));
   U_NEW_ULIB_OBJECT(str_CA_FILE,               U_STRING_FROM_STRINGREP_STORAGE(12));
   U_NEW_ULIB_OBJECT(str_CA_PATH,               U_STRING_FROM_STRINGREP_STORAGE(13));
   U_NEW_ULIB_OBJECT(str_VERIFY_MODE,           U_STRING_FROM_STRINGREP_STORAGE(14));
   U_NEW_ULIB_OBJECT(str_ALLOWED_IP,            U_STRING_FROM_STRINGREP_STORAGE(15));
   U_NEW_ULIB_OBJECT(str_NAME_SOCKET,           U_STRING_FROM_STRINGREP_STORAGE(16));
   U_NEW_ULIB_OBJECT(str_DOCUMENT_ROOT,         U_STRING_FROM_STRINGREP_STORAGE(17));
   U_NEW_ULIB_OBJECT(str_PLUGIN,                U_STRING_FROM_STRINGREP_STORAGE(18));
   U_NEW_ULIB_OBJECT(str_PLUGIN_DIR,            U_STRING_FROM_STRINGREP_STORAGE(19));
   U_NEW_ULIB_OBJECT(str_REQ_TIMEOUT,           U_STRING_FROM_STRINGREP_STORAGE(20));
   U_NEW_ULIB_OBJECT(str_CGI_TIMEOUT,           U_STRING_FROM_STRINGREP_STORAGE(21));
   U_NEW_ULIB_OBJECT(str_VIRTUAL_HOST,          U_STRING_FROM_STRINGREP_STORAGE(22));
   U_NEW_ULIB_OBJECT(str_DIGEST_AUTHENTICATION, U_STRING_FROM_STRINGREP_STORAGE(23));
   U_NEW_ULIB_OBJECT(str_URI,                   U_STRING_FROM_STRINGREP_STORAGE(24));
   U_NEW_ULIB_OBJECT(str_HOST,                  U_STRING_FROM_STRINGREP_STORAGE(25));
   U_NEW_ULIB_OBJECT(str_USER,                  U_STRING_FROM_STRINGREP_STORAGE(26));
   U_NEW_ULIB_OBJECT(str_SERVER,                U_STRING_FROM_STRINGREP_STORAGE(27));
   U_NEW_ULIB_OBJECT(str_METHOD_NAME,           U_STRING_FROM_STRINGREP_STORAGE(28));
   U_NEW_ULIB_OBJECT(str_RESPONSE_TYPE,         U_STRING_FROM_STRINGREP_STORAGE(29));
   U_NEW_ULIB_OBJECT(str_IP_ADDRESS,            U_STRING_FROM_STRINGREP_STORAGE(30));
   U_NEW_ULIB_OBJECT(str_MAX_KEEP_ALIVE,        U_STRING_FROM_STRINGREP_STORAGE(31));
   U_NEW_ULIB_OBJECT(str_PID_FILE,              U_STRING_FROM_STRINGREP_STORAGE(32));

   U_NEW_ULIB_OBJECT(str_htdigest,              U_STRING_FROM_STRINGREP_STORAGE(33));
   U_NEW_ULIB_OBJECT(str_htpasswd,              U_STRING_FROM_STRINGREP_STORAGE(34));
}

UServer_Base::UServer_Base(UFileConfig* cfg)
{
   U_TRACE_REGISTER_OBJECT(0, UServer_Base, "%p", cfg)

   U_INTERNAL_ASSERT_EQUALS(pthis, 0)

   if (str_USE_IPV6 == 0) str_allocate();

   port              = U_DEFAULT_PORT;
   pthis             = this;
   UEventFd::op_mask = R_OK; // W_OK == 2, R_OK == 4...

   if (cfg) loadConfigParam(*cfg);
}

UServer_Base::~UServer_Base()
{
   U_TRACE_UNREGISTER_OBJECT(0, UServer_Base)

   U_INTERNAL_ASSERT_POINTER(socket)

   delete socket;

#ifdef HAVE_LIBEVENT
   if (pevent &&
       isParent())
      {
      UDispatcher::del(pevent);
                delete pevent;
      }
#else
   if (ptime) delete ptime;

   if (UNotifier::empty() == false) UNotifier::erase(this, false); // NB: to avoid to delete himself...
                                    UNotifier::clear();
#endif

#ifdef HAVE_MODULES
   if (vplugin)
      {
      delete vplugin;

      UPlugIn<void*>::clear();
      }
#endif

   if (UHTTP::tmpdir)
      {
      delete UHTTP::tmpdir;
      delete UHTTP::form_name_value;
      }

   if (log)        delete log;
   if (host)       delete host;
   if (proc)       delete proc;
   if (htpasswd)   delete htpasswd;
   if (htdigest)   delete htdigest;
   if (vallow_IP)  delete vallow_IP;

   UClientImage_Base::clear();
}

void UServer_Base::loadConfigParam(UFileConfig& cfg)
{
   U_TRACE(0, "UServer_Base::loadConfigParam(%p)", &cfg)

   U_ASSERT_EQUALS(cfg.empty(), false)

   // --------------------------------------------------------------------------------------------------------------------------------------
   // UServer - configuration parameters
   // --------------------------------------------------------------------------------------------------------------------------------------
   // USE_IPV6      flag indicating the use of ipv6
   // SERVER        host name or ip address for the listening socket
   // PORT          port number             for the listening socket
   // SOCKET_NAME   file name               for the listening socket
   // IP_ADDRESS    public ip address of host for the interface connected to the Internet (autodetected if not specified)
   // ALLOWED_IP    list of comma separated client address for IP-based access control (IPADDR[/MASK])
   //
   // PID_FILE      write pid on file indicated
   // WELCOME_MSG   message of welcome to send initially to client
   // RUN_AS_USER   downgrade the security to that of user account
   // DOCUMENT_ROOT The directory out of which you will serve your documents
   //
   // LOG_FILE      locations   for file log
   // LOG_FILE_SZ   memory size for file log
   //
   // PLUGIN        list of plugins to load, a flexible way to add specific functionality to the server
   // PLUGIN_DIR    directory of plugins to load
   //
   // REQ_TIMEOUT   timeout for request from client
   // CGI_TIMEOUT   timeout for cgi execution
   //
   // MAX_KEEP_ALIVE Specifies the maximum number of requests that can be served through a Keep-Alive (Persistent) session.
   //                (Value <= 1 will disable Keep-Alive)
   //
   // CERT_FILE     server certificate
   // KEY_FILE      server private key
   // PASSWORD      password for server private key
   // CA_FILE       locations of trusted CA certificates used in the verification
   // CA_PATH       locations of trusted CA certificates used in the verification
   // VERIFY_MODE   mode of verification (SSL_VERIFY_NONE=0, SSL_VERIFY_PEER=1, SSL_VERIFY_FAIL_IF_NO_PEER_CERT=2, SSL_VERIFY_CLIENT_ONCE=4)
   //
   // PREFORK_CHILD number of child server processes created at startup ( 0 - serialize, no forking
   //                                                                     1 - classic, forking after client accept
   //                                                                    >1 - pool of serialized processes plus monitoring process)
   // --------------------------------------------------------------------------------------------------------------------------------------

   server         = cfg[*str_SERVER];
   as_user        = cfg[*str_RUN_AS_USER],
   log_file       = cfg[*str_LOG_FILE];
   allow_IP       = cfg[*str_ALLOWED_IP];
   name_sock      = cfg[*str_NAME_SOCKET];
   IP_address     = cfg[*str_IP_ADDRESS];
   document_root  = cfg[*str_DOCUMENT_ROOT];

#ifdef HAVE_IPV6
   UClientImage_Base::bIPv6 = cfg.readBoolean(*str_USE_IPV6);
#endif

   port           = cfg.readLong(*str_PORT, U_DEFAULT_PORT);
   req_timeout    = cfg.readLong(*str_REQ_TIMEOUT);
   cgi_timeout    = cfg.readLong(*str_CGI_TIMEOUT);
   log_file_sz    = cfg.readLong(*str_LOG_FILE_SZ);
   max_Keep_Alive = cfg.readLong(*str_MAX_KEEP_ALIVE);

#ifndef __MINGW32__
   preforked_num_kids = cfg.readLong(*str_PREFORK_CHILD);
#endif

   if (cgi_timeout) UCommand::setTimeout(cgi_timeout);

   UClientImage_Base::setMsgWelcome(cfg[*str_MSG_WELCOME]);

#ifdef HAVE_SSL
   ca_file     = cfg[*str_CA_FILE];
   ca_path     = cfg[*str_CA_PATH];
   key_file    = cfg[*str_KEY_FILE];
   password    = cfg[*str_PASSWORD];
   cert_file   = cfg[*str_CERT_FILE];
   verify_mode = cfg.readLong(*str_VERIFY_MODE);
#endif

   // write pid on file...

   UString pid_file = cfg[*str_PID_FILE];

   if (pid_file.empty() == false) (void) UFile::writeTo(pid_file, UString(u_pid_str, u_pid_str_len));

   // load plugin modules and call server-wide hooks handlerConfig()...

#ifdef HAVE_MODULES
   UString plugin_list = cfg[*str_PLUGIN];

   if (plugin_list.empty() == false)
      {
      UVector<UString> vname(plugin_list);

      if (vname.empty()                                  == false &&
          loadPlugins(cfg[*str_PLUGIN_DIR], vname, &cfg) == U_PLUGIN_HANDLER_ERROR)
         {
         U_ERROR("plugins load FAILED. Going down...", 0);
         }
      }
#endif

   cfg.clear();
   cfg.deallocate();
}

// load plugin modules and call server-wide hooks handlerConfig()...

#ifdef HAVE_MODULES
int UServer_Base::loadPlugins(const UString& plugin_dir, UVector<UString>& plugin_list, UFileConfig* cfg)
{
   U_TRACE(0, "UServer_Base::loadPlugins(%.*S,%p,%p)", U_STRING_TO_TRACE(plugin_dir), &plugin_list, cfg)

   UString name;
   UServerPlugIn* plugin;
   int result = U_PLUGIN_HANDLER_GO_ON;

   vplugin = U_NEW(UVector<UServerPlugIn*>);

   if (plugin_dir.empty() == false) UPlugIn<void*>::setPluginDirectory(plugin_dir.c_str());

   for (uint32_t i = 0, length = plugin_list.size(); i < length; ++i)
      {
      name   = plugin_list[i];
      plugin = UPlugIn<UServerPlugIn*>::create(U_STRING_TO_PARAM(name));

      if (plugin == 0)
         {
         U_WARNING("load of plugin '%.*s' FAILED", U_STRING_TO_TRACE(name));

         U_RETURN(U_PLUGIN_HANDLER_ERROR);
         }

      vplugin->push_back(plugin);

      if (cfg && cfg->searchForObjectStream(U_STRING_TO_PARAM(name)))
         {
         cfg->clear();

         (void) u_snprintf(mod_name, sizeof(mod_name), "[%.*s] ", U_STRING_TO_TRACE(name));

         result = plugin->handlerConfig(*cfg);

         cfg->reset();

         if (result != U_PLUGIN_HANDLER_GO_ON) break;
         }
      }

   mod_name[0] = '\0';

   U_RETURN(result);
}

// manage plugin handler hooks...

#define U_PLUGIN_HANDLER(xxx)                                                                \
                                                                                             \
int UServer_Base::plugins_handler##xxx()                                                     \
{                                                                                            \
   U_TRACE(0, "UServer_Base::plugins_handler"#xxx"()")                                       \
                                                                                             \
   U_INTERNAL_ASSERT_POINTER(vplugin)                                                        \
                                                                                             \
   UServerPlugIn* plugin;                                                                    \
   UPlugIn<void*>* wrapper;                                                                  \
   int result = U_PLUGIN_HANDLER_ERROR;                                                      \
                                                                                             \
   for (uint32_t i = 0, length = vplugin->size(); i < length; ++i)                           \
      {                                                                                      \
      plugin  = vplugin->at(i);                                                              \
      wrapper = UPlugIn<void*>::getObjWrapper(plugin);                                       \
                                                                                             \
      U_INTERNAL_ASSERT_POINTER(wrapper)                                                     \
                                                                                             \
      (void) u_snprintf(mod_name, sizeof(mod_name), "[%.*s] ", U_PLUGIN_TO_TRACE(*wrapper)); \
                                                                                             \
      result = plugin->handler##xxx();                                                       \
                                                                                             \
      if (result != U_PLUGIN_HANDLER_GO_ON) break;                                           \
      }                                                                                      \
                                                                                             \
   mod_name[0] = '\0';                                                                       \
                                                                                             \
   U_RETURN(result);                                                                         \
}

U_PLUGIN_HANDLER(Init)
U_PLUGIN_HANDLER(Read)
U_PLUGIN_HANDLER(Request)
U_PLUGIN_HANDLER(Reset)
#endif

void UServer_Base::init()
{
   U_TRACE(0, "UServer_Base::init()")

   U_INTERNAL_ASSERT_POINTER(UClientImage_Base::socket)

#ifndef __MINGW32__
   if (name_sock.empty() == false) UUnixSocket::setPath(name_sock.data()); // unix socket...
#endif

   if (socket->setServer(server, port, 256) == false)
      {
      if (server.empty()) server = U_STRING_FROM_CONSTANT("*");

      U_ERROR("run as server with local address '%.*s:%d' FAILED...", U_STRING_TO_TRACE(server), port);
      }

   UEventFd::fd = socket->getFd();

   if (log_file.empty() == false) openLog(log_file, log_file_sz);

   // get name host

   host = U_NEW(UString(server.empty() ? getNodeName() : server));

   if (port != 80)
      {
      host->push_back(':');

      (void) host->append(UStringExt::numberToString(port));
      }

   U_SRV_LOG_VAR("HOST registered as: %.*s", U_STRING_TO_TRACE(*host));

   // get IP address host (default source)

   if (IP_address.empty()      &&
       server.empty() == false &&
       u_isIPAddr(UClientImage_Base::bIPv6, U_STRING_TO_PARAM(server)))
      {
      IP_address = server;
      }

   /* The above code does NOT make a connection or send any packets (to 64.233.187.99 which is google). Since UDP is a stateless protocol connect()
    * merely makes a system call which figures out how to route the packets based on the address and what interface (and therefore IP address) it
    * should bind to. Returns an array containing the family (AF_INET), local port, and local address (which is what we want) of the socket.
    */

   UUDPSocket cClientSocket(UClientImage_Base::bIPv6);

   if (cClientSocket.connectServer(U_STRING_FROM_CONSTANT("64.233.187.99"), 1001))
      {
      UClientImage_Base::socket->setLocal((socket->cLocalAddress = cClientSocket.cLocalAddress));

      if (IP_address.empty()) IP_address = UString(socket->getLocalInfo());
      }

   U_SRV_LOG_VAR("SERVER IP ADDRESS registered as: %.*s", U_STRING_TO_TRACE(IP_address));

   // Instructs server to accept connections from the IP address IPADDR. A CIDR mask length can be supplied optionally after
   // a trailing slash, e.g. 192.168.0.0/24, in which case addresses that match in the most significant MASK bits will be allowed.
   // If no options are specified, all clients are allowed. Unauthorized connections are rejected by closing the TCP connection
   // immediately. A warning is logged on the server but nothing is sent to the client.

   if (allow_IP.empty() == false)
      {
      vallow_IP = U_NEW(UVector<UIPAllow*>);

      if (UIPAllow::parseMask(allow_IP, *vallow_IP) == 0)
         {
         delete vallow_IP;
                vallow_IP = 0;
         }
      }

   // DOCUMENT_ROOT: The directory out of which you will serve your documents

   if (document_root.empty() ||
       document_root.equal(U_CONSTANT_TO_PARAM(".")))
      {
      (void) document_root.assign(u_cwd, u_cwd_len);
      }
   else
      {
      u_canonicalize_pathname((char*)document_root.c_str());

      U_INTERNAL_DUMP("document_root = %S", document_root.data())
      }

   if (document_root.first_char() == '.') document_root = UFile::getRealPath(document_root.data());

   if (UFile::chdir(document_root.data(), false) == false)
      {
      U_ERROR("chdir to working directory (DOCUMENT_ROOT) '%s' FAILED. Going down...", document_root.data());
      }

   U_SRV_LOG_VAR("working directory (DOCUMENT_ROOT) changed to '%s'", document_root.data());

   // manage authorization data...

#ifdef HAVE_SSL
   htpasswd = U_NEW(UString(UFile::contentOf(*str_htpasswd)));
   htdigest = U_NEW(UString(UFile::contentOf(*str_htdigest)));

   if (htpasswd->empty())
      {
      delete htpasswd;
             htpasswd = 0;
      }
   else
      {
      U_SRV_LOG_VAR("file data users permission: '%.*s' loaded", U_STRING_TO_TRACE(*str_htpasswd));
      }

   if (htdigest->empty())
      {
      delete htdigest;
             htdigest = 0;
      }
   else
      {
      U_SRV_LOG_VAR("file data users permission: '%.*s' loaded", U_STRING_TO_TRACE(*str_htdigest));
      }

   // For ULIB facility request TODO stateless session cookies... 

   UServices::generateKey();
#endif

   // init plugin modules...

#ifdef HAVE_MODULES
   if (pluginsHandlerInit() != U_PLUGIN_HANDLER_GO_ON) U_ERROR("initialization of plugins FAILED. Going down...", 0);
#endif

   if (as_user.empty() == false)
      {
      const char* user = as_user.data();

      if (u_ranAsUser(user, false))
         {
         U_SRV_LOG_VAR("server run with user '%s' permission", user);
         }
      else
         {
         U_ERROR("set user '%s' context failed...", user);
         }
      }

   if (name_sock.empty()) // no unix socket...
      {
   // socket->setBufferRCV(64 * 1024);
   // socket->setBufferSND(64 * 1024);

      // NB: OPTIMIZATION in benchmarking with ab...
      
                                               socket->setTcpCork(1U);        // 8.9k -> 9.6k
      if (UClientImage_Base::msg_welcome == 0) socket->setTcpDeferAccept(1U); // 8.5k -> 8.9k
      }
}

RETSIGTYPE UServer_Base::handlerForSigTERM(int signo)
{
   U_TRACE(0, "[SIGTERM] UServer_Base::handlerForSigTERM(%d)", signo)

   U_SRV_LOG_MSG("--- SIGTERM (Interrupt) ---");

   U_INTERNAL_ASSERT_EQUALS(flag_loop, true)

   flag_loop = false;

#ifdef HAVE_LIBEVENT
   (void) UDispatcher::exit(0);
#endif

   // NB: we can't use UInterrupt::erase() because it restore the old action (UInterrupt::init)...
   UInterrupt::setHandlerForSignal(SIGTERM, (sighandler_t)SIG_IGN);

   if (isParent()) sendSigTERM();
}

RETSIGTYPE UServer_Base::handlerForSigHUP(int signo)
{
   U_TRACE(0, "[SIGHUP] UServer_Base::handlerForSigHUP(%d)", signo)

   U_INTERNAL_ASSERT_POINTER(pthis)

   U_ASSERT(isParent())

   U_SRV_LOG_MSG("--- SIGHUP (Interrupt) ---");

   pthis->handlerSignal(); // manage before regenering preforked pool of children...

   // NB: we can't use UInterrupt::erase() because it restore the old action (UInterrupt::init)...
   UInterrupt::setHandlerForSignal(SIGTERM, (sighandler_t)SIG_IGN);

   // NB: for logrotate...
   if (isLog()) ULog::reopen();

   sendSigTERM();

   UInterrupt::insert(SIGTERM, (sighandler_t)UServer_Base::handlerForSigTERM); // async signal
}

void UServer_Base::handlerNewConnection()
{
   U_TRACE(1, "UServer_Base::handlerNewConnection()")

   U_INTERNAL_ASSERT_POINTER(pthis)
   U_INTERNAL_ASSERT_POINTER(UClientImage_Base::socket)

   U_INTERNAL_DUMP("flag_loop = %b", flag_loop)

   if (UClientImage_Base::socket->isConnected() == false)
      {
      if (flag_loop == false) return; // check for SIGTERM event...

      const char* msg_error = "";

#  ifdef HAVE_SSL
      if (UClientImage_Base::socket->isSSL()) msg_error = ((USSLSocket*)UClientImage_Base::socket)->getError();
#  endif

      U_SRV_LOG_VAR("accept new client failed %s", msg_error);

      return;
      }

   // Instructs server to accept connections from the IP address IPADDR. A CIDR mask length can be supplied optionally after
   // a trailing slash, e.g. 192.168.0.0/24, in which case addresses that match in the most significant MASK bits will be allowed.
   // If no options are specified, all clients are allowed. Unauthorized connections are rejected by closing the TCP connection
   // immediately. A warning is logged on the server but nothing is sent to the client.

   if (vallow_IP && UIPAllow::isAllowed(UClientImage_Base::socket->remoteIPAddress().getInAddr(), *vallow_IP) == false)
      {
      U_SRV_LOG_VAR("new client connected from '%s', connection denied by access list", UClientImage_Base::socket->getRemoteInfo());

      UClientImage_Base::socket->close();

      return;
      }

   ++num_connection;

   U_INTERNAL_DUMP("num_connection = %d", num_connection)

   // --------------------------------------------------------------------------------------------------------------------------
   // PREFORK_CHILD number of child server processes created at startup ( 0 - serialize, no forking
   //                                                                     1 - classic, forking after accept client
   //                                                                    >1 - pool of process serialize plus monitoring process)
   // --------------------------------------------------------------------------------------------------------------------------

   if (isClassic())
      {
      U_INTERNAL_ASSERT_POINTER(proc)

      if (proc->fork())
         {
         if (proc->child())
            {
            if (isLog()) u_unatexit(&ULog::close); // NB: needed because all instance try to close the log... (inherits from its parent)
            }
         else
            {
            UClientImage_Base::socket->close();

            (void) UProcess::waitpid(); // per evitare piu' di 1 zombie...

            return;
            }
         }
      }

   if (req_timeout) (void) UClientImage_Base::socket->setTimeoutRCV(req_timeout * 1000);

   pthis->newClientImage();

   UClientImage_Base::run();
}

void UServer_Base::run()
{
   U_TRACE(0, "UServer_Base::run()")

   U_INTERNAL_ASSERT_POINTER(pthis)

   pthis->init();

   flag_loop = true;

   UInterrupt::syscall_restart = false;

   UInterrupt::insert( SIGHUP, (sighandler_t)UServer_Base::handlerForSigHUP);  // async signal
   UInterrupt::insert(SIGTERM, (sighandler_t)UServer_Base::handlerForSigTERM); // async signal

#ifndef HAVE_LIBEVENT
   UNotifier::exit_loop_wait_event_for_signal = true;

   if (req_timeout) ptime = U_NEW(UEventTime(req_timeout, 0L));
#endif

   // --------------------------------------------------------------------------------------------------------------------------
   // PREFORK_CHILD number of child server processes created at startup ( 0 - serialize, no forking
   //                                                                     1 - classic, forking after accept client
   //                                                                    >1 - pool of process serialize plus monitoring process)
   // --------------------------------------------------------------------------------------------------------------------------

   setProcessManager();

   // NB: if serialize (0 and >1) we ask to notify for request of connection,
   //     in this way only in the classic model the forked child don't accept new client...

   if (isSerialize()) notifyForEventRequestConnection();

   /**
   * Main loop for the parent process with the new preforked implementation.
   * The parent is just responsible for keeping a pool of children and they accept connections themselves...
   **/

   if (isPreForked())
      {
      UTimeVal to_sleep(1L);
      int pid, status, nkids = 0;

      U_INTERNAL_ASSERT_POINTER(proc)

      while (flag_loop)
         {
         while (nkids < preforked_num_kids)
            {
            if (proc->fork())
               {
               if (proc->child())
                  {
                  if (isLog()) u_unatexit(&ULog::close); // NB: needed because all instance try to close the log... (inherits from its parent)

                  goto preforked_child;
                  }
               else
                  {
                  ++nkids;

                  U_INTERNAL_DUMP("up to %u children", nkids)

                  U_SRV_LOG_VAR("started new child (pid %ld), up to %u children", proc->pid(), nkids);
                  }
               }

            /* Don't start them too quickly, or we might overwhelm a machine that's having trouble. */

            to_sleep.nanosleep();
            }

         /* wait for any children to exit, and then start some more */

         pid = UProcess::waitpid(-1, &status, 0);

         if (pid > 0 && flag_loop) // check for SIGTERM event...
            {
            --nkids;

            U_INTERNAL_DUMP("down to %u children", nkids)

            U_SRV_LOG_VAR("child (pid %d) exited with value %d, down to %u children", pid, status, nkids);
            }

         /* Another little safety brake here: since children should not exit
          * too quickly, pausing before starting them should be harmless. */

         to_sleep.nanosleep();
         }

      goto end;
      }

preforked_child:

   while (flag_loop)
      {
      if (isClientConnect() == false) // check if we can block on accept()...
         {
wait:
         U_SRV_LOG_MSG("waiting for connection");

         if (UInterrupt::event_signal_pending)
            {
            UInterrupt::callHandlerSignal();

            continue;
            }

         // here we go to block on accept(), plus fork() - eventually child start here...

         (void) pthis->handlerRead();

         U_INTERNAL_DUMP("flag_loop = %b", flag_loop)

         continue; // may be interrupt...
         }

#  ifdef HAVE_LIBEVENT
      if (UDispatcher::dispatch(UDispatcher::ONCE)) break; // no more events registered...
#  else
      if (UNotifier::waitForEvent(ptime) == false)  break; // no more events registered...

      // check for idle connection... (timeout)

      U_INTERNAL_DUMP("UNotifier::result = %d", UNotifier::result)

      if (UNotifier::result == 0) // (select() == 0) -> timeout
         {
         if (isSerialize() == false)
            {
            U_SRV_LOG_TIMEOUT(UClientImage_Base::pClientImage);

            break;
            }

         UNotifier* item;

         while (true)
            {
            item = UNotifier::first;

            U_INTERNAL_DUMP("item = %p", item)

            U_INTERNAL_ASSERT_POINTER(item)
            U_INTERNAL_ASSERT_POINTER(item->handler_event_fd)

            // NB: check to avoid to delete himself...

            if (item->handler_event_fd == pthis && (item = item->next) == 0) break;

            U_INTERNAL_DUMP("item = %p", item)

            U_INTERNAL_ASSERT_POINTER(item)
            U_INTERNAL_ASSERT_POINTER(item->handler_event_fd)

            U_SRV_LOG_TIMEOUT((UClientImage_Base*)(item->handler_event_fd));

            UNotifier::erase(item->handler_event_fd, true);
            }
         }
#  endif
      }

   if (flag_loop && isPreForked()) goto wait; // if we are preforked don't go away...

end:

   // NB: needed because only the parent process must close the log...

   if (isLog() && proc)
      {
      if (isChild()) log = 0;
      else           (void) proc->waitAll();
      }
}

UCommand* UServer_Base::loadConfigCommand(UFileConfig& cfg, bool bset)
{
   U_TRACE(0, "UServer_Base::loadConfigCommand(%p,%b)", &cfg, bset)

   U_ASSERT_EQUALS(cfg.empty(), false)

   UCommand* cmd   = 0;
   UString command = cfg[*UServer_Base::str_COMMAND];

   if (command.empty() == false)
      {
      if (U_ENDS_WITH(command, ".sh")) (void) command.insert(0, U_PATH_SHELL);

      UString environment = cfg[*UServer_Base::str_ENVIRONMENT];

      cmd = U_NEW(UCommand);

      if (bset)
         {
         const UString* penv = (environment.empty() ? 0 : &environment);

         cmd->set(command, penv);
         }
      else
         {
         cmd->command     = command;
         cmd->environment = environment;
         }
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

UString UServer_Base::getUserHA1(const UString& user, const UString& realm)
{
   U_TRACE(0, "UServer_Base::getUserHA1(%.*S,%.*S)", U_STRING_TO_TRACE(user), U_STRING_TO_TRACE(realm))

   UString ha1;

   if (htdigest)
      {
      // s.casazza:Protected Area:...............\n

      UString line(100U);

      line.snprintf("%.*s:%.*s:", U_STRING_TO_TRACE(user), U_STRING_TO_TRACE(realm));

      uint32_t pos = htdigest->find(line);

      if (pos != U_NOT_FOUND)
         {
         pos += line.size();
         ha1  = htdigest->substr(pos, 32);

         U_INTERNAL_ASSERT_EQUALS(htdigest->c_char(pos + 32),'\n')
         }
      }

   U_RETURN_STRING(ha1);
}

bool UServer_Base::isUserAuthorized(const UString& user, const UString& password)
{
   U_TRACE(0, "UServer_Base::isUserAuthorized(%.*S,%.*S)", U_STRING_TO_TRACE(user), U_STRING_TO_TRACE(password))

#ifdef HAVE_SSL
   if (htpasswd)
      {
      UString line(100U), output(100U);

      UServices::generateDigest(U_HASH_SHA1, 0, password, output, true);

      line.snprintf("%.*s:{SHA}%.*s\n", U_STRING_TO_TRACE(user), U_STRING_TO_TRACE(output));

      if (htpasswd->find(line) != U_NOT_FOUND) U_RETURN(true);
      }
#endif

   U_RETURN(false);
}

#ifdef HAVE_LIBEVENT
void UServer_Base::operator()(int fd, short event)
{
   U_TRACE(0, "UServer_Base::operator()(%d,%hd)", fd, event)

   U_INTERNAL_ASSERT_EQUALS(event, EV_READ)

   if (handlerRead() == U_NOTIFIER_DELETE) delete this; // as UNotifier do...
}
#endif

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UServer_Base::dump(bool reset) const
{
   *UObjectIO::os << "port                    " << port                    << '\n'
                  << "flag_loop               " << flag_loop               << '\n'
                  << "verify_mode             " << verify_mode             << '\n'
                  << "req_timeout             " << req_timeout             << '\n'
                  << "cgi_timeout             " << cgi_timeout             << '\n'
                  << "verify_mode             " << verify_mode             << '\n'
                  << "log_file_sz             " << log_file_sz             << '\n'
                  << "num_connection          " << num_connection          << '\n'
                  << "preforked_num_kids      " << preforked_num_kids      << '\n'
                  << "digest_authentication   " << digest_authentication   << '\n'
                  << "log           (ULog     " << (void*)log              << ")\n"
                  << "socket        (USocket  " << (void*)socket           << ")\n"
                  << "host          (UString  " << (void*)host             << ")\n"
                  << "server        (UString  " << (void*)&server          << ")\n"
                  << "log_file      (UString  " << (void*)&log_file        << ")\n"
                  << "ca_file       (UString  " << (void*)&ca_file         << ")\n"
                  << "ca_path       (UString  " << (void*)&ca_path         << ")\n"
                  << "allow_IP      (UString  " << (void*)&allow_IP        << ")\n"
                  << "key_file      (UString  " << (void*)&key_file        << ")\n"
                  << "password      (UString  " << (void*)&password        << ")\n"
                  << "cert_file     (UString  " << (void*)&cert_file       << ")\n"
                  << "name_sock     (UString  " << (void*)&name_sock       << ")\n"
                  << "htpasswd      (UString  " << (void*)htpasswd         << ")\n"
                  << "htdigest      (UString  " << (void*)htdigest         << ")\n"
                  << "IP_address    (UString  " << (void*)&IP_address      << ")\n"
                  << "document_root (UString  " << (void*)&document_root   << ")\n"
                  << "proc          (UProcess " << (void*)proc             << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
