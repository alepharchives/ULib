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

#include <ulib/internal/common.h>

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
                          U_STACK_TYPE_9  * U_NUM_ENTRY_MEM_BLOCK + \
                          U_STACK_TYPE_10 * U_NUM_ENTRY_MEM_BLOCK + \
                          U_STACK_TYPE_11 * U_NUM_ENTRY_MEM_BLOCK + \
                          U_STACK_TYPE_12 * U_NUM_ENTRY_MEM_BLOCK + \
                          U_STACK_TYPE_13 * U_NUM_ENTRY_MEM_BLOCK)
// --------------------------------------------------------------------------------------

#ifdef DEBUG
sig_atomic_t UMemoryPool::index_stack_busy = -1;
#endif

uint32_t UMemoryPool::findStackIndex(uint32_t sz)
{
   U_TRACE(0, "UMemoryPool::findStackIndex(%u)", sz)

   U_INTERNAL_ASSERT_RANGE(1,sz,U_MAX_SIZE_PREALLOCATE)

   static const uint32_t stack_type[U_NUM_STACK_TYPE] =
      { U_STACK_TYPE_0,  U_STACK_TYPE_1, U_STACK_TYPE_2, U_STACK_TYPE_3, U_STACK_TYPE_4,  U_STACK_TYPE_5,
        U_STACK_TYPE_6,  U_STACK_TYPE_7, U_STACK_TYPE_8, U_STACK_TYPE_9, U_STACK_TYPE_10, U_STACK_TYPE_11,
        U_STACK_TYPE_12, U_STACK_TYPE_13 };

   uint32_t key;
   int32_t probe, low = -1, high = U_NUM_STACK_TYPE;

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
}

void* UMemoryPool::_malloc_str(size_t sz, uint32_t& capacity)
{
   U_TRACE(0, "UMemoryPool::_malloc_str(%lu,%p)", sz, &capacity)

   void* ptr;

   if (sz > U_MAX_SIZE_PREALLOCATE) ptr = U_SYSCALL(malloc, "%u", (capacity = sz));
   else
      {
      int stack_index;

      if (sz <= U_STACK_TYPE_6) // 128
         {
         capacity    = U_STACK_TYPE_6;
         stack_index = 6;
         }
      else if (sz <= U_STACK_TYPE_9) // 256
         {
         capacity    = U_STACK_TYPE_9;
         stack_index = 9;
         }
      else if (sz <= U_STACK_TYPE_10) // 512
         {
         capacity    = U_STACK_TYPE_10;
         stack_index = 10;
         }
      else if (sz <= U_STACK_TYPE_11) // 1024
         {
         capacity    = U_STACK_TYPE_11;
         stack_index = 11;
         }
      else if (sz <= U_STACK_TYPE_12) // 2048
         {
         capacity    = U_STACK_TYPE_12;
         stack_index = 12;
         }
      else // if (sz <= U_STACK_TYPE_13) // 4096
         {
         capacity    = U_STACK_TYPE_13;
         stack_index = 13;
         }

      U_INTERNAL_DUMP("capacity = %u", capacity)

      ptr = UMemoryPool::pop(stack_index);
      }

   U_RETURN(ptr);
}

void UMemoryPool::_free_str(void* ptr, size_t sz)
{
   U_TRACE(0, "UMemoryPool::_free_str(%p,%lu)", ptr, sz)

   if (sz > U_MAX_SIZE_PREALLOCATE)
      {
      U_SYSCALL_VOID(free, "%p", ptr);
      }
   else
      {
      int stack_index;

      switch (sz)
         {
         case U_STACK_TYPE_6:  stack_index =  6; break; //  128
         case U_STACK_TYPE_9:  stack_index =  9; break; //  256
         case U_STACK_TYPE_10: stack_index = 10; break; //  512
         case U_STACK_TYPE_11: stack_index = 11; break; // 1024
         case U_STACK_TYPE_12: stack_index = 12; break; // 2048
         default:              stack_index = 13; break; // 4096
         }

      U_INTERNAL_DUMP("stack_index = %u", stack_index)

      UMemoryPool::push(ptr, stack_index);
      }
}

// struttura e classe che encapsula lo stack dinamico di puntatori a blocchi preallocati per un 'type' dimensione definito

typedef struct ustackmemorypool {
#ifdef DEBUG
   void* _this;
#endif
   void** pointer_block;
   uint32_t type, len, space;
   uint32_t index, index_stack_mem_block;
} ustackmemorypool;

/*
           --   --        --   --   --        --   --   --
  10      |xx| |  |  --  |  | |  | |  |      |  | |  | |  | -> space
   9      |xx| |  | |xx| |xx| |  | |xx|  --  |  | |xx| |  |
   8  --  |xx| |  | |xx| |xx| |xx| |xx| |  | |xx| |xx| |  |
   7 |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| -> len
   6 |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx|
   5 |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx|
   4 |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx|
   3 |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx|
   2 |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx|
   1 |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx| |xx|
      --   --   --   --   --   --   --   --   --   --   -- 
        4    8   16   32   64  128  256  512 1024 2048 4096 -> type
        0    1    2    3    4    5    6    7    8    9   10 -> index
                                 5    6    7    8    9   10 -> index_stack_mem_block
*/

class U_NO_EXPORT UStackMemoryPool {
public:
   // Check for memory error
   U_MEMORY_TEST

   void** pointer_block;
   uint32_t type, len, space;
   uint32_t index, index_stack_mem_block;

   bool isEmpty() const
      {
      U_TRACE(0, "UStackMemoryPool::isEmpty()")

      U_CHECK_MEMORY

      U_INTERNAL_DUMP("index = %u len = %u space = %u", index, len, space)

      bool result = (len == 0);

      U_RETURN(result);
      }

   bool isFull() const
      {
      U_TRACE(0, "UStackMemoryPool::isFull()")

      U_CHECK_MEMORY

      U_INTERNAL_DUMP("index = %u len = %u space = %u", index, len, space)

      bool result = (len == space);

      U_RETURN(result);
      }

   bool noFull() const  { return !isFull(); }
   bool noEmpty() const { return !isEmpty(); }

   void growPointerBlock()
      {
      U_TRACE(1, "UStackMemoryPool::growPointerBlock()")

      U_INTERNAL_ASSERT_EQUALS(len,space) // isFull()

      // si alloca lo space per numero totale puntatori (blocchi allocati precedentemente + un nuovo insieme di blocchi)
      // relativi a type 'dimensione' stack corrente...

      U_INTERNAL_DUMP("index = %u", index)

      uint32_t old_size = space * sizeof(void*);

      space *= 2;

      // NB: pop() non varia gli attributi 'pointer_block', 'space' e quindi a questo punto si puo' anche fare la pop su se stesso...

      void** new_pointer_block = (void**) UMemoryPool::_malloc(space * sizeof(void*));

      (void) U_SYSCALL(memcpy, "%p,%p,%u", new_pointer_block, pointer_block, old_size);

      void** old_pointer_block = pointer_block;

      pointer_block = new_pointer_block;

      // NB: a questo punto il nuovo stack e' pronto e si puo' anche fare la push su se stesso...

      UMemoryPool::_free(old_pointer_block, old_size);
      }

   void push(void* ptr)
      {
      U_TRACE(0, "UStackMemoryPool::push(%p)", ptr)

      U_INTERNAL_ASSERT_MINOR(index,U_NUM_STACK_TYPE)
      U_INTERNAL_ASSERT_DIFFERS(index,(uint32_t)UMemoryPool::index_stack_busy)
      U_INTERNAL_ASSERT_EQUALS(((long)ptr & (sizeof(long)-1)),0) // memory aligned

      if (isFull()) // len == space
         {
         growPointerBlock();
         }

#  ifdef DEBUG
      UMemoryPool::index_stack_busy = index;

      (void) U_SYSCALL(memset, "%p,%p,%u", ptr, 0, type);
#  endif

      pointer_block[len++] = ptr;

#  ifdef DEBUG
      UMemoryPool::index_stack_busy = -1;
#  endif
      }

   void allocateMemoryBlocks()
      {
      U_TRACE(1, "UStackMemoryPool::allocateMemoryBlocks()")

      U_INTERNAL_DUMP("index = %u index_stack_mem_block = %u", index, index_stack_mem_block)

      U_INTERNAL_ASSERT_EQUALS(len,0) // isEmpty()
      U_INTERNAL_ASSERT_DIFFERS(index,index_stack_mem_block)

      // si alloca un nuovo insieme di blocchi relativi a type stack corrente...

      char* block = (char*) (index_stack_mem_block > (U_NUM_STACK_TYPE -1)
                                 ? U_SYSCALL(malloc, "%u", index_stack_mem_block)
                                 : ((UStackMemoryPool*)(mem_stack+index_stack_mem_block))->pop());

      // ...che viene inizializzato suddividendolo in base al type 'dimensione' e assegnando i valori dei puntatori relativi nel nuovo stack

#  ifdef DEBUG
      UMemoryPool::index_stack_busy = index;
#  endif

      do {
         pointer_block[len++] = (void*) block;

         block += type;
         }
      while (len < U_NUM_ENTRY_MEM_BLOCK);

   // UMemoryPool::index_stack_busy = -1;
      }

   // NB: pop() non varia gli attributi 'pointer_block', 'space'

   void* pop();

   static char mem_block[U_SIZE_MEM_BLOCK];

   static ustackmemorypool mem_stack[U_NUM_STACK_TYPE];

   // NB: inizialmente viene allocato lo spazio per il doppio dei puntatori necessari (quelli cioe' che vengono inizializzati)...

   static void* mem_pointer_block[U_NUM_STACK_TYPE * U_NUM_ENTRY_MEM_BLOCK * 2];

#ifdef DEBUG

   // paint info

   static void paint()
      {
      U_TRACE(0, "UStackMemoryPool::paint()")

      int i;
      uint32_t max_space = 0;

      for (i = 0; i < U_NUM_STACK_TYPE; ++i) if (mem_stack[i].space > max_space) max_space = mem_stack[i].space;

      printf("       %s   %s   %s   %s   %s   %s   %s   %s   %s   %s\n",
             (mem_stack[ 0].space == max_space ? "--" : "  "),
             (mem_stack[ 1].space == max_space ? "--" : "  "),
             (mem_stack[ 2].space == max_space ? "--" : "  "),
             (mem_stack[ 3].space == max_space ? "--" : "  "),
             (mem_stack[ 4].space == max_space ? "--" : "  "),
             (mem_stack[ 5].space == max_space ? "--" : "  "),
             (mem_stack[ 6].space == max_space ? "--" : "  "),
             (mem_stack[ 7].space == max_space ? "--" : "  "),
             (mem_stack[ 8].space == max_space ? "--" : "  "),
             (mem_stack[ 9].space == max_space ? "--" : "  "));

      for (i = max_space-1; i >= 0; --i)
         {
         printf("%5u %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c\n", i+1,
             (mem_stack[ 0].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 0].space == (uint32_t)i ? "--" : (mem_stack[ 0].len > (uint32_t)i ? "xx" : "  ")),
             (mem_stack[ 0].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 1].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 1].space == (uint32_t)i ? "--" : (mem_stack[ 1].len > (uint32_t)i ? "xx" : "  ")),
             (mem_stack[ 1].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 2].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 2].space == (uint32_t)i ? "--" : (mem_stack[ 2].len > (uint32_t)i ? "xx" : "  ")),
             (mem_stack[ 2].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 3].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 3].space == (uint32_t)i ? "--" : (mem_stack[ 3].len > (uint32_t)i ? "xx" : "  ")),
             (mem_stack[ 3].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 4].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 4].space == (uint32_t)i ? "--" : (mem_stack[ 4].len > (uint32_t)i ? "xx" : "  ")),
             (mem_stack[ 4].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 5].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 5].space == (uint32_t)i ? "--" : (mem_stack[ 5].len > (uint32_t)i ? "xx" : "  ")),
             (mem_stack[ 5].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 6].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 6].space == (uint32_t)i ? "--" : (mem_stack[ 6].len > (uint32_t)i ? "xx" : "  ")),
             (mem_stack[ 6].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 7].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 7].space == (uint32_t)i ? "--" : (mem_stack[ 7].len > (uint32_t)i ? "xx" : "  ")),
             (mem_stack[ 7].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 8].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 8].space == (uint32_t)i ? "--" : (mem_stack[ 8].len > (uint32_t)i ? "xx" : "  ")),
             (mem_stack[ 8].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 9].space >  (uint32_t)i ?  '|' :  ' '),
             (mem_stack[ 9].space == (uint32_t)i ? "--" : (mem_stack[ 9].len > (uint32_t)i ? "xx" : "  ")),
             (mem_stack[ 9].space >  (uint32_t)i ?  '|' :  ' '));
         }

      printf("       --   --   --   --   --   --   --   --   --   --\n"
             "         8   16   32   64  128  256  512 1024 2048 4096\n");
      }

#endif
};

// NB: pop() non varia gli attributi 'pointer_block', 'space'

void* UStackMemoryPool::pop()
{
   U_TRACE(0, "UStackMemoryPool::pop()")

   U_INTERNAL_ASSERT_MINOR(index,U_NUM_STACK_TYPE)
   U_INTERNAL_ASSERT_DIFFERS(index,(uint32_t)UMemoryPool::index_stack_busy)

   if (isEmpty()) allocateMemoryBlocks(); // len == 0
#ifdef DEBUG
   else UMemoryPool::index_stack_busy = index;
#endif

   void* ptr = pointer_block[--len];

#ifdef DEBUG
   UMemoryPool::index_stack_busy = -1;
#endif

   U_INTERNAL_ASSERT_EQUALS(((long)ptr & (sizeof(long)-1)),0) // memory aligned

   U_RETURN(ptr);
}

void* UMemoryPool::pop(int stack_index)
{
   U_TRACE(0+256, "UMemoryPool::pop(%d)", stack_index)

   U_INTERNAL_ASSERT_MINOR(stack_index,U_NUM_STACK_TYPE)

   void* ptr = ((UStackMemoryPool*)(UStackMemoryPool::mem_stack+stack_index))->pop();

   U_RETURN(ptr);
}

void UMemoryPool::push(void* ptr, int stack_index)
{
   U_TRACE(0+256, "UMemoryPool::push(%p,%d)", ptr, stack_index)

   U_INTERNAL_ASSERT_MINOR(stack_index,U_NUM_STACK_TYPE)

   ((UStackMemoryPool*)(UStackMemoryPool::mem_stack+stack_index))->push(ptr);
}

void* UMemoryPool::_malloc(size_t sz)
{
   U_TRACE(0, "UMemoryPool::_malloc(%lu)", sz)

   void* ptr = (sz <= U_MAX_SIZE_PREALLOCATE
                  ? UMemoryPool::pop(findStackIndex(sz))
                  : U_SYSCALL(malloc, "%u", sz));

   U_RETURN(ptr);
}

void UMemoryPool::_free(void* ptr, size_t sz)
{
   U_TRACE(0, "UMemoryPool::_free(%p,%lu)", ptr, sz)

   if (sz <= U_MAX_SIZE_PREALLOCATE) UMemoryPool::push(ptr, findStackIndex(sz));
   else                              U_SYSCALL_VOID(free, "%p", ptr);
}

#ifdef DEBUG

void UMemoryPool::printInfo()
{
   U_TRACE(0+256, "UMemoryPool::printInfo()")

   UStackMemoryPool::paint();
}

#endif

#ifdef U_OVERLOAD_NEW_DELETE

void* UMemoryPool::_new(int stack_index)
{
   U_TRACE(0, "UMemoryPool::_new(%d)", stack_index)

   U_INTERNAL_ASSERT_MINOR(stack_index,U_NUM_STACK_TYPE)

   char* ptr = (char*) ((UStackMemoryPool*)(UStackMemoryPool::mem_stack+stack_index))->pop();

   *(int*)ptr = stack_index;

   ptr += sizeof(int);

   U_RETURN((void*)ptr);
}

void UMemoryPool::_delete(void* ptr)
{
   U_TRACE(0, "UMemoryPool::_delete(%p)", ptr)

#if __GNUC__ < 3
   if (ptr == 0) return;
#else
   U_INTERNAL_ASSERT_POINTER(ptr)
#endif

   char* lptr = (char*)ptr - sizeof(int);

// U_INTERNAL_DUMP("*lptr = %M", lptr, 2 * sizeof(int))

   uint32_t stack_index = *(uint32_t*)lptr;

   if (stack_index < U_NUM_STACK_TYPE)
      {
      ((UStackMemoryPool*)(UStackMemoryPool::mem_stack+stack_index))->push(lptr);
      }
   else
      {
      U_SYSCALL_VOID(free, "%p", ptr);
      }
}

#endif

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
#undef  U_TYPE
#define U_TYPE U_STACK_TYPE_10
#undef  U_SPACE
#define U_SPACE (mem_block + U_STACK_TYPE_0 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_1 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_2 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_3 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_4 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_5 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_6 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_7 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_8 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_9 * U_NUM_ENTRY_MEM_BLOCK)
#include "./memory_pool.dat"
#undef  U_TYPE
#define U_TYPE U_STACK_TYPE_11
#undef  U_SPACE
#define U_SPACE (mem_block + U_STACK_TYPE_0  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_1  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_2  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_3  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_4  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_5  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_6  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_7  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_8  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_9  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_10 * U_NUM_ENTRY_MEM_BLOCK)
#include "./memory_pool.dat"
#undef  U_TYPE
#define U_TYPE U_STACK_TYPE_12
#undef  U_SPACE
#define U_SPACE (mem_block + U_STACK_TYPE_0  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_1  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_2  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_3  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_4  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_5  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_6  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_7  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_8  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_9  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_10 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_11 * U_NUM_ENTRY_MEM_BLOCK)
#include "./memory_pool.dat"
#undef  U_TYPE
#define U_TYPE U_STACK_TYPE_13
#undef  U_SPACE
#define U_SPACE (mem_block + U_STACK_TYPE_0  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_1  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_2  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_3  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_4  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_5  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_6  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_7  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_8  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_9  * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_10 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_11 * U_NUM_ENTRY_MEM_BLOCK + \
                             U_STACK_TYPE_12 * U_NUM_ENTRY_MEM_BLOCK)
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
//    uint32_t index, index_stack_mem_block;
// } ustackmemorypool;

#define U_INDEX_STACK_MEM_BLOCK(type) (((type * U_NUM_ENTRY_MEM_BLOCK) > U_MAX_SIZE_PREALLOCATE) ? (type * U_NUM_ENTRY_MEM_BLOCK) : \
                                       U_SIZE_TO_STACK_INDEX(type * U_NUM_ENTRY_MEM_BLOCK))

ustackmemorypool UStackMemoryPool::mem_stack[U_NUM_STACK_TYPE] = { {
#ifdef DEBUG
   &mem_stack[0],
#endif
   mem_pointer_block + 0 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_0, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   0, U_INDEX_STACK_MEM_BLOCK(U_STACK_TYPE_0) }, {
#ifdef DEBUG
   &mem_stack[1],
#endif
   mem_pointer_block + 1 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_1, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   1, U_INDEX_STACK_MEM_BLOCK(U_STACK_TYPE_1) }, {
#ifdef DEBUG
   &mem_stack[2],
#endif
   mem_pointer_block + 2 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_2, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   2, U_INDEX_STACK_MEM_BLOCK(U_STACK_TYPE_2) }, {
#ifdef DEBUG
   &mem_stack[3],
#endif
   mem_pointer_block + 3 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_3, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   3, U_INDEX_STACK_MEM_BLOCK(U_STACK_TYPE_3) }, {
#ifdef DEBUG
   &mem_stack[4],
#endif
   mem_pointer_block + 4 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_4, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   4, U_INDEX_STACK_MEM_BLOCK(U_STACK_TYPE_4) }, {
#ifdef DEBUG
   &mem_stack[5],
#endif
   mem_pointer_block + 5 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_5, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   5, U_INDEX_STACK_MEM_BLOCK(U_STACK_TYPE_5) }, {
#ifdef DEBUG
   &mem_stack[6],
#endif
   mem_pointer_block + 6 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_6, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   6, U_INDEX_STACK_MEM_BLOCK(U_STACK_TYPE_6) }, {
#ifdef DEBUG
   &mem_stack[7],
#endif
   mem_pointer_block + 7 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_7, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   7, U_INDEX_STACK_MEM_BLOCK(U_STACK_TYPE_7) }, {
#ifdef DEBUG
   &mem_stack[8],
#endif
   mem_pointer_block + 8 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_8, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   8, U_INDEX_STACK_MEM_BLOCK(U_STACK_TYPE_8) }, {
#ifdef DEBUG
   &mem_stack[9],
#endif
   mem_pointer_block + 9 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_9, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   9, U_INDEX_STACK_MEM_BLOCK(U_STACK_TYPE_9) }, {
#ifdef DEBUG
   &mem_stack[10],
#endif
   mem_pointer_block + 10 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_10, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   10, U_INDEX_STACK_MEM_BLOCK(U_STACK_TYPE_10) }, {
#ifdef DEBUG
   &mem_stack[11],
#endif
   mem_pointer_block + 11 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_11, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   11, U_INDEX_STACK_MEM_BLOCK(U_STACK_TYPE_11) }, {
#ifdef DEBUG
   &mem_stack[12],
#endif
   mem_pointer_block + 12 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_12, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   12, U_INDEX_STACK_MEM_BLOCK(U_STACK_TYPE_12) }, {
#ifdef DEBUG
   &mem_stack[13],
#endif
   mem_pointer_block + 13 * U_NUM_ENTRY_MEM_BLOCK * 2,
   U_STACK_TYPE_13, U_NUM_ENTRY_MEM_BLOCK, U_NUM_ENTRY_MEM_BLOCK * 2,
   13, U_INDEX_STACK_MEM_BLOCK(U_STACK_TYPE_13) }
};
