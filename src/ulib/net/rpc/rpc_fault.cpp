// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    rpc_fault.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/net/rpc/rpc_fault.h>

const UString* URPCFault::str_encode;
const UString* URPCFault::str_sender;
const UString* URPCFault::str_receiver;
const UString* URPCFault::str_MustUnderstand;
const UString* URPCFault::str_VersionMismatch;
const UString* URPCFault::str_DataEncodingUnknown;

void URPCFault::str_allocate()
{
   U_TRACE(0, "URPCFault::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_encode,0)
   U_INTERNAL_ASSERT_EQUALS(str_sender,0)
   U_INTERNAL_ASSERT_EQUALS(str_receiver,0)
   U_INTERNAL_ASSERT_EQUALS(str_MustUnderstand,0)
   U_INTERNAL_ASSERT_EQUALS(str_VersionMismatch,0)
   U_INTERNAL_ASSERT_EQUALS(str_DataEncodingUnknown,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("%.*s: %.*s%s%.*s") },
      { U_STRINGREP_FROM_CONSTANT("Client") },
      { U_STRINGREP_FROM_CONSTANT("Server") },
      { U_STRINGREP_FROM_CONSTANT("MustUnderstand") },
      { U_STRINGREP_FROM_CONSTANT("VersionMismatch") },
      { U_STRINGREP_FROM_CONSTANT("DataEncodingUnknown") }
   };

   U_NEW_ULIB_OBJECT(str_encode,              U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_sender,              U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_receiver,            U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_MustUnderstand,      U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_VersionMismatch,     U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_DataEncodingUnknown, U_STRING_FROM_STRINGREP_STORAGE(5));
}

void URPCFault::setDetail(const char* format, ...)
{
   U_TRACE(0, "URPCFault::setDetail(%S)", format)

   va_list argp;
   va_start(argp, format);

   detail.vsnprintf(format, argp);

   va_end(argp);
}

UString URPCFault::getFaultCode()
{
   U_TRACE(0, "URPCFault::getFaultCode()")

   UString retval;

   /* SOAP:
    * ----------------------------------------------------------------------------------------------------
    * VersionMismatch: The faulting node found an invalid element information item instead of the expected
    * Envelope element information item. The namespace, local name or both did not match
    * the Envelope element information item required by this recommendation.  
    *
    * MustUnderstand: An immediate child element information item of the SOAP Header element information
    * item targeted at the faulting node that was not understood by the faulting node
    * contained a SOAP mustUnderstand attribute information item with a value of "true".
    *
    * DataEncodingUnknown: A SOAP header block or SOAP body child element information item targeted at the
    * faulting SOAP node is scoped with a data encoding that the faulting node does not support.
    *
    * Sender: The message was incorrectly formed or did not contain the appropriate information in order to
    * succeed. For example, the message could lack the proper authentication or payment information.
    * It is generally an indication that the message is not to be resent without change.
    *
    * Receiver: The message could not be processed for reasons attributable to the processing of the message
    * rather than to the contents of the message itself. For example, processing could include
    * communicating with an upstream SOAP node, which did not respond.  The message could succeed
    * if resent at a later point in time.
    * ----------------------------------------------------------------------------------------------------
    */

   switch (faultCode)
      {
      case Sender:               retval = *str_sender;               break;
      case Receiver:             retval = *str_receiver;             break;
      case MustUnderstand:       retval = *str_MustUnderstand;       break;
      case VersionMismatch:      retval = *str_VersionMismatch;      break;
      case DataEncodingUnknown:  retval = *str_DataEncodingUnknown;  break;
      }

   U_RETURN_STRING(retval);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* URPCFault::dump(bool reset) const
{
   U_CHECK_MEMORY

   *UObjectIO::os << "faultCode              " << faultCode           << '\n'
                  << "detail        (UString " << (void*)&detail      << ")\n"
                  << "faultReason   (UString " << (void*)&faultReason << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
