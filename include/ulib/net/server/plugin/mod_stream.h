// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_stream.h - distributing realtime input
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MOD_STREAM_H
#define U_MOD_STREAM_H 1

#include <ulib/utility/ring_buffer.h>
#include <ulib/net/server/server_plugin.h>

class UCommand;

class U_EXPORT UStreamPlugIn : public UServerPlugIn {
public:

   static const UString* str_URI_PATH;
   static const UString* str_METADATA;
   static const UString* str_CONTENT_TYPE;

   static void str_allocate();

   // COSTRUTTORI

   UStreamPlugIn()
      {
      U_TRACE_REGISTER_OBJECT(0, UStreamPlugIn, "")

      ptr       = 0;
      rbuf      = 0;
      command   = 0;
      fmetadata = 0;

      if (str_URI_PATH == 0) str_allocate();
      }

   virtual ~UStreamPlugIn();

   // define method VIRTUAL of class UServerPlugIn

   // Server-wide hooks

   virtual int handlerConfig(UFileConfig& cfg);
   virtual int handlerInit();
   virtual int handlerRun();

   // Connection-wide hooks

   virtual int handlerRequest();

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UFile* fmetadata;
   URingBuffer* rbuf;
   UCommand* command;
   URingBuffer::rbuf_data* ptr;
   UString uri_path, metadata, content_type;

   static pid_t pid;

   static RETSIGTYPE handlerForSigTERM(int signo);

private:
   UStreamPlugIn(const UStreamPlugIn&) : UServerPlugIn() {}
   UStreamPlugIn& operator=(const UStreamPlugIn&)        { return *this; }
};

#endif
