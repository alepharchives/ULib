// IR.h

#ifndef IR_H
#define IR_H 1

#include "posting.h"

#include <ulib/db/rdb.h>
#include <ulib/command.h>
#include <ulib/tokenizer.h>
#include <ulib/file_config.h>
#include <ulib/utility/services.h>
#include <ulib/container/vector.h>

#include <ulib/application.h>

/*
inverted index: (data structure)

Definition: An index into a set of texts of the words in the texts. The index is accessed by some search method.
Each index entry gives the word and a list of texts, possibly with locations within the text, where the word occurs.
See also full inverted index, inverted file index, block addressing index, index file, external index, forward index.

Note: Suppose we want to search the texts "i love you," "god is love," "love is blind," and "blind justice."
(The words of the text are all lower case for simplicity) If we index by (text, character within the text),
the index with location in text is:

 blind   (3,8);(4,0)
 god     (2,0)
 i       (1,0)
 is      (2,4);(3,5)
 justice (4,6)
 love    (1,2);(2,7);(3,0)
 you     (1,7)

The word "blind" is in document 3 ("love is blind") starting at character 8, so has an entry (3,8).
To find, for instance, documents with both "is" and "love," first look up the words in the index,
then find the intersection of the texts in each list. In this case, documents 2 and 3 have both words.
We can quickly find documents where the words appear close to each other by comparing the character within the text
*/

#ifndef U_CDB_CLASS
#define U_CDB_CLASS UCDB
#define U_RDB_OPEN_WORDS
#define U_RDB_OPEN_NAMES
#endif

extern UCDB* cdb_names;
extern UCDB* cdb_words;

class IR : public UApplication {
public:

   static UTokenizer* t;
   static UPosting* posting;
   static UString* bad_words;
   static UVector<UString>* filter_ext;
   static UVector<UCommand*>* filter_cmd;
   static UVector<UString>* suffix_bad_words;
   static UVector<UString>* suffix_skip_tag_xml;
   static int32_t operation; // 0 -> add, 1 -> sub, 2 -> del, 3 -> check

    IR()
      {
      U_TRACE(5, "IR::IR()")
      }

   ~IR();

   bool openCDB(bool parsing, bool index = false) // NB: must be inline...(see #define)
      {
      U_TRACE(5, "IR::openCDB(%b,%b)", parsing, index)

      UPosting::ignore_case  = cfg.readBoolean(U_STRING_FROM_CONSTANT("IGNORE_CASE"));
      uint32_t cfg_dimension = cfg.readLong(U_STRING_FROM_CONSTANT("DIMENSION"), 1000);

      cdb_names = new U_CDB_CLASS(cfg_db + U_STRING_FROM_CONSTANT("tbl_names.cdb"), false);
      cdb_words = new U_CDB_CLASS(cfg_db + U_STRING_FROM_CONSTANT("tbl_words.cdb"), UPosting::ignore_case);

      if (index ||
         (((U_CDB_CLASS*)cdb_names)->open( U_RDB_OPEN_NAMES ) &&
          ((U_CDB_CLASS*)cdb_words)->open( U_RDB_OPEN_WORDS )))
         {
         posting = new UPosting(cfg_dimension, parsing, index);

         if (parsing)
            {
            const char* cfg_directory = cfg[U_STRING_FROM_CONSTANT("DIRECTORY")].c_str();

            (void) UFile::chdir(cfg_directory, true);
            }

         U_RETURN(true);
         }

      U_RETURN(false);
      }

   void closeCDB(bool reset)
      {
      U_TRACE(5, "IR::closeCDB(%b)", reset)

      // register changes to constant database (CDB)

      if (reset) (void) UFile::chdir(0, true);

      ((URDB*)cdb_names)->closeReorganize();
      ((URDB*)cdb_words)->closeReorganize();
      }

   void deleteDB()
      {
      U_TRACE(5, "IR::deleteDB()")

      delete (U_CDB_CLASS*)cdb_names;
      delete (U_CDB_CLASS*)cdb_words;
      }

   void setBadWords();
   void loadFilters();
   bool loadFileConfig(UString& cfg_index);

   // SERVICES

   static void parse();
   static void loadFiles();
   static void processFile();
   static void processDirectory();

   void run(int argc, char* argv[], char* env[]) // MUST BE INLINE...
      {
      U_TRACE(5, "IR::run(%d,%p,%p)", argc, argv, env)

      UApplication::run(argc, argv, env);

      // manage options

      UString cfg_index;

      if (UApplication::isOptions()) cfg_index = opt['c'];

      // manage arg operation

      // manage file configuration

      (void) loadFileConfig(cfg_index);

      UApplication::exit_value = 1;
      }

protected:
   UFileConfig cfg;
   UString cfg_db, cfg_bad_words, cfg_bad_words_ext, cfg_skip_tag_xml, cfg_filter_ext, cfg_filter_cmd;
};

#endif
