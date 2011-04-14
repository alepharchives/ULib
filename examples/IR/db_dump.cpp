// db_dump.cpp

#include <ulib/string.h>

#undef  PACKAGE
#define PACKAGE "db_dump"
#undef  ARGS
#define ARGS ""

#define U_OPTIONS \
"purpose 'dump databases of index documents files...'\n" \
"option c config 1 'path of configuration file' ''\n"

#include "IR.h"

class Application : public IR {
public:

   void run(int argc, char* argv[], char* env[])
      {
      U_TRACE(5, "Application::run(%d,%p,%p)", argc, argv, env)

      IR::run(argc, argv, env);

      if (IR::openCDB(false))
         {
         UPosting::printDB(cout);

         UApplication::exit_value = 0;
         }
      }

private:
};

U_MAIN(Application)
