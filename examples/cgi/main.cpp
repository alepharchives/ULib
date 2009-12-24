// main.cpp

#include <ulib/cgi/cgi.h>

#undef  PACKAGE
#define PACKAGE "cgi-dump"
#undef  ARGS
#define ARGS ""

#define U_OPTIONS \
"purpose \"program for dumping cgi data...\"\n" \
"option r response 1 \"response to http server\" \"\"\n"

#include <ulib/application.h>

#ifdef FAST_CGI
#  include <ulib/cgi/fcgi.h>
#endif

class Application : public UApplication {
public:

   void run(int argc, char* argv[], char* env[])
      {
      U_TRACE(5, "Application::run(%d,%p,%p)", argc, argv, env)

      UApplication::run(argc, argv, env);

#  ifdef FAST_CGI
      UFCgi fcgi;
      UCGI cgi(&fcgi);

      fcgi.init();

      while (fcgi.accept())
         {
         data = cgi.getPostData();
         }

      fcgi.finish();
#  else
      UCGI cgi;

      data = cgi.getPostData();
#  endif
      }

private:
   UString data;
};

U_MAIN(Application)
