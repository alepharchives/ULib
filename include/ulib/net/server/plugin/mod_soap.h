// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_soap.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_SOAP_H
#define U_MOD_SOAP_H 1

#include <ulib/net/server/server_plugin.h>

class USOAPParser;

class U_EXPORT USoapPlugIn : public UServerPlugIn {
public:

   // COSTRUTTORI

            USoapPlugIn();
   virtual ~USoapPlugIn();

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerInit();

   // Connection-wide hooks

   virtual int handlerRequest();

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   static USOAPParser* soap_parser;

private:
   USoapPlugIn(const USoapPlugIn&) : UServerPlugIn() {}
   USoapPlugIn& operator=(const USoapPlugIn&)        { return *this; }
};

#endif
