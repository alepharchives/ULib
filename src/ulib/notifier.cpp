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

bool       UNotifier::exit_loop_wait_event_for_signal;
UNotifier* UNotifier::pool;
UNotifier* UNotifier::vpool;
UNotifier* UNotifier::first;

#ifdef HAVE_LIBEVENT
void UEventFd::operator()(int fd, short event)
{
   U_TRACE(0, "UEventFd::operator()(%d,%hd)", fd, event)

   int ret = (event == EV_READ ? handlerRead() : handlerWrite());

   U_INTERNAL_DUMP("ret = %d", ret)

   if (ret == U_NOTIFIER_DELETE) UNotifier::erase(this, true);
}
#elif defined(HAVE_EPOLL_WAIT)
int                UNotifier::epollfd;
struct epoll_event UNotifier::events[MAX_EVENTS];
#else
int    UNotifier::fd_set_max;
int    UNotifier::fd_read_cnt;
int    UNotifier::fd_write_cnt;
fd_set UNotifier::fd_set_read;
fd_set UNotifier::fd_set_write;
#endif

void UNotifier::init()
{
   U_TRACE(0, "UNotifier::init()")

#ifdef HAVE_LIBEVENT
   if (u_ev_base) (void) U_SYSCALL(event_reinit, "%p", u_ev_base); // NB: reinitialized the event base after fork()...
   else           u_ev_base = (struct event_base*) U_SYSCALL_NO_PARAM(event_init);

   U_INTERNAL_ASSERT_POINTER(u_ev_base)
#elif defined(HAVE_EPOLL_WAIT)
   int old = epollfd;

   U_INTERNAL_DUMP("old = %d", old)

#  ifdef HAVE_EPOLL_CREATE1
   epollfd = U_SYSCALL(epoll_create1, "%d", EPOLL_CLOEXEC);
   if (epollfd != -1 || errno != ENOSYS) goto next;
#  endif
   epollfd = U_SYSCALL(epoll_create,  "%d", 1024);
next:

   U_INTERNAL_ASSERT_MAJOR(epollfd,0)
   U_INTERNAL_ASSERT_EQUALS(U_READ_IN,EPOLLIN)
   U_INTERNAL_ASSERT_EQUALS(U_WRITE_OUT,EPOLLOUT)

   if (old)
      {
      // NB: reinitialized all after fork()...

      int fd;

      for (UNotifier* item = first; item; item = item->next)
         {
         fd = item->handler_event_fd->fd;

         U_INTERNAL_DUMP("fd = %d", fd)

         (void) U_SYSCALL(epoll_ctl, "%d,%d,%d,%p", old, EPOLL_CTL_DEL, fd, (struct epoll_event*)1);

         struct epoll_event _events = { item->handler_event_fd->op_mask, { int2ptr(fd) } };

         (void) U_SYSCALL(epoll_ctl, "%d,%d,%d,%p", epollfd, EPOLL_CTL_ADD, fd, &_events);
         }

      (void) U_SYSCALL(close, "%d", old);
      }
#endif
}

UNotifier::~UNotifier()
{
   U_TRACE_UNREGISTER_OBJECT(0, UNotifier)

   U_INTERNAL_DUMP("this = %p next = %p handler_event_fd = %p", this, next, handler_event_fd)

   if (next) delete next;

   if (handler_event_fd)
      {
      U_INTERNAL_DUMP("handler_event_fd = %p", handler_event_fd)

      delete handler_event_fd;
      }
}

void UNotifier::insert(UEventFd* handler_event)
{
   U_TRACE(0, "UNotifier::insert(%p)", handler_event)

   U_INTERNAL_DUMP("fd = %d op_mask = %B", handler_event->fd, handler_event->op_mask)

   UNotifier* item;

#ifdef DEBUG
   bool error = false;

   for (item = first; item; item = item->next)
      {
      U_INTERNAL_DUMP("fd = %d", item->handler_event_fd->fd)

      U_INTERNAL_ASSERT_MAJOR(handler_event->fd,         0)
      U_INTERNAL_ASSERT_MAJOR(item->handler_event_fd->fd,0)

      if (handler_event->fd == item->handler_event_fd->fd)
         {
         error = true;

         U_WARNING("insertion of duplicate handler for file descriptor %d...", handler_event->fd);
         }
      }

   if (error) return;
#endif

#ifdef HAVE_LIBEVENT
   U_INTERNAL_ASSERT_POINTER(u_ev_base)

   int mask = EV_PERSIST;

   if (handler_event->op_mask & U_READ_IN)   mask |= EV_READ;
   if (handler_event->op_mask & U_WRITE_OUT) mask |= EV_WRITE;

   U_INTERNAL_DUMP("mask = %B", mask)

   handler_event->pevent = U_NEW(UEvent<UEventFd>(handler_event->fd, mask, *handler_event));

   (void) UDispatcher::add(*(handler_event->pevent));
#elif defined(HAVE_EPOLL_WAIT)
   U_INTERNAL_ASSERT_MAJOR(epollfd,0)

   struct epoll_event _events = { handler_event->op_mask, { int2ptr(handler_event->fd) } };

   (void) U_SYSCALL(epoll_ctl, "%d,%d,%d,%p", epollfd, EPOLL_CTL_ADD, handler_event->fd, &_events);
#else
   if (handler_event->op_mask & U_READ_IN)
      {
      U_INTERNAL_ASSERT_EQUALS(FD_ISSET(handler_event->fd, &fd_set_read),0)

      FD_SET(handler_event->fd, &fd_set_read);

      U_INTERNAL_ASSERT(fd_read_cnt >= 0)

      ++fd_read_cnt;

      U_INTERNAL_DUMP("fd_set_read = %B", __FDS_BITS(&fd_set_read)[0])
      }

   if (handler_event->op_mask & U_WRITE_OUT)
      {
      U_INTERNAL_ASSERT_EQUALS(FD_ISSET(handler_event->fd, &fd_set_write),0)

      FD_SET(handler_event->fd, &fd_set_write);

      U_INTERNAL_ASSERT(fd_write_cnt >= 0)

      ++fd_write_cnt;

      U_INTERNAL_DUMP("fd_set_write = %B", __FDS_BITS(&fd_set_write)[0])
      }

   if (fd_set_max <= handler_event->fd) fd_set_max = handler_event->fd + 1;

   U_INTERNAL_DUMP("fd_set_max = %d", fd_set_max)
#endif

   if (pool)
      {
      item = pool;
      pool = pool->next;
      }
   else
      {
      item = U_NEW(UNotifier);
      }

   item->next = first;
   first      = item;

   item->handler_event_fd = handler_event;
}

int UNotifier::waitForEvent(int fd_max, fd_set* read_set, fd_set* write_set, UEventTime* timeout)
{
   U_TRACE(1, "UNotifier::waitForEvent(%d,%p,%p,%p)", fd_max, read_set, write_set, timeout)

#ifdef HAVE_LIBEVENT
   int result = UDispatcher::dispatch(UDispatcher::ONCE);
#else
   static struct timeval   tmp;
          struct timeval* ptmp = (timeout == 0 ? 0 : &tmp);

loop:
#  ifdef DEBUG
   if ( read_set) U_INTERNAL_DUMP(" read_set = %B", __FDS_BITS( read_set)[0])
   if (write_set) U_INTERNAL_DUMP("write_set = %B", __FDS_BITS(write_set)[0])
#  endif

   U_ASSERT_EQUALS(empty(), false)

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

#  if defined(HAVE_EPOLL_WAIT)
   int result = U_SYSCALL(epoll_wait, "%d,%p,%d,%p", epollfd, events, MAX_EVENTS, (ptmp ? ((tmp.tv_sec * 1000) + (tmp.tv_usec / 1000)) : -1));
#  else
   int result = U_SYSCALL(select, "%d,%p,%p,%p,%p", fd_max, read_set, write_set, 0, ptmp);
#  endif

   if (result == 0) // timeout
      {
      // call the manager of timeout

      U_INTERNAL_ASSERT_POINTER(timeout)

      int ret = timeout->handlerTime();

      // return value:
      // ---------------
      // -1 - normal
      //  0 - monitoring
      // ---------------

      if (ret == 0)
         {
         if (empty() == false)
            {
#        ifndef HAVE_EPOLL_WAIT
            if (read_set)   *read_set = fd_set_read;
            if (write_set) *write_set = fd_set_write;
#        endif

            goto loop;
            }
         }
      }
   else if (result == -1)
      {
      if (UInterrupt::checkForEventSignalPending() &&
          exit_loop_wait_event_for_signal == false)
         {
         goto loop;
         }
#  ifndef HAVE_EPOLL_WAIT
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
#  endif
      }

#  ifdef DEBUG
   if ( read_set) U_INTERNAL_DUMP(" read_set = %B", __FDS_BITS( read_set)[0])
   if (write_set) U_INTERNAL_DUMP("write_set = %B", __FDS_BITS(write_set)[0])
#  endif
#endif

   U_RETURN(result);
}

// NB: n is needeed for rientrance of function (see test_notifier...) 

#ifndef HAVE_LIBEVENT
U_NO_EXPORT bool UNotifier::handlerResult(int& n, UNotifier** ptr, bool bread, bool bwrite, bool bexcept)
{
   U_TRACE(0, "UNotifier::handlerResult(%d,%p,%b,%b,%b)", n, ptr, bread, bwrite, bexcept)

   U_INTERNAL_ASSERT_MAJOR(n,0)
   U_INTERNAL_ASSERT(bread || bwrite)

   int ret;
   bool bdelete = false;
   UNotifier* item = *ptr;
   UEventFd* handler_event = item->handler_event_fd;

   U_INTERNAL_ASSERT_POINTER(handler_event)

   if (bread)
      {
#  ifndef HAVE_EPOLL_WAIT
      U_INTERNAL_ASSERT_MAJOR(fd_read_cnt,0)
      U_INTERNAL_ASSERT(FD_ISSET(handler_event->fd, &fd_set_read))
#  endif
      U_INTERNAL_ASSERT(handler_event->op_mask & U_READ_IN)

      --n;

      ret = (bexcept ? (handler_event->handlerError(0x002), U_NOTIFIER_DELETE) // 0x002 -> USocket::RESET
                     :  handler_event->handlerRead());

      if (ret == U_NOTIFIER_DELETE)
         {
         bdelete = true;

#     ifndef HAVE_EPOLL_WAIT
         --fd_read_cnt;

         U_INTERNAL_DUMP("fd_read_cnt = %d", fd_read_cnt)

         FD_CLR(handler_event->fd, &fd_set_read);
#     endif
         }
      }

   if (bwrite)
      {
#  ifndef HAVE_EPOLL_WAIT
      U_INTERNAL_ASSERT_MAJOR(fd_write_cnt,0)
      U_INTERNAL_ASSERT(FD_ISSET(handler_event->fd, &fd_set_write))
#  endif
      U_INTERNAL_ASSERT(handler_event->op_mask & U_WRITE_OUT)

      --n;

      ret = (bexcept ? (handler_event->handlerError(0x002), U_NOTIFIER_DELETE) // 0x002 -> USocket::RESET
                     :  handler_event->handlerWrite());

      if (ret == U_NOTIFIER_DELETE)
         {
         bdelete = true;

#     ifndef HAVE_EPOLL_WAIT
         --fd_write_cnt;

         U_INTERNAL_DUMP("fd_write_cnt = %d", fd_write_cnt)

         FD_CLR(handler_event->fd, &fd_set_write);
#     endif
         }
      }

   if (bdelete)
      {
      *ptr = item->next;

      item->next = pool;
      pool       = item;

      item->handler_event_fd = 0;

      U_INTERNAL_DUMP("pool = %O", U_OBJECT_TO_TRACE(*pool))

#  ifdef HAVE_EPOLL_WAIT
   // (void) U_SYSCALL(epoll_ctl, "%d,%d,%d,%p", epollfd, EPOLL_CTL_DEL, handler_event->fd, (struct epoll_event*)1);
#  endif

      handler_event->handlerDelete();

      U_RETURN(true);
      }

   U_RETURN(false);
}
#endif

// return false if there are no more events registered...

bool UNotifier::waitForEvent(UEventTime* timeout)
{
   U_TRACE(0, "UNotifier::waitForEvent(%p)", timeout)

   if (first)
      {
#  ifdef HAVE_LIBEVENT
      (void) waitForEvent(0, 0, 0, 0);
#  elif defined(HAVE_EPOLL_WAIT)
      int n = waitForEvent(0, 0, 0, timeout);
#  else
      U_INTERNAL_ASSERT(fd_read_cnt > 0 || fd_write_cnt > 0)

      fd_set read_set, write_set;

      if (fd_read_cnt)   read_set = fd_set_read;
      if (fd_write_cnt) write_set = fd_set_write;

      int n = waitForEvent(fd_set_max,
                   (fd_read_cnt  ? &read_set
                                 : 0),
                   (fd_write_cnt ? &write_set
                                 : 0),
                   timeout);
#endif
      if (n <= 0) goto end;

      UNotifier* item;
      UNotifier** ptr;
      bool bread, bwrite;
      UEventFd* handler_event;

#  ifdef HAVE_LIBEVENT
#  elif defined(HAVE_EPOLL_WAIT)
      bool bexcept;

      for (struct epoll_event* pev = events, *pev_end = pev + n; pev < pev_end; ++pev)
         {
         U_INTERNAL_DUMP("events[%d].data.fd = %d events[%d].events = %B", (pev - events), pev->data.fd, (pev - events), pev->events)

         U_INTERNAL_ASSERT((pev->events & U_READ_IN)   != 0 ||
                           (pev->events & U_WRITE_OUT) != 0 ||
                           (pev->events & EPOLLERR)    != 0)

         ptr = &first;

#     ifdef DEBUG
         bool bfound = false;
#     endif

         while ((item = *ptr))
            {
            handler_event = item->handler_event_fd;

            U_INTERNAL_ASSERT_POINTER(handler_event)

            U_INTERNAL_DUMP("fd = %d op_mask = %B", handler_event->fd, handler_event->op_mask)

            if (handler_event->fd != pev->data.fd)
               {
               ptr = &(*ptr)->next;

               continue;
               }

            bread   = ((pev->events & U_READ_IN)   != 0);
            bwrite  = ((pev->events & U_WRITE_OUT) != 0);
            bexcept = ((pev->events & EPOLLERR)    != 0);

            U_INTERNAL_DUMP("bread = %b bwrite = %b bexcept = %b", bread, bwrite, bexcept)

            (void) handlerResult(n, ptr, bread, bwrite, bexcept);

            if (n == 0)
               {
               U_INTERNAL_DUMP("events[%d]: goto end", (pev - events))

               U_INTERNAL_ASSERT_EQUALS(pev+1,pev_end)

               goto end;
               }

#        ifdef DEBUG
            bfound = true;
#        endif

            break;
            }

#     ifdef DEBUG
         if (bfound == false)
            {
            U_WARNING("epoll_wait() fire events %B on file descriptor %d without handler...", pev->events, pev->data.fd);

            (void) U_SYSCALL(epoll_ctl, "%d,%d,%d,%p", epollfd, EPOLL_CTL_DEL, pev->data.fd, (struct epoll_event*)1);
            }
#     endif
         }
#  else
      bool bdelete;
      int fd_cnt = (fd_read_cnt + fd_write_cnt);

      U_INTERNAL_DUMP("fd_cnt = %d", fd_cnt)

      U_INTERNAL_ASSERT(n <= fd_cnt)

      item =  first;
      ptr  = &first;

      do {
         U_INTERNAL_ASSERT_POINTER(item)

         handler_event = item->handler_event_fd;

         U_INTERNAL_ASSERT_POINTER(handler_event)

         U_INTERNAL_DUMP("fd = %d op_mask = %B", handler_event->fd, handler_event->op_mask)

         bread  = (fd_read_cnt  && FD_ISSET(handler_event->fd, &read_set));
         bwrite = (fd_write_cnt && FD_ISSET(handler_event->fd, &write_set));

         U_INTERNAL_DUMP("bread = %b bwrite = %b", bread, bwrite)

         if (bread || bwrite)
            {
            bdelete = handlerResult(n, ptr, bread, bwrite, false);

            if (n == 0) break;

            if (fd_read_cnt)  FD_CLR(handler_event->fd, &read_set);
            if (fd_write_cnt) FD_CLR(handler_event->fd, &write_set);

            if (bdelete) continue;
            }

         ptr = &(*ptr)->next;
         }
      while ((item = *ptr));

#  ifdef DEBUG
      if (n)
         {
         int fd;

         if (fd_read_cnt)
            {
            for (fd = 0; fd < fd_set_max; ++fd)
               {
               if (FD_ISSET(fd, &read_set))
                  {
                  U_WARNING("select() fire read event on file descriptor %d without handler...", fd);

                  (void) U_SYSCALL(close, "%d", fd);

                  FD_CLR(fd, &read_set);
                  }
               }
            }

         if (fd_write_cnt)
            {
            for (fd = 0; fd < fd_set_max; ++fd)
               {
               if (FD_ISSET(fd, &write_set))
                  {
                  U_WARNING("select() fire write event on file descriptor %d without handler...", fd);

                  (void) U_SYSCALL(close, "%d", fd);

                  FD_CLR(fd, &write_set);
                  }
               }
            }
         }
#  endif

      if (fd_cnt > (fd_read_cnt + fd_write_cnt)) fd_set_max = getNFDS();
#  endif

      U_INTERNAL_DUMP("n = %d", n)
      }

end:
   if (first) U_RETURN(true);

   U_RETURN(false);
}

#if !defined(HAVE_EPOLL_WAIT) && !defined(HAVE_LIBEVENT)

// nfds is the highest-numbered file descriptor in any of the three sets, plus 1.

int UNotifier::getNFDS()
{
   U_TRACE(0, "UNotifier::getNFDS()")

   U_INTERNAL_DUMP("fd_set_max = %d", fd_set_max)

   int nfds = 0;

   for (UNotifier* item = first; item; item = item->next)
      {
      if (nfds < item->handler_event_fd->fd) nfds = item->handler_event_fd->fd;
      }

   ++nfds;

   U_RETURN(nfds);
}

void UNotifier::removeBadFd()
{
   U_TRACE(1, "UNotifier::removeBadFd()")

   int n, fd;
   fd_set fdmask;
   fd_set* rmask;
   fd_set* wmask;
   UEventFd* handler_event;
   UNotifier* item = first;
   UNotifier** ptr = &first;
   bool bread, bwrite, bexcept;
   struct timeval polling = { 0L, 0L };

   do {
      handler_event = item->handler_event_fd;

      U_INTERNAL_ASSERT_POINTER(handler_event)

      fd     =                   handler_event->fd;
      bread  = (fd_read_cnt  && (handler_event->op_mask & U_READ_IN));
      bwrite = (fd_write_cnt && (handler_event->op_mask & U_WRITE_OUT));

      FD_ZERO(&fdmask);
      FD_SET(fd, &fdmask);

      rmask = (bread  ? &fdmask : 0);
      wmask = (bwrite ? &fdmask : 0);

      U_INTERNAL_DUMP("fdmask = %B", __FDS_BITS(&fdmask)[0])

      n = U_SYSCALL(select, "%d,%p,%p,%p,%p", fd+1, rmask, wmask, 0, &polling);

      U_INTERNAL_DUMP("fd = %d op_mask = %B ISSET(read) = %b ISSET(write) = %b", fd, handler_event->op_mask,
                        (rmask ? FD_ISSET(fd, rmask) : false),
                        (wmask ? FD_ISSET(fd, wmask) : false))

      if (n)
         {
         bexcept = (n == -1 ? (n = (bread + bwrite), true) : false);

         if (handlerResult(n, ptr, bread, bwrite, bexcept)) continue;
         }

      ptr = &(*ptr)->next;
      }
   while ((item = *ptr));
}
#endif

U_NO_EXPORT void UNotifier::eraseItem(UNotifier** ptr, bool flag)
{
   U_TRACE(1, "UNotifier::eraseItem(%p,%b)", ptr, flag)

   UNotifier* item = *ptr;

   U_INTERNAL_ASSERT_POINTER(item)

   UEventFd* handler_event = item->handler_event_fd;

   U_INTERNAL_DUMP("fd = %d op_mask = %B", handler_event->fd, handler_event->op_mask)

#ifdef HAVE_LIBEVENT
#elif defined(HAVE_EPOLL_WAIT)
   (void) U_SYSCALL(epoll_ctl, "%d,%d,%d,%p", epollfd, EPOLL_CTL_DEL, handler_event->fd, (struct epoll_event*)1);
#else
   if (handler_event->op_mask & U_READ_IN)
      {
      U_INTERNAL_ASSERT(FD_ISSET(handler_event->fd, &fd_set_read))

      FD_CLR(handler_event->fd, &fd_set_read);

      U_INTERNAL_DUMP("fd_set_read = %B", __FDS_BITS(&fd_set_read)[0])

      --fd_read_cnt;

      U_INTERNAL_ASSERT(fd_read_cnt >= 0)
      }

   if (handler_event->op_mask & U_WRITE_OUT)
      {
      U_INTERNAL_ASSERT(FD_ISSET(handler_event->fd, &fd_set_write))

      FD_CLR(handler_event->fd, &fd_set_write);

      U_INTERNAL_DUMP("fd_set_write = %B", __FDS_BITS(&fd_set_write)[0])

      --fd_write_cnt;

      U_INTERNAL_ASSERT(fd_write_cnt >= 0)
      }

   if (first) fd_set_max = getNFDS();
#endif

   *ptr = item->next;

   if (flag)
      {
      item->next = pool;
      pool       = item;

      item->handler_event_fd = 0;

      U_INTERNAL_DUMP("pool = %O", U_OBJECT_TO_TRACE(*pool))

      handler_event->handlerDelete();
      }
   else
      {
      item->next             = 0;
      item->handler_event_fd = 0;

      delete item;
      }

#ifdef DEBUG
   if (first) U_INTERNAL_DUMP("first = %O", U_OBJECT_TO_TRACE(*first))
#endif
}

void UNotifier::erase(UEventFd* handler_event, bool flag_reuse)
{
   U_TRACE(0, "UNotifier::erase(%p,%b)", handler_event, flag_reuse)

   U_INTERNAL_ASSERT_POINTER(handler_event)

   UNotifier* item;

   for (UNotifier** ptr = &first; (item = *ptr); ptr = &(*ptr)->next)
      {
      if (item->handler_event_fd == handler_event)
         {
         eraseItem(ptr, flag_reuse);

         return;
         }
      }
}

// Call function for all entry

void UNotifier::callForAllEntry(bPFpv function)
{
   U_TRACE(0, "UNotifier::callForAllEntry(%p)", function)

   UNotifier* item =  first;
   UNotifier** ptr = &first;

   while ((item = *ptr))
      {
      if (function(item->handler_event_fd))
         {
         eraseItem(ptr, true);

         continue;
         }

      ptr = &(*ptr)->next;
      }
}

void UNotifier::preallocate(uint32_t n)
{
   U_TRACE(0+256, "UNotifier::preallocate(%u)", n)

   U_INTERNAL_ASSERT_MAJOR(n,0)

   if (n < (1024U * 1024U))
      {
      pool = vpool = U_NEW_VEC(n, UNotifier);

      for (uint32_t i = 0, end = n - 1; i < end; ++i)
         {
         pool[i].next = pool + i + 1;
         }

      U_INTERNAL_DUMP("pool = %O", U_OBJECT_TO_TRACE(*pool))
      }
}

void UNotifier::clear()
{
   U_TRACE(0+256, "UNotifier::clear()")

   U_INTERNAL_DUMP("first = %p vpool = %p pool = %p", first, vpool, pool)

   if (vpool)
      {
      UNotifier* item;
      UNotifier* next;

      for (item = pool; item; item = next)
         {
         next = item->next;
                item->next = 0;

         U_INTERNAL_DUMP("item = %p item->next = %p item->handler_event_fd = %p", item, next, item->handler_event_fd)

         item->handler_event_fd = 0;
         }

      delete[] vpool;
               vpool = 0;
      }
   else
      {
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

#if defined(HAVE_EPOLL_WAIT) && !defined(HAVE_LIBEVENT)
   (void) U_SYSCALL(close, "%d", epollfd);
#endif
}

// param timeoutMS specified the timeout value, in milliseconds.
// A negative value indicates no timeout, i.e. an infinite wait

#ifdef USE_POLL

int UNotifier::waitForEvent(struct pollfd* fds, int timeoutMS)
{
   U_TRACE(1, "UNotifier::waitForEvent(%p,%d)", fds, timeoutMS)

   int ret;

#ifdef DEBUG
   if (timeoutMS != -1) U_INTERNAL_ASSERT_MAJOR(timeoutMS,499)
#endif

loop:
   ret = U_SYSCALL(poll, "%p,%d,%d", fds, 1, timeoutMS);

   if (ret == -1 && UInterrupt::checkForEventSignalPending()) goto loop;

   U_RETURN(ret);
}

#endif

int UNotifier::waitForRead(int fd, int timeoutMS)
{
   U_TRACE(0, "UNotifier::waitForRead(%d,%d)", fd, timeoutMS)

   U_INTERNAL_ASSERT_RANGE(0,fd,4096)

#ifdef DEBUG
   if (timeoutMS != -1) U_INTERNAL_ASSERT_MAJOR(timeoutMS,499)
#endif

#ifdef __MINGW32__
   if (is_socket(fd) == false) U_RETURN(1);
#elif defined(USE_POLL)
   struct pollfd fds[1] = { { fd, POLLIN, 0 } };

   int ret = waitForEvent(fds, timeoutMS);
#else
   UEventTime time(0L, timeoutMS * 1000L);
   UEventTime* ptime = (timeoutMS < 0 ? 0 : (time.adjust(), &time));

   fd_set fdmask;
   FD_ZERO(&fdmask);
   FD_SET(fd, &fdmask);

   int ret = waitForEvent(fd + 1, &fdmask, 0, ptime);
#endif

   U_RETURN(ret);
}

int UNotifier::waitForWrite(int fd, int timeoutMS)
{
   U_TRACE(0, "UNotifier::waitForWrite(%d,%d)", fd, timeoutMS)

   U_INTERNAL_ASSERT_RANGE(0,fd,4096)

#ifdef DEBUG
   if (timeoutMS != -1) U_INTERNAL_ASSERT_MAJOR(timeoutMS,499)
#endif

#ifdef __MINGW32__
   if (is_socket(fd) == false) U_RETURN(1);
#elif defined(USE_POLL)
   struct pollfd fds[1] = { { fd, POLLOUT, 0 } };

   int ret = waitForEvent(fds, timeoutMS);
#else
   UEventTime time(0L, timeoutMS * 1000L);
   UEventTime* ptime = (timeoutMS < 0 ? 0 : (time.adjust(), &time));

   fd_set fdmask;
   FD_ZERO(&fdmask);
   FD_SET(fd, &fdmask);

   int ret = waitForEvent(fd + 1, 0, &fdmask, ptime);
#endif

   U_RETURN(ret);
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

         timeoutMS = -1; // in this way it is only for the first read...
         }

#  ifdef __MINGW32__
      (void) U_SYSCALL(ReadFile, "%p,%p,%lu,%p,%p", (HANDLE)_get_osfhandle(fd), buffer + bytes_read, (single_read ? U_CAPACITY : count - bytes_read),
                                                    (DWORD*)&value, 0);
#  else
      value = U_SYSCALL(read, "%d,%p,%u", fd, buffer + bytes_read, (single_read ? (int)U_CAPACITY : count - bytes_read));
#  endif

      if      (value ==  0) break; // EOF
      else if (value == -1)
         {
         if (UInterrupt::checkForEventSignalPending()) continue;

         break;
         }

      U_INTERNAL_DUMP("BytesRead(%d) = %#.*S", value, value, buffer + bytes_read)

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

         timeoutMS = -1; // in this way it is only for the first write...
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

      U_INTERNAL_DUMP("BytesWritten(%d) = %#.*S", value, value, str + byte_written)

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
   *UObjectIO::os
#ifdef HAVE_LIBEVENT
#elif defined(HAVE_EPOLL_WAIT)
                  << "epollfd                     " << epollfd      << '\n';
#else
                  << "fd_set_max                  " << fd_set_max   << '\n'
                  << "fd_read_cnt                 " << fd_read_cnt  << '\n'
                  << "fd_write_cnt                " << fd_write_cnt << '\n';
   *UObjectIO::os << "fd_set_read                 ";
   char _buffer[70];
    UObjectIO::os->write(_buffer, u_snprintf(_buffer, sizeof(_buffer), "%B", __FDS_BITS(&fd_set_read)[0]));
    UObjectIO::os->put('\n');
   *UObjectIO::os << "fd_set_write                ";
    UObjectIO::os->write(_buffer, u_snprintf(_buffer, sizeof(_buffer), "%B", __FDS_BITS(&fd_set_write)[0]));
    UObjectIO::os->put('\n');
#endif
   *UObjectIO::os << "pool             (UNotifier " << (void*)pool             << ")\n"
                  << "next             (UNotifier " << (void*)next             << ")\n"
                  << "first            (UNotifier " << (void*)first            << ")\n"
                  << "handler_event_fd (UEventFd  " << (void*)handler_event_fd << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
