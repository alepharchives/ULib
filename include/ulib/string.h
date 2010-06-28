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
#  include <sys/mman.h>
#endif

#include <ulib/base/hash.h>
#include <ulib/base/utility.h>

// macro for constant string like "xxx"

#define U_STRING_RFIND(x,str)             (x).rfind(str,U_NOT_FOUND,U_CONSTANT_SIZE(str))             // string constant
#define U_STRING_FIND(x,start,str)        (x).find( str,(start),U_CONSTANT_SIZE(str))                 // string constant
#define U_STRING_FIND_EXT(x,start,str,n)  (x).find( str,(start),U_CONSTANT_SIZE(str),n)               // string constant
#define U_STRING_FROM_CONSTANT(str)       UString(str,U_CONSTANT_SIZE(str))                           // string constant
#define U_ENDS_WITH(x,str)                u_endsWith((x).data(),(x).size(),U_CONSTANT_TO_PARAM(str))  // string constant

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
// 
// NB: U_NOT_FOUND represents the maximum size that the allocator can hold

#ifdef DEBUG
#  define U_STRINGREP_FROM_CONSTANT(c_str) 0, 0, 0, 0, U_CONSTANT_SIZE(c_str), 0, c_str
#elif defined(U_SUBSTR_INC_REF)
#  define U_STRINGREP_FROM_CONSTANT(c_str)    0,    0, U_CONSTANT_SIZE(c_str), 0, c_str
#else
#  define U_STRINGREP_FROM_CONSTANT(c_str)          0, U_CONSTANT_SIZE(c_str), 0, c_str
#endif

#define U_STRING_FROM_STRINGREP_STORAGE(n) UString(&(stringrep_storage[n]))

class UValue;
class UString;
class UStringExt;
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

   // references has two states:
   // --------------------------
   //   0: one reference
   // n>0: n+1 references
   // --------------------------
   int32_t references;

   bool uniq() const
      {
      U_TRACE(0, "UStringRep::uniq()")

      U_RETURN(references == 0); // NB: 0 -> one reference
      }

   void hold()
      {
      U_TRACE(0, "UStringRep::hold()")

      ++references;

      U_INTERNAL_DUMP("this = %p parent = %p references = %d child = %d", this, parent, references + 1, child)

      U_INTERNAL_ASSERT_MAJOR(references,0)
      }

   void release();
   void destroy();

   // Size and Capacity

   uint32_t size() const   { return _length; }
   uint32_t length() const { return _length; }

   bool empty() const
      {
      U_TRACE(0, "UStringRep::empty()")

      U_RETURN(_length == 0);
      }

   uint32_t capacity() const
      {
      U_TRACE(0, "UStringRep::capacity()")

      U_RETURN(_capacity);
      }

   bool writeable() const
      {
      U_TRACE(0, "UStringRep::writeable()")

      U_RETURN((int32_t)_capacity > 0); // mode: -1 -> mmap
                                        //        0 -> const
      }

   bool mmap() const
      {
      U_TRACE(0, "UStringRep::mmap()")

      U_RETURN(_capacity == U_NOT_FOUND); 
      }

   uint32_t space() const
      {
      U_TRACE(0, "UStringRep::space()")

      U_INTERNAL_ASSERT_MAJOR((int32_t)_capacity,0) // writeable()

      U_RETURN(_capacity - _length);
      }

   uint32_t remain(const char* ptr) const
      {
      U_TRACE(0, "UStringRep::remain(%p)", ptr)

      U_INTERNAL_ASSERT((str + ((int32_t)_capacity > 0 ? _capacity : _length)) >= ptr)

      U_RETURN((str + _length) - ptr);
      }

   uint32_t distance(const char* ptr) const
      {
      U_TRACE(0, "UStringRep::distance(%p)", ptr)

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

   uint32_t copy(char* s, uint32_t n, uint32_t pos = 0) const
      {
      U_TRACE(1, "UStringRep::copy(%p,%u,%u)", s, n, pos)

      U_INTERNAL_ASSERT(pos <= _length)

      if (n > (_length - pos)) n = (_length - pos);

      (void) U_SYSCALL(memcpy, "%p,%p,%u", s, str + pos, n);

      U_RETURN(n);
      }

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
      U_TRACE(0, "UStringRep::at(%u) const", pos)

      U_INTERNAL_ASSERT_MINOR(pos, _length)

      return str[pos];
      }

   char first_char() const
      {
      U_TRACE(0, "UStringRep::first_char()")

      U_INTERNAL_ASSERT_MAJOR(_length,0)

      return str[0];
      }

   char last_char() const
      {
      U_TRACE(0, "UStringRep::last_char()")

      U_INTERNAL_ASSERT_MAJOR(_length,0)

      return str[_length - 1];
      }

   char operator[](uint32_t pos) const { return str[pos]; }

   // Compare

   int compare(const char* s, uint32_t n) const
      {
      U_TRACE(0, "UStringRep::compare(%.*S,%u)", n, s, n)

      int r = memcmp(str, s, U_min(_length, n));

      U_INTERNAL_DUMP("str = %.*S", U_min(_length, n), str)

      if (r == 0) r = (_length - n);

      U_RETURN(r);
      }

   int compare(const UStringRep* rep) const { return compare(rep->data(), rep->size()); }

   int compare(const UStringRep* rep, uint32_t depth) const
      {
      U_TRACE(0, "UStringRep::compare(%.*S,%u)", U_min(_length, rep->_length) - depth, rep->str+depth, depth)

      U_INTERNAL_ASSERT(depth <=      _length)
      U_INTERNAL_ASSERT(depth <= rep->_length)

      int r = memcmp(str + depth, rep->str + depth, U_min(_length, rep->_length) - depth);

      U_INTERNAL_DUMP("str[%u] = %.*S", depth, U_min(_length, rep->_length) - depth, str + depth)

      if (r == 0) r = (_length - rep->_length);

      U_RETURN(r);
      }

   int compare(uint32_t pos, uint32_t n1, const char* s, uint32_t n2) const
      {
      U_TRACE(0, "UStringRep::compare(%u,%u,%.*S,%u)", pos, n1, n2, s, n2)

      U_INTERNAL_ASSERT((pos + n1) <= _length)

      int r = memcmp(str + pos, s, U_min(n1, n2));

      if (r == 0) r = (n1 - n2);

      U_RETURN(r);
      }

   // Compare with ignore case

   int comparenocase(const char* s, uint32_t n) const
      {
      U_TRACE(0, "UStringRep::comparenocase(%.*S,%u)", n, s, n)

      int r = strncasecmp(str, s, U_min(_length, n));

      U_INTERNAL_DUMP("str = %.*S", U_min(_length, n), str)

      if (r == 0) r = (_length - n);

      U_RETURN(r);
      }

   int comparenocase(const UStringRep* rep) const { return comparenocase(rep->data(), rep->size()); }

   // Equal

   bool equal(const char* s, uint32_t n) const
      {
      U_TRACE(0, "UStringRep::equal(%.*S,%u)", n, s, n)

      U_INTERNAL_ASSERT_POINTER(s)

      bool r = (_length == n) && (memcmp(str, s, n) == 0);

      U_RETURN(r);
      }

   bool equal(const UStringRep* rep) const { return equal(rep->data(), rep->size()); }

   // Equal with ignore case

   bool equalnocase(const char* s, uint32_t n) const
      {
      U_TRACE(0, "UStringRep::equalnocase(%.*S,%u)", n, s, n)

      U_INTERNAL_ASSERT_POINTER(s)

      bool r = (_length == n) && (strncasecmp(str, s, n) == 0);

      U_RETURN(r);
      }

   bool equalnocase(const UStringRep* rep) const { return equalnocase(rep->data(), rep->size()); }

   bool equal(const UStringRep* rep, bool ignore_case) const
      {
      U_TRACE(0, "UStringRep::equal(%p,%b)", rep, ignore_case)

      uint32_t n    = rep->size();
      const char* s = rep->data();

      if (_length == n)
         {
         bool r = ((ignore_case ? strncasecmp(str, s, n)
                                :      memcmp(str, s, n)) == 0);

         U_RETURN(r);
         }

      U_RETURN(false);
      }

   // Creation

   static UStringRep* strdup(const char* t, uint32_t tlen)
      {
      U_TRACE(0, "UStringRep::strdup(%p,%u)", t, tlen)

      U_INTERNAL_ASSERT_POINTER(t)

      UStringRep* r = create(tlen, tlen, t);

      U_RETURN_POINTER(r, UStringRep);
      }

   static UStringRep* create(uint32_t length, uint32_t capacity, const char* ptr);

   static UStringRep* create(const char* t, uint32_t tlen, uint32_t mode); // mode:  0 -> const
                                                                           //       -1 -> mmap

   // Substring

   UStringRep* substr(const char* t, uint32_t tlen);

   UStringRep* substr(uint32_t pos, uint32_t n) { return substr(str + pos, n); }

   // Assignement

   static void assign(UStringRep*& rep, const char* s, uint32_t n);

   void size_adjust(      uint32_t value = U_NOT_FOUND);
   void size_adjust_force(uint32_t value = U_NOT_FOUND)
      {
      U_TRACE(0, "UStringRep::size_adjust_force(%u)", value)

      _length = (value == U_NOT_FOUND ? u_strlen(str) : value);

      U_INTERNAL_ASSERT(invariant())
      }

#ifdef DEBUG
   bool invariant() const;
   const char* dump(bool reset) const;

   static int32_t max_child;
   static UStringRep* parent_destroy;
   static UStringRep* string_rep_share;
   static bool check_dead_of_source_string_with_child_alive;

   static void errorReferences(                          UStringRep* ptr_object);
   static bool checkIfChild(     const char* name_class, const void* ptr_object);
   static bool checkIfReferences(const char* name_class, const void* ptr_object);
#endif

   // EXTENSION

   bool isText(uint32_t pos) const;
   bool isUTF8(uint32_t pos) const;
   bool isUTF16(uint32_t pos) const;
   bool isBase64(uint32_t pos) const;
   bool isBinary(uint32_t pos) const;
   bool isWhiteSpace(uint32_t pos) const;

   // UTF8 <--> ISO Latin 1

   static UStringRep*   toUTF8(const unsigned char* t, uint32_t tlen);
   static UStringRep* fromUTF8(const unsigned char* t, uint32_t tlen);

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

      uint32_t result = u_hash((unsigned char*)str, _length, ignore_case);

      U_RETURN(result);
      }

   // if the string is quoted...

   bool isQuoted(char c) const
      {
      U_TRACE(0, "UStringRep::isQuoted(%C)", c)

      bool result = (str[0]         == c &&
                     str[_length-1] == c);

      U_RETURN(result);
      }

   // ...unquote it

   void unQuote()
      {
      U_TRACE(0, "UStringRep::unQuote()")

      U_INTERNAL_ASSERT_MAJOR(_length,2)

      ++str;
      _length -= 2;
      }

   void avoidPunctuation();

   // STREAM

   void write(ostream& os) const;

   friend ostream& operator<<(ostream& os, const UStringRep& r) { r.write(os); return os; }

protected:
   uint32_t _length, _capacity;
   const char* str;
   // ----------------> maybe unnamed array of char...

   // The following storage is init'd to 0 by the linker, resulting (carefully) in an empty string with zero(=one) reference...
   static UStringRep* string_rep_null;

private:
    UStringRep() {}
   ~UStringRep() { U_TRACE_UNREGISTER_OBJECT(0, UStringRep) }

   explicit UStringRep(const char* t);
   explicit UStringRep(const char* t, uint32_t tlen);

   UStringRep(const UStringRep&)            {}
   UStringRep& operator=(const UStringRep&) { return *this; }

private:
#if defined(U_SUBSTR_INC_REF) || defined(DEBUG)
   inline void checkIfMReserve() U_NO_EXPORT;
#endif
   inline void set(uint32_t length, uint32_t capacity, const char* ptr) U_NO_EXPORT;

                      friend class UString;
                      friend class UStringExt;
   template <class T> friend class UHashMap;
};

class U_EXPORT UString {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // null string (for container etc...)
   static UString& getStringNull() { return *string_null; }

protected:
   static UString* string_null;

   friend class UValue;
   friend class UStringExt;

public:
   // mutable
   UStringRep* rep;

   // SERVICES

   char* data() const { return rep->data(); }
   bool empty() const { return rep->empty(); }

   uint32_t size() const      { return rep->size(); }
   uint32_t space() const     { return rep->space(); }
   uint32_t length() const    { return rep->length(); }
   uint32_t capacity() const  { return rep->capacity(); }

   static uint32_t max_size() { return UStringRep::max_size(); }

protected:
   // in 'memory reference' si distingue tra set, copy, e assign...

   void set(UStringRep* r)
      {
      U_TRACE(0, "UString::set(%p)", r)

      U_INTERNAL_ASSERT_DIFFERS(rep,r)

      U_CHECK_MEMORY

      rep->release();

      rep = r;
      }

   void copy(UStringRep* r)
      {
      U_TRACE(0, "UString::copy(%p)", r)

      U_CHECK_MEMORY

      rep = r;

      rep->hold();
      }

public:
   void assign(UStringRep* r)
      {
      U_TRACE(0, "UString::assign(%p)", r)

      U_CHECK_MEMORY

      // NB: funziona anche nel caso di (rep == r)...

      r->hold();        // 1. take a copy of new resource

      rep->release();   // 2. release existing resource

      rep = r;          // 3. bind copy to self
      }

   // COSTRUTTORI

   UString()
      {
      U_TRACE_REGISTER_OBJECT(0, UString, "", 0)

      copy(UStringRep::string_rep_null);
      }

   explicit UString(UStringRep* r)
      {
      U_TRACE_REGISTER_OBJECT(0, UString, "%p", r)

      copy(r);
      }

   explicit UString(ustringrep* r)
      {
      U_TRACE_REGISTER_OBJECT(0, UString, "%p", r)

      uustringrep u = { r };

      copy(u.p2);
      }

   UString(const UString& str)
      {
      U_TRACE_REGISTER_OBJECT(0, UString, "%p", &str)

      U_MEMORY_TEST_COPY(str)

      copy(str.rep);
      }

   explicit UString(const char* t);
   explicit UString(const char* t, uint32_t tlen);

   explicit UString(uint32_t n);
   explicit UString(uint32_t n, unsigned char c)
      {
      U_TRACE_REGISTER_OBJECT(0, UString, "%u,%C", n, c)

      rep = UStringRep::create(n, n, 0);

      (void) U_SYSCALL(memset, "%p,%d,%u", rep->data(), c, n);
      }

   // Copy from string

   explicit UString(void* t)
      {
      U_TRACE_REGISTER_OBJECT(0, UString, "%S", (char*)t)

      U_INTERNAL_ASSERT_POINTER(t)

      uint32_t tlen = u_strlen((char*)t);

      rep = UStringRep::create(tlen, tlen, (const char*)t);
      }

   explicit UString(void* t, uint32_t tlen)
      {
      U_TRACE_REGISTER_OBJECT(0, UString, "%.*S,%u", tlen, (char*)t, tlen)

      U_INTERNAL_ASSERT_POINTER(t)

      rep = UStringRep::create(tlen, tlen, (const char*)t);
      }

   explicit UString(const UString& str, uint32_t pos, uint32_t n = U_NOT_FOUND)
      {
      U_TRACE_REGISTER_OBJECT(0, UString, "%p,%u,%u", &str, pos, n)

      U_INTERNAL_ASSERT(pos <= str.size())

      uint32_t sz = str.rep->fold(pos, n);

      if (sz) rep = UStringRep::create(sz, sz, str.rep->str + pos);
      else    copy(UStringRep::string_rep_null);
      }

   // Substring

   explicit UString(UStringRep* _rep, const char* t, uint32_t tlen)
      {
      U_TRACE_REGISTER_OBJECT(0, UString, "%p,%p,%u", _rep, t, tlen)

      rep = _rep->substr(t, tlen);
      }

   explicit UString(UStringRep* _rep, uint32_t pos, uint32_t n = U_NOT_FOUND)
      {
      U_TRACE_REGISTER_OBJECT(0, UString, "%p,%u,%u", _rep, pos, n)

      rep = _rep->substr(pos, _rep->fold(pos, n));
      }

   UString substr(const char* t, uint32_t tlen) const;

   UString substr(uint32_t pos, uint32_t n = U_NOT_FOUND) const;

   // distruttore

   ~UString()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UString)

      rep->release();
      }

   // Replace

   UString& replace(uint32_t pos, uint32_t n1, uint32_t n2, char c);
   UString& replace(uint32_t pos, uint32_t n1, const char* s, uint32_t n2);

   UString& replace(uint32_t pos1, uint32_t n1, const UString& str, uint32_t pos2, uint32_t n2 = U_NOT_FOUND)
      {
      U_TRACE(0, "UString::replace(%u,%u,%p,%u,%u)", pos1, n1, &str, pos2, n2)

      U_INTERNAL_ASSERT(pos2 <= str.size())

      return replace(pos1, n1, str.data() + pos2, str.rep->fold(pos2, n2));
      }

   UString& replace(char c)                                       { return replace(0, size(),          1, c); }
   UString& replace(const char* s)                                { return replace(0, size(),          s, u_strlen(s)); }
   UString& replace(const UString& str)                           { return replace(0, size(), str.data(), str.size()); }
   UString& replace(const char* s, uint32_t n)                    { return replace(0, size(),          s, n); }
   UString& replace(uint32_t pos, uint32_t n, const char* s)      { return replace(pos,    n,          s, u_strlen(s)); }
   UString& replace(uint32_t pos, uint32_t n, const UString& str) { return replace(pos,    n, str.data(), str.size()); }

   // Assignement
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
   UString& assign(uint32_t n, char c) { return replace(0, size(), n, c); }
   UString& assign(const UString& str) { return assign(str.data(), str.size()); }

   UString& operator=(char c)          { return assign(1, c); }
   UString& operator=(const char* s)   { return assign(s); }

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

   bool reserve(uint32_t n); // NB: return true if changed rep...

   // Element access

   const char* begin()  const { return rep->begin(); }
   const char* end()    const { return rep->end(); }
   const char* rbegin() const { return rep->rbegin(); }
   const char* rend()   const { return rep->rend(); }

   char at(uint32_t pos) const         { return rep->at(pos); }
   char operator[](uint32_t pos) const { return rep->operator[](pos); }

   char* _begin()
      {
      U_TRACE(0, "UString::_begin()")

      if (rep->uniq() == false) duplicate();

      return rep->begin();
      }

   char* _end()
      {
      U_TRACE(0, "UString::_end()")

      if (rep->uniq() == false) duplicate();

      return rep->end();
      }

   char* _rbegin()
      {
      U_TRACE(0, "UString::_rbegin()")

      if (rep->uniq() == false) duplicate();

      return rep->rbegin();
      }

   char* _rend()
      {
      U_TRACE(0, "UString::_rend()")

      if (rep->uniq() == false) duplicate();

      return rep->rend();
      }

   // Modifiers

   void push_back(char c) { (void) append(uint32_t(1), c); }

   UString& append(uint32_t n, char c);
   UString& append(const char* s, uint32_t n);

   UString& append(const UString& str, uint32_t pos, uint32_t n = U_NOT_FOUND)
      {
      U_TRACE(0, "UString::append(%p,%u,%u)", &str, pos, n)

      U_INTERNAL_ASSERT(pos <= str.size())

      return append(str.data() + pos, str.rep->fold(pos, n));
      }

   UString& append(const char* s);
   UString& append(const UString& str)       { return append(str.data(), str.size()); }

   UString& operator+=(char c)               { return append(uint32_t(1), c); }
   UString& operator+=(const char* s)        { return append(s, u_strlen(s)); }
   UString& operator+=(const UString& str);

   // OPTMIZE APPEND (BUFFERED)

   static char* ptrbuf;
   static char  appbuf[1024];

   void append(unsigned char c)
      {
      U_TRACE(0, "UString::append(%C)", c)

      U_INTERNAL_ASSERT_RANGE(appbuf,ptrbuf,appbuf+sizeof(appbuf))

      if ((ptrbuf - appbuf) == sizeof(appbuf))
         {
         (void) append(appbuf, sizeof(appbuf));

         ptrbuf = appbuf;
         }

      *ptrbuf++ = c;
      }

   void append()
      {
      U_TRACE(0, "UString::append()")

      if (ptrbuf > appbuf)
         {
         (void) append(appbuf, ptrbuf - appbuf);

         ptrbuf = appbuf;
         }
      }

   // operator +

   friend UString operator+(const UString& lhs, char rhs);
   friend UString operator+(const UString& lhs, const char* rhs);
   friend UString operator+(const UString& lhs, const UString& rhs);
   friend UString operator+(char lhs,           const UString& rhs);
   friend UString operator+(const char* lhs,    const UString& rhs);

   UString& insert(uint32_t pos, const UString& str)
      { return replace(pos, 0, str.data(), str.size()); }

   UString& insert(uint32_t pos1, const UString& str, uint32_t pos2, uint32_t n = U_NOT_FOUND)
      {
      U_TRACE(0, "UString::insert(%u,%p,%u,%u)", pos1, &str, pos2, n)

      U_INTERNAL_ASSERT(pos2 <= str.size())

      return replace(pos1, 0, str.data() + pos2, str.rep->fold(pos2, n));
      }

   UString& insert(uint32_t pos, const char* s)             { return replace(pos, 0, s, u_strlen(s)); }
   UString& insert(uint32_t pos, const char* s, uint32_t n) { return replace(pos, 0, s, n); }
   UString& insert(uint32_t pos, uint32_t n, char c)        { return replace(pos, 0, n, c); }

   void clear();

   void resize(uint32_t n, char c = '\0');

   UString& erase(uint32_t pos = 0, uint32_t n = U_NOT_FOUND) { return replace(pos, rep->fold(pos, n), "", 0); }

   // C-Style String

   void setNullTerminated() const;

   const char* c_str() const
      {
      U_TRACE(0, "UString::c_str()")

      if (isNullTerminated() == false) setNullTerminated();
      
      U_RETURN(rep->str);
      }

   char* c_strdup() const                                            { return strndup(rep->data(), rep->size()); }
   char* c_strndup(uint32_t pos = 0, uint32_t n = U_NOT_FOUND) const { return strndup(rep->data() + pos, rep->fold(pos, n)); }

   UString  copy();
   uint32_t copy(char* s, uint32_t n, uint32_t pos = 0) const { return rep->copy(s, n, pos); }

   // STRING OPERATIONS

   // The `find' function searches string for a specified string (possibly a single character) and returns
   // its starting position. You can supply the parameter pos to specify the position where search must begin

   uint32_t find(const char* s, uint32_t pos, uint32_t s_len, uint32_t how_much = U_NOT_FOUND) const;

   uint32_t find(const UString& str, uint32_t pos = 0, uint32_t how_much = U_NOT_FOUND) const
      { return find(str.data(), pos, str.size(), how_much); }

   uint32_t find(char c,        uint32_t pos = 0) const;
   uint32_t find(const char* s, uint32_t pos = 0) const { return find(s, pos, u_strlen(s), U_NOT_FOUND); }

   // The `rfind' function searches from end to beginning string for a specified string (possibly a single
   // character) and returns its starting position. You can supply the parameter pos to specify the position
   // where search must begin

   uint32_t rfind(const char* s,      uint32_t pos, uint32_t n) const;
   uint32_t rfind(char c,             uint32_t pos = U_NOT_FOUND) const;
   uint32_t rfind(const char* s,      uint32_t pos = U_NOT_FOUND) const { return rfind(s, pos, u_strlen(s)); }
   uint32_t rfind(const UString& str, uint32_t pos = U_NOT_FOUND) const { return rfind(str.data(), pos, str.size()); }

   // The `find_first_of' function searches string for the first match of any character stored in s and returns its position
   // The `find_last_of'  function searches string for the last  match of any character stored in s and returns its position

   uint32_t find_first_of(const char* s,      uint32_t pos, uint32_t n) const;
   uint32_t find_first_of(char c,             uint32_t pos = 0) const { return find(c, pos); }
   uint32_t find_first_of(const char* s,      uint32_t pos = 0) const { return find_first_of(s, pos, u_strlen(s)); }
   uint32_t find_first_of(const UString& str, uint32_t pos = 0) const { return find_first_of(str.data(), pos, str.size()); }

   uint32_t find_last_of(const char* s,       uint32_t pos, uint32_t n) const;
   uint32_t find_last_of(char c,              uint32_t pos = U_NOT_FOUND) const { return rfind(c, pos); }
   uint32_t find_last_of(const char* s,       uint32_t pos = U_NOT_FOUND) const { return find_last_of(s, pos, u_strlen(s)); }
   uint32_t find_last_of(const UString& str,  uint32_t pos = U_NOT_FOUND) const { return find_last_of(str.data(), pos, str.size()); }

   // The `find_first_not_of' function searches the first element of string that doesn't match any character stored in s
   // and returns its position
   // The `find_last_not_of'  function searches the last  element of string that doesn't match any character stored in s
   // and returns its position

   uint32_t find_first_not_of(const char* s,      uint32_t pos, uint32_t n) const;
   uint32_t find_first_not_of(char c,             uint32_t pos = 0) const;
   uint32_t find_first_not_of(const char* s,      uint32_t pos = 0) const { return find_first_not_of(s, pos, u_strlen(s)); }
   uint32_t find_first_not_of(const UString& str, uint32_t pos = 0) const { return find_first_not_of(str.data(), pos, str.size()); }

   uint32_t find_last_not_of(const char* s,      uint32_t pos, uint32_t n) const;
   uint32_t find_last_not_of(char c,             uint32_t pos = U_NOT_FOUND) const;
   uint32_t find_last_not_of(const char* s,      uint32_t pos = U_NOT_FOUND) const { return find_last_not_of(s, pos, u_strlen(s)); }
   uint32_t find_last_not_of(const UString& str, uint32_t pos = U_NOT_FOUND) const { return find_last_not_of(str.data(), pos, str.size()); }

   // Find with ignore case

   uint32_t findnocase(const char* s, uint32_t pos, uint32_t s_len, uint32_t how_much = U_NOT_FOUND) const;

   uint32_t findnocase(const UString& str, uint32_t pos = 0, uint32_t how_much = U_NOT_FOUND) const
      { return findnocase(str.data(), pos, str.size(), how_much); }

   uint32_t findnocase(const char* s, uint32_t pos = 0) const { return findnocase(s, pos, u_strlen(s), U_NOT_FOUND); }

   // Compare

   int compare(UStringRep* _rep) const          { return rep->compare(_rep); }
   int compare(const char* s, uint32_t n) const { return rep->compare(s, n); }
   int compare(const UString& str) const        { return rep->compare(str.rep); }

   int compare(uint32_t pos, uint32_t n1, const char* s, uint32_t n2) const
      { return rep->compare(pos, U_min(size() - pos, n1), s, U_min(u_strlen(s), n2)); }

   int compare(const char* s) const { return rep->compare(s, u_strlen(s)); }

   int compare(uint32_t pos, uint32_t n, const char* s) const
      { return rep->compare(pos, U_min(size() - pos, n), s, u_strlen(s)); }

   int compare(uint32_t pos, uint32_t n, const UString& str) const
      { return rep->compare(pos, U_min(size() - pos, n), str.data(), str.size()); }

   int compare(uint32_t pos1, uint32_t n1, const UString& str, uint32_t pos2, uint32_t n2) const
      { return rep->compare(pos1, U_min(size() - pos1, n1), str.data() + pos2,
                                  U_min(str.size() - pos2, n2)); }

   // Compare with ignore case

   int comparenocase(const char* s, uint32_t n) const     { return rep->comparenocase(s, n); }
   int comparenocase(const char* s) const                 { return rep->comparenocase(s, u_strlen(s)); }

   int comparenocase(UStringRep* _rep) const              { return rep->comparenocase(_rep); }
   int comparenocase(const UString& str) const            { return rep->comparenocase(str.rep); }

   // Same string representation

   bool same(UStringRep* _rep) const                      { return (rep == _rep); }
   bool same(const UString& str) const                    { return same(str.rep); }

   // Equal

   bool equal(const char* s) const;
   bool equal(const char* s, uint32_t n) const;

   bool equal(UStringRep* _rep) const;
   bool equal(const UString& str) const                   { return equal(str.rep); }

   // Equal with ignore case

   bool equalnocase(const char* s, uint32_t n) const      { return rep->equalnocase(s, n); }
   bool equalnocase(const char* s) const                  { return rep->equalnocase(s, u_strlen(s)); }

   bool equalnocase(UStringRep* _rep) const               { return same(_rep) || rep->equalnocase(_rep); }
   bool equalnocase(const UString& str) const             { return equalnocase(str.rep); }

   bool equal(UStringRep* _rep, bool ignore_case) const   { return same(_rep) || rep->equal(_rep, ignore_case); }
   bool equal(const UString& str, bool ignore_case) const { return equal(str.rep, ignore_case); }

   // STREAM

   istream& getline(istream& is, char delim = '\n');

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

#ifdef DEBUG
   bool invariant() const;
   const char* dump(bool reset) const;
#endif

   // -----------------------------------------------------------------------------------------------------------------------
   // START EXTENSION TO STDLIBC++
   // -----------------------------------------------------------------------------------------------------------------------

   bool isNull() const                       { return (rep == UStringRep::string_rep_null); }
   bool isMmap() const                       { return rep->mmap(); }
   bool isNullTerminated() const             { return (rep->str[rep->_length] == '\0'); }
   bool isText(uint32_t pos = 0) const       { return rep->isText(pos); }
   bool isUTF8(uint32_t pos = 0) const       { return rep->isUTF8(pos); }
   bool isUTF16(uint32_t pos = 0) const      { return rep->isUTF16(pos); }
   bool isBinary(uint32_t pos = 0) const     { return rep->isBinary(pos); }
   bool isBase64(uint32_t pos = 0) const     { return rep->isBase64(pos); }
   bool isWhiteSpace(uint32_t pos = 0) const { return rep->isWhiteSpace(pos); }

   char  last_char() const             { return rep->last_char(); }
   char  first_char() const            { return rep->first_char(); }
   char  c_char(uint32_t pos) const    { return rep->at(pos); }
   char* c_pointer(uint32_t pos) const { return (rep->data() + pos); }

   uint32_t remain(const char* ptr) const    { return rep->remain(ptr); }
   uint32_t distance(const char* ptr) const  { return rep->distance(ptr); }

   void setFromInode(uint64_t* p)  { (void) replace((const char*)p, sizeof(uint64_t)); }

   uint32_t hash(bool ignore_case = false) const { return rep->hash(ignore_case); }

   // references

   void hold()    const { rep->hold(); }
   void release() const { rep->release(); }

   // if the string is quoted...

   bool isQuoted(char c = '"') const { return rep->isQuoted(c); }

   // ...unquote it

   void unQuote()
      {
      U_TRACE(0, "UString::unQuote()")

      if (rep->_length > 2) rep->unQuote();
      else                  clear();
      }

   void avoidPunctuation() { return rep->avoidPunctuation(); }

   // set uniq

   void duplicate(uint32_t space = 0) const;

   bool uniq() const      { return rep->uniq(); }
   bool writeable() const { return rep->writeable(); }

   // (manage UString as memory mapped area...)

   void mmap(char* map, uint32_t len)
      {
      U_TRACE(0, "UString::mmap(%.*S,%u)", len, map, len)

      U_INTERNAL_ASSERT(map != MAP_FAILED)

      set(UStringRep::create(map, len, U_NOT_FOUND));

      U_INTERNAL_ASSERT(invariant())
      }

   // (manage UString as buffer...)

   void size_adjust(      uint32_t value = U_NOT_FOUND) { rep->size_adjust(value); }
   void size_adjust_force(uint32_t value = U_NOT_FOUND) { rep->size_adjust_force(value); }

   void setEmpty()      { rep->size_adjust(0); }
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

#ifdef __MINGW32__
#undef  snprintf
#undef vsnprintf
#endif

   void snprintf(    const char* format, ...);
   void snprintf_add(const char* format, ...);

   void vsnprintf(const char* format, va_list argp)
      {
      U_TRACE(0, "UString::vsnprintf(%S)", format)

      U_INTERNAL_ASSERT(isNull() == false)
      U_INTERNAL_ASSERT_MAJOR((int32_t)rep->_capacity,0) // writeable()
      U_INTERNAL_ASSERT_MAJOR(rep->_capacity,u_strlen(format))

      rep->_length = u_vsnprintf(rep->data(), rep->capacity(), format, argp); 

      U_INTERNAL_ASSERT(invariant())
      }

   void vsnprintf_add(const char* format, va_list argp)
      {
      U_TRACE(0, "UString::vsnprintf_add(%S)", format)

      U_INTERNAL_ASSERT(isNull() == false)
      U_INTERNAL_ASSERT(rep->references == 0)

      rep->_length += u_vsnprintf(c_pointer(rep->_length), rep->space(), format, argp); 

      U_INTERNAL_ASSERT(invariant())
      }

#ifdef HAVE_STRTOF
   float       strtof() const          { return rep->strtof(); }
#endif
   double      strtod() const          { return rep->strtod(); }
#ifdef HAVE_STRTOLD
   long double strtold() const         { return rep->strtold(); }
#endif
   long     strtol(int base = 0) const { return rep->strtol(base); }
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
