// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    string.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file.h>

#include <errno.h>

static ustringrep empty_string_rep_storage = {
#ifdef DEBUG
   (void*)U_CHECK_MEMORY_SENTINEL, // memory_error (_this)
#endif
#if defined(U_SUBSTR_INC_REF) || defined(DEBUG)
   0, // parent - substring increment reference of source string
#  ifdef DEBUG
   0, // child  - substring capture event 'DEAD OF SOURCE STRING WITH CHILD ALIVE'...
#  endif
#endif
   0, // _length
   0, // _capacity
   0, // references
  ""  // str - NB: we need an address (see c_str() or isNullTerminated()) and must be null terminated...
};

static uustringrep uustringrepnull      = { &empty_string_rep_storage };
UStringRep* UStringRep::string_rep_null = uustringrepnull.p2;
struct ustring                            { ustringrep* rep; };
static ustring empty_string_storage     = { &empty_string_rep_storage };

union uustring {
   ustring* p1;
   UString* p2;
};

static uustring uustringnull  = { &empty_string_storage };
UString* UString::string_null = uustringnull.p2;

U_NO_EXPORT void UStringRep::set(uint32_t __length, uint32_t __capacity, const char* ptr)
{
   U_TRACE(0, "UStringRep::set(%u,%u,%p)", __length, __capacity, ptr)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(ptr)

#if defined(U_SUBSTR_INC_REF) || defined(DEBUG)
   parent = 0;
#  ifdef DEBUG
   child  = 0;
#  endif
#endif

   _length    = __length;
   _capacity  = __capacity; // [0 const | -1 mmap]...
   references = 0;
   str        = ptr;
}

UStringRep::UStringRep(const char* t, uint32_t tlen)
{
   U_TRACE_REGISTER_OBJECT(0, UStringRep, "%.*S,%u", tlen, t, tlen)

   U_INTERNAL_ASSERT_POINTER(t)
   U_INTERNAL_ASSERT_MAJOR(tlen, 0)

   set(tlen, 0U, t);
}

// NB: ctor is private...

UStringRep::~UStringRep()
{
   U_TRACE(0, "UStringRep::~UStringRep()")

   // NB: we don't use delete (dtor) because it add a deallocation to the destroy process...

   U_ERROR("I can't use UStringRep on stack!");
}

UStringRep* UStringRep::create(uint32_t length, uint32_t capacity, const char* ptr)
{
   U_TRACE(1, "UStringRep::create(%u,%u,%p)", length, capacity, ptr)

   U_INTERNAL_ASSERT_MAJOR(capacity, 0)

   char* _ptr;
   UStringRep* r;

   // NB: we don't use new (ctor) because we want an allocation with more space for string data...

#if !defined(ENABLE_MEMPOOL) || !defined(__linux__)
      r = (UStringRep*) U_SYSCALL(malloc, "%u", capacity + (1 + sizeof(UStringRep)));
   _ptr = (char*)(r + 1);
#else
   if (capacity <= U_CAPACITY)
      {
      if (capacity == U_CAPACITY) r = (UStringRep*) UMemoryPool::pop(9);
      else
         {
         int stack_index;

         // NB: we need an array of char[_capacity],
         //     plus a terminating null char element,
         //     plus enough for the UStringRep data structure...

         uint32_t sz = capacity + (1 + sizeof(UStringRep));

         if (sz <= U_STACK_TYPE_4)      // 128
            {
            capacity    = U_STACK_TYPE_4 - (1 + sizeof(UStringRep));
            stack_index = 4;
            }
         else if (sz <= U_STACK_TYPE_5) // 256
            {
            capacity    = U_STACK_TYPE_5 - (1 + sizeof(UStringRep));
            stack_index = 5;
            }
         else if (sz <= U_STACK_TYPE_6) // 512
            {
            capacity    = U_STACK_TYPE_6 - (1 + sizeof(UStringRep));
            stack_index = 6;
            }
         else if (sz <= U_STACK_TYPE_7) // 1024
            {
            capacity    = U_STACK_TYPE_7 - (1 + sizeof(UStringRep));
            stack_index = 7;
            }
         else if (sz <= U_STACK_TYPE_8) // 2048
            {
            capacity    = U_STACK_TYPE_8 - (1 + sizeof(UStringRep));
            stack_index = 8;
            }
         else // if (sz <= U_STACK_TYPE_9) // 4096
            {
            capacity    = U_STACK_TYPE_9 - (1 + sizeof(UStringRep));
            stack_index = 9;
            }

         U_INTERNAL_DUMP("sz = %u capacity = %u stack_index = %u", sz, capacity, stack_index)

         r = (UStringRep*) UMemoryPool::pop(stack_index);
         }

      _ptr = (char*)(r + 1);
      }
   else
      {
      _ptr = UFile::mmap(&capacity, -1, PROT_READ | PROT_WRITE, U_MAP_ANON, 0);

      if (_ptr == MAP_FAILED) U_RETURN_POINTER(string_rep_null, UStringRep);

      r = U_MALLOC_TYPE(UStringRep);
      }
#endif

#ifdef DEBUG
   U_SET_LOCATION_INFO;
   U_REGISTER_OBJECT_PTR(0,UStringRep,r)
   r->memory._this = (void*)U_CHECK_MEMORY_SENTINEL;
#endif

   r->set(length, capacity, _ptr);

   if (length && ptr)
      {
      U__MEMCPY((void*)_ptr, ptr, length);

      _ptr[length] = '\0';
      }

   U_INTERNAL_ASSERT(r->invariant())

   U_RETURN_POINTER(r, UStringRep);
}

void UStringRep::release()
{
   U_TRACE(0, "UStringRep::release()")

   U_INTERNAL_DUMP("this = %p parent = %p references = %u child = %d", this, parent, references + 1, child)

   U_CHECK_MEMORY

   if (references)
      {
      --references;

      return;
      }

   // NB: we don't use delete (dtor) because add a deallocation to the destroy process...

   U_INTERNAL_DUMP("_capacity = %u str(%u) = %.*S", _capacity, _length, _length, str)

   U_INTERNAL_ASSERT_EQUALS(references, 0)
   U_INTERNAL_ASSERT_DIFFERS(this, string_rep_null)

#if defined(U_SUBSTR_INC_REF) || defined(DEBUG)
   if (parent)
#  ifdef U_SUBSTR_INC_REF
      parent->release(); // NB: solo la morte della substring de-referenzia la source...
#  else
      {
      U_INTERNAL_ASSERT_EQUALS(child,0)

      U_INTERNAL_DUMP("parent->child = %d", parent->child)

      U_INTERNAL_ASSERT_RANGE(1,parent->child,max_child)

      parent->child--;
      }
   else // source...
      {
      if (child)
         {
         if (UObjectDB::fd > 0)
            {
            parent_destroy = this;

            U_DUMP_OBJECT("DEAD OF SOURCE STRING WITH CHILD ALIVE - child of this", checkIfChild)
            }
         else
            {
            char buffer[4096];

            (void) u__snprintf(buffer, sizeof(buffer), "DEAD OF SOURCE STRING WITH CHILD ALIVE: child(%u) source(%u) = %.*S", child, _length, _length, str);

            if (check_dead_of_source_string_with_child_alive)
               {
               U_INTERNAL_ASSERT_MSG(false, buffer)
               }
            else
               {
               U_WARNING("%s", buffer);
               }
            }
         }
      }
#  endif
#  ifdef DEBUG
   U_UNREGISTER_OBJECT(0, this)
#  endif
#endif

#if !defined(ENABLE_MEMPOOL) || !defined(__linux__)
   U_SYSCALL_VOID(free, "%p", (void*)this);
#else
   if (_capacity <= U_CAPACITY)
      {
           if (_capacity == U_CAPACITY) UMemoryPool::push(this, 9);
      else if (_capacity == 0)          UMemoryPool::push(this, U_SIZE_TO_STACK_INDEX(sizeof(UStringRep))); // NB: no room for data, constant string...
      else
         {
         int stack_index;

         // NB: we need an array of char[_capacity],
         //     plus a terminating null char element,
         //     plus enough for the UStringRep data structure...

         uint32_t sz = _capacity + (1 + sizeof(UStringRep));

         switch (sz)
            {
            case U_STACK_TYPE_4: stack_index = 4;                                                     break; //  128
            case U_STACK_TYPE_5: stack_index = 5;                                                     break; //  256
            case U_STACK_TYPE_6: stack_index = 6;                                                     break; //  512
            case U_STACK_TYPE_7: stack_index = 7;                                                     break; // 1024
            case U_STACK_TYPE_8: stack_index = 8;                                                     break; // 2048
            default:             stack_index = 9; U_INTERNAL_ASSERT_EQUALS(sz,U_MAX_SIZE_PREALLOCATE) break; // 4096
            }

         U_INTERNAL_DUMP("stack_index = %u", stack_index)

         UMemoryPool::push(this, stack_index);
         }
      }
   else
      {
      if (_capacity != U_NOT_FOUND)
         {
         U_INTERNAL_ASSERT_EQUALS(_capacity & U_PAGEMASK, 0)

         UMemoryPool::deallocate((void*)str, _capacity);
         }
      else
         {
         ptrdiff_t resto = (ptrdiff_t)str % PAGESIZE;

         U_INTERNAL_DUMP("resto = %u _length = %u", resto, _length)

         if (resto)
            {
                str -= resto;
            _length += resto;
            }

         (void) U_SYSCALL(munmap, "%p,%lu", (void*)str, _length);
         }

      U_FREE_TYPE(this, UStringRep); // NB: in debug mode the memory area is zeroed...
      }
#endif
}

#ifdef DEBUG
// substring capture event 'DEAD OF SOURCE STRING WITH CHILD ALIVE'...
int32_t     UStringRep::max_child;
UStringRep* UStringRep::parent_destroy;
UStringRep* UStringRep::string_rep_share;
bool        UStringRep::check_dead_of_source_string_with_child_alive = true;

bool UStringRep::checkIfReferences(const char* name_class, const void* ptr_object)
{
   U_TRACE(0, "UStringRep::checkIfReferences(%S,%p)", name_class, ptr_object)

   if (U_STREQ(name_class, "UString"))
      {
      U_INTERNAL_DUMP("references = %u", ((UString*)ptr_object)->rep->references + 1)

      if (((UString*)ptr_object)->rep == string_rep_share) U_RETURN(true);
      }

   U_RETURN(false);
}

bool UStringRep::checkIfChild(const char* name_class, const void* ptr_object)
{
   U_TRACE(0, "UStringRep::checkIfChild(%S,%p)", name_class, ptr_object)

   if (U_STREQ(name_class, "UStringRep"))
      {
      U_INTERNAL_DUMP("parent = %p", ((UStringRep*)ptr_object)->parent)

      if (((UStringRep*)ptr_object)->parent == parent_destroy) U_RETURN(true);
      }

   U_RETURN(false);
}
#endif

UStringRep* UStringRep::substr(const char* t, uint32_t tlen)
{
   U_TRACE(0, "UStringRep::substr(%.*S,%u)", tlen, t, tlen)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(tlen <= _length)

   UStringRep* r;

   if (tlen == 0)
      {
      r = string_rep_null;

      r->references++;
      }
   else
      {
      U_INTERNAL_ASSERT_RANGE(str,t,end())

      r = U_NEW(UStringRep(t, tlen));

#  if defined(U_SUBSTR_INC_REF) || defined(DEBUG)
      UStringRep* p = this;

      while (p->parent)
         {
         p = p->parent;

         U_INTERNAL_ASSERT(p->invariant())
         }

      r->parent = p;

#     ifdef U_SUBSTR_INC_REF
      p->references++; // substring increment reference of source string
#     else
      p->child++;      // substring capture event 'DEAD OF SOURCE STRING WITH CHILD ALIVE'...

      max_child = U_max(max_child, p->child);
#     endif

      U_INTERNAL_DUMP("r->parent = %p max_child = %d", r->parent, max_child)
#  endif
      }

   U_RETURN_POINTER(r, UStringRep);
}

void UStringRep::assign(UStringRep*& rep, const char* s, uint32_t n)
{
   U_TRACE(0, "UStringRep::assign(%p,%S,%u)", rep, s, n)

   if (rep->references ||
       rep->_capacity < n)
      {
      rep->release();

      rep = U_NEW(UStringRep(s, n));
      }
   else
      {
      char* ptr = (char*)rep->str;

      U_INTERNAL_ASSERT_MAJOR(n,0)
      U_INTERNAL_ASSERT_DIFFERS(ptr,s)

      U__MEMCPY(ptr, s, n);

      ptr[(rep->_length = n)] = '\0';
      }
}

uint32_t UStringRep::copy(char* s, uint32_t n, uint32_t pos) const
{
   U_TRACE(1, "UStringRep::copy(%p,%u,%u)", s, n, pos)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(pos <= _length)

   if (n > (_length - pos)) n = (_length - pos);

   U_INTERNAL_ASSERT_MAJOR(n,0)

   U__MEMCPY(s, str + pos, n);

   U_RETURN(n);
}

void UStringRep::trim()
{
   U_TRACE(0, "UStringRep::trim()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_EQUALS(_capacity,0)

   // skip white space from start

   while (_length && u__isspace(*str))
      {
      ++str;
      --_length;
      }

   U_INTERNAL_DUMP("_length = %u", _length)

   // skip white space from end

   while (_length && u__isspace(str[_length-1])) --_length;
}

__pure int UStringRep::compare(const UStringRep* rep, uint32_t depth) const
{
   U_TRACE(0, "UStringRep::compare(%p,%u)", rep, depth)

   U_CHECK_MEMORY

   int r;
   uint32_t min = U_min(_length, rep->_length);

   U_INTERNAL_DUMP("min = %u", min)

   if (depth > min) goto next;

   r = memcmp(str + depth, rep->str + depth, min - depth);

   U_INTERNAL_DUMP("str[%u] = %.*S", depth, min - depth, str + depth)

   if (r == 0)
next:
      r = (_length - rep->_length);

   U_RETURN(r);
}

// ----------------------------------------------
// gcc: call is unlikely and code size would grow
// ----------------------------------------------
__pure bool UStringRep::isBase64(uint32_t pos) const
{
   U_TRACE(0, "UStringRep::isBase64(%u)", pos)

   U_CHECK_MEMORY

   if (_length)
      {
      U_INTERNAL_ASSERT_MINOR(pos,_length)

      bool result = u_isBase64(str + pos, _length - pos);

      U_RETURN(result);
      }

   U_RETURN(false);
}

__pure bool UStringRep::isWhiteSpace(uint32_t pos) const
{
   U_TRACE(0, "UStringRep::isWhiteSpace(%u)", pos)

   U_CHECK_MEMORY

   if (_length)
      {
      U_INTERNAL_ASSERT_MINOR(pos,_length)

      bool result = u_isWhiteSpace(str + pos, _length - pos);

      U_RETURN(result);
      }

   U_RETURN(false);
}

__pure bool UStringRep::someWhiteSpace(uint32_t pos) const
{
   U_TRACE(0, "UStringRep::someWhiteSpace(%u)", pos)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_MINOR(pos,_length)

   for (; pos < _length; ++pos)
      {
      if (u__isspace(str[pos])) U_RETURN(true);
      }

   U_RETURN(false);
}

__pure bool UStringRep::isEndHeader(uint32_t pos) const
{
   U_TRACE(0, "UStringRep::isEndHeader(%u)", pos)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_MINOR(pos,_length)

   uint32_t _remain = (_length - pos);

   if (_remain >= 4 &&
       (*(uint32_t*)(str + pos)) == (*(uint32_t*)U_CRLF2))
      {
      u_line_terminator     = U_CRLF;
      u_line_terminator_len = 2;

      U_INTERNAL_ASSERT(u__islterm(str[pos]))

      U_RETURN(true);
      }
   else if (_remain >= 2 &&
            (*(uint16_t*)(str + pos)) == (*(uint16_t*)U_LF2))
      {
      u_line_terminator     = U_LF;
      u_line_terminator_len = 1;

      U_INTERNAL_ASSERT(u__islterm(str[pos]))

      U_RETURN(true);
      }

   U_RETURN(false);
}

__pure bool UStringRep::isText(uint32_t pos) const
{
   U_TRACE(0, "UStringRep::isText(%u)", pos)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_MINOR(pos,_length)

   bool result = u_isText((const unsigned char*)(str + pos), _length - pos);

   U_RETURN(result);
}

__pure bool UStringRep::isBinary(uint32_t pos) const
{
   U_TRACE(0, "UStringRep::isBinary(%u)", pos)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_MINOR(pos,_length)

   bool result = u_isBinary((const unsigned char*)(str + pos), _length - pos);

   U_RETURN(result);
}

__pure bool UStringRep::isUTF8(uint32_t pos) const
{
   U_TRACE(0, "UStringRep::isUTF8(%u)", pos)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_MINOR(pos,_length)

   bool result = u_isUTF8((const unsigned char*)(str + pos), _length - pos);

   U_RETURN(result);
}

__pure bool UStringRep::isUTF16(uint32_t pos) const
{
   U_TRACE(0, "UStringRep::isUTF16(%u)", pos)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_MINOR(pos,_length)

   bool result = u_isUTF16((const unsigned char*)(str + pos), _length - pos);

   U_RETURN(result);
}

void UStringRep::size_adjust(uint32_t value)
{
   U_TRACE(0+256, "UStringRep::size_adjust(%u)", value)

   U_CHECK_MEMORY

#ifdef DEBUG
   if (references)
      {
      string_rep_share = this;

      U_DUMP_OBJECT("shared with this", checkIfReferences)

      U_INTERNAL_ASSERT_MSG(false, "CANNOT ADJUST SIZE OF A REFERENCED STRING...")
      }
#endif

   _length = (value == U_NOT_FOUND ? u__strlen(str) : value);

   U_INTERNAL_ASSERT(invariant())
}

UString::UString(const char* t)
{
   U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, UString, "%S", t)

   uint32_t len = (t ? u__strlen(t) : 0);

   if (len) rep = U_NEW(UStringRep(t, len));
   else     _copy(UStringRep::string_rep_null);

   U_INTERNAL_DUMP("this = %p rep = %p", this, rep)

   U_INTERNAL_ASSERT(invariant())
}

UString::UString(const char* t, uint32_t len)
{
   U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, UString, "%.*S,%u", len, t, len)

   if (len) rep = U_NEW(UStringRep(t, len));
   else     _copy(UStringRep::string_rep_null);

   U_INTERNAL_DUMP("this = %p rep = %p", this, rep)

   U_INTERNAL_ASSERT(invariant())
}

UString::UString(const UString& str, uint32_t pos, uint32_t n)
{
   U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, UString, "%p,%u,%u", &str, pos, n)

   U_INTERNAL_ASSERT(pos <= str.size())

   uint32_t sz = str.rep->fold(pos, n);

   if (sz) rep = UStringRep::create(sz, sz, str.rep->str + pos);
   else    _copy(UStringRep::string_rep_null);

   U_INTERNAL_DUMP("this = %p rep = %p", this, rep)

   U_INTERNAL_ASSERT(invariant())
}

UString::UString(ustringrep* r)
{
   U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, UString, "%p", r)

#ifdef DEBUG
   r->_this = (void*)U_CHECK_MEMORY_SENTINEL;
#endif

   uustringrep u = { r };

   _copy(u.p2);

   U_INTERNAL_DUMP("this = %p rep = %p", this, rep)

   U_INTERNAL_ASSERT(invariant())
}

UString& UString::operator=(const UString& str)
{
   U_TRACE(0, "UString::operator=(%p)", &str)

   _assign(str.rep);

   return *this;
}

UString UString::substr(const char* t, uint32_t tlen) const
{
   U_TRACE(0, "UString::substr(%.*S,%u)", tlen, t, tlen)

   UString result(rep, t, tlen);

   U_RETURN_STRING(result);
}

char* UString::c_strdup() const                             { return strndup(rep->str, rep->_length); }
char* UString::c_strndup(uint32_t pos, uint32_t n) const    { return strndup(rep->str + pos, rep->fold(pos, n)); }

UString& UString::assign(const char* s)                     { return assign(s, u__strlen(s)); }
UString& UString::append(const char* s)                     { return append(s, u__strlen(s)); }
UString& UString::append(UStringRep* _rep)                  { return append(_rep->str, _rep->_length); }
UString& UString::append(const UString& str)                { return append(str.data(), str.size()); }

__pure bool UString::equal(UStringRep* _rep) const          { return same(_rep) || rep->equal(_rep); }
__pure bool UString::equal(const char* s) const             { return rep->equal(s, u__strlen(s)); }
__pure bool UString::equal(const char* s, uint32_t n) const { return rep->equal(s, n); }
__pure bool UString::equalnocase(const UString& str) const  { return equalnocase(str.rep); }

void UString::size_adjust_force(uint32_t value)             { rep->size_adjust_force(value); }

void UString::setEmpty()                                    { rep->size_adjust(0); }

UString  UString::substr(uint32_t pos, uint32_t n) const    { return substr(rep->str + pos, rep->fold(pos, n)); }

UString& UString::operator+=(const UString& str)            { return append(str.data(), str.size()); }

UString& UString::erase(uint32_t pos, uint32_t n)           { return replace(pos, rep->fold(pos, n), "", 0); }

__pure uint32_t UString::find(const UString& str, uint32_t pos, uint32_t how_much) const
   { return find(str.data(), pos, str.size(), how_much); }

__pure uint32_t UString::findnocase(const UString& str, uint32_t pos, uint32_t how_much) const
   { return findnocase(str.data(), pos, str.size(), how_much); }

void UString::clear()
{
   U_TRACE(0, "UString::clear()")

   _assign(UStringRep::string_rep_null);

   U_INTERNAL_ASSERT(invariant())
}

UString UString::copy() const
{
   U_TRACE(0, "UString::copy()")

   if (empty()) U_RETURN_STRING(getStringNull());

   uint32_t sz = rep->_length;

   UString copia((void*)rep->str, sz);

   U_RETURN_STRING(copia);
}
// ----------------------------------------------

// how much space we preallocate

#define U_HOW_MUCH_PREALLOCATE(sz1,sz2) \
            (sz1 <= U_CAPACITY        ? sz2 : \
             sz1 >= (2 * 1024 * 1024) ? sz1 : (sz1 * 2) + (3 * PAGESIZE))

// SERVICES

UString::UString(uint32_t n)
{
   U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, UString, "%u", n)

   rep = UStringRep::create(0U, U_HOW_MUCH_PREALLOCATE(n,n), 0);

   U_INTERNAL_DUMP("this = %p rep = %p rep->memory._this = %p", this, rep, rep->memory._this)

   U_INTERNAL_ASSERT(invariant())
}

UString::UString(uint32_t n, unsigned char c)
{
   U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, UString, "%u,%C", n, c)

   rep = UStringRep::create(n, n, 0);

   (void) U_SYSCALL(memset, "%p,%d,%u", (void*)rep->str, c, n);

   U_INTERNAL_DUMP("this = %p rep = %p", this, rep)

   U_INTERNAL_ASSERT(invariant())
}

UString::UString(uint32_t len, uint32_t sz, char* ptr) // NB: for UStringExt::deflate()...
{
   U_TRACE_REGISTER_OBJECT_WITHOUT_CHECK_MEMORY(0, UString, "%u,%u,%p", len, sz, ptr)

   U_INTERNAL_ASSERT_MAJOR(sz, U_CAPACITY)
   U_INTERNAL_ASSERT_EQUALS(sz & U_PAGEMASK, 0)

   rep = U_MALLOC_TYPE(UStringRep);

#ifdef DEBUG
   U_SET_LOCATION_INFO;
   U_REGISTER_OBJECT_PTR(0,UStringRep,rep)
   rep->memory._this = (void*)U_CHECK_MEMORY_SENTINEL;
#endif

   rep->set(len, sz, ptr);

   U_INTERNAL_DUMP("this = %p rep = %p", this, rep)

   U_INTERNAL_ASSERT(invariant())
}

void UString::setBuffer(uint32_t n)
{
   U_TRACE(0, "UString::setBuffer(%u)", n)

   U_INTERNAL_ASSERT_RANGE(1,n,max_size())

   U_INTERNAL_DUMP("rep = %p rep->parent = %p rep->references = %u rep->child = %d", rep, rep->parent, rep->references + 1, rep->child)

   if (rep->references == 0 &&
       n <= rep->_capacity)
      {
      ((char*)rep->str)[(rep->_length = 0)] = '\0';
      }
   else
      {
      _set(UStringRep::create(0U, U_HOW_MUCH_PREALLOCATE(n,n), 0));
      }

   U_INTERNAL_ASSERT(invariant())
}

bool UString::reserve(uint32_t n)
{
   U_TRACE(0, "UString::reserve(%u)", n)

   U_INTERNAL_ASSERT(n <= max_size())

   // Make room for a total of n element

   U_INTERNAL_DUMP("rep = %p rep->parent = %p rep->references = %u rep->child = %d", rep, rep->parent, rep->references + 1, rep->child)

   if (rep->_capacity >= n) U_RETURN(false);

   _set(UStringRep::create(rep->_length, U_HOW_MUCH_PREALLOCATE(n,U_CAPACITY), rep->str));

   U_INTERNAL_ASSERT(invariant())

   U_RETURN(true);
}

// manage UString as memory mapped area...

void UString::mmap(const char* map, uint32_t len)
{
   U_TRACE(0, "UString::mmap(%.*S,%u)", len, map, len)

   U_INTERNAL_ASSERT(map != MAP_FAILED)

   if (isMmap())
      {
      rep->str     = map;
      rep->_length = len;
      }
   else
      {
      _set(U_NEW(UStringRep(map, len)));

      rep->_capacity = U_NOT_FOUND;

#  if defined(MADV_SEQUENTIAL) && defined(__linux__)
      if (len > (64 * PAGESIZE)) (void) U_SYSCALL(madvise, "%p,%u,%d", (void*)map, len, MADV_SEQUENTIAL);
#  endif
      }

   U_INTERNAL_ASSERT(invariant())
}

U_NO_EXPORT char* UString::__replace(uint32_t pos, uint32_t n1, uint32_t n2)
{
   U_TRACE(0, "UString::__replace(%u,%u,%u)", pos, n1, n2)

   U_INTERNAL_ASSERT_DIFFERS(n2, U_NOT_FOUND)

   uint32_t sz = size();

   U_INTERNAL_ASSERT(pos <= sz)

   uint32_t sz1 = rep->fold(pos, n1),
            n   = sz + n2  - sz1;

   U_INTERNAL_DUMP("sz1 = %u, n = %u", sz1, n)

   if (n == 0)
      {
      _assign(UStringRep::string_rep_null);

      return 0;
      }

   int32_t how_much = sz - pos - sz1;

   U_INTERNAL_DUMP("how_much = %d", how_much)

   U_INTERNAL_ASSERT(how_much >= 0)

         char* str = (char*)rep->str;
   const char* src = str + pos + sz1;

   uint32_t __capacity = rep->_capacity;

   if (__capacity == U_NOT_FOUND) __capacity = 0;

   if (rep->references ||
       n > __capacity)
      {
      U_INTERNAL_DUMP("__capacity = %u, n = %u", __capacity, n)

      if (__capacity < n) __capacity = n;

      UStringRep* r = UStringRep::create(n, __capacity, 0);

      if (pos)      U__MEMCPY((void*)r->str,            str, pos);
      if (how_much) U__MEMCPY((char*)r->str + pos + n2, src, how_much);

      _set(r);

      str = (char*)r->str;
      }
   else if (how_much > 0 &&
            n1 != n2)
      {
      (void) U_SYSCALL(memmove, "%p,%p,%u", str + pos + n2, src, how_much);
      }

   str[(rep->_length = n)] = '\0';

   return str + pos;
}

UString& UString::replace(uint32_t pos, uint32_t n1, const char* s, uint32_t n2)
{
   U_TRACE(0, "UString::replace(%u,%u,%S,%u)", pos, n1, s, n2)

   char* ptr = __replace(pos, n1, n2);

   if (ptr && n2) U__MEMCPY(ptr, s, n2);

   U_INTERNAL_ASSERT(invariant())

   return *this;
}

UString& UString::replace(uint32_t pos, uint32_t n1, uint32_t n2, char c)
{
   U_TRACE(0, "UString::replace(%u,%u,%u,%C)", pos, n1, n2, c)

   char* ptr = __replace(pos, n1, n2);

   if (ptr && n2) (void) U_SYSCALL(memset, "%p,%d,%u", ptr, c, n2);

   U_INTERNAL_ASSERT(invariant())

   return *this;
}

void UString::unQuote()
{
   U_TRACE(0, "UString::unQuote()")

   uint32_t len = rep->_length;

        if (len            <= 2) clear();
   else if (rep->_capacity == 0) rep->unQuote();
   else
      {
      len -= 2;

      char* ptr = (char*) rep->str;

      (void) U_SYSCALL(memmove, "%p,%p,%u", ptr, ptr + 1, len);

      ptr[(rep->_length = len)] = '\0';
      }
}

U_NO_EXPORT char* UString::__append(uint32_t n)
{
   U_TRACE(0, "UString::__append(%u)", n)

   UStringRep* r;
   char* str = (char*)rep->str;
   uint32_t sz = size(), sz1 = sz + n;

   U_INTERNAL_DUMP("sz1 = %u", sz1)

   U_INTERNAL_ASSERT_MAJOR(sz1,0)

   if (rep->references ||
       sz1 > rep->_capacity)
      {
      r = UStringRep::create(sz, U_HOW_MUCH_PREALLOCATE(sz1,U_CAPACITY), str);

      _set(r);

      str = (char*)r->str;
      }

   str[(rep->_length = sz1)] = '\0';

   return str + sz;
}

UString& UString::append(const char* s, uint32_t n)
{
   U_TRACE(0, "UString::append(%.*S,%u)", n, s, n)

   if (n)
      {
      char* ptr = __append(n);

      U__MEMCPY(ptr, s, n);
      }

   U_INTERNAL_ASSERT(invariant())

   return *this;
}

UString& UString::append(uint32_t n, char c)
{
   U_TRACE(0, "UString::append(%u,%C)", n, c)

   if (n)
      {
      char* ptr    = __append(n);
            ptr[0] = c;

      if (--n) (void) U_SYSCALL(memset, "%p,%d,%u", ptr+1, c, n);
      }

   U_INTERNAL_ASSERT(invariant())

   return *this;
}

void UString::duplicate() const
{
   U_TRACE(0, "UString::duplicate()")

   uint32_t sz = size();

   if (sz) ((UString*)this)->_set(UStringRep::create(sz, sz, rep->str));
   else
      {
        ((UString*)this)->_set(UStringRep::create(0, 100U, 0));

      *(((UString*)this)->UString::rep->begin()) = '\0';
      }

   U_INTERNAL_ASSERT(invariant())
   U_INTERNAL_ASSERT(isNullTerminated())
}

void UString::setNullTerminated() const
{
   U_TRACE(0, "UString::setNullTerminated()")

   // A file is mapped in multiples of the page size. For a file that is not a multiple of the page size,
   // the remaining memory is zeroed when mapped, and writes to that region are not written out to the file.

   if (writeable() == false ||
       (isMmap() && (rep->_length % PAGESIZE) == 0))
      {
      duplicate();
      }
   else
      {
      rep->setNullTerminated();
      }

   U_INTERNAL_ASSERT_EQUALS(u__strlen(rep->str), rep->_length)
}

void UString::resize(uint32_t n, unsigned char c)
{
   U_TRACE(0, "UString::resize(%u,%C)", n, c)

   U_INTERNAL_ASSERT(n <= max_size())

   uint32_t sz = size();

   if      (n > sz) (void) append(n - sz, c);
   else if (n < sz) erase(n);
   else             size_adjust(n);

   U_INTERNAL_ASSERT(invariant())
}

// The `find' function searches string for a specified string (possibly a single character) and returns
// its starting position. You can supply the parameter pos to specify the position where search must begin

__pure uint32_t UString::find(const char* s, uint32_t pos, uint32_t s_len, uint32_t how_much) const
{
   U_TRACE(0, "UString::find(%S,%u,%u,%u)", s, pos, s_len, how_much)

   U_INTERNAL_ASSERT_MAJOR(s_len,0)

   // An empty string consists of no characters, therefore it should be
   // found at every point in a UString, except beyond the end...
   // if (s_len == 0) U_RETURN(pos <= size() ? pos : U_NOT_FOUND);

   uint32_t n = rep->fold(pos, how_much);

   U_INTERNAL_DUMP("rep->_length = %u", rep->_length)

   U_INTERNAL_ASSERT(n <= rep->_length)

   const char* str = rep->str;
   const char* ptr = (const char*) u_find(str + pos, n, s, s_len);

   n = (ptr ? ptr - str : U_NOT_FOUND);

   U_RETURN(n);
}

__pure uint32_t UString::findnocase(const char* s, uint32_t pos, uint32_t s_len, uint32_t how_much) const
{
   U_TRACE(0, "UString::findnocase(%S,%u,%u,%u)", s, pos, s_len, how_much)

   U_INTERNAL_ASSERT_MAJOR(s_len,1)

   uint32_t n     = rep->fold(pos, how_much);
    int32_t __end = n - s_len + 1;

   if (__end > 0)
      {
      const char* str = rep->str + pos;

      for (int32_t xpos = 0; xpos < __end; ++xpos)
         {
         if (strncasecmp(str + xpos, s, s_len) == 0) U_RETURN(pos+xpos);
         }
      }

   U_RETURN(U_NOT_FOUND);
}

__pure uint32_t UString::find(unsigned char c, uint32_t pos) const
{
   U_TRACE(0, "UString::find(%C,%u)", c, pos)

   uint32_t sz  = size(),
            ret = U_NOT_FOUND;

   const char* str = rep->str;

   if (pos < sz)
      {
      uint32_t how_much = (sz - pos);

   // U_INTERNAL_DUMP("how_much = %u", how_much)

      void* p = (void*) memchr(str + pos, c, how_much);

      if (p) ret = (const char*)p - str;
      }

   U_RETURN(ret);
}

// this is rfind(). instead of starting at the beginning of the string and searching for the text's first occurence,
// rfind() starts its search at the end and returns the last occurence.

__pure uint32_t UString::rfind(unsigned char c, uint32_t pos) const
{
   U_TRACE(0, "UString::rfind(%C,%u)", c, pos)

   uint32_t sz = size();

   if (sz)
      {
      uint32_t xpos = sz - 1;

      if (xpos > pos) xpos = pos;

      const char* str = rep->str;

      for (++xpos; xpos-- > 0; )
         {
         if (str[xpos] == c) U_RETURN(xpos);
         }
      }

   U_RETURN(U_NOT_FOUND);
}

__pure uint32_t UString::rfind(const char* s, uint32_t pos, uint32_t n) const
{
   U_TRACE(0, "UString::rfind(%S,%u,%u)", s, pos, n)

   uint32_t sz = size();

   if (n <= sz)
      {
      pos = U_min(sz - n, pos);

      const char* str = rep->str;

      do {
         if (memcmp(str + pos, s, n) == 0) U_RETURN(pos);
         }
      while (pos-- > 0);
      }

   U_RETURN(U_NOT_FOUND);
}

// Instead of searching for the entire string, find_first_of() returns as soon as a single common element is found
// between the strings being compared. And yes, this means that the find_first_of()s that take a single char are
// exactly the same as the find() functions with the same parameters

__pure uint32_t UString::find_first_of(const char* s, uint32_t pos, uint32_t n) const
{
   U_TRACE(0, "UString::find_first_of(%S,%u,%u)", s, pos, n)

   if (n)
      {
      uint32_t sz = size();

      const char* str = rep->str;

      for (; pos < sz; ++pos)
         {
         if (memchr(s, str[pos], n)) U_RETURN(pos);
         }
      }

   U_RETURN(U_NOT_FOUND);
}

__pure uint32_t UString::find_last_of(const char* s, uint32_t pos, uint32_t n) const
{
   U_TRACE(0, "UString::find_last_of(%S,%u,%u)", s, pos, n)

   uint32_t sz = size();

   if (sz && n)
      {
      if (--sz > pos) sz = pos;

      const char* str = rep->str;

      do {
         if (memchr(s, str[sz], n)) U_RETURN(sz);
         }
      while (sz-- != 0);
      }

   U_RETURN(U_NOT_FOUND);
}

// Now these functions, instead of returning an index to the first common element,
// returns an index to the first non-common element

__pure uint32_t UString::find_first_not_of(const char* s, uint32_t pos, uint32_t n) const
{
   U_TRACE(0, "UString::find_first_not_of(%S,%u,%u)", s, pos, n)

   if (n)
      {
      uint32_t sz   = size(),
               xpos = pos;

      const char* str = rep->str;

      for (; xpos < sz; ++xpos)
         {
         if (memchr(s, str[xpos], n) == 0) U_RETURN(xpos);
         }
      }

   U_RETURN(U_NOT_FOUND);
}

__pure uint32_t UString::find_first_not_of(unsigned char c, uint32_t pos) const
{
   U_TRACE(0, "UString::find_first_not_of(%C,%u)", c, pos)

   uint32_t sz   = size(),
            xpos = pos;

   const char* str = rep->str;

   for (; xpos < sz; ++xpos)
      {
      if (str[xpos] != c) U_RETURN(xpos);
      }

   U_RETURN(U_NOT_FOUND);
}

__pure uint32_t UString::find_last_not_of(const char* s, uint32_t pos, uint32_t n) const
{
   U_TRACE(0, "UString::find_last_not_of(%S,%u,%u)", s, pos, n)

   uint32_t sz = size();

   if (sz && n)
      {
      if (--sz > pos) sz = pos;

      const char* str = rep->str;

      do {
         if (memchr(s, str[sz], n) == 0) U_RETURN(sz);
         }
      while (sz-- != 0);
      }

   U_RETURN(U_NOT_FOUND);
}

__pure uint32_t UString::find_last_not_of(unsigned char c, uint32_t pos) const
{
   U_TRACE(0, "UString::find_last_not_of(%C,%u)", c, pos)

   uint32_t sz = size();

   if (sz)
      {
      uint32_t xpos = sz - 1;

      if (xpos > pos) xpos = pos;

      const char* str = rep->str;

      for (++xpos; xpos-- > 0; )
         {
         if (str[xpos] != c) U_RETURN(xpos);
         }
      }

   U_RETURN(U_NOT_FOUND);
}

// EXTENSION

void UString::snprintf(const char* format, ...)
{
   U_TRACE(0, "UString::snprintf(%S)", format)

   va_list argp;
   va_start(argp, format);

   UString::vsnprintf(format, argp); 

   va_end(argp);
}

void UString::snprintf_add(const char* format, ...)
{
   U_TRACE(0, "UString::snprintf_add(%S)", format)

   va_list argp;
   va_start(argp, format);

   UString::vsnprintf_add(format, argp); 

   va_end(argp);
}

__pure bool UStringRep::strtob() const
{
   U_TRACE(0, "UStringRep::strtob()")

   if (_length)
      {
      unsigned char c = str[0];

      if (           c  == '1' ||
          u__toupper(c) == 'Y')
         {
         U_RETURN(true);
         }

      U_INTERNAL_ASSERT(          c  == '0' ||
                        u__toupper(c) == 'N')
      }

   U_RETURN(false);
}

long UStringRep::strtol(int base) const
{
   U_TRACE(0, "UStringRep::strtol(%d)", base)

   if (_length)
      {
      char* eos = (char*)str + _length;

      if (isNullTerminated() == false && writeable()) *eos = '\0';

      errno = 0;

      char* endptr;
      long  result = (long) strtoul(str, &endptr, base);

      U_INTERNAL_DUMP("errno = %d", errno)

      if (endptr &&
          endptr < eos)
         {
         U_NUMBER_SUFFIX(result, *endptr);
         }

      U_RETURN(result);
      }

   U_RETURN(0L);
}

#ifdef HAVE_STRTOULL

int64_t UStringRep::strtoll(int base) const
{
   U_TRACE(0, "UStringRep::strtoll(%d)", base)

   if (_length)
      {
      char* eos = (char*)str + _length;

      if (isNullTerminated() == false && writeable()) *eos = '\0';

      char* endptr;
      int64_t result = (int64_t) strtoull(str, &endptr, base);

      U_INTERNAL_DUMP("errno = %d", errno)

      if (endptr &&
          endptr < eos)
         {
         U_NUMBER_SUFFIX(result, *endptr);
         }

      U_RETURN(result);
      }

   U_RETURN(0LL);
}

#endif

#ifdef HAVE_STRTOF

// extern "C" { float strtof(const char* nptr, char** endptr); }

float UStringRep::strtof() const
{
   U_TRACE(0, "UStringRep::strtof()")

   if (_length)
      {
      if (isNullTerminated() == false && writeable()) setNullTerminated();

      float result = ::strtof(str, 0);

      U_INTERNAL_DUMP("errno = %d", errno)

      U_RETURN(result);
      }

   U_RETURN(0);
}

#endif

double UStringRep::strtod() const
{
   U_TRACE(0, "UStringRep::strtod()")

   if (_length)
      {
      if (isNullTerminated() == false && writeable()) setNullTerminated();

      double result = ::strtod(str, 0);

      U_INTERNAL_DUMP("errno = %d", errno)

      U_RETURN(result);
      }

   U_RETURN(0);
}

#ifdef HAVE_STRTOLD

// extern "C" { long double strtold(const char* nptr, char** endptr); }

long double UStringRep::strtold() const
{
   U_TRACE(0, "UStringRep::strtold()")

   if (_length)
      {
      if (isNullTerminated() == false && writeable()) setNullTerminated();

      long double result = ::strtold(str, 0);

      U_INTERNAL_DUMP("errno = %d", errno)

      U_RETURN(result);
      }

   U_RETURN(0);
}

#endif

// UTF8 <--> ISO Latin 1

UStringRep* UStringRep::fromUTF8(const unsigned char* s, uint32_t n)
{
   U_TRACE(0, "UStringRep::fromUTF8(%.*S,%u)", n, s, n)

   U_INTERNAL_ASSERT_POINTER(s)

   if (n == 0) return UStringRep::string_rep_null;

   int c, c1, c2;
   UStringRep* r = UStringRep::create(n, n, 0);

   char* p                   = (char*)r->str;
   const unsigned char* _end = s + n;

   while (s < _end)
      {
      if (  s < (_end - 1)        &&
          (*s     & 0xE0) == 0xC0 &&
          (*(s+1) & 0xC0) == 0x80)
         {
         c1 = *s++ & 0x1F;
         c2 = *s++ & 0x3F;
         c  = (c1 << 6) + c2;

         U_INTERNAL_DUMP("c = %d %C", c, (char)c)
         }
      else
         {
         c = *s++;
         }

      *p++ = (unsigned char)c;
      }

   r->_length = p - r->str;

   U_INTERNAL_ASSERT(r->invariant())

   U_RETURN_POINTER(r, UStringRep);
}

UStringRep* UStringRep::toUTF8(const unsigned char* s, uint32_t n)
{
   U_TRACE(0, "UStringRep::toUTF8(%.*S,%u)", n, s, n)

   U_INTERNAL_ASSERT_POINTER(s)

   if (n == 0) return UStringRep::string_rep_null;

   int c;
   UStringRep* r = UStringRep::create(n, n * 2, 0);

   char* p                   = (char*)r->str;
   const unsigned char* _end = s + n;

   while (s < _end)
      {
      c = *s++;

      if (c >= 0x80)
         {
         *p++ = ((c >> 6) & 0x1F) | 0xC0;
         *p++ = ( c       & 0x3F) | 0x80;

         continue;
         }

      *p++ = (unsigned char)c;
      }

   r->_length = p - r->str;

   U_INTERNAL_ASSERT(r->invariant())

   U_RETURN_POINTER(r, UStringRep);
}

// STREAM

void UStringRep::write(ostream& os) const
{
   U_TRACE(0, "UStringRep::write(%p)", &os)

   bool need_quote;

   if ((need_quote = (_length == 0)) == false)
      {
      for (char* s = (char*)str, *_end = s + _length; s < _end; ++s)
         {
         if (strchr(" \"\\\n\r\t\b", *s))
            {
            need_quote = true;

            break;
            }
         }
      }

   if (need_quote == false) os.write(str, _length);
   else
      {
      os.put('"');

      char* p;
      char* s    = (char*)str;
      char* _end = s + _length;

      while (s < _end)
         {
         p = (char*) memchr(s, '"', _end - s);

         if (p == 0)
            {
            os.write(s, _end - s);

            break;
            }

         os.write(s, p - s);

         if (*(p-1) == '\\') os.put('\\');
                             os.put('\\');
                             os.put('"');

         s = p + 1;
         }

      os.put('"');
      }
}

// OPTMIZE APPEND (BUFFERED)

char  UString::appbuf[1024];
char* UString::ptrbuf = appbuf;

U_EXPORT istream& operator>>(istream& in, UString& str)
{
   U_TRACE(0+256, "UString::operator>>(%p,%p)", &in, &str)

   uint32_t extracted = 0;

   if (in.good())
      {
      streambuf* sb = in.rdbuf();

      int c = sb->sbumpc();

      if (in.flags() & ios::skipws)
         {
         while ((c != EOF) &&
                u__isspace(c))
            {
            c = sb->sbumpc();
            }
         }

      if (c != EOF)
         {
         if (str.empty() == false) str.erase();

         streamsize w = in.width();

         uint32_t n = (w > 0 ? (uint32_t)w
                             : str.max_size());

         while (extracted < n &&
                u__isspace(c) == false)
            {
            str._append(c);

            ++extracted;

            c = sb->sbumpc();

            U_INTERNAL_DUMP("c = %C, EOF = %C", c, EOF)

            if (c == EOF) break;
            }

         str._append();
         }

      if (c == EOF) in.setstate(ios::eofbit);
      else          sb->sputbackc(c);

      in.width(0);

      U_INTERNAL_DUMP("size = %u, str = %.*S", str.size(), str.size(), str.data())
      }

   if (extracted == 0) in.setstate(ios::failbit);
   else                str.setNullTerminated();

   U_INTERNAL_ASSERT(str.invariant())

   return in;
}

istream& UString::getline(istream& in, unsigned char delim)
{
   U_TRACE(0+256, "UString::getline(%p,%C)", &in, delim)

   int c          = EOF;
   bool extracted = false;

   if (in.good())
      {
      erase();

      streambuf* sb = in.rdbuf();

      while (true)
         {
         c = sb->sbumpc();

         U_INTERNAL_DUMP("c = %C", c)

         if (c == '\\')
            {
            c = sb->sbumpc();

            U_INTERNAL_DUMP("c = %C", c)

            if (c == delim)
               {
               _append(delim);

               continue;
               }

            if (strchr("nrt\nbfvae", c))
               {
               if (c != '\n') _append('\\');
               else
                  {
                  // compress multiple white-space in a single new-line...

                  do { c = sb->sbumpc(); } while (c != EOF && u__isspace(c));

                  if (c != EOF)
                     {
                     sb->sputbackc(c);

                     c = '\n';
                     }
                  }
               }
            }

         if (c == EOF)
            {
            in.setstate(ios::eofbit);

            break;
            }

         if (c == delim) break;

         _append(c);
         }

      _append();

      U_INTERNAL_DUMP("size = %u, str = %.*S", size(), size(), data())

      extracted = (empty() == false);

      if (extracted) setNullTerminated();
      }

   if (c         != delim &&
       extracted == false)
      {
      in.setstate(ios::failbit);
      }

   U_INTERNAL_ASSERT(invariant())

   return in;
}

U_EXPORT ostream& operator<<(ostream& out, const UString& str)
{
   U_TRACE(0+256, "UString::operator<<(%p,%p)", &out, &str)

   if (out.good())
      {
      const char* s = str.data();

      streamsize res,
                 w   = out.width(),
                 len = (streamsize)str.size();

      if (w <= len) res = out.rdbuf()->sputn(s, len);
      else
         {
         int plen = (int)(w - len);

         ios::fmtflags fmt = (out.flags() & ios::adjustfield);

         if (fmt == ios::left)
            {
            res = out.rdbuf()->sputn(s, len);

            // Padding last

            for (int i = 0; i < plen; ++i) (void) out.rdbuf()->sputc(' ');
            }
         else
            {
            // Padding first

            for (int i = 0; i < plen; ++i) (void) out.rdbuf()->sputc(' ');

            res = out.rdbuf()->sputn(s, len);
            }
         }

      U_INTERNAL_DUMP("len = %u, res = %u, w = %u", len, res, w)

      out.width(0);

      if (res != len) out.setstate(ios::failbit);
      }

   return out;
}

// operator +

U_EXPORT UString operator+(const UString& lhs, const UString& rhs)
{ UString str(lhs); str.append(rhs); return str; }

U_EXPORT UString operator+(const char* lhs, const UString& rhs)
{
   uint32_t len = u__strlen(lhs);
   UString str(len + rhs.size());

   (void) str.append(lhs, len);
   (void) str.append(rhs);

   return str;
}

U_EXPORT UString operator+(char lhs, const UString& rhs)
{
   UString str(1U + rhs.size());

   (void) str.append(1U, lhs);
   (void) str.append(rhs);

   return str;
}

U_EXPORT UString operator+(const UString& lhs, char rhs)
{ UString str(lhs); (void) str.append(1U, rhs); return str; }

U_EXPORT UString operator+(const UString& lhs, const char* rhs)
{ UString str(lhs); (void) str.append(rhs, u__strlen(rhs)); return str; }

#if defined(DEBUG) || defined(U_TEST)
#  include <ulib/internal/objectIO.h>

const char* UStringRep::dump(bool reset) const
{
   *UObjectIO::os << "length     " << _length       << '\n'
                  << "capacity   " << _capacity     << '\n'
                  << "references " << references+1  << '\n'
#              ifdef DEBUG
                  << "parent     " << (void*)parent << '\n'
                  << "child      " << child         << '\n'
#              endif
                  << "str        " << (void*)str    << ' ';

   char buffer[1024];

   UObjectIO::os->write(buffer, u__snprintf(buffer, sizeof(buffer), "%.*S", U_min(_length, 256U), str));

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

bool UStringRep::invariant() const
{
   if (this != string_rep_null)
      {
      U_CHECK_MEMORY

      return string_rep_null->invariant();
      }
   else if (_length)
      {
      U_WARNING("error on string_rep_null: (not empty)\n"
                "--------------------------------------------------\n%s", UStringRep::dump(true));

      return false;
      }

   if (_capacity < _length &&
       _capacity)
      {
      U_WARNING("error on rep string: (overflow)\n"
                "--------------------------------------------------\n%s", UStringRep::dump(true));

      return false;
      }

   if ((int32_t)references < 0)
      {
      U_WARNING("error on rep string: (leak reference)\n"
                "--------------------------------------------------\n%s", UStringRep::dump(true));

      return false;
      }

   return true;
}

bool UString::invariant() const
{
   if (rep == 0)
      {
      U_WARNING("error on string: (rep = null pointer)");

      return false;
      }

   return rep->invariant();
}
#endif

#ifdef DEBUG
const char* UString::dump(bool reset) const
{
   U_CHECK_MEMORY_OBJECT(rep)

   *UObjectIO::os << "rep (UStringRep " << (void*)rep << ")";

   if (rep == rep->string_rep_null) *UObjectIO::os << " == UStringRepNull";

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}
#endif
