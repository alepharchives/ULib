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

int   UClient_Base::log_file_sz;
ULog* UClient_Base::log;

UClient_Base::UClient_Base(UFileConfig* cfg) : response(U_CAPACITY), buffer(U_CAPACITY), host_port(100U), logbuf(100U)
{
   U_TRACE_REGISTER_OBJECT(0, UClient_Base, "%p", cfg)

   timeoutMS = U_TIMEOUT;

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

   if (log) delete log;

   delete socket;

#ifdef DEBUG
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

   U_ASSERT(host.empty() == false)

   bool host_differs = (host  != server),
        port_differs = (_port != port);

   U_INTERNAL_DUMP("host_differs = %b port_differs = %b", host_differs, port_differs)

   server = host;
   port   = _port;

   // If the URL contains a port, then add that to the Host header

   if (host_differs ||
       port_differs)
      {
      host_port.replace(host);

      if (_port != 80)
         {
         char tmp[6];
         int size = snprintf(tmp, sizeof(tmp), "%d", _port);

         host_port.push_back(':');
         host_port.append(tmp, size);
         }

      U_INTERNAL_DUMP("host_port = %.*S", U_STRING_TO_TRACE(host_port));

      U_RETURN(true);
      }

   U_RETURN(false);
}

void UClient_Base::loadConfigParam(UFileConfig& cfg)
{
   U_TRACE(0, "UClient_Base::loadConfigParam(%p)", &cfg)

   if (Url::str_ftp == 0) Url::str_allocate();
                 UServer_Base::str_allocate();

   U_ASSERT_EQUALS(cfg.empty(), false)

   // ----------------------------------------------------------------------------------------------------------------------
   // client - configuration parameters
   // ----------------------------------------------------------------------------------------------------------------------
   // NAME_SOCKET   name file for the listening socket
   // USE_IPV6      flag to indicate use of ipv6
   // SERVER        host name or ip address for server
   // PORT          port number for the server
   // CERT_FILE     certificate of client
   // KEY_FILE      private key of client
   // PASSWORD      password for private key of client
   // CA_FILE       locations of trusted CA certificates used in the verification
   // CA_PATH       locations of trusted CA certificates used in the verification
   // VERIFY_MODE   mode of verification (SSL_VERIFY_NONE=0, SSL_VERIFY_PEER=1,
   //                                     SSL_VERIFY_FAIL_IF_NO_PEER_CERT=2, SSL_VERIFY_CLIENT_ONCE=4)
   //
   // LOG_FILE      locations   for file log
   // LOG_FILE_SZ   memory size for file log
   // ----------------------------------------------------------------------------------------------------------------------

   ca_file   = cfg[*UServer_Base::str_CA_FILE];
   ca_path   = cfg[*UServer_Base::str_CA_PATH];
   key_file  = cfg[*UServer_Base::str_KEY_FILE];
   password  = cfg[*UServer_Base::str_PASSWORD];
   cert_file = cfg[*UServer_Base::str_CERT_FILE];
   name_sock = cfg[*UServer_Base::str_NAME_SOCKET];

   log_file    = cfg[*UServer_Base::str_LOG_FILE];
   log_file_sz = cfg.readLong(*UServer_Base::str_LOG_FILE_SZ);

   if (log_file.empty() == false)
      {
      log = U_NEW(ULog(log_file, log_file_sz));

      ULog::setServer(false);
      }

#ifdef HAVE_IPV6
   bIPv6       = cfg.readBoolean(*UServer_Base::str_USE_IPV6);
#endif
   verify_mode = cfg.readLong(*UServer_Base::str_VERIFY_MODE);

   UString host = cfg[*UServer_Base::str_SERVER];

   if (host.empty()      == false ||
       name_sock.empty() == false)
      {
      (void) setHostPort(name_sock.empty() ? host : name_sock, cfg.readLong(*UServer_Base::str_PORT, 443));
      }

// cfg.clear();
// cfg.deallocate();
}

bool UClient_Base::connect()
{
   U_TRACE(0, "UClient_Base::connect()")

   U_INTERNAL_ASSERT_EQUALS(socket->isConnected(), false)  // Guard against multiple connections

   if (socket->connectServer(server, port))
      {
      (void) socket->setTimeoutRCV(timeoutMS);

      if (log) socket->getRemoteInfo(logbuf);

      U_RETURN(true);
      }

   response.snprintf("Sorry, couldn't connect to server '%.*s:%d'%R", U_STRING_TO_TRACE(server), port, NULL);

   if (log) ULog::log("%.*s\n", U_STRING_TO_TRACE(response));

   U_RETURN(false);
}

bool UClient_Base::setUrl(const UString& location)
{
   U_TRACE(0, "UClient_Base::setUrl(%.*S)", U_STRING_TO_TRACE(location))

   U_INTERNAL_DUMP("service = %.*S uri = %.*S", U_STRING_TO_TRACE(service), U_HTTP_URI_TO_TRACE)

   static char tmp[U_CAPACITY];

   // Check we've been passed a absolute URL

   if (location.find(*Url::str_http) == U_NOT_FOUND)
      {
      char* p;
      uint32_t len, size;
      const char* src  = UHTTP::http_info.uri;
      const char* end  = src + UHTTP::http_info.uri_len;

      char* dest = tmp;
      char* ptr  = dest;

      while (src < end)
         {
         p = (char*) memchr(src, '/', end - src);

         if (p == NULL) break;

         len = p - src + 1;

         U_INTERNAL_DUMP("segment = %.*S", len, src)

         (void) memcpy(dest, src, len);

         src   = p + 1;
         dest += len;
         }

      len = location.size();

      (void) memcpy(dest, location.data(), len);

      size = dest - ptr + len;

   // (void) memcpy(UHTTP::http_info.uri, tmp, size);

      UHTTP::http_info.uri     = tmp;
      UHTTP::http_info.uri_len = size;

      U_INTERNAL_DUMP("uri = %.*S", U_HTTP_URI_TO_TRACE)

      U_RETURN(false);
      }

   Url url(location);

   service = url.getService();

   if (service.empty() == false)
      {
      U_ASSERT(service.find(*Url::str_http) != U_NOT_FOUND) // Check we've been passed a valid URL...

      service.duplicate(); // services depend on url string...

#  ifdef HAVE_SSL
      if (socket->isSSL()) ((USSLSocket*)socket)->setActive(isHttps());
#  endif
      }

   UString value = url.getFile();

   if (value.empty() == false)
      {
      UHTTP::http_info.uri     = tmp;
      UHTTP::http_info.uri_len = value.copy(tmp, value.size());

      U_INTERNAL_DUMP("service = %.*S uri = %.*S", U_STRING_TO_TRACE(service), U_HTTP_URI_TO_TRACE)
      }

   bool bchange = setHostPort(url.getHost(), url.getPort());

   U_RETURN(bchange);
}

void UClient_Base::wrapRequestWithHTTP(const char* extension, const char* content_type)
{
   U_TRACE(0, "UClient_Base::wrapRequestWithHTTP(%S,%S)", extension, content_type)

   // Add the MIME-type headers to the request for HTTP server

   UString tmp(800U + UHTTP::http_info.uri_len + UHTTP::http_info.query_len + request.size());

   tmp.snprintf("%.*s %.*s%s%.*s HTTP/1.1\r\n"
                "Host: %.*s\r\n"
                "User-Agent: ULib/1.0\r\n"
                "%s",
                U_HTTP_METHOD_TO_TRACE,
                U_HTTP_URI_TO_TRACE,
                (UHTTP::http_info.query ? "?" : ""),
                U_HTTP_QUERY_TO_TRACE,
                U_STRING_TO_TRACE(host_port),
                extension);

   if (request.empty() == false)
      {
      U_INTERNAL_ASSERT_POINTER(content_type)

      tmp.snprintf_add("Content-Type: %s\r\n"
                       "Content-Length: %u\r\n"
                       "\r\n",
                       content_type,
                       request.size());

      tmp += request;
      }

   request = tmp;
}

bool UClient_Base::sendRequest()
{
   U_TRACE(0, "UClient_Base::sendRequest()")

   U_INTERNAL_ASSERT(socket->isOpen())

   bool result = USocketExt::write(socket, request);

   if (log) ULog::log("send request (%u bytes) %#.*S to %.*s\n", request.size(), U_STRING_TO_TRACE(request), U_STRING_TO_TRACE(logbuf));

   U_RETURN(result);
}

void UClient_Base::logResponse(const UString& data)
{
   U_TRACE(0, "UClient_Base::logResponse(%.*S)", U_STRING_TO_TRACE(data))

   U_INTERNAL_ASSERT_POINTER(log)

   ULog::log("received response (%u bytes) %#.*S from %.*s\n", data.size(), U_STRING_TO_TRACE(data), U_STRING_TO_TRACE(logbuf));
}

bool UClient_Base::readResponse()
{
   U_TRACE(0, "UClient_Base::readResponse()")

   clearData();

   if (UHTTP::readHTTP(socket, buffer, response)) // read HTTP message data
      {
      if (log) logResponse(buffer);

      U_RETURN(true);
      }

   U_RETURN(false);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UClient_Base::dump(bool reset) const
{
   *UObjectIO::os << "bIPv6                               " << bIPv6             << '\n'
                  << "port                                " << port              << '\n'
                  << "timeoutMS                           " << timeoutMS         << '\n'
                  << "verify_mode                         " << verify_mode       << '\n'
                  << "log_file_sz                         " << log_file_sz       << '\n'
                  << "log            (ULog                " << (void*)log        << ")\n"
                  << "server         (UString             " << (void*)&server    << ")\n"
                  << "ca_file        (UString             " << (void*)&ca_file   << ")\n"
                  << "ca_path        (UString             " << (void*)&ca_path   << ")\n"
                  << "logbuf         (UString             " << (void*)&logbuf    << ")\n"
                  << "key_file       (UString             " << (void*)&key_file  << ")\n"
                  << "password       (UString             " << (void*)&password  << ")\n"
                  << "log_file       (UString             " << (void*)&log_file  << ")\n"
                  << "cert_file      (UString             " << (void*)&cert_file << ")\n"
                  << "name_sock      (UString             " << (void*)&name_sock << ")\n"
                  << "buffer         (UString             " << (void*)&buffer    << ")\n"
                  << "request        (UString             " << (void*)&request   << ")\n"
                  << "service        (UString             " << (void*)&service   << ")\n"
                  << "response       (UString             " << (void*)&response  << ")\n"
                  << "host_port      (UString             " << (void*)&host_port << ")\n"
                  << "socket         (USocket             " << (void*)socket     << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
