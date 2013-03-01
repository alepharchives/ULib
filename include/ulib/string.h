// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    string.h - Components for manipulating sequences of characters
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_USTRING_H
#define ULIB_USTRING_H 1

#include <ulib/base/hash.h>
#include <ulib/base/utility.h>
#include <ulib/internal/common.h>

#include <iostream>

#ifdef __clang__
#  define ios        std::ios
#  define istream    std::istream
#  define ostream    std::ostream
#  define streambuf  std::streambuf
#  define streamsize std::streamsize
#endif

#ifndef __MINGW32__
#  ifndef    CONFIG_MMAP_ALLOW_UNINITIALIZED
#     define CONFIG_MMAP_ALLOW_UNINITIALIZED
#  endif
#  include <sys/mman.h>
#  if defined(MAP_ANON) && !defined(MAP_ANONYMOUS)
#     define MAP_ANONYMOUS MAP_ANON /* Don't use a file */
#  endif
#endif

#ifdef MAP_UNINITIALIZED // (since Linux 2.6.33)
#define U_MAP_ANON (MAP_PRIVATE | MAP_ANONYMOUS | MAP_UNINITIALIZED)
#else
#define U_MAP_ANON (MAP_PRIVATE | MAP_ANONYMOUS)
#endif

// macro for constant string like "xxx"

#define U_STRING_RFIND(x,str)                  (x).rfind(str,U_NOT_FOUND,U_CONSTANT_SIZE(str))    // string constant
#define U_STRING_FIND(x,start,str)             (x).find(str,(start),U_CONSTANT_SIZE(str))         // string constant
#define U_STRING_FIND_EXT(x,start,str,n)       (x).find(str,(start),U_CONSTANT_SIZE(str),n)       // string constant
#define U_STRING_FINDNOCASE_EXT(x,start,str,n) (x).findnocase(str,(start),U_CONSTANT_SIZE(str),n) // string constant

#define U_STRING_FROM_CONSTANT(str) UString(str,U_CONSTANT_SIZE(str))                             // string constant
#define U_ENDS_WITH(x,str)          u_endsWith((x).data(),(x).size(),U_CONSTANT_TO_PARAM(str))    // string constant

// UString content and size

#define U_STRING_TO_PARAM(str)              (str).data(),(str).size()
#define U_STRING_TO_TRACEX(str,start,count) count,(str).data()+start

#ifdef DEBUG_DEBUG
#define U_STRING_TO_TRACE(str)  U_min(128,(str).size()),(str).data()
#else
#define U_STRING_TO_TRACE(str)            (str).size(), (str).data()
#endif

// UStringRep: string representation
// 
// The string object requires only one allocation. The allocation function which gets a block of raw bytes
// and with room enough and constructs a UStringRep object at the front. Invariants:
// ------------------------------------------------------------------------------------------------------
// 1. string really contains length+1 characters; last is set to '\0' only on call to c_str()
// 2. capacity >= length - allocated memory is always capacity+1
// 3. references has two states:
//       0: one reference
//     n>0: n+1 references
// 4. all fields == 0 is an empty string, given the extra storage beyond-the-end for a null terminator;
//    thus, the shared empty string representation needs no constructor
// ------------------------------------------------------------------------------------------------------
// Note that the UStringRep object is a POD so that you can have a static "empty string" UStringRep object
// already "constructed" before static constructors have run. The reference-count encoding is chosen so that
// a 0 indicates one reference, so you never try to destroy the empty-string UStringRep object

#ifdef DEBUG
#  define U_STRINGREP_FROM_CONSTANT(c_str) (void*)U_CHECK_MEMORY_SENTINEL, 0, 0, U_CONSTANT_SIZE(c_str), 0, 0, c_str
#elif defined(U_SUBSTR_INC_REF)
#  define U_STRINGREP_FROM_CONSTANT(c_str)                                 0,    U_CONSTANT_SIZE(c_str), 0, 0, c_str
#else
#  define U_STRINGREP_FROM_CONSTANT(c_str)                                       U_CONSTANT_SIZE(c_str), 0, 0, c_str
#endif

#define U_STRING_FROM_STRINGREP_STORAGE(n) UString(&(stringrep_storage[n]))

class UCDB;
class URDB;
class UHTTP;
class UValue;
class UString;
class UStringExt;
class UHttpPlugIn;
class USSLSession;
class Application;
class UHashMapNode;
class URDBClient_Base;
template <class T> class UVector;
template <class T> class UHashMap;

class U_EXPORT UStringRep {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

#if defined(U_SUBSTR_INC_REF) || defined(DEBUG)
   UStringRep* parent; // manage substring to increment reference of source string
#  ifdef DEBUG
   int32_t child;      // manage substring to capture event 'DEAD OF SOURCE STRING WITH CHILD ALIVE'...
#  endif
#endif

   // --------------------------
   // references has two states:
   // --------------------------
   //   0: one reference
   // n>0: n+1 references
   // --------------------------

   bool uniq() const
      {
      U_TRACE(0, "UStringRep::uniq()")

      U_CHECK_MEMORY

      bool result = (references == 0); // NB: 0 -> one reference

      U_RETURN(result);
      }

   void hold()
      {
      U_TRACE(0, "UStringRep::hold()")

      ++references;

      U_INTERNAL_DUMP("this = %p parent = %p references = %d child = %d", this, parent, references + 1, child)

      U_CHECK_MEMORY
      }

   void release(); // NB: we don't use delete (dtor) because add a deallocation to the destroy object process...

   // NB: we don't use new (ctor) because we want an allocation with more space for string data...
   static UStringRep* create(uint32_t length, uint32_t capacity, const char* ptr);

   // Size and Capacity

   uint32_t size() const   { return _length; }
   uint32_t length() const { return _length; }

   bool empty() const
      {
      U_TRACE(0, "UStringRep::empty()")

      U_CHECK_MEMORY

      U_RETURN(_length == 0);
      }

   uint32_t capacity() const
      {
      U_TRACE(0, "UStringRep::capacity()")

      U_CHECK_MEMORY

      U_RETURN(_capacity);
      }

   bool writeable() const
      {
      U_TRACE(0, "UStringRep::writeable()")

      U_CHECK_MEMORY

      U_RETURN(_capacity != 0); // mode: 0 -> const
      }

   bool isNullTerminated() const  { return (str [_length] == '\0'); }
   void setNullTerminated() const { ((char*)str)[_length]  = '\0'; }

   uint32_t space() const
      {
      U_TRACE(0, "UStringRep::space()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR((int32_t)_capacity,0)

      U_RETURN(_capacity - _length);
      }

   uint32_t remain(const char* ptr) const
      {
      U_TRACE(0, "UStringRep::remain(%p)", ptr)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT((str + ((int32_t)_capacity > 0 ? _capacity : _length)) >= ptr)

      U_RETURN((str + _length) - ptr);
      }

   uint32_t distance(const char* ptr) const
      {
      U_TRACE(0, "UStringRep::distance(%p)", ptr)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT((str + ((int32_t)_capacity > 0 ? _capacity : _length)) >= ptr)

      U_RETURN(ptr - str);
      }

   static uint32_t fold(uint32_t pos, uint32_t off, uint32_t sz)
      {
      U_TRACE(0, "UStringRep::fold(%u,%u,%u)", pos, off, sz)

      uint32_t dist   = (sz - pos),
               newoff = (off < dist ? off : dist);

      U_RETURN(newoff);
      }

   uint32_t fold(uint32_t pos, uint32_t off) const { return fold(pos, off, _length); }

   // The maximum number of individual char elements of an individual string is determined by max_size()
   // (Whereas U_NOT_FOUND is the maximum number of bytes the allocator can allocate)

   static uint32_t max_size(uint32_t sz = U_NOT_FOUND) { return ((sz - sizeof(UStringRep))/sizeof(char)) - 1; }

   // C-Style String

   char* data() const { return (char*)str; }

   void copy(char* s, uint32_t n = U_NOT_FOUND, uint32_t pos = 0) const;

   // ELEMENT ACCESS

         char* begin()        { return (char*)str; }
   const char* begin() const  { return str; }
         char* end()          { return (char*)(str + _length); }
   const char* end() const    { return str + _length; }
         char* rbegin()       { return (char*)(str + _length - 1); }
   const char* rbegin() const { return str + _length - 1; }
         char* rend()         { return (char*)(str + 1); }
   const char* rend() const   { return str + 1; }

   char at(uint32_t pos) const
      {
      U_TRACE(0, "UStringRep::at(%u)", pos)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MINOR(pos, _length)

      U_RETURN(str[pos]);
      }

   char first_char() const
      {
      U_TRACE(0, "UStringRep::first_char()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(_length,0)

      U_RETURN(str[0]);
      }

   char last_char() const
      {
      U_TRACE(0, "UStringRep::last_char()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(_length,0)

      U_RETURN(str[_length - 1]);
      }

   char operator[](uint32_t pos) const { return str[pos]; }

   // Compare

   int compare(const char* s, uint32_t n) const
      {
      U_TRACE(0, "UStringRep::compare(%.*S,%u)", n, s, n)

      U_CHECK_MEMORY

      int r = memcmp(str, s, U_min(_length, n));

      U_INTERNAL_DUMP("str = %.*S", U_min(_length, n), str)

      if (r == 0) r = (_length - n);

      U_RETURN(r);
      }

   int compare(const UStringRep* rep) const { return compare(rep->str, rep->_length); }

   int compare(uint32_t pos, uint32_t n1, const char* s, uint32_t n2) const
      {
      U_TRACE(0, "UStringRep::compare(%u,%u,%.*S,%u)", pos, n1, n2, s, n2)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT((pos + n1) <= _length)

      int r = memcmp(str + pos, s, U_min(n1, n2));

      if (r == 0) r = (n1 - n2);

      U_RETURN(r);
      }

   int compare(const UStringRep* rep, uint32_t depth) const __pure;

   // Compare with ignore case

   int comparenocase(const char* s, uint32_t n) const
      {
      U_TRACE(0, "UStringRep::comparenocase(%.*S,%u)", n, s, n)

      U_CHECK_MEMORY

      int r = strncasecmp(str, s, U_min(_length, n));

      U_INTERNAL_DUMP("str = %.*S", U_min(_length, n), str)

      if (r == 0) r = (_length - n);

      U_RETURN(r);
      }

   int comparenocase(const UStringRep* rep) const { return comparenocase(rep->str, rep->_length); }

   // Equal

   bool equal(const char* s, uint32_t n) const
      {
      U_TRACE(0, "UStringRep::equal(%#.*S,%u)", n, s, n)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_POINTER(s)

      if ((_length == n) && (memcmp(str, s, n) == 0)) U_RETURN(true);

      U_RETURN(false);
      }

   bool equal(const UStringRep* rep) const { return equal(rep->str, rep->_length); }

   // Equal with ignore case

   bool equalnocase(const char* s, uint32_t n) const
      {
      U_TRACE(0, "UStringRep::equalnocase(%.*S,%u)", n, s, n)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_POINTER(s)

      bool r = (_length == n) && (strncasecmp(str, s, n) == 0);

      U_RETURN(r);
      }

   bool equalnocase(const UStringRep* rep) const { return equalnocase(rep->str, rep->_length); }

   bool equal(const UStringRep* rep, bool ignore_case) const
      {
      U_TRACE(0, "UStringRep::equal(%p,%b)", rep, ignore_case)

      U_CHECK_MEMORY

      uint32_t n    = rep->_length;
      const char* s = rep->str;

      if (_length == n)
         {
         bool r = ((ignore_case ? strncasecmp(str, s, n)
                                :      memcmp(str, s, n)) == 0);

         U_RETURN(r);
         }

      U_RETURN(false);
      }

   // Substring

   UStringRep* substr(const char* t, uint32_t tlen);

   UStringRep* substr(uint32_t pos, uint32_t n) { return substr(str + pos, n); }

   // Assignment

   static void assign(UStringRep*& rep, const char* s, uint32_t n);

   void size_adjust(      uint32_t value = U_NOT_FOUND);
   void size_adjust_force(uint32_t value = U_NOT_FOUND)
      {
      U_TRACE(0, "UStringRep::size_adjust_force(%u)", value)

      U_CHECK_MEMORY

      _length = (value == U_NOT_FOUND ? u__strlen(str) : value);

      U_INTERNAL_ASSERT(invariant())
      }

#if defined(DEBUG) || defined(U_TEST)
   bool invariant() const;
   const char* dump(bool reset) const;
#endif

   // EXTENSION

   bool isText(uint32_t pos) const __pure;
   bool isUTF8(uint32_t pos) const __pure;
   bool isUTF16(uint32_t pos) const __pure;
   bool isBase64(uint32_t pos) const __pure;
   bool isBinary(uint32_t pos) const __pure;
   bool isEndHeader(uint32_t pos) const __pure;
   bool isWhiteSpace(uint32_t pos) const __pure;
   bool someWhiteSpace(uint32_t pos) const __pure;

   // UTF8 <--> ISO Latin 1

   static UStringRep*   toUTF8(const unsigned char* t, uint32_t tlen);
   static UStringRep* fromUTF8(const unsigned char* t, uint32_t tlen);

   bool strtob() const __pure;
#ifdef HAVE_STRTOF
   float strtof() const;
#endif
   double strtod() const;
#ifdef HAVE_STRTOLD
   long double strtold() const;
#endif
   long strtol(int base = 0) const;
#ifdef HAVE_STRTOULL
   int64_t strtoll(int base = 0) const;
#endif

   uint32_t hash(bool ignore_case = false) const
      {
      U_TRACE(0, "UStringRep::hash(%b)", ignore_case)

      U_CHECK_MEMORY

      uint32_t result = u_hash((unsigned char*)str, _length, ignore_case);

      U_RETURN(result);
      }

   // for constant string

   void trim();

   // if the string is quoted...

   bool isQuoted(unsigned char c) const
      {
      U_TRACE(0, "UStringRep::isQuoted(%C)", c)

      U_CHECK_MEMORY

      bool result = (str[0]         == c &&
                     str[_length-1] == c);

      U_RETURN(result);
      }

   // ...unquote it

   void unQuote()
      {
      U_TRACE(0, "UStringRep::unQuote()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(_length, 2)
      U_INTERNAL_ASSERT_EQUALS(_capacity, 0)

      ++str;
      _length -= 2;
      }

   // STREAM

   void write(ostream& os) const;

   friend ostream& operator<<(ostream& os, const UStringRep& r) { r.write(os); return os; }

protected:
   uint32_t _length,
            _capacity,  // [0 const | -1 mmap]...
            references; // NB: must be here, see string_rep_null...
   const char* str;
   // ----------------> maybe unnamed array of char...

   // The following storage is init'd to 0 by the linker, resulting (carefully) in an empty string with zero(=one) reference...
   static UStringRep* string_rep_null;

#ifdef DEBUG
   static int32_t max_child;
   static UStringRep* parent_destroy;
   static UStringRep* string_rep_share;
   static bool check_dead_of_source_string_with_child_alive;

   static bool checkIfChild(     const char* name_class, const void* ptr_object);
   static bool checkIfReferences(const char* name_class, const void* ptr_object);
#endif

private:
   explicit UStringRep(const char* t, uint32_t tlen);
           ~UStringRep();

   UStringRep(const UStringRep&)            {}
   UStringRep& operator=(const UStringRep&) { return *this; }

   void set(uint32_t length, uint32_t capacity, const char* ptr) U_NO_EXPORT;

                      friend void ULib_init();
   template <class T> friend void u_construct(T*, uint32_t);

                      friend class UCDB;
                      friend class URDB;
                      friend class UHTTP;
                      friend class UString;
   template <class T> friend class UVector;
   template <class T> friend class UHashMap;
                      friend struct UObjectIO;
                      friend class UStringExt;
                      friend class UHttpPlugIn;
                      friend class USSLSession;
                      friend class Application;
                      friend class UHashMapNode;
                      friend class URDBClient_Base;
};

class U_EXPORT UString {
public:

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // null string (for container etc...)

   static UString& getStringNull()
      {
      U_INTERNAL_ASSERT_EQUALS((bool)*string_null, false)

      return *string_null;
      }

protected:
   static UString* string_null;

                      friend void ULib_init();

                      friend class UValue;
   template <class T> friend class UVector;
                      friend class UStringExt;

   explicit UString(uint32_t len, uint32_t sz, char* ptr); // NB: for UStringExt::deflate()...

public:
   // mutable
   UStringRep* rep;

   // SERVICES

   char* data() const         { return rep->data(); }
   bool empty() const         { return rep->empty(); }

   operator bool() const      { return (rep->_length != 0); }

   uint32_t size() const      { return rep->size(); }
   uint32_t space() const     { return ((int32_t)rep->_capacity > 0 ? rep->_capacity - rep->_length : 0); }
   uint32_t length() const    { return rep->length(); }
   uint32_t capacity() const  { return rep->capacity(); }

   static uint32_t max_size() { return UStringRep::max_size(); }

protected:
   // in 'memory reference' distinction is made between set, copy, e assign...

   void _set(UStringRep* r)
      {
      U_TRACE(0, "UString::_set(%p)", r)

      U_INTERNAL_ASSERT_DIFFERS(rep,r)

      rep->release();

      rep = r;

      U_CHECK_MEMORY_OBJECT(rep)
      }

   void _copy(UStringRep* r)
      {
      U_TRACE(0, "UString::_copy(%p)", r)

      rep = r;

      rep->hold();

      U_CHECK_MEMORY_OBJECT(rep)
      }

public:
   void _assign(UStringRep* r)
      {
      U_TRACE(0, "UString::_assign(%p)", r)

      // NB: works also int the case of (rep == r)...

      r->hold();        // 1. take a copy of new resource

      rep->release();   // 2. release existing resource

      rep = r;          // 3. bind copy to self

      U_CHECK_MEMORY_OBJECT(rep)
      }

   // COSTRUTTORI

   UString()
      {
      U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, UString, "")

      _copy(UStringRep::string_rep_null);

      U_INTERNAL_DUMP("this = %p rep = %p", this, rep)

      U_INTERNAL_ASSERT(invariant())
      }

   explicit UString(UStringRep* r)
      {
      U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, UString, "%p", r)

      _copy(r);

      U_INTERNAL_DUMP("this = %p rep = %p", this, rep)

      U_INTERNAL_ASSERT(invariant())
      }

   explicit UString(ustringrep* r);

   UString(const UString& str)
      {
      U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, UString, "%p", &str)

      _copy(str.rep);

      U_INTERNAL_DUMP("this = %p rep = %p", this, rep)

      U_INTERNAL_ASSERT(invariant())
      }

   explicit UString(const char* t);
   explicit UString(const char* t, uint32_t tlen);

   explicit UString(uint32_t n);
   explicit UString(uint32_t n, unsigned char c);

   // Copy from string

   explicit UString(void* t)
      {
      U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, UString, "%S", (char*)t)

      U_INTERNAL_ASSERT_POINTER(t)

      uint32_t tlen = u__strlen((char*)t);

      rep = UStringRep::create(tlen, tlen, (const char*)t);

      U_INTERNAL_DUMP("this = %p rep = %p", this, rep)

      U_INTERNAL_ASSERT(invariant())
      }

   explicit UString(void* t, uint32_t tlen)
      {
      U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, UString, "%.*S,%u", tlen, (char*)t, tlen)

      U_INTERNAL_ASSERT_POINTER(t)

      rep = UStringRep::create(tlen, tlen, (const char*)t);

      U_INTERNAL_DUMP("this = %p rep = %p", this, rep)

      U_INTERNAL_ASSERT(invariant())
      }

   explicit UString(const UString& str, uint32_t pos, uint32_t n = U_NOT_FOUND);

   // Substring

   explicit UString(UStringRep* _rep, const char* t, uint32_t tlen)
      {
      U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, UString, "%p,%p,%u", _rep, t, tlen)

      rep = _rep->substr(t, tlen);

      U_INTERNAL_DUMP("this = %p rep = %p", this, rep)

      U_INTERNAL_ASSERT(invariant())
      }

   explicit UString(UStringRep* _rep, uint32_t pos, uint32_t n = U_NOT_FOUND)
      {
      U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, UString, "%p,%u,%u", _rep, pos, n)

      rep = _rep->substr(pos, _rep->fold(pos, n));

      U_INTERNAL_DUMP("this = %p rep = %p", this, rep)

      U_INTERNAL_ASSERT(invariant())
      }

   UString substr(const char* t, uint32_t tlen) const;

   UString substr(uint32_t pos, uint32_t n = U_NOT_FOUND) const;

   // destructor

   ~UString()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UString)

      U_INTERNAL_DUMP("this = %p rep = %p", this, rep)

      U_INTERNAL_ASSERT_POINTER(rep)

      U_CHECK_MEMORY_OBJECT(rep)

      rep->release();
      }

   // Replace

   UString& replace(uint32_t pos, uint32_t n1, uint32_t n2, char c); // NB: unsigned char conflict with a uint32_t at the same parameter position...
   UString& replace(uint32_t pos, uint32_t n1, const char* s, uint32_t n2);

   UString& replace(uint32_t pos1, uint32_t n1, const UString& str, uint32_t pos2, uint32_t n2 = U_NOT_FOUND)
      {
      U_TRACE(0, "UString::replace(%u,%u,%p,%u,%u)", pos1, n1, &str, pos2, n2)

      U_INTERNAL_ASSERT(pos2 <= str.size())

      return replace(pos1, n1, str.data() + pos2, str.rep->fold(pos2, n2));
      }

   UString& replace(const char* s)                                { return replace(0U, size(),          s, u__strlen(s)); }
   UString& replace(unsigned char c)                              { return replace(0U, size(),         1U, c); }
   UString& replace(const UString& str)                           { return replace(0U, size(), str.data(), str.size()); }
   UString& replace(const char* s, uint32_t n)                    { return replace(0U, size(),          s, n); }
   UString& replace(uint32_t pos, uint32_t n, const char* s)      { return replace(pos,     n,          s, u__strlen(s)); }
   UString& replace(uint32_t pos, uint32_t n, const UString& str) { return replace(pos,     n, str.data(), str.size()); }

   // Assignment
   // NB: assign() NON GARANTISCE PROPRIETA' DELLA STRINGA, replace() SI...

   UString& assign(const char* s, uint32_t n)
      {
      U_TRACE(0, "UString::assign(%S,%u)", s, n)

      UStringRep::assign(rep, s, n);

      U_INTERNAL_ASSERT(invariant())

      return *this;
      }

   UString& assign(const UString& str, uint32_t pos, uint32_t n = U_NOT_FOUND)
      {
      U_TRACE(0, "UString::assign(%p,%u,%u)", &str, pos, n)

      U_INTERNAL_ASSERT(pos <= str.size())

      return assign(str.data() + pos, str.rep->fold(pos, n));
      }

   UString& assign(const char* s);
   UString& assign(const UString& str)          { return assign(str.data(), str.size()); }
   UString& assign(uint32_t n, unsigned char c) { return replace(0U, size(), n, c); }

   UString& operator=(const char* s)   { return assign(s); }
   UString& operator=(unsigned char c) { return assign(1U, c); }

   UString& operator=(const UString& str);

   void swap(UString& str)
      {
      U_TRACE(0, "UString::swap(%p)", &str)

      UStringRep* tmp = rep;

          rep = str.rep;
      str.rep = tmp;
      }

   void swap(UString& lhs, UString& rhs) { lhs.swap(rhs); }

   // Make room for a total of n element

   bool reserve(uint32_t n); // NB: return true if has changed rep...

   // Element access

   const char* begin()  const { return rep->begin(); }
   const char* end()    const { return rep->end(); }
   const char* rbegin() const { return rep->rbegin(); }
   const char* rend()   const { return rep->rend(); }

   __pure char at(uint32_t pos) const           { return rep->at(pos); }
          char operator[](uint32_t pos) const   { return rep->operator[](pos); }

   char* _begin()
      {
      U_TRACE(0, "UString::_begin()")

      if (uniq() == false) duplicate();

      return rep->begin();
      }

   char* _end()
      {
      U_TRACE(0, "UString::_end()")

      if (uniq() == false) duplicate();

      return rep->end();
      }

   char* _rbegin()
      {
      U_TRACE(0, "UString::_rbegin()")

      if (uniq() == false) duplicate();

      return rep->rbegin();
      }

   char* _rend()
      {
      U_TRACE(0, "UString::_rend()")

      if (uniq() == false) duplicate();

      return rep->rend();
      }

   // Modifiers

   void push(unsigned      char c) { (void) append(1U, c); }
   void push_back(unsigned char c) { (void) append(1U, c); }

   UString& append(uint32_t n, char c); // NB: unsigned char conflict with a uint32_t at the same parameter position...
   UString& append(const char* s, uint32_t n);

   UString& append(const UString& str, uint32_t pos, uint32_t n = U_NOT_FOUND)
      {
      U_TRACE(0, "UString::append(%p,%u,%u)", &str, pos, n)

      U_INTERNAL_ASSERT(pos <= str.size())

      return append(str.data() + pos, str.rep->fold(pos, n));
      }

   UString& append(const char* s);
   UString& append(UStringRep* _rep);
   UString& append(const UString& str);

   UString& operator+=(const char* s)        { return append(s, u__strlen(s)); }
   UString& operator+=(unsigned char c)      { return append(1U, c); }
   UString& operator+=(const UString& str);

   // OPTMIZE APPEND (BUFFERED)

   static char* ptrbuf;
   static char* appbuf;

   void _append(unsigned char c)
      {
      U_TRACE(0, "UString::_append(%C)", c)

      U_INTERNAL_ASSERT_RANGE(appbuf,ptrbuf,appbuf+1024)

      if ((ptrbuf - appbuf) == sizeof(appbuf))
         {
         (void) append(appbuf, sizeof(appbuf));

         ptrbuf = appbuf;
         }

      *ptrbuf++ = c;
      }

   void _append()
      {
      U_TRACE(0, "UString::_append()")

      if (ptrbuf > appbuf)
         {
         (void) append(appbuf, ptrbuf - appbuf);

         ptrbuf = appbuf;
         }
      }

   // operator +

   friend UString operator+(const UString& lhs, const char* rhs);
   friend UString operator+(const UString& lhs, char rhs);
   friend UString operator+(const UString& lhs, const UString& rhs);

   friend UString operator+(char           lhs, const UString& rhs);
   friend UString operator+(const char*    lhs, const UString& rhs);

   UString& insert(uint32_t pos, const UString& str)
      { return replace(pos, 0, str.data(), str.size()); }

   UString& insert(uint32_t pos1, const UString& str, uint32_t pos2, uint32_t n = U_NOT_FOUND)
      {
      U_TRACE(0, "UString::insert(%u,%p,%u,%u)", pos1, &str, pos2, n)

      U_INTERNAL_ASSERT(pos2 <= str.size())

      return replace(pos1, 0, str.data() + pos2, str.rep->fold(pos2, n));
      }

   UString& insert(uint32_t pos,             char c) { return replace(pos, 0, 1, c); }
   UString& insert(uint32_t pos, uint32_t n, char c) { return replace(pos, 0, n, c); } // NB: uchar conflict with a uint32_t at the same parameter position

   UString& insert(uint32_t pos, const char* s)             { return replace(pos, 0, s, u__strlen(s)); }
   UString& insert(uint32_t pos, const char* s, uint32_t n) { return replace(pos, 0, s, n); }

   void clear();

   void resize(uint32_t n, unsigned char c = '\0');

   UString& erase(uint32_t pos = 0, uint32_t n = U_NOT_FOUND);

   // C-Style String

   void setNullTerminated() const;

   const char* c_str() const
      {
      U_TRACE(0, "UString::c_str()")

      if (isNullTerminated() == false) setNullTerminated();

      U_RETURN(rep->str);
      }

   char* c_strdup() const;
   char* c_strndup(uint32_t pos = 0, uint32_t n = U_NOT_FOUND) const;

   UString copy() const;
   void    copy(char* s, uint32_t n = U_NOT_FOUND, uint32_t pos = 0) const { rep->copy(s, n, pos); }

   // STRING OPERATIONS

   // The `find' function searches string for a specified string (possibly a single character) and returns
   // its starting position. You can supply the parameter pos to specify the position where search must begin

   uint32_t find(const char* s,      uint32_t pos, uint32_t s_len, uint32_t how_much = U_NOT_FOUND) const __pure;
   uint32_t find(const UString& str, uint32_t pos = 0,             uint32_t how_much = U_NOT_FOUND) const __pure;

   uint32_t find(const char* s,   uint32_t pos = 0) const { return find(s, pos, u__strlen(s), U_NOT_FOUND); }
   uint32_t find(unsigned char c, uint32_t pos = 0) const __pure;

   // The `rfind' function searches from end to beginning string for a specified string (possibly a single
   // character) and returns its starting position. You can supply the parameter pos to specify the position
   // where search must begin

   uint32_t rfind(const char* s,      uint32_t pos, uint32_t n) const __pure;
   uint32_t rfind(const char* s,      uint32_t pos = U_NOT_FOUND) const { return rfind(s, pos, u__strlen(s)); }
   uint32_t rfind(unsigned char c,    uint32_t pos = U_NOT_FOUND) const __pure;
   uint32_t rfind(const UString& str, uint32_t pos = U_NOT_FOUND) const { return rfind(str.data(), pos, str.size()); }

   // The `find_first_of' function searches string for the first match of any character stored in s and returns its position
   // The `find_last_of'  function searches string for the last  match of any character stored in s and returns its position

   uint32_t find_first_of(const char* s,      uint32_t pos, uint32_t n) const __pure;
   uint32_t find_first_of(const char* s,      uint32_t pos = 0) const { return find_first_of(s, pos, u__strlen(s)); }
   uint32_t find_first_of(unsigned char c,    uint32_t pos = 0) const { return find(c, pos); }
   uint32_t find_first_of(const UString& str, uint32_t pos = 0) const { return find_first_of(str.data(), pos, str.size()); }

   uint32_t find_last_of(const char* s,       uint32_t pos, uint32_t n) const __pure;
   uint32_t find_last_of(const char* s,       uint32_t pos = U_NOT_FOUND) const { return find_last_of(s, pos, u__strlen(s)); }
   uint32_t find_last_of(unsigned char c,     uint32_t pos = U_NOT_FOUND) const { return rfind(c, pos); }
   uint32_t find_last_of(const UString& str,  uint32_t pos = U_NOT_FOUND) const { return find_last_of(str.data(), pos, str.size()); }

   // The `find_first_not_of' function searches the first element of string that doesn't match any character stored in s
   // and returns its position
   // The `find_last_not_of'  function searches the last  element of string that doesn't match any character stored in s
   // and returns its position

   uint32_t find_first_not_of(const char* s,      uint32_t pos, uint32_t n) const __pure;
   uint32_t find_first_not_of(const char* s,      uint32_t pos = 0) const { return find_first_not_of(s, pos, u__strlen(s)); }
   uint32_t find_first_not_of(unsigned char c,    uint32_t pos = 0) const __pure;
   uint32_t find_first_not_of(const UString& str, uint32_t pos = 0) const { return find_first_not_of(str.data(), pos, str.size()); }

   uint32_t find_last_not_of(const char* s,      uint32_t pos, uint32_t n) const __pure;
   uint32_t find_last_not_of(const char* s,      uint32_t pos = U_NOT_FOUND) const { return find_last_not_of(s, pos, u__strlen(s)); }
   uint32_t find_last_not_of(unsigned char c,    uint32_t pos = U_NOT_FOUND) const __pure;
   uint32_t find_last_not_of(const UString& str, uint32_t pos = U_NOT_FOUND) const { return find_last_not_of(str.data(), pos, str.size()); }

   // Find with ignore case

   uint32_t findnocase(const char* s,      uint32_t pos, uint32_t s_len, uint32_t how_much = U_NOT_FOUND) const __pure;
   uint32_t findnocase(const UString& str, uint32_t pos = 0,             uint32_t how_much = U_NOT_FOUND) const __pure;

   uint32_t findnocase(const char* s,      uint32_t pos = 0) const { return findnocase(s, pos, u__strlen(s), U_NOT_FOUND); }

   // Compare

   int compare(const char* s) const             { return rep->compare(s, u__strlen(s)); }
   int compare(const char* s, uint32_t n) const { return rep->compare(s, n); }

   int compare(UStringRep* _rep) const          { return rep->compare(_rep); }
   int compare(const UString& str) const        { return rep->compare(str.rep); }

   int compare(uint32_t pos, const char* s) const
      { return rep->compare(pos, u__strlen(s), s, u__strlen(s)); }

   int compare(uint32_t pos, uint32_t n1, const char* s, uint32_t n2) const
      { return rep->compare(pos, U_min(size() - pos, n1), s, U_min(u__strlen(s), n2)); }

   int compare(uint32_t pos, uint32_t n, const char* s) const
      { return rep->compare(pos, U_min(size() - pos, n), s, u__strlen(s)); }

   __pure int compare(uint32_t pos, uint32_t n, const UString& str) const
      { return rep->compare(pos, U_min(size() - pos, n), str.data(), str.size()); }

   __pure int compare(uint32_t pos1, uint32_t n1, const UString& str, uint32_t pos2, uint32_t n2) const
      { return rep->compare(pos1, U_min(size() - pos1, n1), str.data() + pos2,
                                  U_min(str.size() - pos2, n2)); }

   // Compare with ignore case

   int comparenocase(const char* s, uint32_t n) const { return rep->comparenocase(s, n); }
   int comparenocase(const char* s) const             { return rep->comparenocase(s, u__strlen(s)); }

   int comparenocase(UStringRep* _rep) const          { return rep->comparenocase(_rep); }
   int comparenocase(const UString& str) const        { return rep->comparenocase(str.rep); }

   // Same string representation

   bool same(UStringRep* _rep) const                      { return (rep == _rep); }
   bool same(const UString& str) const                    { return same(str.rep); }

   // Equal

   bool equal(const char* s) const __pure;
   bool equal(const char* s, uint32_t n) const __pure;

   bool equal(UStringRep* _rep) const __pure;
   bool equal(const UString& str) const                   { return equal(str.rep); }

   // Equal with ignore case

   bool equalnocase(const char* s, uint32_t n) const      { return rep->equalnocase(s, n); }
   bool equalnocase(const char* s) const                  { return rep->equalnocase(s, u__strlen(s)); }

   bool equalnocase(UStringRep* _rep) const               { return same(_rep) || rep->equalnocase(_rep); }
   bool equalnocase(const UString& str) const __pure;

   bool equal(UStringRep* _rep,   bool ignore_case) const { return same(_rep) || rep->equal(_rep, ignore_case); }
   bool equal(const UString& str, bool ignore_case) const { return equal(str.rep, ignore_case); }

   // STREAM

   istream& getline(istream& is, unsigned char delim = '\n');

   friend U_EXPORT istream& operator>>(istream& is,       UString& str);
   friend U_EXPORT ostream& operator<<(ostream& os, const UString& str);

   // --------------------------------------------------------------
   // STREAM - EXTENSION TO STDLIBC++
   // --------------------------------------------------------------

   void get(istream& is)
      {
      U_TRACE(0, "UString::get(%p)", &is)

      if (is.peek() == '"')
         {
         (void) is.get(); // skip '"'

         (void) getline(is, '"');
         }
      else
         {
         is >> *this;
         }
      }

   void write(ostream& os) const { rep->write(os); }

   // --------------------------------------------------------------

#if defined(DEBUG) || defined(U_TEST)
   bool invariant() const;
#endif
#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

   // -----------------------------------------------------------------------------------------------------------------------
   // START EXTENSION TO STDLIBC++
   // -----------------------------------------------------------------------------------------------------------------------

   bool isNull() const                         { return (rep == UStringRep::string_rep_null); }
   bool isNullTerminated() const               { return rep->isNullTerminated(); }
   bool isText(uint32_t pos = 0) const         { return rep->isText(pos); }
   bool isUTF8(uint32_t pos = 0) const         { return rep->isUTF8(pos); }
   bool isUTF16(uint32_t pos = 0) const        { return rep->isUTF16(pos); }
   bool isBinary(uint32_t pos = 0) const       { return rep->isBinary(pos); }
   bool isBase64(uint32_t pos = 0) const       { return rep->isBase64(pos); }
   bool isEndHeader(uint32_t pos = 0) const    { return rep->isEndHeader(pos); }
   bool isWhiteSpace(uint32_t pos = 0) const   { return rep->isWhiteSpace(pos); }
   bool someWhiteSpace(uint32_t pos = 0) const { return rep->someWhiteSpace(pos); }

   char  last_char() const             { return rep->last_char(); }
   char  first_char() const            { return rep->first_char(); }
   char  c_char(uint32_t pos) const    { return rep->at(pos); }
   char* c_pointer(uint32_t pos) const { return (rep->data() + pos); }

          uint32_t remain(  const char* ptr) const { return rep->remain(ptr); }
   __pure uint32_t distance(const char* ptr) const { return rep->distance(ptr); }

   void setFromInode(uint64_t* p)  { (void) replace((const char*)p, sizeof(uint64_t)); }

   uint32_t hash(bool ignore_case = false) const { return rep->hash(ignore_case); }

   // references

   void hold()    const { rep->hold(); }
   void release() const { rep->release(); }

   // for constant string

   void trim() const { rep->trim(); }

   // if the string is quoted...

   bool isQuoted(unsigned char c = '"') const { return rep->isQuoted(c); }

   // ...unquote it

   void unQuote();

   // set uniq

   void duplicate() const;

   bool uniq() const      { return rep->uniq(); }
   bool writeable() const { return rep->writeable(); }

   // manage UString as constant string...

   void setConstant(const char* t, uint32_t tlen)
      {
      U_TRACE(0, "UString::setConstant(%.*S,%u)", tlen, t, tlen)

      _set(U_NEW(UStringRep(t, tlen)));

      U_INTERNAL_ASSERT(invariant())
      }

   // manage UString as memory mapped area...

   bool isMmap() const
      {
      U_TRACE(0, "UString::isMmap()")

      bool result = (rep->_capacity == U_NOT_FOUND); 

      U_RETURN(result); 
      }

   void mmap(const char* map, uint32_t len);

   // manage UString as buffer...

   void size_adjust(      uint32_t value = U_NOT_FOUND) { rep->size_adjust(value); }
   void size_adjust_force(uint32_t value = U_NOT_FOUND);

   void setEmpty();
   void setEmptyForce() { rep->size_adjust_force(0); }

   void setBuffer(uint32_t n);

   void moveToBeginDataInBuffer(uint32_t n)
      {
      U_TRACE(1, "UString::moveToBeginDataInBuffer(%u)", n)

      U_INTERNAL_ASSERT_RANGE(1,n,max_size())
      U_INTERNAL_ASSERT_MAJOR(rep->_length,n)
      U_INTERNAL_ASSERT_MAJOR(rep->_capacity,n)

#  if defined(DEBUG) && !defined(U_SUBSTR_INC_REF)
      U_INTERNAL_ASSERT(rep->references == 0)
#  endif

      rep->_length -= n;

      (void) U_SYSCALL(memmove, "%p,%p,%u", (void*)rep->str, rep->str + n, rep->_length);

      U_INTERNAL_ASSERT(invariant())
      }

   void snprintf(    const char* format, ...);
   void snprintf_add(const char* format, ...);

   void vsnprintf(const char* format, va_list argp)
      {
      U_TRACE(0, "UString::vsnprintf(%S)", format)

      U_ASSERT(writeable())
      U_INTERNAL_ASSERT(isNull() == false)
      U_INTERNAL_ASSERT(rep->references == 0)
      U_INTERNAL_ASSERT_MAJOR(rep->_capacity,u__strlen(format))

      // NB: +1 because we want space for null-terminator...

      rep->_length = u__vsnprintf(rep->data(), rep->_capacity+1, format, argp); 

      U_INTERNAL_DUMP("ret = %u buffer_size = %u", rep->_length, rep->_capacity+1)

      U_INTERNAL_ASSERT(invariant())
      }

   void vsnprintf_add(const char* format, va_list argp)
      {
      U_TRACE(0, "UString::vsnprintf_add(%S)", format)

      U_ASSERT(writeable())
      U_INTERNAL_ASSERT(isNull() == false)
      U_INTERNAL_ASSERT(rep->references == 0)
      U_ASSERT_MAJOR(rep->space(),u__strlen(format))

      // NB: +1 because we want space for null-terminator...

      uint32_t ret = u__vsnprintf(c_pointer(rep->_length), rep->space()+1, format, argp); 

      U_INTERNAL_DUMP("ret = %u buffer_size = %u", ret, rep->space()+1)

      rep->_length += ret; 

      U_INTERNAL_ASSERT(invariant())
      }

   bool strtob() const                 { return rep->strtob(); }
#ifdef HAVE_STRTOF
   float strtof() const                { return rep->strtof(); }
#endif
   double strtod() const               { return rep->strtod(); }
#ifdef HAVE_STRTOLD
   long double strtold() const         { return rep->strtold(); }
#endif
   long strtol(int base = 0) const     { return rep->strtol(base); }
#ifdef HAVE_STRTOULL
   int64_t strtoll(int base = 0) const { return rep->strtoll(base); }
#endif

   // UTF8 <--> ISO Latin 1

   static UString toUTF8(const unsigned char* t, uint32_t tlen)
      {
      U_TRACE(0, "UString::toUTF8(%.*S,%u)", tlen, t, tlen)

      UString utf8(UStringRep::toUTF8(t, tlen));

      U_RETURN_STRING(utf8);
      }

   static UString fromUTF8(const unsigned char* t, uint32_t tlen)
      {
      U_TRACE(0, "UString::fromUTF8(%.*S,%u)", tlen, t, tlen)

      UString isolat1(UStringRep::fromUTF8(t, tlen));

      U_RETURN_STRING(isolat1);
      }

   // -----------------------------------------------------------------------------------------------------------------------
   // END EXTENSION TO STDLIBC++
   // -----------------------------------------------------------------------------------------------------------------------

private:
   char* __append(uint32_t n) U_NO_EXPORT;
   char* __replace(uint32_t pos, uint32_t n1, uint32_t n2) U_NO_EXPORT;
};

// operator ==

inline bool operator==(const UStringRep& lhs, const UStringRep& rhs) { return lhs.equal(&rhs); }
inline bool operator==(const UString& lhs,    const UString& rhs)    { return lhs.equal(rhs); }
inline bool operator==(const char* lhs,       const UString& rhs)    { return rhs.equal(lhs); }
inline bool operator==(const UString& lhs,    const char* rhs)       { return lhs.equal(rhs); }

// operator !=

inline bool operator!=(const UStringRep& lhs, const UStringRep& rhs) { return lhs.equal(&rhs) == false; }
inline bool operator!=(const UString& lhs,    const UString& rhs)    { return lhs.equal(rhs)  == false; }
inline bool operator!=(const char* lhs,       const UString& rhs)    { return rhs.equal(lhs)  == false; }
inline bool operator!=(const UString& lhs,    const char* rhs)       { return lhs.equal(rhs)  == false; }

// operator <

inline bool operator<(const UStringRep& lhs, const UStringRep& rhs)  { return lhs.compare(&rhs) < 0; }
inline bool operator<(const UString& lhs,    const UString& rhs)     { return lhs.compare(rhs)  < 0; }
inline bool operator<(const char* lhs,       const UString& rhs)     { return rhs.compare(lhs)  > 0; }
inline bool operator<(const UString& lhs,    const char* rhs)        { return lhs.compare(rhs)  < 0; }

// operator >

inline bool operator>(const UStringRep& lhs, const UStringRep& rhs)  { return lhs.compare(&rhs) > 0; }
inline bool operator>(const UString& lhs,    const UString& rhs)     { return lhs.compare(rhs)  > 0; }
inline bool operator>(const char* lhs,       const UString& rhs)     { return rhs.compare(lhs)  < 0; }
inline bool operator>(const UString& lhs,    const char* rhs)        { return lhs.compare(rhs)  > 0; }

// operator <=

inline bool operator<=(const UStringRep& lhs, const UStringRep& rhs) { return lhs.compare(&rhs) <= 0; }
inline bool operator<=(const UString& lhs,    const UString& rhs)    { return lhs.compare(rhs)  <= 0; }
inline bool operator<=(const char* lhs,       const UString& rhs)    { return rhs.compare(lhs)  >= 0; }
inline bool operator<=(const UString& lhs,    const char* rhs)       { return lhs.compare(rhs)  <= 0; }

// operator >=

inline bool operator>=(const UStringRep& lhs, const UStringRep& rhs) { return lhs.compare(&rhs) >= 0; }
inline bool operator>=(const UString& lhs,    const UString& rhs)    { return lhs.compare(rhs)  >= 0; }
inline bool operator>=(const char* lhs,       const UString& rhs)    { return rhs.compare(lhs)  <= 0; }
inline bool operator>=(const UString& lhs,    const char* rhs)       { return lhs.compare(rhs)  >= 0; }

#endif
