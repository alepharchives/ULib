// cquery.cpp

#include <ulib/query/parser.h>
#include <ulib/utility/string_ext.h>

#include "cquery.h"

bool                   WeightWord::check_for_duplicate;
uint32_t               WeightWord::size_streambuf_vec;
UVector<WeightWord*>*  WeightWord::vec;
UHashMap<WeightWord*>* WeightWord::tbl;

void WeightWord::clear()
{
   U_TRACE(5, "WeightWord::clear()")

   if (tbl)
      {
      tbl->deallocate();

      delete tbl;
             tbl = 0;
      }

   if (vec)
      {
      delete vec;
             vec = 0;
      }

   size_streambuf_vec = 0;
}

void WeightWord::allocVector(uint32_t n)
{
   U_TRACE(5, "WeightWord::allocVector(%u)", n)

   U_INTERNAL_ASSERT_EQUALS(vec,0)

   vec = U_NEW(UVector<WeightWord*>(n));

   size_streambuf_vec = sizeof("(  )") - 1;
}

UVector<WeightWord*>* WeightWord::fromStream(istream& is)
{
   U_TRACE(5, "WeightWord::fromStream(%p)", &is)

   uint32_t vsize;

   is >> vsize;

   clear();
   allocVector(vsize);

   // load filenames

   UVector<UString> vtmp(vsize);

   is.get(); // skip ' '

   is >> vtmp;

   UPosting::word_freq = 0;

   UVector<WeightWord*>* result = vec;

   for (uint32_t i = 0; i < vsize; ++i)
      {
      *UPosting::filename = vtmp[i];

      push();
      }

   vec = 0;

   U_RETURN_POINTER(result,UVector<WeightWord*>);
}

UVector<WeightWord*>* WeightWord::duplicate(UVector<WeightWord*>* v)
{
   U_TRACE(5, "WeightWord::duplicate(%p)", v)

   U_INTERNAL_ASSERT_POINTER(v)

   WeightWord* item;
   uint32_t i, sz = v->size();

   UVector<WeightWord*>* result = U_NEW(UVector<WeightWord*>(sz));

   for (i = 0; i < sz; ++i)
      {
      item = (*v)[i];

      result->push_back(U_NEW(WeightWord(item->filename, 0)));
      }

   U_RETURN_POINTER(result,UVector<WeightWord*>);
}

void WeightWord::push()
{
   U_TRACE(5, "WeightWord::push()")

   U_INTERNAL_DUMP("UPosting::word_freq = %d", UPosting::word_freq)

   U_ASSERT_EQUALS(UPosting::filename->empty(),false)

   WeightWord* item = U_NEW(WeightWord(*UPosting::filename, UPosting::word_freq));

   if (check_for_duplicate)
      {
      if (tbl == 0)
         {
         tbl = U_NEW(UHashMap<WeightWord*>);

         tbl->allocate();
         }

      if (tbl->find(*UPosting::filename))
         {
         U_INTERNAL_DUMP("DUPLICATE")

         return;
         }

      tbl->insertAfterFind(*UPosting::filename, item);
      }

   if (vec == 0) allocVector();

   size_streambuf_vec += UPosting::filename->size() + 1;

   vec->push_back(item);
}

__pure int WeightWord::compareObj(const void* obj1, const void* obj2)
{
   U_TRACE(5, "WeightWord::compareObj(%p,%p)", obj1, obj2)

   int cmp = ((*(const WeightWord**)obj1)->word_freq < (*(const WeightWord**)obj2)->word_freq ?  1 :
              (*(const WeightWord**)obj1)->word_freq > (*(const WeightWord**)obj2)->word_freq ? -1 :
              (*(const WeightWord**)obj1)->filename.compare((*(const WeightWord**)obj2)->filename));

   return cmp;
}

void WeightWord::sortObjects()
{
   U_TRACE(5+256, "WeightWord::sortObjects()")

   if (size() > 1) vec->sort(compareObj); 
}

void WeightWord::dumpObjects()
{
   U_TRACE(5, "WeightWord::dumpObjects()")

   if (vec)
      {
      uint32_t sz = vec->size();

      if (sz > 1) vec->sort(compareObj);

      for (uint32_t i = 0; i < sz; ++i) cout << (*vec)[i]->filename << "\n";
      }
}

// STREAM

U_EXPORT istream& operator>>(istream& is, WeightWord& w)
{
   U_TRACE(5, "WeightWord::operator>>(%p,%p)", &is, &w)

   w.filename.get(is);

   return is;
}

U_EXPORT ostream& operator<<(ostream& os, const WeightWord& w)
{
   U_TRACE(5, "WeightWord::operator<<(%p,%p)", &os, &w)

   w.filename.write(os);

   return os;
}

UString*      Query::request;
UQueryParser* Query::parser;

Query::Query()
{
   U_TRACE(5, "Query::Query()")

   U_INTERNAL_ASSERT_EQUALS(parser,  0)
   U_INTERNAL_ASSERT_EQUALS(request, 0)

   parser  = U_NEW(UQueryParser);
   request = U_NEW(UString);
}

Query::~Query()
{
   U_TRACE(5, "Query::~Query()")

   clear();

   delete parser;
   delete request;
}

void Query::clear()
{
   U_TRACE(5, "Query::clear()")

     UPosting::reset();
   WeightWord::clear();

    parser->clear();
   request->clear();
}

void Query::query_meta(UStringRep* word_rep, UStringRep* value)
{
   U_TRACE(5, "Query::query_meta(%.*S,%p)", U_STRING_TO_TRACE(*word_rep), value)

   if (u_pfn_match(      word_rep->data(),       word_rep->size(),
               UPosting::word->data(), UPosting::word->size(), u_pfn_flags))
      {
      UPosting::posting->assign(value);

      UPosting::callForPostingAndSetFilename(WeightWord::push);
      }
}

void Query::push(UStringRep* str_inode, UStringRep* filename)
{
   U_TRACE(5, "Query::push(%#.*S,%.*S)", U_STRING_TO_TRACE(*str_inode), U_STRING_TO_TRACE(*filename))

   UPosting::filename->assign(filename);

   UPosting::word_freq = 0;

   WeightWord::push();
}

void Query::query_expr(UStringRep* str_inode, UStringRep* filename)
{
   U_TRACE(5, "Query::query_expr(%#.*S,%.*S)", U_STRING_TO_TRACE(*str_inode), U_STRING_TO_TRACE(*filename))

   UPosting::setDocID(str_inode);

   if (parser->evaluate()) push(str_inode, filename);
}

// NB: may be there are difficult with quoting (MINGW)...

const char* Query::checkQuoting(char* argv[])
{
   U_TRACE(5, "Query::checkQuoting(%p)", argv)

   U_INTERNAL_DUMP("optind = %d", optind)

   U_INTERNAL_ASSERT_RANGE(1,optind,3)

   U_DUMP_ATTRS(argv)

   // [0] -> path_prog
   // [1] -> "-c"
   // [2] -> "index.cfg"
   // [3] -> "query..."
   // [4] -> '\0'

   const char* ptr = argv[optind];

   if (argv[optind+1])
      {
      request->setBuffer(U_CAPACITY);

      bool bquote;

      do {
         U_INTERNAL_DUMP("ptr = %S", ptr)

         bquote = (*ptr != '"' && strchr(ptr, ' ') != 0);

                     request->push_back(' ');
         if (bquote) request->push_back('"');
              (void) request->append(ptr);
         if (bquote) request->push_back('"');
         }
      while ((ptr = argv[++optind]) && ptr[0]);

      ptr = request->c_str()+1;

      U_WARNING("quoting issue detected, actual query to be executed is <%s>", ptr);
      }

   U_RETURN_POINTER(ptr,const char);
}

void Query::run(const char* ptr)
{
   U_TRACE(5, "Query::run(%S)", ptr)

   *UPosting::word = UStringExt::removeEscape(UStringExt::trim(ptr, u_str_len(ptr)));

   U_INTERNAL_DUMP("UPosting::word = %.*S", U_STRING_TO_TRACE(*UPosting::word))

   ptr = UPosting::word->c_str();

   bool is_or  = (   strstr(ptr, " or ")  != 0),
        is_and = (   strstr(ptr, " and ") != 0),
        is_not = (   strstr(ptr, " not ") != 0),
        isnot  = (U_STRNEQ(ptr, "not "));

   if ((is_or || is_and || is_not || isnot) ||
       (   strstr(ptr, " OR ")  != 0) ||
       (   strstr(ptr, " AND ") != 0) ||
       (   strstr(ptr, " NOT ") != 0) ||
         U_STRNEQ(ptr,  "NOT "))
      {
      if (is_or)  *UPosting::word = UStringExt::substitute(*UPosting::word, U_CONSTANT_TO_PARAM(" or "),
                                                                            U_CONSTANT_TO_PARAM(" OR "));
      if (is_and) *UPosting::word = UStringExt::substitute(*UPosting::word, U_CONSTANT_TO_PARAM(" and "),
                                                                            U_CONSTANT_TO_PARAM(" AND "));
      if (is_not) *UPosting::word = UStringExt::substitute(*UPosting::word, U_CONSTANT_TO_PARAM(" not "),
                                                                            U_CONSTANT_TO_PARAM(" NOT "));

      if (isnot)
         {
         ((char*)ptr)[0] = 'N';
         ((char*)ptr)[1] = 'O';
         ((char*)ptr)[2] = 'T';
         }

      if (parser->parse(*UPosting::word))
         {
         parser->startEvaluate(UPosting::findDocID);

         cdb_names->callForAllEntry(query_expr);
         }

      return;
      }

   bool is_space = (strchr(ptr, ' ') != 0);

   if (strpbrk(ptr, "?*"))
      {
      if (is_space) U_ERROR("syntax error on query...");

      if (*UPosting::word == U_STRING_FROM_CONSTANT("*")) cdb_names->callForAllEntry(push);
      else
         {
         WeightWord::check_for_duplicate = true;

         if (UPosting::ignore_case) u_pfn_flags |= FNM_CASEFOLD;

         cdb_words->callForAllEntry(query_meta);

         WeightWord::check_for_duplicate = false;
         }
      }
   else
      {
      UPosting::callForPosting(WeightWord::push, is_space);
      }
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* WeightWord::dump(bool reset) const
{
   *UObjectIO::os << "word_freq          " << word_freq        << '\n'
                  << "filename  (UString " << (void*)&filename << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
