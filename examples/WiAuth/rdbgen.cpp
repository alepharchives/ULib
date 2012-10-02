// rdbgen.cpp

#include <ulib/db/rdb.h>

#ifdef HAVE_STRSTREAM_H
#  include <strstream.h>
#else
#  include <ulib/replace/strstream.h>
#endif

#undef  PACKAGE
#define PACKAGE "rdbgen"

#undef  ARGS
#define ARGS "<path_of_db_file> <number_of_command> [ parameters ]"

#define PURPOSE \
"reliable database (rdb) managing\n" \
"------------------------------------------\n" \
"List of commands:\n" \
"\n" \
" 1 - get   -    parameter: <key>\n" \
" 2 - del   -    parameter: <key>\n" \
" 3 - store -    parameter: <key> <value>\n" \
" 4 - dump  - no parameter\n" \
"--------------------------------------------"

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

      UString path_of_db_file(argv[optind++]); 

      if (path_of_db_file.empty()) U_ERROR("missing <path_of_db_file> argument...");

      URDB x(path_of_db_file, false);

      if (x.open(0, false, true))
         {
         const char* method = argv[optind++];

         if (method == 0) U_ERROR("missing <number_of_command> argument...");

         int op = atoi(method);

         switch (op)
            {
            case 1: // get
               {
               UString key(argv[optind]), value = x[key]; 

               UFile::writeToTmpl("/tmp/rdbdump.txt", value);
               }
            break;

            case 2: // del
               {
               UString key(argv[optind]); 

               UApplication::exit_value = x.remove(key);
               }
            break;

            case 3: // store
               {
               UString key(argv[optind]), value(argv[++optind]);

               UApplication::exit_value = x.store(key, value);
               }
            break;

            case 4: // dump
               {
               UFile::writeToTmpl("/tmp/rdbdump.txt", x.print());
               }
            break;

            default:
               U_ERROR("num_method not valid...");
            break;
            }

         x.closeReorganize();
         }
      }

private:
};

U_MAIN(Application)
