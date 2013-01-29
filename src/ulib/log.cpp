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

#ifndef __MINGW32__
#  ifdef __clang__
#  undef  NULL
#  define NULL 0
#  endif
#  define SYSLOG_NAMES
#  include <syslog.h>
#  include <sys/utsname.h>
#endif

#ifdef USE_LIBZ // check for crc32
#  include <ulib/utility/interrupt.h>
#  include <ulib/utility/string_ext.h>
#  include <zlib.h>
void*           ULog::pthread;
uint32_t        ULog::path_compress;
UString*        ULog::buffer_path_compress;
#endif

vPF             ULog::backup_log_function;
bool            ULog::bsyslog;
bool            ULog::log_data_must_be_unmapped;
ULog*           ULog::pthis;
char*           ULog::LOG_FILE_SZ;
ULock*          ULog::lock;
uint32_t        ULog::map_size;
const char*     ULog::fmt = "*** %s %N (%ubit, pid %P) [%U@%H] ***\n";
const char*     ULog::prefix;
const char*     ULog::dir_log_gz;
ULog::log_data* ULog::ptr_log_data;

#define LOG_ptr  ptr_log_data->file_ptr
#define LOG_page ptr_log_data->file_page

#if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD)
#  include <ulib/thread.h>

class UBackupThread : public UThread {
public:

   UBackupThread() : UThread(true, false) {}

   UString data_to_write;

   virtual void run()
      {
      U_TRACE(0, "UBackupThread::run()")

loop:
      suspend();

      if (data_to_write.empty() == false) ULog::backup_write(*ULog::buffer_path_compress, data_to_write);

      goto loop;
      }
};
#endif

ULog::~ULog()
{
   U_TRACE_UNREGISTER_OBJECT(0, ULog)

   u_unatexit(&ULog::close); // unregister function of close at exit()...

   ULog::close();

   if (lock) delete lock;

#ifdef USE_LIBZ
   if (buffer_path_compress) delete buffer_path_compress;
#endif
}

void ULog::startup()
{
   U_TRACE(0, "ULog::startup()")

   U_INTERNAL_ASSERT_POINTER(fmt)

   log(fmt, "STARTUP", sizeof(void*) * 8);

   log("Building Environment: " PLATFORM_VAR " (" __DATE__ ")\n", 0);

#ifndef __MINGW32__
   struct utsname u;

   (void) U_SYSCALL(uname, "%p", &u);

   log("Current Operating System: %s %s v%s %s\n", u.sysname, u.machine, u.version, u.release);
#endif

#if defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)
   log("Big endian arch detected\n", 0);
#endif
}

bool ULog::open(uint32_t _size, mode_t mode)
{
   U_TRACE(0, "ULog::open(%u,%d)", _size, mode)

   // NB: we need to check because all instance try to close the log... (inherits from its parent)

   u_atexit(&ULog::close); // register function of close at exit()...

   if (UFile::getPath() == U_STRING_FROM_CONSTANT("syslog"))
      {
      prefix  = 0;
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

      pthis        = this;
      ptr_log_data = U_MALLOC_TYPE(log_data);

      LOG_ptr = LOG_page = (_size ? (UFile::map + file_size)      : 0); // append mode...
             LOG_FILE_SZ = (_size ? (UFile::map + UFile::st_size) : 0);

      U_INTERNAL_ASSERT_EQUALS(lock,0)

      lock = U_NEW(ULock);

      U_INTERNAL_ASSERT_POINTER(lock)

#  ifdef USE_LIBZ
      char suffix[32];
      uint32_t len_suffix = u__snprintf(suffix, sizeof(suffix), ".%4D.gz");

      buffer_path_compress = U_NEW(UString(MAX_FILENAME_LEN));

      char* ptr = buffer_path_compress->data();

      dir_log_gz = (const char*) U_SYSCALL(getenv, "%S", "DIR_LOG_GZ");

      if (dir_log_gz == 0)
         {
         (void) UFile::setPathFromFile(*pthis, ptr, suffix, len_suffix);

         buffer_path_compress->size_adjust();

         path_compress = (buffer_path_compress->size() - len_suffix - 1);
         }
      else
         {
         UString name = pthis->UFile::getName();
         uint32_t len = u__strlen(dir_log_gz), sz = name.size();

         U__MEMCPY(ptr, dir_log_gz, len);

          ptr  += len;
         *ptr++ = '/';

         buffer_path_compress->size_adjust(len + 1 + sz + len_suffix);

         U__MEMCPY(ptr, name.data(), sz);
                   ptr += sz;
         U__MEMCPY(ptr, suffix, len_suffix);

         path_compress = buffer_path_compress->distance(ptr) + 1;

         buffer_path_compress->UString::setNullTerminated();
         }

      U_INTERNAL_DUMP("buffer_path_compress(%u) = %.*S path_compress = %u",
                       buffer_path_compress->size(), U_STRING_TO_TRACE(*buffer_path_compress), path_compress)

#  if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD)
      U_INTERNAL_ASSERT_EQUALS(pthread, 0)

      U_NEW_ULIB_OBJECT(pthread, UBackupThread);

      U_INTERNAL_DUMP("pthread = %p", pthread)

      ((UThread*)pthread)->start();
#  endif

      backup_log_function = &ULog::backup;

      UInterrupt::setHandlerForSignal(SIGUSR1, (sighandler_t)ULog::handlerSIGUSR1);
#  endif

      if (fmt) startup();

      U_RETURN(true);
      }

end:
   U_ERROR("cannot init log file %.*S...", U_FILE_TO_TRACE(*this));

   U_RETURN(false);
}

void ULog::setShared(log_data* ptr)
{
   U_TRACE(0, "ULog::setShared(%p)", ptr)

   if (LOG_FILE_SZ &&
       bsyslog == false)
      {
      U_INTERNAL_ASSERT_POINTER(lock)
      U_INTERNAL_ASSERT_POINTER(ptr_log_data)

      if (ptr == 0)
         {
         map_size = sizeof(log_data);

         ptr = (log_data*) UFile::mmap(&map_size);

         U_INTERNAL_ASSERT_DIFFERS(ptr, MAP_FAILED)

         log_data_must_be_unmapped = true;
         }

      ptr->file_ptr  = LOG_ptr;
      ptr->file_page = LOG_page;

      U_FREE_TYPE(ptr_log_data, log_data);

      ptr_log_data = ptr;

      lock->init(&(ptr_log_data->lock_shared));
      }
}

void ULog::msync()
{
   U_TRACE(0, "ULog::msync()")

   if (bsyslog == false)
      {
      U_INTERNAL_ASSERT_POINTER(ptr_log_data)

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

   if (LOG_FILE_SZ == 0) (void) pthis->UFile::writev(iov, n);
   else
      {
      lock->lock();

      U_INTERNAL_DUMP("LOG_ptr = %p", LOG_ptr)

      for (int i = 0; i < n; ++i)
         {
         if ((LOG_ptr + iov[i].iov_len) > LOG_FILE_SZ)
            {
            if (backup_log_function) backup_log_function();

            LOG_ptr = LOG_page = pthis->UFile::map;
            }

         U__MEMCPY(LOG_ptr, iov[i].iov_base, iov[i].iov_len);
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

   struct iovec iov[1] = { { (caddr_t)buffer, u__vsnprintf(buffer, sizeof(buffer), format, argp) } };

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

   if (prefix) iov[0].iov_len  = u__snprintf( buffer,                  sizeof(buffer),                  prefix, 0);
               iov[0].iov_len += u__vsnprintf(buffer + iov[0].iov_len, sizeof(buffer) - iov[0].iov_len, format, argp);

   va_end(argp);

   write(iov, 1);
}

void ULog::logger(const char* ident, int priority, const char* format, ...)
{
   U_TRACE(0, "ULog::logger(%S,%d,%S)", ident, priority, format)

#ifndef __MINGW32__
   if (format == 0)
      {
      U_SYSCALL_VOID(openlog, "%S,%d,%d", ident, 0, 0);

      return;
      }

   uint32_t len;
   char buffer[4096];

   va_list argp;
   va_start(argp, format);

   len = u__vsnprintf(buffer, sizeof(buffer), format, argp);

   va_end(argp);

   U_SYSCALL_VOID(syslog, "%d,%S,%d,%p", priority, "%.*s", len, buffer);
#endif
}

// Decode a symbolic name to a numeric value

__pure U_NO_EXPORT int ULog::decode(const char* name, uint32_t len, bool bfacility)
{
   U_TRACE(0, "ULog::decode(%.*S,%u,%b)", len, name, len, bfacility)

#ifndef __MINGW32__
   for (CODE* c = (bfacility ? facilitynames : prioritynames); c->c_name; ++c)
      {
      if (strncasecmp(name, c->c_name, len) == 0) U_RETURN(c->c_val);
      }
#endif

   U_RETURN(-1);
}

__pure int ULog::getPriorityForLogger(const char* s)
{
   U_TRACE(0, "ULog::getPriorityForLogger(%S)", s)

   int res;

#ifndef __MINGW32__
   int fac, lev;
   const char* ptr;

   for (ptr = s; *ptr && *ptr != '.'; ++ptr) {}

   if (*ptr)
      {
      U_INTERNAL_ASSERT_EQUALS(*ptr, '.')

      fac = decode(s, ptr++ - s, true);
      }
   else
      {
      ptr = s;
      fac = LOG_USER;
      }

   lev = decode(s, ptr - s, false);

   res = ((lev & LOG_PRIMASK) | (fac & LOG_FACMASK));
#endif

   U_RETURN(res);
}

void ULog::close()
{
   U_TRACE(0, "ULog::close()")

   if (pthis)
      {
      U_INTERNAL_DUMP("pthis = %p", pthis)

      if (bsyslog)
         {
#     ifndef __MINGW32__
         U_SYSCALL_VOID_NO_PARAM(closelog);
#     endif
         }
      else
         {
         U_INTERNAL_ASSERT_POINTER(pthis)

         lock->lock();

         if (fmt) log(fmt, "SHUTDOWN", sizeof(void*) * 8);

         if (LOG_FILE_SZ)
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

         if (log_data_must_be_unmapped)
            {
            U_ASSERT(lock->isShared())

            UFile::munmap(ptr_log_data, map_size);
            }
         }
      }
}

#ifdef USE_LIBZ
void ULog::backup_write(UString& pathfile, UString& data)
{
   U_TRACE(0, "ULog::backup_write(%.*S,%.*S)", U_STRING_TO_TRACE(pathfile), U_STRING_TO_TRACE(data))

   (void) u__snprintf(pathfile.c_pointer(path_compress), 17, "%4D");

   U_ASSERT_EQUALS(UFile::access(pathfile.data(), R_OK | W_OK), false)

   (void) UFile::writeTo(pathfile, data);

   data.clear();
}

void ULog::backup()
{
   U_TRACE(0, "ULog::backup()")

   U_INTERNAL_DUMP("pthis = %p bsyslog = %b", pthis, bsyslog)

   if (pthis &&
       bsyslog == false)
      {
      uint32_t size;
      bool locked = lock->isLocked();

      if (LOG_FILE_SZ)
         {
         if (locked == false) lock->lock();

         size = LOG_ptr - pthis->UFile::map;
         }
      else
         {
         size = pthis->UFile::size();

         if (pthis->UFile::memmap(PROT_READ | PROT_WRITE) == false) return;
         }

      U_INTERNAL_ASSERT(size <= pthis->UFile::st_size)

      UString data_to_write = UStringExt::deflate(pthis->UFile::map, size, true);

#  if defined(HAVE_PTHREAD_H) && defined(ENABLE_THREAD)
      if (((UBackupThread*)pthread)->data_to_write.empty())
         {
         ((UBackupThread*)pthread)->data_to_write = data_to_write;

         ((UBackupThread*)pthread)->resume();
         }
      else
         {
         UString x = buffer_path_compress->copy();
#  else
         {
         UString x = *buffer_path_compress;
#  endif

         ULog::backup_write(x, data_to_write);
         }

      if (LOG_FILE_SZ)
         {
         if (locked == false) lock->unlock();
         }
      else
         {
                pthis->UFile::munmap();
         (void) pthis->UFile::ftruncate(0);
         }
      }
}
#endif

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* ULog::dump(bool _reset) const
{
   U_CHECK_MEMORY

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
                  << "LOG_FILE_SZ               " << (void*)LOG_FILE_SZ         << '\n'
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
