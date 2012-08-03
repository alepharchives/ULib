// cdbmake.cpp

#include <ulib/db/cdb.h>

#ifdef HAVE_STRSTREAM_H
#  include <strstream.h>
#else
#  include <ulib/replace/strstream.h>
#endif

#undef  PACKAGE
#define PACKAGE "cdbmake"

#define ARGS "[path of file that contains a series of encoded records]"

#define U_OPTIONS \
"purpose 'create a constant database (cdb)'\n"

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

      UString records = UFile::contentOf(argv[optind]);

      if (records.empty() == false)
         {
         UCDB x(false);

         if (x.UFile::creat(U_STRING_FROM_CONSTANT("/tmp/cdbmake.cdb")))
            {
            istrstream is(U_STRING_TO_PARAM(records));

            x.UFile::ftruncate(records.size() * 2);
            x.UFile::memmap(PROT_READ | PROT_WRITE);

            is >> x; // NB: this do ftruncate() e munmap()...

            x.UFile::close();
            x.UFile::reset();
            }
         }
      }

private:
};

U_MAIN(Application)
