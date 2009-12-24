// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    flexer.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_FLEXER_H
#define U_FLEXER_H

/**
 * @file flexer.h
 */

#include <ulib/string.h>

#ifndef yyFlexLexerOnce
#  include <FlexLexer.h>
#endif

#ifndef min
#  define min(x,y) (x < y ? x : y)
#endif

#ifndef max
#  define max(x,y) (x > y ? x : y)
#endif

/**
 * @class UFlexer
 *
 * Implementazione di FlexLexer per ulib.
 */

struct U_NO_EXPORT UFlexerReference {
   const char* ptr;
   int offset, length;
};

#if !defined(YYSTYPE) && !defined(YYSTYPE_IS_DECLARED)
typedef union YYSTYPE {
   UFlexerReference ref;
   int number;
} YYSTYPE;
#  define YYSTYPE_IS_DECLARED
#endif

class U_EXPORT UFlexer : public yyFlexLexer {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UFlexer() : yyFlexLexer()
      {
      U_TRACE_REGISTER_OBJECT(0, UFlexer, "", 0)

      parsed_chars = write_position = 0;
      }

   UFlexer(const UString& data_) : yyFlexLexer(), data(data_)
      {
      U_TRACE_REGISTER_OBJECT(0, UFlexer, "%.*S", U_STRING_TO_TRACE(data_))

      parsed_chars = write_position = 0;
      }

   virtual ~UFlexer()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UFlexer)
      }

#ifndef YY_DECL
   /**
    * ordinary flex scanners: it scans the input stream, consuming tokens, until a rule's action returns a value.
    * NB: this function code is produced by flex program...
   */
   virtual int yylex(void* yyval);
#endif

   /**
    * reads up to `max_size' characters into buf and returns the number of characters read.
    * To indicate end-of-input, return 0 characters.
   */
   virtual int LexerInput(char* buf, int max_size)
      {
      U_TRACE(0, "UFlexer::LexerInput(%p,%d)", buf, max_size)

      int length = min(max((int)data.size() - write_position, 0), max_size);

      U_INTERNAL_DUMP("length = %d data.size() = %u", length, data.size())

      if (length)
         {
         length = data.copy(buf, length, write_position);

         write_position += length;

         U_RETURN(length);
         }

      U_RETURN(0);
      }

   void reset()
      {
      U_TRACE(0, "UFlexer::reset()")

   // yyFlexLexer::yy_flush_buffer(yyFlexLexer::yy_current_buffer);

      parsed_chars = write_position = 0;
      }

   // VARIE

   uint32_t size() const { return data.size(); }

   const char* getData() const { return data.data(); }

   void setData(const UString& data_)
      {
      U_TRACE(0, "UFlexer::setData(%.*S)", U_STRING_TO_TRACE(data_))

      data = data_;
      }

   UString substr(int offset, int length) { return data.substr(offset, length); }

   int getParsedChars()
      {
      U_TRACE(0, "UFlexer::getParsedChars()")

      U_RETURN(parsed_chars);
      }

   void setParsedChars(int i)
      {
      U_TRACE(0, "UFlexer::setParsedChars(%d)", i)

      parsed_chars = i;
      }

   // DEBUG

#ifdef DEBUG
   void        test();
   const char* dump(bool reset) const;
#endif

protected:
   UString data;
   int parsed_chars, write_position;

private:
   UFlexer(const UFlexer&) : yyFlexLexer() {}
   UFlexer& operator=(const UFlexer&)      { return *this; }
};

#endif
