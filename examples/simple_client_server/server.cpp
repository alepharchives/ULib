// server.cpp

#include <ulib/file_config.h>

#ifndef U_NO_SSL
#define U_NO_SSL
#endif

#include <ulib/net/tcpsocket.h>
#include <ulib/utility/services.h>
#include <ulib/container/vector.h>
#include <ulib/net/server/server.h>

#undef  PACKAGE
#define PACKAGE "simple server"
#undef  ARGS
#define ARGS "<file_config>"

#define U_OPTIONS \
"purpose \"simple server for testing, read config file specified with arg <file_config>...\"\n"

#include <ulib/application.h>

static UVector<UString>* request_response;

class UClientImageExample : public UClientImage<UTCPSocket> {
public:

   UClientImageExample() : UClientImage<UTCPSocket>()
      {
      U_TRACE_REGISTER_OBJECT(5, UClientImageExample, "")
      }

   ~UClientImageExample()
      {
      U_TRACE_UNREGISTER_OBJECT(5, UClientImageExample)
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool _reset) const { return UClientImage<UTCPSocket>::dump(_reset); }
#endif

protected:

   // define method VIRTUAL of class UEventFd

   virtual int handlerRead()
      {
      U_TRACE(5, "UClientImageExample::handlerRead()")

      reset(); // virtual method

      int result = (USocketExt::read(socket, *rbuffer, U_SINGLE_READ, 3 * 1000)
                           ? U_NOTIFIER_OK
                           : U_NOTIFIER_DELETE);

      if (result == U_NOTIFIER_OK)
         {
         if (UServer_Base::isLog()) logRequest();

         for (unsigned i = 0; i < request_response->size(); i += 2)
            {
            if (UServices::match(*rbuffer, (*request_response)[i]))
               {
               *wbuffer = (*request_response)[i+1];

               break;
               }
            }

         result = UClientImage<UTCPSocket>::handlerWrite();
         }

      U_RETURN(result);
      }
};

U_MACROSERVER(UServerExample, UClientImageExample, UTCPSocket);

class Application : public UApplication {
public:

   ~Application()
      {
      U_TRACE(5, "Application::~Application()")

      delete server;
      delete request_response;
      }

   void run(int argc, char* argv[], char* env[])
      {
      U_TRACE(5, "Application::run(%d,%p,%p)", argc, argv, env)

      UApplication::run(argc, argv, env);

      // manage config file

      if (argv[optind] == NULL) U_ERROR("argument 'file_config' not specified...");

      request_response  = new UVector<UString>();

      // load config file (section SERVER and section REQUEST_AND_RESPONSE)

      UString pathname(argv[optind]);

      if (config_file.open(pathname)                == false ||
          config_file.loadVector(*request_response) == false)
         {
         U_ERROR("config file '%s' not valid...", config_file.getPath().data());
         }

      // -----------------------------------------
      // server - configuration parameters
      // -----------------------------------------
      // USE_IPV6     flag to indicate use of ipv6
      // MSG_WELCOME  message of welcome
      // LOG_FILE     locations for file log
      // -----------------------------------------

      server = new UServerExample(&config_file);

      server->go();
      }

private:
   UServerExample* server;
   UFileConfig config_file;
};

U_MAIN(Application)
