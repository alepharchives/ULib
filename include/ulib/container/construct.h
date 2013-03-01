// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    construct.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_CONSTRUCT_H
#define ULIB_CONSTRUCT_H 1

#include <ulib/string.h>

// default behaviour

template <class T>
inline void u_construct(T** ptr)
{
   U_TRACE(0, "u_construct<T>(%p)", ptr)
}

template <class T>
inline void u_construct(T* ptr, uint32_t n)
{
   U_TRACE(0, "u_construct<T>(%p,%u)", ptr, n)
}

template <class T>
inline void u_destroy(T* ptr)
{
   U_TRACE(0, "u_destroy<T>(%p)", ptr)

   U_INTERNAL_ASSERT_POINTER(ptr)

   delete ptr;
}

template <class T>
inline void u_destroy(T** ptr, uint32_t n)
{
   U_TRACE(0, "u_destroy<T>(%p,%u)", ptr, n)

   for (uint32_t i = 0; i < n; ++i) delete ptr[i];
}

template <>
inline void u_construct(UStringRep** prep)
{
   U_TRACE(0, "u_construct<UStringRep*>(%p)", prep)

   (*prep)->hold();
}

template <>
inline void u_construct(UStringRep* rep, uint32_t n)
{
   U_TRACE(0, "u_construct<UStringRep*>(%p,%u)", rep, n)

   rep->references += n;

   U_INTERNAL_DUMP("references = %d", rep->references + 1)
}

template <>
inline void u_destroy(UStringRep* rep)
{
   U_TRACE(0, "u_destroy<UStringRep*>(%p)", rep)

   rep->release();
}

template <>
inline void u_destroy(UStringRep** rep, uint32_t n)
{
   U_TRACE(0, "u_destroy<UStringRep*>(%p,%u)", rep, n)

   for (uint32_t i = 0; i < n; ++i) rep[i]->release();
}

#endif
