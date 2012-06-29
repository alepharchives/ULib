// client.cpp

#include <ulib/base/utility.h>

#include <ulib/log.h>
#include <ulib/file_config.h>
#include <ulib/net/tcpsocket.h>
#include <ulib/container/vector.h>
#include <ulib/net/server/client_image.h>

#undef  PACKAGE
#define PACKAGE "simple client"
#undef  ARGS
#define ARGS "<file_config>"

#define U_OPTIONS \
"purpose \"simple client for testing, read config file specified with arg <file_config>...\"\n" \
"option p port       1 \"port number for connecting the server\" \"10001\"\n" \
"option l log        1 \"log file for data processing\" \"\"\n" \
"option m max_length 1 \"max length log string for request and response data\" \"256\"\n"

#include <ulib/application.h>

static ULog* ulog;

class Application : public UApplication {
public:

#define LOG_MSG0(msg)            if (ulog) ulog->log("%.*s\n",U_CONSTANT_TO_TRACE(msg))
#define LOG_MSGb(msg)            if (ulog) ulog->log(msg" %.*s\n",U_STRING_TO_TRACE(buffer))
#define LOG_MSGB(format,args...) if (ulog) ulog->log(format" %.*s\n",args,U_STRING_TO_TRACE(buffer))

   void get_welcome()
      {
      U_TRACE(5, "Application::get_welcome()")

      if (msg_welcome.empty() == false)
         {
         iBytesTransferred = pcClientSocket->recv(pcBuffer, sizeof(pcBuffer));

         if (pcClientSocket->checkIO(iBytesTransferred))
            {
            LOG_MSGB("received welcome message %#.*S from", iBytesTransferred, pcBuffer);

            if (u_pfn_match(pcBuffer, iBytesTransferred, U_STRING_TO_PARAM(msg_welcome), u_pfn_flags) == false)
               {
               LOG_MSG0("welcome message mismatched...");
               }
            }
         }
      }

   bool send_request()
      {
      U_TRACE(5, "Application::send_request()")

      UString req = request_response[index];

      LOG_MSGB("send request %#.*S to ", U_STRING_TO_TRACE(req));

      iBytesTransferred = pcClientSocket->send(U_STRING_TO_PARAM(req));

      bool result = pcClientSocket->checkIO(iBytesTransferred);

      U_RETURN(result);
      }

   bool get_response()
      {
      U_TRACE(5, "Application::get_response()")

      iBytesTransferred = pcClientSocket->recv(pcBuffer, sizeof(pcBuffer));

      if (pcClientSocket->checkIO(iBytesTransferred))
         {
         LOG_MSGB("received response %#.*S from", iBytesTransferred, pcBuffer);

         UString resp = request_response[index+1];

         if (u_pfn_match(pcBuffer, iBytesTransferred, U_STRING_TO_PARAM(resp), u_pfn_flags) == false)
            {
            LOG_MSG0("response message mismatched...");
            }

         U_RETURN(true);
         }

      U_RETURN(false);
      }

   void run(int argc, char* argv[], char* env[])
      {
      U_TRACE(5, "Application::run(%d,%p,%p)", argc, argv, env)

      UApplication::run(argc, argv, env);

      // manage config file

      if (argv[optind] == NULL) U_ERROR("arg <file_config> not specified...");

      // load config file (section SERVER and section REQUEST_AND_RESPONSE)

      UString pathname(argv[optind]);

      if (config_file.open(pathname)               == false ||
          config_file.loadVector(request_response) == false)
         {
         U_ERROR("config file <%s> not valid...", config_file.getPath().data());
         }

      // -------------------------------------------
      // client - configuration parameters
      // -------------------------------------------
      // ENABLE_IPV6    flag to indicate use of ipv6
      // MSG_WELCOME    message of welcome
      // SERVER_ADDRESS TCP/IP address of server
      // -------------------------------------------

      msg_welcome = config_file[U_STRING_FROM_CONSTANT("MSG_WELCOME")];
      bool bIPv6  = config_file.readBoolean(U_STRING_FROM_CONSTANT("ENABLE_IPV6"));

      pcClientSocket = new UTCPSocket(bIPv6);

      // manage server address

      UString address = config_file[U_STRING_FROM_CONSTANT("SERVER_ADDRESS")];

      if (address.empty() == true) address = U_STRING_FROM_CONSTANT("localhost");

      // manage port number

      UString port = opt['p'];

      if (pcClientSocket->connectServer(address, port.empty() ? 10001 : port.strtol()) == false)
         {
         U_ERROR("connect to server address <%.*s:%.*s> failed...", address.size(), address.data(), port.size(), port.data());
         }

      // maybe manage logging...

      UString log_file = opt['l'];

      if (log_file.empty() == false)
         {
         u_init_ulib_hostname();

         U_INTERNAL_DUMP("u_hostname(%u) = %.*S", u_hostname_len, u_hostname_len, u_hostname)

         u_init_ulib_username();

         ulog = U_NEW(ULog(log_file, 1024 * 1024, "%10D> "));

         // manage max string length for log request and response

         UString maxlength = opt['m'];

         if (maxlength.empty() == false) u_printf_string_max_length = maxlength.strtol();
         }

      // manage connection

      buffer.reserve(100);
      pcClientSocket->setTimeoutRCV();
      pcClientSocket->getRemoteInfo(buffer);

      LOG_MSGb("client connected to");

      get_welcome();

      for (index = 0; index < request_response.size(); index += 2)
         {
         if (send_request() == false ||
             get_response() == false) break;
         }

      if (pcClientSocket->isBroken() == false)
         {
         LOG_MSGb("closing client connection to");
         }

      if (ulog) ulog->close();

#ifdef DEBUG
      delete ulog;
      delete pcClientSocket;
#endif
      }

private:
   UTCPSocket* pcClientSocket;
   unsigned index;
   int iBytesTransferred;
   UString buffer, msg_welcome;
   UFileConfig config_file;
   UVector<UString> request_response;

private:
   char pcBuffer[65535];
};

U_MAIN(Application)
