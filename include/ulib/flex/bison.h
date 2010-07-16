// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    bison.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_BISON_H
#define U_BISON_H

/**
 * @file bison.h
 */

#include <ulib/flex/flexer.h>

/**
 * @class UBison
 *
 * Implementazione di Bison per ulib.
 */

class U_EXPORT UBison : public UFlexer {
public:

   // COSTRUTTORI

   UBison()
      {
      U_TRACE_REGISTER_OBJECT(0, UBison, "", 0)
      }

   UBison(const UString& data_) : UFlexer(data_)
      {
      U_TRACE_REGISTER_OBJECT(0, UBison, "%.*S", U_STRING_TO_TRACE(data_))
      }

   ~UBison()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UBison)
      }

   // VARIE

   bool parse(void* obj = 0)
      {
      U_TRACE(0, "UBison::parse(%p)", obj)

      U_ASSERT(data.empty() == false)

      /*
      extern int yydebug;
      yydebug = 1;
      */

      if (obj == 0) obj = this;

      extern int yyparse(void*);
      bool ok = (yyparse(obj) == 0);

      U_INTERNAL_DUMP("yyparse() = %b, parsed_chars = %d, size() = %u", ok, parsed_chars, data.size())

      U_RETURN(ok);
      }

   bool parse(const UString& data, void* obj = 0)
      {
      U_TRACE(0, "UBison::parse(%.*S,%p)", U_STRING_TO_TRACE(data), obj)

      setData(data);

      bool ok = parse(obj);

      U_RETURN(ok);
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

private:
   UBison(const UBison&) : UFlexer() {}
   UBison& operator=(const UBison&)  { return *this; }
};

#endif
