// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    server.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_SERVER_H
#define U_SERVER_H 1

#include <ulib/log.h>
#include <ulib/process.h>
#include <ulib/notifier.h>
#include <ulib/utility/interrupt.h>
#include <ulib/utility/socket_ext.h>
#include <ulib/net/server/client_image.h>
#include <ulib/net/server/server_plugin.h>

/**
   @class UServer

   @brief Handles incoming connections.

   The UServer class contains the methods needed to write a portable server.
   In general, a server listens for incoming network requests on a well-known
   IP address and port number. When a connection request is received,
   the UServer makes this connection available to the server program as a socket.
   The socket represents a two-way (full-duplex) connection with the client.

   In common with normal socket programming, the life-cycle of a UServer follows this basic course:
   1) bind() to an IP-address/port number and listen for incoming connections
   2) accept() a connection request
   3) deal with the request, or pass the created socket to another thread or process to be dealt with
   4) return to step 2 for the next client connection request
*/

// For example: U_MACROSERVER(UServerExample, UClientExample, UTCPSocket);
// ---------------------------------------------------------------------------------------------
#ifdef DEBUG
#  define U_MACROSERVER(server_class,client_class,type_socket) \
class server_class : public UServer<type_socket> { \
public: \
 server_class(UFileConfig* cfg) : UServer<type_socket>(cfg) { U_TRACE_REGISTER_OBJECT(  5, server_class, "%p", cfg) } \
~server_class()                                             { U_TRACE_UNREGISTER_OBJECT(5, server_class) } \
const char* dump(bool reset) const { return UServer<type_socket>::dump(reset); } \
protected: \
virtual void preallocate(uint32_t n) { \
U_TRACE(5, #server_class "::preallocate(%u)", n) \
vClientImage = U_NEW_VEC(n, client_class); } }
#else
#  define U_MACROSERVER(server_class,client_class,type_socket) \
class server_class : public UServer<type_socket> { \
public: \
 server_class(UFileConfig* cfg) : UServer<type_socket>(cfg) {} \
~server_class()                                             {} \
protected: \
virtual void preallocate(uint32_t n) { vClientImage = new client_class[n]; } }
#endif
// ---------------------------------------------------------------------------------------------

// manage write to log server

#define U_SRV_LOG(               fmt,args...)  { if (UServer_Base::isLog()) ULog::log("%s"fmt"\n",      UServer_Base::mod_name , ##args); }
#define U_SRV_LOG_WITH_ADDR(cimg,fmt,args...)  { if (UServer_Base::isLog()) ULog::log("%s"fmt" %.*s\n", UServer_Base::mod_name , ##args, \
                                                                   U_STRING_TO_TRACE(*((cimg)->logbuf))); }

#define U_SRV_LOG_TIMEOUT(cimg) U_SRV_LOG("client connected didn't send any request in %u secs (timeout), close connection %.*s", \
                                                   UServer_Base::getReqTimeout(), U_STRING_TO_TRACE(*((cimg)->logbuf)))

class UHTTP;
class UCommand;
class UFileConfig;
class USSIPlugIn;
class UHttpPlugIn;
class UFCGIPlugIn;
class USCGIPlugIn;
class UNoCatPlugIn;
class UClient_Base;
class UProxyPlugIn;
class UStreamPlugIn;
class UClientThread;

class U_EXPORT UServer_Base : public UEventFd {
public:

   // -----------------------------------------------------------------------------------------------------------------------------
   // UServer - configuration parameters
   // -----------------------------------------------------------------------------------------------------------------------------
   // USE_IPV6      flag indicating the use of ipv6
   // SERVER        host name or ip address for the listening socket
   // PORT          port number             for the listening socket
   // SOCKET_NAME   file name               for the listening socket
   // IP_ADDRESS    public ip address of host for the interface connected to the Internet (autodetected if not specified)
   // ALLOWED_IP    list of comma separated client address for IP-based access control (IPADDR[/MASK])
   //
   // LISTEN_BACKLOG       max number of ready to be delivered connections to accept()
   // USE_TCP_OPTIMIZATION flag indicating the use of TCP/IP options to optimize data transmission (DEFER_ACCEPT, QUICKACK)
   //
   // PID_FILE      write pid on file indicated
   // WELCOME_MSG   message of welcome to send initially to client
   // RUN_AS_USER   downgrade the security to that of user account
   // DOCUMENT_ROOT The directory out of which you will serve your documents
   //
   // LOG_FILE      locations for file log
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
   //                (Value <= 0 will disable Keep-Alive)
   //
   // DH_FILE       dh param (these are the bit DH parameters from "Assigned Number for SKIP Protocols")
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
   // -----------------------------------------------------------------------------------------------------------------------------

   static const UString* str_USE_IPV6;
   static const UString* str_PORT;
   static const UString* str_MSG_WELCOME;
   static const UString* str_COMMAND;
   static const UString* str_ENVIRONMENT;
   static const UString* str_RUN_AS_USER;
   static const UString* str_PREFORK_CHILD;
   static const UString* str_LOG_FILE;
   static const UString* str_LOG_FILE_SZ;
   static const UString* str_LOG_MSG_SIZE;
   static const UString* str_CERT_FILE;
   static const UString* str_KEY_FILE;
   static const UString* str_PASSWORD;
   static const UString* str_DH_FILE;
   static const UString* str_CA_FILE;
   static const UString* str_CA_PATH;
   static const UString* str_VERIFY_MODE;
   static const UString* str_ALLOWED_IP;
   static const UString* str_SOCKET_NAME;
   static const UString* str_DOCUMENT_ROOT;
   static const UString* str_PLUGIN;
   static const UString* str_PLUGIN_DIR;
   static const UString* str_REQ_TIMEOUT;
   static const UString* str_CGI_TIMEOUT;
   static const UString* str_VIRTUAL_HOST;
   static const UString* str_DIGEST_AUTHENTICATION;
   static const UString* str_URI;
   static const UString* str_HOST;
   static const UString* str_USER;
   static const UString* str_SERVER;
   static const UString* str_METHOD_NAME;
   static const UString* str_RESPONSE_TYPE;
   static const UString* str_IP_ADDRESS;
   static const UString* str_MAX_KEEP_ALIVE;
   static const UString* str_PID_FILE;
   static const UString* str_USE_TCP_OPTIMIZATION;
   static const UString* str_LISTEN_BACKLOG;

   static void str_allocate();

   // bound to the port number, etc...

          void init();
   static void run();
          void loadConfigParam(UFileConfig& file);

   // tipologia server...

   static int     getPort()       { return port; }
   static int     getCgiTimeout() { return cgi_timeout; }
   static int     getReqTimeout() { return (timeoutMS / 1000); }
   static bool    isIPv6()        { return UClientImage_Base::bIPv6; }
   static UString getHost()       { return *host; }

   // The directory out of which you will serve your documents...

   UString document_root;

   static UString getDocumentRoot()
      {
      U_TRACE(0, "UServer_Base::getDocumentRoot()")

      U_INTERNAL_ASSERT_POINTER(pthis)

      U_RETURN_STRING(pthis->document_root);
      }

   static bool isFileInsideDocumentRoot(const UString& path)
      {
      U_TRACE(0, "UServer_Base::isFileInsideDocumentRoot(%.*S)", U_STRING_TO_TRACE(path))

      U_INTERNAL_ASSERT_POINTER(pthis)

      bool result = (path.size() >= pthis->document_root.size());

      U_RETURN(result);
      }

   // -------------------------------------------------------------------
   // MANAGE PLUGIN MODULES
   // -------------------------------------------------------------------

   static char mod_name[20];
   static UEventFd* handler_event;

   // load plugin modules and call server-wide hooks handlerConfig()...
   static int loadPlugins(UString& plugin_dir, const UString& plugin_list, UFileConfig* cfg);

   // Server-wide hooks
   static int pluginsHandlerInit();

   // Connection-wide hooks
   static int pluginsHandlerREAD();
   static int pluginsHandlerRequest();
   static int pluginsHandlerReset();

   // ----------------------------------------------------------------------------------------------------------------------------
   // Manage process server
   // ----------------------------------------------------------------------------------------------------------------------------
   // PREFORK_CHILD number of child server processes created at startup: -1 - thread approach (experimental)
   //                                                                     0 - serialize, no forking
   //                                                                     1 - classic, forking after client accept
   //                                                                    >1 - pool of serialized processes plus monitoring process
   // ----------------------------------------------------------------------------------------------------------------------------

   typedef struct shared_data {
      struct timeval _timeval;
      sig_atomic_t tot_connection;
   } shared_data;

   static int preforked_num_kids; // keeping a pool of children and that they accept connections themselves
   static uint32_t shared_data_add;
   static shared_data* ptr_shared_data;

   static bool isPreForked()
      {
      U_TRACE(0, "UServer_Base::isPreForked()")

      U_INTERNAL_DUMP("preforked_num_kids = %d", preforked_num_kids)

      bool result = (preforked_num_kids > 1);

      U_RETURN(result);
      }

   static bool isClassic()
      {
      U_TRACE(0, "UServer_Base::isClassic()")

      U_INTERNAL_DUMP("preforked_num_kids = %d", preforked_num_kids)

      bool result = (preforked_num_kids ==  1);

      U_RETURN(result);
      }

   static bool isChild()
      {
      U_TRACE(0, "UServer_Base::isChild()")

      U_INTERNAL_DUMP("preforked_num_kids = %d", preforked_num_kids)

      bool result = (preforked_num_kids >= 1 && proc->child());

      U_RETURN(result);
      }

   static void sendSigTERM()
      {
      U_TRACE(0, "UServer_Base::sendSigTERM()")

      UProcess::kill(0, SIGTERM); // SIGTERM is sent to every process in the process group of the calling process...
      }

   // NB: if server with no prefork (ex: nodog) process the HTTP CGI request with fork....

   static bool parallelization(); // it creates a copy of itself, return true if parent...

   static bool isParallelization()
      {
      U_TRACE(0, "UServer_Base::isParallelization()")

      U_INTERNAL_DUMP("flag_loop = %b", flag_loop)

      U_INTERNAL_ASSERT_EQUALS(flag_loop, false)

      bool result = (preforked_num_kids <= 0 && proc->child());

      U_RETURN(result);
      }

   // manage log server...

   static ULog* log;

   static bool isLog() { return (log != 0); }

   static void openLog(const UString& name, uint32_t size)
      {
      U_TRACE(0, "UServer_Base::openLog(%.*S,%u)", U_STRING_TO_TRACE(name), size)

      log = U_NEW(ULog(name, size));

      if (preforked_num_kids) ULog::setServer(true);
      else                    ULog::setClient();
      }

   static void      logCommandMsgError(const char* cmd);
   static UCommand* loadConfigCommand(UFileConfig& cfg);

   // NETWORK CTX

   static UString getIPAddress()                              { return pthis->IP_address; }
   static UString getMacAddress(    const char* device_or_ip) { return USocketExt::getMacAddress(socket->getFd(), device_or_ip); }
   static UString getNetworkAddress(const char* device)       { return USocketExt::getNetworkAddress(socket->getFd(), device); }
   static UString getNetworkDevice( const char* exclude)      { return USocketExt::getNetworkDevice(exclude); }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UString server,      // host name or ip address for the listening socket
           as_user,     // change the current working directory to the user's home dir, and downgrade security to that user account
           log_file,    // locations for file log
           dh_file,     // These are the 1024 bit DH parameters from "Assigned Number for SKIP Protocols"
           cert_file,   // locations for certificate of server
           key_file,    // locations for private key of server
           password,    // password for private key of server
           ca_file,     // locations of trusted CA certificates used in the verification
           ca_path,     // locations of trusted CA certificates used in the verification
           allow_IP,    // Interpret a "HOST/BITS" IP mask specification. (Ex. 192.168.1.64/28)
           name_sock,   // name file for the listening socket
           IP_address;  // IP address of this server

   static int port,           // the port number to bind to
              iBackLog,       // max number of ready to be delivered connections to accept()
              timeoutMS,      // the time-out value in milliseconds for client request
              cgi_timeout,    // the time-out value in seconds for output cgi process
              verify_mode,    // mode of verification ssl connection
              max_Keep_Alive, // Specifies the maximum number of requests that can be served through a Keep-Alive (Persistent) session
              num_connection;

   static UString* host;
   static UProcess* proc;
   static USocket* socket;
   static UEventTime* ptime;
   static UServer_Base* pthis;
   static UString* senvironment;
   static int start_index_reuse_object;
   static UVector<UIPAllow*>* vallow_IP;
   static UClientImage_Base* vClientImage;
   static bool flag_loop, flag_use_tcp_optimization;

   // COSTRUTTORI

            UServer_Base(UFileConfig* cfg);
   virtual ~UServer_Base();

   // VARIE

   class U_NO_EXPORT UTimeoutConnection : public UEventTime {
   public:

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UTimeoutConnection() : UEventTime(getReqTimeout(), 0L)
      {
      U_TRACE_REGISTER_OBJECT(0, UTimeoutConnection, "")
      }

   virtual ~UTimeoutConnection()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UTimeoutConnection)
      }

   // define method VIRTUAL of class UEventTime

   virtual int handlerTime()
      {
      U_TRACE(0, "UTimeoutConnection::handlerTime()")

      // there are idle connection... (timeout)

      UNotifier::callForAllEntry(handlerTimeoutConnection);

      // return value:
      // ---------------
      // -1 - normal
      //  0 - monitoring
      // ---------------

      U_RETURN(handler_event ? -1 : 0);
      }

#  ifdef DEBUG
   const char* dump(bool _reset) const { return UEventTime::dump(_reset); }
#  endif

   private:
   UTimeoutConnection(const UTimeoutConnection&) : UEventTime() {}
   UTimeoutConnection& operator=(const UTimeoutConnection&)     { return *this; }
   };

   static void handlerNewConnection();
   static bool handlerTimeoutConnection(void* cimg);
   static void handlerCloseConnection(UClientImage_Base* ptr);

   static void        runAsUser();
   static const char* getNumConnection();

   static UVector<UString>* vplugin_name;
   static UVector<UServerPlugIn*>* vplugin;

   // define method VIRTUAL of class UEventFd

   virtual int handlerRead(); // This method is called to accept a new connection on the server socket

   // method VIRTUAL to redefine

   virtual void handlerSignal()
      {
      U_TRACE(0, "UServer_Base::handlerSignal()")
      }

   virtual void preallocate(uint32_t n) = 0;

   // SERVICES

   static RETSIGTYPE handlerForSigHUP( int signo);
   static RETSIGTYPE handlerForSigTERM(int signo);

private:
   friend class UHTTP;
   friend class USSIPlugIn;
   friend class UHttpPlugIn;
   friend class USCGIPlugIn;
   friend class UFCGIPlugIn;
   friend class UProxyPlugIn;
   friend class UNoCatPlugIn;
   friend class UClient_Base;
   friend class UStreamPlugIn;
   friend class UClientThread;
   friend class UClientImage_Base;

   static void loadStaticLinkedModules(const char* name) U_NO_EXPORT;

   UServer_Base(const UServer_Base&) : UEventFd() {}
   UServer_Base& operator=(const UServer_Base&)   { return *this; }
};

template <class Socket> class U_EXPORT UServer : public UServer_Base {
public:

   UServer(UFileConfig* cfg) : UServer_Base(cfg)
      {
      U_TRACE_REGISTER_OBJECT(0, UServer, "%p", cfg)

      socket =                U_NEW(Socket(UClientImage_Base::bIPv6));
      UClientImage_Base::init(U_NEW(Socket(UClientImage_Base::bIPv6)));
      }

   virtual ~UServer()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UServer)
      }

   // MANAGE ALL...

   void go()
      {
      U_TRACE(0, "UServer<Socket>::go()")

      UServer_Base::run(); // loop waiting for connection
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return UServer_Base::dump(reset); }
#endif

protected:

   // method VIRTUAL to redefine

   /**
   create a new UClientImage object representing a client connection to the server.
   Derived classes that have overridden UClientImage object may call this function to implement the creation logic
   */

   virtual void preallocate(uint32_t n)
      {
      U_TRACE(0+256, "UServer<Socket>::preallocate(%u)", n)

      vClientImage = U_NEW_VEC(n, UClientImage<Socket>);
      }

private:
   UServer(const UServer&) : UServer_Base(0) {}
   UServer& operator=(const UServer&)        { return *this; }
};

#ifdef HAVE_SSL // specializzazione con USSLSocket

template <> class U_EXPORT UServer<USSLSocket> : public UServer_Base {
public:

   UServer(UFileConfig* cfg) : UServer_Base(cfg)
      {
      U_TRACE_REGISTER_OBJECT(0, UServer<USSLSocket>, "%p", cfg)

      socket =                U_NEW(USSLSocket(UClientImage_Base::bIPv6));
      UClientImage_Base::init(U_NEW(USSLSocket(UClientImage_Base::bIPv6, getSocket()->ctx)));
      }

   virtual ~UServer()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UServer<USSLSocket>)
      }

   // SERVICES

   static USSLSocket* getSocket() { return (USSLSocket*) pthis->socket; }

   bool askForClientCertificate()
      {
      U_TRACE(0, "UServer<USSLSocket>::askForClientCertificate()")

      USSLSocket* sslsocket = (USSLSocket*)UClientImage_Base::socket;

      U_INTERNAL_ASSERT(sslsocket->isSSL())

      bool has_cert = (sslsocket->getPeerCertificate() != 0);

      if (has_cert == false)
         {
         U_SRV_LOG_WITH_ADDR(UClientImage_Base::pClientImage, "ask for a client certificate to");

         if (sslsocket->askForClientCertificate())
            {
            has_cert = true;

            UClientImage_Base::pClientImage->logCertificate(sslsocket->getPeerCertificate());
            }
         }

      U_RETURN(has_cert);
      }

   // MANAGE ALL...

   void go()
      {
      U_TRACE(0, "UServer<USSLSocket>::go()")

      U_INTERNAL_ASSERT(getSocket()->isSSL())
      U_INTERNAL_ASSERT(  dh_file.isNullTerminated())
      U_INTERNAL_ASSERT(  ca_file.isNullTerminated())
      U_INTERNAL_ASSERT(  ca_path.isNullTerminated())
      U_INTERNAL_ASSERT( key_file.isNullTerminated())
      U_INTERNAL_ASSERT( password.isNullTerminated())
      U_INTERNAL_ASSERT(cert_file.isNullTerminated())

      // Load our certificate

      if (getSocket()->setContext(dh_file.data(), cert_file.data(), key_file.data(), password.data(), ca_file.data(),  ca_path.data(), verify_mode) == false)
         {
         U_ERROR("SSL setContext() failed...");
         }

      UServer_Base::run(); // loop waiting for connection
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return UServer_Base::dump(reset); }
#endif

protected:

   // method VIRTUAL to redefine

   /**
   create a new UClientImage object representing a client connection to the server.
   Derived classes that have overridden UClientImage object may call this function to implement the creation logic
   */

   virtual void preallocate(uint32_t n)
      {
      U_TRACE(0+256, "UServer<USSLSocket>::preallocate(%u)", n)

      vClientImage = U_NEW_VEC(n, UClientImage<USSLSocket>);
      }

private:
   UServer<USSLSocket>(const UServer<USSLSocket>&) : UServer_Base(0) {}
   UServer<USSLSocket>& operator=(const UServer<USSLSocket>&)        { return *this; }
};

#endif
#endif
