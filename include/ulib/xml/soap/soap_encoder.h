// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    soap_encoder.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_SOAP_ENCODER_H
#define ULIB_SOAP_ENCODER_H 1

#include <ulib/utility/base64.h>
#include <ulib/net/rpc/rpc_encoder.h>

/**
   @class USOAPEncoder
*/

// Here's something that templates and C++ still can't handle. Using this in conjunction with
// the encodeArgument() methods below, we avoid type WRT the argument names. Spell it wrong and
// you get a compile time error. Better at compile-time then run-time

#define U_SOAP_ENCODE_ARG(arg)            ((USOAPEncoder*)URPCMethod::encoder)->encodeArgument(U_STRING_FROM_CONSTANT("" #arg ""), arg)
#define U_SOAP_ENCB64_ARG(arg)            ((USOAPEncoder*)URPCMethod::encoder)->encodeArgBase64(U_STRING_FROM_CONSTANT("" #arg ""),arg)
#define U_SOAP_ENCODE_RES(arg)            ((USOAPEncoder*)URPCMethod::encoder)->encodeResponse(arg)
#define U_SOAP_ENCODE_NAME_ARG(name, arg) ((USOAPEncoder*)URPCMethod::encoder)->encodeArgument(name,                               arg)
#define U_SOAP_ENCB64_NAME_ARG(name, arg) ((USOAPEncoder*)URPCMethod::encoder)->encodeArgBase64(name,                              arg)

class U_EXPORT USOAPEncoder : public URPCEncoder {
public:

   static const UString* str_boolean;
   static const UString* str_byte;
   static const UString* str_unsignedByte;
   static const UString* str_short;
   static const UString* str_unsignedShort;
   static const UString* str_int;
   static const UString* str_unsignedInt;
   static const UString* str_long;
   static const UString* str_unsignedLong;
   static const UString* str_float;
   static const UString* str_double;
   static const UString* str_string;
   static const UString* str_base64Binary;
   static const UString* str_response;
   static const UString* str_encode_wrap;
   static const UString* str_mismatch;
   static const UString* str_envelope;

   static void str_allocate();

   // COSTRUTTORI

   USOAPEncoder()
      {
      U_TRACE_REGISTER_OBJECT(0, USOAPEncoder, "")

      if (str_boolean == 0) str_allocate();
      }

   virtual ~USOAPEncoder()
      {
      U_TRACE_UNREGISTER_OBJECT(0, USOAPEncoder)
      }

   // ENCODING FUNCTIONS

   virtual void encodeArgument(const UString& argName, const UString& argType, const UString& argContent);

   void encodeArgument(const UString& argName, const UString& argType, bool value)
      { URPCEncoder::encodeArgument(argName, argType, value); }

   void encodeArgument(const UString& argName, const UString& argType, char value)
      { URPCEncoder::encodeArgument(argName, argType, value); }

   void encodeArgument(const UString& argName, const UString& argType, unsigned char value)
      { URPCEncoder::encodeArgument(argName, argType, value); }

   void encodeArgument(const UString& argName, const UString& argType, short value)
      { URPCEncoder::encodeArgument(argName, argType, value); }

   void encodeArgument(const UString& argName, const UString& argType, unsigned short value)
      { URPCEncoder::encodeArgument(argName, argType, value); }

   void encodeArgument(const UString& argName, const UString& argType, int value)
      { URPCEncoder::encodeArgument(argName, argType, value); }

   void encodeArgument(const UString& argName, const UString& argType, unsigned int value)
      { URPCEncoder::encodeArgument(argName, argType, value); }

   void encodeArgument(const UString& argName, const UString& argType, long int value)
      { URPCEncoder::encodeArgument(argName, argType, value); }

   void encodeArgument(const UString& argName, const UString& argType, unsigned long int value)
      { URPCEncoder::encodeArgument(argName, argType, value); }

   void encodeArgument(const UString& argName, const UString& argType, const long long& value)
      { URPCEncoder::encodeArgument(argName, argType, value); }

   void encodeArgument(const UString& argName, const UString& argType, const unsigned long long& value)
      { URPCEncoder::encodeArgument(argName, argType, value); }

   void encodeArgument(const UString& argName, const UString& argType, float value)
      { URPCEncoder::encodeArgument(argName, argType, value); }

   void encodeArgument(const UString& argName, const UString& argType, const double& value)
      { URPCEncoder::encodeArgument(argName, argType, value); }

   // This set of overloads encodes the argument using the xsd:[type] data as <argName>value</argName>.
   // To avoid spelling errors, I recommend using the encodeArg macro, which can produce the argument name and value

   void encodeArgument(const UString& argName, bool value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%b)", U_STRING_TO_TRACE(argName), value)

      encodeArgument(argName, *str_boolean, value);
      }

   void encodeArgument(const UString& argName, char value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%C)", U_STRING_TO_TRACE(argName), value)

      encodeArgument(argName, *str_byte, value);
      }

   void encodeArgument(const UString& argName, unsigned char value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%C)", U_STRING_TO_TRACE(argName), value)

      encodeArgument(argName, *str_unsignedByte, value);
      }

   void encodeArgument(const UString& argName, short value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%d)", U_STRING_TO_TRACE(argName), value)

      encodeArgument(argName, *str_short, value);
      }

   void encodeArgument(const UString& argName, unsigned short value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%d)", U_STRING_TO_TRACE(argName), value)

      encodeArgument(argName, *str_unsignedShort, value);
      }

   void encodeArgument(const UString& argName, int value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%d)", U_STRING_TO_TRACE(argName), value)

      encodeArgument(argName, *str_int, value);
      }

   void encodeArgument(const UString& argName, unsigned int value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%u)", U_STRING_TO_TRACE(argName), value)

      encodeArgument(argName, *str_unsignedInt, value);
      }

   void encodeArgument(const UString& argName, long int value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%ld)", U_STRING_TO_TRACE(argName), value)

      encodeArgument(argName, *str_int, value);
      }

   void encodeArgument(const UString& argName, unsigned long int value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%lu)", U_STRING_TO_TRACE(argName), value)

      encodeArgument(argName, *str_unsignedInt, value);
      }

   void encodeArgument(const UString& argName, const long long& value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%lld)", U_STRING_TO_TRACE(argName), value)

      encodeArgument(argName, *str_long, value);
      }

   void encodeArgument(const UString& argName, const unsigned long long& value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%llu)", U_STRING_TO_TRACE(argName), value)

      encodeArgument(argName, *str_unsignedLong, value);
      }

   void encodeArgument(const UString& argName, float value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%f)", U_STRING_TO_TRACE(argName), value)

      encodeArgument(argName, *str_float, value);
      }

   void encodeArgument(const UString& argName, const double& value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%g)", U_STRING_TO_TRACE(argName), value)

      encodeArgument(argName, *str_double, value);
      }

   void encodeArgument(const UString& argName, const UString& value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgument(%.*S,%.*S)", U_STRING_TO_TRACE(argName), U_STRING_TO_TRACE(value))

      encodeArgument(argName, *str_string, value);
      }

   // It takes a pointer and the length of the data to encode from that pointer in bytes, then base64 encodes the data

   void encodeArgBase64(const UString& argName, const UString& value)
      {
      U_TRACE(0, "USOAPEncoder::encodeArgBase64(%.*S,%.*S)", U_STRING_TO_TRACE(argName), U_STRING_TO_TRACE(value))

      if (value.empty() == false)
         {
         buffer.setBuffer(value.size() * 2);

         UBase64::encode(value, buffer);
         }

      encodeArgument(argName, *str_base64Binary, buffer);
      }

   void encodeResponse(const UString& value)
      {
      U_TRACE(0, "USOAPEncoder::encodeResponse(%.*S)", U_STRING_TO_TRACE(value))

      encodedValue.replace(value);
      }

   // Encodes the complete fault into a complete URPCEnvelope

   virtual UString encodeFault(URPCFault* fault)
      {
      U_TRACE(0, "USOAPEncoder::encodeFault(%p)", fault)

      fault->encode(buffer); // Encode the fault

      U_SOAP_ENCODE_RES(buffer);

      U_RETURN_STRING(encodedValue);
      }

   static UString encodeVersionMismatch() { return *str_mismatch; }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return URPCEncoder::dump(reset); }
#endif

protected:

   // Called by the encodeMethodResponse and encodeMethodCall members to finish encoding the method.
   // As you may guess, the two methods share a lot, and this item holds that common code

   virtual UString encodeMethod(URPCMethod& method, const UString& nsName); // namespace qualified element information

private:
   USOAPEncoder(const USOAPEncoder&) : URPCEncoder() {}
   USOAPEncoder& operator=(const USOAPEncoder&)      { return *this; }
};

#endif
