/** ============================================================================
//
// = LIBRARY
//    ulibase - c library
//
// = FILENAME
//    xml.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================ */

#ifndef ULIBASE_XML_H
#define ULIBASE_XML_H 1

#include <ulib/base/base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Encode-Decode xml into a buffer
*/

extern U_EXPORT uint32_t u_xml_encode(const unsigned char* s, uint32_t n, unsigned char* result);
extern U_EXPORT uint32_t u_xml_decode(const unsigned char* s, uint32_t n, unsigned char* result);

#ifdef __cplusplus
}
#endif

#endif
