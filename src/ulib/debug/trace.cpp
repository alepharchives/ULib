// ============================================================================
//
// = LIBRARY
//    ulibdbg - c++ library
//
// = FILENAME
//    trace.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================
 
#include <ulib/base/trace.h>
#include <ulib/base/utility.h>

#include <ulib/debug/trace.h>
#include <ulib/debug/crono.h>

#ifndef __MINGW32__
#  include <sys/uio.h>
#endif

#include <errno.h>

static UCrono* time_syscall_read_or_write;

UTrace::UTrace(int level, const char* format, ...)
{
   U_INTERNAL_TRACE("UTrace::UTrace(%d,%s)", level, format)

   // reset buffers

   buffer_trace[0]  = buffer_syscall[0]  = '\0';
   buffer_trace_len = buffer_syscall_len = 0;

   // E' possibile mascheratura del trace per metodi eccessivamente complessi (Es: ricorsivi) tramite
   // il byte alto del parametro 'level'

   active = u_trace_check_if_active(level, this);

   if (active)
      {
      va_list argp;
      va_start(argp, format);

      buffer_trace_len = u_vsnprintf(buffer_trace, sizeof(buffer_trace), format, argp);

      va_end(argp);

      u_trace_check_if_interrupt(); // check for context manage signal event

      int skip = (u_trace_num_tab == 0);

      struct iovec iov[4] = { { (caddr_t)u_trace_tab, u_trace_num_tab++ },
                              { (caddr_t)U_CONSTANT_TO_PARAM("{Call   ") },
                              { (caddr_t)buffer_trace, buffer_trace_len },
                              { (caddr_t)"\n", 1 } };

      u_trace_writev(iov + skip, 4 - skip);
      }
}

UTrace::~UTrace()
{
   U_INTERNAL_TRACE("UTrace::~UTrace()");

   if (active)
      {
      int skip = (u_trace_num_tab == 1);

      struct iovec iov[4] = { { (caddr_t)u_trace_tab, --u_trace_num_tab },
                              { (caddr_t)U_CONSTANT_TO_PARAM("}Return ") },
                              { (caddr_t)buffer_trace, buffer_trace_len },
                              { (caddr_t)"\n", 1 } };

      u_trace_writev(iov + skip, 4 - skip);
      }

   u_trace_dtor((active ? 1 : 0), this);
}

// NB: trace_return() non fa altro che aggiungere all'attributo 'buffer_trace' dell'oggetto il valore di ritorno del metodo.
// L'attributo 'buffer_trace' viene poi stampato nel distruttore dell'oggetto (vedi sopra)...

void UTrace::trace_return(const char* format, ...)
{
   U_INTERNAL_TRACE("UTrace::trace_return(%s)", format)

   if (active)
      {
      (void) strcat(buffer_trace, " = ");

      buffer_trace_len += 3;

      va_list argp;
      va_start(argp, format);

      buffer_trace_len += u_vsnprintf(buffer_trace + buffer_trace_len, sizeof(buffer_trace) - buffer_trace_len, format, argp);

      va_end(argp);

      U_INTERNAL_PRINT("buffer_trace=%s", buffer_trace)
      }
}
 
void UTrace::trace_syscall(const char* format, ...)
{
   U_INTERNAL_TRACE("UTrace::trace_syscall(%s)", format)

   va_list argp;
   va_start(argp, format);

   buffer_syscall_len = u_vsnprintf(buffer_syscall, sizeof(buffer_trace), format, argp);

   va_end(argp);

   U_INTERNAL_PRINT("buffer_syscall=%s", buffer_syscall)

   if (active)
      {
      flag_syscall_read_or_write = (U_STRNEQ(format, "::read(")  ||
                                    U_STRNEQ(format, "::write(") ||
                                    U_STRNEQ(format, "::send(")  ||
                                    U_STRNEQ(format, "::recv("));

      if (flag_syscall_read_or_write)
         {
         if (time_syscall_read_or_write == 0) time_syscall_read_or_write = new UCrono;

         time_syscall_read_or_write->start();
         }
      }

   errno = 0;
#ifdef __MINGW32__
   SetLastError(0);
#endif
}

void UTrace::trace_sysreturn(bool error, const char* format, ...)
{
   U_INTERNAL_TRACE("UTrace::trace_sysreturn(%d,%s)", error, format)

#ifdef __MINGW32__
   if (error == false &&
       active         &&
       format         &&
       format[1] == 'd') // int (BOOL for mingw)
      {
      va_list argp;
      va_start(argp, format);

      error = (va_arg(argp, int)                 == 0 &&
               strstr(buffer_syscall, "::fcntl") == 0);

      va_end(argp);
      }

   if (error      &&
       errno == 0 &&
       strstr(buffer_syscall, "::getenv") == 0)
      {
      errno = - GetLastError();

      if (errno == 0) error = false;
      }
#endif

   if (error ||
       active)
      {
      if (format) // check for system call with return void (Es: free())
         {
         (void) strcat(buffer_syscall, " = ");

         buffer_syscall_len += 3;

         va_list argp;
         va_start(argp, format);

         buffer_syscall_len += u_vsnprintf(buffer_syscall + buffer_syscall_len, sizeof(buffer_syscall) - buffer_syscall_len, format, argp);

         va_end(argp);

         U_INTERNAL_PRINT("buffer_syscall=%s", buffer_syscall)

         if (error)
            {
            if (errno)
               {
               char msg_sys_error[sizeof(buffer_syscall)];

               buffer_syscall_len += u_snprintf(msg_sys_error, sizeof(buffer_syscall), "%R", 0); // NB: the last argument (0) is necessary...

               U_INTERNAL_ASSERT_MINOR(buffer_syscall_len, sizeof(buffer_syscall))

               (void) strcat(buffer_syscall, msg_sys_error);

               u_errno = errno;
               }

            U_WARNING("%s", buffer_syscall);
            }
         }

      if (active)
         {
         if (error == false &&
             flag_syscall_read_or_write)
            {
            va_list argp;
            va_start(argp, format);
            ssize_t bytes_read_or_write = va_arg(argp, ssize_t);
            va_end(argp);

            // NB: check se vale la pena di dare l'informazione... (la dimensione di I/O puo' essere significativa)

            if (bytes_read_or_write > (ssize_t)(10 * 1024))
               {
                             time_syscall_read_or_write->stop();
               long dltime = time_syscall_read_or_write->getTimeElapsed();

               if (dltime > 0)
                  {
                  int units;
                  char msg[sizeof(buffer_syscall)];
                  double rate = u_calcRate(bytes_read_or_write, dltime, &units);

                  buffer_syscall_len += u_snprintf(msg, sizeof(buffer_syscall), " (%7.2f%s/s)", rate, u_short_units[units]);

                  U_INTERNAL_ASSERT_MINOR(buffer_syscall_len, sizeof(buffer_syscall))

                  (void) strcat(buffer_syscall, msg);
                  }
               }
            }

         struct iovec iov[3] = { { (caddr_t)u_trace_tab,    u_trace_num_tab },
                                 { (caddr_t)buffer_syscall, buffer_syscall_len },
                                 { (caddr_t)"\n", 1 } };

         u_trace_writev(iov, 3);
         }
      }
}
