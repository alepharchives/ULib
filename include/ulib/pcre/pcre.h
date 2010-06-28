// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    pcre.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_PCRE_H
#define ULIB_PCRE_H 1

#include <ulib/container/vector.h>

extern "C" {
#include <pcre.h>
}

/**
   @class UPCRE

   @brief This class is a wrapper around the PCRE library.

   You can use this class to search in strings using reular expressions as well as getting matched sub strings.
   It does currently not support all features, which the underlying PCRE library provides, but the most important
   stuff is implemented. If you want to learn more about regular expressions which can be used with PCRE, then
   please read the following documentation:
   <a href="http://www.perldoc.com/perl5.8.0/pod/perlre.html">perlre - Perl regular expressions</a>
 */

/**
   additional binary flag in the hope Philip never ever uses the same in the future
   @see UPCRE(const UString& expression, uint32_t int flags)
*/

#define PCRE_GLOBAL 0x10000

class U_EXPORT UPCRE {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator.
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   /* init pointers with NULL */

   void zero()
      {
      U_TRACE(0, "UPCRE::zero()")

      tables       = 0;
      p_pcre       = 0;
      sub_vec      = 0;
      err_str      = 0;
      resultset    = 0;
      stringlist   = 0;
      num_matches  = -1;
      p_pcre_extra = 0;
      }

   /* reset all counters and free objects, prepare for another search */

   void reset()
      {
      U_TRACE(0, "UPCRE::reset()")

      did_match   = false;
      num_matches = -1;
      }

   /**
   Empty Constructor. Create a new empty UPCRE object. This is the simplest constructor available,
   you might consider one of the other constructors as a better solution. You need to initialize
   this UPCRE object, if you use the empty constructor. You can use the available method set() to
   assign it an expression

   @return A new empty UPCRE object
   */

   UPCRE()
      {
      U_TRACE_REGISTER_OBJECT(0, UPCRE, "", 0)

      zero();
      }

   /**
   Compile the given pattern. An UPCRE object created this way can be used multiple times to do searches

   @param expression a string, which must be a valid perl regular expression.
   @param flags option bits can be one or more of the following bits:
   - PCRE_ANCHORED        anchored pattern.
   - PCRE_CASELESS        case insensitive search.
   - PCRE_DOLLAR_ENDONLY  dollar sign matches only at end.
   - PCRE_DOTALL          newline is contained in .
                          (A dot in an expression matches newlines too, which is normally not the case)
   - PCRE_EXTENDED        whitespace characters will be ignored (except within character classes or if escaped)
   - PCRE_EXTRA           use perl incompatible pcre extensions
   - PCRE_MULTILINE       match on multiple lines, thus ^ and $ are interpreted as the start
                          and end of the entire string, not of a single line
   - PCRE_NO_AUTO_CAPTURE disable the use of numbered capturing parentheses in the pattern
   - PCRE_UNGREEDY        quantifiers behave not greedy by default
   - PCRE_UTF8            use utf8 support.
   - PCRE_GLOBAL          (internal flag) match multiple times - used only in the replace() method
   @see pcreapi(3) manpage

   @return A new UPCRE object, which holds the compiled pattern.
   */

   UPCRE(const UString& expression, int flags);

   /**
   Compile the given pattern. An UPCRE object created this way can be used multiple times to do searches

   @param expression a string, which must be a valid perl regular expression
   @param flags can be one or more of the following letters:
   - <b>i</b> Search case insensitive
   - <b>m</b> Match on multiple lines, thus ^ and $ are interpreted as the start and end of the entire string,
              not of a single line
   - <b>s</b> A dot in an expression matches newlines too(which is normally not the case)
   - <b>x</b> Whitespace characters will be ignored (except within character classes or if escaped)
   - <b>g</b> Match multiple times. This flags affects only the behavior of the replace() method

   @return A new UPCRE object, which holds the compiled pattern.
   */

   UPCRE(const UString& expression, const char* flags = "") : _expression(expression)
      {
      U_TRACE_REGISTER_OBJECT(0, UPCRE, "%.*S,%S", U_STRING_TO_TRACE(expression), flags)

      _flags = 0;

      for (int i = 0; flags[i] != '\0'; ++i)
         {
         switch (flags[i])
            {
            case 'i': _flags |= PCRE_CASELESS;  case_t = true;   break;
            case 'm': _flags |= PCRE_MULTILINE;                  break;
            case 's': _flags |= PCRE_DOTALL;                     break;
            case 'x': _flags |= PCRE_EXTENDED;                   break;
            case 'g':                           global_t = true; break;
            }
         }

      zero();
      compile();
      }

   /**
   The destructor will automatically invoked if the object is no more used. It frees all the memory allocated by pcre.
   */

   ~UPCRE();

   /**
   reset the object and re-intialize it

   @param expression a string, which must be a valid perl regular expression
   */

   void set(const UString& expression)
      {
      U_TRACE(0, "UPCRE::set(%.*S)", U_STRING_TO_TRACE(expression))

      if (expression.empty() == false)
         {
         _flags      = 0;
         _expression = expression;

         case_t = global_t = false;

         reset();
         compile();
         }
      }

   UString getMask() { return _expression; }

   /**
   Return pointer to underlying pcre object.
   The pcre object allows you to access the pcre API directly. E.g. if your are using pcre version 4.x and want to use the
   new functionality which is currently not supported by the class.
   An example would be: pcre_fullinfo(), pcre_study() or the callout functionality

   @return pcre* pointer to pcre object.
   @see man pcre
   @see pcre_extra* getPcreExtra()
   */

   pcre* getPcre() { return p_pcre; }

   /**
   Return pointer to underlying pcre_extra structure.
   The returned pcre_extra structure can be used in conjunction with the pcre* object returned by pcre()

   @return pcre_extra* pointer to pcre_extra structure.
   @see pcre* getPcre()
   */

   pcre_extra* getPcreExtra() { return p_pcre_extra; }

   /**
   Analyze pattern for speeding up the matching process.
   When a pattern is going to be used several times, it is worth spending more time
   analyzing it in order to speed up the time taken for matching.
   */

   void study(int options = 0)
      {
      U_TRACE(1, "UPCRE::study(%d)", options)

      p_pcre_extra = (pcre_extra*) U_SYSCALL(pcre_study, "%p,%d,%p", p_pcre, options, &err_str);

#  ifdef DEBUG
      if (err_str) U_INTERNAL_DUMP("pcre_study() failed: %S", err_str);
#  endif
      }

   /**
   Do a search on the given string beginning at the given offset.

   @param stuff the string in which you want to search for something
   @param offset the offset where to start the search
   @param options can contain any combination of options PCRE_ANCHORED, PCRE_NOTBOL, PCRE_NOTEOL. PCRE_NOTEMPTY

   @return boolean <b>true</b> if the regular expression matched. <b>false</b> if not
   */

   bool search(const char* stuff, uint32_t stuff_len = 0, int offset = 0, int options = 0);

   bool search(const UString& stuff, int offset = 0, int options = 0) { return search(U_STRING_TO_PARAM(stuff), offset, options); }

   /**
   Test if a search was successfull
   This method must be invoked <b>after</b> calling search()

   @return boolean <b>true</b> if the search was successfull at all, or <b>false</b> if not
   */

   bool matched() { return did_match; };

   /**
   Get the number of substrings generated

   @return the number of substrings generated
   */

   int matches() { return num_matches; }

   /**
   Return a vector of substrings, if any

   @return a pointer to an UVector<UString>, which may be NULL, if no substrings has been found
   */

   UVector<UString>* getSubStrings() { return resultset; }

   /**
   Get a substring at a known position

   @param pos the position of the substring to return. Identical to perl's $1..$n

   @return the substring at the given position

   Example:
   @code
   UString mysub = regex.getMatch(1);
   @endcode
   Get the first substring that matched the expression in the "regex" object
   */

   UString getMatch(int pos, bool check = false);

   /**
   Get the length of a substring at a known position

   @param pos the position of the substring-length to return. Identical to perl's $1..$n

   @return the length substring at the given position
   */

   uint32_t getMatchLength(int pos) { return getMatch(pos).size(); }

   /**
   Return substring of a match at a known possition using the array notation

   @param pos the position of the substring to return. Identical to perl's $1..$n

   @return the substring at the given position

   Example:
   @code
   UString mysub = regex[1];
   @endcode
   Get the first substring that matched the expression in the "regex" object
   @see UString getMatch(int pos)
   */

   UString operator[](int index) { return getMatch(index); }

   /**
   Get the start position of the entire match within the searched string.
   This method returns the character position of the first character of the entire match within the searched string

   @return the integer character position of the first character of the entire match
   Example:
   @code
   UPCRE regex("([0-9]+)\s([a-z]+)");   // search for the date(makes 2 substrings
   regex.search("The 11th september."); // do the search on this string
   int pos = regex.getMatchStart();     // returns 4, because "11th september" begins at the 4th character
                                        // inside the search string
   @endcode
   @see int getMatchStart(int pos)
   @see int getMatchEnd(int pos)
   @see int getMatchEnd()
   */

   int getMatchStart()
      {
      U_TRACE(0, "UPCRE::getMatchStart()")

      int result = (sub_vec ? sub_vec[0] : -1);

      U_RETURN(result);
      }

   /**
   Get the start position of a substring within the searched string.
   This method returns the character position of the first character of a substring withing the searched string

   @param pos the position of the substring. Identical to perl's $1..$n

   @return the integer character position of the first character of a substring. Positions are starting at 0

   Example:
   @code
   UPCRE regex("([0-9]+)");             // search for numerical characters
   regex.search("The 11th september."); // do the search on this string
   UString day = regex.getMatch(1);     // returns "11"
   int pos = regex.getMatchStart(1);    // returns 4, because "11" begins at the 4th character inside the search string
   @endcode
   @see int getMatchStart()
   @see int getMatchEnd()
   @see int getMatchEnd(int pos)
   */

   int getMatchStart(int pos)
      {
      U_TRACE(0, "UPCRE::getMatchStart(%d)", pos)

      U_INTERNAL_ASSERT_RANGE(0,pos,num_matches)

      /* sub_vec[0] and [1] is the start/end of the entire string. */

      int result = sub_vec[ (++pos) * 2 ];

      U_RETURN(result);
      }

   /**
   Get the end position of the entire match within the searched string.
   This method returns the character position of the last character of the entire match within the searched string

   @return the integer character position of the last character of the entire match

   Example:
   @code
   UPCRE regex("([0-9]+)\s([a-z]+)");   // search for the date (makes 2 substrings)
   regex.search("The 11th september."); // do the search on this string
   int pos = regex.getMatchEnd();       // returns 17, because "11th september", which is the entire match,
                                        // ends at the 17th character inside the search string
   @endcode
   @see int getMatchStart()
   @see int getMatchStart(int pos)
   @see int getMatchEnd(int pos)
   */

   int getMatchEnd()
      {
      U_TRACE(0, "UPCRE::getMatchEnd()")

      int result = (sub_vec ? sub_vec[1] - 1 : -1);

      U_RETURN(result);
      }

   /**
   Get the end position of a substring within the searched string.
   This method returns the character position of the last character of a substring withing the searched string

   @param pos the position of the substring. Identical to perl's $1..$n

   @return the integer character position of the last character of a substring. Positions are starting at 0

   Example:
   @code
   UPCRE regex("([0-9]+)");             // search for numerical characters
   regex.search("The 11th september."); // do the search on this string
   UString day = regex.getMatch(1);     // returns "11"
   int pos = regex.getMatchEnd(1);      // returns 5, because "11" ends at the 5th character inside the search string
   @endcode
   @see int getMatchStart(int pos)
   @see int getMatchStart()
   @see int getMatchEnd()
   */

   int getMatchEnd(int pos)
      {
      U_TRACE(0, "UPCRE::getMatchEnd(%d)", pos)

      U_INTERNAL_ASSERT_RANGE(0,pos,num_matches)

      /* the end offset of a subpattern points to the first offset of the next substring, therefore -1 */

      int result = sub_vec[ ((++pos) * 2) + 1 ] - 1;

      U_RETURN(result);
      }

   /**
   split a string into pieces. This method will split the given string into a vector
   of strings using the compiled expression (given to the constructor)

   @param vec an vector of strings
   @param piece The string you want to split into it's parts
   @param limit the maximum number of elements you want to get back from split()
   @param start_offset at which substring the returned vector should start
   @param end_offset at which substring the returned vector should end

   @return the number of split parts
   */

   uint32_t split(UVector<UString>& vec, const UString& piece, int limit = 0, int start_offset = 0, int end_offset = 0);

   /**
   Replace parts of a string using regular expressions. This method is the counterpart of the perl s// operator.
   It replaces the substrings which matched the given regular expression (given to the constructor) with the supplied string

   @param piece the string in which you want to search and replace
   @param with  the string which you want to place on the positions which match the expression (given to the constructor)
   */

   UString replace(const UString& piece, const UString& with);

   // regular expressions that I have found the most useful for day-to-day web programming tasks...

// static bool isValidURL(const UString& url);
   static bool validateUsername(const UString& username); // /^[a-zA-Z0-9_]{3,16}$/

   // Matching an XHTML/XML tag with a certain attribute value
   // The function tags an attribute, value, input text, and an optional tag name as arguments.
   // If no tag name is specified it will match any tag with the specified attribute and attribute value

   static uint32_t getTag(UVector<UString>& vec, const UString& xml, const char* attr, const char* value, const char* tag = "\\w+");

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   pcre* p_pcre;                /* pcre object pointer */
   pcre_extra* p_pcre_extra;    /* stuff required by pcre lib */
   const unsigned char* tables; /* locale tables */
   UVector<UString>* resultset; /* store substrings, if any */

   int* sub_vec;
   const char* err_str;
   const char** stringlist;

   UString _expression;         /* the given regular expression */
   int _flags;                  /* the given flags, 0 if not defined */
   int num_matches;             /* number of matches if UVector<UString>* expected */
   int sub_len, erroffset;
   bool did_match;              /* true if the expression produced a match */
   bool case_t, global_t;       /* internal compile flags, used by replace() and split() */

   void compile();              /* compile the pattern */

private:
   void checkBrackets() U_NO_EXPORT;
   const char* status(int num) U_NO_EXPORT;

   /* replace $1 .. $n with the corresponding substring, used by replace() */

   UString _replace_vars(const UString& piece) U_NO_EXPORT;

   UPCRE(const UPCRE&)            {}
   UPCRE& operator=(const UPCRE&) { return *this; }
};

#endif
