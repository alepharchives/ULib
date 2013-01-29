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
class UBackupThread;

typedef void (*vPFpf)(const ULog*);

class U_EXPORT ULog : public UFile {
public:

   typedef struct log_data {
      char* file_ptr;
      char* file_page;
      sem_t lock_shared;
   } log_data;

   static ULog* pthis;
   static ULock* lock;
   static const char* fmt;
   static char* LOG_FILE_SZ;
   static uint32_t map_size;
   static const char* prefix;
   static const char* dir_log_gz;
   static log_data* ptr_log_data;
   static vPF backup_log_function;
   static bool bsyslog, log_data_must_be_unmapped;

   // COSTRUTTORI

   ULog(const UString& path, uint32_t _size, const char* _prefix) : UFile(path)
      {
      U_TRACE_REGISTER_OBJECT(0, ULog, "%.*S,%u,%S", U_STRING_TO_TRACE(path), _size, _prefix)

      U_INTERNAL_ASSERT_EQUALS(ptr_log_data,0)

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
          LOG_FILE_SZ == 0)
         {
         pthis->UFile::reopen(O_CREAT | O_RDWR | O_APPEND);
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

      bool result = (LOG_FILE_SZ != 0);

      U_RETURN(result);
      }

   static void msync(); // flushes changes made to memory mapped log file back to disk
   static void startup();
   static void setShared(log_data* ptr);

   // write with prefix

   static void log(const char* format, ...);           // (buffer write == 4096)

   // write direct without prefix

   static void write(const char* format, ...);         // (buffer write == 4096)
   static void write(const struct iovec* iov, int n);

   // logger

   static int getPriorityForLogger(const char* s) __pure; // decode a symbolic name to a numeric value

   static void logger(const char* ident, int priority, const char* format, ...); // (buffer write == 4096)

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   bool open(uint32_t size, mode_t mode = 0664);

#ifdef USE_LIBZ
   static void* pthread;
   static uint32_t path_compress;
   static UString* buffer_path_compress;

   // if overwrite log file compress it as gzip...

   static void backup();
   static void backup_write(UString& pathfile, UString& data);

   static RETSIGTYPE handlerSIGUSR1(int signo) // manage signal SIGUSR1
      {
      U_TRACE(0, "ULog::handlerSIGUSR1(%d)", signo)

      backup();
      }
#endif

private:
   static int decode(const char* name, uint32_t len, bool bfacility) __pure U_NO_EXPORT;

   ULog(const ULog&) : UFile()  {}
   ULog& operator=(const ULog&) { return *this; }

   friend class UBackupThread;
};

#endif
