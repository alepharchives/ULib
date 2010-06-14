// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//   value.cpp - Represents a JSON (JavaScript Object Notation) value
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/json/value.h>
#include <ulib/container/vector.h>
#include <ulib/container/hash_map.h>

#include <limits.h>

UValue::UValue(ValueType type)
{
   U_TRACE_REGISTER_OBJECT(0, UValue, "%d", type)

   U_INTERNAL_ASSERT_RANGE(0,type,OBJECT_VALUE)

   switch ((type_ = type))
      {
      case BOOLEAN_VALUE: value.bool_ = false;                  break;
      case     INT_VALUE:
      case    UINT_VALUE: value.int_  = 0;                      break;
      case    NULL_VALUE:
      case    REAL_VALUE: value.real_ = 0.0;                    break;
      case  STRING_VALUE: value.ptr_  = UString::string_null;   break;
      case   ARRAY_VALUE: value.ptr_  = U_NEW(UVector<void*>);  break;
      case  OBJECT_VALUE: value.ptr_  = U_NEW(UHashMap<void*>);
       ((UHashMap<void*>*)value.ptr_)->allocate();              break;
      }
}

UValue::~UValue()
{
   U_TRACE_UNREGISTER_OBJECT(0, UValue)

   U_INTERNAL_ASSERT_RANGE(0,type_,OBJECT_VALUE)

   switch (type_)
      {
      case STRING_VALUE:
         {
         U_INTERNAL_ASSERT_POINTER(value.ptr_)

         delete (UString*)value.ptr_;
         }
      break;

      case ARRAY_VALUE:
         {
         U_INTERNAL_ASSERT_POINTER(value.ptr_)

         delete (UVector<void*>*)value.ptr_;
         }
      break;

      case OBJECT_VALUE:
         {
         U_INTERNAL_ASSERT_POINTER(value.ptr_)

         ((UHashMap<void*>*)value.ptr_)->deallocate();

         delete (UHashMap<void*>*)value.ptr_;
         }
      break;
      }
}

// CONVERSION

bool UValue::asBool() const
{
   U_TRACE(0, "UValue::asBool()")

   U_INTERNAL_ASSERT_RANGE(0,type_,OBJECT_VALUE)

   bool result = false;

   switch (type_)
      {
      case    NULL_VALUE:                                                   break;
      case     INT_VALUE: if (value.int_)         result = true;            break;
      case    UINT_VALUE: if (value.uint_)        result = true;            break;
      case    REAL_VALUE: if (value.real_ != 0.0) result = true;            break;
      case BOOLEAN_VALUE:                         result = value.bool_;     break;

      case  STRING_VALUE: result = ((UString*)value.ptr_)->empty();         break;
      case   ARRAY_VALUE: result = ((UVector<void*>*)value.ptr_)->empty();  break;
      case  OBJECT_VALUE: result = ((UHashMap<void*>*)value.ptr_)->empty(); break;
      }

   U_RETURN(result);
}

int UValue::asInt() const
{
   U_TRACE(0, "UValue::asInt()")

   U_INTERNAL_ASSERT_RANGE(0,type_,OBJECT_VALUE)

   int result = 0;

   switch (type_)
      {
      case    NULL_VALUE:                                       break;
      case     INT_VALUE:                  result = value.int_; break;
      case BOOLEAN_VALUE: if (value.bool_) result = 1;          break;
      case    UINT_VALUE:
         U_INTERNAL_ASSERT_MSG(value.uint_ < (unsigned)INT_MAX, "Integer out of signed integer range")
         result = (int)value.uint_;
      break;
      case    REAL_VALUE:
         U_INTERNAL_ASSERT_MSG(value.real_ >= INT_MIN && value.real_ <= INT_MAX, "Real out of signed integer range")
         result = (int)value.real_;
      break;

      case  STRING_VALUE:
      case   ARRAY_VALUE:
      case  OBJECT_VALUE:
         U_INTERNAL_ASSERT_MSG(false, "Type is not convertible to signed integer...")
      break;
      }

   U_RETURN(result);
}

unsigned int UValue::asUInt() const
{
   U_TRACE(0, "UValue::asUInt()")

   U_INTERNAL_ASSERT_RANGE(0,type_,OBJECT_VALUE)

   unsigned int result = 0;

   switch (type_)
      {
      case    NULL_VALUE:                                        break;
      case    UINT_VALUE:                  result = value.uint_; break;
      case BOOLEAN_VALUE: if (value.bool_) result = 1;           break;
      case     INT_VALUE:
         U_INTERNAL_ASSERT_MSG(value.int_ >= 0, "Negative integer can not be converted to unsigned integer")
         result = value.int_;
      break;

      case    REAL_VALUE:
         U_INTERNAL_ASSERT_MSG(value.real_ >= 0.0 && value.real_ <= UINT_MAX, "Real out of unsigned integer range")
         result = (unsigned int)value.real_;
      break;

      case  STRING_VALUE:
      case   ARRAY_VALUE:
      case  OBJECT_VALUE:
         U_INTERNAL_ASSERT_MSG(false, "Type is not convertible to unsigned integer...")
      break;
      }

   U_RETURN(result);
}

double UValue::asDouble() const
{
   U_TRACE(0, "UValue::asDouble()")

   U_INTERNAL_ASSERT_RANGE(0,type_,OBJECT_VALUE)

   double result = 0.0;

   switch (type_)
      {
      case    NULL_VALUE:                                        break;
      case     INT_VALUE:                  result = value.int_;  break;
      case    UINT_VALUE:                  result = value.uint_; break;
      case    REAL_VALUE:                  result = value.real_; break;
      case BOOLEAN_VALUE: if (value.bool_) result = 1.0;         break;

      case  STRING_VALUE:
      case   ARRAY_VALUE:
      case  OBJECT_VALUE:
         U_INTERNAL_ASSERT_MSG(false, "Type is not convertible to double...")
      break;
      }

   U_RETURN(result);
}

UString UValue::asString() const
{
   U_TRACE(0, "UValue::asString()")

   U_INTERNAL_ASSERT_RANGE(0,type_,OBJECT_VALUE)

   UString result;

   switch (type_)
      {
      case    NULL_VALUE:                                                                                            break;
      case  STRING_VALUE: result = *(UString*)value.ptr_;                                                            break;
      case BOOLEAN_VALUE: result = (value.bool_ ? U_STRING_FROM_CONSTANT("true") : U_STRING_FROM_CONSTANT("false")); break;

      case     INT_VALUE:
      case    UINT_VALUE:
      case    REAL_VALUE:
      case   ARRAY_VALUE:
      case  OBJECT_VALUE:
         U_INTERNAL_ASSERT_MSG(false, "Type is not convertible to string...")
      break;
      }

   U_RETURN_STRING(result);
}

bool UValue::isConvertibleTo(UValue::ValueType other) const
{
   U_TRACE(0, "UValue::isConvertibleTo(%d)", other)

   U_INTERNAL_ASSERT_RANGE(0,type_,OBJECT_VALUE)

   bool result = false;

   switch (type_)
      {
      case    NULL_VALUE: result = true;                                          break;
      case BOOLEAN_VALUE: result = (other != NULL_VALUE || value.bool_ == false); break;

      case     INT_VALUE: result = (other == NULL_VALUE && value.int_ == 0)        ||
                                    other ==  INT_VALUE                            ||
                                   (other == UINT_VALUE && value.int_ >= 0)        ||
                                    other == REAL_VALUE                            ||
                                    other == BOOLEAN_VALUE;
      break;
      case    UINT_VALUE: result = (other == NULL_VALUE && value.uint_ ==       0) ||
                                   (other ==  INT_VALUE && value.uint_ <= INT_MAX) ||
                                    other == UINT_VALUE                            ||
                                    other == REAL_VALUE                            ||
                                    other == BOOLEAN_VALUE;
      break;
      case    REAL_VALUE: result = (other == NULL_VALUE && value.real_ ==     0.0)                            ||
                                   (other ==  INT_VALUE && value.real_ >= INT_MIN && value.real_ <=  INT_MAX) ||
                                   (other == UINT_VALUE && value.real_ >=     0.0 && value.real_ <= UINT_MAX) ||
                                    other == REAL_VALUE                                                       ||
                                    other == BOOLEAN_VALUE;
      break;
      case  STRING_VALUE: result = (other == NULL_VALUE && ((UString*)value.ptr_)->empty()) ||
                                    other == STRING_VALUE;
      break;
      case   ARRAY_VALUE: result = (other == NULL_VALUE && ((UVector<void*>*)value.ptr_)->empty()) ||
                                    other == ARRAY_VALUE;
      break;
      case  OBJECT_VALUE: result = (other == NULL_VALUE && ((UHashMap<void*>*)value.ptr_)->empty()) ||
                                    other == OBJECT_VALUE;
      break;
      }

   U_RETURN(result);
}

uint32_t UValue::getMemberNames(UVector<UString>& members) const
{
   U_TRACE(0, "UValue::getMemberNames(%p)", &members)

   U_INTERNAL_ASSERT_RANGE(0,type_,OBJECT_VALUE)
   U_INTERNAL_ASSERT(type_ == OBJECT_VALUE || type_ == NULL_VALUE)

   uint32_t n = members.size(), size = 0;

   if (type_ != NULL_VALUE)
      {
      ((UHashMap<void*>*)value.ptr_)->getKeys(members);

      size = members.size() - n;
      }

   U_RETURN(size);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UValue::dump(bool reset) const
{
   *UObjectIO::os << "type_ " << type_ << '\n';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
