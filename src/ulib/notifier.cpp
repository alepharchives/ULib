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
#include <ulib/net/socket.h>
#include <ulib/utility/interrupt.h>

#ifdef HAVE_PTHREAD_H
#  include "ulib/thread.h"
#endif

#include <errno.h>

#ifdef HAVE_LIBEVENT
void UEventFd::operator()(int _fd, short event)
{
   U_TRACE(0, "UEventFd::operator()(%d,%hd)", _fd, event)

   int ret = (event == EV_READ ? handlerRead() : handlerWrite());

   U_INTERNAL_DUMP("ret = %d", ret)

   if (ret == U_NOTIFIER_DELETE) UNotifier::erase(this);
}
#elif defined(HAVE_EPOLL_WAIT)
int                 UNotifier::epollfd;
struct epoll_event* UNotifier::events;
struct epoll_event* UNotifier::pevents;
#else
int                 UNotifier::fd_set_max;
int                 UNotifier::fd_read_cnt;
int                 UNotifier::fd_write_cnt;
fd_set              UNotifier::fd_set_read;
fd_set              UNotifier::fd_set_write;
#endif
#ifdef USE_POLL
struct pollfd       UNotifier::fds[1];
#endif

int                              UNotifier::nfd_ready; // the number of file descriptors ready for the requested I/O
int                              UNotifier::min_connection;
int                              UNotifier::num_connection;
int                              UNotifier::max_connection;
void*                            UNotifier::pthread;
UEventFd**                       UNotifier::lo_map_fd;
UGenericHashMap<int,UEventFd*>*  UNotifier::hi_map_fd; // maps a fd to a node pointer

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
   epollfd = U_SYSCALL(epoll_create, "%d", max_connection);
next:
   U_INTERNAL_ASSERT_MAJOR(epollfd,0)
   U_INTERNAL_ASSERT_EQUALS(U_WRITE_OUT,EPOLLOUT|EPOLLET)

   if (old)
      {
      U_INTERNAL_DUMP("num_connection = %u", num_connection)

      if (num_connection)
         {
         U_INTERNAL_ASSERT_POINTER(lo_map_fd)
         U_INTERNAL_ASSERT_POINTER(hi_map_fd)

         // NB: reinitialized all after fork()...

         int fd;
         UEventFd* handler_event;

         for (fd = 1; fd < max_connection; ++fd)
            {
            if ((handler_event = lo_map_fd[fd]))
               {
               U_INTERNAL_DUMP("fd = %d op_mask = %d %B", fd, handler_event->op_mask, handler_event->op_mask)

               (void) U_SYSCALL(epoll_ctl, "%d,%d,%d,%p", old, EPOLL_CTL_DEL, fd, (struct epoll_event*)1);

               struct epoll_event _events = { handler_event->op_mask | EPOLLRDHUP, { handler_event } };

               (void) U_SYSCALL(epoll_ctl, "%d,%d,%d,%p", epollfd, EPOLL_CTL_ADD, fd, &_events);
               }
            }

         if (hi_map_fd->first())
            {
            do {
               handler_event = hi_map_fd->elem();

               U_INTERNAL_DUMP("fd = %d op_mask = %d %B", fd, handler_event->op_mask, handler_event->op_mask)

               (void) U_SYSCALL(epoll_ctl, "%d,%d,%d,%p", old, EPOLL_CTL_DEL, handler_event->fd, (struct epoll_event*)1);

               struct epoll_event _events = { handler_event->op_mask | EPOLLRDHUP, { handler_event } };

               (void) U_SYSCALL(epoll_ctl, "%d,%d,%d,%p", epollfd, EPOLL_CTL_ADD, handler_event->fd, &_events);
               }
            while (hi_map_fd->next());
            }
         }

      (void) U_SYSCALL(close, "%d", old);

      return;
      }
#endif

   if (lo_map_fd == 0)
      {
      U_INTERNAL_ASSERT_EQUALS(hi_map_fd,0)
      U_INTERNAL_ASSERT_MAJOR(max_connection,0)

      lo_map_fd = U_CALLOC_VECTOR(max_connection, UEventFd);

      hi_map_fd = U_NEW(UGenericHashMap<int,UEventFd*>);

      hi_map_fd->allocate(max_connection);

#  if defined(HAVE_EPOLL_WAIT) && !defined(HAVE_LIBEVENT)
      U_INTERNAL_ASSERT_EQUALS(events,0)

      pevents = events = U_CALLOC_N(max_connection+1, struct epoll_event);
#  endif
      }
}

UEventFd* UNotifier::find(int fd)
{
   U_TRACE(0, "UNotifier::find(%d)", fd)

   U_INTERNAL_ASSERT_POINTER(lo_map_fd)
   U_INTERNAL_ASSERT_POINTER(hi_map_fd)

   if (fd < max_connection)
      {
      U_INTERNAL_DUMP("num_connection = %u", num_connection)

      U_RETURN_POINTER(lo_map_fd[fd],UEventFd);
      }

#ifdef HAVE_PTHREAD_H
   if (pthread) ((UThread*)pthread)->lock();
#endif

   UEventFd* handler_event = (hi_map_fd->find(fd) ? hi_map_fd->elem() : 0);

#ifdef HAVE_PTHREAD_H
   if (pthread) ((UThread*)pthread)->unlock();
#endif

   U_RETURN_POINTER(handler_event,UEventFd);
}

void UNotifier::insert(UEventFd* item)
{
   U_TRACE(0, "UNotifier::insert(%p)", item)

   U_INTERNAL_ASSERT_POINTER(item)

   int fd = item->fd;

   U_INTERNAL_DUMP("fd = %d op_mask = %B num_connection = %d", fd, item->op_mask, num_connection)

   U_INTERNAL_ASSERT_MAJOR(fd,0)
   U_ASSERT_EQUALS(find(fd),0)

   if (fd < max_connection) lo_map_fd[fd] = item;
   else
      {
#  ifdef HAVE_PTHREAD_H
      if (pthread) ((UThread*)pthread)->lock();
#  endif

      hi_map_fd->insert(fd, item);

#  ifdef HAVE_PTHREAD_H
      if (pthread) ((UThread*)pthread)->unlock();
#  endif
      }

#ifdef HAVE_LIBEVENT
   U_INTERNAL_ASSERT_POINTER(u_ev_base)

   int mask = EV_PERSIST | (item->op_mask == U_READ_IN ? EV_READ : EV_WRITE);

   U_INTERNAL_DUMP("mask = %B", mask)

   item->pevent = U_NEW(UEvent<UEventFd>(fd, mask, *item));

   (void) UDispatcher::add(*(item->pevent));
#elif defined(HAVE_EPOLL_WAIT)
   U_INTERNAL_ASSERT_MAJOR(epollfd,0)

   struct epoll_event _events = { item->op_mask | EPOLLRDHUP, { item } };

   (void) U_SYSCALL(epoll_ctl, "%d,%d,%d,%p", epollfd, EPOLL_CTL_ADD, fd, &_events);
#else
   if (item->op_mask == U_READ_IN)
      {
      U_INTERNAL_ASSERT_EQUALS(FD_ISSET(fd, &fd_set_read),0)
      U_INTERNAL_ASSERT_EQUALS(FD_ISSET(fd, &fd_set_write),0)

      FD_SET(fd, &fd_set_read);

      U_INTERNAL_ASSERT(fd_read_cnt >= 0)

      ++fd_read_cnt;

#  ifndef __MINGW32__
      U_INTERNAL_DUMP("fd_set_read = %B", __FDS_BITS(&fd_set_read)[0])
#  endif
      }
   else
      {
      U_INTERNAL_ASSERT_EQUALS(item->op_mask,U_WRITE_OUT)
      U_INTERNAL_ASSERT_EQUALS(FD_ISSET(fd, &fd_set_write),0)

      FD_SET(fd, &fd_set_write);

      U_INTERNAL_ASSERT(fd_write_cnt >= 0)

      ++fd_write_cnt;

#  ifndef __MINGW32__
      U_INTERNAL_DUMP("fd_set_write = %B", __FDS_BITS(&fd_set_write)[0])
#  endif
      }

   if (fd_set_max <= fd) fd_set_max = fd + 1;

   U_INTERNAL_DUMP("fd_set_max = %d", fd_set_max)
#endif
}

U_NO_EXPORT void UNotifier::handlerDelete(UEventFd* item)
{
   U_TRACE(0, "UNotifier::handlerDelete(%p)", item)

   U_INTERNAL_ASSERT_POINTER(item)

   int fd = item->fd;

   U_INTERNAL_DUMP("fd = %d op_mask = %B num_connection = %d", fd, item->op_mask, num_connection)

   U_INTERNAL_ASSERT_MAJOR(fd,0)
   U_ASSERT_EQUALS(item,find(fd))

   if (fd < max_connection) lo_map_fd[fd] = 0;
   else
      {
#  ifdef HAVE_PTHREAD_H
      if (pthread) ((UThread*)pthread)->lock();
#  endif

      (void) hi_map_fd->erase(fd);

#  ifdef HAVE_PTHREAD_H
      if (pthread) ((UThread*)pthread)->unlock();
#  endif
      }

#ifdef HAVE_LIBEVENT
#elif defined(HAVE_EPOLL_WAIT)
#else
   if (item->op_mask == U_READ_IN)
      {
      U_INTERNAL_ASSERT(FD_ISSET(fd, &fd_set_read))
      U_INTERNAL_ASSERT_EQUALS(FD_ISSET(fd, &fd_set_write),0)

      FD_CLR(fd, &fd_set_read);

#  ifndef __MINGW32__
      U_INTERNAL_DUMP("fd_set_read = %B", __FDS_BITS(&fd_set_read)[0])
#  endif

      --fd_read_cnt;

      U_INTERNAL_ASSERT(fd_read_cnt >= 0)
      }
   else
      {
      U_INTERNAL_ASSERT(FD_ISSET(fd, &fd_set_write))
      U_INTERNAL_ASSERT_EQUALS(item->op_mask, U_WRITE_OUT)
      U_INTERNAL_ASSERT_EQUALS(FD_ISSET(fd, &fd_set_read),0)

      FD_CLR(fd, &fd_set_write);

#  ifndef __MINGW32__
      U_INTERNAL_DUMP("fd_set_write = %B", __FDS_BITS(&fd_set_write)[0])
#  endif

      --fd_write_cnt;

      U_INTERNAL_ASSERT(fd_write_cnt >= 0)
      }

   if (empty() == false) fd_set_max = getNFDS();
#endif

   item->handlerDelete();

   U_INTERNAL_ASSERT_EQUALS(item->fd,0)
}

void UNotifier::modify(UEventFd* item)
{
   U_TRACE(0, "UNotifier::modify(%p)", item)

   U_INTERNAL_ASSERT_POINTER(item)

   int fd = item->fd;

   U_INTERNAL_DUMP("fd = %d op_mask = %B", fd, item->op_mask)

   U_INTERNAL_ASSERT_MAJOR(fd,0)
   U_ASSERT_EQUALS(item,find(fd))

#ifdef HAVE_LIBEVENT
   U_INTERNAL_ASSERT_POINTER(u_ev_base)

   int mask = EV_PERSIST | (item->op_mask == U_READ_IN ? EV_READ : EV_WRITE);

   U_INTERNAL_DUMP("mask = %B", mask)

   UDispatcher::del(item->pevent);
             delete item->pevent;
                    item->pevent = U_NEW(UEvent<UEventFd>(fd, mask, *item));

   (void) UDispatcher::add(*(item->pevent));
#elif defined(HAVE_EPOLL_WAIT)
   U_INTERNAL_ASSERT_MAJOR(epollfd,0)

   struct epoll_event _events = { item->op_mask | EPOLLRDHUP, { item } };

   (void) U_SYSCALL(epoll_ctl, "%d,%d,%d,%p", epollfd, EPOLL_CTL_MOD, fd, &_events);
#else
#  ifndef __MINGW32__
   U_INTERNAL_DUMP("fd_set_read  = %B", __FDS_BITS(&fd_set_read)[0])
   U_INTERNAL_DUMP("fd_set_write = %B", __FDS_BITS(&fd_set_write)[0])
#endif

   if (item->op_mask == U_READ_IN)
      {
      U_INTERNAL_ASSERT(FD_ISSET(fd, &fd_set_write))
      U_INTERNAL_ASSERT_EQUALS(FD_ISSET(fd, &fd_set_read),0)

      FD_SET(fd, &fd_set_read);
      FD_CLR(fd, &fd_set_write);

      ++fd_read_cnt;
      --fd_write_cnt;
      }
   else
      {
      U_INTERNAL_ASSERT(FD_ISSET(fd, &fd_set_read))
      U_INTERNAL_ASSERT_EQUALS(item->op_mask, U_WRITE_OUT)
      U_INTERNAL_ASSERT_EQUALS(FD_ISSET(fd, &fd_set_write),0)

      FD_CLR(fd, &fd_set_read);
      FD_SET(fd, &fd_set_write);

      --fd_read_cnt;
      ++fd_write_cnt;
      }

#  ifndef __MINGW32__
   U_INTERNAL_DUMP("fd_set_read  = %B", __FDS_BITS(&fd_set_read)[0])
   U_INTERNAL_DUMP("fd_set_write = %B", __FDS_BITS(&fd_set_write)[0])
#endif

   U_INTERNAL_ASSERT(fd_read_cnt  >= 0)
   U_INTERNAL_ASSERT(fd_write_cnt >= 0)
#endif
}

void UNotifier::erase(UEventFd* item)
{
   U_TRACE(1, "UNotifier::erase(%p)", item)

   U_INTERNAL_ASSERT_POINTER(item)

   handlerDelete(item);

#ifdef HAVE_LIBEVENT
   UDispatcher::del(item->pevent);
             delete item->pevent;
                    item->pevent = 0;
#endif
}

int UNotifier::waitForEvent(int fd_max, fd_set* read_set, fd_set* write_set, UEventTime* timeout)
{
   U_TRACE(1, "UNotifier::waitForEvent(%d,%p,%p,%p)", fd_max, read_set, write_set, timeout)

   int result;

#ifdef HAVE_LIBEVENT
   result = -1;
#elif defined(HAVE_EPOLL_WAIT)
   int _timeout = (timeout ? timeout->getMilliSecond() : -1);
loop:
   result = U_SYSCALL(epoll_wait, "%d,%p,%d,%d", epollfd, events, max_connection, _timeout);
#else
   static struct timeval   tmp;
          struct timeval* ptmp = (timeout == 0 ? 0 : &tmp);
loop:
#  if defined(DEBUG) && !defined(__MINGW32__)
   if ( read_set) U_INTERNAL_DUMP(" read_set = %B", __FDS_BITS( read_set)[0])
   if (write_set) U_INTERNAL_DUMP("write_set = %B", __FDS_BITS(write_set)[0])
#  endif

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
#endif

#ifndef HAVE_LIBEVENT
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
          UInterrupt::exit_loop_wait_event_for_signal == false)
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

#  if defined(DEBUG) && !defined(__MINGW32__)
   if ( read_set) U_INTERNAL_DUMP(" read_set = %B", __FDS_BITS( read_set)[0])
   if (write_set) U_INTERNAL_DUMP("write_set = %B", __FDS_BITS(write_set)[0])
#  endif
#endif

   U_RETURN(result);
}

#ifndef HAVE_LIBEVENT
U_NO_EXPORT void UNotifier::handlerResult(UEventFd* handler_event, bool bread, bool bexcept)
{
   U_TRACE(0, "UNotifier::handlerResult(%p,%b,%b)", handler_event, bread, bexcept)

   int ret;

   U_INTERNAL_ASSERT_POINTER(handler_event)

   U_INTERNAL_DUMP("fd = %d op_mask = %B", handler_event->fd, handler_event->op_mask)

   if (bread ||
       bexcept)
      {
#  ifndef HAVE_EPOLL_WAIT
      U_INTERNAL_ASSERT_MAJOR(fd_read_cnt,0)
      U_INTERNAL_ASSERT(FD_ISSET(handler_event->fd, &fd_set_read))
#  endif

      ret = (bexcept ? (handler_event->handlerError(USocket::BROKEN | USocket::EPOLLERROR), U_NOTIFIER_DELETE)
                     :  handler_event->handlerRead());
      }
   else
      {
      U_INTERNAL_ASSERT_EQUALS(handler_event->op_mask,U_WRITE_OUT)

#  ifdef HAVE_EPOLL_WAIT
      U_INTERNAL_ASSERT_DIFFERS((pevents->events & U_WRITE_OUT),0)
#  else
      U_INTERNAL_ASSERT_MAJOR(fd_write_cnt,0)
      U_INTERNAL_ASSERT(FD_ISSET(handler_event->fd, &fd_set_write))
#  endif

      ret = handler_event->handlerWrite();
      }

   --nfd_ready;

   U_INTERNAL_DUMP("nfd_ready = %d", nfd_ready)

   if (ret == U_NOTIFIER_DELETE)
      {
#  ifdef HAVE_EPOLL_WAIT
      U_INTERNAL_ASSERT_EQUALS(handler_event, pevents->data.ptr)
#  endif

      handlerDelete(handler_event);
      }
}
#endif

// return false if there are no more events registered...

bool UNotifier::waitForEvent(UEventTime* timeout)
{
   U_TRACE(0, "UNotifier::waitForEvent(%p)", timeout)

#ifdef HAVE_LIBEVENT
   (void) UDispatcher::dispatch(UDispatcher::ONCE);
#elif defined(HAVE_EPOLL_WAIT)
   nfd_ready = waitForEvent(0, 0, 0, timeout);
#else
   U_INTERNAL_ASSERT(fd_read_cnt > 0 || fd_write_cnt > 0)

   fd_set read_set, write_set;

   if (fd_read_cnt)   read_set = fd_set_read;
   if (fd_write_cnt) write_set = fd_set_write;

   nfd_ready = waitForEvent(fd_set_max,
                (fd_read_cnt  ? &read_set
                              : 0),
                (fd_write_cnt ? &write_set
                              : 0),
                timeout);
#endif
#ifndef HAVE_LIBEVENT
   if (nfd_ready > 0)
      {
      bool bread;
      UEventFd* handler_event;

#  ifdef HAVE_EPOLL_WAIT
      bool bexcept;
      pevents = events + nfd_ready;

loop:
      --pevents;

      U_INTERNAL_ASSERT(pevents >= events)

      handler_event = (UEventFd*) pevents->data.ptr;

      U_INTERNAL_ASSERT_DIFFERS(handler_event,0)

      U_INTERNAL_DUMP("events[%d].events = %B", (pevents - events), pevents->events)

      bread   = ((pevents->events & U_READ_IN)  != 0);
      bexcept = ((pevents->events & EPOLLERR)   != 0 ||
                 (pevents->events & EPOLLHUP)   != 0 ||
                 (pevents->events & EPOLLRDHUP) != 0);

      U_INTERNAL_DUMP("bread = %b bwrite = %b bexcept = %b", bread, ((pevents->events & U_WRITE_OUT) != 0), bexcept)

      U_INTERNAL_ASSERT((pevents->events & U_READ_IN)   != 0 ||
                        (pevents->events & U_WRITE_OUT) != 0 ||
                        (pevents->events & EPOLLHUP)    != 0 ||
                        (pevents->events & EPOLLERR)    != 0 ||
                        (pevents->events & EPOLLRDHUP)  != 0)

      U_INTERNAL_DUMP("fd = %d op_mask = %B handler_event = %p", handler_event->fd, handler_event->op_mask, handler_event)

      U_INTERNAL_ASSERT_MAJOR(handler_event->fd,0)

      handlerResult(handler_event, bread, bexcept);

      if (nfd_ready == 0)
         {
         U_INTERNAL_DUMP("events[%d]: goto end", (pevents - events))

         U_INTERNAL_ASSERT_EQUALS(pevents, events)

         goto end;
         }

      goto loop;
#  else
      int fd, fd_cnt = (fd_read_cnt + fd_write_cnt);

      U_INTERNAL_DUMP("fd_cnt = %d", fd_cnt)

      U_INTERNAL_ASSERT(nfd_ready <= fd_cnt)

      for (fd = 1; fd < fd_set_max; ++fd)
         {
         bread = (fd_read_cnt && FD_ISSET(fd, &read_set));

         if (bread ||
             (fd_write_cnt && FD_ISSET(fd, &write_set)))
            {
            handler_event = find(fd);

            U_INTERNAL_ASSERT_DIFFERS(handler_event,0)

            U_INTERNAL_DUMP("fd = %d op_mask = %B handler_event = %p", handler_event->fd, handler_event->op_mask, handler_event)

            handlerResult(handler_event, bread, false);

            if (nfd_ready == 0)
               {
               U_INTERNAL_DUMP("fd = %d: goto end", fd)

               if (fd_cnt > (fd_read_cnt + fd_write_cnt)) fd_set_max = getNFDS();

               goto end;
               }
            }
         }
#  endif
      }
end:
#endif
   if (empty()) U_RETURN(false); // empty queue

   U_RETURN(true);
}

void UNotifier::callForAllEntryDynamic(bPFpv function)
{
   U_TRACE(0, "UNotifier::callForAllEntryDynamic(%p)", function)

   UEventFd* item;

   U_INTERNAL_DUMP("num_connection = %u", num_connection)

   if (num_connection > min_connection)
      {
      for (int fd = 1; fd < max_connection; ++fd)
         {
         if ((item = lo_map_fd[fd]))
            {
            U_INTERNAL_DUMP("fd = %d op_mask = %B", item->fd, item->op_mask)

            if (function(item)) erase(item);
            }
         }

      if (hi_map_fd->first())
         {
         do {
            item = hi_map_fd->elem();

            U_INTERNAL_DUMP("fd = %d op_mask = %B", item->fd, item->op_mask)

            if (function(item)) erase(item);
            }
         while (hi_map_fd->next());
         }
      }
}

void UNotifier::clear(bool bthread)
{
   U_TRACE(0, "UNotifier::clear(%b)", bthread)

   U_INTERNAL_ASSERT_POINTER(lo_map_fd)
   U_INTERNAL_ASSERT_POINTER(hi_map_fd)
   U_INTERNAL_ASSERT_MAJOR(max_connection,0)

   UEventFd* item;

   U_INTERNAL_DUMP("num_connection = %u", num_connection)

   if (num_connection)
      {
      for (int fd = 1; fd < max_connection; ++fd)
         {
         if ((item = lo_map_fd[fd]))
            {
            U_INTERNAL_DUMP("fd = %d op_mask = %B", item->fd, item->op_mask)

            if (item->fd) erase(item);
            }
         }

      if (hi_map_fd->first())
         {
         do {
            item = hi_map_fd->elem();

            U_INTERNAL_DUMP("fd = %d op_mask = %B", item->fd, item->op_mask)

            if (item->fd) erase(item);
            }
         while (hi_map_fd->next());
         }
      }

   if (bthread == false)
      {
      U_FREE_VECTOR(lo_map_fd, max_connection, UEventFd);

      hi_map_fd->deallocate();

      delete hi_map_fd;
      }

#if defined(HAVE_EPOLL_WAIT) && !defined(HAVE_LIBEVENT)
   U_INTERNAL_ASSERT_POINTER(events)

   if (bthread == false) U_FREE_N(events, max_connection+1, struct epoll_event);

   (void) U_SYSCALL(close, "%d", epollfd);
#endif
}

#ifdef HAVE_LIBEVENT
#elif defined(HAVE_EPOLL_WAIT)
#else
int UNotifier::getNFDS() // nfds is the highest-numbered file descriptor in any of the three sets, plus 1.
{
   U_TRACE(0, "UNotifier::getNFDS()")

   int fd, nfds = 0;

   U_INTERNAL_DUMP("fd_set_max = %d", fd_set_max)

   for (fd = 1; fd < fd_set_max; ++fd)
      {
      if (find(fd))
         {
         U_INTERNAL_DUMP("fd = %d", fd)

         if (nfds < fd) nfds = fd;
         }
      }

   ++nfds;

   U_RETURN(nfds);
}

void UNotifier::removeBadFd()
{
   U_TRACE(1, "UNotifier::removeBadFd()")

   int fd;
   fd_set fdmask;
   fd_set* rmask;
   fd_set* wmask;
   UEventFd* handler_event;
   bool bread, bwrite, bexcept;
   struct timeval polling = { 0L, 0L };

   for (fd = 1; fd < fd_set_max; ++fd)
      {
      handler_event = find(fd);

      if (handler_event == 0) continue;

      U_INTERNAL_DUMP("fd = %d op_mask = %B handler_event = %p", handler_event->fd, handler_event->op_mask, handler_event)

      bread  = (fd_read_cnt  && (handler_event->op_mask & U_READ_IN));
      bwrite = (fd_write_cnt && (handler_event->op_mask & U_WRITE_OUT));

      FD_ZERO(&fdmask);
      FD_SET(fd, &fdmask);

      rmask = (bread  ? &fdmask : 0);
      wmask = (bwrite ? &fdmask : 0);

      U_INTERNAL_DUMP("fdmask = %B", __FDS_BITS(&fdmask)[0])

      nfd_ready = U_SYSCALL(select, "%d,%p,%p,%p,%p", fd+1, rmask, wmask, 0, &polling);

      U_INTERNAL_DUMP("fd = %d op_mask = %B ISSET(read) = %b ISSET(write) = %b", fd, handler_event->op_mask,
                        (rmask ? FD_ISSET(fd, rmask) : false),
                        (wmask ? FD_ISSET(fd, wmask) : false))

      if (nfd_ready)
         {
         bexcept = (nfd_ready == -1 ? (nfd_ready = (bread + bwrite), true) : false);

         handlerResult(handler_event, bread, bexcept);
         }
      }
}
#endif

// param timeoutMS specified the timeout value, in milliseconds.
// A negative value indicates no timeout, i.e. an infinite wait

#ifdef USE_POLL
int UNotifier::waitForEvent(int timeoutMS)
{
   U_TRACE(1, "UNotifier::waitForEvent(%d)", timeoutMS)

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

#ifdef USE_POLL
   // NB: POLLRDHUP stream socket peer closed connection, or ***** shut down writing half of connection ****

   fds[0].fd     = fd;
   fds[0].events = POLLIN;

   int ret = waitForEvent(timeoutMS);

   // NB: we don't check for POLLERR or POLLHUP because we have problem in same case (command.test)

   U_INTERNAL_DUMP("revents = %d %B", fds[0].revents, fds[0].revents)
#else

#  ifdef __MINGW32__
   HANDLE h = is_pipe(fd);

   if (h != INVALID_HANDLE_VALUE)
      {
      DWORD count = 0;

      while (U_SYSCALL(PeekNamedPipe, "%p,%p,%ld,%p,%p,%p", h, 0, 0, 0, &count, 0) &&
             count == 0                                                            &&
             timeoutMS > 0)
         {
         Sleep(1000);

         timeoutMS -= 1000;
         }

      U_RETURN(count);
      }
#  endif

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

#ifdef USE_POLL
   // NB: POLLRDHUP stream socket peer closed connection, or ***** shut down writing half of connection ****

   fds[0].fd      = fd;
   fds[0].events  = POLLOUT;
   fds[0].revents = 0;

   int ret = waitForEvent(timeoutMS);

   // NB: POLLERR Error condition (output only).
   //     POLLHUP Hang up         (output only).

   U_INTERNAL_DUMP("revents = %d %B", fds[0].revents, fds[0].revents)

   if (ret == 1 &&
       (((fds[0].revents & POLLERR) != 0) ||
        ((fds[0].revents & POLLHUP) != 0)))
      {
      U_INTERNAL_ASSERT_EQUALS(::write(fd,fds,1),-1)

      U_RETURN(-1);
      }
#else

#  ifdef __MINGW32__
   if (is_pipe(fd) != INVALID_HANDLE_VALUE) U_RETURN(1);
#  endif

   UEventTime time(0L, timeoutMS * 1000L);
   UEventTime* ptime = (timeoutMS < 0 ? 0 : (time.adjust(), &time));

   fd_set fdmask;
   FD_ZERO(&fdmask);
   FD_SET(fd, &fdmask);

   int ret = waitForEvent(fd + 1, 0, &fdmask, ptime);
#endif

   U_RETURN(ret);
}

// param timeoutms specified the timeout value, in milliseconds.
// a negative value indicates no timeout, i.e. an infinite wait

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
         if (waitForRead(fd, timeoutMS) <= 0) break;

         timeoutMS = -1; // in this way it is only for the first read...
         }

#  ifdef __MINGW32__
      (void) U_SYSCALL(ReadFile, "%p,%p,%lu,%p,%p", (HANDLE)_get_osfhandle(fd), buffer + bytes_read, (single_read ? U_CAPACITY : count - bytes_read),
                                                    (DWORD*)&value, 0);
#  else
      value = U_SYSCALL(read, "%d,%p,%u", fd, buffer + bytes_read, (single_read ? (int)U_CAPACITY : count - bytes_read));
#  endif

      if      (value ==  0) break; // eof
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
         if (waitForWrite(fd, timeoutMS) <= 0) break;

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

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UNotifier::dump(bool reset) const
{
#ifdef HAVE_LIBEVENT
#elif defined(HAVE_EPOLL_WAIT)
   *UObjectIO::os << "epollfd                     " << epollfd        << '\n';
#else
   *UObjectIO::os << "fd_set_max                  " << fd_set_max     << '\n'
                  << "fd_read_cnt                 " << fd_read_cnt    << '\n'
                  << "fd_write_cnt                " << fd_write_cnt   << '\n';
   *UObjectIO::os << "fd_set_read                 ";
   char _buffer[70];
    UObjectIO::os->write(_buffer, u_sn_printf(_buffer, sizeof(_buffer), "%B", __FDS_BITS(&fd_set_read)[0]));
    UObjectIO::os->put('\n');
   *UObjectIO::os << "fd_set_write                ";
    UObjectIO::os->write(_buffer, u_sn_printf(_buffer, sizeof(_buffer), "%B", __FDS_BITS(&fd_set_write)[0]));
    UObjectIO::os->put('\n');
#endif
   *UObjectIO::os << "nfd_ready                   " << nfd_ready      << '\n'
                  << "min_connection              " << min_connection << '\n'
                  << "num_connection              " << num_connection << '\n'
                  << "max_connection              " << max_connection;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
