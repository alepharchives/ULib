/* csp_interface.h */

#include <stdbool.h>

#include <ulib/base/base.h>
#include <ulib/internal/chttp.h>

#ifdef HAVE_V8
extern char* runv8(const char* jssrc); /* compiles and executes javascript and returns the script return value as string */
#endif

extern char*        get_reply(void);
extern unsigned int get_reply_capacity(void);
