// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    input.h - input for Common Gateway Interface
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_CGI_INPUT_H
#define ULIB_CGI_INPUT_H 1

#include <ulib/string.h>

/**
   @class UCgiInput

   This class allows the data source for the CGI application to be something other than standard input.
   This is useful, in fact necessary, when using FastCgi. Library users wishing to exploit this functionality
   should create a subclass and override the read and getenv methods.
*/

class U_EXPORT UCgiInput {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   /** Constructor of the class
   */
   UCgiInput()
      {
      U_TRACE_REGISTER_OBJECT(0, UCgiInput, "", 0)
      }

   /** Destructor of the class
   */
   virtual ~UCgiInput()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UCgiInput)
      }

   /** Read data from a data source
   */
   virtual uint32_t read(char* data, uint32_t length) const { return _read(data, length); }

   /** Query the value of an environment variable
   */
   virtual UString getenv(const char* varName)const { return _getenv(varName); }

   // SERVICES

   static UString  _getenv(const char* varName);
   static uint32_t _read(char* data, uint32_t length);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
};

#endif
