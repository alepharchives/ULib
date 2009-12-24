// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    fcgi.h - Fast Common Gateway Interface
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_FCGI_H
#define ULIB_FCGI_H 1

#include <ulib/cgi/input.h>

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>
#endif

#include <fcgio.h>

/**
   @class UFCgi

   Class that implements input and output through a FastCGI request. This class provides access to the input
   byte-stream and environment variable interfaces of a FastCGI request. It also provides access to the request's
   output and error streams, using a similar interface.
*/

class U_EXPORT UFCgi : public UCgiInput {
public:

   /** Constructor of the class
   */
   UFCgi()
      {
      U_TRACE_REGISTER_OBJECT(0, UFCgi, "", 0)
      }

   /** Destructor of the class
   */
   virtual ~UFCgi()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UFCgi)
      }

   // SERVICES

   void init()
      {
      U_TRACE(0, "UFCGI::init()")

      FCGX_Init();
      FCGX_InitRequest(&request, 0, 0);
      }

   void finish()
      {
      U_TRACE(0, "UFCGI::finish()")

      FCGX_Finish_r(&request);
      }

   bool accept()
      {
      U_TRACE(0, "UFCGI::accept()")

      bool result = (FCGX_Accept_r(&request) == 0);

      U_RETURN(result);
      }

   /** Read data from the request's input stream
   */
   virtual uint32_t read(char* data, uint32_t length) const
      {
      U_TRACE(0, "UFCgi::read(%p,%u)", data, length)

      uint32_t result = FCGX_GetStr(data, length, request.in);

      U_RETURN(result);
      }

   /** Query the value of an environment variable stored in the request
   */
   virtual UString getenv(const char* varName) const
      {
      U_TRACE(0, "UFCgi::getenv(%S)", varName)

      // Parse environment

      int pos;
      char* name;
      char* value;

      for (char** e = request.envp; *e != NULL; ++e)
         {
         name  = *e;
         value = strchr(name, '=');
         pos   = (value - name);

         U_INTERNAL_ASSERT_POINTER(value)

         if (strncmp(varName, name, pos) == 0)
            {
            UString result(name + pos + 1);

            U_RETURN_STRING(result);
            }
         }

      U_RETURN_STRING(UString::getStringNull());
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const
      {
      *UObjectIO::os << "request " << (void*)&request;

      if (reset)
         {
         UObjectIO::output();

         return UObjectIO::buffer_output;
         }

      return 0;
      }
#endif

protected:
   FCGX_Request request;
};

#endif
