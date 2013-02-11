// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    hash_map.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/container/vector.h>
#include <ulib/container/hash_map.h>

bool        UHashMap<void*>::stop_call_for_all_entry;
uPFpcu      UHashMap<void*>::gperf;
UStringRep* UHashMap<void*>::pkey;

void UHashMap<void*>::lookup(UStringRep* keyr)
{
   U_TRACE(0, "UHashMap<void*>::lookup(%.*S)", U_STRING_TO_TRACE(*keyr))

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_MAJOR(_capacity,0)

   if (gperf) index = gperf(U_STRING_TO_PARAM(*keyr));
   else
      {
      hash  = keyr->hash(ignore_case);
      index = hash % _capacity;
      }

   U_INTERNAL_DUMP("index = %u", index)

   U_INTERNAL_ASSERT_MINOR(index,_capacity)

   for (node = table[index]; node; node = node->next)
      {
      if (node->key->equal(keyr, ignore_case)) break;
      }

   U_INTERNAL_DUMP("node = %p", node)
}

void* UHashMap<void*>::erase(UStringRep* _key)
{
   U_TRACE(0, "UHashMap<void*>::erase(%.*S)", U_STRING_TO_TRACE(*_key))

   lookup(_key);

   if (node)
      {
      void* _elem = node->elem;

      eraseAfterFind();

      U_RETURN(_elem);
      }

   U_RETURN((void*)0);
}

// OPERATOR []

void* UHashMap<void*>::at(UStringRep* _key)
{
   U_TRACE(0, "UHashMap<void*>::at(%.*S)", U_STRING_TO_TRACE(*_key))

   lookup(_key);

   if (node) U_RETURN(node->elem);

   U_RETURN((void*)0);
}

void UHashMap<void*>::insertAfterFind(UStringRep* _key, void* _elem)
{
   U_TRACE(0, "UHashMap<void*>::insertAfterFind(%.*S,%p)", U_STRING_TO_TRACE(*_key), _elem)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_EQUALS(node,0)

   // antepongo l'elemento all'inizio della lista delle collisioni

   node = table[index] = U_NEW(UHashMapNode(_key, _elem, table[index], hash));

   ++_length;

   U_INTERNAL_DUMP("_length = %u", _length)
}

void UHashMap<void*>::_eraseAfterFind()
{
   U_TRACE(0, "UHashMap<void*>::_eraseAfterFind()")

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("node = %p", node)

   UHashMapNode* prev = 0;

   for (UHashMapNode* pnode = table[index]; pnode; pnode = pnode->next)
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
}

void UHashMap<void*>::eraseAfterFind()
{
   U_TRACE(0, "UHashMap<void*>::eraseAfterFind()")

   _eraseAfterFind();

   delete node;

   --_length;

   U_INTERNAL_DUMP("_length = %u", _length)
}

void UHashMap<void*>::replaceKey(const UString& _key)
{
   U_TRACE(0, "UHashMap<void*>::replaceKey(%.*S)", U_STRING_TO_TRACE(_key))

   UHashMapNode* pnode = node;

   _eraseAfterFind();

   lookup(_key);

   U_INTERNAL_ASSERT_EQUALS(node,0)

   pnode->hash = hash;
   pnode->next = table[index];

   pnode->key->release(); // NB: si decrementa la reference della stringa...

   pnode->key = _key.rep;

   pnode->key->hold();    // NB: si incrementa la reference della stringa...

   // antepongo l'elemento all'inizio della lista delle collisioni

   node = table[index] = pnode;
}

void UHashMap<void*>::reserve(uint32_t n)
{
   U_TRACE(0, "UHashMap<void*>::reserve(%u)", n)

   U_INTERNAL_ASSERT_EQUALS(gperf,0)
   U_INTERNAL_ASSERT_MAJOR(_capacity,0)

   uint32_t new_capacity = U_GET_NEXT_PRIME_NUMBER(n);

   if (new_capacity == _capacity) return;

   UHashMapNode** old_table    = table;
   uint32_t       old_capacity = _capacity, i;

   allocate(new_capacity);

#ifdef DEBUG
   int sum = 0, max = 0, min = 1024, width;
#endif

   // inserisco i vecchi elementi

   UHashMapNode* _next;

   for (i = 0; i < old_capacity; ++i)
      {
      if (old_table[i])
         {
         node = old_table[i];

#     ifdef DEBUG
         ++sum;
         width = -1;
#     endif

         do {
#        ifdef DEBUG
            ++width;
#        endif

            _next  = node->next;
            index  = node->hash % _capacity;

            U_INTERNAL_DUMP("i = %u index = %u hash = %u", i, index, node->hash)

            // antepongo l'elemento all'inizio della lista delle collisioni

            node->next   = table[index];
            table[index] = node;
            }
         while ((node = _next));

#     ifdef DEBUG
         if (max < width) max = width;
         if (min > width) min = width;
#     endif
         }
      }

   UMemoryPool::_free(old_table, old_capacity, sizeof(UHashMapNode*));

   U_INTERNAL_DUMP("OLD: collision(min,max) = (%3d,%3d) - distribution = %3f", min, max, (double)_length / (double)sum)

#ifdef DEBUG
   sum = 0, max = 0, min = 1024;

   UHashMapNode* _n;

   for (i = 0; i < _capacity; ++i)
      {
      if (table[i])
         {
         _n = table[i];

         ++sum;
         width = -1;

         do {
            ++width;

            _next = _n->next;
            }
         while ((_n = _next));

         if (max < width) max = width;
         if (min > width) min = width;
         }
      }
#endif

   U_INTERNAL_DUMP("NEW: collision(min,max) = (%3d,%3d) - distribution = %3f", min, max, (double)_length / (double)sum)
}

bool UHashMap<void*>::first()
{
   U_TRACE(0, "UHashMap<void*>::first()")

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

bool UHashMap<void*>::next()
{
   U_TRACE(0, "UHashMap<void*>::next()")

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

void UHashMap<void*>::callForAllEntry(vPFprpv function)
{
   U_TRACE(0, "UHashMap<void*>::callForAllEntry(%p)", function)

   U_INTERNAL_DUMP("_length = %u", _length)

#ifdef DEBUG
   int sum = 0, max = 0, min = 1024, width;
#endif

   UHashMapNode* n;
   UHashMapNode* _next;

   for (uint32_t i = 0; i < _capacity; ++i)
      {
      if (table[i])
         {
         n = table[i];

#     ifdef DEBUG
         ++sum;
         width = -1;
#     endif

         do {
#        ifdef DEBUG
            ++width;
#        endif

            _next = n->next; // NB: function can delete the node...

            function(n->key, n->elem);

            if (stop_call_for_all_entry)
               {
               stop_call_for_all_entry = false;

               return;
               }
            }
         while ((n = _next));

#     ifdef DEBUG
         if (max < width) max = width;
         if (min > width) min = width;
#     endif
         }
      }

   U_INTERNAL_DUMP("collision(min,max) = (%d,%d) - distribution = %f", min, max, (double)_length / (double)sum)
}

void UHashMap<void*>::getKeys(UVector<UString>& vec)
{
   U_TRACE(0, "UHashMap<void*>::getKeys(%p)", &vec)

   for (index = 0; index < _capacity; ++index)
      {
      if (table[index])
         {
         node = table[index];

         do {
            vec.UVector<UStringRep*>::push(node->key);
            }
         while ((node = node->next));
         }
      }
}

void UHashMap<void*>::_callForAllEntrySorted(vPFprpv function)
{
   U_TRACE(0, "UHashMap<void*>::_callForAllEntrySorted(%p)", function)

   U_INTERNAL_ASSERT_MAJOR(_length, 1)

   UVector<UString> vkey(_length);

   getKeys(vkey);

   U_ASSERT_EQUALS(_length, vkey.size())

   vkey.sort(ignore_case);

   UStringRep* r;

   for (uint32_t i = 0, n = _length; i < n; ++i)
      {
      r = vkey.UVector<UStringRep*>::at(i);

      lookup(r);

      U_INTERNAL_ASSERT_POINTER(node)

      function(r, node->elem);

      if (stop_call_for_all_entry)
         {
         stop_call_for_all_entry = false;
                  
         return;
         }
      }
}

// specializzazione stringa

void UHashMap<UString>::insertAfterFind(const UString& _key, const UString& str)
{
   U_TRACE(0, "UHashMap<UString>::insertAfterFind(%.*S,%.*S)", U_STRING_TO_TRACE(_key), U_STRING_TO_TRACE(str))

   UHashMap<UStringRep*>::insertAfterFind(_key, str.rep);
}

UString UHashMap<UString>::erase(const UString& _key)
{
   U_TRACE(0, "UHashMap<UString>::erase(%.*S)", U_STRING_TO_TRACE(_key))

   UHashMap<void*>::lookup(_key);

   if (node)
      {
      UString str(elem());

      eraseAfterFind();

      U_RETURN_STRING(str);
      }

   U_RETURN_STRING(UString::getStringNull());
}

// OPERATOR []

UString UHashMap<UString>::at(UStringRep* _key)
{
   U_TRACE(0, "UHashMap<UString>::at(%.*S)", U_STRING_TO_TRACE(*_key))

   UHashMap<void*>::lookup(_key);

   if (node)
      {
      UString str(elem());

      U_RETURN_STRING(str);
      }

   U_RETURN_STRING(UString::getStringNull());
}

UString UHashMap<UString>::at(const char* _key, uint32_t keylen)
{
   U_TRACE(0, "UHashMap<UString>::at(%.*S,%u)", keylen, _key, keylen)

   U_INTERNAL_ASSERT_POINTER(pkey)

   pkey->str     = _key;
   pkey->_length = keylen;

   return at(pkey);
}

void* UHashMap<void*>::operator[](const char* _key)
{
   U_TRACE(0, "UHashMap<void*>::operator[](%S)", _key)

   U_INTERNAL_ASSERT_POINTER(pkey)

   pkey->str     =           _key;
   pkey->_length = u__strlen(_key);

   return at(pkey);
}

UString UHashMap<UString>::operator[](const char* _key)
{
   U_TRACE(0, "UHashMap<UString>::operator[](%S)", _key)

   U_INTERNAL_ASSERT_POINTER(pkey)

   pkey->str     =           _key;
   pkey->_length = u__strlen(_key);

   return at(pkey);
}

void* UHashMap<void*>::erase(const char* _key)
{
   U_TRACE(0, "UHashMap<void*>::erase(%S)", _key)

   U_INTERNAL_ASSERT_POINTER(pkey)

   pkey->str     =           _key;
   pkey->_length = u__strlen(_key);

   return erase(pkey);
}

// storage session

UHashMap<UString>* UHashMap<UString>::fromStream(istream& is)
{
   U_TRACE(0, "UHashMap<UString>::fromStream(%p)", &is)

   U_INTERNAL_ASSERT_EQUALS(is.peek(), '[')

   UHashMap<UString>* t = U_NEW(UHashMap<UString>);

   t->allocate();

   is >> *t;

   U_RETURN_POINTER(t,UHashMap<UString>);
}

UHashMap<UString>* UHashMap<UString>::duplicate(UHashMap<UString>* t)
{
   U_TRACE(0, "UHashMap<UString>::duplicate(%p)", t)

   UHashMap<UString>* t1 = 0;

   if (t)
      {
      t1 = U_NEW(UHashMap<UString>);

      uint32_t index, n = t->_capacity;

      t1->allocate(n);

      // inserisco i vecchi elementi

      UHashMapNode* _next;

      for (uint32_t i = 0; i < n; ++i)
         {
         if (t->table[i])
            {
            t1->node = t->table[i];

            do {
               _next  = t1->node->next;
               index  = t1->node->hash % n;

               U_INTERNAL_DUMP("i = %u index = %u hash = %u", i, index, t1->node->hash)

               // antepongo l'elemento all'inizio della lista delle collisioni

               t1->node->next   = t1->table[index];
               t1->table[index] = t1->node;
               }
            while ((t1->node = _next));
            }
         }
      }

   U_RETURN_POINTER(t1,UHashMap<UString>);
}

// STREAMS

U_EXPORT istream& operator>>(istream& is, UHashMap<UString>& t)
{
   U_TRACE(0+256, "UHashMap<UString>::operator>>(%p,%p)", &is, &t)

   U_INTERNAL_ASSERT_MAJOR(t._capacity,0)

   int c = EOF;

   if (is.good())
      {
      UString key(U_CAPACITY), str(U_CAPACITY);

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

         str.get(is);

         U_INTERNAL_ASSERT(str)
         U_INTERNAL_ASSERT(str.isNullTerminated())

         t.insert(key, str);
         }
      while (c != EOF);
      }

   if (c == EOF)       is.setstate(ios::eofbit);
// if (t._length == 0) is.setstate(ios::failbit);

   return is;
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UHashMapNode::dump(bool reset) const
{
   *UObjectIO::os << "elem               " << elem        << '\n'
                  << "hash               " << hash        << '\n'
                  << "key  (UStringRep   " << (void*)key  << ")\n"
                  << "next (UHashMapNode " << (void*)next << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

const char* UHashMap<void*>::dump(bool reset) const
{
   *UObjectIO::os << "hash               " << hash         << '\n'
                  << "index              " << index        << '\n'
                  << "table              " << (void*)table << '\n'
                  << "_length            " << _length      << "\n"
                  << "_capacity          " << _capacity    << '\n'
                  << "ignore_case        " << ignore_case  << '\n'
                  << "node (UHashMapNode " << (void*)node  << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
