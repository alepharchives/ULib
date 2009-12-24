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
      end   = s + data.size();
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

   void setData(const UString& data)
      {
      U_TRACE(0, "UTokenizer::setData(%.*S)", U_STRING_TO_TRACE(data))

      str = data;
      s   = data.data();
      end = s + data.size();
      }

   void skipSpaces()
      {
      U_TRACE(0, "UTokenizer::skipSpaces()")

      while (s < end && u_isspace(*s)) ++s;
      }

   void setPointer(const char* ptr)
      {
      U_TRACE(0, "UTokenizer::setPointer(%S)", ptr)

      U_INTERNAL_ASSERT_MINOR(ptr,end)

      s = ptr;
      }

   const char* getPointer() const
      {
      U_TRACE(0, "UTokenizer::getPointer()")

      U_RETURN(s);
      }

   void setDistance(uint32_t pos)
      {
      U_TRACE(0, "UTokenizer::setDistance(%u)", pos)

      s = str.c_pointer(pos);
      }

   uint32_t getDistance() const
      {
      U_TRACE(0, "UTokenizer::getDistance()")

      uint32_t pos = str.distance(s);

      U_RETURN(pos);
      }

   // get next token

   bool   next(UString& tok, char c);
   bool extend(UString& tok, char c); // extend the actual token to the next char 'c'... (see PEC_report.cpp)
   bool   next(UString& tok, bool* bgroup);

   // EXT

   bool tokenSeen(UString* x);

   UString getTokenQueryParser();

   static bool group_skip;
   static const char* group;
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
