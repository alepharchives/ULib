// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    soap_client.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_SOAP_CLIENT_H
#define ULIB_SOAP_CLIENT_H 1

#include <ulib/utility/uhttp.h>
#include <ulib/net/server/server.h>
#include <ulib/net/rpc/rpc_client.h>
#include <ulib/xml/soap/soap_parser.h>
#include <ulib/xml/soap/soap_encoder.h>

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>
#endif

template <class Socket> class U_EXPORT USOAPClient : public URPCClient<Socket> {
public:

   // Costruttori

   USOAPClient(UFileConfig* cfg) : URPCClient<Socket>(cfg)
      {
      U_TRACE_REGISTER_OBJECT(0, USOAPClient, "%p", cfg)

      delete URPCMethod::encoder;
             URPCMethod::encoder = U_NEW(USOAPEncoder);
      }

   virtual ~USOAPClient()
      {
      U_TRACE_UNREGISTER_OBJECT(0, USOAPClient)
      }

   // define method VIRTUAL

   virtual bool sendRequest()
      {
      U_TRACE(0, "USOAPClient::sendRequest()")

      bool result = UClient_Base::sendRequest(); 

      U_RETURN(result);
      }

   virtual bool readResponse()
      {
      U_TRACE(0, "USOAPClient::readResponse()")

      bool result = UClient_Base::readHTTPResponse();

      U_RETURN(result);
      }

   void clearData()
      {
      U_TRACE(0, "USOAPClient::clearData()")

      parser.clearData();

      UClient_Base::clearData();
      }

   bool processRequest(URPCMethod& method)
      {
      U_TRACE(0, "USOAPClient::processRequest(%p)", &method)

      U_INTERNAL_ASSERT_POINTER(URPCMethod::encoder)

      this->UClient_Base::request = URPCMethod::encoder->encodeMethodCall(method, *URPCMethod::str_ns);

      UHTTP::setInfo(U_CONSTANT_TO_PARAM("POST"), U_CONSTANT_TO_PARAM("/soap"));

      UClient_Base::wrapRequestWithHTTP("", "application/soap+xml; charset=\"utf-8\"");

      if (this->sendRequest() &&
          this->readResponse() &&
          parser.parse(this->UClient_Base::response))
         {
         if (parser.getMethodName() == *USOAPParser::str_fault)
            {
            this->UClient_Base::response = parser.getFaultResponse();
            }
         else
            {
            this->UClient_Base::response = parser.getResponse();

            U_RETURN(true);
            }
         }

      U_RETURN(false);
      }

#ifdef DEBUG
   const char* dump(bool _reset) const
      {
      URPCClient<Socket>::dump(false);

      *UObjectIO::os << '\n'
                     << "parser         (USOAPParser         " << (void*)&parser << ')';

      if (_reset)
         {
         UObjectIO::output();

         return UObjectIO::buffer_output;
         }

      return 0;
      }
#endif

protected:
   USOAPParser parser;

private:
   USOAPClient(const USOAPClient&) : URPCClient<Socket>(0) {}
   USOAPClient& operator=(const USOAPClient&)              { return *this; }
};

#endif
