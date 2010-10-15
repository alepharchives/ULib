// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    vector.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_VECTOR_H
#define ULIB_VECTOR_H 1

#include <ulib/container/construct.h>

// #define U_RING_BUFFER

/**
  Simple vector template class. Supports pushing at end and random-access deletions. Dynamically sized.
*/

template <class T> class UVector;

template <> class U_EXPORT UVector<void*> {
public:
   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // allocate and deallocate methods

   void allocate(uint32_t size);

   void deallocate()
      {
      U_TRACE(0, "UVector<void*>::deallocate()")

      U_CHECK_MEMORY

      if (_capacity) U_FREE_VECTOR(vec, _capacity, void);
      }

   // Costruttori e distruttore

   UVector(uint32_t n = 64U) // create an empty vector with a size estimate
      {
      U_TRACE_REGISTER_OBJECT(0, UVector<void*>, "%u", n)

      allocate(n);

      _length = 0;
      }

   ~UVector()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UVector<void*>)

      deallocate();
      }

   // Size and Capacity

   uint32_t size() const
      {
      U_TRACE(0, "UVector<void*>::size()")

      U_RETURN(_length);
      }

   uint32_t capacity() const
      {
      U_TRACE(0, "UVector<void*>::capacity()")

      U_RETURN(_capacity);
      }

   bool empty() const
      {
      U_TRACE(0, "UVector<void*>::empty()")

      U_RETURN(_length == 0);
      }

   // Make room for a total of n element

   void reserve(uint32_t n);

   // ELEMENT ACCESS

   void* begin()  { return *vec; }
   void* end()    { return *(vec + _length); }
   void* rbegin() { return *(vec + _length - 1); }
   void* rend()   { return *(vec + 1); }
   void* front()  { return begin(); }
   void* back()   { return rbegin(); }

   void*& at(uint32_t pos)
      {
      U_TRACE(0, "UVector<void*>::at(%u)", pos)

      U_INTERNAL_ASSERT_MAJOR(_capacity, 0)
      U_INTERNAL_ASSERT_MINOR(pos, _length)

      return vec[pos];
      }

   void* at(uint32_t pos) const
      {
      U_TRACE(0, "UVector<void*>::at(%u) const", pos)

      U_INTERNAL_ASSERT_MAJOR(_capacity, 0)
      U_INTERNAL_ASSERT_MINOR(pos, _length)

      return vec[pos];
      }

   void*& operator[](uint32_t pos)       { return at(pos); }
   void*  operator[](uint32_t pos) const { return at(pos); }

   void replace(uint32_t pos, void* elem)
      {
      U_TRACE(0, "UVector<void*>::replace(%u,%p)", pos, elem)

      U_INTERNAL_ASSERT_MAJOR(_capacity, 0)
      U_INTERNAL_ASSERT_MINOR(pos, _length)

      vec[pos] = elem;
      }

   // STACK OPERATIONS

   void push(     void* elem);
   void push_back(void* elem) { push(elem); } // add to end

   void* top() // return last element
      {
      U_TRACE(0, "UVector<void*>::top()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(_length, 0)
      U_INTERNAL_ASSERT_RANGE(1,_length,_capacity)

      return vec[_length-1];
      }

   void* pop() // remove last element
      {
      U_TRACE(0, "UVector<void*>::pop()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(_length, 0)
      U_INTERNAL_ASSERT_RANGE(1,_length,_capacity)

      return vec[--_length];
      }

   // LIST OPERATIONS

   void insert(uint32_t pos,             void* elem); // add           elem before pos
   void insert(uint32_t pos, uint32_t n, void* elem); // add n copy of elem before pos

   void erase(uint32_t pos) // remove element at pos
      {
      U_TRACE(1, "UVector<void*>::erase(%u)", pos)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MINOR(pos, _length)
      U_INTERNAL_ASSERT_RANGE(1,_length,_capacity)

      --_length;

      (void) U_SYSCALL(memmove, "%p,%p,%u", vec + pos, vec + pos + 1, (_length - pos) * sizeof(void*));
      }

   void erase(uint32_t first, uint32_t last) // erase [first,last[
      {
      U_TRACE(1, "UVector<void*>::erase(%u,%u)",  first, last)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT(last <= _length)
      U_INTERNAL_ASSERT_MINOR(first, last)
      U_INTERNAL_ASSERT_MINOR(first, _length)
      U_INTERNAL_ASSERT_RANGE(1,_length,_capacity)

      (void) U_SYSCALL(memmove, "%p,%p,%u", vec + first, vec + last, (_length - last) * sizeof(void*));

      _length = (_length - (last - first));
      }

   // RING BUFFER

#ifdef U_RING_BUFFER
   void init()
      {
      U_TRACE(0, "UVector<void*>::init()")

      _start = _end = 0;
      }

   uint32_t start() const
      {
      U_TRACE(0, "UVector<void*>::start()")

      U_RETURN(_start);
      }

   uint32_t end() const
      {
      U_TRACE(0, "UVector<void*>::end()")

      U_RETURN(_end);
      }

   void resize(uint32_t n)
      {
      U_TRACE(0, "UVector<void*>::resize(%u)", n)

      if (_length == _capacity) _end = _length;

      reserve(n);
      }
#endif

   // BINARY HEAP

   // The binary heap is a heap ordered binary tree. A binary tree allows each node
   // in the tree to have two children. Each node has a value associated with it,
   // called its key. The term `heap ordered' means that no child in the tree has a
   // key greater than the key of its parent. By maintaining heap order in the tree,
   // the root node has the smallest key. Because the heap has simple access to the
   // minimum node, the find_min() operation to takes O(1) time.
   // Because of the binary heaps simplicity, it is possible to maintain it using
   // one dimensional arrays. The root node is located at position 1 in the array.
   // The first child of the root is located at position 2 and the second child at
   // position 3. In general, the children of the node at position i are located at
   // 2*i and 2*i + 1. So the children of the node at position 3 in the array are
   // located at positions 6 and 7. Similarly, the parent of the node at position i
   // is located at i div 2. Note that array entry 0 is unused.

   void* bh_min() const
      {
      U_TRACE(0, "UVector<void*>::bh_min()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(_capacity, 0)
      U_INTERNAL_ASSERT_RANGE(1,_length,_capacity)

      // The item at the top of the binary heap has the minimum key value.

      return vec[1];
      }

   // Call function for all entry

   void callForAllEntry(vPFpv function)
      {
      U_TRACE(0, "UVector<void*>::callForAllEntry(%p)", function)

      U_CHECK_MEMORY

      U_INTERNAL_DUMP("_length = %u", _length)

      for (uint32_t i = 0; i < _length; ++i) function(vec[i]);
      }

   void callForAllEntry(bPFpv function)
      {
      U_TRACE(0, "UVector<void*>::callForAllEntry(%p)", function)

      U_CHECK_MEMORY

      U_INTERNAL_DUMP("_length = %u", _length)

      for (uint32_t i = 0; i < _length && function(vec[i]); ++i) {}
      }

   uint32_t find(void* elem)
      {
      U_TRACE(0, "UVector<void*>::find(%p)", elem)

      U_CHECK_MEMORY

      for (uint32_t i = 0; i < _length; ++i)
         {
         if (vec[i] == elem) U_RETURN(i);
         }

      U_RETURN(U_NOT_FOUND);
      }

   // EXTENSION

   void sort(qcompare compare_obj)
      {
      U_TRACE(0+256, "UVector<void*>::sort(%p)", compare_obj)

      U_INTERNAL_DUMP("_length = %u", _length)

      U_INTERNAL_ASSERT_RANGE(2,_length,_capacity)

      U_SYSCALL_VOID(qsort, "%p,%d,%d,%p", (void*)vec, (size_t)_length, sizeof(void*), compare_obj);
      }

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   void** vec;
   uint32_t _length, _capacity;
#ifdef U_RING_BUFFER
   uint32_t _start, _end;
#endif

private:
   UVector<void*>(const UVector<void*>&)            {}
   UVector<void*>& operator=(const UVector<void*>&) { return *this; }
};

template <class T> class U_EXPORT UVector<T*> : public UVector<void*> {
public:

   void clear() // erase all element
      {
      U_TRACE(0, "UVector<T*>::clear()")

      U_CHECK_MEMORY

      U_INTERNAL_DUMP("_length = %u", _length)

      U_INTERNAL_ASSERT(_length <= _capacity)

      u_destroy<T>((T**)vec, _length);

      _length = 0;
      }

   // Costruttori e distruttore

   UVector(uint32_t n = 64U) : UVector<void*>(n)
      {
      U_TRACE_REGISTER_OBJECT(0, UVector<T*>, "%u", n)
      }

   ~UVector()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UVector<T*>)

      clear();
      }

   // ELEMENT ACCESS

   T* begin()  { return (T*) UVector<void*>::begin(); }
   T* end()    { return (T*) UVector<void*>::end(); }
   T* rbegin() { return (T*) UVector<void*>::rbegin(); }
   T* rend()   { return (T*) UVector<void*>::rend(); }
   T* front()  { return (T*) UVector<void*>::front(); }
   T* back()   { return (T*) UVector<void*>::back(); }

   T*& at(uint32_t pos)       { return (T*&) UVector<void*>::at(pos); }
   T*  at(uint32_t pos) const { return (T*)  UVector<void*>::at(pos); }

   T*& operator[](uint32_t pos)       { return at(pos); }
   T*  operator[](uint32_t pos) const { return at(pos); }

   void replace(uint32_t pos, T* elem)
      {
      U_TRACE(0, "UVector<T*>::replace(%u,%p)", pos, elem)

      u_construct<T>((T*)elem);
      u_destroy<T>((T*)vec[pos]);

      UVector<void*>::replace(pos, elem);
      }

   // STACK OPERATIONS

   void push(T* elem) // add to end
      {
      U_TRACE(0, "UVector<T*>::push(%p)", elem)

      u_construct<T>(elem);

      UVector<void*>::push(elem);
      }

   void push_back(T* elem) { push(elem); } 

   T* top() // return last element
      {
      U_TRACE(0, "UVector<T*>::top()")

      T* elem = (T*) UVector<void*>::top();

      U_RETURN_POINTER(elem,T);
      }

   T* pop() // remove last element
      {
      U_TRACE(0, "UVector<T*>::pop()")

      T* elem = (T*) UVector<void*>::pop();

      U_RETURN_POINTER(elem,T);
      }

   // LIST OPERATIONS

   void insert(uint32_t pos, T* elem) // add elem before pos
      {
      U_TRACE(0, "UVector<T*>::insert(%u,%p)", pos, elem)

      u_construct<T>(elem);

      UVector<void*>::insert(pos, elem);
      }

   void insert(uint32_t pos, uint32_t n, T* elem) // add n copy of elem before pos
      {
      U_TRACE(0, "UVector<T*>::insert(%u,%u,%p)", pos, n, elem)

      u_construct<T>(elem, n);

      UVector<void*>::insert(pos, n, elem);
      }

   void erase(uint32_t pos) // remove element at pos
      {
      U_TRACE(0, "UVector<T*>::erase(%u)", pos)

      u_destroy<T>((T*)vec[pos]);

      UVector<void*>::erase(pos);
      }

   void erase(uint32_t first, uint32_t last) // erase [first,last[
      {
      U_TRACE(0, "UVector<T*>::erase(%u,%u)",  first, last)

      u_destroy<T>((T**)(vec+first), last - first);

      UVector<void*>::erase(first, last);
      }

   // ASSIGNMENTS

   void assign(uint32_t n, T* elem)
      {
      U_TRACE(0, "UVector<T*>::assign(%u,%p)", n, elem)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(n, 0)
      U_INTERNAL_ASSERT(_length <= _capacity)

      u_construct<T>(elem, n);

      u_destroy<T>((T**)vec, U_min(n, _length));

      if (n > _capacity)
         {
         UVector<void*>::deallocate();
         UVector<void*>::allocate(n);
         }

      for (uint32_t i = 0; i < n; ++i) vec[i] = elem;

      _length = n;
      }

   // RING BUFFER

#ifdef U_RING_BUFFER
   int put(T* elem) // queue an element at the end
      {
      U_TRACE(0, "UVector<T*>::put(%p)", elem)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(_capacity, 0)
      U_INTERNAL_ASSERT(_length <= _capacity)

      U_INTERNAL_DUMP("_end = %u", _end)

      u_construct<T>(elem);

      int result;

      if (_length < _capacity)
         {
         result = 0;

         ++_length;
         }
      else
         {
         result = -1;

         u_destroy<T>((T*)vec[_end]);
         }

      vec[_end] = elem;

      _end = (_end + 1) % _capacity;

      U_RETURN(result);
      }

   T* get() // dequeue the element off the front
      {
      U_TRACE(0, "UVector<T*>::get()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(_capacity, 0)
      U_INTERNAL_ASSERT(_length <= _capacity)

      U_INTERNAL_DUMP("_start = %u", _start)

      if (_length)
         {
         --_length;

         T* elem = (T*) vec[_start];

         _start = (_start + 1) % _capacity;

         U_RETURN_POINTER(elem,T);
         }

      U_RETURN_POINTER(0,T);
      }
#endif

   // BINARY HEAP

   T* bh_min() const { return (T*) UVector<void*>::bh_min(); }

   void bh_put(T* elem)
      {
      U_TRACE(0, "UVector<T*>::bh_put(%p)", elem)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(_capacity, 0)
      U_INTERNAL_ASSERT(_length <= _capacity)

      u_construct<T>(elem);

      if (++_length == _capacity) reserve(_capacity * 2);

      // i - insertion point
      // j - parent of i
      // y - parent's entry in the heap.

      T* y;
      uint32_t j;

      // i initially indexes the new entry at the bottom of the heap.

      uint32_t i = _length;

      // Stop if the insertion point reaches the top of the heap.

      while (i >= 2)
         {
         // j indexes the parent of i. y is the parent's entry.

         j = i / 2;
         y = (T*) vec[j];

         // We have the correct insertion point when the item is >= parent
         // Otherwise we move the parent down and insertion point up.

         if (*((T*)elem) >= *y) break;

         vec[i] = y;

         i = j;
         }

      // Insert the new item at the insertion point found.

      vec[i] = elem;
      }

   T* bh_get()
      {
      U_TRACE(0, "UVector<T*>::bh_get()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(_capacity, 0)
      U_INTERNAL_ASSERT(_length <= _capacity)

      if (_length)
         {
         T* elem = bh_min();

         // y - the heap entry of the root.
         // j - the current insertion point for the root.
         // k - the child of the insertion point.
         // z - heap entry of the child of the insertion point.

         T* z;

         // Get the value of the root and initialise the insertion point and child.

         T* y       = (T*) vec[_length--];
         uint32_t j = 1;
         uint32_t k = 2 * 1;

         // sift-up only if there is a child of the insertion point.

         while (k <= _length)
            {
            // Choose the minimum child unless there is only one.

            z = (T*) vec[k];

            if (k < _length)
               {
               if (*z > *((T*) vec[k + 1])) z = (T*) vec[++k];
               }

            // We stop if the insertion point for the root is in the correct place.
            // Otherwise the child goes up and the root goes down. (i.e. swap)

            if (*y <= *z) break;

            vec[j] = z;

            j = k;
            k = 2 * j;
            }

         // Insert the root in the correct place in the heap.

         vec[j] = y;

         U_RETURN_POINTER(elem,T);
         }

      U_RETURN_POINTER(0,T);
      }

   // EXTENSION

   uint32_t find(bPFpv function)
      {
      U_TRACE(0, "UVector<T*>::find(%p)", function)

      U_CHECK_MEMORY

      U_INTERNAL_DUMP("_length = %u", _length)

      T* elem;

      for (uint32_t i = 0; i < _length; ++i)
         {
         elem = at(i);

         if (function(elem)) U_RETURN(i);
         }

      U_RETURN(U_NOT_FOUND);
      }

   // STREAMS

   friend istream& operator>>(istream& is, UVector<T*>& v)
      {
      U_TRACE(0+256,"UVector<T*>::operator>>(%p,%p)", &is, &v)

      U_INTERNAL_ASSERT_MAJOR(v._capacity,0)
      U_INTERNAL_ASSERT(is.peek() == '[' || is.peek() == '(')

      int c = EOF;

      if (is.good())
         {
         streambuf* sb = is.rdbuf();

         T elem;

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

            is >> elem;

            v.push(&elem);
            }
         }

      if (c == EOF) is.setstate(ios::eofbit);

   // NB: we can load an empty vector
   // -------------------------------------------------
   // if (v._length == 0) is.setstate(ios::failbit);
   // -------------------------------------------------

      return is;
      }

   friend ostream& operator<<(ostream& _os, const UVector<T*>& v)
      {
      U_TRACE(0+256, "UVector<T*>::operator<<(%p,%p)", &_os, &v)

      _os.put('(');
      _os.put(' ');

      for (void** ptr = v.vec; ptr < (v.vec + v._length); ++ptr)
         {
         _os << *((T*)(*ptr));

         _os.put(' ');
         }

      _os.put(')');

      return _os;
      }

#ifdef DEBUG
   const char* dump(bool reset) const { return UVector<void*>::dump(reset); }
#endif

private:
   UVector<T*>(const UVector<T*>&)            {}
   UVector<T*>& operator=(const UVector<T*>&) { return *this; }
};

// specializzazione stringa

template <> class U_EXPORT UVector<UString> : public UVector<UStringRep*> {
public:

   // Costruttori e distruttore

   UVector(uint32_t n = 64U);

   UVector(const UString& str, char delim)
      {
      U_TRACE_REGISTER_OBJECT(0, UVector<UString>, "%.*S,%C", U_STRING_TO_TRACE(str), delim)

      (void) split(str, delim);
      }

   UVector(const UString& str, const char* delim = 0);

   ~UVector();

   // ELEMENT ACCESS

   UString begin()  { return UString(UVector<UStringRep*>::begin()); }
   UString end()    { return UString(UVector<UStringRep*>::end()); }
   UString rbegin() { return UString(UVector<UStringRep*>::rbegin()); }
   UString rend()   { return UString(UVector<UStringRep*>::rend()); }
   UString front()  { return UString(UVector<UStringRep*>::front()); }
   UString back()   { return UString(UVector<UStringRep*>::back()); }

   UString at(uint32_t pos) const { return UString(UVector<UStringRep*>::at(pos)); }

   UString operator[](uint32_t pos) const { return at(pos); }

   void replace(uint32_t pos, const UString& str)
      {
      U_TRACE(0, "UVector<UString>::replace(%u,%.*S)", pos, U_STRING_TO_TRACE(str))

      UVector<UStringRep*>::replace(pos, str.rep);
      }

   // STACK OPERATIONS

   void push(     const UString& str);
   void push_back(const UString& str) { push(str); } // add to end

   UString top() // return last element
      {
      U_TRACE(0, "UVector<UString>::top()")

      UStringRep* rep = UVector<UStringRep*>::top();

      U_INTERNAL_ASSERT_POINTER(rep)

      UString str(rep);

      rep->release();

      U_RETURN_STRING(str);
      }

   UString pop() // remove last element
      {
      U_TRACE(0, "UVector<UString>::pop()")

      UStringRep* rep = UVector<UStringRep*>::pop();

      U_INTERNAL_ASSERT_POINTER(rep)

      UString str(rep);

      rep->release();

      U_RETURN_STRING(str);
      }

   // LIST OPERATIONS

   void insert(uint32_t pos, const UString& str) // add elem before pos
      {
      U_TRACE(0, "UVector<UString>::insert(%u,%.*S)", pos, U_STRING_TO_TRACE(str))

      UVector<UStringRep*>::insert(pos, str.rep);
      }

   void insert(uint32_t pos, uint32_t n, const UString& str) // add n copy of elem before pos
      {
      U_TRACE(0, "UVector<UString>::insert(%u,%u,%.*S)", pos, n, U_STRING_TO_TRACE(str))

      UVector<UStringRep*>::insert(pos, n, str.rep);
      }

   // ASSIGNMENTS

   void assign(uint32_t n, const UString& str)
      {
      U_TRACE(0, "UVector<UString>::assign(%u,%p)", n, U_STRING_TO_TRACE(str))

      UVector<UStringRep*>::assign(n, str.rep);
      }

   // RING BUFFER

#ifdef U_RING_BUFFER
   int put(const UString& str) // queue an element at the end
      {
      U_TRACE(0, "UVector<UString>::put(%.*S)", U_STRING_TO_TRACE(str))

      return UVector<UStringRep*>::put(str.rep);
      }

   UString get() // dequeue the element off the front
      {
      U_TRACE(0, "UVector<UString>::get()")

      UStringRep* rep = UVector<UStringRep*>::get();

      if (rep)
         {
         UString str(rep);

         rep->release();

         U_RETURN_STRING(str);
         }

      U_RETURN_STRING(UString::getStringNull());
      }
#endif

   // BINARY HEAP

   UString bh_min() const { return UString(UVector<UStringRep*>::bh_min()); }

   void bh_put(const UString& str)
      {
      U_TRACE(0, "UVector<UString>::bh_put(%.*S)", U_STRING_TO_TRACE(str))

      UVector<UStringRep*>::bh_put(str.rep);
      }

   UString bh_get()
      {
      U_TRACE(0, "UVector<UString>::bh_get()")

      UStringRep* rep = UVector<UStringRep*>::bh_get();

      if (rep)
         {
         UString str(rep);

         rep->release();

         U_RETURN_STRING(str);
         }

      U_RETURN_STRING(UString::getStringNull());
      }

   // EXTENSION

   uint32_t split(const UString& str,       char  delim);
   uint32_t split(const UString& str, const char* delim = 0, bool dup = false);

   UString join(const char* delim = "\n", uint32_t delim_len = 1);

   uint32_t contains(const UString& str,    bool ignore_case = false);
   bool     contains(UVector<UString>& vec, bool ignore_case = false);

   bool isContained(const UString& str, bool ignore_case = false) { return (contains(str, ignore_case) != U_NOT_FOUND); }

   // Check equality with an existing vector object

   bool isEqual(UVector<UString>& _vec, bool ignore_case = false)
      {
      U_TRACE(0, "UVector<UString>::isEqual(%p,%b)", &_vec, ignore_case)

      U_CHECK_MEMORY

      U_INTERNAL_DUMP("_length = %u", _length)

      if (_length != _vec.size()) U_RETURN(false);

      return _isEqual(_vec, ignore_case); 
      }

   static int qscomp(const void* p, const void* q)
      {
      U_TRACE(0, "UVector<UString>::qscomp(%p,%p)", p, q)

      return (*(UStringRep**)p)->comparenocase(*(UStringRep**)q);
      }

   void sort(bool ignore_case = false)
      {
      U_TRACE(0, "UVector<UString>::sort(%b)", ignore_case)

      U_INTERNAL_DUMP("_length = %u", _length)

      U_INTERNAL_ASSERT_RANGE(2,_length,_capacity)

      if (ignore_case) UVector<void*>::sort(UVector<UString>::qscomp);
      else             mksort((UStringRep**)vec, _length, 0);
      }

   uint32_t find(const UString& str, bool ignore_case = false)
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

   uint32_t find(const char* s, uint32_t n, uint32_t offset = 0)
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

   uint32_t findSorted(const UString& str, bool ignore_case = false);

   // AS SET

   void insertAsSet(const UString& str) // no duplicate value
      {
      U_TRACE(0, "UVector<UString>::insertAsSet(%.*S)", U_STRING_TO_TRACE(str))

      if (empty() || find(str) == U_NOT_FOUND) push(str);
      }

   uint32_t intersection(UVector<UString>& set1, UVector<UString>& set2);

   // STREAMS

   uint32_t readline(istream& is);

   friend U_EXPORT istream& operator>>(istream& is,       UVector<UString>& v);
   friend          ostream& operator<<(ostream& os, const UVector<UString>& v) { return operator<<(os, (const UVector<UStringRep*>&)v); }

private:
   static void mksort(UStringRep** a, int n, int depth);
          bool _isEqual(UVector<UString>& vec, bool ignore_case);

   UVector<UString>(const UVector<UString>&) : UVector<UStringRep*>() {}
   UVector<UString>& operator=(const UVector<UString>&)               { return *this; }
};

#endif
