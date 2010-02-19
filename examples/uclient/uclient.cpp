// uclient.cpp

#include <ulib/file_config.h>
#include <ulib/net/client/http.h>
#include <ulib/ssl/certificate.h>
#include <ulib/ssl/net/sslsocket.h>
#include <ulib/net/server/server.h>

#undef  PACKAGE
#define PACKAGE "uclient"

#define ARGS "[HTTP URL...]"

#define U_OPTIONS \
"purpose \"simple client http...\"\n" \
"option c config  1 \"path of configuration file\" \"\"\n" \
"option u upload  1 \"path of file to upload to url\" \"\"\n" \
"option i include 0 \"Include the HTTP-header in the output. The HTTP-header includes things like server-name, date of the document, HTTP-version\" \"\"\n"

#include <ulib/application.h>

class Application : public UApplication {
public:

   ~Application()
      {
      U_TRACE(5, "Application::~Application()")

      result.clear(); // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...

      delete client;
      }

   void run(int argc, char* argv[], char* env[])
      {
      U_TRACE(5, "Application::run(%d,%p,%p)", argc, argv, env)

      UApplication::run(argc, argv, env);

      UApplication::exit_value = 1;

      // manage options

      bool include = false;

      if (UApplication::isOptions())
         {
         cfg_str =  opt['c'];
         upload  =  opt['u'];
         include = (opt['i'] == U_STRING_FROM_CONSTANT("1"));
         }

      // manage arg operation

      const char* url = argv[optind++];

      arg = UString(url);

      Url srv(arg);

      // manage file configuration

      if (cfg_str.empty()) cfg_str = U_STRING_FROM_CONSTANT("uclient.cfg");

      // ----------------------------------------------------------------------------------------------------------------------------------
      // uclient - configuration parameters
      // ----------------------------------------------------------------------------------------------------------------------------------
      // USE_IPV6     flag to indicate use of ipv6
      // SERVER       host name or ip address for server
      // PORT         port number for the server
      // CERT_FILE    certificate of client
      // KEY_FILE     private key of client
      // PASSWORD     password for private key of client
      // CA_FILE      locations of trusted CA certificates used in the verification
      // CA_PATH      locations of trusted CA certificates used in the verification
      // VERIFY_MODE  mode of verification (SSL_VERIFY_NONE=0, SSL_VERIFY_PEER=1,
      //                                    SSL_VERIFY_FAIL_IF_NO_PEER_CERT=2,
      //                                    SSL_VERIFY_CLIENT_ONCE=4)
      //
      // FOLLOW_REDIRECTS     if yes manage to automatically follow redirects from server
      // USER                 if     manage to follow redirects, in response to a HTTP_UNAUTHORISED response from the HTTP server: user
      // PASSWORD_AUTH        if     manage to follow redirects, in response to a HTTP_UNAUTHORISED response from the HTTP server: password
      // ----------------------------------------------------------------------------------------------------------------------------------

      (void) cfg.open(cfg_str);

      client = new UHttpClient<USSLSocket>(&cfg);

      user             = cfg[*UServer_Base::str_USER];
      password         = cfg[U_STRING_FROM_CONSTANT("PASSWORD_AUTH")];
      follow_redirects = cfg.readBoolean(U_STRING_FROM_CONSTANT("FOLLOW_REDIRECTS"));

      client->setFollowRedirects(follow_redirects);
      client->setRequestPasswordAuthentication(user, password);

      if (upload.empty() == false)
         {
         UFile file(upload);

         if (client->upload(srv, file))
            {
            UApplication::exit_value = 0;
            }
         }
      else if (client->connectServer(srv) &&
               client->sendRequest(result))
         {
         UApplication::exit_value = 0;
         }

      result = (include ? client->getResponse()
                        : client->getContent());

      if (result.empty() == false)
         {
         cout.write(U_STRING_TO_PARAM(result));
         }
      }

private:
   UHttpClient<USSLSocket>* client;
   UFileConfig cfg;
   UString cfg_str, upload, user, password, result, arg;
   bool follow_redirects;
};

U_MAIN(Application)
