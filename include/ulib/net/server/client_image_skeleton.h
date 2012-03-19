// ===================================================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    client_image_skeleton.h - this is a skeleton for handle accepted connections from UServer client
//
// = AUTHOR
//    Stefano Casazza
//
// ===================================================================================================

#ifndef U_CLIENT_IMAGE_SKELETON_H
#define U_CLIENT_IMAGE_SKELETON_H 1

#include <ulib/net/server/client_image.h>

/**
   @class USkeletonClientImage

   @brief Handles accepted connections from UServer client
*/

template <class Socket> class U_EXPORT USkeletonClientImage : public UClientImage<Socket> {
public:

   USkeletonClientImage(USocket* p) : UClientImage<Socket>(p)
      {
      U_TRACE_REGISTER_OBJECT(0, USkeletonClientImage, "%p", p)
      }

   virtual ~USkeletonClientImage()
      {
      U_TRACE_UNREGISTER_OBJECT(0, USkeletonClientImage)
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return UClientImage_Base::dump(reset); }
#endif

protected:

   // define method VIRTUAL of class UEventFd

   virtual int handlerRead()
      {
      U_TRACE(0, "USkeletonClientImage::handlerRead()")

      int result = UClientImage_Base::handlerRead(); // read request...

      U_RETURN(result);
      }

private:
   USkeletonClientImage(const USkeletonClientImage&) : UClientImage<Socket>(0) {}
   USkeletonClientImage& operator=(const USkeletonClientImage&)                { return *this; }
};

#ifdef USE_LIBSSL // specializzazione con USSLSocket

template <> class U_EXPORT USkeletonClientImage<USSLSocket> : public UClientImage<USSLSocket> {
public:

   USkeletonClientImage(USocket* p) : UClientImage<USSLSocket>(p)
      {
      U_TRACE_REGISTER_OBJECT(0, USkeletonClientImage<USSLSocket>, "%p", p)
      }

   virtual ~USkeletonClientImage()
      {
      U_TRACE_UNREGISTER_OBJECT(0, USkeletonClientImage<USSLSocket>)
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const { return UClientImage_Base::dump(reset); }
#endif

protected:

   // define method VIRTUAL of class UEventFd

   virtual int handlerRead()
      {
      U_TRACE(0, "USkeletonClientImage::handlerRead()")

      int result = UClientImage_Base::handlerRead(); // read request...

      U_RETURN(result);
      }

private:
   USkeletonClientImage<USSLSocket>(const USkeletonClientImage<USSLSocket>&) : UClientImage<USSLSocket>(0) {}
   USkeletonClientImage<USSLSocket>& operator=(const USkeletonClientImage<USSLSocket>&)                    { return *this; }
};

#endif

#endif
