// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    gen_hash_map.h - general purpose templated hash table class
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_GENERIC_HASH_MAP_H
#define ULIB_GENERIC_HASH_MAP_H 1

#include <ulib/internal/common.h>

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>
#endif

/**
 * Functor used by UGenericHashMap class to generate a hashcode for an object of type T
 * It must be specialized for your own class
 */

template <typename T> struct UHashCodeFunctor;

/**
 * Functor used by UGenericHashMap class to compare for equality two objects of type T
 * It can be specialized for your own class, by default it simply uses operator==()
 */

template <typename T> struct UEqualsFunctor { bool operator()(const T& a, const T& b) const { return (a == b); } };

/**
 * UGenericHashMap is a general purpose templated hash table class.
 */

template <typename K, typename I,
          typename H = UHashCodeFunctor<K>, typename E = UEqualsFunctor<K> > class UGenericHashMap {
public:
   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

protected:
   struct UGenericHashMapNode // structure for keeping a linked-list of elements
      {
      K key;
      I item;
      uint32_t hash;
      UGenericHashMapNode* next;

      UGenericHashMapNode(const K& _key, const I& _item, UGenericHashMapNode* _next, uint32_t _hash)
         {
         key  = _key;
         item = _item;
         hash = _hash;
         next = _next;
         }
      };

   E equals;
   H hashcode;
   UGenericHashMapNode* node;
   UGenericHashMapNode** table;
   uint32_t _length, _capacity, index, hash;

   // Find a elem in the array with <key>

   template <typename X>
   void lookup(const X& _key)
      {
      U_TRACE(0, "UGenericHashMap<K,I>::lookup(%p)", &_key)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(_capacity,0)

      hash  = hashcode(_key);
      index = hash % _capacity;

      U_INTERNAL_DUMP("index = %u", index)

      U_INTERNAL_ASSERT_MINOR(index,_capacity)

      for (node = table[index]; node; node = node->next)
         {
         if (node->key == _key) break;
         }

      U_INTERNAL_DUMP("node = %p", node)
      }

public:
   // Costruttori e distruttore

   UGenericHashMap()
      {
      U_TRACE_REGISTER_OBJECT(0, UGenericHashMap, "")

      _length = _capacity = 0;
      }

   ~UGenericHashMap()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UGenericHashMap)
      }

   // allocate and deallocate methods

   void allocate(uint32_t n = 53)
      {
      U_TRACE(0, "UGenericHashMap<K,I>::allocate(%u)", n)

      U_CHECK_MEMORY

      table     = (UGenericHashMapNode**) UMemoryPool::_malloc(&n, sizeof(UGenericHashMapNode*), true);
      _capacity = n;
      }

   void deallocate()
      {
      U_TRACE(0, "UGenericHashMap<K,I>::deallocate()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_MAJOR(_capacity,0)

      UMemoryPool::_free(table, _capacity, sizeof(UGenericHashMapNode*));

      _capacity = 0;
      }

   // Size and Capacity

   uint32_t size() const
      {
      U_TRACE(0, "UGenericHashMap<K,I>::size()")

      U_RETURN(_length);
      }

   uint32_t capacity() const
      {
      U_TRACE(0, "UGenericHashMap<K,I>::capacity()")

      U_RETURN(_capacity);
      }

   bool empty() const
      {
      U_TRACE(0, "UGenericHashMap<K,I>::empty()")

      U_RETURN(_length == 0);
      }

   // Ricerche

   template <typename X>
   bool find(const X& _key)
      {
      U_TRACE(0, "UGenericHashMap<K,I>::find(%p)", &_key)

      lookup(_key);

      U_RETURN(node != 0);
      }

   // Set/get methods

   I& elem() { return node->item; }

   template <typename X>
   I& operator[](const X& _key)
      {
      U_TRACE(0, "UGenericHashMap<K,I>::operator[](%p)", &_key)

      lookup(_key);

      U_INTERNAL_ASSERT_POINTER(node)

      return node->item;
      }

   // Explicit method to access the key portion of the current element. The key cannot be modified

   const K& key() const { return node->key; }

   // dopo avere chiamato find() (non effettuano il lookup)

   void eraseAfterFind()
      {
      U_TRACE(0, "UGenericHashMap<K,I>::eraseAfterFind()")

      U_CHECK_MEMORY

      U_INTERNAL_DUMP("node = %p", node)

      UGenericHashMapNode* prev = 0;

      for (UGenericHashMapNode* pnode = table[index]; pnode; pnode = pnode->next)
         {
         if (pnode == node)
            {
            /* lista self-organizing (move-to-front), antepongo l'elemento
             * trovato all'inizio della lista delle collisioni...
             */

            if (prev)
               {
               prev->next   = pnode->next;
               pnode->next  = table[index];
               table[index] = pnode;
               }

            U_INTERNAL_ASSERT_EQUALS(node,table[index])

            break;
            }

         prev = pnode;
         }

      U_INTERNAL_DUMP("prev = %p", prev)

      /* presuppone l'elemento da cancellare all'inizio della lista
      * delle collisioni - lista self-organizing (move-to-front)...
      */

      U_INTERNAL_ASSERT_EQUALS(node,table[index])

      table[index] = node->next;

      delete node;

      --_length;

      U_INTERNAL_DUMP("_length = %u", _length)
      }

   template <typename X, typename Y>
   void insertAfterFind(const X& _key, const Y& _elem)
      {
      U_TRACE(0, "UGenericHashMap<K,I>::insertAfterFind(%p,%p)", &_key, &_elem)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_EQUALS(node,0)

      // antepongo l'elemento all'inizio della lista delle collisioni

      node = table[index] = U_NEW(UGenericHashMapNode(_key, _elem, table[index], hash));

      ++_length;

      U_INTERNAL_DUMP("_length = %u", _length)
      }

   template <typename Y>
   void replaceAfterFind(const Y& _elem)
      {
      U_TRACE(0, "UGenericHashMap<K,I>::replaceAfterFind(%p)", &_elem)

      node->item = _elem;
      }

   template <typename X, typename Y>
   void insert(const X& _key, const Y& _item)
      {
      U_TRACE(0, "UGenericHashMap<K,I>::insert(%p,%p)", &_key, &_item)

      lookup(_key);

      insertAfterFind(_key, _item);
      }

   template <typename X>
   bool erase(const X& _key)
      {
      U_TRACE(0, "UGenericHashMap<K,I>::erase(%p)", &_key)

      lookup(_key);

      if (node)
         {
         eraseAfterFind();

         U_RETURN(true);
         }

      U_RETURN(false);
      }

   // Make room for a total of n element

   void reserve(uint32_t n)
      {
      U_TRACE(0, "UGenericHashMap<K,I>::reserve(%u)", n)

      U_INTERNAL_ASSERT_EQUALS(_capacity,0)

      uint32_t new_capacity = U_GET_NEXT_PRIME_NUMBER(n);

      if (new_capacity == _capacity) return;

      UGenericHashMapNode** old_table = table;
      uint32_t           old_capacity = _capacity, i;

      allocate(new_capacity);

      // inserisco i vecchi elementi

      UGenericHashMapNode* _next;

      for (i = 0; i < old_capacity; ++i)
         {
         if (old_table[i])
            {
            node = old_table[i];

            do {
               _next = node->next;
               index = node->hash % _capacity;

               U_INTERNAL_DUMP("i = %u index = %u hash = %u", i, index, node->hash)

               // antepongo l'elemento all'inizio della lista delle collisioni

               node->next   = table[index];
               table[index] = node;
               }
            while ((node = _next));
            }
         }

      UMemoryPool::_free(old_table, old_capacity, sizeof(UGenericHashMapNode*));
      }

   // Cancellazione tabella

   void clear()
      {
      U_TRACE(0, "UGenericHashMap<K,I>::clear()")

      U_INTERNAL_DUMP("_length = %u", _length)

#  ifdef DEBUG
      int sum = 0, max = 0, min = 1024, width;
#  endif

      UGenericHashMapNode* _next;

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

               _next = node->next;

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

   // Call function for all entry

   bool first()
      {
      U_TRACE(0, "UGenericHashMap<K,I>::first()")

      U_INTERNAL_DUMP("_length = %u", _length)

      for (index = 0; index < _capacity; ++index)
         {
         if (table[index])
            {
            node = table[index];

            U_RETURN(true);
            }
         }

      U_RETURN(false);
      }

   bool next()
      {
      U_TRACE(0, "UGenericHashMap<K,I>::next()")

      U_INTERNAL_DUMP("index = %u node = %p next = %p", index, node, node->next)

      if ((node = node->next)) U_RETURN(true);

      for (++index; index < _capacity; ++index)
         {
         if (table[index])
            {
            node = table[index];

            U_RETURN(true);
            }
         }

      U_RETURN(false);
      }

   void callForAllEntry(bPFpvpv function)
      {
      U_TRACE(0, "UGenericHashMap<K,I>::callForAllEntry(%p)", function)

      U_INTERNAL_DUMP("_length = %u", _length)

      UGenericHashMapNode* n;
      UGenericHashMapNode* _next;

      for (uint32_t i = 0; i < _capacity; ++i)
         {
         if (table[i])
            {
            n = table[i];

            do {
               _next = n->next; // NB: function can delete the node...

               if (function(&(n->key), &(n->elem)) == false) return;
               }
            while ((n = _next));
            }
         }
      }

#ifdef DEBUG
   const char* dump(bool reset) const
      {
      *UObjectIO::os << "hash      " << hash         << '\n'
                     << "node      " << (void*)node  << '\n'
                     << "index     " << index        << '\n'
                     << "table     " << (void*)table << '\n'
                     << "_length   " << _length      << "\n"
                     << "_capacity " << _capacity;

      if (reset)
         {
         UObjectIO::output();

         return UObjectIO::buffer_output;
         }

      return 0;
      }
#endif

private:
   UGenericHashMap(const UGenericHashMap&)            {}
   UGenericHashMap& operator=(const UGenericHashMap&) { return *this; }

   template<typename A, typename B, typename C, typename D>
   UGenericHashMap(const UGenericHashMap<A,B,C,D>&)            {}

   template<typename A, typename B, typename C, typename D>
   UGenericHashMap& operator=(const UGenericHashMap<A,B,C,D>&) { return *this; }
};

/*
// Functor used by UGenericHashMap class to generate a hashcode for an object of type UString
template <> struct UHashCodeFunctor<UString> { uint32_t operator()(const UString& str) const { return str.hash(false); } };
*/

#endif
