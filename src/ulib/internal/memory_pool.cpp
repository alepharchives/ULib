// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    memory_pool.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file.h>

// --------------------------------------------------------------------------------------
// U_NUM_ENTRY_MEM_BLOCK: numero di blocchi da preallocare per 'type' stack

#define U_NUM_ENTRY_MEM_BLOCK 32 // (per fare i test assegnare a 2)

// U_SIZE_MEM_BLOCK: space da preallocare totale per i vari 'type' stack definiti (~256k bytes)

#define U_SIZE_MEM_BLOCK (U_STACK_TYPE_0  * U_NUM_ENTRY_MEM_BLOCK + \
                          U_STACK_TYPE_1  * U_NUM_ENTRY_MEM_BLOCK + \
                          U_STACK_TYPE_2  * U_NUM_ENTRY_MEM_BLOCK + \
                          U_STACK_TYPE_3  * U_NUM_ENTRY_MEM_BLOCK + \
                          U_STACK_TYPE_4  * U_NUM_ENTRY_MEM_BLOCK + \
                          U_STACK_TYPE_5  * U_NUM_ENTRY_MEM_BLOCK + \
                          U_STACK_TYPE_6  * U_NUM_ENTRY_MEM_BLOCK + \
                          U_STACK_TYPE_7  * U_NUM_ENTRY_MEM_BLOCK + \
                          U_STACK_TYPE_8  * U_NUM_ENTRY_MEM_BLOCK + \
                          U_STACK_TYPE_9  * U_NUM_ENTRY_MEM_BLOCK)
// --------------------------------------------------------------------------------------

#if defined(DEBUG) || defined(U_TEST)
sig_atomic_t UMemoryPool::index_stack_busy = -1;
#endif

/*
uint32_t UMemoryPool::stackIndexToSize(uint32_t sz)
{
   U_TRACE(0, "UMemoryPool::stackIndexToSize(%u)", sz)

   U_INTERNAL_ASSERT_RANGE(1,sz,U_MAX_SIZE_PREALLOCATE)

   static const uint32_t stack_type[U_NUM_STACK_TYPE] = // 10
      { U_STACK_TYPE_0,  U_STACK_TYPE_1, U_STACK_TYPE_2,
        U_STACK_TYPE_3,  U_STACK_TYPE_4, U_STACK_TYPE_5,
        U_STACK_TYPE_6,  U_STACK_TYPE_7, U_STACK_TYPE_8,
        U_STACK_TYPE_9 };

   uint32_t key;
    int32_t probe, low = -1, high = U_NUM_STACK_TYPE; // 10

   while ((high - low) > 1)
      {
      probe = (low + high) >> 1;
      key   = stack_type[probe];

      U_INTERNAL_DUMP("low = %d high = %d probe = %d key = %u", low, high, probe, key)

      if (key > sz) high = probe;
      else           low = probe;
      }

   if (low == -1 || (key = stack_type[low], sz > key)) ++low;

   U_INTERNAL_ASSERT(sz <= stack_type[low])

   U_RETURN(low);

   if (sz <= U_STACK_TYPE_0) U_RETURN(0);
   if (sz <= U_STACK_TYPE_1) U_RETURN(1);
   if (sz <= U_STACK_TYPE_2) U_RETURN(2);
   if (sz <= U_STACK_TYPE_3) U_RETURN(3);
   if (sz <= U_STACK_TYPE_4) U_RETURN(4);
   if (sz <= U_STACK_TYPE_5) U_RETURN(5);
   if (sz <= U_STACK_TYPE_6) U_RETURN(6);
   if (sz <= U_STACK_TYPE_7) U_RETURN(7);
   if (sz <= U_STACK_TYPE_8) U_RETURN(8);
                             U_RETURN(9);

   static uint32_t last_sz, last_stack_type;

   if (sz != last_sz)
      {
      if (sz <= U_STACK_TYPE_0) { last_stack_type = 0; goto end; }
      if (sz <= U_STACK_TYPE_1) { last_stack_type = 1; goto end; }
      if (sz <= U_STACK_TYPE_2) { last_stack_type = 2; goto end; }
      if (sz <= U_STACK_TYPE_3) { last_stack_type = 3; goto end; }
      if (sz <= U_STACK_TYPE_4) { last_stack_type = 4; goto end; }
      if (sz <= U_STACK_TYPE_5) { last_stack_type = 5; goto end; }
      if (sz <= U_STACK_TYPE_6) { last_stack_type = 6; goto end; }
      if (sz <= U_STACK_TYPE_7) { last_stack_type = 7; goto end; }
      if (sz <= U_STACK_TYPE_8) { last_stack_type = 8; goto end; }
                                  last_stack_type = 9;
      }

#ifdef DEBUG
   static uint32_t num_call, cache_miss;

                      ++num_call;
   if (sz != last_sz) ++cache_miss;

   U_INTERNAL_DUMP("num_call = %u cache_miss = %u", num_call, cache_miss)
#endif

end:
   U_RETURN(last_stack_type);
}
*/

// struttura e classe che encapsula lo stack dinamico di puntatori a blocchi preallocati per un 'type' dimensione definito

typedef struct ustackmemorypool {
#ifdef DEBUG
   ustackmemorypool* _this;
#endif
   void** pointer_block;
   uint32_t type, len, space;
   uint32_t index;
} ustackmemorypool;

#define U_STACK_INDEX_TO_SIZE(index)  (index == 0 ? U_STACK_TYPE_0 : \
                                       index == 1 ? U_STACK_TYPE_1 : \
                                       index == 2 ? U_STACK_TYPE_2 : \
                                       index == 3 ? U_STACK_TYPE_3 : \
                                       index == 4 ? U_STACK_TYPE_4 : \
                                       index == 5 ? U_STACK_TYPE_5 : \
                                       index == 6 ? U_STACK_TYPE_6 : \
                                       index == 7 ? U_STACK_TYPE_7 : \
                                       index == 8 ? U_STACK_TYPE_8 : U_MAX_SIZE_PREALLOCATE)

/*
           --   --        --   --   --        --   -- 
  10      |xx| |  |  --  |  | |  | |  |      |  | |  | -> space
   9      |xx| |  | |xx| |xx| |  | |xx|  --  |  | |xx|
   8  --  |xx| |  | |xx| |xx| |xx| |xx| |  | |xx| |xx|
   7 |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| -> len
   6 |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx|
   5 |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx|
   4 |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx|
   3 |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx|
   2 |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx|
   1 |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx|
      --   --   --   --   --   --   --   --   --   -- 
       8   24   32   56  128  256  512  1024 2048 4096 -> type
       0    1    2    3    4    5    6     7    8    9 -> index
*/

class U_NO_EXPORT UStackMemoryPool {
public:

   // Check for memory error
   U_MEMORY_TEST

   void** pointer_block;
   uint32_t type, len, space;
   uint32_t index;

   void deallocatePointerBlock()
      {
      U_TRACE(0, "UStackMemoryPool::deallocatePointerBlock()")

#  if defined(ENABLE_MEMPOOL) && defined(__linux__)
      U_INTERNAL_DUMP("&mem_pointer_block[  0] = %p", &mem_pointer_block[0])
      U_INTERNAL_DUMP("     pointer_block      = %p",      pointer_block)
      U_INTERNAL_DUMP("&mem_pointer_block[%3u] = %p",                    U_NUM_STACK_TYPE * U_NUM_ENTRY_MEM_BLOCK * 2,
                                                      &mem_pointer_block[U_NUM_STACK_TYPE * U_NUM_ENTRY_MEM_BLOCK * 2])

      if (pointer_block < &mem_pointer_block[0] ||
          pointer_block > &mem_pointer_block[U_NUM_STACK_TYPE * U_NUM_ENTRY_MEM_BLOCK * 2])
         {
         uint32_t size = space * sizeof(void*);

         U_INTERNAL_DUMP("size = %u", size)

         U_INTERNAL_ASSERT_EQUALS(size & U_PAGEMASK, 0)

         UMemoryPool::deallocate(pointer_block, size);
         }
#  endif
      }

   void growPointerBlock(uint32_t new_space)
      {
      U_TRACE(0, "UStackMemoryPool::growPointerBlock(%u)", new_space)

      U_INTERNAL_ASSERT_MAJOR(index, 0)

      // NB: si alloca lo space per numero totale puntatori
      //     (blocchi allocati precedentemente + un nuovo insieme di blocchi)
      //     relativi a type 'dimensione' stack corrente...

      uint32_t size = new_space * sizeof(void*);
      void** new_block = (void**) UFile::mmap(&size, -1, PROT_READ | PROT_WRITE, U_MAP_ANON, 0);
      new_space = (size / sizeof(void*));

      if (len) U__MEMCPY(new_block, pointer_block, len * sizeof(void*));

#  if defined(DEBUG) || defined(U_TEST)
      UMemoryPool::index_stack_busy = index;
#  endif

      deallocatePointerBlock();

      space         = new_space;
      pointer_block = new_block;

#  if defined(DEBUG) || defined(U_TEST)
      UMemoryPool::index_stack_busy = -1;
#  endif

      U_INTERNAL_DUMP("index = %u type = %u len = %u space = %u", index, type, len, space)
      }

   void allocateMemoryBlocks(uint32_t n)
      {
      U_TRACE(0, "UStackMemoryPool::allocateMemoryBlocks(%u)", n)

      U_INTERNAL_ASSERT_MAJOR(n, len)

      // si alloca un nuovo insieme di blocchi relativi a 'type' stack corrente...

      uint32_t num_entry = (n - len),
               size      = num_entry * type;

      U_INTERNAL_DUMP("num_entry = %u size = %u", num_entry, size)

      if (index == 0)
         {
         U_INTERNAL_ASSERT_EQUALS(len, 0)

#     if defined(DEBUG) || defined(U_TEST)
         UMemoryPool::index_stack_busy = index;
#     endif

         pointer_block = (void**) UFile::mmap(&size, -1, PROT_READ | PROT_WRITE, U_MAP_ANON, 0);
         len = space   = (size / type);

#     if defined(DEBUG) || defined(U_TEST)
         UMemoryPool::index_stack_busy = -1;
#     endif
         }
      else
         {
         char* pblock = (char*) UMemoryPool::_malloc(&num_entry, type);

         uint32_t new_len = len + num_entry;

         U_INTERNAL_DUMP("num_entry = %u new_len = %u", num_entry, new_len)

         U_INTERNAL_ASSERT(new_len >= U_NUM_ENTRY_MEM_BLOCK)

         char* eblock = pblock + (num_entry * type);

         if (space <= new_len) growPointerBlock(space + num_entry); // NB: this call can change len...

#     if defined(DEBUG) || defined(U_TEST)
         UMemoryPool::index_stack_busy = index;
#     endif

         // ...che viene inizializzato suddividendolo in base al type
         //    'dimensione' e assegnando i valori dei puntatori relativi nel nuovo stack

         do {
                                           eblock -= type;
            pointer_block[len++] = (void*) eblock;
            }
         while (--num_entry);

#     if defined(DEBUG) || defined(U_TEST)
         UMemoryPool::index_stack_busy = -1;
#     endif

         U_INTERNAL_ASSERT_EQUALS(pblock, eblock)
         }

      U_INTERNAL_DUMP("index = %u type = %u len = %u space = %u", index, type, len, space)
      }

   void* pop()
      {
      U_TRACE(0, "UStackMemoryPool::pop()")

      U_INTERNAL_DUMP("index = %u type = %u len = %u space = %u", index, type, len, space, len)

      U_INTERNAL_ASSERT_MINOR(index, U_NUM_STACK_TYPE) // 10
      U_INTERNAL_ASSERT_DIFFERS(index, (uint32_t)UMemoryPool::index_stack_busy)

      if (len == 0) allocateMemoryBlocks(space);

#  if defined(DEBUG) || defined(U_TEST)
      UMemoryPool::index_stack_busy = index;
#  endif

      void** pblock = pointer_block + --len;

#  if defined(DEBUG) || defined(U_TEST)
      UMemoryPool::index_stack_busy = -1;
#  endif

      U_INTERNAL_DUMP("index = %u type = %u len = %u space = %u", index, type, len, space)

      void* ptr = (index == 0 ? (void*)pblock : *pblock);

      U_INTERNAL_ASSERT_EQUALS(((long)ptr & (sizeof(long)-1)), 0) // memory aligned

      U_RETURN(ptr);
      }

   void push(void* ptr)
      {
      U_TRACE(0, "UStackMemoryPool::push(%p)", ptr)

      U_INTERNAL_DUMP("index = %u type = %u len = %u space = %u", index, type, len, space, len)

      U_INTERNAL_ASSERT_MAJOR(index, 0)
      U_INTERNAL_ASSERT_MINOR(index, U_NUM_STACK_TYPE) // 10
      U_INTERNAL_ASSERT_EQUALS(index, U_SIZE_TO_STACK_INDEX(type))
      U_INTERNAL_ASSERT_EQUALS(type, U_STACK_INDEX_TO_SIZE(index))
      U_INTERNAL_ASSERT_DIFFERS(index, (uint32_t)UMemoryPool::index_stack_busy)
      U_INTERNAL_ASSERT_EQUALS(((long)ptr & (sizeof(long)-1)), 0) // memory aligned

      if (len == space) growPointerBlock(space << 1); // NB: this call can change len...

#  if defined(DEBUG) || defined(U_TEST)
      (void) U_SYSCALL(memset, "%p,%d,%u", (void*)ptr, 0, type); // NB: in debug mode the memory area is zeroed to enhance showing bugs...

      UMemoryPool::index_stack_busy = index;
#  endif

      pointer_block[len++] = (void*)ptr;

#  if defined(DEBUG) || defined(U_TEST)
      UMemoryPool::index_stack_busy = -1;
#  endif

      U_INTERNAL_DUMP("index = %u type = %u len = %u space = %u", index, type, len, space)
      }

#ifdef DEBUG
   static void paint(ostream& os); // print info
#endif

   // NB: inizialmente viene allocato lo spazio per il doppio dei
   //     puntatori necessari (quelli cioe' che vengono inizializzati)...

#if defined(ENABLE_MEMPOOL) && defined(__linux__)
   static char  mem_block[U_SIZE_MEM_BLOCK];
   static void* mem_pointer_block[U_NUM_STACK_TYPE * U_NUM_ENTRY_MEM_BLOCK * 2];

   static ustackmemorypool mem_stack[U_NUM_STACK_TYPE]; // 10
#endif
};

#if defined(ENABLE_MEMPOOL) && defined(__linux__) && defined(DEBUG)
bool UMemoryPool::check(void* ptr)
{
   U_TRACE(0, "UMemoryPool::check(%p)", ptr)

   UStackMemoryPool* pstack;

   for (uint32_t stack_index = 0; stack_index < U_NUM_STACK_TYPE; ++stack_index)
      {
      pstack = (UStackMemoryPool*)(UStackMemoryPool::mem_stack+stack_index);

      U_INTERNAL_DUMP("stack[%u].index = %u stack[%u].len = %5u stack[%u].space = %5u",
                       stack_index, pstack->index,
                       stack_index, pstack->len,
                       stack_index, pstack->space)

      for (uint32_t i = 0; i < pstack->len; ++i)
         {
         if (ptr == pstack->pointer_block[i])
            {
            U_ERROR("duplicate entry on memory pool: stack[%u].index = %u stack[%u].len = %u stack[%u].space = %u stack[%u].pointer_block[%u] = %p",
                       stack_index,    pstack->index,
                       stack_index,    pstack->len,
                       stack_index,    pstack->space,
                       stack_index, i, pstack->pointer_block[i]);
            }
         }
      }

   U_RETURN(true);
}
#endif

void UMemoryPool::allocateMemoryBlocks(int stack_index, uint32_t n)
{
   U_TRACE(0+256, "UMemoryPool::allocateMemoryBlocks(%d,%u)", stack_index, n)

   U_INTERNAL_ASSERT_MAJOR(stack_index, 0)
   U_INTERNAL_ASSERT_MINOR(stack_index, U_NUM_STACK_TYPE) // 10

#if defined(ENABLE_MEMPOOL) && defined(__linux__)
   UStackMemoryPool* ptr = (UStackMemoryPool*)(UStackMemoryPool::mem_stack+stack_index);

   U_INTERNAL_DUMP("index = %u type = %u len = %u space = %u", ptr->index, ptr->type, ptr->len, ptr->space)

   if (n > ptr->len) ptr->allocateMemoryBlocks(n);
#endif
}

void UMemoryPool::push(void* ptr, int stack_index)
{
   U_TRACE(0+256, "UMemoryPool::push(%p,%d)", ptr, stack_index)

   U_INTERNAL_ASSERT_MINOR(stack_index, U_NUM_STACK_TYPE) // 10

#if !defined(ENABLE_MEMPOOL) || !defined(__linux__)
   U_SYSCALL_VOID(free, "%p", ptr);
#else
   U_ASSERT(check(ptr))

   if (stack_index) ((UStackMemoryPool*)(UStackMemoryPool::mem_stack+stack_index))->push(ptr);
#endif
}

void* UMemoryPool::pop(int stack_index)
{
   U_TRACE(0+256, "UMemoryPool::pop(%d)", stack_index)

   U_INTERNAL_ASSERT_MINOR(stack_index, U_NUM_STACK_TYPE) // 10

#if !defined(ENABLE_MEMPOOL) || !defined(__linux__)
   void* ptr = U_SYSCALL(malloc, "%u", U_STACK_INDEX_TO_SIZE(stack_index));
#else
   void* ptr = ((UStackMemoryPool*)(UStackMemoryPool::mem_stack+stack_index))->pop();

   U_ASSERT(check(ptr))
#endif

   U_RETURN(ptr);
}

void* UMemoryPool::_malloc(uint32_t num, uint32_t type_size, bool bzero)
{
   U_TRACE(0, "UMemoryPool::_malloc(%u,%u,%b)", num, type_size, bzero)

   U_INTERNAL_ASSERT_MAJOR(num, 0)

   void* ptr;
   uint32_t length = (num * type_size);

   U_INTERNAL_DUMP("length = %u", length)

#if !defined(ENABLE_MEMPOOL) || !defined(__linux__)
   ptr = U_SYSCALL(malloc, "%u", length);
#else
   if (length <= U_MAX_SIZE_PREALLOCATE)
      {
      int stack_index = U_SIZE_TO_STACK_INDEX(length);

      ptr    = ((UStackMemoryPool*)(UStackMemoryPool::mem_stack+stack_index))->pop();
      length = U_STACK_INDEX_TO_SIZE(stack_index);
      }
   else
      {
      ptr = UFile::mmap(&length, -1, PROT_READ | PROT_WRITE, U_MAP_ANON, 0);
      }
#endif

   if (bzero) (void) U_SYSCALL(memset, "%p,%d,%u", ptr, 0, length);

   U_RETURN(ptr);
}

void* UMemoryPool::_malloc(uint32_t* pnum, uint32_t type_size, bool bzero)
{
   U_TRACE(1, "UMemoryPool::_malloc(%p,%u,%b)", pnum, type_size, bzero)

   U_INTERNAL_ASSERT_POINTER(pnum)

   void* ptr;
   uint32_t length = (*pnum * type_size);

   U_INTERNAL_DUMP("length = %u", length)

#if !defined(ENABLE_MEMPOOL) || !defined(__linux__)
   ptr = U_SYSCALL(malloc, "%u", length);
#else
   if (length <= U_MAX_SIZE_PREALLOCATE)
      {
      int stack_index = U_SIZE_TO_STACK_INDEX(length);

      ptr    = ((UStackMemoryPool*)(UStackMemoryPool::mem_stack+stack_index))->pop();
      length = U_STACK_INDEX_TO_SIZE(stack_index);
      }
   else
      {
      ptr = UFile::mmap(&length, -1, PROT_READ | PROT_WRITE, U_MAP_ANON, 0);
      }

   *pnum = length / type_size;

   U_INTERNAL_DUMP("*pnum = %u length = %u", *pnum, length)
#endif

   if (bzero) (void) U_SYSCALL(memset, "%p,%d,%u", ptr, 0, length);

   U_RETURN(ptr);
}

void UMemoryPool::deallocate(void* ptr, uint32_t length)
{
   U_TRACE(1, "UMemoryPool::deallocate(%p,%u)", ptr, length)

#if !defined(ENABLE_MEMPOOL) || !defined(__linux__)
   U_SYSCALL_VOID(free, "%p", ptr);
#else
   if (UFile::isLastAllocation(ptr, length))
      {
      UFile::pfree  = (char*)ptr;
      UFile::nfree += length;

      U_INTERNAL_DUMP("UFile::nfree = %u UFile::pfree = %p", UFile::nfree, UFile::pfree)
      }
   else
      {
#  if !defined(DEBUG) && defined(HAVE_ARCH64)
      (void) U_SYSCALL(madvise, "%p,%lu,%d", (void*)ptr, length, MADV_DONTNEED);
#  else
      // munmap() is expensive. A series of page table entries must be cleaned up, and the VMA must be unlinked.
      // By contrast, madvise(MADV_DONTNEED) only needs to set a flag in the VMA and has the further benefit that
      // no system call is required to reallocate the memory. That operation informs the kernel that the pages can
      // be (destructively) discarded from memory; if the process tries to access the pages again, they will either
      // be faulted in from the underlying file, for a file mapping, or re-created as zero-filled pages, for the
      // anonymous mappings that are employed by user-space allocators. Of course, re-creating the pages zero filled
      // is normally exactly the desired behavior for a user-space memory allocator. (The only potential downside is
      // that process address space is not freed, but this tends not to matter on 64-bit systems) 

      (void) U_SYSCALL(munmap, "%p,%lu", (void*)ptr, length);
#  endif
      }
#endif
}

void UMemoryPool::_free(void* ptr, uint32_t num, uint32_t type_size)
{
   U_TRACE(1, "UMemoryPool::_free(%p,%u,%u)", ptr, num, type_size)

#if !defined(ENABLE_MEMPOOL) || !defined(__linux__)
   U_SYSCALL_VOID(free, "%p", ptr);
#else
   uint32_t length = (num * type_size);

   U_INTERNAL_DUMP("length = %u", length)

   if (length <= U_MAX_SIZE_PREALLOCATE) UMemoryPool::push(ptr, U_SIZE_TO_STACK_INDEX(length));
   else
      {
#  if defined(DEBUG) && defined(ENABLE_NEW_VECTOR)
      if ((length & U_PAGEMASK) != 0) U_INTERNAL_ASSERT_MAJOR(type_size, 1)
#  endif

      deallocate(ptr, (length + U_PAGEMASK) & ~U_PAGEMASK);
      }
#endif
}

#ifdef DEBUG
void UStackMemoryPool::paint(ostream& os) // paint info
{
   U_TRACE(0, "UStackMemoryPool::paint(%p)", &os)

#if defined(ENABLE_MEMPOOL) && defined(__linux__)
   int i;
   char buffer[256];
   uint32_t max_space = 0;

   for (i = 0; i < U_NUM_STACK_TYPE; ++i)
      {
      if (mem_stack[i].space > max_space) max_space = mem_stack[i].space;

      (void) snprintf(buffer, sizeof(buffer),
                      "stack[%u].type = %4u stack[%u].len = %5u stack[%u].space = %5u\n",
                       i, mem_stack[i].type,
                       i, mem_stack[i].len,
                       i, mem_stack[i].space);

      os << buffer;
      }

   (void) snprintf(buffer, sizeof(buffer),
#     ifdef HAVE_ARCH64
          "\n        8   24   32   56  128  256  512  1024 2048 4096\n"
#     else
          "\n        4   16   36   48  128  256  512  1024 2048 4096\n"
#     endif
          "       %s   %s   %s   %s   %s   %s   %s   %s   %s   %s\n",
          (mem_stack[0].space == max_space ? "--" : "  "),
          (mem_stack[1].space == max_space ? "--" : "  "),
          (mem_stack[2].space == max_space ? "--" : "  "),
          (mem_stack[3].space == max_space ? "--" : "  "),
          (mem_stack[4].space == max_space ? "--" : "  "),
          (mem_stack[5].space == max_space ? "--" : "  "),
          (mem_stack[6].space == max_space ? "--" : "  "),
          (mem_stack[7].space == max_space ? "--" : "  "),
          (mem_stack[8].space == max_space ? "--" : "  "),
          (mem_stack[9].space == max_space ? "--" : "  "));

   os << buffer;

   for (i = max_space-1; i >= 0; --i)
      {
      (void) snprintf(buffer, sizeof(buffer), "%5u %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c\n", i+1,
          (mem_stack[0].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[0].space == (uint32_t)i ? "--" : (mem_stack[0].len > (uint32_t)i ? "xx" : "  ")),
          (mem_stack[0].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[1].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[1].space == (uint32_t)i ? "--" : (mem_stack[1].len > (uint32_t)i ? "xx" : "  ")),
          (mem_stack[1].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[2].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[2].space == (uint32_t)i ? "--" : (mem_stack[2].len > (uint32_t)i ? "xx" : "  ")),
          (mem_stack[2].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[3].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[3].space == (uint32_t)i ? "--" : (mem_stack[3].len > (uint32_t)i ? "xx" : "  ")),
          (mem_stack[3].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[4].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[4].space == (uint32_t)i ? "--" : (mem_stack[4].len > (uint32_t)i ? "xx" : "  ")),
          (mem_stack[4].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[5].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[5].space == (uint32_t)i ? "--" : (mem_stack[5].len > (uint32_t)i ? "xx" : "  ")),
          (mem_stack[5].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[6].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[6].space == (uint32_t)i ? "--" : (mem_stack[6].len > (uint32_t)i ? "xx" : "  ")),
          (mem_stack[6].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[7].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[7].space == (uint32_t)i ? "--" : (mem_stack[7].len > (uint32_t)i ? "xx" : "  ")),
          (mem_stack[7].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[8].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[8].space == (uint32_t)i ? "--" : (mem_stack[8].len > (uint32_t)i ? "xx" : "  ")),
          (mem_stack[8].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[9].space >  (uint32_t)i ?  '|' :  ' '),
          (mem_stack[9].space == (uint32_t)i ? "--" : (mem_stack[9].len > (uint32_t)i ? "xx" : "  ")),
          (mem_stack[9].space >  (uint32_t)i ?  '|' :  ' '));

      os << buffer;
      }

   (void) snprintf(buffer, sizeof(buffer),
          "       --   --   --   --   --   --   --   --   --   --\n"
#     ifdef HAVE_ARCH64
          "        8   24   32   56  128  256  512  1024 2048 4096"
#     else
          "        4   16   36   48  128  256  512  1024 2048 4096\n"
#     endif
          "\n");

   os << buffer << endl;
#endif
}

void UMemoryPool::printInfo(ostream& os)
{
   U_TRACE(0+256, "UMemoryPool::printInfo(%p)", &os)

   UStackMemoryPool::paint(os);
}
#endif

#if defined(ENABLE_MEMPOOL) && defined(__linux__)
char UStackMemoryPool::mem_block[U_SIZE_MEM_BLOCK];
// ----------------------------------------------------------------------------
// NB: l'inizializzazione seguente dipende strettamente da 'U_NUM_ENTRY_MEM_BLOCK'
// che normalmente e' uguale a 32, altrimenti (per fare i test si assegna a 2...)
// si devono commentare nel file "memory_pool.dat" le righe eccessive...
// ***************************************************************** memory_pool.dat
// U_SPACE + U_TYPE *   0, U_SPACE + U_TYPE *   1, U_SPACE + U_TYPE *   2,
// U_SPACE + U_TYPE *   3, U_SPACE + U_TYPE *   4, U_SPACE + U_TYPE *   5,
// U_SPACE + U_TYPE *   6, U_SPACE + U_TYPE *   7, U_SPACE + U_TYPE *   8,
// .................................................................
// U_SPACE + U_TYPE * 126, U_SPACE + U_TYPE * 127,
// ***************************************************************** memory_pool.dat
// ----------------------------------------------------------------------------
void* UStackMemoryPool::mem_pointer_block[U_NUM_STACK_TYPE * U_NUM_ENTRY_MEM_BLOCK * 2] = {
#undef  U_TYPE
#define U_TYPE U_STACK_TYPE_0
#undef  U_SPACE
#define U_SPACE mem_block
#include "./memory_pool.dat"
#undef  U_TYPE
#define U_TYPE U_STACK_TYPE_1
#undef  U_SPACE
#define U_SPACE (mem_block + U_STACK_TYPE_0 * U_NUM_ENTRY_MEM_BLOCK)
#include "./memory_pool.dat"
#undef  U_TYPE
#define U_TYPE U_STACK_TYPE_2
#undef  U_SPACE
#define U_SPACE (mem_block + U_STACK_TYPE_0 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_1 * U_NUM_ENTRY_MEM_BLOCK)
#include "./memory_pool.dat"
#undef  U_TYPE
#define U_TYPE U_STACK_TYPE_3
#undef  U_SPACE
#define U_SPACE (mem_block + U_STACK_TYPE_0 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_1 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_2 * U_NUM_ENTRY_MEM_BLOCK)
#include "./memory_pool.dat"
#undef  U_TYPE
#define U_TYPE U_STACK_TYPE_4
#undef  U_SPACE
#define U_SPACE (mem_block + U_STACK_TYPE_0 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_1 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_2 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_3 * U_NUM_ENTRY_MEM_BLOCK)
#include "./memory_pool.dat"
#undef  U_TYPE
#define U_TYPE U_STACK_TYPE_5
#undef  U_SPACE
#define U_SPACE (mem_block + U_STACK_TYPE_0 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_1 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_2 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_3 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_4 * U_NUM_ENTRY_MEM_BLOCK)
#include "./memory_pool.dat"
#undef  U_TYPE
#define U_TYPE U_STACK_TYPE_6
#undef  U_SPACE
#define U_SPACE (mem_block + U_STACK_TYPE_0 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_1 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_2 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_3 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_4 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_5 * U_NUM_ENTRY_MEM_BLOCK)
#include "./memory_pool.dat"
#undef  U_TYPE
#define U_TYPE U_STACK_TYPE_7
#undef  U_SPACE
#define U_SPACE (mem_block + U_STACK_TYPE_0 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_1 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_2 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_3 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_4 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_5 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_6 * U_NUM_ENTRY_MEM_BLOCK)
#include "./memory_pool.dat"
#undef  U_TYPE
#define U_TYPE U_STACK_TYPE_8
#undef  U_SPACE
#define U_SPACE (mem_block + U_STACK_TYPE_0 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_1 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_2 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_3 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_4 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_5 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_6 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_7 * U_NUM_ENTRY_MEM_BLOCK)
#include "./memory_pool.dat"
#undef  U_TYPE
#define U_TYPE U_STACK_TYPE_9
#undef  U_SPACE
#define U_SPACE (mem_block + U_STACK_TYPE_0 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_1 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_2 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_3 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_4 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_5 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_6 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_7 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_8 * U_NUM_ENTRY_MEM_BLOCK)
#include "./memory_pool.dat"
};

// struttura e classe che encapsula lo stack dinamico di puntatori a blocchi preallocati per un 'type' dimensione definito
//
// typedef struct ustackmemorypool {
// #ifdef DEBUG
//    void* _this;
// #endif
//    void** pointer_block;
//    uint32_t type, len, space;
//    uint32_t index;
// } ustackmemorypool;

ustackmemorypool UStackMemoryPool::mem_stack[U_NUM_STACK_TYPE] = { { // 10
#ifdef DEBUG
   &mem_stack[0],
#endif
   mem_pointer_block + 0 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_0, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   0 }, {
#ifdef DEBUG
   &mem_stack[1],
#endif
   mem_pointer_block + 1 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_1, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   1 }, {
#ifdef DEBUG
   &mem_stack[2],
#endif
   mem_pointer_block + 2 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_2, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   2 }, {
#ifdef DEBUG
   &mem_stack[3],
#endif
   mem_pointer_block + 3 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_3, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   3 }, {
#ifdef DEBUG
   &mem_stack[4],
#endif
   mem_pointer_block + 4 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_4, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   4 }, {
#ifdef DEBUG
   &mem_stack[5],
#endif
   mem_pointer_block + 5 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_5, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   5 }, {
#ifdef DEBUG
   &mem_stack[6],
#endif
   mem_pointer_block + 6 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_6, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   6 }, {
#ifdef DEBUG
   &mem_stack[7],
#endif
   mem_pointer_block + 7 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_7, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   7 }, {
#ifdef DEBUG
   &mem_stack[8],
#endif
   mem_pointer_block + 8 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_8, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   8 }, {
#ifdef DEBUG
   &mem_stack[9],
#endif
   mem_pointer_block + 9 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_9, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   9 }
};
#endif
