// update.cpp

#include <ulib/string.h>

#undef  PACKAGE
#define PACKAGE "update"
#undef  ARGS
#define ARGS ""

#define U_OPTIONS \
"purpose \"update index document files generated by index...\"\n" \
"option c config     1 \"path of configuration file\" \"\"\n" \
"option a add        1 \"list of files to add to index\" \"\"\n" \
"option s substitute 1 \"list of files to substitute in index\" \"\"\n" \
"option d delete     1 \"path of files to delete from index\" \"\"\n"

#define U_CDB_CLASS URDB
#define U_RDB_OPEN_WORDS 0
#define U_RDB_OPEN_NAMES 0

#include "IR.h"

class Application : public IR {
public:

   static void parse(void* name)
      {
      U_TRACE(5, "Application::parse(%p)", name)

      UPosting::filename->assign((UStringRep*)name);

      IR::parse();
      }

   void run(int argc, char* argv[], char* env[])
      {
      U_TRACE(5, "Application::run(%d,%p,%p)", argc, argv, env)

      IR::run(argc, argv, env);

      if (UPosting::dir_content_as_doc) U_ERROR("sorry, not implemented...", 0);

      // manage options

      if (UApplication::isOptions())
         {
         opt_file_to_add = opt['a'];
         opt_file_to_sub = opt['s'];
         opt_file_to_del = opt['d'];
         }

      if (IR::openCDB(true))
         {
         // load all filenames in argument

         if (opt_file_to_add.empty() == false) (void) UFile::buildFilenameListFrom(file_to_add, opt_file_to_add);
         if (opt_file_to_sub.empty() == false) (void) UFile::buildFilenameListFrom(file_to_sub, opt_file_to_sub);
         if (opt_file_to_del.empty() == false) (void) UFile::buildFilenameListFrom(file_to_del, opt_file_to_del);

         // process all filenames in argument

         IR::setBadWords();

         if (opt_file_to_add.empty() == false)
            {
            operation = 0; // add

            file_to_add.callForAllEntry(parse);
            }

         if (opt_file_to_sub.empty() == false)
            {
            operation = 1; // sub

            file_to_sub.callForAllEntry(parse);
            }

         if (opt_file_to_del.empty() == false)
            {
            operation = 2; // del

            file_to_del.callForAllEntry(parse);
            }

         // register to constant database (CDB)

         IR::closeCDB(true);

         UApplication::exit_value = 0;
         }
      }

private:
   UVector<UString> file_to_add, file_to_sub, file_to_del;
   UString opt_file_to_add, opt_file_to_sub, opt_file_to_del;
};

U_MAIN(Application)
