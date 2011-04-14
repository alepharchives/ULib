// ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    trace.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIBASE_TRACE_H
#define ULIBASE_TRACE_H 1

#include <ulib/base/base.h>

#ifdef __cplusplus
extern "C" {
#endif

extern U_EXPORT int      u_trace_fd;
extern U_EXPORT char     u_trace_tab[256]; /* 256 max indent */
extern U_EXPORT uint32_t u_trace_num_tab;

extern U_EXPORT void u_trace_close(void);
extern U_EXPORT void u_trace_initFork(void);
extern U_EXPORT void u_trace_check_init(void);
extern U_EXPORT bool u_trace_isActive(int level) __pure;
extern U_EXPORT void u_trace_check_if_interrupt(void); /* check for context manage signal event - interrupt */
extern U_EXPORT void u_trace_dump(const char* restrict format, ...);
extern U_EXPORT void u_trace_write(const char* restrict t, uint32_t tlen);
extern U_EXPORT void u_trace_writev(const struct iovec* restrict iov, int n);
extern U_EXPORT void u_trace_init(bool force, bool info, bool offset);

/* E' possibile mascherare il trace per metodi eccessivamente complessi
  (Es: ricorsivi) tramite il byte alto del parametro 'level' */

extern U_EXPORT void u_trace_dtor(int active, void* restrict hook);
extern U_EXPORT bool u_trace_check_if_active(int level, void* restrict hook);

/* Attivazione-disattivazione temporanea */

extern U_EXPORT void u_trace_suspend(int resume);

#ifdef __cplusplus
}
#endif

#endif
