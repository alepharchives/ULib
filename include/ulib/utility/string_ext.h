// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    string_ext.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_STRING_EXT_H
#define ULIB_STRING_EXT_H 1

#include <ulib/container/vector.h>

#ifdef HAVE_SSL
#  include <openssl/pem.h>
#endif

struct U_EXPORT UStringExt {

#ifdef HAVE_SSL
   static UString BIOtoString(BIO* bio);
#endif

   static UString deflate(const UString& s);                       // .gz   compress
   static UString gunzip( const UString& s, uint32_t sz_orig = 0); // .gz uncompress

   // LZO method

   static UString   compress(const UString& s);
   static UString decompress(const UString& s);

   static bool isCompress(const UString& s) { return U_STRNEQ(s.data(), U_LZOP_COMPRESS); }

   // Convert integer to string

   static UString numberToString(long i)
      {
      U_TRACE(0, "UStringExt::numberToString(%ld)", i)

      UString x(32U);

      x.snprintf("%ld", i);

      U_RETURN_STRING(x);
      }

   // convert letter to upper or lower case

   static UString tolower(const UString& s);
   static UString toupper(const UString& s);

   // manage pathname

   static UString suffix(  const UString& s, char sep = '.');
   static UString dirname( const UString& s);
   static UString basename(const UString& s);

   // check if string s1 start with string s2

   static bool startsWith(const UString& s1, const UString& s2)
      { return u_startsWith(U_STRING_TO_PARAM(s1), U_STRING_TO_PARAM(s2)); }

   static bool startsWith(const UString& s1, const char* s2, uint32_t n2)
      { return u_startsWith(U_STRING_TO_PARAM(s1), s2, n2); }

   // check if string s1 terminate with string s2

   static bool endsWith(const UString& s1, const UString& s2)
      { return u_endsWith(U_STRING_TO_PARAM(s1), U_STRING_TO_PARAM(s2)); }

   static bool endsWith(const UString& s1, const char* s2, uint32_t n2)
      { return u_endsWith(U_STRING_TO_PARAM(s1), s2, n2); }

   // SUBSTITUTE: sostituzione di tutte le occorrenze di 'a' con 'b'

   static UString substitute(const char* s, uint32_t n,
                             const char* a, uint32_t n1,
                             const char* b, uint32_t n2);

   static UString substitute(const UString& s, char a, char b)
      { return substitute(U_STRING_TO_PARAM(s), &a, 1, &b, 1); }

   static UString substitute(const UString& s, char a, const char* b, uint32_t n2)
      { return substitute(U_STRING_TO_PARAM(s), &a, 1, b, n2); }

   static UString substitute(const UString& s, const char* a, uint32_t n1, char b)
      { return substitute(U_STRING_TO_PARAM(s), a, n1, &b, 1); }

   static UString substitute(const UString& s, const char* a, uint32_t n1, const char* b, uint32_t n2)
      { return substitute(U_STRING_TO_PARAM(s), a, n1, b, n2); }

   static UString substitute(const UString& s, const UString& a, const UString& b)
      { return substitute(U_STRING_TO_PARAM(s), U_STRING_TO_PARAM(a), U_STRING_TO_PARAM(b)); }

   // dos2unix '\n' convertor

   static UString dos2unix(const UString& s, bool unix2dos = false);

   // convert tabs to spaces

   static UString expandTab(const char* s, uint32_t n, int tab = 3);
   static UString expandTab(const UString& s, int tab = 3) { return expandTab(U_STRING_TO_PARAM(s), tab); }

   // expand path (~/... and ~user/... plus $var and $var/...)

   static UString expandPath(const char* s, uint32_t n);
   static UString expandPath(const UString& s) { return expandPath(U_STRING_TO_PARAM(s)); }

   // recursively expand environment variables if needed

   static UString expandEnvVar(const char* s, uint32_t n);
   static UString expandEnvVar(const UString& s) { return expandEnvVar(U_STRING_TO_PARAM(s)); }

   static void putenv(const char* name, uint32_t value)
      {
      U_TRACE(1, "UStringExt::putenv(%S,%u)", name, value)

      UString buffer(U_CAPACITY);

      buffer.snprintf("%s=%u", name, value);

      char* p = buffer.c_strdup();

      (void) U_SYSCALL(putenv, "%S", p);
      }

   static void putenv(const char* name, const char* value)
      {
      U_TRACE(1, "UStringExt::putenv(%S,%S)", name, value)

      UString buffer(U_CAPACITY);

      buffer.snprintf("%s=%s", name, value);

      char* p = buffer.c_strdup();

      (void) U_SYSCALL(putenv, "%S", p);
      }

   static void putenv(const char* name, const UString& value)
      {
      U_TRACE(1, "UStringExt::putenv(%S,%.*S)", name, U_STRING_TO_TRACE(value))

      UString buffer(U_CAPACITY);

      buffer.snprintf("%s=%.*s", name, U_STRING_TO_TRACE(value));

      char* p = buffer.c_strdup();

      (void) U_SYSCALL(putenv, "%S", p);
      }

   // manage escaping for delimiter character
   
   static UString removeEscape(const char* s, uint32_t n);
   static UString insertEscape(const char* s, uint32_t n, char delimiter = '"');

   static UString removeEscape(const UString& s)
      {
      U_TRACE(0, "UStringExt::removeEscape(%.*S)", U_STRING_TO_TRACE(s))

      uint32_t sz     = s.size();
      const char* str = s.data();
      void* ptr       = (void*) memchr(str, '\\', sz);

      return (ptr ? removeEscape(str, sz) : s);
      }

   static UString insertEscape(const UString& s, char delimiter = '"')
      {
      U_TRACE(0, "UStringExt::insertEscape(%.*S,%C)", U_STRING_TO_TRACE(s), delimiter)

      uint32_t sz     = s.size();
      const char* str = s.data();
      void* ptr       = (void*) memchr(str, delimiter, sz);

      return (ptr ? insertEscape(str, sz, delimiter) : s);
      }

   // Returns a string that has whitespace removed from the start and the end (leading and trailing)

   static UString trim(const char* s, uint32_t n);
   static UString trim(const UString& s) { return trim(U_STRING_TO_PARAM(s)); }

   // returns a string that has whitespace removed from the start and the end, and
   // which has each sequence of internal whitespace replaced with a single space

   static UString simplifyWhiteSpace(const char* s, uint32_t n);
   static UString simplifyWhiteSpace(const UString& s) { return simplifyWhiteSpace(U_STRING_TO_PARAM(s)); }

   // Sort two version numbers, comparing equivalently seperated strings of digits numerically
   // ----------------------------------------------------------------------------------------
   // Returns a positive number if (a > b)
   // Returns a negative number if (a < b)
   // Returns zero              if (a == b)
   // ----------------------------------------------------------------------------------------
   static int compareversion(const char* a, uint32_t n1, const char* b, uint32_t n2);

   static int compareversion(const UString& s, const UString& a)
      { return compareversion(U_STRING_TO_PARAM(s), U_STRING_TO_PARAM(a)); }

   static int compareversion(const UString& s, const char* a, uint32_t n)
      { return compareversion(U_STRING_TO_PARAM(s), a, n); }

   /* Verifies that the passed string is actually an e-mail address */

   static bool isEmailAddress(const UString& s)
      {
      U_TRACE(0, "UStringExt::isEmailAddress(%.*S)", U_STRING_TO_TRACE(s))

      if (u_validate_email_address(U_STRING_TO_PARAM(s))) U_RETURN(true);

      U_RETURN(false);
      }

   // retrieve information on form elements as couple <name1>=<value1>&<name2>=<value2>&...

   static uint32_t getNameValueFromData(const UString& content, UVector<UString>& name_value, const char* delim = "&", uint32_t dlen = 1);

#  define U_TOKEN_NM 4U
#  define U_TOKEN_LN (U_TOKEN_NM + 8U)

   // -----------------------------------------------------------------------------------------------------------------------
   // Very simple RPC-like layer
   //
   // Requests and responses are build of little packets each containing a U_TOKEN_NM-byte ascii token,
   // an 8-byte hex value or length, and optionally data corresponding to the length.
   // -----------------------------------------------------------------------------------------------------------------------

   // built token name (U_TOKEN_NM characters) and value (32-bit int, as 8 hex characters)

   static void buildTokenInt(const char* token, uint32_t value, UString& buffer);

   static void buildTokenString(const char* token, const UString& value, UString& buffer)
      {
      U_TRACE(0, "UStringExt::buildTokenString(%S,%.*S,%.*S)", token, U_STRING_TO_TRACE(value), U_STRING_TO_TRACE(buffer))

      buildTokenInt(token, value.size(), buffer);

      buffer.append(value);
      }

   static void buildTokenVector(const char* token, UVector<UString>& vec, UString& buffer)
      {
      U_TRACE(0, "UStringExt::buildTokenVector(%S,%p,%.*S)", token, &vec, U_STRING_TO_TRACE(buffer))

      uint32_t argc = vec.size();

      buildTokenInt(token, argc, buffer);

      for (uint32_t i = 0; i < argc; ++i)
         {
         buildTokenString("ARGV", vec[i], buffer);
         }
      }
};

#endif
