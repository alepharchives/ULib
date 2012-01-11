// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    usp_macro.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_USP_MACRO_H
#define U_USP_MACRO_H 1

#include <ulib/all.h>

#define USP_PUTS(string)        (void)UClientImage_Base::wbuffer->append(string)
#define USP_PRINTF(fmt,args...) (UClientImage_Base::_buffer->snprintf(fmt , ##args),USP_PUTS(*UClientImage_Base::_buffer))

#define USP_FORM_NAME(n)               (UHTTP::getFormValue(*UClientImage_Base::_value,(0+(n*2))),                *UClientImage_Base::_value)
#define USP_FORM_VALUE(n)              (UHTTP::getFormValue(*UClientImage_Base::_value,(1+(n*2))),                *UClientImage_Base::_value)
#define USP_FORM_VALUE_FROM_NAME(name) (UHTTP::getFormValue(*UClientImage_Base::_value,U_CONSTANT_TO_PARAM(name)),*UClientImage_Base::_value)

#define USP_PRINTF_FORM_NAME(fmt,n)    (USP_FORM_NAME(n), USP_PRINTF(fmt,U_STRING_TO_TRACE(*UClientImage_Base::_value)))
#define USP_PRINTF_FORM_VALUE(fmt,n)   (USP_FORM_VALUE(n),USP_PRINTF(fmt,U_STRING_TO_TRACE(*UClientImage_Base::_value)))

#define USP_PRINTF_XML(fmt,args...) \
            (UClientImage_Base::_buffer->snprintf(fmt , ##args), \
             UXMLEscape::encode(*UClientImage_Base::_buffer,*UClientImage_Base::_encoded), \
             USP_PUTS(*UClientImage_Base::_encoded))

#define USP_PRINTF_XML_FORM_NAME(fmt,n)  ((void)USP_FORM_NAME( n),USP_PRINTF_XML(fmt,U_STRING_TO_TRACE(*UClientImage_Base::_value)))
#define USP_PRINTF_XML_FORM_VALUE(fmt,n) ((void)USP_FORM_VALUE(n),USP_PRINTF_XML(fmt,U_STRING_TO_TRACE(*UClientImage_Base::_value)))

#endif
