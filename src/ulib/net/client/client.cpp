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
   U_TRACE(0, "UClient_Base::str_allocate()")

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
   timeoutMS = U_TIMEOUT;
}

UClient_Base::UClient_Base(UFileConfig* cfg) : response(U_CAPACITY), buffer(U_CAPACITY), host_port(100U), logbuf(100U)
{
   U_TRACE_REGISTER_OBJECT(0, UClient_Base, "%p", cfg)

   if (cfg) loadConfigParam(*cfg);
   else
      {
      bIPv6     = false;
      port      = verify_mode = 0;
      timeoutMS = U_TIMEOUT;
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

   U_ASSERT(host.empty() == false)

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
         int size = snprintf(tmp, sizeof(tmp), "%d", _port);

         host_port.push_back(':');
         host_port.append(tmp, size);
         }

      U_INTERNAL_DUMP("host_port = %.*S", U_STRING_TO_TRACE(host_port))

      U_RETURN(true);
      }

   U_RETURN(false);
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
   // USE_IPV6      flag to indicate use of ipv6
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
      if (UServer_Base::isLog())
         {
         U_ASSERT_EQUALS(log_file, UServer_Base::pthis->log_file)

         log                    = UServer_Base::log;
         log_shared_with_server = true;
         }
      else
         {
         log = U_NEW(ULog(log_file, cfg.readLong(*UServer_Base::str_LOG_FILE_SZ)));

         ULog::setServer(false);
         }
      }

#ifdef HAVE_IPV6
   bIPv6       = cfg.readBoolean(*UServer_Base::str_USE_IPV6);
#endif
   timeoutMS   = cfg.readLong(*str_RES_TIMEOUT) * 1000;
   verify_mode = cfg.readLong(*UServer_Base::str_VERIFY_MODE);

   UString host      = cfg[*UServer_Base::str_SERVER],
           name_sock = cfg[*UServer_Base::str_SOCKET_NAME];

   if (     host.empty() == false ||
       name_sock.empty() == false)
      {
      (void) setHostPort(name_sock.empty() ? host : name_sock, cfg.readLong(*UServer_Base::str_PORT));
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
      if (log)
         {
              if (port)           socket->getRemoteInfo(logbuf);
         else if (logbuf.empty()) logbuf = '"' + host_port + '"';
         }

      U_RETURN(true);
      }

   response.snprintf("Sorry, couldn't connect to server %.*S%R", U_STRING_TO_TRACE(host_port), 0); // NB: the last argument (0) is necessary...

   if (log)
      {
      ULog::log("%s%.*s\n",
                  log_shared_with_server ? UServer_Base::mod_name : "",
                  U_STRING_TO_TRACE(response));
      }

   U_RETURN(false);
}

bool UClient_Base::setUrl(const UString& location)
{
   U_TRACE(0, "UClient_Base::setUrl(%.*S)", U_STRING_TO_TRACE(location))

   // Check we've been passed a absolute URL

   if (u_isURL(U_STRING_TO_PARAM(location)) == false)
      {
      U_INTERNAL_DUMP("uri = %.*S", U_HTTP_URI_TO_TRACE)

      char* p;
      uint32_t len;
      const char* src  =       UHTTP::http_info.uri;
      const char* _end = src + UHTTP::http_info.uri_len;

      static char _buffer[U_PATH_MAX];

      char* dest = _buffer;
      char* ptr  = dest;

      while (src < _end)
         {
         p = (char*) memchr(src, '/', _end - src);

         if (p == NULL) break;

         len = p - src + 1;

         U_INTERNAL_DUMP("segment = %.*S", len, src)

         (void) u_memcpy(dest, src, len);

         src   = p + 1;
         dest += len;
         }

      len = location.size();

      (void) u_memcpy(dest, location.data(), len);

      UHTTP::http_info.uri     = _buffer;
      UHTTP::http_info.uri_len = dest - ptr + len;

      U_INTERNAL_DUMP("uri = %.*S", U_HTTP_URI_TO_TRACE)

      U_RETURN(false);
      }

#ifdef DEBUG
   uri.clear(); // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE... (uri is substr of url)
#endif

   url.set(location);

#ifdef HAVE_SSL
   if (socket->isSSL()) ((USSLSocket*)socket)->setActive(url.isHTTPS());
#endif

   uri = url.getFile();

   UHTTP::http_info.uri     = uri.data();
   UHTTP::http_info.uri_len = uri.size();

   U_INTERNAL_DUMP("uri = %.*S", U_HTTP_URI_TO_TRACE)

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
                "User-Agent: ULib/" VERSION "\r\n"
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

bool UClient_Base::sendRequest(bool bread_response)
{
   U_TRACE(0, "UClient_Base::sendRequest(%b)", bread_response)

   bool result;
   int counter = 0;

send:
   result = false;

   for (; counter < 2; ++counter)
      {
      if (isConnected() ||
          UClient_Base::connect())
         {
         result = USocketExt::write(socket, request);

         if (isConnected()) break;

         if (log)
            {
            ULog::log("%sConnection to %.*s reset by peer%R\n",
                     log_shared_with_server ? UServer_Base::mod_name : "",
                     U_STRING_TO_TRACE(logbuf), 0); // NB: the last argument (0) is necessary...
            }
         }
      }

   if (result)
      {
      if (bread_response                                                        &&
          USocketExt::read(socket, response, U_SINGLE_READ, timeoutMS) == false &&
          isConnected()                                                == false)
         {
         if (log)
            {
            ULog::log("%sConnection to %.*s reset by peer%R\n",
                     log_shared_with_server ? UServer_Base::mod_name : "",
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
                     log_shared_with_server ? UServer_Base::mod_name : "",
                     request.size(), U_STRING_TO_TRACE(request), U_STRING_TO_TRACE(logbuf));

         if (bread_response) logResponse(response);
         }

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
                  log_shared_with_server ? UServer_Base::mod_name : "",
                  data.size(), U_STRING_TO_TRACE(data), U_STRING_TO_TRACE(logbuf));
}

bool UClient_Base::readHTTPResponse()
{
   U_TRACE(0, "UClient_Base::readHTTPResponse()")

   // read HTTP message data

   clearData();

   if (UHTTP::readHTTPHeader(socket, buffer))
      {
      uint32_t pos = buffer.find(*USocket::str_content_length, UHTTP::http_info.startHeader, UHTTP::http_info.szHeader);

      if (pos != U_NOT_FOUND)
         {
         UHTTP::http_info.clength = (uint32_t) strtoul(buffer.c_pointer(pos + USocket::str_content_length->size() + 2), 0, 0);

         if (UHTTP::readHTTPBody(socket, buffer, response))
            {
            if (log) logResponse(buffer);

            U_RETURN(true);
            }
         }
      }

   U_RETURN(false);
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
                  << "request        (UString             " << (void*)&request         << ")\n"
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
