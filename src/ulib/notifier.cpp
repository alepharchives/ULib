// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    notifier.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/notifier.h>
#include <ulib/utility/interrupt.h>

#include <errno.h>

int        UNotifier::result;
int        UNotifier::fd_set_max;
int        UNotifier::fd_read_cnt;
int        UNotifier::fd_write_cnt;
bool       UNotifier::exit_loop_wait_event_for_signal;
fd_set     UNotifier::fd_set_read;
fd_set     UNotifier::fd_set_write;
UNotifier* UNotifier::pool;
UNotifier* UNotifier::first;

UNotifier::~UNotifier()
{
   U_TRACE_UNREGISTER_OBJECT(0, UNotifier)

   if (next)             delete next;
   if (handler_event_fd) delete handler_event_fd;
}

// NB: n e' necessario per la rientranza delle funzioni (vedi test_notifier...) 

U_NO_EXPORT
bool UNotifier::handlerResult(int& n, UNotifier* item, UNotifier** ptr, UEventFd* handler_event, bool bread, bool bwrite, bool flag_handler_call)
{
   U_TRACE(0, "UNotifier::handlerResult(%d,%p,%p,%p,%b,%b,%b)", n, item, ptr, handler_event, bread, bwrite, flag_handler_call)

   U_INTERNAL_ASSERT(bread || bwrite)

   int ret;

   if (bread)
      {
      U_INTERNAL_ASSERT_MAJOR(n,0)

      --n;

      ret = (flag_handler_call ? handler_event->handlerRead() : U_NOTIFIER_DELETE);

      if (ret != U_NOTIFIER_OK)
         {
         U_INTERNAL_ASSERT(FD_ISSET(handler_event->fd, &fd_set_read))

         FD_CLR(handler_event->fd, &fd_set_read);

         --fd_read_cnt;

         U_INTERNAL_DUMP("fd_read_cnt = %d", fd_read_cnt)

         U_INTERNAL_ASSERT(fd_read_cnt >= 0)

         if (ret == U_NOTIFIER_UPDATE) // update
            {
            U_INTERNAL_ASSERT_DIFFERS(handler_event->op_mask & W_OK, 0)

            // NB: scelta necessaria altrimenti bisogna scorrere sempre tutta la lista...

            U_INTERNAL_ASSERT_EQUALS(FD_ISSET(handler_event->fd, &fd_set_write),0)

            FD_SET(handler_event->fd, &fd_set_write);

            U_INTERNAL_ASSERT(fd_write_cnt >= 0)

            ++fd_write_cnt;

            U_INTERNAL_DUMP("fd_write_cnt = %d", fd_write_cnt)
            }
         else // ret == U_NOTIFIER_DELETE (failed)
            {
            // check if we must erase from list

            if (handler_event->op_mask != R_OK) handler_event->op_mask &= ~R_OK;
            else
               {
               *ptr = item->next;

               item->next = pool;
               pool       = item;

               U_INTERNAL_DUMP("pool = %O", U_OBJECT_TO_TRACE(*pool))

               if (handler_event->fd == (fd_set_max - 1)) fd_set_max = handler_event->fd;

               // this must be done in some way with libevent...

               delete item->handler_event_fd;
                      item->handler_event_fd = 0;

               U_RETURN(false);
               }
            }
         }
      }

   if (bwrite)
      {
      U_INTERNAL_ASSERT_MAJOR(n,0)

      --n;

      ret = (flag_handler_call ? handler_event->handlerWrite() : U_NOTIFIER_DELETE);

      if (ret != U_NOTIFIER_OK)
         {
         U_INTERNAL_ASSERT(FD_ISSET(handler_event->fd, &fd_set_write))

         FD_CLR(handler_event->fd, &fd_set_write);

         --fd_write_cnt;

         U_INTERNAL_DUMP("fd_write_cnt = %d", fd_write_cnt)

         U_INTERNAL_ASSERT(fd_write_cnt >= 0)

         if (ret == U_NOTIFIER_UPDATE) // update
            {
            U_INTERNAL_ASSERT_DIFFERS(handler_event->op_mask & R_OK, 0)

            // NB: scelta necessaria altrimenti bisogna scorrere sempre tutta la lista...

            U_INTERNAL_ASSERT_EQUALS(FD_ISSET(handler_event->fd, &fd_set_read),0)

            FD_SET(handler_event->fd, &fd_set_read);

            U_INTERNAL_ASSERT(fd_read_cnt >= 0)

            ++fd_read_cnt;

            U_INTERNAL_DUMP("fd_read_cnt = %d", fd_read_cnt)
            }
         else // ret == U_NOTIFIER_DELETE (failed)
            {
            // check if we must erase from list

            if (handler_event->op_mask != W_OK) handler_event->op_mask &= ~W_OK;
            else
               {
               *ptr = item->next;

               item->next = pool;
               pool       = item;

               U_INTERNAL_DUMP("pool = %O", U_OBJECT_TO_TRACE(*pool))

               if (handler_event->fd == (fd_set_max - 1)) fd_set_max = handler_event->fd;

               // this must be done in some way with libevent...

               delete item->handler_event_fd;
                      item->handler_event_fd = 0;

               U_RETURN(false);
               }
            }
         }
      }

   U_RETURN(true);
}

void UNotifier::removeBadFd()
{
   U_TRACE(1, "UNotifier::removeBadFd()")

   int n, fd;
   fd_set fdmask;
   fd_set* rmask;
   fd_set* wmask;
   UEventFd* handler_event;
   struct timeval polling = { 0L, 0L };
   bool bread, bwrite, flag_handler_call;

   UNotifier* item =  first;
   UNotifier** ptr = &first;

   do {
      U_INTERNAL_ASSERT_POINTER(item)

      handler_event = item->handler_event_fd;

      U_INTERNAL_ASSERT_POINTER(handler_event)

      fd     =                   handler_event->fd;
      bread  = (fd_read_cnt  && (handler_event->op_mask & R_OK));
      bwrite = (fd_write_cnt && (handler_event->op_mask & W_OK));

      FD_ZERO(&fdmask);
      FD_SET(fd, &fdmask);

      rmask = (bread  ? &fdmask : 0);
      wmask = (bwrite ? &fdmask : 0);

      U_INTERNAL_DUMP("fdmask = %B", __FDS_BITS(&fdmask)[0])

      n = U_SYSCALL(select, "%d,%p,%p,%p,%p", fd+1, rmask, wmask, 0, &polling);

      U_INTERNAL_DUMP("fd = %d op_mask = %d ISSET(read) = %b ISSET(write) = %b", fd, handler_event->op_mask,
                        (rmask ? FD_ISSET(fd, rmask) : false), (wmask ? FD_ISSET(fd, wmask) : false))

      if (n)
         {
         flag_handler_call = (n == -1 ? (n = (bread + bwrite), false) : true);

         if (handlerResult(n, item, ptr, handler_event, bread, bwrite, flag_handler_call) == false) continue;
         }

      ptr = &(*ptr)->next;
      }
   while ((item = *ptr));
}

void UNotifier::waitForEvent(int fd_max, fd_set* read_set, fd_set* write_set, UEventTime* timeout)
{
   U_TRACE(1, "UNotifier::waitForEvent(%d,%p,%p,%p)", fd_max, read_set, write_set, timeout)

   static struct timeval   tmp;
          struct timeval* ptmp = (timeout == 0 ? 0 : &tmp);

loop:

#ifdef DEBUG
   if ( read_set) U_INTERNAL_DUMP(" read_set = %B", __FDS_BITS( read_set)[0])
   if (write_set) U_INTERNAL_DUMP("write_set = %B", __FDS_BITS(write_set)[0])
#endif

   // On Linux, the function select modifies timeout to reflect the amount of time not slept;
   // most other implementations do not do this. This causes problems both when Linux code which
   // reads timeout is ported to other operating systems, and when code is ported to Linux that
   // reuses a struct timeval for multiple selects in a loop without reinitializing it.
   // Consider timeout to be undefined after select returns

   if (timeout)
      {
      tmp = *(struct timeval*)timeout;

      U_INTERNAL_DUMP("timeout = { %ld %6ld }", tmp.tv_sec, tmp.tv_usec)
      }

   result = U_SYSCALL(select, "%d,%p,%p,%p,%p", fd_max, read_set, write_set, 0, ptmp);

   if (result == 0) // timeout
      {
      // chiama il gestore dell'evento scadenza temporale

      U_INTERNAL_ASSERT_POINTER(timeout)

      int ret = timeout->handlerTime();

      // return value:
      // ---------------
      // -1 - normal
      //  0 - monitoring
      // ---------------

      if (ret == 0) goto loop;
      }
   else if (result == -1)
      {
      if (UInterrupt::checkForEventSignalPending() &&
          exit_loop_wait_event_for_signal == false)
         {
         goto loop;
         }
      else if (errno == EBADF) // ci sono descrittori di file diventati invalidi (possibile con EPIPE)
         {
         removeBadFd();

         if (empty() == false)
            {
            if (read_set)   *read_set = fd_set_read;
            if (write_set) *write_set = fd_set_write;

            goto loop;
            }
         }
      }

#ifdef DEBUG
   if ( read_set) U_INTERNAL_DUMP(" read_set = %B", __FDS_BITS( read_set)[0])
   if (write_set) U_INTERNAL_DUMP("write_set = %B", __FDS_BITS(write_set)[0])
#endif
}

// return false if there are no more events registered...

bool UNotifier::waitForEvent(UEventTime* timeout)
{
   U_TRACE(0, "UNotifier::waitForEvent(%p)", timeout)

   if (empty()) U_RETURN(false);

   U_INTERNAL_ASSERT(fd_read_cnt > 0 || fd_write_cnt > 0)

   fd_set read_set, write_set;

   if (fd_read_cnt)   read_set = fd_set_read;
   if (fd_write_cnt) write_set = fd_set_write;

   waitForEvent(fd_set_max,
                (fd_read_cnt  ? &read_set
                              : 0),
                (fd_write_cnt ? &write_set
                              : 0),
                timeout);

   if (result > 0)
      {
      int n = result;
      bool bread, bwrite;
      UEventFd* handler_event;

      UNotifier* item =  first;
      UNotifier** ptr = &first;

      do {
         U_INTERNAL_ASSERT_POINTER(item)

         handler_event = item->handler_event_fd;

         U_INTERNAL_ASSERT_POINTER(handler_event)

         bread = (fd_read_cnt &&
                  (handler_event->op_mask & R_OK) &&
                  FD_ISSET(handler_event->fd, &read_set));

         bwrite = (fd_write_cnt &&
                   (handler_event->op_mask & W_OK) &&
                   FD_ISSET(handler_event->fd, &write_set));

         U_INTERNAL_DUMP("fd = %d op_mask = %d bread = %b bwrite = %b", handler_event->fd, handler_event->op_mask, bread, bwrite)

         if ((bread || bwrite) &&
             handlerResult(n, item, ptr, handler_event, bread, bwrite, true) == false) continue;

         ptr = &(*ptr)->next;
         }
      while (n > 0 && (item = *ptr));
      }

   bool esito = (empty() == false);

   U_INTERNAL_DUMP("result = %d", result)

   U_RETURN(esito);
}

void UNotifier::insert(UEventFd* handler_event)
{
   U_TRACE(0, "UNotifier::insert(%p)", handler_event)

   U_INTERNAL_DUMP("op_mask = %B", handler_event->op_mask)

   if (handler_event->op_mask & R_OK)
      {
      // NB: scelta necessaria altrimenti bisogna scorrere sempre tutta la lista...

      U_INTERNAL_ASSERT_EQUALS(FD_ISSET(handler_event->fd, &fd_set_read),0)

      FD_SET(handler_event->fd, &fd_set_read);

      U_INTERNAL_ASSERT(fd_read_cnt >= 0)

      ++fd_read_cnt;
      }

   if (handler_event->op_mask & W_OK)
      {
      // NB: scelta necessaria altrimenti bisogna scorrere sempre tutta la lista...

      U_INTERNAL_ASSERT_EQUALS(FD_ISSET(handler_event->fd, &fd_set_write),0)

      FD_SET(handler_event->fd, &fd_set_write);

      U_INTERNAL_ASSERT(fd_write_cnt >= 0)

      ++fd_write_cnt;
      }

   if (fd_set_max <= handler_event->fd) fd_set_max = handler_event->fd + 1;

   UNotifier* item;

   if (pool)
      {
      if (pool->handler_event_fd) delete pool->handler_event_fd;

      item = pool;
      pool = pool->next;
      }
   else
      {
      item = U_NEW(UNotifier);
      }

   item->handler_event_fd = handler_event;

   item->next = first;
   first      = item;
}

void UNotifier::erase(UEventFd* handler_event, bool flag_reuse)
{
   U_TRACE(0, "UNotifier::erase(%p,%b)", handler_event, flag_reuse)

   U_INTERNAL_ASSERT_POINTER(first)
   U_INTERNAL_ASSERT_POINTER(handler_event)

   UNotifier* item =  first;
   UNotifier** ptr = &first;

   do {
      if (item->handler_event_fd == handler_event)
         {
         if (handler_event->op_mask & R_OK)
            {
            U_INTERNAL_ASSERT(FD_ISSET(handler_event->fd, &fd_set_read))

            --fd_read_cnt;

            U_INTERNAL_ASSERT(fd_read_cnt >= 0)

            FD_CLR(handler_event->fd, &fd_set_read);

            U_INTERNAL_DUMP("fd_set_read = %B", __FDS_BITS(&fd_set_read)[0])
            }

         if (handler_event->op_mask & W_OK)
            {
            U_INTERNAL_ASSERT(FD_ISSET(handler_event->fd, &fd_set_write))

            --fd_write_cnt;

            U_INTERNAL_ASSERT(fd_write_cnt >= 0)

            FD_CLR(handler_event->fd, &fd_set_write);

            U_INTERNAL_DUMP("fd_set_write = %B", __FDS_BITS(&fd_set_write)[0])
            }

         if (handler_event->fd == (fd_set_max - 1)) fd_set_max = handler_event->fd;

         *ptr = item->next;

         if (flag_reuse)
            {
            item->next = pool;
            pool       = item;

            delete item->handler_event_fd;
                   item->handler_event_fd = 0;

            U_INTERNAL_DUMP("pool = %O", U_OBJECT_TO_TRACE(*pool))
            }
         else
            {
            item->next             = 0;
            item->handler_event_fd = 0;

            delete item;
            }

         break;
         }

      ptr = &(*ptr)->next;
      }
   while ((item = *ptr));
}

bool UNotifier::isHandler(UEventFd* handler_event)
{
   U_TRACE(0, "UNotifier::isHandler(%p)", handler_event)

   U_INTERNAL_ASSERT_POINTER(first)

   UNotifier* item =  first;
   UNotifier** ptr = &first;

   do {
      if (item->handler_event_fd == handler_event) U_RETURN(true);

      ptr = &(*ptr)->next;
      }
   while ((item = *ptr));

   U_RETURN(false);
}

void UNotifier::clear()
{
   U_TRACE(0, "UNotifier::clear()")

   U_INTERNAL_DUMP("first = %p pool = %p", first, pool)

   if (first)
      {
      delete first;
             first = 0;
      }

   if (pool)
      {
      delete pool;
             pool = 0;
      }
}

// param timeoutMS specified the timeout value, in milliseconds.
// A negative value indicates no timeout, i.e. an infinite wait

#ifdef USE_POLL

void UNotifier::waitForEvent(struct pollfd* fds, int timeoutMS)
{
   U_TRACE(1, "UNotifier::waitForEvent(%p,%d)", fds, timeoutMS)

loop:
   result = U_SYSCALL(poll, "%p,%d,%d", fds, 1, timeoutMS);

   if (result == -1 && UInterrupt::checkForEventSignalPending()) goto loop;
}

#endif

int UNotifier::waitForRead(int fd, int timeoutMS)
{
   U_TRACE(0, "UNotifier::waitForRead(%d,%d)", fd, timeoutMS)

   U_INTERNAL_ASSERT_RANGE(0,fd,4096)

#ifdef __MINGW32__

   if (is_socket(fd) == false) U_RETURN(1);

#elif defined(USE_POLL)

   struct pollfd fds[1] = { { fd, POLLIN, 0 } };

   waitForEvent(fds, timeoutMS);

   U_RETURN(result);

#endif

   UEventTime time(0L, timeoutMS * U_MILLISEC);
   UEventTime* ptime = (timeoutMS < 0 ? 0 : (time.adjust(), &time));

   fd_set fdmask;
   FD_ZERO(&fdmask);
   FD_SET(fd, &fdmask);

   waitForEvent(fd + 1, &fdmask, 0, ptime);

   U_RETURN(result);
}

int UNotifier::waitForWrite(int fd, int timeoutMS)
{
   U_TRACE(0, "UNotifier::waitForWrite(%d,%d)", fd, timeoutMS)

   U_INTERNAL_ASSERT_RANGE(0,fd,4096)

#ifdef __MINGW32__

   if (is_socket(fd) == false) U_RETURN(1);

#elif defined(USE_POLL)

   struct pollfd fds[1] = { { fd, POLLOUT, 0 } };

   waitForEvent(fds, timeoutMS);

   U_RETURN(result);

#endif

   UEventTime time(0L, timeoutMS * U_MILLISEC);
   UEventTime* ptime = (timeoutMS < 0 ? 0 : (time.adjust(), &time));

   fd_set fdmask;
   FD_ZERO(&fdmask);
   FD_SET(fd, &fdmask);

   waitForEvent(fd + 1, 0, &fdmask, ptime);

   U_RETURN(result);
}

// param timeoutMS specified the timeout value, in milliseconds.
// A negative value indicates no timeout, i.e. an infinite wait

uint32_t UNotifier::read(int fd, char* buffer, int count, int timeoutMS)
{
   U_TRACE(1, "UNotifier::read(%d,%p,%d,%d)", fd, buffer, count, timeoutMS)

   U_INTERNAL_ASSERT_DIFFERS(fd,-1)

   ssize_t value;
   int bytes_read = 0;
   bool single_read = (count == U_SINGLE_READ);

   do {
      if (timeoutMS != -1)
         {
         if (waitForRead(fd, timeoutMS) != 1)
            {
            errno = EAGAIN;

            break;
            }

         timeoutMS = -1; // in this way is only for the first read...
         }

#  ifdef __MINGW32__
      (void) U_SYSCALL(ReadFile, "%p,%p,%lu,%p,%p", (HANDLE)_get_osfhandle(fd), buffer + bytes_read, (single_read ? U_CAPACITY : count - bytes_read),
                                                    (DWORD*)&value, 0);
#  else
      value = U_SYSCALL(read, "%d,%p,%u", fd, buffer + bytes_read, (single_read ? U_CAPACITY : count - bytes_read));
#  endif

      if      (value ==  0) break; // EOF
      else if (value == -1)
         {
         if (UInterrupt::checkForEventSignalPending()) continue;

         break;
         }

      U_INTERNAL_DUMP("bytes read : %#.*S", value, buffer + bytes_read)

      bytes_read += value;

      if (single_read) break; // only one read()...
      }
   while (bytes_read < count);

   U_RETURN(bytes_read);
}

// param timeoutMS specified the timeout value, in milliseconds.
// A negative value indicates no timeout, i.e. an infinite wait

uint32_t UNotifier::write(int fd, const char* str, int count, int timeoutMS)
{
   U_TRACE(1, "UNotifier::write(%d,%.*S,%d,%d)", fd, count, str, count, timeoutMS)

   U_INTERNAL_ASSERT_DIFFERS(fd,-1)

   ssize_t value;
   int byte_written = 0;

   do {
      if (timeoutMS != -1)
         {
         if (waitForWrite(fd, timeoutMS) != 1)
            {
            errno = EAGAIN;

            break;
            }

         timeoutMS = -1; // in this way is only for the first write...
         }

#  ifdef __MINGW32__
      (void) U_SYSCALL(WriteFile, "%p,%p,%lu,%p,%p", (HANDLE)_get_osfhandle(fd), str + byte_written, count - byte_written, (DWORD*)&value, 0);
#  else
      value = U_SYSCALL(write, "%d,%S,%u", fd, str + byte_written, count - byte_written);
#  endif

      if      (value ==  0) break;
      else if (value == -1)
         {
         if (UInterrupt::checkForEventSignalPending()) continue;

         break;
         }

      U_INTERNAL_DUMP("byte written : %#.*S", value, str + byte_written)

      byte_written += value;
      }
   while (byte_written < count);

   U_RETURN(byte_written);
}

// STREAM

U_NO_EXPORT void UNotifier::outputEntry(ostream& os) const
{
   U_INTERNAL_TRACE("UNotifier::outputEntry(%p)", &os)

   U_CHECK_MEMORY

   os.put('{');
   os.put(' ');
   os << handler_event_fd->fd;
   os.put(' ');
   os << handler_event_fd->op_mask;
   os.put(' ');
   os.put('}');
}

U_EXPORT ostream& operator<<(ostream& os, const UNotifier& n)
{
   U_TRACE(0+256, "UNotifier::operator<<(%p,%p)", &os, &n)

   os.put('(');
   os.put(' ');

   if (n.handler_event_fd) n.outputEntry(os);
   else                    os << (void*)&n;

   for (UNotifier* item = n.next; item; item = item->next)
      {
      os.put(' ');

      if (item->handler_event_fd) item->outputEntry(os);
      else                        os << (void*)item;
      }

   os.put(' ');
   os.put(')');

   return os;
}

#ifdef DEBUG

void UNotifier::printInfo(ostream& os)
{
   U_TRACE(0+256, "UNotifier::printInfo(%p)", &os)

   os << "first = ";

   if (first) os << *first;
   else       os <<  first;

   os << "\npool  = ";

   if (pool) os << *pool;
   else      os <<  pool;

   os << "\n";
}

#  include <ulib/internal/objectIO.h>

const char* UNotifier::dump(bool reset) const
{
   *UObjectIO::os << "result                      " << result       << '\n'
                  << "fd_set_max                  " << fd_set_max   << '\n'
                  << "fd_read_cnt                 " << fd_read_cnt  << '\n'
                  << "fd_write_cnt                " << fd_write_cnt << '\n';

   *UObjectIO::os << "fd_set_read                 ";
   char buffer1[70];
   UObjectIO::os->write(buffer1, u_snprintf(buffer1, 70, "%B", __FDS_BITS(&fd_set_read)[0]));
   UObjectIO::os->put('\n');

   *UObjectIO::os << "fd_set_write                ";
   char buffer2[70];
   UObjectIO::os->write(buffer2, u_snprintf(buffer2, 70, "%B", __FDS_BITS(&fd_set_write)[0]));
   UObjectIO::os->put('\n');

   *UObjectIO::os << "pool             (UNotifier " << (void*)pool             << ")\n"
                  << "first            (UNotifier " << (void*)first            << ")\n"
                  << "next             (UNotifier " << (void*)next             << ")\n"
                  << "handler_event_fd (UEventFd  " << (void*)handler_event_fd << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
