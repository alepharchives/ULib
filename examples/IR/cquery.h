// cquery.h

#ifndef IR_CQUERY_H
#define IR_CQUERY_H 1

#include "IR.h"

class WeightWord {
public:

   // Check Memory
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   UString filename;
   uint32_t word_freq;

   static bool check_for_duplicate;
   static  UVector<WeightWord*>* vec;
   static UHashMap<WeightWord*>* tbl;
   static uint32_t size_streambuf_vec;

   // COSTRUTTORE

   WeightWord() : word_freq(0)
      {
      U_TRACE_REGISTER_OBJECT(5, WeightWord, "")
      }

   WeightWord(const UString& x, uint32_t f) : filename(x), word_freq(f)
      {
      U_TRACE_REGISTER_OBJECT(5, WeightWord, "%.*S,%u", U_STRING_TO_TRACE(filename), word_freq)
      }

   ~WeightWord()
      {
      U_TRACE_UNREGISTER_OBJECT(5, WeightWord)
      }

   // SERVICES

   static void push();
   static void clear();
   static void sortObjects();
   static void dumpObjects();
   static void allocVector(uint32_t n = 64);
   static int  compareObj(const void* obj1, const void* obj2) __pure;

   static uint32_t size() { return (vec ? vec->size() : 0); }

   static UVector<WeightWord*>* fromStream(istream& is);
   static UVector<WeightWord*>* duplicate(UVector<WeightWord*>* v);

   // STREAM

   friend U_EXPORT istream& operator>>(istream& is,       WeightWord& w);
   friend U_EXPORT ostream& operator<<(ostream& os, const WeightWord& w);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif
};

class UQueryParser;

class Query {
public:

   // Check Memory
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   static UString* request;
   static UQueryParser* parser;

   // COSTRUTTORE

    Query();
   ~Query();

   // SERVICES

   void run(const char* ptr);

   // NB: may be there are difficult with quoting (MINGW)...
   static const char* checkQuoting(char* argv[]); 

   static void clear();
   static void push(      UStringRep* str_inode, UStringRep* filename);
   static void query_expr(UStringRep* str_inode, UStringRep* filename);
   static void query_meta(UStringRep*  word_rep, UStringRep* value);
};

#endif
