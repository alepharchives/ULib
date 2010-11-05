// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    ring_buffer.h - ring buffer implementation
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_RING_BUFFER_H
#define ULIB_RING_BUFFER_H 1

#include <ulib/file.h>
#include <ulib/utility/lock.h>

class UStreamPlugIn;

class U_EXPORT URingBuffer {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   typedef struct rbuf_data {
      fd_set readers; // bitmask
      int pwrite, readd_cnt;
      int pread[FD_SETSIZE];
   } rbuf_data;

   // Costruttori

   URingBuffer()
      {
      U_TRACE_REGISTER_OBJECT(0, URingBuffer, "")

      ptr = 0;
      }

   ~URingBuffer();

   // SERVICES

   int  open();            // Returns a read descriptor
   void init(int size);    // Initialize ring buffer
   void close(int readd);  // Close a read descriptor

   /**
    * Test whether buffer is empty
    */

   bool isEmpty(int readd)
      {
      U_TRACE(0, "URingBuffer::isEmpty(%d)", readd)

      U_INTERNAL_ASSERT_POINTER(ptr)

      U_INTERNAL_DUMP("pwrite = %d pread[%d] = %d", ptr->pwrite, readd, ptr->pread[readd])

      bool result = (ptr->pwrite == ptr->pread[readd]);

      U_RETURN(result);
      }

   /**
    * Return the number of bytes waiting in the buffer
    *
    * Example: read min. 1000, max. <bufsize> bytes
    *
    * if ((avail = rbuf.avail(readd)) >= 1000)
    *    count = rbuf.read(readd, buffer, min(avail, bufsize));
    * else
    *    ...
    */

   int avail(int readd)
      {
      U_TRACE(0, "URingBuffer::avail(%d)", readd)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_POINTER(ptr)
      U_INTERNAL_ASSERT_MINOR(readd, FD_SETSIZE)
      U_INTERNAL_ASSERT(FD_ISSET(readd, &(ptr->readers)))

      U_INTERNAL_DUMP("pwrite = %d pread[%d] = %d", ptr->pwrite, readd, ptr->pread[readd])

      int _avail = ptr->pwrite - ptr->pread[readd];

      if (_avail < 0) _avail += size;

      U_RETURN(_avail);
      }

   /**
    * Return the number of free bytes in the buffer
    *
    * Example: write <buflen> bytes
    *
    * if (rbuf.free() >= buflen)
    *    count = rbuff.write(readd, buffer, buflen);
    * else
    *    ...
    */

   int free()
      {
      U_TRACE(0, "URingBuffer::free()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_POINTER(ptr)

      U_INTERNAL_DUMP("pwrite = %d", ptr->pwrite)

      int _free = min_pread() - ptr->pwrite;

      if (_free <= 0) _free += size;

      U_RETURN(_free-1);
      }

   /**
    * Write <len> bytes to ring buffer + packet header (2 bytes) if specified
    *
    * Return returns number of bytes transferred
    */

   int write(const char* buf, int len, bool pkt);

   int write(const UString& _buf, bool pkt) { return write(_buf.data(), _buf.size(), pkt); }

   int readFromFdAndWrite(int fd);

   /**
    * Read <len> bytes from ring buffer into <buf> or if <len> == -1 return the first packet
    *
    * Returns number of bytes transferred
    */

   int read(int readd, char* buf, int len);

   int readAndWriteToFd(int readd, int fd);

   // STREAM

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   char* ptrd;
   ULock lock;
   int size;
   rbuf_data* ptr;

private:
   /**
    * Return the read descriptor that have the major number of bytes waiting in the buffer
    */

   int min_pread() U_NO_EXPORT;

   /**
    * If there is exactly one reader and one writer, there is no need to lock read or write operations.
    */

   void checkLocking() U_NO_EXPORT;

   URingBuffer(const URingBuffer&)            {}
   URingBuffer& operator=(const URingBuffer&) { return *this; }

   friend class UStreamPlugIn;
};

#endif
