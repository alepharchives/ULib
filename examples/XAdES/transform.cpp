// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    transform.cpp - xml Digital SIGnature with libxml2
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include "xpath.h"

// the allowed transforms usages
// the transform's name
// the transform's identification string (href)

int         UTranformBase64::_usage    = DSIG;
const char* UTranformBase64::_name     = "base64";
const char* UTranformBase64::_href     = "http://www.w3.org/2000/09/xmldsig#base64";

int         UTranformInputURI::_usage  = DSIG;
const char* UTranformInputURI::_name   = "input-uri";
const char* UTranformInputURI::_href   = "";

UVector<UIOCallback*>* UTranformInputURI::allIOCallbacks;

int         UTranformInclC14N::_usage  = DSIG|C14N;
const char* UTranformInclC14N::_name   = "c14n";
const char* UTranformInclC14N::_href   = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315";

int         UTranformXPointer::_usage  = DSIG;
const char* UTranformXPointer::_name   = "xpointer";
const char* UTranformXPointer::_href   = "http://www.w3.org/2001/04/xmldsig-more/xptr";

int         UTranformSha1::_usage      = DIGEST;
const char* UTranformSha1::_name       = "sha1";
const char* UTranformSha1::_href       = "http://www.w3.org/2000/09/xmldsig#sha1";

int         UTranformRsaMd5::_usage    = SIGNATURE;
const char* UTranformRsaMd5::_name     = "rsa-md5";
const char* UTranformRsaMd5::_href     = "http://www.w3.org/2001/04/xmldsig-more#rsa-md5";

int         UTranformRsaSha1::_usage   = SIGNATURE;
const char* UTranformRsaSha1::_name    = "rsa-sha1";
const char* UTranformRsaSha1::_href    = "http://www.w3.org/2000/09/xmldsig#rsa-sha1";

UTranformXPointer::~UTranformXPointer()
{
   U_TRACE_UNREGISTER_OBJECT(0, UTranformXPointer)

   dataList.clear();
}

/**
 * @data    the input binary data
 * @final   the flag: if set to true then it's the last data chunk.
 *
 * Process binary @data by calling transform's execute method and pushes 
 * results to next transform.
 *
 * Returns: true on success or a false value if an error occurs.
 */

bool UBaseTransform::pushBin(const UString& data, bool final)
{
   U_TRACE(0, "UBaseTransform::pushBin(%.*S,%b)", U_STRING_TO_TRACE(data), final)

   /*
   xmlSecSize inSize = 0;
   xmlSecSize outSize = 0;
   int finalData = 0;
   int ret;

   do {
   // append data to input buffer
   if(dataSize > 0) {
   xmlSecSize chunkSize;

   xmlSecAssert2(data != NULL, -1);

   chunkSize = dataSize;
   if(chunkSize > XMLSEC_TRANSFORM_BINARY_CHUNK) {
   chunkSize = XMLSEC_TRANSFORM_BINARY_CHUNK;
   }

   ret = xmlSecBufferAppend(&(transform->inBuf), data, chunkSize);

   dataSize -= chunkSize;
   data += chunkSize;
   }

   // process data

   inSize = xmlSecBufferGetSize(&(transform->inBuf));

   outSize = xmlSecBufferGetSize(&(transform->outBuf));

   finalData = (((dataSize == 0) && (final != 0)) ? 1 : 0);

   ret = xmlSecTransformExecute(transform, finalData, transformCtx);

   // push data to the next transform

   inSize = xmlSecBufferGetSize(&(transform->inBuf));

   outSize = xmlSecBufferGetSize(&(transform->outBuf));

   if(inSize > 0) {
   finalData = 0;
   }

   // we don't want to puch too much

   if(outSize > XMLSEC_TRANSFORM_BINARY_CHUNK) {
   outSize = XMLSEC_TRANSFORM_BINARY_CHUNK;
   finalData = 0;
   }

   if((transform->next != NULL) && ((outSize > 0) || (finalData != 0))) {
   ret = xmlSecTransformPushBin(transform->next, 
   xmlSecBufferGetData(&(transform->outBuf)),
   outSize,
   finalData,
   transformCtx);
   }

   // remove data anyway

   if(outSize > 0) {
   ret = xmlSecBufferRemoveHead(&(transform->outBuf), outSize);
   }
   } while((dataSize > 0) || (outSize > 0));
   */    

   U_RETURN(true);
}

/**
 * @data    the buffer to store result data.
 * 
 * Pops data from previous transform in the chain, processes data by calling
 * transform's execute method and returns result in the @data buffer.
 * 
 * Returns: true on success or a false value if an error occurs.
 */

bool UBaseTransform::popBin(UString& data)
{
   U_TRACE(0, "UBaseTransform::popBin(%p,%b)", &data)

   /*
   xmlSecSize outSize;
   int final = 0;
   int ret;

   while((xmlSecBufferGetSize(&(transform->outBuf)) == 0) && (final == 0)) {
   // read data from previous transform if exist
   if(transform->prev != NULL) {    
   xmlSecSize inSize, chunkSize;

   inSize = xmlSecBufferGetSize(&(transform->inBuf));
   chunkSize = XMLSEC_TRANSFORM_BINARY_CHUNK;

   // ensure that we have space for at least one data chunk
   ret = xmlSecBufferSetMaxSize(&(transform->inBuf), inSize + chunkSize);

   // get data from previous transform
   ret = xmlSecTransformPopBin(transform->prev, 
   xmlSecBufferGetData(&(transform->inBuf)) + inSize,
   chunkSize, &chunkSize, transformCtx);

   // adjust our size if needed
   if(chunkSize > 0) {
   ret = xmlSecBufferSetSize(&(transform->inBuf), inSize + chunkSize);
   final = 0; // the previous transform returned some data..
   } else {
   final = 1; // no data returned from previous transform, we are done
   }
   } else {
   final = 1; // no previous transform, we are "permanently final"
   }  

   // execute our transform
   ret = xmlSecTransformExecute(transform, final, transformCtx);
   }

   // copy result (if any)
   outSize = xmlSecBufferGetSize(&(transform->outBuf)); 
   if(outSize > maxDataSize) {
   outSize = maxDataSize;
   }

   // we don't want to put too much
   if(outSize > XMLSEC_TRANSFORM_BINARY_CHUNK) {
   outSize = XMLSEC_TRANSFORM_BINARY_CHUNK;
   }
   if(outSize > 0) {
   xmlSecAssert2(xmlSecBufferGetData(&(transform->outBuf)), -1);

   memcpy(data, xmlSecBufferGetData(&(transform->outBuf)), outSize);

   ret = xmlSecBufferRemoveHead(&(transform->outBuf), outSize);
   }

   // set the result size
   (*dataSize) = outSize;
   return(0);
   */

   U_RETURN(true);
}

/**
 * @nodes   the input nodes.
 *
 * Processes @nodes by calling transform's execute method and pushes 
 * result to the next transform in the chain.
 *
 * Returns: true on success or a false value if an error occurs.
 */

bool UBaseTransform::pushXml(xmlNodePtr nodes)
{
   U_TRACE(0, "UBaseTransform::pushXml(%p)", nodes)

   /*
   int ret;

   xmlSecAssert2(xmlSecTransformIsValid(transform), -1);
   xmlSecAssert2(transform->inNodes == NULL, -1);
   xmlSecAssert2(transform->outNodes == NULL, -1);
   xmlSecAssert2(transformCtx != NULL, -1);

   // execute our transform
   transform->inNodes = nodes;
   ret = xmlSecTransformExecute(transform, 1, transformCtx);

   // push result to the next transform (if exist)
   if(transform->next != NULL) {
   ret = xmlSecTransformPushXml(transform->next, transform->outNodes, transformCtx);
   }        
   return(0);
   */

   U_RETURN(true);
}

/**
 * @nodes   the pointer to store pointer to result nodes.
 *
 * Pops data from previous transform in the chain, processes the data 
 * by calling transform's execute method and returns result in @nodes.
 * 
 * Returns: true on success or a false value if an error occurs.
 */

bool UBaseTransform::popXml(xmlNodePtr* nodes)
{
   U_TRACE(0, "UBaseTransform::popXml(%p)", nodes)

   /*
   int ret;

   xmlSecAssert2(xmlSecTransformIsValid(transform), -1);
   xmlSecAssert2(transform->inNodes == NULL, -1);
   xmlSecAssert2(transform->outNodes == NULL, -1);
   xmlSecAssert2(transformCtx != NULL, -1);

   // pop result from the prev transform (if exist)
   if(transform->prev != NULL) {
   ret = xmlSecTransformPopXml(transform->prev, &(transform->inNodes), transformCtx);
   }        

   // execute our transform
   ret = xmlSecTransformExecute(transform, 1, transformCtx);

   // return result if requested
   if(nodes != NULL) {
   (*nodes) = transform->outNodes;
   }

   return(0);
   */

   U_RETURN(true);
}

bool UTranformXPointer::setExpr(const char* expr, int nodeSetType, xmlNodePtr node)
{
   U_TRACE(0, "UTranformXPointer::setExpr(%S,%d,%p)", expr, nodeSetType, node)

   UBaseTransform::hereNode = node;

   UXPathData* data = U_NEW(UXPathData(UXPathData::XPOINTER, nodeSetType, expr));

   if (data->registerNamespaces(node))
      {
      dataList.push(data);

      U_RETURN(true);
      }

   delete data;

   U_RETURN(false);
}

// Opens the given @uri for reading.

UTranformInputURI::UTranformInputURI(const char* uri)
{
   U_TRACE_REGISTER_OBJECT(0, UTranformInputURI, "%S", uri)

   /*
    * Try to find one of the input accept method accepting that scheme
    * Go in reverse to give precedence to user defined handlers.
    * try with an unescaped version of the uri
    */

   char* unescaped = U_SYSCALL(xmlURIUnescapeString, "%S,%d,%S", uri, 0, NULL);

   if (unescaped != NULL)
      {
      clbks = find(unescaped);

      if (clbks) clbksCtx = clbks->opencallback(unescaped);

      U_SYSCALL_VOID(xmlFree, "%p", unescaped);
      }

   // If this failed try with a non-escaped uri this may be a strange filename

   if (clbks == 0)
      {
      clbks = find(uri);

      if (clbks) clbksCtx = clbks->opencallback(uri);
      }
}

UIOCallback* UTranformInputURI::find(const char* uri)
{
   U_TRACE(0, "UTranformInputURI::find(%S)", uri)

   U_INTERNAL_ASSERT_POINTER(allIOCallbacks)

   UIOCallback* callbacks;

   for (uint32_t i = 0, n = allIOCallbacks->size(); i < n; ++i)
      {
      callbacks = (*allIOCallbacks)[i];

      if (callbacks->matchcallback(uri)) U_RETURN_POINTER(callbacks, UIOCallback);
      }

   U_RETURN_POINTER(0, UIOCallback);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UBaseTransform::dump(bool reset) const
{
   *UObjectIO::os << "status    " << status << '\n'
                  << "operation " << operation;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

const char* UIOCallback::dump(bool reset) const
{
   *UObjectIO::os << "opencallback  " << (void*)opencallback  << '\n'
                  << "readcallback  " << (void*)readcallback  << '\n'
                  << "closecallback " << (void*)closecallback << '\n'
                  << "matchcallback " << (void*)matchcallback;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
