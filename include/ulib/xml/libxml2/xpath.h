// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    xpath.h - xml Digital SIGnature with libxml2
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_DSIG_XPATH_H
#define ULIB_DSIG_XPATH_H 1

#include <ulib/xml/libxml2/transform.h>

#include <libxml/xpath.h>

class U_EXPORT UXPathData {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   enum DataType {
      XPATH,
      XPATH2,
      XPOINTER
   };

   // The basic nodes sets types

   enum NodeSetType {
      NORMAL,                       // nodes set = nodes in the list.
      INVERT,                       // nodes set = all document nodes minus nodes in the list.
      TREE,                         // nodes set = nodes in the list and all their subtress.
      TREE_WITHOUT_COMMENTS,        // nodes set = nodes in the list and all their subtress but no comment nodes.
      TREE_INVERT,                  // nodes set = all document nodes minus nodes in the list and all their subtress.
      TREE_WITHOUT_COMMENTS_INVERT, // nodes set = all document nodes minus (nodes in the list and all their subtress plus all comment nodes).
      LIST                          // nodes set = all nodes in the chidren list of nodes sets.
   };

   // The simple nodes sets operations

   enum NodeSetOp {
      INTERSECTION,
      SUBTRACTION,
      UNION
   };

   UXPathData()
      {
      U_TRACE_REGISTER_OBJECT(0, UXPathData, "", 0)
      }

   ~UXPathData()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UXPathData)
      }

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   int type;
   int nodeSetOp;
   int nodeSetType;    
   const char* expr;
// xmlXPathContextPtr ctx;

private:
   UXPathData(const UXPathData&)            {}
   UXPathData& operator=(const UXPathData&) { return *this; }
};

#endif
