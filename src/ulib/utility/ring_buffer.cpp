// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    ring_buffer.cpp - ring buffer implementation
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/utility/ring_buffer.h>

URingBuffer::~URingBuffer()
{
   U_TRACE_UNREGISTER_OBJECT(0, URingBuffer)

   if (ptr) UFile::munmap(ptr, size + sizeof(rbuf_data));
}

// Initialize ring buffer and queue

void URingBuffer::init(int _size)
{
   U_TRACE(0, "URingBuffer::init(%d)", _size)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_EQUALS(ptr, 0)
   U_INTERNAL_ASSERT_MAJOR(_size, 0)

   ptr = (rbuf_data*) UFile::mmap(_size);

   U_INTERNAL_ASSERT_DIFFERS(ptr,MAP_FAILED)

   size = _size      - sizeof(rbuf_data);
   ptrd = (char*)ptr + sizeof(rbuf_data);
}

// Returns a read descriptor

int URingBuffer::open()
{
   U_TRACE(0, "URingBuffer::open()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(ptr)

   for (int i = 0; i < FD_SETSIZE; ++i)
      {
      if (FD_ISSET(i, &(ptr->readers)) == false)
         {
         FD_SET(i, &(ptr->readers));

         ptr->pread[i] = ptr->pwrite; // NB: start to read from here...

         ptr->readd_cnt++;

         checkLocking();

         U_RETURN(i);
         }
      }

   U_RETURN(-1);
}

// Close a read descriptor

void URingBuffer::close(int readd)
{
   U_TRACE(0, "URingBuffer::close(%d)", readd)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(ptr)
   U_INTERNAL_ASSERT_MINOR(readd, FD_SETSIZE)
   U_INTERNAL_ASSERT(FD_ISSET(readd, &(ptr->readers)))

   FD_CLR(readd, &(ptr->readers));

   ptr->readd_cnt--;

   checkLocking();
}

U_NO_EXPORT void URingBuffer::checkLocking()
{
   U_TRACE(0, "URingBuffer::checkLocking()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(ptr)

   U_INTERNAL_DUMP("readd_cnt = %d", ptr->readd_cnt)

   if (lock.isShared() == false)
      {
      if (ptr->readd_cnt  > 1) lock.init(0);
      }
   else
      {
      if (ptr->readd_cnt <= 1) lock.destroy();
      }
}

U_NO_EXPORT int URingBuffer::min_pread()
{
   U_TRACE(0, "URingBuffer::min_pread()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(ptr)

   int _avail, _readd_cnt = 0, min_pread = 0, min_avail = -1;

   for (int i = 0; i < FD_SETSIZE; ++i)
      {
      if (FD_ISSET(i, &(ptr->readers)))
         {
         _avail = avail(i);

         if (_avail > min_avail)
            {
            min_avail = _avail;
            min_pread = ptr->pread[i];
            }

         if (++_readd_cnt >= ptr->readd_cnt) break;
         }
      }

   U_RETURN(min_pread);
}

#define U_RINGBUFFER_PKTHDRSIZE 2

int URingBuffer::write(const char* buf, int len, bool pkt)
{
   U_TRACE(0, "URingBuffer::write(%.*S,%u,%b)", len, buf, len, pkt)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(ptr)

   int todo = len, _free, split;

   if (pkt)
      {
      // Length of buffer currently limited to 65535 bytes max

      U_INTERNAL_ASSERT_MINOR(len, 65535)

      len += U_RINGBUFFER_PKTHDRSIZE;
      }

   lock.lock();

   // NB: check write size and number of bytes free...

   _free = free();

   if (len > _free) len = _free;

   if (len == 0)
      {
      pkt = false;

      goto end;
      }

   split = ((ptr->pwrite + len) > size ? size - ptr->pwrite : 0);

   U_INTERNAL_DUMP("split = %d", split)

   if (pkt)
      {
      // Write a packet header into the ringbuffer

      if (len < U_RINGBUFFER_PKTHDRSIZE) U_RETURN(-1);

      char* _ptr = ptrd + ptr->pwrite;

      _ptr[0] = len >> 8;
      _ptr[1] = len & 0xff;

      ptr->pwrite = (ptr->pwrite + U_RINGBUFFER_PKTHDRSIZE) % size;

      split -= U_RINGBUFFER_PKTHDRSIZE;
      }

   if (split > 0)
      {
      (void) U_SYSCALL(memcpy, "%p,%p,%u", ptrd + ptr->pwrite, buf, split);

      buf  += split;
      todo -= split;

      ptr->pwrite = 0;
      }

   (void) U_SYSCALL(memcpy, "%p,%p,%u", ptrd + ptr->pwrite, buf, todo);

   ptr->pwrite = (ptr->pwrite + todo) % size;

end:
   lock.unlock();

   if (pkt) len -= U_RINGBUFFER_PKTHDRSIZE;

   U_RETURN(len);
}

int URingBuffer::readFromFdAndWrite(int fd)
{
   U_TRACE(0, "URingBuffer::readFromFdAndWrite(%d)", fd)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(ptr)

   int todo, split, nread = -1;

   lock.lock();

   // NB: check write size and number of bytes free...

   todo = free();

   if (todo == 0) goto end;

   errno = 0;
   split = ((ptr->pwrite + todo) > size ? size - ptr->pwrite : 0);

   U_INTERNAL_DUMP("split = %d", split)

   if (split > 0)
      {
      nread = U_SYSCALL(read, "%d,%p,%u", fd, ptrd + ptr->pwrite, split);

      if (nread <= 0) goto end;

      ptr->pwrite = (ptr->pwrite + nread) % size;

      if (split > nread) goto end;

      todo -= nread;
      }

   todo = U_SYSCALL(read, "%d,%p,%u", fd, ptrd + ptr->pwrite, todo);

   if (todo > 0)
      {
      nread      += todo;
      ptr->pwrite = (ptr->pwrite + todo) % size;
      }

end:
   lock.unlock();

   U_RETURN(nread);
}

int URingBuffer::read(int readd, char* buf, int len)
{
   U_TRACE(0, "URingBuffer::read(%d,%p,%d)", readd, buf, len)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(ptr)
   U_INTERNAL_ASSERT_MINOR(readd, FD_SETSIZE)
   U_INTERNAL_ASSERT(FD_ISSET(readd, &(ptr->readers)))

   if (len == -1)
      {
      // Read a packet header from the ringbuffer

      if ((ptr->pread[readd] + U_RINGBUFFER_PKTHDRSIZE) > size) U_RETURN(-1);

      char* _ptr = ptrd + ptr->pread[readd];

      len = (_ptr[0] << 8) + _ptr[1];

      U_INTERNAL_DUMP("len = %d", len)

      U_INTERNAL_ASSERT_MINOR(len, 65535)

      ptr->pread[readd] = (ptr->pread[readd] + U_RINGBUFFER_PKTHDRSIZE) % size;
      }

   int todo = len, _avail, split;

   lock.lock();

   // NB: check read size and number of bytes available...

   _avail = avail(readd);

   if (len > _avail) len = _avail;

   if (len == 0) goto end;

   split = ((ptr->pread[readd] + len) > size ? size - ptr->pread[readd] : 0);

   U_INTERNAL_DUMP("split = %d", split)

   if (split > 0)
      {
      (void) U_SYSCALL(memcpy, "%p,%p,%u", buf, ptrd + ptr->pread[readd], split);

      buf  += split;
      todo -= split;

      ptr->pread[readd] = 0;
      }

   (void) U_SYSCALL(memcpy, "%p,%p,%u", buf, ptrd + ptr->pread[readd], todo);

   ptr->pread[readd] = (ptr->pread[readd] + todo) % size;

end:
   lock.unlock();

   U_RETURN(len);
}

int URingBuffer::readAndWriteToFd(int readd, int fd)
{
   U_TRACE(0, "URingBuffer::readAndWriteToFd(%d,%d)", readd, fd)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(ptr)

   int todo, split, nwrite = 0;

   lock.lock();

   // NB: check write size and number of bytes free...

   todo  = avail(readd);
   errno = 0;

   if (todo == 0) goto end;

   split = ((ptr->pread[readd] + todo) > size ? size - ptr->pread[readd] : 0);

   U_INTERNAL_DUMP("split = %d", split)

   if (split > 0)
      {
      nwrite = U_SYSCALL(write, "%d,%p,%u", fd, ptrd + ptr->pread[readd], split);

      if (nwrite <= 0) goto end;

      ptr->pread[readd] = (ptr->pread[readd] + nwrite) % size;

      if (split > nwrite) goto end;

      todo -= nwrite;
      }

   todo = U_SYSCALL(write, "%d,%p,%u", fd, ptrd + ptr->pread[readd], todo);

   if (todo > 0)
      {
      nwrite           += todo;
      ptr->pread[readd] = (ptr->pread[readd] + todo) % size;
      }

end:
   lock.unlock();

   U_RETURN(nwrite);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* URingBuffer::dump(bool reset) const
{
   *UObjectIO::os << "ptr         " << (void*)ptr   << '\n'
                  << "size        " << size         << '\n'
                  << "lock (ULock " << (void*)&lock << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif