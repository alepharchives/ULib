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

extern U_EXPORT void*    u_plock;
extern U_EXPORT int      u_trace_fd;
extern U_EXPORT char     u_trace_tab[256]; /* 256 max indent */
extern U_EXPORT uint32_t u_trace_num_tab;

U_EXPORT void u_trace_close(void);
U_EXPORT void u_trace_initFork(void);
U_EXPORT void u_trace_check_init(void);
U_EXPORT bool u_trace_isActive(int level) __pure;
U_EXPORT void u_trace_check_if_interrupt(void); /* check for context manage signal event - interrupt */
U_EXPORT void u_trace_dump(const char* restrict format, ...);
U_EXPORT void u_trace_write(const char* restrict t, uint32_t tlen);
U_EXPORT void u_trace_writev(const struct iovec* restrict iov, int n);
U_EXPORT void u_trace_init(bool force, bool info, bool offset);

/* E' possibile mascherare il trace per metodi eccessivamente complessi
  (Es: ricorsivi) tramite il byte alto del parametro 'level' */

U_EXPORT void u_trace_dtor(int active, void* restrict hook);
U_EXPORT bool u_trace_check_if_active(int level, void* restrict hook);

/* Attivazione-disattivazione temporanea */

U_EXPORT void u_trace_suspend(int resume);

#ifdef __cplusplus
}
#endif

#endif
