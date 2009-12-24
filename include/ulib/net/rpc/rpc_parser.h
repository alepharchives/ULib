// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    rpc_parser.h - RPC parser
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_RPC_PARSER_H
#define ULIB_RPC_PARSER_H 1

#include <ulib/net/rpc/rpc.h>
#include <ulib/net/rpc/rpc_envelope.h>

/**
   @class URPCParser

   @brief URPCParser is a parser RPC
*/

class URPCObject;

class U_EXPORT URPCParser {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   URPCParser(UVector<UString>* arg = 0)
      {
      U_TRACE_REGISTER_OBJECT(0, URPCParser, "%p", arg)

      if (arg == 0)
         {
         URPC::allocate();

         arg = URPC::rpc_info;
         }

      envelope.arg = arg;
      }

   ~URPCParser()
      {
      U_TRACE_UNREGISTER_OBJECT(0, URPCParser)

      clearData();

      U_INTERNAL_ASSERT_POINTER(envelope.arg)

      delete envelope.arg;
      }

   // SERVICES

   void clearData();

   UVector<UString>& getArgument()  { return *(envelope.arg); }

   UString getNsName() const        { return envelope.nsName; }      // return the name of namespace qualified
                                                                     // element information
   UString getMethodName() const    { return envelope.methodName; }  // return the name of the method
                                                                     // the caller wants to execute

   // ---------------------------------------------------------------------
   // Given a URPCObject this calls the appropriate URPCMethod.
   // If this fails, ask for a URPCFault. Returns a RPC send-ready response
   // ---------------------------------------------------------------------
   // bContainsFault: Indicates if the returned string contains a fault
   // ---------------------------------------------------------------------

   UString processMessage(const UString& msg, URPCObject& object, bool& bContainsFault);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   URPCEnvelope envelope;

private:
   URPCParser(const URPCParser&)            {}
   URPCParser& operator=(const URPCParser&) { return *this; }
};

#endif
