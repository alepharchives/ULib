// userver.cpp

#include <ulib/file_config.h>

#if defined(U_SSL_SOCKET)
#  include <ulib/ssl/net/sslsocket.h>
#  define Server UServer<USSLSocket>
#elif defined(U_TCP_SOCKET)
#  include <ulib/net/tcpsocket.h>
#  define Server UServer<UTCPSocket>
#elif defined(U_UNIX_SOCKET)
#  include <ulib/net/unixsocket.h>
#  define Server UServer<UUnixSocket>
#else
#  error "you must define the socket type (U_SSL_SOCKET | U_TCP_SOCKET | U_UNIX_SOCKET)"
#endif

#include <ulib/net/server/server.h>

#undef  PACKAGE
#define PACKAGE "userver"
#undef  ARGS
#define ARGS ""

#define U_OPTIONS \
"purpose 'application server by ULib' \n" \
"option c config 1 'path of configuration file' ''\n"

#include <ulib/application.h>

class Application : public UApplication {
public:

   ~Application()
      {
      U_TRACE(5, "Application::~Application()")

      delete server;
      }

   void run(int argc, char* argv[], char* env[])
      {
      U_TRACE(5, "Application::run(%d,%p,%p)", argc, argv, env)

      UApplication::run(argc, argv, env);

      // manage options

      UString cfg_str;

      if (UApplication::isOptions()) cfg_str = opt['c'];

      // The "hash seed" is a feature to perturb the results to avoid "algorithmic complexity attacks"
      // ---------------------------------------------------------------------------------------------
      // http://lwn.net/Articles/474365/

      u_init_ulib_seed_hash();

      // manage file configuration

      if (cfg_str.empty()) cfg_str = U_STRING_FROM_CONSTANT(U_SYSCONFDIR "/userver.cfg");

      // ---------------------------------------------------------------------------------------------------------------------------------------
      // userver - configuration parameters
      // ---------------------------------------------------------------------------------------------------------------------------------------
      // ENABLE_IPV6    flag to indicate use of ipv6
      // SERVER         host name or ip address for the listening socket
      // PORT           port number for the listening socket
      // SOCKET_NAME    file name   for the listening socket
      // IP_ADDRESS     public ip address of host for the interface connected to the Internet (autodetected if not specified)
      // ALLOWED_IP     list of comma separated client address for IP-based access control (IPADDR[/MASK])
      //
      // LISTEN_BACKLOG        max number of ready to be delivered connections to accept()
      // USE_TCP_OPTIMIZATION  flag indicating the use of TCP/IP options to optimize data transmission (TCP_CORK, TCP_DEFER_ACCEPT, TCP_QUICKACK)
      // SET_REALTIME_PRIORITY flag indicating that the preforked processes will be scheduled under the real-time policies SCHED_FIFO
      //
      // PID_FILE       write pid on file indicated
      // WELCOME_MSG    message of welcome to send initially to client
      // RUN_AS_USER    downgrade security to that user account
      // DOCUMENT_ROOT  The directory out of which you will serve your documents
      //
      // LOG_FILE       locations for file log
      // LOG_FILE_SZ    memory size for file log
      // LOG_MSG_SIZE  limit length of print network message to LOG_MSG_SIZE chars (default 128)
      //
      // PLUGIN         list of plugins to load, a flexible way to add specific functionality to the server
      // PLUGIN_DIR     directory of plugins to load
      //
      // REQ_TIMEOUT    timeout for request from client
      // CGI_TIMEOUT    timeout for cgi execution
      //
      // MAX_KEEP_ALIVE Specifies the maximum number of requests that can be served through a Keep-Alive (Persistent) session.
      //                (Value <= 0 will disable Keep-Alive) (default 1020)
      //
      // DH_FILE        DH param
      // CERT_FILE      certificate of server
      // KEY_FILE       private key of server
      // PASSWORD       password for private key of server
      // CA_FILE        locations of trusted CA certificates used in the verification
      // CA_PATH        locations of trusted CA certificates used in the verification
      // VERIFY_MODE    mode of verification (SSL_VERIFY_NONE=0, SSL_VERIFY_PEER=1, SSL_VERIFY_FAIL_IF_NO_PEER_CERT=2, SSL_VERIFY_CLIENT_ONCE=4)
      //
      // PREFORK_CHILD  number of child server processes created at startup ( 0 - serialize, no forking
      //                                                                      1 - classic, forking after accept client)
      //                                                                     >1 - pool of process serialize plus monitoring process)
      // ---------------------------------------------------------------------------------------------------------------------------------------

      (void) cfg.open(cfg_str);

      server = new Server(&cfg);

      server->go();
      }

private:
   Server* server;
   UFileConfig cfg; // NB: we put this here to avoid unnecessary destructor at runtime...
};

U_MAIN(Application)
