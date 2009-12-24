/** ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    url.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================ */

#ifndef ULIBASE_URL_H
#define ULIBASE_URL_H 1

#include <ulib/base/base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Encode-Decode url into a buffer

Synopsis: Performs HTTP escaping on a string. This works as follows: all characters except alphanumerics
          and spaces are converted into the 3-byte sequence "%xx" where xx is the character's hexadecimal value;
          spaces are replaced by '+'. Line breaks are stored as "%0D%0A", where a 'line break' is any one of:
          "\n", "\r", "\n\r", or "\r\n"
*/

/* RFC2231 encoding (which in HTTP is mostly used for giving UTF8-encoded filenames in the Content-Disposition header) */

#define U_RFC2231 " *'%()<>@,;:\\\"/[]?="

extern U_EXPORT uint32_t u_url_encode(const unsigned char* s, uint32_t n, unsigned char* result, const char* extra_enc_chars);
extern U_EXPORT uint32_t u_url_decode(const unsigned char* s, uint32_t n, unsigned char* result, bool no_line_break);

#ifdef __cplusplus
}
#endif

#endif
