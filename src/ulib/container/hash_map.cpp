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

bool   UHashMap<void*>::stop_call_for_all_entry;
uPFpcu UHashMap<void*>::gperf;

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

   UHashMapNode* prev = 0;

   for (node = table[index]; node; node = node->next)
      {
      if (node->key->equal(keyr, ignore_case))
         {
         // lista self-organizing (move-to-front), antepongo
         // l'elemento trovato all'inizio della lista delle collisioni...

         if (prev)
            {
            prev->next   = node->next;
            node->next   = table[index];
            table[index] = node;
            }

         U_INTERNAL_ASSERT_EQUALS(node,table[index])

         break;
         }

      prev = node;
      }

   U_INTERNAL_DUMP("node = %p", node)
}

void* UHashMap<void*>::erase(const UString& key)
{
   U_TRACE(0, "UHashMap<void*>::erase(%.*S)", U_STRING_TO_TRACE(key))

   lookup(key);

   if (node)
      {
      void* elem = node->elem;

      eraseAfterFind();

      U_RETURN(elem);
      }

   U_RETURN((void*)0);
}

void UHashMap<UString>::insertAfterFind(const UString& key, const UString& str)
{
   U_TRACE(0, "UHashMap<UString>::insertAfterFind(%.*S,%.*S)", U_STRING_TO_TRACE(key), U_STRING_TO_TRACE(str))

   UHashMap<UStringRep*>::insertAfterFind(key, str.rep);
}

// OPERATOR []

void* UHashMap<void*>::at(UStringRep* key)
{
   U_TRACE(0, "UHashMap<void*>::at(%.*S)", U_STRING_TO_TRACE(*key))

   lookup(key);

   if (node)
      {
      void* elem = node->elem;

      U_RETURN(elem);
      }

   U_RETURN((void*)0);
}

void UHashMap<void*>::insertAfterFind(const UString& key, void* elem)
{
   U_TRACE(0, "UHashMap<void*>::insertAfterFind(%.*S,%p)", U_STRING_TO_TRACE(key), elem)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_EQUALS(node,0)

   // antepongo l'elemento all'inizio della lista delle collisioni

   node = table[index] = U_NEW(UHashMapNode(key, elem, table[index], hash));

   ++_length;

   U_INTERNAL_DUMP("_length = %u", _length)
}

void UHashMap<void*>::eraseAfterFind()
{
   U_TRACE(0, "UHashMap<void*>::eraseAfterFind()")

   // presuppone l'elemento da cancellare all'inizio della lista
   // delle collisioni - lista self-organizing (move-to-front)...

   U_INTERNAL_ASSERT_EQUALS(node,table[index])

   table[index] = node->next;

   delete node;

   --_length;

   U_INTERNAL_DUMP("_length = %u", _length)
}

void UHashMap<void*>::reserve(uint32_t n)
{
   U_TRACE(0, "UHashMap<void*>::reserve(%u)", n)

   U_INTERNAL_ASSERT_EQUALS(gperf,0)
   U_INTERNAL_ASSERT_MAJOR(_capacity,0)

   UHashMapNode** old_table    = table;
   uint32_t       old_capacity = _capacity;

   allocate(U_GET_NEXT_PRIME_NUMBER(n));

   // inserisco i vecchi elementi

   UHashMapNode* next;

   for (uint32_t i = 0; i < old_capacity; ++i)
      {
      if (old_table[i])
         {
         node = old_table[i];

         do {
            next  = node->next;
            index = node->hash % _capacity;

            U_INTERNAL_DUMP("i = %u index = %u hash = %u", i, index, node->hash)

            // antepongo l'elemento all'inizio della lista delle collisioni

            node->next   = table[index];
            table[index] = node;
            }
         while ((node = next));
         }
      }

   U_FREE_VECTOR(old_table, old_capacity, UHashMapNode);
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
   UHashMapNode* next;

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
#     ifdef DEBUG
            ++width;
#     endif

            next = n->next; // NB: function can delete the node...

            function(n->key, n->elem);

            if (stop_call_for_all_entry)
               {
               stop_call_for_all_entry = false;

               return;
               }
            }
         while ((n = next));

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

UString UHashMap<UString>::erase(const UString& key)
{
   U_TRACE(0, "UHashMap<UString>::erase(%.*S)", U_STRING_TO_TRACE(key))

   UHashMap<void*>::lookup(key);

   if (node)
      {
      UString str(elem());

      eraseAfterFind();

      U_RETURN_STRING(str);
      }

   U_RETURN_STRING(UString::getStringNull());
}

// OPERATOR []

UString UHashMap<UString>::at(UStringRep* key)
{
   U_TRACE(0, "UHashMap<UString>::at(%.*S)", U_STRING_TO_TRACE(*key))

   UHashMap<void*>::lookup(key);

   if (node)
      {
      UString str(elem());

      U_RETURN_STRING(str);
      }

   U_RETURN_STRING(UString::getStringNull());
}

// STREAMS

U_EXPORT istream& operator>>(istream& is, UHashMap<UString>& t)
{
   U_TRACE(0+256, "UHashMap<UString>::operator>>(%p,%p)", &is, &t)

   U_INTERNAL_ASSERT_MAJOR(t._capacity,0)

   int c = EOF;

   if (is.good())
      {
      streambuf* sb = is.rdbuf();

      UString key(U_CAPACITY), str(U_CAPACITY);

      // NB: we need this way for plugin...

      if (is.peek() == '{') c = sb->sbumpc(); // skip '{'

      do {
         do { c = sb->sbumpc(); } while (u_isspace(c) && c != EOF); // skip white-space

      // U_INTERNAL_DUMP("c = %C", c)

         if (c == '}' || c == EOF) break;

         if (c == '#')
            {
            do { c = sb->sbumpc(); } while (c != '\n' && c != EOF); // skip line comment

            continue;
            }

         U_INTERNAL_ASSERT_EQUALS(u_isspace(c),false)

         sb->sputbackc(c);

         key.get(is);

         U_INTERNAL_ASSERT(key.isNullTerminated())
         U_INTERNAL_ASSERT_EQUALS(key.empty(),false)

         do { c = sb->sbumpc(); } while (u_isspace(c) && c != EOF); // skip white-space

      // U_INTERNAL_DUMP("c = %C", c)

         if (c == EOF) break;

         U_INTERNAL_ASSERT_EQUALS(u_isspace(c),false)

         sb->sputbackc(c);

         str.get(is);

         U_INTERNAL_ASSERT(str.isNullTerminated())
         U_INTERNAL_ASSERT_EQUALS(str.empty(),false)

         t.insert(key, str);
         }
      while (c != EOF);
      }

   if (c == EOF)       is.setstate(ios::eofbit);
// if (t._length == 0) is.setstate(ios::failbit);

   return is;
}

U_EXPORT ostream& operator<<(ostream& os, const UHashMap<UString>& t)
{
   U_TRACE(0+256, "UHashMap<UString>::operator<<(%p,%p)", &os, &t)

   UHashMapNode* node;
   UHashMapNode* next;

   os.put('{');
   os.put('\n');

   for (UHashMapNode** ptr = t.table; ptr < (t.table + t._capacity); ++ptr)
      {
      if (*ptr)
         {
         node = *ptr;

         do {
            next = node->next;

            node->key->write(os);

            os.put('\t');

            ((UStringRep*)node->elem)->write(os);

            os.put('\n');
            }
         while ((node = next));
         }
      }

   os.put('}');

   return os;
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
