// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    context.h - xml Digital SIGnature with libxml2
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_DSIG_CONTEXT_H
#define ULIB_DSIG_CONTEXT_H 1

#include "transforms.h"

class UDSIGContext;
class UReferenceCtx;

// xml Digital SIGnature processing context

class UTransformCtx {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // URI transform type bit mask

   enum UriType {
   // NONE          = 0x0000, // URI type is unknown or not set
      EMPTY         = 0x0001, // empty URI ("") type
      SAME_DOCUMENT = 0x0002, // same document ("#...") but not empty ("") URI type
      TYPE_LOCAL    = 0x0004, // local URI ("file:///....") type
      TYPE_REMOTE   = 0x0008, // remote URI type
      TYPE_ANY      = 0xFFFF  // Any URI type
   };

   // COSTRUTTORI

    UTransformCtx()
      {
      U_TRACE_REGISTER_OBJECT(0, UTransformCtx, "", 0)

      uri         = 0;
      status      = 0;
      xptrExpr    = 0;
      enabledUris = TYPE_ANY;
      }

   ~UTransformCtx()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UTransformCtx)

      if (uri) 
         {
         U_SYSCALL_VOID(free, "%p", (void*)uri);
         U_SYSCALL_VOID(free, "%p", (void*)xptrExpr);
         }
      }

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   int status;           // the transforms chain processing status
   int enabledUris;      // the allowed transform data source uri types
   const char* uri;      // the data source URI without xpointer expression
   const char* xptrExpr; // the xpointer expression from data source URI (if any)

   UVector<UBaseTransform*> chain;

   static UVector<UString>* enabledTransforms;

   // SERVICES

   bool execute();
   bool setURI(const char* uri, xmlNodePtr node);
   bool nodesListRead(xmlNodePtr node, int usage);
   bool verifyNodeContent(UBaseTransform* transform, xmlNodePtr node);

   static void            registerDefault();
   static UBaseTransform* findByHref(const char* href);
   static UBaseTransform* nodeRead(xmlNodePtr node, int usage);

private:
   UTransformCtx(const UTransformCtx&)            {}
   UTransformCtx& operator=(const UTransformCtx&) { return *this; }

   friend class UDSIGContext;
   friend class UReferenceCtx;
};

class U_EXPORT UDSIGContext {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

    UDSIGContext();
   ~UDSIGContext();

   // SERVICES

   bool verify(UXML2Document& document);

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
// xmlSecKeyInfoCtx     keyInfoReadCtx;             // the reading key context
// xmlSecKeyInfoCtx     keyInfoWriteCtx;            // the writing key context (not used for signature verification).
// xmlSecPtrListPtr     enabledReferenceTransforms; // the list of transforms allowed in <dsig:Reference/> node.
// xmlSecTransformId    defSignMethodId;            // the default signing method klass.
// xmlSecTransformId    defC14NMethodId;            // the default c14n method klass.
// xmlSecTransformId    defDigestMethodId;          // the default digest method klass.
     
// xmlSecKeyPtr         signKey;             // the signature key; application may set before calling #xmlSecDSigCtxSign or #xmlSecDSigCtxVerify functions.
// xmlSecBufferPtr      result;              // the pointer to signature (not valid for signature verification).
// xmlSecTransformPtr   preSignMemBufMethod; // the pointer to binary buffer right before signature (valid only if FLAGS_STORE_SIGNATURE flag is set).

   int                     status;               // the <dsig:Signature/> processing status.
   int                     operation;            // the operation: sign or verify.
   int                     enabledReferenceUris; // the URI types allowed for <dsig:Reference/> node.
   xmlNodePtr              keyInfoNode;          // the pointer to <dsig:keyInfo/> node.
   xmlNodePtr              signValueNode;        // the pointer to <dsig:SignatureValue/> node.
   xmlNodePtr              signedInfoNode;       // the pointer to <dsig:signedInfo/> node.
   const char*             id;                   // the pointer to Id attribute of <dsig:Signature/> node.
   UTransformCtx           transformCtx;         // the <dsig:SignedInfo/> node processing context.
   UBaseTransform*         signMethod;           // the pointer to signature transform.
   UBaseTransform*         c14nMethod;           // the pointer to c14n transform.
   UVector<UReferenceCtx*> manifestReferences;   // the list of references in <dsig:Manifest/> nodes.
   UVector<UReferenceCtx*> signedInfoReferences; // the list of references in <dsig:SignedInfo/> node.      

   static UDSIGContext* pthis;

   // SERVICES

   bool processKeyInfoNode();
   bool processSignedInfoNode();
   bool processObjectNode(xmlNodePtr objectNode);
   bool processSignatureNode(xmlNodePtr signature);
   bool processManifestNode(xmlNodePtr manifestNode);

private:
   UDSIGContext(const UDSIGContext&)            {}
   UDSIGContext& operator=(const UDSIGContext&) { return *this; }

   friend class UTransformCtx;
   friend class UReferenceCtx;
};

/* The <dsig:Reference/> processing context
 *
 * Reference is an element that may occur one or more times. It specifies
 * a digest algorithm and digest value, and optionally an identifier of the 
 * object being signed, the type of the object, and/or a list of transforms 
 * to be applied prior to digesting. The identification (URI) and transforms 
 * describe how the digested content (i.e., the input to the digest method) 
 * was created. The Type attribute facilitates the processing of referenced 
 * data. For example, while this specification makes no requirements over 
 * external data, an application may wish to signal that the referent is a 
 * Manifest. An optional ID attribute permits a Reference to be referenced 
 * from elsewhere
 */

class UReferenceCtx {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   enum Origin {
      MANIFEST,   // reference in <dsig:Manifest> node
      SIGNED_INFO // reference in <dsig:SignedInfo> node
   };

   enum Status {
      UNKNOWN   = 0, // the status is unknow
      SUCCEEDED = 1, // the processing succeeded
      INVALID   = 2  // the processing failed
   };

   // COSTRUTTORI

   UReferenceCtx(int org)
      {
      U_TRACE_REGISTER_OBJECT(0, UReferenceCtx, "%d", org)

      id           = 0;
      uri          = 0;
      type         = 0;
      status       = UNKNOWN;
      origin       = org;
      digestMethod = 0;
      }

   ~UReferenceCtx()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UReferenceCtx)

      if (digestMethod) delete digestMethod; // the pointer to digest transform
      }

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   int status;                   // the reference processing status
   int origin;                   // the reference processing transforms context
   const char* id;               // the <dsig:Reference/> node ID attribute
   const char* uri;              // the <dsig:Reference/> node URI attribute
   const char* type;             // the <dsig:Reference/> node Type attribute
   UString  digest_result;       // the pointer to digest result
   UTransformCtx transformCtx;   // the reference processing transforms context
   UBaseTransform* digestMethod; // the pointer to digest transform

   // SERVICES

   bool processNode(xmlNodePtr node);

private:
   UReferenceCtx(const UReferenceCtx&)            {}
   UReferenceCtx& operator=(const UReferenceCtx&) { return *this; }

   friend class UDSIGContext;
};

#endif
