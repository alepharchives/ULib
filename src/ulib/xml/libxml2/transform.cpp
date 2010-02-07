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

#include <ulib/xml/libxml2/xpath.h>

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
