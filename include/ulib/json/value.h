// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    value.h - Represents a JSON (JavaScript Object Notation) value
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_VALUE_H
#define ULIB_VALUE_H 1

#include <ulib/string.h>

/** \brief Represents a <a HREF="http://www.json.org">JSON</a> value.
 *
 * This class is a discriminated union wrapper that can represents a:
 *
 * - 'null'
 * - boolean
 * - signed integer
 * - unsigned integer
 * - double
 * - UTF-8 string
 * - an ordered list of Value
 * - collection of name/value pairs (javascript object)
 *
 * The type of the held value is represented by a #ValueType and can be obtained using type().
 *
 * values of an #OBJECT_VALUE or #ARRAY_VALUE can be accessed using operator[]() methods.
 *
 * Non const methods will automatically create the a #nullValue element if it does not exist.
 *
 * The sequence of an #ARRAY_VALUE will be automatically resize and initialized
 * with #NULL_VALUE. resize() can be used to enlarge or truncate an #ARRAY_VALUE.
 *
 * The get() methods can be used to obtain default value in the case the required element
 * does not exist.
 *
 * It is possible to iterate over the list of a #OBJECT_VALUE values using
 * the getMemberNames() method.
 */

union uuvalue {
   bool bool_;
   int int_;
   unsigned int uint_;
   void* ptr_;
   double real_;
};

template <class T> class UVector;

class U_EXPORT UValue {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   /** \brief Type of the value held by a UValue object
    */

   enum ValueType {
      NULL_VALUE = 0, // 'null' value
   BOOLEAN_VALUE = 1, // bool value
       INT_VALUE = 2, // signed integer value
      UINT_VALUE = 3, // unsigned integer value
      REAL_VALUE = 4, // double value
    STRING_VALUE = 5, // UTF-8 string value
     ARRAY_VALUE = 6, // array value (ordered list)
    OBJECT_VALUE = 7  // object value (collection of name/value pairs).
   };

   // Costruttori

   UValue()
      {
      U_TRACE_REGISTER_OBJECT(0, UValue, "", 0)

      type_       = NULL_VALUE;
      value.real_ = 0.0;
      }

   UValue(bool value_)
      {
      U_TRACE_REGISTER_OBJECT(0, UValue, "%b", value_)

      type_       = BOOLEAN_VALUE;
      value.bool_ = value_;
      }

   UValue(int value_)
      {
      U_TRACE_REGISTER_OBJECT(0, UValue, "%d", value_)

      type_      = INT_VALUE;
      value.int_ = value_;
      }

   UValue(unsigned int value_)
      {
      U_TRACE_REGISTER_OBJECT(0, UValue, "%u", value_)

      type_       = UINT_VALUE;
      value.uint_ = value_;
      }

   UValue(double value_)
      {
      U_TRACE_REGISTER_OBJECT(0, UValue, "%f", value_)

      type_       = REAL_VALUE;
      value.real_ = value_;
      }

   UValue(void* value_, uint32_t len)
      {
      U_TRACE_REGISTER_OBJECT(0, UValue, "%.*S,%u", len, (char*)value_, len)

      type_      = STRING_VALUE;
      value.ptr_ = U_NEW(UString(value_, len));
      }

   UValue(const char* value_, uint32_t len)
      {
      U_TRACE_REGISTER_OBJECT(0, UValue, "%.*S,%u", len, value_, len)

      type_      = STRING_VALUE;
      value.ptr_ = U_NEW(UString(value_, len));
      }

   UValue(ValueType type);

   ~UValue();

   // SERVICES

   bool isNull()
      {
      U_TRACE(0, "UValue::isNull()")

      bool result = (type_ == NULL_VALUE);

      U_RETURN(result);
      }

   bool isBool()
      {
      U_TRACE(0, "UValue::isBool()")

      bool result = (type_ == BOOLEAN_VALUE);

      U_RETURN(result);
      }

   bool isInt()
      {
      U_TRACE(0, "UValue::isInt()")

      bool result = (type_ == INT_VALUE);

      U_RETURN(result);
      }

   bool isUInt()
      {
      U_TRACE(0, "UValue::isUInt()")

      bool result = (type_ == UINT_VALUE);

      U_RETURN(result);
      }

   bool isIntegral()
      {
      U_TRACE(0, "UValue::isIntegral()")

      bool result = (type_ ==     INT_VALUE ||
                     type_ ==    UINT_VALUE ||
                     type_ == BOOLEAN_VALUE);

      U_RETURN(result);
      }

   bool isDouble()
      {
      U_TRACE(0, "UValue::isDouble()")

      bool result = (type_ == REAL_VALUE);

      U_RETURN(result);
      }

   bool isNumeric()
      {
      U_TRACE(0, "UValue::isNumeric()")

      bool result = (type_ ==     INT_VALUE ||
                     type_ ==    UINT_VALUE ||
                     type_ == BOOLEAN_VALUE ||
                     type_ ==    REAL_VALUE);

      U_RETURN(result);
      }

   bool isString() const
      {
      U_TRACE(0, "UValue::isString()")

      bool result = (type_ == STRING_VALUE);

      U_RETURN(result);
      }

   bool isArray() const
      {
      U_TRACE(0, "UValue::isArray()")

      bool result = (type_ ==  NULL_VALUE ||
                     type_ == ARRAY_VALUE);

      U_RETURN(result);
      }

   bool isObject() const
      {
      U_TRACE(0, "UValue::isObject()")

      bool result = (type_ ==   NULL_VALUE ||
                     type_ == OBJECT_VALUE);

      U_RETURN(result);
      }

   ValueType type() const { return (ValueType)type_; }

   // CONVERSION

   bool         asBool() const;
   int          asInt() const;
   unsigned int asUInt() const;
   double       asDouble() const;
   UString      asString() const;

   bool isConvertibleTo(UValue::ValueType other) const;

   // MEMBERS

   // \brief Return a list of the member names.
   //
   // If null, return an empty list.
   // \pre type() is OBJECT_VALUE or NULL_VALUE
   // \post if type() was NULL_VALUE, it remains NULL_VALUE 

   uint32_t getMemberNames(UVector<UString>& members) const;

   // STREAM

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   int type_;
   union uuvalue value;

private:
   UValue(const UValue&)            {}
   UValue& operator=(const UValue&) { return *this; }
};

#endif
