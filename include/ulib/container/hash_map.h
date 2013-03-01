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
                   class UHTTP;
                   class UValue;
template <class T> class UVector;
                   class USSLSession;
                   class UDataSession;

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

   UHashMapNode(UStringRep* _key, void* _elem, UHashMapNode* _next, uint32_t _hash) : elem(_elem), key(_key), next(_next), hash(_hash)
      {
      U_TRACE_REGISTER_OBJECT(0, UHashMapNode, "%.*S,%p,%p,%u", U_STRING_TO_TRACE(*_key), _elem, _next, _hash)

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

   UHashMapNode& operator=(const UHashMapNode& n)
      {
      U_TRACE(0, "UHashMapNode::operator=(%p)", &n)

      U_MEMORY_TEST_COPY(n)

      elem = n.elem;
      key  = n.key;
      next = n.next;
      hash = n.hash;

      return *this;
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

      _length = _capacity = _space = 0;

      ignore_case = _ignore_case;
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

      table     = (UHashMapNode**) UMemoryPool::_malloc(&n, sizeof(UHashMapNode*), true);
      _capacity = n;
      }

   void deallocate()
      {
      U_TRACE(0, "UHashMap<void*>::deallocate()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(_capacity, 0)

      UMemoryPool::_free(table, _capacity, sizeof(UHashMapNode*));

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

   bool ignoreCase() const  { return ignore_case; }

   // Ricerche

   bool find(const UString& _key)
      {
      U_TRACE(0, "UHashMap<void*>::find(%.*S)", U_STRING_TO_TRACE(_key))

      lookup(_key);

      U_RETURN(node != NULL);
      }

   // Set/get methods

   void* operator[](const char*    _key);
   void* operator[](UStringRep*    _key) { return at(_key); }
   void* operator[](const UString& _key) { return at(_key.rep); }

   void*       elem() const { return node->elem; }
   UStringRep*  key() const { return node->key; }

   template <typename T> T* get(const UString& _key)
      {
      U_TRACE(0, "UHashMap<void*>::get(%.*S)", U_STRING_TO_TRACE(_key))

      return (T*) operator[](_key);
      }

   // Sets a field, overwriting any existing value

   void insert(const UString& _key, void* _elem)
      {
      U_TRACE(0, "UHashMap<void*>::insert(%.*S,%p)", U_STRING_TO_TRACE(_key), _elem)

      lookup(_key);

      insertAfterFind(_key, _elem);
      }

   // dopo avere chiamato find() (non effettuano il lookup)

   void insertAfterFind(UStringRep*    _key, void* _elem);
   void insertAfterFind(const UString& _key, void* _elem) { insertAfterFind(_key.rep, _elem); }

   void   eraseAfterFind();
   void replaceAfterFind(void* _elem)
      {
      U_TRACE(0, "UHashMap<void*>::replaceAfterFind(%p)", _elem)

      node->elem = _elem;
      }

   void replaceKey(const UString& key);

   void* erase(const char*    _key);
   void* erase(UStringRep*    _key);
   void* erase(const UString& _key) { return erase(_key.rep); }

   // Make room for a total of n element

   void reserve(uint32_t n);

   // Call function for all entry

   bool next();
   bool first();

   void getKeys(UVector<UString>& vec);

   void callForAllEntry(vPFprpv function);
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

   static UStringRep* pkey;
   static bool stop_call_for_all_entry;

   // Find a elem in the array with <key>

   void* at(UStringRep* keyr);
   void  lookup(UStringRep* keyr);
   void  lookup(const UString& _key) { return lookup(_key.rep); }

   void _eraseAfterFind();
   void _callForAllEntrySorted(vPFprpv function);

private:
   UHashMap<void*>(const UHashMap<void*>&)            {}
   UHashMap<void*>& operator=(const UHashMap<void*>&) { return *this; }

   friend class UCDB;
   friend class UHTTP;
   friend class UValue;
   friend class USSLSession;

   friend void ULib_init();
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

   T* erase(const char*    _key) { return (T*) UHashMap<void*>::erase(_key); }
   T* erase(UStringRep*    _key) { return (T*) UHashMap<void*>::erase(_key); }
   T* erase(const UString& _key) { return (T*) UHashMap<void*>::erase(_key.rep); }

   T* elem() const { return (T*) UHashMap<void*>::elem(); }

   T* operator[](const char*    _key) { return (T*) UHashMap<void*>::operator[](_key); }
   T* operator[](UStringRep*    _key) { return (T*) UHashMap<void*>::operator[](_key); }
   T* operator[](const UString& _key) { return (T*) UHashMap<void*>::operator[](_key); }

   void eraseAfterFind()
      {
      U_TRACE(0, "UHashMap<T*>::eraseAfterFind()")

      U_INTERNAL_ASSERT_POINTER(node)

      u_destroy<T>((T*)node->elem);

      UHashMap<void*>::eraseAfterFind();
      }

   void insertAfterFind(UStringRep* _key, T* _elem)
      {
      U_TRACE(0, "UHashMap<T*>::insertAfterFind(%.*S,%p)", U_STRING_TO_TRACE(*_key), _elem)

      u_construct<T>(&_elem);

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

   void insertAfterFind(const UString& _key, T* _elem) { insertAfterFind(_key.rep, _elem); }

   void replaceAfterFind(T* _elem)
      {
      U_TRACE(0, "UHashMap<T*>::replaceAfterFind(%p)", _elem)

      U_INTERNAL_ASSERT_POINTER(node)

      u_construct<T>(&_elem);
        u_destroy<T>((T*)node->elem);

      UHashMap<void*>::replaceAfterFind(_elem);
      }

   // Sets a field, overwriting any existing value

   void insert(const UString& _key, T* _elem)
      {
      U_TRACE(0, "UHashMap<T*>::insert(%.*S,%p)", U_STRING_TO_TRACE(_key), _elem)

      UHashMap<void*>::lookup(_key);

      insertAfterFind(_key, _elem);
      }

#ifdef DEBUG
   bool check_memory() const // check all element
      {
      U_TRACE(0, "UHashMap<T*>::check_memory()")

      U_CHECK_MEMORY

      U_INTERNAL_DUMP("_length = %u", _length)

      if (_length)
         {
         T* _elem;
         UHashMapNode* _node;
         UHashMapNode* _next;
         int sum = 0, max = 0, min = 1024, width;

         for (uint32_t _index = 0; _index < _capacity; ++_index)
            {
            _node = table[_index];

            if (_node)
               {
               ++sum;
               width = -1;

               do {
                  ++width;

                  _next  =     _node->next;
                  _elem = (T*) _node->elem;

                  U_INTERNAL_DUMP("_elem = %p", _elem)

                  U_CHECK_MEMORY_OBJECT(_elem)
                  }
               while ((_node = _next));

               if (max < width) max = width;
               if (min > width) min = width;
               }
            }

         U_INTERNAL_DUMP("collision(min,max) = (%d,%d) - distribution = %f", min, max, (double)_length / (double)sum)
         }

      U_RETURN(true);
      }
#endif

   // Cancellazione tabella

   void clear()
      {
      U_TRACE(0+256, "UHashMap<T*>::clear()")

      U_INTERNAL_ASSERT(check_memory())

      U_INTERNAL_DUMP("_length = %u", _length)

      if (_length)
         {
         T* _elem;
         UHashMapNode* _next;

         for (index = 0; index < _capacity; ++index)
            {
            if (table[index])
               {
               node = table[index];

               do {
                  _next  =     node->next;
                  _elem = (T*) node->elem;

                  u_destroy<T>(_elem);

                  delete node;
                  }
               while ((node = _next));

               table[index] = 0;
               }
            }

         _length = 0;
         }
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

   void assign(UHashMap<T*>& t)
      {
      U_TRACE(0, "UHashMap<T*>::assign(%p)", &t)

      U_INTERNAL_ASSERT_EQUALS(gperf,0)
      U_INTERNAL_ASSERT_DIFFERS(this, &t)
      U_INTERNAL_ASSERT_EQUALS(ignore_case, t.ignore_case)

      clear();

      if (t._length)
         {
         T* _elem;
         UHashMapNode** ptr;

         allocate(t._capacity);

         for (index = 0; index < t._capacity; ++index)
            {
            node = t.table[index];

            if (node)
               {
               ptr = table+index;

               U_INTERNAL_ASSERT_EQUALS(*ptr, 0)

               do {
                  *ptr = U_NEW(UHashMapNode(node, *ptr)); // lo si inserisce nella lista collisioni...

                  _elem = (T*) (*ptr)->elem;

                  U_INTERNAL_DUMP("_elem = %p", _elem)

                  U_ASSERT_EQUALS(_elem, t[node->key])

                  u_construct<T>(&_elem);
                  }
               while ((node = node->next));
               }
            }

         _length = t._length;
         }

      U_INTERNAL_DUMP("_length = %u", _length)

      U_INTERNAL_ASSERT_EQUALS(_length, t._length)
      }

   // STREAMS

   friend istream& operator>>(istream& is, UHashMap<T*>& t)
      {
      U_TRACE(0+256, "UHashMap<T*>::operator>>(%p,%p)", &is, &t)

      U_INTERNAL_ASSERT_MAJOR(t._capacity, 0)

      int c = EOF;

      if (is.good())
         {
         T* _elem = U_NEW(T);
         UString key(U_CAPACITY);

         streambuf* sb = is.rdbuf();

         // NB: we need this way for plugin...

         int terminator = EOF;

         if (is.peek() == '{' ||
             is.peek() == '[')
            {
            c = sb->sbumpc(); // skip '{' or '['

            terminator = (c == '{' ? '}' : ']');
            }

         do {
            do { c = sb->sbumpc(); } while (u__isspace(c) && c != EOF); // skip white-space

         // U_INTERNAL_DUMP("c = %C", c)

            if (terminator == c) break;

            if (terminator == EOF &&
                (c == '}' || c == ']'))
               {
               break;
               }

            if (c == EOF) break;

            if (c == '#')
               {
               do { c = sb->sbumpc(); } while (c != '\n' && c != EOF); // skip line comment

               continue;
               }

            U_INTERNAL_ASSERT_EQUALS(u__isspace(c),false)

            sb->sputbackc(c);

            key.get(is);

            U_INTERNAL_ASSERT(key)
            U_INTERNAL_ASSERT(key.isNullTerminated())

            do { c = sb->sbumpc(); } while (u__isspace(c) && c != EOF); // skip white-space

         // U_INTERNAL_DUMP("c = %C", c)

            if (c == EOF) break;

            U_INTERNAL_ASSERT_EQUALS(u__isspace(c),false)

            sb->sputbackc(c);

            is >> *_elem;

            if (is.bad()) is.clear();
            else          t.insert(key, _elem);
            }
         while (c != EOF);

         u_destroy<T>(_elem);
         }

      if (c == EOF) is.setstate(ios::eofbit);

   // NB: we can load an empty table
   // -------------------------------------------------
   // if (t._length == 0) is.setstate(ios::failbit);
   // -------------------------------------------------

      return is;
      }

   friend ostream& operator<<(ostream& _os, const UHashMap<T*>& t)
      {
      U_TRACE(0+256, "UHashMap<T*>::operator<<(%p,%p)", &_os, &t)

      U_INTERNAL_ASSERT(t.check_memory())

      U_INTERNAL_DUMP("t._length = %u", t._length)

      UHashMapNode* node;
      UHashMapNode* next;

      _os.put('[');
      _os.put('\n');

      for (UHashMapNode** ptr = t.table; ptr < (t.table + t._capacity); ++ptr)
         {
         if (*ptr)
            {
            node = *ptr;

            do {
               next = node->next;

               node->key->write(_os);

               _os.put('\t');

               _os << *((T*)node->elem);

               _os.put('\n');
               }
            while ((node = next));
            }
         }

      _os.put(']');

      return _os;
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

   void insertAfterFind(const UString& key, const UString& str);

   // OPERATOR []

   UString operator[](const char*    _key);
   UString operator[](UStringRep*    _key) { return at(_key);     }
   UString operator[](const UString& _key) { return at(_key.rep); }

   // STREAMS

   friend U_EXPORT istream& operator>>(istream& is,       UHashMap<UString>& t);
   friend U_EXPORT ostream& operator<<(ostream& os, const UHashMap<UString>& t) { return operator<<(os, (const UHashMap<UStringRep*>&)t); }

protected:
   UString at(UStringRep* keyr);
   UString at(const char* _key, uint32_t keylen);

   // storage session

   static UHashMap<UString>* fromStream(istream& is);
   static UHashMap<UString>* duplicate(UHashMap<UString>* t);

private:
   UHashMap<UString>(const UHashMap<UString>&) : UHashMap<UStringRep*>() {}
   UHashMap<UString>& operator=(const UHashMap<UString>&)                { return *this; }

   friend class UHTTP;
   friend class UDataSession;
};

#endif
