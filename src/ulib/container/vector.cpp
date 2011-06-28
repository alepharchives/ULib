// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    vector.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/container/vector.h>

#ifdef HAVE_STRSTREAM_H
#  include <strstream.h>
#else
#  include <ulib/replace/strstream.h>
#endif

void UVector<void*>::push(void* elem) // add to end
{
   U_TRACE(0, "UVector<void*>::push(%p)", elem)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(_length <= _capacity)

   if (_length == _capacity) reserve(_capacity * 2);

   vec[_length++] = elem;
}

// LIST OPERATIONS

void UVector<void*>::insert(uint32_t pos, void* elem) // add elem before pos
{
   U_TRACE(0, "UVector<void*>::insert(%u,%p)", pos, elem)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT(pos <= _length)
   U_INTERNAL_ASSERT(_length <= _capacity)

   if (_length == _capacity)
      {
      void**   old_vec      = vec;
      uint32_t old_capacity = _capacity;

      allocate(_capacity * 2);

      (void) u_memcpy(vec,           old_vec,                  pos  * sizeof(void*));
      (void) u_memcpy(vec + pos + 1, old_vec + pos, (_length - pos) * sizeof(void*));

      U_FREE_VECTOR(old_vec, old_capacity, void);
      }
   else
      {
      (void) U_SYSCALL(memmove, "%p,%p,%u", vec + pos + 1, vec + pos, (_length - pos) * sizeof(void*));
      }

   vec[pos] = elem;

   ++_length;
}

void UVector<void*>::insert(uint32_t pos, uint32_t n, void* elem) // add n copy of elem before pos
{
   U_TRACE(0, "UVector<void*>::insert(%u,%u,%p)", pos, n, elem)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_MAJOR(n,0)
   U_INTERNAL_ASSERT(pos <= _length)
   U_INTERNAL_ASSERT(_length <= _capacity)

   uint32_t new_length = _length + n;

   if (new_length > _capacity)
      {
      void**   old_vec      = vec;
      uint32_t old_capacity = _capacity;

      allocate(new_length * 2);

      (void) u_memcpy(vec,           old_vec,                  pos  * sizeof(void*));
      (void) u_memcpy(vec + pos + n, old_vec + pos, (_length - pos) * sizeof(void*));

      U_FREE_VECTOR(old_vec, old_capacity, void);
      }
   else
      {
      (void) U_SYSCALL(memmove, "%p,%p,%u", vec + pos + n, vec + pos, (_length - pos) * sizeof(void*));
      }

   for (uint32_t i = 0; i < n; ++i)
      {
      vec[pos++] = elem;
      }

   _length = new_length;
}

void UVector<void*>::reserve(uint32_t n)
{
   U_TRACE(0, "UVector<void*>::reserve(%u)", n)

        if (n == 0) allocate(64); // NB: the check n == 0 is specific for class UTree...
   else if (n != _capacity)
      {
      U_INTERNAL_ASSERT_MAJOR(_capacity,0)

      void**   old_vec      = vec;
      uint32_t old_capacity = _capacity;

      allocate(n);

      if (_length) (void) u_memcpy(vec, old_vec, _length * sizeof(void*));

      U_FREE_VECTOR(old_vec, old_capacity, void);
      }
}

// specializzazione stringa

UVector<UString>::UVector(const UString& x, const char* delim) : UVector<UStringRep*>(64)
{
   U_TRACE_REGISTER_OBJECT(0, UVector<UString>, "%.*S,%S", U_STRING_TO_TRACE(x), delim)

   const char* s = x.data();

   if (*s == '[' ||
       *s == '(')
      {
      istrstream is(s, x.size());

      is >> *this;
      }
   else
      {
      (void) split(x, delim);
      }


   if (_length) reserve(_length);
}

UVector<UString>::~UVector()
{
   U_TRACE_UNREGISTER_OBJECT(0, UVector<UString>)
}

void UVector<UString>::push(const UString& str) // add to end
{
   U_TRACE(0, "UVector<UString>::push(%.*S)", U_STRING_TO_TRACE(str))

   UVector<UStringRep*>::push(str.rep);
}

__pure uint32_t UVector<UString>::find(const char* s, uint32_t n, uint32_t offset)
{
   U_TRACE(0, "UVector<UString>::find(%#.*S,%u)", n, s, n)

   U_CHECK_MEMORY

   UStringRep* r;

   for (uint32_t i = 0; i < _length; ++i)
      {
      r = UVector<UStringRep*>::at(i);

      if (u_find(r->data() + offset, r->size() - offset, s, n)) U_RETURN(i);
      }

   U_RETURN(U_NOT_FOUND);
}

__pure uint32_t UVector<UString>::find(const UString& str, bool ignore_case)
{
   U_TRACE(0, "UVector<UString>::find(%.*S,%b)", U_STRING_TO_TRACE(str), ignore_case)

   U_CHECK_MEMORY

   UStringRep* r;

   for (uint32_t i = 0; i < _length; ++i)
      {
      r = UVector<UStringRep*>::at(i);

      if ((ignore_case ? str.equalnocase(r) : str.equal(r)) == true) U_RETURN(i);
      }

   U_RETURN(U_NOT_FOUND);
}

__pure uint32_t UVector<UString>::findSorted(const UString& str, bool ignore_case)
{
   U_TRACE(0, "UVector<UString>::findSorted(%.*S,%b)", U_STRING_TO_TRACE(str), ignore_case)

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("_length = %u", _length)

   U_INTERNAL_ASSERT_RANGE(1,_length,_capacity)

   UStringRep* key;
   UStringRep* target = str.rep;
   int32_t cmp = -1, probe, low = -1, high = _length;

   while ((high - low) > 1)
      {
      probe = ((low + high) & 0xFFFFFFFFL) >> 1;

      U_INTERNAL_DUMP("low = %d high = %d probe = %d", low, high, probe)

      key = UVector<UStringRep*>::at(probe);
      cmp = (ignore_case ? key->comparenocase(target)
                         : key->compare(      target));

           if (cmp  > 0) high = probe;
      else if (cmp == 0) U_RETURN(probe);
      else               low = probe;
      }

   if (low == -1 || (key = UVector<UStringRep*>::at(low),
                     (ignore_case ? key->comparenocase(target)
                                  : key->compare(target))) != 0)
      {
      U_RETURN(U_NOT_FOUND);
      }

   U_RETURN(low);
}

uint32_t UVector<UString>::contains(const UString& str, bool ignore_case)
{
   U_TRACE(0, "UVector<UString>::contains(%.*S,%b)", U_STRING_TO_TRACE(str), ignore_case)

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("_length = %u", _length)

   U_INTERNAL_ASSERT(_length <= _capacity)

   UString elem;
   uint32_t i, n;

   for (i = 0; i < _length; ++i)
      {
      elem = at(i);
      n    = (ignore_case ? elem.findnocase(str) : elem.find(str));

      if (n != U_NOT_FOUND) U_RETURN(i);
      }

   U_RETURN(U_NOT_FOUND);
}

bool UVector<UString>::contains(UVector<UString>& _vec, bool ignore_case)
{
   U_TRACE(0, "UVector<UString>::contains(%p,%b)", &_vec, ignore_case)

   U_CHECK_MEMORY

   UString elem;

   for (uint32_t i = 0, n = _vec.size(); i < n; ++i)
      {
      elem = _vec.at(i);

      if (contains(elem, ignore_case) != U_NOT_FOUND) U_RETURN(true);
      }

   U_RETURN(false);
}

// Check equality with an existing vector object

bool UVector<UString>::_isEqual(UVector<UString>& _vec, bool ignore_case)
{
   U_TRACE(0, "UVector<UString>::_isEqual(%p,%b)", &_vec, ignore_case)

   U_INTERNAL_ASSERT(_length <= _capacity)

   if (_length)
      {
      UString elem;

      for (uint32_t i = 0; i < _length; ++i)
         {
         elem = at(i);

         if (_vec.find(elem, ignore_case) == U_NOT_FOUND) U_RETURN(false);
         }

      U_RETURN(true);
      }

   U_RETURN(false);
}

void UVector<UString>::sort(bool ignore_case)
{
   U_TRACE(0, "UVector<UString>::sort(%b)", ignore_case)

   U_INTERNAL_DUMP("_length = %u", _length)

   U_INTERNAL_ASSERT_RANGE(2,_length,_capacity)

   if (ignore_case) UVector<void*>::sort(UVector<UString>::qscomp);
   else             mksort((UStringRep**)vec, _length, 0);
}

UString UVector<UString>::operator[](uint32_t pos) const { return at(pos); }

UString UVector<UString>::join(const char* t, uint32_t tlen)
{
   U_TRACE(0, "UVector<UString>::join(%.*S,%u)", tlen, t, tlen)

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("_length = %u", _length)

   U_INTERNAL_ASSERT(_length <= _capacity)

   uint32_t i   = 0,
            len = 0;

   for (; i < _length; ++i) len += ((UStringRep*)vec[i])->size();

   len += (_length - 1) * tlen;

   UString str(len);

   str.size_adjust(len);

   i = 0;
   uint32_t sz;
   UStringRep* rep;
   char* ptr = str.data();

   while (true)
      {
      sz = (rep = (UStringRep*)vec[i])->size();

      if (sz) (void) u_memcpy(ptr, rep->data(), sz);

      if (++i < _length)
         {
         ptr += sz;

         if (tlen)
            {
            (void) u_memcpy(ptr, t, tlen);

            ptr += tlen;
            }
         }
      else
         {
         break;
         }
      }

   U_RETURN_STRING(str);
}

uint32_t UVector<UString>::split(const UString& str, const char* delim, bool dup)
{
   U_TRACE(0, "UVector<UString>::split(%.*S,%S,%b)", U_STRING_TO_TRACE(str), delim, dup)

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("_length = %u", _length)

   U_INTERNAL_ASSERT(_length <= _capacity)

   const char* p;
   UStringRep* r;

   uint32_t len, n  = _length;
   const char* s    = str.data();
   const char* _end = s + str.size();

   if (*s == '"')
      {
      ++s;

      _end -= 1;
      }

   while (s < _end)
      {
      s = u_delimit_token(s, &p, _end, delim, '#');

      U_INTERNAL_DUMP("s = %p end = %p", s, _end)

      if (s <= _end)
         {
         len = s++ - p;

         r = (dup ? UStringRep::create(len, len, p)
                  : str.rep->substr(p, len));

         UVector<void*>::push(r);
         }
      }

   U_RETURN(_length - n);
}

uint32_t UVector<UString>::split(const UString& str, char delim)
{
   U_TRACE(0, "UVector<UString>::split(%.*S,%C)", U_STRING_TO_TRACE(str), delim)

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("_length = %u", _length)

   U_INTERNAL_ASSERT(_length <= _capacity)

   const char* p;
   UStringRep* r;

   uint32_t n       = _length;
   const char* s    = str.data();
   const char* _end = s + str.size();

   if (*s == '"')
      {
      ++s;

      _end -= 1;
      }

   while (s < _end)
      {
      // skip char delimiter

      if (*s == delim)
         {
         ++s;

         continue;
         }

      // delimit token with char delimiter

      p = s;
      s = (const char*) memchr(s, delim, _end - s);

      if (s == 0) s = _end;

      r = str.rep->substr(p, s - p);

      UVector<void*>::push(r);

      ++s;
      }

   U_RETURN(_length - n);
}

uint32_t UVector<UString>::intersection(UVector<UString>& set1, UVector<UString>& set2)
{
   U_TRACE(0, "UVector<UString>::intersection(%p,%p)", &set1, &set2)

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("_length = %u", _length)

   U_INTERNAL_ASSERT(_length <= _capacity)

   if (set1.empty() ||
       set2.empty()) U_RETURN(0);

   UString elem;
   uint32_t i, n = _length;

   for (i = 0; i < set1._length; ++i)
      {
      elem = set1.at(i);

      if (set2.find(elem) != U_NOT_FOUND) push(elem);
      }

   U_RETURN(_length - n);
}

// THREE-WAY RADIX QUICKSORT
// ------------------------------------------------------
// Multikey Quicksort - Dr. Dobb's Journal, November 1998
//
// by Jon Bentley and Robert Sedgewick
// ------------------------------------------------------

static inline int chfunc(UStringRep* a[], int i, int depth)
{
   U_TRACE(0, "chfunc(%p,%d,%d)", a, i, depth)

   UStringRep* t = a[i];

   U_INTERNAL_DUMP("t = %.*S", U_STRING_TO_TRACE(*t))

   int result = (t->data())[depth];

   U_RETURN(result);
}

/* Simple version

static inline void swap2(UStringRep* a[], int i, int j)
{
   U_TRACE(0, "swap2(%p,%d,%d)", a, i, j)

   UStringRep* t = a[i];
   a[i] = a[j];
   a[j] = t;
}

static inline void vecswap2(UStringRep* a[], int i, int j, int n)
{
   U_TRACE(0, "vecswap2(%p,%d,%d,%d)", a, i, j, n)

   while (n-- > 0) swap2(a, i++, j++);
}

#define ch(i) chfunc(a, i, depth)

void UVector<UString>::mksort(UStringRep** a, int n, int depth)
{
   U_TRACE(0+256, "UVector<UString>::mksort(%p,%d,%d)", a, n, depth)

   int le, lt, gt, ge, r, v;

   if (n <= 1) return;

   swap2(a, 0, u_random(n) % n);

   v = ch(0);
   le = lt = 1;
   gt = ge = n-1;

   for (;;)
      {
      for (; lt <= gt && ch(lt) <= v; lt++) if (ch(lt) == v) swap2(a, le++, lt);
      for (; lt <= gt && ch(gt) >= v; gt--) if (ch(gt) == v) swap2(a, gt, ge--);

      if (lt > gt) break;

      swap2(a, lt++, gt--);
      }

   r = U_min(le, lt-le);

   vecswap2(a, 0, lt-r, r);

   r = U_min(ge-gt, n-ge-1);

   vecswap2(a, lt, n-r, r);

   mksort(a, lt-le, depth);

   if (v != 0) mksort(a + lt-le, le + n-ge-1, depth+1);
               mksort(a + n-(ge-gt),   ge-gt, depth); 
}
*/

/* Faster version */

static inline void vecswap2(UStringRep** a, UStringRep** b, int n)
{
   U_TRACE(0, "vecswap2(%p,%p,%d)", a, b, n)

   UStringRep* t;

   while (n-- > 0)
      {
      t    = *a;
      *a++ = *b;
      *b++ = t;
      }
}

#define ch(a) chfunc(a, 0, depth)

#define med3(a, b, c) med3func(a, b, c, depth)

#define swap2(a,b) { t = *(a); *(a) = *(b); *(b) = t; }

static inline UStringRep** med3func(UStringRep** a, UStringRep** b, UStringRep** c, int depth)
{
   U_TRACE(0, "med3func(%p,%p,%p,%d)", a, b, c, depth)

   int va, vb, vc;
   UStringRep** result;

   if ((va = ch(a)) == (vb = ch(b)))   U_RETURN_POINTER(a, UStringRep*);
   if ((vc = ch(c)) == va || vc == vb) U_RETURN_POINTER(c, UStringRep*);

   result = (va < vb ? (vb < vc ? b : (va < vc ? c : a))
                     : (vb > vc ? b : (va < vc ? a : c)));

   U_RETURN_POINTER(result, UStringRep*);
}

static inline void inssort(UStringRep** a, int n, int depth)
{
   U_TRACE(0, "inssort(%p,%d,%d)", a, n, depth)

   UStringRep** pi, **pj, *t;

   for (pi = a + 1; --n > 0; ++pi)
      {
      for (pj = pi; pj > a; --pj)
         {
         // Inline strcmp: break if *(pj-1) <= *pj

         if ((*(pj-1))->compare(*pj, depth) <= 0) break;

         swap2(pj, pj-1);
         }
      }
}

void UVector<UString>::mksort(UStringRep** a, int n, int depth)
{
   U_TRACE(0+256, "UVector<UString>::mksort(%p,%d,%d)", a, n, depth)

   int d, r, partval;
   UStringRep** pa, **pb, **pc, **pd, **pl, **pm, **pn, *t;

   if (n <= 10) // insertion sort
      {
      inssort(a, n, depth);

      return;
      }

   pl = a;
   pm = a + (n / 2);
   pn = a + (n - 1);

   if (n > 50)
      {
      // On big arrays, pseudomedian of 9

      d  = (n / 8);
      pl = med3(pl,         pl + d, pl + 2 * d);
      pm = med3(pm - d,     pm,     pm + d);
      pn = med3(pn - 2 * d, pn - d, pn);
      }

   pm = med3(pl, pm, pn);

   swap2(a, pm);

   partval = ch(a);

   pa = pb = a + 1;
   pc = pd = a + n - 1;

   for (;;)
      {
      while (pb <= pc &&
             (r = ch(pb) - partval) <= 0)
         {
         if (r == 0)
            {
            swap2(pa, pb);

            ++pa;
            }

         ++pb;
         }

      while (pb <= pc &&
             (r = ch(pc) - partval) >= 0)
         {
         if (r == 0)
            {
            swap2(pc, pd);

            --pd;
            }

         --pc;
         }

      if (pb > pc) break;

      swap2(pb, pc);

      ++pb;
      --pc;
      }

   pn = a + n;
   r  = U_min(pa - a,  pb - pa);     vecswap2(a,  pb - r, r);
   r  = U_min(pd - pc, pn - pd - 1); vecswap2(pb, pn - r, r);

   if ((r = pb - pa)  > 1) mksort(a, r, depth);
   if (ch(a+r)       != 0) mksort(a + r, pa - a + pn - pd - 1, depth + 1);
   if ((r = pd - pc)  > 1) mksort(a + n - r, r, depth);
}

// STREAMS

uint32_t UVector<UString>::readline(istream& is)
{
   U_TRACE(0, "UVector<UString>::readline(%p)", &is)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_MAJOR(_capacity,0)

   uint32_t n = _length;
   UString str(U_CAPACITY);

   while (is.good())
      {
      str.getline(is);

      if (str.empty() == false) push(str);
      }

   U_RETURN(_length - n);
}

U_EXPORT istream& operator>>(istream& is, UVector<UString>& v)
{
   U_TRACE(0+256,"UVector<UString>::operator>>(%p,%p)", &is, &v)

   U_INTERNAL_ASSERT_MAJOR(v._capacity,0)
   U_INTERNAL_ASSERT(is.peek() == '[' || is.peek() == '(')

   int c = EOF;

   if (is.good())
      {
      streambuf* sb = is.rdbuf();

      UString str(U_CAPACITY);

      c = sb->sbumpc(); // skip '[' or '('

      while (c != EOF)
         {
         do { c = sb->sbumpc(); } while (u_isspace(c) && c != EOF); // skip white-space

      // U_INTERNAL_DUMP("c = %C", c)

         if (c == ']' ||
             c == ')' ||
             c == EOF) break;

         if (c == '#')
            {
            do { c = sb->sbumpc(); } while (c != '\n' && c != EOF); // skip line comment

            continue;
            }

         U_INTERNAL_ASSERT_EQUALS(u_isspace(c),false)

         sb->sputbackc(c);

         str.get(is);

      // U_INTERNAL_ASSERT_EQUALS(str.empty(),false) (per file configurazione con elementi posizionali...)

         v.push(str);
         }
      }

   if (c == EOF) is.setstate(ios::eofbit);

// NB: we can load an empty vector (ex. mod_http)...
// -------------------------------------------------
// if (v._length == 0) is.setstate(ios::failbit);
// -------------------------------------------------

   return is;
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UVector<void*>::dump(bool reset) const
{
   *UObjectIO::os << "vec       " << (void*)vec << '\n'
                  << "_length   " << _length    << '\n'
                  << "_capacity " << _capacity;

                  /*
                  << "tail      " << tail       << '\n'
                  << "head      " << head       << '\n'
                  */

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
