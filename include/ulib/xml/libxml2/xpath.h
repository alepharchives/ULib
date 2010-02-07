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

#include <ulib/xml/libxml2/transforms.h>

#include <libxml/xpointer.h>

class UTranformXPointer;

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
      NORMAL,                       // nodes in the list.
      INVERT,                       // all document nodes minus nodes in the list.
      TREE,                         // nodes in the list and all their subtrees.
      TREE_WITHOUT_COMMENTS,        // nodes in the list and all their subtrees but no comment nodes.
      TREE_INVERT,                  // all document nodes minus nodes in the list and all their subtrees.
      TREE_WITHOUT_COMMENTS_INVERT, // all document nodes minus (nodes in the list and all their subtress plus all comment nodes).
      LIST                          // all nodes in the chidren list of nodes sets.
   };

   // The simple nodes sets operations

   enum NodeSetOp {
      INTERSECTION,
      SUBTRACTION,
      UNION
   };

   // COSTRUTTORI

    UXPathData(int data_type, int nodeSetType, const char* expr);
   ~UXPathData();

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   int type;
   int nodeSetOp;
   int nodeSetType;    
   const char* expr;
   xmlXPathContextPtr ctx;

   bool registerNamespaces(xmlNodePtr node); /* register namespaces */

private:
   UXPathData(const UXPathData&)            {}
   UXPathData& operator=(const UXPathData&) { return *this; }

   friend class UTranformXPointer;
};

#endif
