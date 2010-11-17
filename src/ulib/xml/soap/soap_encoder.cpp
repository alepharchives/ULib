// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    soap_encoder.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/xml/soap/soap_encoder.h>

const UString* USOAPEncoder::str_boolean;
const UString* USOAPEncoder::str_byte;
const UString* USOAPEncoder::str_unsignedByte;
const UString* USOAPEncoder::str_short;
const UString* USOAPEncoder::str_unsignedShort;
const UString* USOAPEncoder::str_int;
const UString* USOAPEncoder::str_unsignedInt;
const UString* USOAPEncoder::str_long;
const UString* USOAPEncoder::str_unsignedLong;
const UString* USOAPEncoder::str_float;
const UString* USOAPEncoder::str_double;
const UString* USOAPEncoder::str_string;
const UString* USOAPEncoder::str_base64Binary;
const UString* USOAPEncoder::str_encode_wrap;
const UString* USOAPEncoder::str_response;
const UString* USOAPEncoder::str_mismatch;
const UString* USOAPEncoder::str_envelope;

void USOAPEncoder::str_allocate()
{
   U_TRACE(0, "USOAPEncoder::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_boolean,0)
   U_INTERNAL_ASSERT_EQUALS(str_byte,0)
   U_INTERNAL_ASSERT_EQUALS(str_unsignedByte,0)
   U_INTERNAL_ASSERT_EQUALS(str_short,0)
   U_INTERNAL_ASSERT_EQUALS(str_unsignedShort,0)
   U_INTERNAL_ASSERT_EQUALS(str_int,0)
   U_INTERNAL_ASSERT_EQUALS(str_unsignedInt,0)
   U_INTERNAL_ASSERT_EQUALS(str_long,0)
   U_INTERNAL_ASSERT_EQUALS(str_unsignedLong,0)
   U_INTERNAL_ASSERT_EQUALS(str_float,0)
   U_INTERNAL_ASSERT_EQUALS(str_double,0)
   U_INTERNAL_ASSERT_EQUALS(str_string,0)
   U_INTERNAL_ASSERT_EQUALS(str_base64Binary,0)
   U_INTERNAL_ASSERT_EQUALS(str_response,0)
   U_INTERNAL_ASSERT_EQUALS(str_encode_wrap,0)
   U_INTERNAL_ASSERT_EQUALS(str_mismatch,0)
   U_INTERNAL_ASSERT_EQUALS(str_envelope,0)

   static ustringrep stringrep_storage[] = {
   { U_STRINGREP_FROM_CONSTANT("boolean") },
   { U_STRINGREP_FROM_CONSTANT("byte") },
   { U_STRINGREP_FROM_CONSTANT("unsignedByte") },
   { U_STRINGREP_FROM_CONSTANT("short") },
   { U_STRINGREP_FROM_CONSTANT("unsignedShort") },
   { U_STRINGREP_FROM_CONSTANT("int") },
   { U_STRINGREP_FROM_CONSTANT("unsignedInt") },
   { U_STRINGREP_FROM_CONSTANT("long") },
   { U_STRINGREP_FROM_CONSTANT("unsignedLong") },
   { U_STRINGREP_FROM_CONSTANT("float") },
   { U_STRINGREP_FROM_CONSTANT("double") },
   { U_STRINGREP_FROM_CONSTANT("string") },
   { U_STRINGREP_FROM_CONSTANT("base64Binary") },
   { U_STRINGREP_FROM_CONSTANT("Response") },
   { U_STRINGREP_FROM_CONSTANT("<%.*s xsi:type=\"xsd:%.*s\">%.*s</%.*s>") },
   { U_STRINGREP_FROM_CONSTANT(
   "<?xml version=\"1.0\" ?>"
   "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
      "<soap:Header>"
         "<soap:Upgrade>"
            "<soap:SupportedEnvelope qname=\"ns1:Envelope\" xmlns:ns1=\"http://www.w3.org/2003/05/soap-envelope\"/>"
         "</soap:Upgrade>"
      "</soap:Header>"
      "<soap:Body>"
         "<soap:Fault>"
            "<faultcode>soap:VersionMismatch</faultcode>"
            "<faultstring>Version Mismatch</faultstring>"
         "</soap:Fault>"
      "</soap:Body>"
   "</soap:Envelope>") },
   { U_STRINGREP_FROM_CONSTANT(
   "<?xml version='1.0' ?>"
   "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\">"
      "<soap:Header>%.*s</soap:Header>"
         "<soap:Body>"
            "<ns:%.*s soap:encodingStyle=\"http://www.w3.org/2003/05/soap-encoding\" xmlns:ns=\"%.*s\">%.*s</ns:%.*s>"
         "</soap:Body>"
   "</soap:Envelope>") }
};

   U_NEW_ULIB_OBJECT(str_boolean,       U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_byte,          U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_unsignedByte,  U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_short,         U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_unsignedShort, U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_int,           U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_unsignedInt,   U_STRING_FROM_STRINGREP_STORAGE(6));
   U_NEW_ULIB_OBJECT(str_long,          U_STRING_FROM_STRINGREP_STORAGE(7));
   U_NEW_ULIB_OBJECT(str_unsignedLong,  U_STRING_FROM_STRINGREP_STORAGE(8));
   U_NEW_ULIB_OBJECT(str_float,         U_STRING_FROM_STRINGREP_STORAGE(9));
   U_NEW_ULIB_OBJECT(str_double,        U_STRING_FROM_STRINGREP_STORAGE(10));
   U_NEW_ULIB_OBJECT(str_string,        U_STRING_FROM_STRINGREP_STORAGE(11));
   U_NEW_ULIB_OBJECT(str_base64Binary,  U_STRING_FROM_STRINGREP_STORAGE(12));
   U_NEW_ULIB_OBJECT(str_response,      U_STRING_FROM_STRINGREP_STORAGE(13));
   U_NEW_ULIB_OBJECT(str_encode_wrap,   U_STRING_FROM_STRINGREP_STORAGE(14));
   U_NEW_ULIB_OBJECT(str_mismatch,      U_STRING_FROM_STRINGREP_STORAGE(15));
   U_NEW_ULIB_OBJECT(str_envelope,      U_STRING_FROM_STRINGREP_STORAGE(16));
}

void USOAPEncoder::encodeArgument(const UString& argName, const UString& argType, const UString& argContent)
{
   U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%.*S,%.*S)", U_STRING_TO_TRACE(argName),
                                                              U_STRING_TO_TRACE(argType),
                                                              U_STRING_TO_TRACE(argContent))

   encodedValue.setBuffer(str_encode_wrap->size() +
                          (argName.size() * 2)    +
                          argType.size()          +
                          argContent.size() + 100U);

   encodedValue.snprintf(str_encode_wrap->data(), U_STRING_TO_TRACE(argName),
                                                  U_STRING_TO_TRACE(argType),
                                                  U_STRING_TO_TRACE(argContent),
                                                  U_STRING_TO_TRACE(argName));

   arg.push(encodedValue);
}

UString USOAPEncoder::encodeMethod(URPCMethod& method, const UString& nsName) // namespace qualified element information
{
   U_TRACE(0, "USOAPEncoder::encodeMethod(%p,%.*S)", &method, U_STRING_TO_TRACE(nsName))

   method.encode(); // Encode the method by virtual method...

   if (method.hasFailed())
      {
      U_RETURN_STRING(encodedValue);
      }
   else
      {
      uint32_t num_arg = arg.size();

      if      (num_arg  > 1) encodedValue = arg.join(0, 0);
      else if (num_arg == 1) encodedValue = arg[0];
      else                   encodedValue.setEmpty();

      UString szMethodName     = method.getMethodName(),
              szHeaderContents = method.getHeaderContent();

      if (bIsResponse) szMethodName += *str_response;

      buffer.setBuffer(nsName.size()           +
                       str_envelope->size()    +
                       encodedValue.size()     +
                       szHeaderContents.size() +
                       (szMethodName.size() * 2) + 100U);

      buffer.snprintf(str_envelope->data(), U_STRING_TO_TRACE(szHeaderContents), U_STRING_TO_TRACE(szMethodName),
                                            U_STRING_TO_TRACE(nsName),
                                            U_STRING_TO_TRACE(encodedValue),     U_STRING_TO_TRACE(szMethodName));

      U_RETURN_STRING(buffer);
      }
}
