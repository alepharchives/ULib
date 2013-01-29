// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    tokenizer.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_TOKENIZER_H
#define ULIB_TOKENIZER_H 1

#include <ulib/string.h>

/**
 * TOKEN ID
 **/

#define U_TK_ERROR       -1
#define U_TK_AND          1
#define U_TK_OR           2
#define U_TK_EQ           3
#define U_TK_NE           4
#define U_TK_GT           5
#define U_TK_GE           6
#define U_TK_LT           7
#define U_TK_LE           8
#define U_TK_STARTS_WITH  9
#define U_TK_ENDS_WITH   10
#define U_TK_IS_PRESENT  11
#define U_TK_CONTAINS    12
#define U_TK_PLUS        13
#define U_TK_MINUS       14
#define U_TK_MULT        15
#define U_TK_DIV         16
#define U_TK_MOD         17
#define U_TK_NOT         18
#define U_TK_FN_CALL     19
#define U_TK_LPAREN      20
#define U_TK_RPAREN      21
#define U_TK_VALUE       22
#define U_TK_COMMA       23
#define U_TK_NAME        24
#define U_TK_PID         25

template <class T> class UVector;

class U_EXPORT UTokenizer {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UTokenizer(const char* d = 0)
      {
      U_TRACE_REGISTER_OBJECT(0, UTokenizer, "%S", d)

      delim = d;
      }

   UTokenizer(const UString& data, const char* d = 0) : str(data)
      {
      U_TRACE_REGISTER_OBJECT(0, UTokenizer, "%.*S,%S", U_STRING_TO_TRACE(data), d)

      s     = data.data();
      end   = data.end();
      delim = d;
      }

   ~UTokenizer()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UTokenizer)
      }

   // VARIE

   bool atEnd()
      {
      U_TRACE(0, "UTokenizer::atEnd()")

      bool result = (s >= end);

      U_RETURN(result);
      }

   void setDelimiter(const char* sep)
      {
      U_TRACE(0, "UTokenizer::setDelimiter(%S)", sep)

      delim = sep;
      }

   void setData(const UString& data);

   void skipSpaces()
      {
      U_TRACE(0, "UTokenizer::skipSpaces()")

      while (s < end && u__isspace(*s)) ++s;
      }

   void setPointer(const char* ptr)
      {
      U_TRACE(0, "UTokenizer::setPointer(%S)", ptr)

      U_INTERNAL_ASSERT(ptr <= end)

      s = ptr;
      }

   const char* getPointer() const
      {
      U_TRACE(0, "UTokenizer::getPointer()")

      U_RETURN(s);
      }

   const char* getEnd() const
      {
      U_TRACE(0, "UTokenizer::getEnd()")

      U_RETURN(end);
      }

   void setDistance(uint32_t pos)
      {
      U_TRACE(0, "UTokenizer::setDistance(%u)", pos)

      s = str.c_pointer(pos);
      }

   uint32_t getDistance() const __pure
      {
      U_TRACE(0, "UTokenizer::getDistance()")

      uint32_t pos = (s < end ? str.distance(s) : str.size());

      U_RETURN(pos);
      }

   // get current char

   char current()
      {
      U_TRACE(0, "UTokenizer::current()")

      if (s >= end) U_RETURN('\0');

      U_RETURN(*s);
      }

   // go prev char

   void back()
      {
      U_TRACE(0, "UTokenizer::back()")

      U_INTERNAL_ASSERT_MAJOR(s,str.data())

      --s;
      }

   // get next char

   char next()
      {
      U_TRACE(0, "UTokenizer::next()")

      if (s >= end) U_RETURN('\0');

      U_RETURN(*s++);
      }

   // get next token

   bool   next(UString& tok, char c);
   bool extend(UString& tok, char c); // extend the actual token to the next char 'c'... (see PEC_report.cpp)

   bool   next(UString& tok, bPFi func);
   bool   next(UString& tok, bool* bgroup);

   // EXT

   UString getTokenQueryParser();
   int     getTokenId(UString& token);
   bool    tokenSeen(const UString* x);

   bool skipNumber(bool& isReal);
   bool skipToken(const char* token, uint32_t sz);

   static const char* group;
   static bool group_skip, avoid_punctuation;
   static uint32_t group_len, group_len_div_2;

   static void setGroup(const char* grp, uint32_t grp_len = 0, bool bskip = false)
      {
      U_TRACE(0, "UTokenizer::setGroup(%S,%u,%b)", grp, grp_len, bskip)

      group           = grp;
      group_skip      = bskip;
      group_len       = grp_len;
      group_len_div_2 = grp_len / 2;
      }

   static void setSkipTagXML(bool flag)
      {
      U_TRACE(0, "UTokenizer::setSkipTagXML(%b)", flag)

      if (flag) setGroup(U_CONSTANT_TO_PARAM("<>"), true);
      else      setGroup(0,                      0, false);
      }

   static void setAvoidPunctuation(bool flag)
      {
      U_TRACE(0, "UTokenizer::setAvoidPunctuation(%b)", flag)

      avoid_punctuation = flag;
      }

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   const char* s;
   const char* end;
   const char* delim;
   UString str;

private:
   UTokenizer(const UTokenizer&)            {}
   UTokenizer& operator=(const UTokenizer&) { return *this; }
};

#endif
