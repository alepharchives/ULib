// ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    base_trace.c
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================
 
#include <ulib/base/trace.h>

#ifndef __MINGW32__
#  include <sys/uio.h>
#  include <sys/mman.h>
#endif

#include <stdlib.h>
#include <stddef.h>

int      u_trace_fd;
char     u_trace_tab[256]; /* 256 max indent */
uint32_t u_trace_num_tab;

static int      level_active;
static uint32_t file_size;

static void printInfo(void)
{
   U_INTERNAL_TRACE("printInfo()", 0)

   /* segnala caratteristiche esecuzione modalita' trace */

   if (u_trace_fd > 0)
      {
      U_MESSAGE("TRACE<%Won%W>: Level<%W%d%W> MaxSize<%W%d%W>%W", GREEN, YELLOW, CYAN, level_active,
                                                                  YELLOW, CYAN, file_size, YELLOW, RESET);
      }
   else
      {
      U_MESSAGE("TRACE<%Woff%W>%W", RED, YELLOW, RESET);
      }
}

static char* file_mem;
static char* file_ptr;
static char* file_limit;

void u_trace_check_if_interrupt(void) /* check for context manage signal event - interrupt */
{
   U_INTERNAL_TRACE("u_trace_check_if_interrupt()", 0)

   if (file_size            &&
       file_ptr != file_mem &&
       *(file_ptr-1) != '\n')
      {
      *file_ptr++ = '\n';
      }
}

void u_trace_writev(const struct iovec* iov, int n)
{
   U_INTERNAL_TRACE("u_trace_writev(%p,%d)", iov, n)

   U_INTERNAL_ASSERT_MINOR(u_trace_num_tab,sizeof(u_trace_tab))

   if (file_size == 0) writev(u_trace_fd, iov, n);
   else
      {
      int i = 0;

      for (; i < n; ++i)
         {
      /* U_INTERNAL_PRINT("iov[%d].iov_len=%d ov[%d].iov_base=%p", i, iov[i].iov_len, i, iov[i].iov_base) */

         U_INTERNAL_ASSERT_MAJOR(iov[i].iov_len,0)
         U_INTERNAL_ASSERT_POINTER(iov[i].iov_base)

         if ((file_ptr + iov[i].iov_len) > file_limit) file_ptr = file_mem;

         (void) memcpy(file_ptr, iov[i].iov_base, iov[i].iov_len);

         file_ptr += iov[i].iov_len;
         }
      }
}

void u_trace_write(const char* t, uint32_t tlen)
{
   struct iovec iov[3] = { { (caddr_t)u_trace_tab, u_trace_num_tab },
                           { (caddr_t)t, tlen },
                           { (caddr_t)"\n", 1 } };

   U_INTERNAL_TRACE("u_trace_write(%s,%u)", t, tlen)

   u_trace_writev(iov, 3);
}

void u_trace_close(void)
{
   /* NB: disattivazione immediata trace nel caso di ricezione signal nelle call system chiamate in questo metodo... */

   int lfd = u_trace_fd;
             u_trace_fd = 0;

   U_INTERNAL_TRACE("u_trace_close()", 0)

   if (lfd > STDERR_FILENO)
      {
      if (file_size)
         {
         ptrdiff_t write_size = file_ptr - file_mem;

         U_INTERNAL_ASSERT_MINOR(write_size,(ptrdiff_t)file_size)

         (void)  msync(file_mem, write_size, MS_SYNC);
         (void) munmap(file_mem, file_size);

         (void) ftruncate(lfd, write_size);
         (void) fsync(lfd);

         file_size = 0;
         }

      (void) close(lfd);
      }
}

/* E' possibile mascheratura del trace per metodi eccessivamente complessi
(Es: ricorsivi) tramite il byte alto del parametro 'level' */

static void* flag_mask_level;

/* attivazione-disattivazione trace */
 
static bool flag_signal; /* tramite signal SIGUSR2 */
static struct sigaction act;

static RETSIGTYPE handlerSIGUSR2(int signo)
{
   U_INTERNAL_TRACE("handlerSIGUSR2(%d)", signo)

   flag_signal = true;
}

static void setHandlerSIGUSR2(void)
{
   U_INTERNAL_TRACE("setHandlerSIGUSR2()", 0)

   act.sa_handler = handlerSIGUSR2;

   (void) sigaction(SIGUSR2, &act, 0);
}

static bool flag_init;

void u_trace_init(bool force, bool info, bool offset)
{
   char* env = getenv("UTRACE");

   U_INTERNAL_TRACE("u_trace_init(%d,%d,%d)", force, info, offset)

   U_INTERNAL_ASSERT_RANGE(0,info,1)
   U_INTERNAL_ASSERT_RANGE(0,force,1)
   U_INTERNAL_ASSERT_RANGE(0,offset,1)

   flag_init = true;

   if ( env &&
       *env)
      {
      char suffix;

      if (*env == '"' || *env == '\'') ++env; // normalizzazione...

      if (*env == '-')
         {
         ++env;

         u_trace_fd = STDERR_FILENO;
         }

      /* format: <level> <max_size_log> <u_flag_test>
                    -1        500k           0
      */

      (void) sscanf(env, "%d%d%c%d", &level_active, &file_size, &suffix, &u_flag_test);

      if (file_size) U_NUMBER_SUFFIX(file_size, suffix);
      }
   else
      {
      level_active = (force ? 0 : -1);
      }

   (void) memset(u_trace_tab, '\t', sizeof(u_trace_tab));

   if (level_active >= 0)
      {
      flag_mask_level = 0; /* NB: necessary check for incoerent state... */

      if (u_trace_fd == STDERR_FILENO) file_size = 0;
      else
         {
         char name[128];

         (void) u_snprintf(name, 128, "trace.%N.%P", 0);

         /* NB: O_RDWR e' necessario per mmap(MAP_SHARED)... */

         u_trace_fd = open(name, O_CREAT | O_RDWR | O_BINARY | (offset ? O_APPEND : 0), 0666);

         if (u_trace_fd == -1) U_WARNING("error on create file <%s>", name);
         else
            {
            /* gestione dimensione massima... */

            if (file_size)
               {
               off_t start = (offset ? lseek(u_trace_fd, 0, SEEK_END) : 0);

               if (ftruncate(u_trace_fd, file_size))
                  {
                  U_WARNING("out of space on file system, (required %lu bytes)", file_size);

                  file_size = 0;
                  }
               else
                  {
                  /* NB: PROT_READ evita strani SIGSEGV... */

                  file_mem = (char*) mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, u_trace_fd, 0);

                  if (file_mem == MAP_FAILED)
                     {
                     file_mem  = 0;
                     file_size = 0;

                     (void) ftruncate(u_trace_fd, 0);
                     }

                  file_ptr   = file_mem + start;
                  file_limit = file_mem + file_size;
                  }
               }
            }
         }

      if (u_trace_fd > STDERR_FILENO)
         {
         /* register function of close trace at exit... */

         u_atexit(&u_trace_close);
         }
      }

   if (info) printInfo();

   /* si abilita attivazione-disattivazione tramite signal SIGUSR2 */

   setHandlerSIGUSR2();
}

static void handlerSignal(void)
{
   U_INTERNAL_TRACE("handlerSignal()", 0)

   if (u_trace_fd <= 0 ||
       level_active < 0)
      {
      (void) putenv((char*)"UTRACE=0");

      u_trace_init(true, true, false);
      }
   else
      {
      (void) putenv((char*)"UTRACE");

      u_trace_close();

      level_active = -1;

      printInfo();
      }

   flag_signal = false;

#ifdef __MINGW32__
   setHandlerSIGUSR2();
#endif
}

bool u_trace_isActive(int level)
{
   bool result = (u_trace_fd > 0      &&
                 flag_mask_level == 0 &&
                 (level & 0x000000ff) >= level_active);

   U_INTERNAL_TRACE("u_trace_isActive(%d)", level)

   U_INTERNAL_PRINT("fd=%d level_active=%d flag_mask_level=%p result=%d", u_trace_fd, level_active, flag_mask_level, result)

   return result;
}

bool u_trace_check_if_active(int level, void* hook)
{
   bool active;

   U_INTERNAL_TRACE("u_trace_check_if_active(%d,%p)", level, hook)

   if (flag_init)
      {
      if (flag_signal) handlerSignal();
      }
   else
      {
      u_trace_init(false, false, false);
      }

   active = (level == -1 ? (u_flag_test > 0 && level_active == 0) : u_trace_isActive(level));

   if (active)
      {
      if (u_flag_test == 0 &&
          (level & 0x0000ff00))
         {
         flag_mask_level = hook;
         }

      return true;
      }

   return false;
}

void u_trace_check_init(void)
{
   U_INTERNAL_TRACE("u_trace_check_init()", 0)

   // controllo se sono avvenute precedenti creazioni di oggetti globali
   // che possono avere forzato l'inizializzazione del file di trace...

   U_INTERNAL_ASSERT_RANGE(0,flag_init,1)

   if (flag_init         &&
       level_active >= 0 &&
       u_trace_fd != STDERR_FILENO)
      {
      char name[128];

      (void) u_snprintf(name, 128, "trace.%N.%P", 0);

      (void) rename("trace..", name);
      }
   else
      {
      u_trace_init(false, false, false);
      }

   printInfo();
}

void u_trace_dtor(int active, void* hook)
{
   U_INTERNAL_TRACE("u_trace_dtor(%d,%p)", active, hook)

   U_INTERNAL_ASSERT_RANGE(0,active,1)

   if (active && flag_mask_level == hook) flag_mask_level = 0;

   if (flag_signal) handlerSignal();
}

void u_trace_suspend(int resume)
{
   U_INTERNAL_TRACE("u_trace_suspend(%d)", resume)

   U_INTERNAL_ASSERT_RANGE(0,resume,1)

   if (u_flag_test ==  0 &&
       u_flag_exit != -1)
      {
      static int   cnt_suspend;           /* disabilita eventuale ricorsione... */
      static void* flag_mask_level_save;

      if (resume)
         {
         if (--cnt_suspend == 0)
            {
            flag_mask_level      = (flag_mask_level_save != (void*)0x0001 ? flag_mask_level_save : 0);
            flag_mask_level_save = 0;
            }
         }
      else
         {
         if (++cnt_suspend == 1)
            {
            flag_mask_level_save = (flag_mask_level != (void*)0x0001 ? flag_mask_level : 0);
            flag_mask_level      =                     (void*)0x0001;
            }
         }

      U_INTERNAL_PRINT("cnt_suspend=%d flag_mask_level=%p flag_mask_level_save=%p",
                           cnt_suspend, flag_mask_level, flag_mask_level_save)

      U_INTERNAL_ASSERT(cnt_suspend >= 0)
      }
}

void u_trace_dump(const char* format, ...)
{
   va_list argp;
   char buffer[4096];
   uint32_t buffer_len;

   U_INTERNAL_TRACE("u_trace_dump(%s)", format)

   va_start(argp, format);

   buffer_len = u_vsnprintf(buffer, 4096, format, argp);

   va_end(argp);

   {
   struct iovec iov[3] = { { (caddr_t)u_trace_tab, u_trace_num_tab },
                           { (caddr_t)buffer, buffer_len },
                           { (caddr_t)"\n", 1 } };

   u_trace_writev(iov, 3);
   }
}

void u_trace_initFork(void)
{
   U_INTERNAL_TRACE("u_trace_initFork()", 0)

   if (u_trace_fd > STDERR_FILENO)
      {
      if (file_size)
         {
         (void) munmap(file_mem, file_size);
                                 file_size = 0;
         }

      (void) close(u_trace_fd);
                   u_trace_fd = 0;
      }

   u_trace_init(false, true, false);
}
