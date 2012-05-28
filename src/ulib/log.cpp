// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    log.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/log.h>
#include <ulib/utility/lock.h>

#ifdef USE_LIBZ // check for crc32
#  include <ulib/utility/interrupt.h>
#  include <zlib.h>
#endif

#ifndef __MINGW32__
#  include <syslog.h>
#  include <sys/utsname.h>
#endif

vPF             ULog::backup_log_function;
bool            ULog::bsyslog;
ULog*           ULog::pthis;
char*           ULog::file_limit;
ULock*          ULog::lock;
const char*     ULog::fmt = "*** %s %N (pid %P) [%U@%H] ***\n";
const char*     ULog::prefix;
ULog::log_data* ULog::ptr;

#define LOG_ptr  ptr->file_ptr
#define LOG_page ptr->file_page

ULog::~ULog()
{
   U_TRACE_UNREGISTER_OBJECT(0, ULog)

   u_unatexit(&ULog::close); // unregister function of close at exit()...

   ULog::close();

   if (lock) delete lock;
}

bool ULog::open(uint32_t _size, mode_t mode)
{
   U_TRACE(0, "ULog::open(%u,%d)", _size, mode)

   if (UFile::getPath() == U_STRING_FROM_CONSTANT("syslog"))
      {
      pthis   = this;
      bsyslog = true;

#  ifndef __MINGW32__
      U_SYSCALL_VOID(openlog, "%S,%d,%d", u_progname, LOG_PID, LOG_LOCAL0);
#  endif

      U_RETURN(true);
      }

   if (UFile::creat(O_CREAT | O_RDWR | O_APPEND, mode))
      {
      uint32_t file_size = 0;

      if (_size)
         {
         file_size = UFile::size();

         if (UFile::ftruncate(_size)               == false ||
             UFile::memmap(PROT_READ | PROT_WRITE) == false)
            {
            goto end;
            }
         }

      pthis = this;
      ptr   = U_MALLOC_TYPE(log_data);

      LOG_ptr = LOG_page = (_size ? (UFile::map + file_size)      : 0); // append mode...
              file_limit = (_size ? (UFile::map + UFile::st_size) : 0);

      U_INTERNAL_ASSERT_EQUALS(lock,0)

      lock = U_NEW(ULock);

      U_INTERNAL_ASSERT_POINTER(lock)

      U_RETURN(true);
      }

end:
   U_ERROR("cannot init log file %.*S...", U_FILE_TO_TRACE(*this));

   U_RETURN(false);
}

void ULog::setShared()
{
   U_TRACE(0, "ULog::setShared()")

   if (file_limit &&
       bsyslog == false)
      {
      U_INTERNAL_ASSERT_POINTER(ptr)
      U_INTERNAL_ASSERT_POINTER(lock)

      log_data* tmp = ptr;

      ptr = (log_data*) UFile::mmap(sizeof(log_data) + sizeof(sem_t));

      U_INTERNAL_ASSERT_DIFFERS(ptr,MAP_FAILED)

      *ptr = *tmp; // copy structure...

      U_FREE_TYPE(tmp, log_data);

      lock->init((sem_t*)((char*)ptr + sizeof(log_data)));
      }
}

void ULog::init()
{
   U_TRACE(1, "ULog::init()")

   if (bsyslog) prefix = 0;
   else
      {
      U_INTERNAL_ASSERT_POINTER(pthis)
      U_INTERNAL_ASSERT_DIFFERS(pthis->UFile::fd,-1)

#  ifdef USE_LIBZ
      backup_log_function = &ULog::backup;

      UInterrupt::setHandlerForSignal(SIGUSR1, (sighandler_t)ULog::handlerSIGUSR1);
#  endif
      }

   if (fmt)
      {
      log(fmt, "STARTUP");

      log("Building Environment: " PLATFORM_VAR " (" __DATE__ ")\n", 0);

#  ifndef __MINGW32__
      struct utsname u;

      (void) U_SYSCALL(uname, "%p", &u);

      log("Current Operating System: %s %s v%s %s\n", u.sysname, u.machine, u.version, u.release);
#  endif
      }

   // NB: we need to check because all instance try to close the log... (inherits from its parent)

   u_atexit(&ULog::close); // register function of close at exit()...
}

void ULog::msync()
{
   U_TRACE(0, "ULog::msync()")

   if (bsyslog == false)
      {
      U_INTERNAL_ASSERT_POINTER(ptr)

      UFile::msync(LOG_ptr, LOG_page);

      LOG_page = LOG_ptr;
      }
}

void ULog::write(const struct iovec* iov, int n)
{
   U_TRACE(0, "ULog::write(%p,%d)", iov, n)

   U_INTERNAL_ASSERT_POINTER(pthis)

   if (bsyslog)
      {
#  ifndef __MINGW32__
      for (int i = 0; i < n; ++i) U_SYSCALL_VOID(syslog, "%d,%S,%d,%p", LOG_INFO, "%.*s", (int)iov[i].iov_len, (char*)iov[i].iov_base);
#  endif

      return;
      }

   if (file_limit == 0) (void) pthis->UFile::writev(iov, n);
   else
      {
      lock->lock();

      U_INTERNAL_DUMP("LOG_ptr = %p", LOG_ptr)

      for (int i = 0; i < n; ++i)
         {
         if ((LOG_ptr + iov[i].iov_len) > file_limit)
            {
            if (backup_log_function) backup_log_function();

            LOG_ptr = LOG_page = pthis->UFile::map;
            }

         (void) u__memcpy(LOG_ptr, iov[i].iov_base, iov[i].iov_len);
                          LOG_ptr +=                iov[i].iov_len;

         U_INTERNAL_DUMP("memcpy() -> %.*S", iov[i].iov_len, LOG_ptr - iov[i].iov_len)
         }

      U_INTERNAL_DUMP("LOG_ptr = %p", LOG_ptr)

      lock->unlock();
      }
}

void ULog::write(const char* format, ...)
{
   U_TRACE(0, "ULog::write(%S)", format)

   char buffer[4096];

   va_list argp;
   va_start(argp, format);

   struct iovec iov[1] = { { (caddr_t)buffer, u_vsnprintf(buffer, sizeof(buffer), format, argp) } };

   va_end(argp);

   write(iov, 1);
}

void ULog::log(const char* format, ...)
{
   U_TRACE(0, "ULog::log(%S)", format)

   char buffer[4096];
   struct iovec iov[1] = { { (caddr_t)buffer, 0 } };

   va_list argp;
   va_start(argp, format);

   if (prefix) iov[0].iov_len  = u__snprintf(buffer,                  sizeof(buffer),                  prefix, 0);
               iov[0].iov_len += u_vsnprintf(buffer + iov[0].iov_len, sizeof(buffer) - iov[0].iov_len, format, argp); 

   va_end(argp);

   write(iov, 1);
}

void ULog::close()
{
   U_TRACE(0, "ULog::close()")

   if (pthis)
      {
      U_INTERNAL_DUMP("pthis = %p", pthis)

      if (bsyslog == false) lock->lock();

      if (fmt) log(fmt, "SHUTDOWN");

      if (bsyslog == false)
         {
         U_INTERNAL_ASSERT_POINTER(pthis)

         if (file_limit)
            {
            uint32_t size = LOG_ptr - pthis->UFile::map;

            U_INTERNAL_ASSERT_MINOR(size, pthis->UFile::st_size)

            ULog::msync();

                   pthis->UFile::munmap();
            (void) pthis->UFile::ftruncate(size);
                   pthis->UFile::fsync();
            }

         pthis->UFile::close();

         pthis = 0;

         lock->unlock();

         if (lock->isShared()) UFile::munmap(ptr, sizeof(log_data) + sizeof(sem_t));

         return;
         }

#  ifndef __MINGW32__
      U_SYSCALL_VOID_NO_PARAM(closelog);
#  endif
      }
}

#ifdef USE_LIBZ
void ULog::backup()
{
   U_TRACE(1, "ULog::backup()")

   U_INTERNAL_DUMP("pthis = %p bsyslog = %b", pthis, bsyslog)

   if (pthis == 0 || bsyslog) return;

   static const char* dir_log_gz = (const char*) U_SYSCALL(getenv, "%S", "DIR_LOG_GZ");

   uint32_t len_suffix;
   char suffix[32], buffer_path[MAX_FILENAME_LEN];
   char* path = buffer_path;

   bool locked = lock->isLocked();

   if (file_limit &&
       locked == false)
      {
      lock->lock();
      }

   len_suffix = u__snprintf(suffix, sizeof(suffix), ".%6D.gz");

   if (dir_log_gz)
      {
      UString name = pthis->UFile::getName();
      uint32_t len = u__strlen(dir_log_gz), sz = name.size();

      (void) U_SYSCALL(u__memcpy, "%p,%p,%u", path, dir_log_gz, len);

       path  += len;
      *path++ = '/';

      len += 1 + sz + len_suffix;

      U_INTERNAL_ASSERT_MINOR(len, (int32_t)MAX_FILENAME_LEN)

      (void) U_SYSCALL(u__memcpy, "%p,%p,%u", path, name.data(), sz);
      (void) U_SYSCALL(u__memcpy, "%p,%p,%u", path+sz,   suffix, len_suffix);

      buffer_path[len] = '\0';

      U_INTERNAL_DUMP("buffer_path(%u) = %S", len, buffer_path)
      }
   else
      {
      (void) UFile::setPathFromFile(*pthis, path, suffix, len_suffix);
      }

   U_ASSERT_EQUALS(UFile::access(buffer_path, R_OK | W_OK), false)

   gzFile fp = (gzFile) U_SYSCALL(gzopen, "%S,%S", buffer_path, "wb");

   if (fp)
      {
      uint32_t size;

      if (file_limit)
         {
         size = LOG_ptr - pthis->UFile::map;
         }
      else
         {
         size = pthis->UFile::size();

         if (pthis->UFile::memmap(PROT_READ | PROT_WRITE) == false) return;
         }

      U_INTERNAL_ASSERT(size <= pthis->UFile::st_size)

#  ifdef DEBUG
      int value = U_SYSCALL(gzwrite, "%p,%p,%u", fp, pthis->UFile::map, size);
      U_INTERNAL_ASSERT_EQUALS(value,(int)size)
#  else
      (void) U_SYSCALL(gzwrite, "%p,%p,%u", fp, pthis->UFile::map, size);
#  endif

      (void) U_SYSCALL(gzclose, "%p", fp);

      if (file_limit == 0)
         {
                pthis->UFile::munmap();
         (void) pthis->UFile::ftruncate(0);
         }
      }

   if (file_limit &&
       locked == false)
      {
      lock->unlock();
      }
}
#endif

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* ULog::dump(bool _reset) const
{
   UFile::dump(false);

   *UObjectIO::os << '\n'
                  << "fmt                       " << '"';

   char buffer[1024];

   if (fmt) UObjectIO::os->write(buffer, u__snprintf(buffer, 1024, "%.*s", u__strlen(fmt), fmt));

   *UObjectIO::os << "\"\n"
                  << "prefix                    ";

   if (prefix) UObjectIO::os->write(buffer, u__snprintf(buffer, 1024, "%.*S", u__strlen(prefix), prefix));

   *UObjectIO::os << '\n'
                  << "bsyslog                   " << bsyslog                    << '\n'
                  << "file_limit                " << (void*)file_limit          << '\n'
                  << "backup_log_function       " << (void*)backup_log_function << '\n'
                  << "lock        (ULock        " << (void*)lock                << ")\n"
                  << "pthis       (ULog         " << (void*)pthis               << ')';
                  /*
                  << "LOG_ptr                   " << ((void*)LOG_ptr)           << '\n'
                  << "LOG_page                  " << ((void*)LOG_page)          << '\n'
                  */

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
