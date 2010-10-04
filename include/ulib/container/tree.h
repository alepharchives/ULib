// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    tree.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_TREE_H
#define ULIB_TREE_H 1

#include <ulib/container/vector.h>

template <class T> class UTree;

template <> class U_EXPORT UTree<void*> : public UVector<void*> {
public:

   // Costruttori e distruttore

   UTree(void* __elem = 0, void* __parent = 0, uint32_t n = 0) : UVector<void*>(n)
      {
      U_TRACE_REGISTER_OBJECT(0, UTree<void*>, "%p,%p,%u", __elem, __parent, n)

      _elem   = __elem;
      _parent = __parent;
      }

   ~UTree()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UTree<void*>)
      }

   // ACCESS

   void* elem() const { return _elem; }

   UTree<void*>*   parent() const               { return (UTree<void*>*)_parent; }
   UVector<void*>* vector() const               { return (UVector<void*>*)this; }
   UTree<void*>*   childAt(uint32_t pos) const  { return (UTree<void*>*)vector()->at(pos); }

   // SERVICES

   bool null() const
      {
      U_TRACE(0, "UTree<void*>::null()")

      U_RETURN(_elem == 0);
      }

   bool root() const
      {
      U_TRACE(0, "UTree<void*>::root()")

      U_RETURN(_parent == 0);
      }

   bool empty() const
      {
      U_TRACE(0, "UTree<void*>::empty()")

      U_RETURN(_elem == 0 && _length == 0);
      }

   uint32_t numChild() const
      {
      U_TRACE(0, "UTree<void*>::numChild()")

      U_RETURN(_length);
      }

   // compute the depth to the root

   uint32_t depth() const
      {
      U_TRACE(0, "UTree<void*>::depth()")

      uint32_t result = 0;
      const UTree<void*>* p = this;

      while (p->parent() != 0)
         {
         ++result;

         p = p->parent();
         }

      U_RETURN(result);
      }

   // OPERATIONS

   void setRoot(void* __elem)
      {
      U_TRACE(0, "UTree<void*>::setRoot(%p)", __elem)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_EQUALS(_parent, 0)

      _elem = __elem;
      }

   void setParent(void* __parent)
      {
      U_TRACE(0, "UTree<void*>::setParent(%p)", __parent)

      U_CHECK_MEMORY

      _parent = __parent;
      }

   // STACK

   UTree<void*>* push(void* __elem) // add to end
      {
      U_TRACE(0, "UTree<void*>::push(%p)", __elem)

      UTree<void*>* p = U_NEW(UTree<void*>(__elem, this, (size_allocate ? : 64)));

      UVector<void*>::push(p);

      U_RETURN_POINTER(p,UTree<void*>);
      }

   UTree<void*>* push_back(void* __elem)
      {
      U_TRACE(0, "UTree<void*>::push_back(%p)", __elem)

      if (_parent == 0 &&
          _elem   == 0)
         {
         _elem = __elem;

         return this;
         }
      else
         {
         return push(__elem);
         }
      }

   UTree<void*>* top() // return last element
      {
      U_TRACE(0, "UTree<void*>::top()")

      return (UTree<void*>*) UVector<void*>::top();
      }

   UTree<void*>* pop() // remove last element
      {
      U_TRACE(0, "UTree<void*>::pop()")

      return (UTree<void*>*) UVector<void*>::pop();
      }

   // LIST

   UTree<void*>* insert(uint32_t pos, void* __elem) // add elem before pos
      {
      U_TRACE(0, "UTree<void*>::insert(%u,%p)", pos, __elem)

      UTree<void*>* p = U_NEW(UTree<void*>(__elem, this));

      UVector<void*>::insert(pos, p);

      U_RETURN_POINTER(p,UTree<void*>);
      }

   void erase(uint32_t pos) // remove element at pos
      {
      U_TRACE(0, "UTree<void*>::erase(%u)", pos)

      delete (UTree<void*>*) vec[pos];

      UVector<void*>::erase(pos);
      }

   // Call function for all entry

   void callForAllEntry(vPFpvpv function);

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

   static uint32_t size_allocate;

protected:
   void* _elem;
   void* _parent;

private:
   UTree<void*>(const UTree<void*>&) : UVector<void*>() {}
   UTree<void*>& operator=(const UTree<void*>&)         { return *this; }
};

template <class T> class U_EXPORT UTree<T*> : public UTree<void*> {
public:

   void clear() // erase all element
      {
      U_TRACE(0, "UTree<T*>::clear()")

      if (_elem)
         {
         u_destroy<T>((T*)_elem);

         _elem = 0;
         }

      if (UVector<void*>::empty() == false)
         {
         void** _end = vec + _length;

         for (void** ptr = vec; ptr < _end; ++ptr) delete (UTree<T*>*)(*ptr);

         _length = 0;
         }
      }

   // Costruttori e distruttore

   UTree(T* __elem = 0, T* __parent = 0, uint32_t n = 0) : UTree<void*>(__elem, __parent, n)
      {
      U_TRACE_REGISTER_OBJECT(0, UTree<T*>, "%p,%p,%u", __elem, __parent, n)
      }

   ~UTree()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UTree<T*>)

      clear();
      }

   // ACCESS

   T* elem() const { return (T*) _elem; }

   UTree<T*>*   parent() const               { return (UTree<T*>*)_parent; }
   UVector<T*>* vector() const               { return (UVector<T*>*)this; }
   UTree<T*>*   childAt(uint32_t pos) const  { return (UTree<T*>*)((UVector<void*>*)this)->at(pos); }

   T* begin()  { return ((UTree<T*>*)UVector<void*>::begin())->elem(); }
   T* end()    { return ((UTree<T*>*)UVector<void*>::end())->elem(); }
   T* rbegin() { return ((UTree<T*>*)UVector<void*>::rbegin())->elem(); }
   T* rend()   { return ((UTree<T*>*)UVector<void*>::rend())->elem(); }
   T* front()  { return ((UTree<T*>*)UVector<void*>::front())->elem(); }
   T* back()   { return ((UTree<T*>*)UVector<void*>::back())->elem(); }

   T* at(uint32_t pos) const { return ((UTree<T*>*)UVector<void*>::at(pos))->elem(); }

   T* operator[](uint32_t pos) const { return at(pos); }

   // OPERATIONS

   void setRoot(T* __elem)
      {
      U_TRACE(0, "UTree<T*>::setRoot(%p)", __elem)

      u_construct<T>(__elem);

      UTree<void*>::setRoot(__elem);
      }

   // STACK

   UTree<T*>* push(T* __elem) // add to end
      {
      U_TRACE(0, "UTree<T*>::push(%p)", __elem)

      u_construct<T>(__elem);

      return (UTree<T*>*) UTree<void*>::push(__elem);
      }

   UTree<T*>* push_back(T* __elem)
      {
      U_TRACE(0, "UTree<T*>::push_back(%p)", __elem)

      if (_parent == 0 &&
          _elem   == 0)
         {
         setRoot(__elem);

         return this;
         }
      else
         {
         return push(__elem);
         }
      }

   UTree<T*>* top() // return last element
      {
      U_TRACE(0, "UTree<T*>::top()")

      return (UTree<T*>*) UVector<void*>::top();
      }

   UTree<T*>* pop() // remove last element
      {
      U_TRACE(0, "UTree<T*>::pop()")

      return (UTree<T*>*) UVector<void*>::pop();
      }

   // LIST

   UTree<T*>* insert(uint32_t pos, T* __elem) // add elem before pos
      {
      U_TRACE(0, "UTree<T*>::insert(%u,%p)", pos, __elem)

      u_construct<T>(__elem);

      return (UTree<T*>*) UTree<void*>::insert(pos, __elem);
      }

   void erase(uint32_t pos) // remove element at pos
      {
      U_TRACE(0, "UTree<T*>::erase(%u)", pos)

      delete (UTree<T*>*) vec[pos];

      UVector<void*>::erase(pos);
      }

   // STREAMS

   friend ostream& operator<<(ostream& os, const UTree<T*>& t)
      {
      U_TRACE(0+256, "UTree<T*>::operator<<(%p,%p)", &os, &t)

      for (uint32_t i = 0; i < t.depth(); ++i)
         {
         os.put('\t');
         }

      os.put('[');

      if (t.null() == false)
         {
         os.put(' ');

         os << *(t.elem());
         }

      if (t.UVector<void*>::empty() == false)
         {
         UTree<T*>* p;

         for (void** ptr = t.vec; ptr < (t.vec + t._length); ++ptr)
            {
            p = (UTree<T*>*)(*ptr);

            os.put('\n');

            os << *p;
            }
         }

      os.put(' ');
      os.put(']');

      return os;
      }

#ifdef DEBUG
   const char* dump(bool reset) const { return UTree<void*>::dump(reset); }
#endif

private:
   UTree<T*>(const UTree<T*>&)            {}
   UTree<T*>& operator=(const UTree<T*>&) { return *this; }
};

// specializzazione stringa

template <> class U_EXPORT UTree<UString> : public UTree<UStringRep*> {
public:

   // Costruttori e distruttore

   UTree(uint32_t n = 64) : UTree<UStringRep*>(0, 0, n)
      {
      U_TRACE_REGISTER_OBJECT(0, UTree<UString>, "%u", n)
      }

   ~UTree()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UTree<UString>)
      }

   // ACCESS

   UString elem() const
      {
      U_TRACE(0, "UTree<UString>::elem()")

      if (_elem)
         {
         UString str((UStringRep*)_elem);

         U_RETURN_STRING(str);
         }

      U_RETURN_STRING(UString::getStringNull());
      }

   UTree<UString>*   parent() const                { return (UTree<UString>*)_parent; }
   UVector<UString>* vector() const                { return (UVector<UString>*)this; }
   UTree<UString>*   childAt(uint32_t pos) const   { return (UTree<UString>*)((UVector<void*>*)this)->at(pos); }

   UString begin()  { return ((UTree<UString>*)UVector<void*>::begin())->elem(); }
   UString end()    { return ((UTree<UString>*)UVector<void*>::end())->elem(); }
   UString rbegin() { return ((UTree<UString>*)UVector<void*>::rbegin())->elem(); }
   UString rend()   { return ((UTree<UString>*)UVector<void*>::rend())->elem(); }
   UString front()  { return ((UTree<UString>*)UVector<void*>::front())->elem(); }
   UString back()   { return ((UTree<UString>*)UVector<void*>::back())->elem(); }

   UString at(uint32_t pos) const { return ((UTree<UString>*)UVector<void*>::at(pos))->elem(); }

   UString operator[](uint32_t pos) const { return at(pos); }

   // OPERATIONS

   void setRoot(const UString& str)
      {
      U_TRACE(0, "UTree<UString>::setRoot(%.*S)", U_STRING_TO_TRACE(str))

      UTree<UStringRep*>::setRoot(str.rep);
      }

   // STACK

   UTree<UString>* push(const UString& str)
      {
      U_TRACE(0, "UTree<UString>::push(%.*S)", U_STRING_TO_TRACE(str))

      return (UTree<UString>*) UTree<UStringRep*>::push(str.rep);
      }

   UTree<UString>* push_back(const UString& str)
      {
      U_TRACE(0, "UTree<UString>::push_back(%.*S)", U_STRING_TO_TRACE(str))

      return (UTree<UString>*) UTree<UStringRep*>::push_back(str.rep);
      }

   UTree<UString>* top()
      {
      U_TRACE(0, "UTree<UString>::top()")

      return (UTree<UString>*) UTree<UStringRep*>::top();
      }

   UTree<UString>* pop()
      {
      U_TRACE(0, "UTree<UString>::pop()")

      return (UTree<UString>*) UTree<UStringRep*>::pop();
      }

   // LIST

   void insert(uint32_t pos, const UString& str) // add elem before pos
      {
      U_TRACE(0, "UTree<UString>::insert(%u,%.*S)", pos, U_STRING_TO_TRACE(str))

      UTree<UStringRep*>::insert(pos, str.rep);
      }

   // EXTENSION

   uint32_t find(const UString& str);

   // STREAMS

   friend U_EXPORT istream& operator>>(istream& is, UTree<UString>& t);

   friend ostream& operator<<(ostream& os, const UTree<UString>& t)
      { return operator<<(os, (const UTree<UStringRep*>&)t); }

private:
   UTree<UString>(const UTree<UString>&) : UTree<UStringRep*>() {}
   UTree<UString>& operator=(const UTree<UString>&)             { return *this; }
};

#endif
