// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    log.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_LOG_H
#define ULIB_LOG_H 1

#include <ulib/file.h>
#include <ulib/utility/lock.h>

class ULog;

typedef void (*vPFpf)(const ULog*);

class U_EXPORT ULog : public UFile {
public:

   typedef struct log_data {
      char* file_ptr;
      char* file_page;
   } log_data;

   static ULog* pthis;
   static ULock* lock;
   static log_data* ptr;
   static const char* fmt;
   static char* file_limit;
   static const char* prefix;
   static vPF backup_log_function;
   static bool bsyslog, log_data_mmap;

   // COSTRUTTORI

   ULog(const UString& path, uint32_t _size, const char* _prefix) : UFile(path)
      {
      U_TRACE_REGISTER_OBJECT(0, ULog, "%.*S,%u,%S", U_STRING_TO_TRACE(path), _size, _prefix)

      U_INTERNAL_ASSERT_EQUALS(ptr,0)

      prefix = _prefix;

      (void) ULog::open(_size);
      }

   ~ULog();

   // VARIE

   bool open(const UString& path, uint32_t _size = 1024 * 1024, mode_t mode = 0664) { UFile::setPath(path); return ULog::open(_size, mode); }

   static void close();
   static bool isSysLog() { return bsyslog; }

   static void reopen()
      {
      U_TRACE(0, "ULog::reopen()")

      U_INTERNAL_DUMP("pthis = %p", pthis)

      if (pthis            &&
          bsyslog == false &&
          file_limit == 0)
         {
         pthis->UFile::reopen();
         }
      }

   // manage shared log

   static bool isShared()
      {
      U_TRACE(0, "ULog::isShared()")

      U_INTERNAL_ASSERT_POINTER(pthis)

      bool result = pthis->lock->isShared();

      U_RETURN(result);
      }

   static bool isMemoryMapped()
      {
      U_TRACE(0, "ULog::isMemoryMapped()")

      U_INTERNAL_ASSERT_POINTER(pthis)

      bool result = (file_limit != 0);

      U_RETURN(result);
      }

   static void startup();
   static void setShared(log_data* log_data_ptr);

   // write with prefix

   static void log(const char* format, ...);           // (buffer write == 4096)

   // write direct without prefix

   static void write(const char* format, ...);         // (buffer write == 4096)
   static void write(const struct iovec* iov, int n);

   // flushes changes made to memory mapped log file back to disk

   static void msync();

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   bool open(uint32_t size, mode_t mode = 0664);

#ifdef USE_LIBZ
   static void backup(); // if overwrite log file compress it with gzip...

   static RETSIGTYPE handlerSIGUSR1(int signo) // manage signal SIGUSR1
      {
      U_TRACE(0, "ULog::handlerSIGUSR1(%d)", signo)

      backup();
      }
#endif

private:
   ULog(const ULog&) : UFile()  {}
   ULog& operator=(const ULog&) { return *this; }
};

#endif
