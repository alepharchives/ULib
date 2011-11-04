// ============================================================================
//
// = LIBRARY
//    ulibdbg - c++ library
//
// = FILENAME
//    objectDB.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/base/hash.h>
#include <ulib/base/utility.h>

#include <ulib/debug/objectDB.h>

#ifndef __MINGW32__
#  include <sys/uio.h>
#  include <sys/mman.h>
#endif

#include <stdlib.h>
#include <stddef.h>

typedef bool (*vPFpObjectDumpable)(const UObjectDumpable*);

class U_NO_EXPORT UHashMapObjectDumpable {
public:
   const UObjectDumpable* objDumper;

   UHashMapObjectDumpable()
      {
      U_INTERNAL_TRACE("UHashMapObjectDumpable::UHashMapObjectDumpable()")
      }

   ~UHashMapObjectDumpable()
      {
      U_INTERNAL_TRACE("UHashMapObjectDumpable::~UHashMapObjectDumpable()")
      }

   // Services

   static UHashMapObjectDumpable* node;
   static UHashMapObjectDumpable** table;
   static uint32_t index, random, num, counter, table_size;

   static void init(uint32_t size)
      {
      U_INTERNAL_TRACE("UHashMapObjectDumpable::init(%u)", size)

      table_size = U_GET_NEXT_PRIME_NUMBER(size);
      table      = (UHashMapObjectDumpable**) calloc(table_size, sizeof(UHashMapObjectDumpable*));
      }

   static void lookup(const void* ptr_object)
      {
      U_INTERNAL_TRACE("UHashMapObjectDumpable::lookup(%p)", ptr_object)

      // This hash function is designed so that a changing just one bit in input 'i' will potentially affect the
      // every bit in hash(i), and the correlation between succesive hashes is (hopefully) extremely small (if not zero)

#  ifdef HAVE_ARCH64
      random = u_random64((uint64_t)ptr_object);
#  else
      random = u_random(  (uint32_t)ptr_object);
#  endif

      index = random % table_size;

      U_INTERNAL_PRINT("index = %u", index)

      U_INTERNAL_ASSERT_MINOR(index,table_size)

      for (node = table[index]; node; node = node->next)
         {
         if (node->objDumper->ptr_object == ptr_object) break;
         }

      U_INTERNAL_PRINT("node = %p", node)
      }

   static void resize()
      {
      U_INTERNAL_TRACE("UHashMapObjectDumpable::resize()")

      uint32_t                 old_table_size = table_size;
      UHashMapObjectDumpable** old_table      = table;

      init(old_table_size);

      // inserisco i vecchi elementi

      UHashMapObjectDumpable* next;

      for (uint32_t i = 0; i < old_table_size; ++i)
         {
         if (old_table[i])
            {
            node = old_table[i];

            do {
               next  = node->next;
               index = (node->hash % table_size);

               U_INTERNAL_PRINT("hash = %u i = %u index = %u", node->hash, i, index)

               // antepongo l'elemento all'inizio della lista delle collisioni

               node->next   = table[index];
               table[index] = node;
               }
            while ((node = next));
            }
         }

      free(old_table);
      }

   static void insert(UObjectDumpable* dumper)
      {
      U_INTERNAL_TRACE("UHashMapObjectDumpable::insert(%p)", dumper)

      if (num > table_size) resize();

      lookup(dumper->ptr_object);

      // This is for to cope with hierarchies of dumpable classes. In such cases we typically want only one dump,
      // corresponding to the most derived instance. To achieve this, the handle registered for the subobject
      // corresponding to the base class is overwritten (hence on destruction of the subobject its handle won't
      // exist anymore and we'll have to check for that)

      if (node)
         {
         U_INTERNAL_PRINT("ptr_object = %p base_class = %s derived_class = %s",
                           dumper->ptr_object, node->objDumper->name_class, dumper->name_class);

         delete node->objDumper;
         }
      else
         {
         node = new UHashMapObjectDumpable;

         // antepongo l'elemento all'inizio della lista delle collisioni

         node->hash   = random;
         node->next   = table[index];
         table[index] = node;

         ++num;
         }

      node->objDumper = dumper;

      dumper->cnt = ++counter;

      U_INTERNAL_PRINT("num = %u counter = %u", num, counter)
      }

   static void erase(const void* ptr_object)
      {
      U_INTERNAL_TRACE("UHashMapObjectDumpable::erase(%p)", ptr_object)

      lookup(ptr_object);

      if (node == 0) return;

      UHashMapObjectDumpable* prev = 0;

      for (UHashMapObjectDumpable* pnode = table[index]; pnode; pnode = pnode->next)
         {
         if (pnode == node)
            {
            U_INTERNAL_ASSERT_EQUALS(pnode->objDumper->ptr_object, ptr_object)

            /* lista self-organizing (move-to-front), antepongo l'elemento
             * trovato all'inizio della lista delle collisioni...
             */

            if (prev)
               {
               prev->next   = pnode->next;
               node->next   = table[index];
               table[index] = pnode;
               }

            U_INTERNAL_ASSERT_EQUALS(node,table[index])

            break;
            }

         prev = pnode;
         }

      U_INTERNAL_PRINT("prev = %p", prev)

      /* presuppone l'elemento da cancellare all'inizio della lista
       * delle collisioni - lista self-organizing (move-to-front)...
       */

      U_INTERNAL_ASSERT_EQUALS(node,table[index])

      table[index] = node->next;

      delete node->objDumper;
      delete node;

      --num;

      U_INTERNAL_PRINT("num = %u", num)
      }

   static void callForAllEntry(vPFpObjectDumpable function)
      {
      U_INTERNAL_TRACE("UHashMapObjectDumpable::callForAllEntry(%p)", function)

      U_INTERNAL_PRINT("num = %u", num)

      if (num)
         {
         int sum = 0, max = 0, min = 1024, width;

         for (index = 0; index < table_size; ++index)
            {
            if (table[index])
               {
               node = table[index];

               ++sum;

               width = -1;

               do {
                  ++width;

                  if (function(node->objDumper) == false) return;
                  }
               while ((node = node->next));

               if (max < width) max = width;
               if (min > width) min = width;
               }
            }

         U_INTERNAL_PRINT("collision(min,max) = (%d,%d) - distribution = %f", min, max, (double)num / (double)sum)
         }
      }

private:
   UHashMapObjectDumpable* next;
   uint32_t hash;
};

U_NO_EXPORT uint32_t     UHashMapObjectDumpable::num;
U_NO_EXPORT uint32_t     UHashMapObjectDumpable::index;
U_NO_EXPORT uint32_t     UHashMapObjectDumpable::random;
U_NO_EXPORT uint32_t     UHashMapObjectDumpable::counter;
U_NO_EXPORT uint32_t     UHashMapObjectDumpable::table_size;
UHashMapObjectDumpable*  UHashMapObjectDumpable::node;
UHashMapObjectDumpable** UHashMapObjectDumpable::table;

U_EXPORT    int      UObjectDB::fd;
U_EXPORT    int      UObjectDB::level_active;
U_EXPORT    bool     UObjectDB::flag_new_object;
U_EXPORT    bool     UObjectDB::flag_ulib_object;

U_NO_EXPORT char*    UObjectDB::file_ptr;
U_NO_EXPORT char*    UObjectDB::file_mem;
U_NO_EXPORT char*    UObjectDB::file_limit;
U_NO_EXPORT uint32_t UObjectDB::file_size;

U_NO_EXPORT char     UObjectDB::buffer1[64];
U_NO_EXPORT char     UObjectDB::buffer2[256];

U_NO_EXPORT char*    UObjectDB::lbuf;
U_NO_EXPORT char*    UObjectDB::lend;
U_NO_EXPORT bPFpcpv  UObjectDB::checkObject;

U_NO_EXPORT iovec    UObjectDB::liov[7] = {
   { 0, 0 },
   { (caddr_t) UObjectDB::buffer1, 0 },
   { (caddr_t) UObjectDB::buffer2, 0 },
   { 0, 0 },
   { (caddr_t)  U_CONSTANT_TO_PARAM(U_LF2) },
   { 0, 0 },
   { (caddr_t) "\n---------------------------------------"
               "-----------------------------------------\n", 82 }
};

void UObjectDB::init(bool flag, bool info)
{
   U_INTERNAL_TRACE("UObjectDB::init(%d,%d)", flag, info)

   char* env = getenv("UOBJDUMP");

   if ( env &&
       *env)
      {
      if (*env == '"' || *env == '\'') ++env; // normalizzazione...

      // format: <level_active> <max_size_log> <table_size>
      //                1           500k           100

      char suffix;
      char name[128];
      uint32_t table_size = 0;

      (void) sscanf(env, "%d%u%c%u", &level_active, &file_size, &suffix, &table_size);

      if (file_size) U_NUMBER_SUFFIX(file_size, suffix);

      (void) u_sn_printf(name, 128, "object.%N.%P", 0);

      // NB: O_RDWR e' necessario per mmap(MAP_SHARED)...

      fd = open(name, O_CREAT | O_RDWR | O_BINARY, 0666);

      if (fd != -1)
         {
         // gestione dimensione massima...

         if (file_size)
            {
            if (ftruncate(fd, file_size))
               {
               U_WARNING("out of space on file system, (required %u bytes)", file_size);

               file_size = 0;
               }
            else
               {
               /* NB: PROT_READ evita strani SIGSEGV... */

               file_mem = (char*) mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

               if (file_mem == MAP_FAILED)
                  {
                  file_mem  = 0;
                  file_size = 0;

                  (void) ftruncate(fd, file_size);
                  }

               file_ptr   = file_mem;
               file_limit = file_mem + file_size;
               }
            }
         }

      if (fd > 0)
         {
         if (flag)
            {
            UHashMapObjectDumpable::init(table_size);

            u_atexit(&UObjectDB::close); // register function of close dump at exit...
            }
         }
      }

   // segnala caratteristiche esecuzione modalita' dump oggetti live...

   if (info)
      {
      if (fd >0) U_MESSAGE("OBJDUMP<%Won%W>: Level<%W%d%W> MaxSize<%W%d%W>%W",GREEN,YELLOW,CYAN,level_active,YELLOW,CYAN,file_size,YELLOW,RESET);
      else       U_MESSAGE("OBJDUMP<%Woff%W>%W",RED,YELLOW,RESET);
      }
}

U_NO_EXPORT void UObjectDB::_write(const struct iovec* iov, int _n)
{
   U_INTERNAL_TRACE("UObjectDB::_write(%p,%d)", iov, _n)

   if (file_size == 0) (void) writev(fd, iov, _n);
   else
      {
      for (int i = 0; i < _n; ++i)
         {
         U_INTERNAL_ASSERT_RANGE(1,iov[i].iov_len,file_size)

         if ((file_ptr + iov[i].iov_len) > file_limit) file_ptr = file_mem;

         (void) u_memcpy(file_ptr, iov[i].iov_base, iov[i].iov_len);

         file_ptr += iov[i].iov_len;
         }
      }
}

void UObjectDB::close()
{
   U_INTERNAL_TRACE("UObjectDB::close()")

   if (fd > 0)
      {
      dumpObjects();

      if (file_size)
         {
         ptrdiff_t write_size = file_ptr - file_mem;

         U_INTERNAL_ASSERT_MINOR(write_size,(ptrdiff_t)file_size)

         (void)  msync(file_mem, write_size, MS_SYNC);
         (void) munmap(file_mem, file_size);

         (void) ftruncate(fd, write_size);
         (void) fsync(fd);

         file_size = 0;
         }

      (void) ::close(fd);

      fd = 0;
      }
}

void UObjectDB::initFork()
{
   U_INTERNAL_TRACE("UObjectDB::initFork()")

   U_INTERNAL_ASSERT_RANGE(1,fd,1024)

   if (file_size)
      {
      (void) munmap(file_mem, file_size);

      file_size = 0;
      }

   (void) ::close(fd);

   init(false, true);
}

void UObjectDB::registerObject(UObjectDumpable* dumper)
{
   U_INTERNAL_TRACE("UObjectDB::registerObject(%p)", dumper)

   if (flag_ulib_object) dumper->level = -1;

   dumper->num_line      = u_num_line;
   dumper->name_file     = u_name_file;
   dumper->name_function = u_name_function;

   UHashMapObjectDumpable::insert(dumper);
}

void UObjectDB::unregisterObject(const void* ptr_object)
{
   U_INTERNAL_TRACE("UObjectDB::unregisterObject(%p)", ptr_object)

   UHashMapObjectDumpable::erase(ptr_object);
}

// dump

void UObjectDB::dumpObject(const UObjectDumpable* dumper)
{
   U_INTERNAL_TRACE("UObjectDB::dumpObject(%p)", dumper)

   U_INTERNAL_ASSERT(dumper->level >= level_active)

   liov[0].iov_len  =  u_str_len(dumper->name_class);
   liov[0].iov_base = (caddr_t) dumper->name_class;

   (void) sprintf(buffer1, " %p size %d level %d", // cnt %09d",
                  dumper->ptr_object, dumper->size_object, dumper->level); //, dumper->cnt);

   (void) sprintf(buffer2, "\n%s(%d)\n", dumper->name_file, dumper->num_line);

   liov[1].iov_len = u_str_len(buffer1);
   liov[2].iov_len = u_str_len(buffer2);

   liov[3].iov_len  = u_str_len(dumper->name_function);
   liov[3].iov_base = (caddr_t) dumper->name_function;

   liov[5].iov_base = (caddr_t) dumper->dump();
   liov[5].iov_len  = u_str_len((const char*)liov[5].iov_base);
}

// dump single object...

U_NO_EXPORT bool UObjectDB::printObjLive(const UObjectDumpable* dumper)
{
   U_INTERNAL_TRACE("UObjectDB::printObjLive(%p)", dumper)

   if (dumper->level >= level_active &&
       checkObject(dumper->name_class, dumper->ptr_object))
      {
      dumpObject(dumper);

      for (int i = 0; i < 7; ++i)
         {
         if ((lbuf + liov[i].iov_len) > lend) return false;

         (void) u_memcpy(lbuf, liov[i].iov_base, liov[i].iov_len);

         lbuf += liov[i].iov_len;
         }
      }

   return true;
}

uint32_t UObjectDB::dumpObject(char* buffer, uint32_t buffer_size, bPFpcpv check_object)
{
   U_INTERNAL_TRACE("UObjectDB::dumpObject(%p,%u,%p)", buffer, buffer_size, check_object)

   lbuf        = buffer;
   lend        = buffer + buffer_size;
   checkObject = check_object;

   UHashMapObjectDumpable::callForAllEntry(UObjectDB::printObjLive);

   U_INTERNAL_ASSERT(lbuf >= buffer)

   uint32_t result = (lbuf - buffer);

   return result;
}

// sorting object live for time creation...

uint32_t                UObjectDB::n;
const UObjectDumpable** UObjectDB::vec_obj_live;

U_NO_EXPORT bool UObjectDB::addObjLive(const UObjectDumpable* dumper)
{
   U_INTERNAL_TRACE("UObjectDB::addObjLive(%p)", dumper)

   if (n < 8192)
      {
      vec_obj_live[n++] = dumper;

      return true;
      }

   return false;
}

U_NO_EXPORT int __pure UObjectDB::compareDumper(const void* dumper1, const void* dumper2)
{
   U_INTERNAL_TRACE("UObjectDB::compareDumper(%p,%p)", dumper1, dumper2)

   int cmp = ((*(const UObjectDumpable**)dumper1)->cnt <
              (*(const UObjectDumpable**)dumper2)->cnt ? -1 : 1);

   return cmp;
}

void UObjectDB::dumpObjects()
{
   U_INTERNAL_TRACE("UObjectDB::dumpObjects()")

   const UObjectDumpable* obj_live[8192];

   vec_obj_live = &obj_live[n = 0];

   UHashMapObjectDumpable::callForAllEntry(UObjectDB::addObjLive);

   U_INTERNAL_ASSERT(n <= UHashMapObjectDumpable::num)

   qsort(obj_live, n, sizeof(const UObjectDumpable*), compareDumper);

   const UObjectDumpable* dumper;

   for (uint32_t i = 0; i < n; ++i)
      {
      dumper = obj_live[i];

      if (dumper->level >= level_active)
         {
         dumpObject(dumper);

         _write(liov, 7);
         }
      }
}
