// rdbdump.cpp

#include <ulib/db/rdb.h>

#ifdef HAVE_STRSTREAM_H
#  include <strstream.h>
#else
#  include <ulib/replace/strstream.h>
#endif

#undef  PACKAGE
#define PACKAGE "rdbdump"

#define ARGS "[path of db file]"

#define U_OPTIONS \
"purpose 'dump a reliable database (rdb)'\n"

#include <ulib/application.h>

class Application : public UApplication {
public:

   ~Application()
      {
      U_TRACE(5, "Application::~Application()")
      }

   void run(int argc, char* argv[], char* env[])
      {
      U_TRACE(5, "Application::run(%d,%p,%p)", argc, argv, env)

      UApplication::run(argc, argv, env);

      URDB x(UString(argv[optind]), false);

      if (x.open(0, false, true))
         {
         UFile::writeToTmpl("/tmp/rdbdump.txt", x.print());

         x.closeReorganize();
         }
      }

private:
};

U_MAIN(Application)
