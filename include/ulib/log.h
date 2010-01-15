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
   static bool bsyslog;
   static log_data* ptr;
   static const char* fmt;
   static char* file_limit;
   static const char* prefix;
   static vPF backup_log_function;

   // COSTRUTTORI

   ULog()
      {
      U_TRACE_REGISTER_OBJECT(0, ULog, "", 0)

      U_INTERNAL_ASSERT_EQUALS(ptr,0)
      }

   ULog(const UString& path, uint32_t size = 1024 * 1024) : UFile(path)
      {
      U_TRACE_REGISTER_OBJECT(0, ULog, "%.*S,%u", U_STRING_TO_TRACE(path), size)

      U_INTERNAL_ASSERT_EQUALS(ptr,0)

      (void) ULog::open(size);
      }

   ~ULog();

   // VARIE

   bool open(const UString& path, uint32_t size = 1024 * 1024, mode_t mode = 0664) { UFile::setPath(path); return ULog::open(size, mode); }

   static void init();
   static void close();
   static void reopen();
   static bool isSysLog() { return bsyslog; }

   // manage shared log

   static void setShared();

   // set log mode

   static void setClient()
      {
      U_TRACE(0, "ULog::setClient()")

      prefix = "%4D> ";

      init();
      }

   static void setServer(bool shared)
      {
      U_TRACE(0, "ULog::setServer(%b)", shared)

      if (shared) setShared();

      prefix = "(pid %P) %4D> ";

      init();
      }

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

#ifdef HAVE_LIBZ
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
