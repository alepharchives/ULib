// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    hash_map.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_HASH_MAP_H
#define ULIB_HASH_MAP_H 1

#include <ulib/container/construct.h>

typedef uint32_t (*uPFpcu) (const char* t, uint32_t tlen);
typedef void     (*vPFprpv)(UStringRep* key, void* elem);
typedef bool     (*bPFprpv)(UStringRep* key, void* elem);

                   class UCDB;
                   class UValue;
template <class T> class UVector;

class U_NO_EXPORT UHashMapNode {
public:
   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   void* elem;
   UStringRep* key;
   UHashMapNode* next;
   uint32_t hash;

   // COSTRUTTORI

   UHashMapNode(const UString& _key, void* _elem, UHashMapNode* _next, uint32_t _hash)
         : elem(_elem), key(_key.rep), next(_next), hash(_hash)
      {
      U_TRACE_REGISTER_OBJECT(0, UHashMapNode, "%.*S,%p,%p,%u", U_STRING_TO_TRACE(_key), _elem, _next, _hash)

      key->hold(); // NB: si incrementa la reference della stringa...
      }

   UHashMapNode(UHashMapNode* n, UHashMapNode* _next) : elem(n->elem), key(n->key), next(_next), hash(n->hash)
      {
      U_TRACE_REGISTER_OBJECT(0, UHashMapNode, "%p,%p", n, _next)

      key->hold(); // NB: si incrementa la reference della stringa...
      }

   ~UHashMapNode()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UHashMapNode)

      key->release(); // NB: si decrementa la reference della stringa...
      }

#ifdef DEBUG
   U_EXPORT const char* dump(bool reset) const;
#endif
};

template <class T> class UHashMap;

template <> class U_EXPORT UHashMap<void*> {
public:
   static uPFpcu gperf;

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // Costruttori e distruttore

   UHashMap(bool _ignore_case = false)
      {
      U_TRACE_REGISTER_OBJECT(0, UHashMap<void*>, "%b", _ignore_case)

      ignore_case = _ignore_case;

      _length = _capacity = _space = 0;
      }

   ~UHashMap()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UHashMap<void*>)
      }

   // allocate and deallocate methods

   void allocate(uint32_t n = 53)
      {
      U_TRACE(0, "UHashMap<void*>::allocate(%u)", n)

      U_CHECK_MEMORY

      table     = U_MALLOC_VECTOR(n, UHashMapNode);
      _capacity = n;

      (void) memset(table, '\0', n * sizeof(UHashMapNode*));
      }

   void deallocate()
      {
      U_TRACE(0, "UHashMap<void*>::deallocate()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(_capacity, 0)

      U_FREE_VECTOR(table, _capacity, UHashMapNode);

      _capacity = 0;
      }

   // Size and Capacity

   uint32_t size() const
      {
      U_TRACE(0, "UHashMap<void*>::size()")

      U_RETURN(_length);
      }

   uint32_t capacity() const
      {
      U_TRACE(0, "UHashMap<void*>::capacity()")

      U_RETURN(_capacity);
      }

   uint32_t space() const
      {
      U_TRACE(0, "UHashMap<void*>::space()")

      U_RETURN(_space);
      }

   void setSpace(uint32_t s)
      {
      U_TRACE(0, "UHashMap<void*>::setSpace(%u)", s)

      _space = s;
      }

   bool empty() const
      {
      U_TRACE(0, "UHashMap<void*>::empty()")

      U_RETURN(_length == 0);
      }

   void setIgnoreCase(bool flag)
      {
      U_TRACE(0, "UHashMap<void*>::setIgnoreCase(%b)", flag)

      U_INTERNAL_ASSERT_EQUALS(_length, 0)

      ignore_case = flag;
      }

   bool ignoreCase() const { return ignore_case; }

   // Ricerche

   bool find(const UString& _key)
      {
      U_TRACE(0, "UHashMap<void*>::find(%.*S)", U_STRING_TO_TRACE(_key))

      lookup(_key);

      U_RETURN(node != NULL);
      }

   // Set/get methods

   void* operator[](const char*    _key) { UStringRep keyr(_key); return at(&keyr); }
   void* operator[](const UString& _key) {                        return at(_key.rep); }

   void*       elem() const { return node->elem; }
   UStringRep*  key() const { return node->key; }

   // dopo avere chiamato find() (non effettuano il lookup)

   void   eraseAfterFind();
   void  insertAfterFind(const UString& key, void*  elem);
   void replaceAfterFind(                    void* _elem)
      {
      U_TRACE(0, "UHashMap<void*>::replaceAfterFind(%p)", _elem)

      node->elem = _elem;
      }

   void* erase(const UString& key);

   // Make room for a total of n element

   void reserve(uint32_t n);

   // Call function for all entry

   bool next();
   bool first();

   void callForAllEntry(vPFprpv function);

   void getKeys(UVector<UString>& vec);

   void callForAllEntrySorted(vPFprpv function)
      {
      U_TRACE(0, "UHashMap<void*>::callForAllEntrySorted(%p)", function)

      U_INTERNAL_DUMP("_length = %u", _length)

      if (_length < 2)
         {
         callForAllEntry(function);

         return;
         }

      _callForAllEntrySorted(function);
      }

   static void stopCallForAllEntry() { stop_call_for_all_entry = true; }

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UHashMapNode* node;
   UHashMapNode** table;
   uint32_t _length, _capacity, _space, index, hash;
   bool ignore_case;

   static bool stop_call_for_all_entry;

   // Find a elem in the array with <key>

   void* at(UStringRep* keyr);
   void  lookup(UStringRep* keyr);
   void  lookup(const UString& _key) { return lookup(_key.rep); }

   void _callForAllEntrySorted(vPFprpv function);

private:
   UHashMap<void*>(const UHashMap<void*>&)            {}
   UHashMap<void*>& operator=(const UHashMap<void*>&) { return *this; }

   friend class UCDB;
   friend class UValue;
};

template <class T> class U_EXPORT UHashMap<T*> : public UHashMap<void*> {
public:

   // Costruttori e distruttore

   UHashMap(bool _ignore_case = false) : UHashMap<void*>(_ignore_case)
      {
      U_TRACE_REGISTER_OBJECT(0, UHashMap<T*>, "%b", _ignore_case)
      }

   ~UHashMap()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UHashMap<T*>)
      }

   // Inserimento e cancellazione elementi dalla tabella

   T* erase(const UString& _key)
      { return (T*) UHashMap<void*>::erase(_key); }

   T* elem() const { return (T*) UHashMap<void*>::elem(); }

   T* operator[](const char*    _key) { return (T*) UHashMap<void*>::operator[](_key); }
   T* operator[](const UString& _key) { return (T*) UHashMap<void*>::operator[](_key); }

   void eraseAfterFind()
      {
      U_TRACE(0, "UHashMap<T*>::eraseAfterFind()")

      U_INTERNAL_ASSERT_POINTER(node)

      u_destroy<T>((T*)node->elem);

      UHashMap<void*>::eraseAfterFind();
      }

   void insertAfterFind(const UString& _key, void* _elem)
      {
      U_TRACE(0, "UHashMap<T*>::insertAfterFind(%.*S,%p)", U_STRING_TO_TRACE(_key), _elem)

      u_construct<T>((T*)_elem);

      if (node)
         {
         u_destroy<T>((T*)node->elem);

         node->elem = _elem;
         }
      else
         {
         UHashMap<void*>::insertAfterFind(_key, _elem);
         }
      }

   void replaceAfterFind(void* _elem)
      {
      U_TRACE(0, "UHashMap<T*>::replaceAfterFind(%p)", _elem)

      U_INTERNAL_ASSERT_POINTER(node)

      u_construct<T>((T*)_elem);
        u_destroy<T>((T*)node->elem);

      UHashMap<void*>::replaceAfterFind(_elem);
      }

   // Sets a field, overwriting any existing value

   void insert(const UString& _key, void* _elem)
      {
      U_TRACE(0, "UHashMap<T*>::insert(%.*S,%p)", U_STRING_TO_TRACE(_key), _elem)

      UHashMap<void*>::lookup(_key);

      insertAfterFind(_key, _elem);
      }

   // Cancellazione tabella

   void clear()
      {
      U_TRACE(0, "UHashMap<T*>::clear()")

      U_INTERNAL_DUMP("_length = %u", _length)

#  ifdef DEBUG
      int sum = 0, max = 0, min = 1024, width;
#  endif

      T* _elem;
      UHashMapNode* _next;

      for (index = 0; index < _capacity; ++index)
         {
         if (table[index])
            {
            node = table[index];

#        ifdef DEBUG
            ++sum;
            width = -1;
#        endif

            do {
#           ifdef DEBUG
               ++width;
#           endif

               _next  =     node->next;
               _elem = (T*) node->elem;

               u_destroy<T>(_elem);

               delete node;
               }
            while ((node = _next));

#        ifdef DEBUG
            if (max < width) max = width;
            if (min > width) min = width;
#        endif

            table[index] = 0;
            }
         }

      U_INTERNAL_DUMP("collision(min,max) = (%d,%d) - distribution = %f", min, max, (double)_length / (double)sum)

      _length = 0;
      }

   void callWithDeleteForAllEntry(bPFprpv function)
      {
      U_TRACE(0, "UHashMap<T*>::callWithDeleteForAllEntry(%p)", function)

      U_INTERNAL_DUMP("_length = %u", _length)

      T* _elem;
      UHashMapNode** ptr;

      for (index = 0; index < _capacity; ++index)
         {
         if (table[index])
            {
            node = table[index];
            ptr  = table + index;

            do {
               if (stop_call_for_all_entry)
                  {
                  stop_call_for_all_entry = false;

                  return;
                  }

               if (function(node->key, node->elem))
                  {
                  *ptr = node->next; // lo si toglie dalla lista collisioni...

                  _elem = (T*) node->elem;

                  u_destroy<T>(_elem);

                  delete node;

                  --_length;

                  continue;
                  }

               ptr = &(*ptr)->next;
               }
            while ((node = *ptr));
            }
         }

      U_INTERNAL_DUMP("_length = %u", _length)
      }

   void assign(UHashMap<T*>& h)
      {
      U_TRACE(0, "UHashMap<T*>::assign(%p)", &h)

      U_INTERNAL_ASSERT_DIFFERS(this, &h)
      U_INTERNAL_ASSERT_EQUALS(gperf, 0)
      U_INTERNAL_ASSERT_EQUALS(ignore_case, h.ignore_case)

      clear();
      allocate(h._capacity);

      T* _elem;
      UHashMapNode** ptr;

      for (uint32_t _index = 0; _index < h._capacity; ++_index)
         {
         if (h.table[_index])
            {
            node = h.table[_index];
            ptr  = table + _index;

            do {
               *ptr = U_NEW(UHashMapNode(node, *ptr)); // lo si inserisce nella lista collisioni...

               _elem = (T*) (*ptr)->elem;

               u_construct<T>(_elem);
               }
            while ((node = node->next));
            }
         }

      _length = h._length;
      }

#ifdef DEBUG
   const char* dump(bool reset) const { return UHashMap<void*>::dump(reset); }
#endif

private:
   UHashMap<T*>(const UHashMap<T*>&)            {}
   UHashMap<T*>& operator=(const UHashMap<T*>&) { return *this; }
};

// specializzazione stringa

template <> class U_EXPORT UHashMap<UString> : public UHashMap<UStringRep*> {
public:

   // Costruttori e distruttore

   UHashMap(bool _ignore_case = false) : UHashMap<UStringRep*>(_ignore_case)
      {
      U_TRACE_REGISTER_OBJECT(0, UHashMap<UString>, "%b", _ignore_case)
      }

   ~UHashMap()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UHashMap<UString>)
      }

   // Inserimento e cancellazione elementi dalla tabella

   void insertAfterFind(const UString& key, const UString& str);

   void replaceAfterFind(const UString& str)
      {
      U_TRACE(0, "UHashMap<T*>::replaceAfterFind(%.*S)", U_STRING_TO_TRACE(str))

      UHashMap<UStringRep*>::replaceAfterFind(str.rep);
      }

   void insert(const UString& _key, const UString& str)
      {
      U_TRACE(0, "UHashMap<UString>::insert(%.*S,%.*S)", U_STRING_TO_TRACE(_key), U_STRING_TO_TRACE(str))

      UHashMap<UStringRep*>::insert(_key, str.rep);

      _space += _key.size() + str.size();
      }

   UString erase(const UString& key);

   // OPERATOR []

   UString operator[](const char*    _key) { UStringRep keyr(_key); return at(&keyr); }
   UString operator[](const UString& _key) {                        return at(_key.rep); }

   // STREAMS

   friend U_EXPORT istream& operator>>(istream& is,       UHashMap<UString>& t);
   friend U_EXPORT ostream& operator<<(ostream& os, const UHashMap<UString>& t);

protected:
   UString at(UStringRep* keyr);

private:
   UHashMap<UString>(const UHashMap<UString>&) : UHashMap<UStringRep*>() {}
   UHashMap<UString>& operator=(const UHashMap<UString>&)                { return *this; }
};

#endif
