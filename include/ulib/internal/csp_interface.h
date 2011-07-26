/* csp_interface.h */

#include <stdbool.h>

#include <ulib/base/base.h>
#include <ulib/internal/chttp.h>

extern char*        get_reply(void);
extern unsigned int get_reply_capacity(void);

#ifdef HAVE_V8
extern char*        runv8(const char* jssrc); /* compiles and executes javascript and returns the script return value as string */
#endif

extern unsigned int u_url_decode(            char* s, unsigned int n, unsigned char* result, int no_line_break);
extern unsigned int u_xml_decode(            char* s, unsigned int n, unsigned char* result);
extern unsigned int u_base64_decode(         char* s, unsigned int n, unsigned char* result);
extern unsigned int u_url_encode(   unsigned char* s, unsigned int n, unsigned char* result, char* extra_enc_chars);
extern unsigned int u_xml_encode(   unsigned char* s, unsigned int n, unsigned char* result);
extern unsigned int u_base64_encode(unsigned char* s, unsigned int n, unsigned char* result);
