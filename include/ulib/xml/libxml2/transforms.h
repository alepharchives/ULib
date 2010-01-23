// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    transforms.h - wrapping of libxml2
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_UXML2TRANSFORM_H
#define ULIB_UXML2TRANSFORM_H 1

#include <ulib/xml/libxml2/document.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>

class U_EXPORT UBaseTranform {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // The transform usage bit mask

   enum Usage {
      NONE       = 0x0000, // usage is unknown or undefined
      DSIG       = 0x0001, // Transform could be used in <dsig:Transform>
      C14N       = 0x0002, // Transform could be used in <dsig:CanonicalizationMethod>
      DIGEST     = 0x0004, // Transform could be used in <dsig:DigestMethod>
      SIGNATURE  = 0x0008, // Transform could be used in <dsig:SignatureMethod>
      ENCRYPTION = 0x0010, // Transform could be used in <enc:EncryptionMethod>
      ANY        = 0xFFFF  // Transform could be used for operation
   };

   Usage usage;      // the allowed transforms usages
   const char* name; // the transform's name
   const char* href; // the transform's identification string (href)

            UBaseTranform() {}
   virtual ~UBaseTranform() {}

   // method VIRTUAL to define

   virtual int readNode()     { return 0; } // the XML node read method
   virtual int writeNode()    { return 0; } // the XML node write method
   virtual int setKey()       { return 0; } // the set key method
   virtual int setKeyReq()    { return 0; } // the set key requirements method
   virtual int verify()       { return 0; } // the verify method (for digest and signature transforms)
   virtual int getDataType()  { return 0; } // the input/output data type query method
   virtual int pushBin()      { return 0; } // the binary data "push thru chain" processing method
   virtual int popBin()       { return 0; } // the binary data "pop from chain" procesing method
   virtual int pushXml()      { return 0; } // the XML data "push thru chain" processing method
   virtual int popXml()       { return 0; } // the XML data "pop from chain" procesing method
   virtual int execute()      { return 0; } // the low level data processing method used by default implementations of pushBin,popBin,pushXml and popXml

private:
   UBaseTranform(const UBaseTranform&)            {}
   UBaseTranform& operator=(const UBaseTranform&) { return *this; }
};

 /*
  * The Base64 transform klass (http://www.w3.org/TR/xmldsig-core/#sec-Base-64).
  * The normative specification for base64 decoding transforms is RFC 2045
  * (http://www.ietf.org/rfc/rfc2045.txt). The base64 Transform element has 
  * no content. The input is decoded by the algorithms. This transform is 
  * useful if an application needs to sign the raw data associated with 
  * the encoded content of an element.
  */

class U_EXPORT UTranformBase64 {
public:

   UTranformBase64()
      {
      U_TRACE_REGISTER_OBJECT(0, UTranformBase64, "", 0)

      usage = DSIG;
      name  = "base64";
      href  = "http://www.w3.org/2000/09/xmldsig#base64";
      }

   ~UTranformBase64()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UTranformBase64)
      }

private:
   UTranformBase64(const UTranformBase64&)            {}
   UTranformBase64& operator=(const UTranformBase64&) { return *this; }
};

class U_EXPORT UTranformInclC14N {
public:

   UTranformInclC14N()
      {
      U_TRACE_REGISTER_OBJECT(0, UTranformInclC14N, "", 0)

      usage = DSIG;
      name  = "base64";
      href  = "http://www.w3.org/2000/09/xmldsig#base64";
      }

   ~UTranformInclC14N()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UTranformInclC14N)
      }

private:
   UTranformInclC14N(const UTranformInclC14N&)            {}
   UTranformInclC14N& operator=(const UTranformInclC14N&) { return *this; }
};


class U_EXPORT UXML2Transform {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // The transform execution status

   enum State {
      NONE     = 0,  // status unknown
      WORKING  = 1,  // transform is executed
      FINISHED = 2,  // transform finished
      OK       = 3,  // transform succeeded
      FAIL     = 4   // transform failed (an error occur)
   };

   // transform data type bit mask

   enum DataType {
      NONE   = 0x0000, // transform data type is unknown or nor data expected
      BINARY = 0x0001, // binary transform data
      XML    = 0x0002  // xml transform data
   };

   // URI transform type bit mask

   enum UriType {
      NONE          = 0x0000, // URI type is unknown or not set
      EMPTY         = 0x0001, // empty URI ("") type
      SAME_DOCUMENT = 0x0002, // same document ("#...") but not empty ("") URI type
      TYPE_LOCAL    = 0x0004, // local URI ("file:///....") type
      TYPE_REMOTE   = 0x0008, // remote URI type
      TYPE_ANY      = 0xFFFF  // Any URI type
   };

   // The transform operation

   enum Operation {
      NONE     = 0,  // operation is unknown
      ENCODE   = 1,  // encode operation (for base64 transform)
      DECODE   = 2,  // decode operation (for base64 transform)
      SIGN     = 3,  // sign or digest operation
      VERIFY   = 4   // verification of signature or digest operation
      ENCRYPT  = 5,  // encryption operation
      DECRYPT  = 6,  // decryption operation
   };

   // The transform operation mode

   enum Mode {
      NONE = 0, // mode unknown
      PUSH = 1, // pushing data thru transform
      POP  = 2  // popping data from transform
   };

   // The transform structure

   typedef struct transform {
      Status status;       // the current status
      Operation operation; // the transform's operation
      UBaseTranform* id;   // the transform id (pointer to)
      UString inBuf;       // the input binary data buffer
      UString outBuf;      // the output binary data buffer
      xmlNodePtr hereNode; // the pointer to transform's <dsig:Transform /> node
      xmlNodePtr inNodes;  // the input XML nodes
      xmlNodePtr outNodes; // the output XML nodes
   } transform;

   // COSTRUTTORI

    UXML2Transform(const UString& xmldoc);
   ~UXML2Transform();

   const char* getName() const
      {
      U_TRACE(0, "UXML2Transform::getName()")

      U_INTERNAL_ASSERT_POINTER(impl_)

      const char* result = (impl_->name ? (const char*)impl_->name : "");

      U_RETURN(result);
      }

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:

    int status,         // the transforms chain processing status
        enabledUris;    // the allowed transform data source uri types

    xmlChar* uri;       // the data source URI without xpointer expression
    xmlChar* xptrExpr;  // the xpointer expression from data source URI (if any)

   // xmlSecPtrList  enabledTransforms;
    
private:
   UXML2Transform(const UXML2Transform&)            {}
   UXML2Transform& operator=(const UXML2Transform&) { return *this; }
};

#endif
