// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    client.cpp - Handles a connections with a server
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file_config.h>
#include <ulib/net/client/client.h>
#include <ulib/net/server/server.h>

bool  UClient_Base::log_shared_with_server;
ULog* UClient_Base::log;

const UString* UClient_Base::str_RES_TIMEOUT;

void UClient_Base::str_allocate()
{
   U_TRACE(0+256, "UClient_Base::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_RES_TIMEOUT,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("RES_TIMEOUT") }
   };

   U_NEW_ULIB_OBJECT(str_RES_TIMEOUT, U_STRING_FROM_STRINGREP_STORAGE(0));
}

UClient_Base::UClient_Base() : response(U_CAPACITY), buffer(U_CAPACITY), host_port(100U), logbuf(100U)
{
   U_TRACE_REGISTER_OBJECT(0, UClient_Base, "")

   bIPv6     = false;
   port      = verify_mode = 0;
   timeoutMS = U_TIMEOUT_MS;
}

UClient_Base::UClient_Base(UFileConfig* cfg) : response(U_CAPACITY), buffer(U_CAPACITY), host_port(100U), logbuf(100U)
{
   U_TRACE_REGISTER_OBJECT(0, UClient_Base, "%p", cfg)

   u_init_ulib_hostname();

   U_INTERNAL_DUMP("u_hostname(%u) = %.*S", u_hostname_len, u_hostname_len, u_hostname)

   u_init_ulib_username();

   U_INTERNAL_DUMP("u_user_name(%u) = %.*S", u_user_name_len, u_user_name_len, u_user_name)

   timeoutMS = U_TIMEOUT_MS;

   if (cfg) loadConfigParam(*cfg);
   else
      {
      bIPv6 = false;
      port  = verify_mode = 0;
      }
}

UClient_Base::~UClient_Base()
{
   U_TRACE_UNREGISTER_OBJECT(0, UClient_Base)

   U_INTERNAL_ASSERT_POINTER(socket)

   if (log &&
       log_shared_with_server == false)
      {
      delete log;
      }

   delete socket;

#ifdef DEBUG
        uri.clear(); // uri can depend on url...
        url.clear(); // url can depend on response... (Location: xxx)
   response.clear(); // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE... (response may be substr of buffer)
#endif
}

void UClient_Base::clearData()
{
   U_TRACE(0, "UClient_Base::clearData()")

     buffer.setEmpty();
   response.setEmpty();
}

bool UClient_Base::setHostPort(const UString& host, int _port)
{
   U_TRACE(0, "UClient_Base::setHostPort(%.*S,%d)", U_STRING_TO_TRACE(host), _port)

   U_INTERNAL_ASSERT(host)

   bool host_differs = (host  != server),
        port_differs = (_port != port);

   U_INTERNAL_DUMP("host_differs = %b port_differs = %b", host_differs, port_differs)

   server = host.copy(); // NB: we cannot depend on url...
   port   = _port;

   // If the URL contains a port, then add that to the Host header

   if (host_differs ||
       port_differs)
      {
      host_port.replace(host);

      if (_port &&
          _port != 80)
         {
         char tmp[6];
         int size = u__snprintf(tmp, sizeof(tmp), "%d", _port);

         host_port.push_back(':');
         host_port.append(tmp, size);
         }

      U_INTERNAL_DUMP("host_port = %.*S", U_STRING_TO_TRACE(host_port))

      U_RETURN(true);
      }

   U_RETURN(false);
}

void UClient_Base::setLogShared()
{
   U_TRACE(0, "UClient_Base::setLogShared()")

   U_INTERNAL_ASSERT_POINTER(UServer_Base::log)

   log                    = UServer_Base::log;
   log_shared_with_server = true;
}

void UClient_Base::loadConfigParam(UFileConfig& cfg)
{
   U_TRACE(0, "UClient_Base::loadConfigParam(%p)", &cfg)

   U_ASSERT_EQUALS(cfg.empty(), false)

   if (UServer_Base::str_LOG_FILE    == 0) UServer_Base::str_allocate();
   if (              str_RES_TIMEOUT == 0)               str_allocate();

   // ----------------------------------------------------------------------------------------------------------------------
   // client - configuration parameters
   // ----------------------------------------------------------------------------------------------------------------------
   // SOCKET_NAME   name file for the listening socket
   //
   // ENABLE_IPV6   flag to indicate use of ipv6
   // SERVER        host name or ip address for server
   // PORT          port number for the server
   //
   // RES_TIMEOUT   timeout for response from server
   //
   // LOG_FILE      locations   for file log
   // LOG_FILE_SZ   memory size for file log
   //
   // CERT_FILE     certificate of client
   // KEY_FILE      private key of client
   // PASSWORD      password for private key of client
   // CA_FILE       locations of trusted CA certificates used in the verification
   // CA_PATH       locations of trusted CA certificates used in the verification
   // VERIFY_MODE   mode of verification (SSL_VERIFY_NONE=0, SSL_VERIFY_PEER=1,
   //                                     SSL_VERIFY_FAIL_IF_NO_PEER_CERT=2, SSL_VERIFY_CLIENT_ONCE=4)
   // ----------------------------------------------------------------------------------------------------------------------

   ca_file   = cfg[*UServer_Base::str_CA_FILE];
   ca_path   = cfg[*UServer_Base::str_CA_PATH];
   key_file  = cfg[*UServer_Base::str_KEY_FILE];
   password  = cfg[*UServer_Base::str_PASSWORD];
   cert_file = cfg[*UServer_Base::str_CERT_FILE];
   log_file  = cfg[*UServer_Base::str_LOG_FILE];

   if (log_file.empty() == false)
      {
      if (UServer_Base::isLog() == false) log = U_NEW(ULog(log_file, cfg.readLong(*UServer_Base::str_LOG_FILE_SZ), "(pid %P) %10D> "));
      else
         {
         U_ASSERT_EQUALS(log_file, *UServer_Base::pthis->log_file)

         setLogShared();

         log->startup();
         }
      }

#ifdef ENABLE_IPV6
   bIPv6       = cfg.readBoolean(*UServer_Base::str_ENABLE_IPV6);
#endif
   verify_mode = cfg.readLong(*UServer_Base::str_VERIFY_MODE);

   UString host      = cfg[*UServer_Base::str_SERVER],
           name_sock = cfg[*UServer_Base::str_SOCKET_NAME];

   if (     host.empty() == false ||
       name_sock.empty() == false)
      {
      (void) setHostPort(name_sock.empty() ? host : name_sock, cfg.readLong(*UServer_Base::str_PORT));
      }

   UString value = cfg[*str_RES_TIMEOUT];

   if (value.empty() == false)
      {
      timeoutMS = value.strtol();

      if (timeoutMS) timeoutMS *= 1000;
      else           timeoutMS  = -1;
      }

   U_INTERNAL_DUMP("timeoutMS = %u", timeoutMS)

// cfg.clear();
// cfg.deallocate();
}

bool UClient_Base::connect()
{
   U_TRACE(0, "UClient_Base::connect()")

   U_INTERNAL_ASSERT_EQUALS(socket->isConnected(), false)  // Guard against multiple connections

   if (socket->connectServer(server, port))
      {
      if (log)
         {
         if (port)
            {
            logbuf.setEmpty();

            U_ipaddress_StrAddressUnresolved(&(socket->cRemoteAddress)) = true;

            socket->cRemoteAddress.resolveStrAddress();

            USocketExt::setRemoteInfo(socket, logbuf);
            }
         else if (logbuf.empty())
            {
            logbuf = '"' + host_port + '"';
            }
         }

      U_RETURN(true);
      }

   response.setBuffer(100U);

   response.snprintf("Sorry, couldn't connect to server %.*S%R", U_STRING_TO_TRACE(host_port), 0); // NB: the last argument (0) is necessary...

   if (log)
      {
      ULog::log("%s%.*s\n",
                  log_shared_with_server ? UServer_Base::mod_name->data() : "",
                  U_STRING_TO_TRACE(response));
      }

   U_RETURN(false);
}

bool UClient_Base::setUrl(const UString& location)
{
   U_TRACE(0, "UClient_Base::setUrl(%.*S)", U_STRING_TO_TRACE(location))

   // check we've been passed a absolute URL

   if (u_isUrlScheme(U_STRING_TO_PARAM(location)) == 0)
      {
      char* p;
      char* ptr;
      char* dest;
      uint32_t len;
      char buf[U_PATH_MAX];

      const char*  src =       uri.data();
      const char* _end = src + uri.size();

      U_INTERNAL_DUMP("uri = %.*S", U_STRING_TO_TRACE(uri))

      ptr = dest = buf;

      while (src < _end)
         {
         p = (char*) memchr(src, '/', _end - src);

         if (p == 0) break;

         len = p - src + 1;

         U_INTERNAL_DUMP("segment = %.*S", len, src)

         U__MEMCPY(dest, src, len);

         src   = p + 1;
         dest += len;
         }

      len = location.size();

      U__MEMCPY(dest, location.data(), len);

      (void) uri.replace(buf, dest - ptr + len);

      U_INTERNAL_DUMP("uri = %.*S", U_STRING_TO_TRACE(uri))

      U_RETURN(false);
      }

   url.set(location);

#ifdef USE_LIBSSL
   if (socket->isSSL()) ((USSLSocket*)socket)->setActive(url.isHTTPS());
#endif

   uri = url.getPathAndQuery();

   U_INTERNAL_DUMP("uri = %.*S", U_STRING_TO_TRACE(uri))

   bool bchange = setHostPort(url.getHost(), url.getPort());

   U_RETURN(bchange);
}

UString UClient_Base::wrapRequestWithHTTP(UString* req,
                                          const char* method, uint32_t method_len,
                                          const char* _uri,   uint32_t uri_len,
                                          const char* extension, const char* content_type)
{
   U_TRACE(0, "UClient_Base::wrapRequestWithHTTP(%p,%.*S,%u,%.*S,%u,%S,%S)",req,method_len,method,method_len,uri_len,_uri,uri_len,extension,content_type)

   // Add the MIME-type headers to the request for HTTP server

   UString tmp(800U + uri_len + (req ? req->size() : 0));

   tmp.snprintf("%.*s %.*s HTTP/1.1\r\n"
                "Host: %.*s\r\n"
                "User-Agent: " PACKAGE_NAME "/" ULIB_VERSION "\r\n"
                "%s",
                method_len, method, 
                uri_len, _uri,
                U_STRING_TO_TRACE(host_port),
                extension);

   if (req)
      {
      U_INTERNAL_ASSERT(*req)
      U_INTERNAL_ASSERT_POINTER(content_type)

      tmp.snprintf_add("Content-Type: %s\r\n"
                       "Content-Length: %u\r\n"
                       "\r\n",
                       content_type,
                       req->size());

      (void) tmp.append(*req);
      }

   U_RETURN_STRING(tmp);
}

bool UClient_Base::sendRequest(struct iovec* iov, int iovcnt, bool bread_response)
{
   U_TRACE(0, "UClient_Base::sendRequest(%p,%d,%b)", iov, iovcnt, bread_response)

   bool result;
   int i, iov_len[128], counter = 0, ncount = 0;

   for (i = 0; i < iovcnt; ++i) ncount += (iov_len[i] = iov[i].iov_len);

send:
   result = false;

   for (; counter < 2; ++counter)
      {
      if (isConnected() ||
          UClient_Base::connect())
         {
         result = (USocketExt::writev(socket, iov, iovcnt, ncount, timeoutMS) == ncount);

         if (result) break;

              if (isConnected()) close();
         else if (log)
            {
            ULog::log("%sconnection to %.*s reset by peer%R\n",
                      log_shared_with_server ? UServer_Base::mod_name->data() : "",
                      U_STRING_TO_TRACE(logbuf), 0); // NB: the last argument (0) is necessary...
            }

         for (i = 0; i < iovcnt; ++i) iov[i].iov_len = iov_len[i]; // NB: we need to reset a partial write...
         }
      }

   U_INTERNAL_DUMP("result = %b", result)

   if (result)
      {
      if (bread_response                                                        &&
          USocketExt::read(socket, response, U_SINGLE_READ, timeoutMS) == false &&
          isConnected()                                                == false)
         {
         if (log)
            {
            ULog::log("%sconnection to %.*s reset by peer%R\n",
                     log_shared_with_server ? UServer_Base::mod_name->data() : "",
                     U_STRING_TO_TRACE(logbuf), 0); // NB: the last argument (0) is necessary...
            }

         if (++counter < 2 &&
             (log_shared_with_server == false || // check for SIGTERM event...
              UServer_Base::flag_loop))
            {
            if (log) errno = 0;

            goto send;
            }

         U_RETURN(false);
         }

      if (log)
         {
         ULog::log("%ssend request (%u bytes) %.*S to %.*s\n",
                     log_shared_with_server ? UServer_Base::mod_name->data() : "",
                     ncount, iov[0].iov_len, iov[0].iov_base, U_STRING_TO_TRACE(logbuf));

         if (bread_response) logResponse(response);
         }

      reset();

      U_RETURN(true);
      }

   U_RETURN(false);
}

// read data response

bool UClient_Base::readResponse(int count)
{
   U_TRACE(0, "UClient_Base::readResponse(%d)", count)

   if (USocketExt::read(socket, response, count, timeoutMS))
      {
      if (log) logResponse(response);

      U_RETURN(true);
      }

   U_RETURN(false);
}

void UClient_Base::logResponse(const UString& data)
{
   U_TRACE(0, "UClient_Base::logResponse(%.*S)", U_STRING_TO_TRACE(data))

   U_INTERNAL_ASSERT_POINTER(log)

   ULog::log("%sreceived response (%u bytes) %.*S from %.*s\n",
                  log_shared_with_server ? UServer_Base::mod_name->data() : "",
                  data.size(), U_STRING_TO_TRACE(data), U_STRING_TO_TRACE(logbuf));
}

bool UClient_Base::readHTTPResponse()
{
   U_TRACE(0, "UClient_Base::readHTTPResponse()")

   // read HTTP message data

   clearData();

   if (UHTTP::readHeader(socket, buffer) &&
       UHTTP::findEndHeader(     buffer))
      {
      uint32_t pos = buffer.find(*USocket::str_content_length, u_http_info.startHeader, u_http_info.szHeader);

      if (pos != U_NOT_FOUND)
         {
         u_http_info.clength = (uint32_t) strtoul(buffer.c_pointer(pos + USocket::str_content_length->size() + 2), 0, 0);

         if (UHTTP::readBody(socket, &buffer, response))
            {
            if (log) logResponse(buffer);

            U_RETURN(true);
            }
         }
      }

   U_RETURN(false);
}

bool UClient_Base::readRPCResponse()
{
   U_TRACE(0, "UClient_Base::readRPCResponse()")

   // NB: we force for U_SUBSTR_INC_REF case (string can be referenced more)...

     buffer.setEmptyForce();
   response.setEmptyForce();

   uint32_t rstart = 0;

   if (URPC::readTokenString(socket, 0, buffer, rstart, response))
      {
      // NB: we force for U_SUBSTR_INC_REF case (string can be referenced more)...

      buffer.size_adjust_force(U_TOKEN_NM);
      }

   U_INTERNAL_DUMP("buffer = %.*S response = %.*S)", U_STRING_TO_TRACE(buffer), U_STRING_TO_TRACE(response))

   bool result = (buffer.empty() == false);

   U_RETURN(result);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UClient_Base::dump(bool _reset) const
{
   *UObjectIO::os << "bIPv6                               " << bIPv6                   << '\n'
                  << "port                                " << port                    << '\n'
                  << "timeoutMS                           " << timeoutMS               << '\n'
                  << "verify_mode                         " << verify_mode             << '\n'
                  << "log_shared_with_server              " << log_shared_with_server  << '\n'
                  << "log            (ULog                " << (void*)log              << ")\n"
                  << "uri            (UString             " << (void*)&uri             << ")\n"
                  << "server         (UString             " << (void*)&server          << ")\n"
                  << "ca_file        (UString             " << (void*)&ca_file         << ")\n"
                  << "ca_path        (UString             " << (void*)&ca_path         << ")\n"
                  << "logbuf         (UString             " << (void*)&logbuf          << ")\n"
                  << "key_file       (UString             " << (void*)&key_file        << ")\n"
                  << "password       (UString             " << (void*)&password        << ")\n"
                  << "log_file       (UString             " << (void*)&log_file        << ")\n"
                  << "cert_file      (UString             " << (void*)&cert_file       << ")\n"
                  << "buffer         (UString             " << (void*)&buffer          << ")\n"
                  << "response       (UString             " << (void*)&response        << ")\n"
                  << "host_port      (UString             " << (void*)&host_port       << ")\n"
                  << "socket         (USocket             " << (void*)socket           << ')';

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
