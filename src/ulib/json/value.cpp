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

#include <ulib/tokenizer.h>
#include <ulib/json/value.h>
#include <ulib/utility/escape.h>
#include <ulib/container/vector.h>
#include <ulib/container/hash_map.h>

#include <limits.h>

UValue::UValue(ValueType type)
{
   U_TRACE_REGISTER_OBJECT(0, UValue, "%d", type)

   U_INTERNAL_ASSERT_RANGE(0,type,OBJECT_VALUE)

   switch ((type_ = type))
      {
      case BOOLEAN_VALUE: value.bool_ = false;                    break;
      case     INT_VALUE:
      case    UINT_VALUE: value.int_  = 0;                        break;
      case    NULL_VALUE:
      case    REAL_VALUE: value.real_ = 0.0;                      break;
      case  STRING_VALUE: value.ptr_  = UString::string_null;     break;
      case   ARRAY_VALUE: value.ptr_  = U_NEW( UVector<UValue*>); break;
      case  OBJECT_VALUE: value.ptr_  = U_NEW(UHashMap<UValue*>);
                  ((UHashMap<UValue*>*)value.ptr_)->allocate();   break;
      }
}

void UValue::clear() // erase all element
{
   U_TRACE(0, "UValue::clear()")

   U_CHECK_MEMORY

   U_INTERNAL_DUMP("type_ = %d", type_)

   U_INTERNAL_ASSERT_RANGE(0,type_,OBJECT_VALUE)

   switch (type_)
      {
      case STRING_VALUE:
         {
         U_INTERNAL_ASSERT_POINTER(value.ptr_)

         U_INTERNAL_DUMP("value.ptr_ = %.*S", U_STRING_TO_TRACE(*getString()))

         delete getString();
         }
      break;

      case ARRAY_VALUE:
         {
         U_INTERNAL_ASSERT_POINTER(value.ptr_)

         delete getArray();
         }
      break;

      case OBJECT_VALUE:
         {
         U_INTERNAL_ASSERT_POINTER(value.ptr_)

         getObject()->clear();
         getObject()->deallocate();

         delete getObject();
         }
      break;
      }

   type_       = NULL_VALUE;
   value.real_ = 0.0;
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

      case  STRING_VALUE: result = getString()->empty(); break;
      case   ARRAY_VALUE: result =  getArray()->empty(); break;
      case  OBJECT_VALUE: result = getObject()->empty(); break;
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
      case    NULL_VALUE:                        break;
      case  STRING_VALUE: result = *getString(); break;

      case BOOLEAN_VALUE: result = (value.bool_ ? U_STRING_FROM_CONSTANT("true")
                                                : U_STRING_FROM_CONSTANT("false")); break;

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
      case  STRING_VALUE: result = (other == NULL_VALUE && getString()->empty()) ||
                                    other == STRING_VALUE;
      break;
      case   ARRAY_VALUE: result = (other == NULL_VALUE &&  getArray()->empty()) ||
                                    other == ARRAY_VALUE;
      break;
      case  OBJECT_VALUE: result = (other == NULL_VALUE && getObject()->empty()) ||
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
      getObject()->getKeys(members);

      size = members.size() - n;
      }

   U_RETURN(size);
}

uint32_t UValue::size() const
{
   U_TRACE(0, "UValue::size()")

   uint32_t result = 0;

        if (type_ == STRING_VALUE) result = getString()->size();
   else if (type_ ==  ARRAY_VALUE) result =  getArray()->size();
   else if (type_ == OBJECT_VALUE) result = getObject()->size();

   U_RETURN(result);
}

UValue& UValue::operator[](uint32_t pos)
{
   U_TRACE(0, "UValue::operator[](%u)", pos)

   UValue* result = (type_ == ARRAY_VALUE ? getArray()->at(pos) : this);

   return *result;
}

UValue& UValue::operator[](const UString& key)
{
   U_TRACE(0, "UValue::operator[](%.*S)", U_STRING_TO_TRACE(key))

   UValue* result = (type_ == OBJECT_VALUE ? getObject()->operator[](key) : this);

   return *result;
}

void UValue::output(UString& result, UValue& value)
{
   U_TRACE(0, "UValue::output(%.*S,%p)", U_STRING_TO_TRACE(result), &value)

   U_INTERNAL_ASSERT_RANGE(0,value.type_,OBJECT_VALUE)

   char buffer[32];

   switch (value.type_)
      {
      case    NULL_VALUE:                  (void) result.append(U_CONSTANT_TO_PARAM("null"));  break;
      case BOOLEAN_VALUE: value.asBool() ? (void) result.append(U_CONSTANT_TO_PARAM("true"))
                                         : (void) result.append(U_CONSTANT_TO_PARAM("false")); break;

      case  INT_VALUE: (void) result.append(buffer, u_snprintf(buffer, sizeof(buffer), "%d", value.asInt()));  break;
      case UINT_VALUE: (void) result.append(buffer, u_snprintf(buffer, sizeof(buffer), "%u", value.asUInt())); break;
      case REAL_VALUE:
         {
         uint32_t n = u_snprintf(buffer, sizeof(buffer), "%#.16g", value.asDouble());

         const char* ch = buffer + n - 1;

         if (*ch == '0')
            {
            while (ch > buffer && *ch == '0') --ch;

            const char* last_nonzero = ch;

            while (ch >= buffer)
               {
               switch (*ch)
                  {
                  case '0':
                  case '1':
                  case '2':
                  case '3':
                  case '4':
                  case '5':
                  case '6':
                  case '7':
                  case '8':
                  case '9': --ch; continue;
                  case '.': n = last_nonzero - buffer + 2; // Truncate zeroes to save bytes in output, but keep one.
                  default: goto end;
                  }
               }
            }

end:
         (void) result.append(buffer, n);

         break;
         }

      case STRING_VALUE:
         {
         (void) result.reserve(result.size() + value.getString()->size() * 6);

         UEscape::encode(*(value.getString()), result, true);
         }
      break;

      case ARRAY_VALUE:
         {
         (void) result.append(1, '[');

         for (uint32_t index = 0, size = value.size(); index < size; ++index)
            {
            if (index) (void) result.append(1, ',');

            output(result, value[index]);
            }

         (void) result.append(1, ']');
         }
      break;

      case OBJECT_VALUE:
         {
         (void) result.append(1, '{');

         UString name;
         uint32_t sz = value.size();
         UVector<UString> members(sz);
         (void) value.getMemberNames(members);

      // if (sz > 1) members.sort();

         for (uint32_t index = 0; index < sz; ++index)
            {
            if (index) (void) result.append(1, ',');

            name = members[index];

            (void) result.reserve(result.size() + name.size() * 6);

            UEscape::encode(name, result, true);

            (void) result.append(1, ':');

            output(result, value[name]);
            }

         (void) result.append(1, '}');
         }
      break;
      }
}

U_NO_EXPORT bool UValue::readValue(UTokenizer& tok, UValue* value)
{
   U_TRACE(0, "UValue::readValue(%p,%p)", &tok, value)

   tok.skipSpaces();

   char c;
   bool result;
   const char* start = tok.getPointer();

   c = tok.next();

   switch (c)
      {
      case '\0':
         {
         result             = true;
         value->type_       = NULL_VALUE;
         value->value.real_ = 0.0;
         }
      break;

      case 'n':
         {
         value->type_       = NULL_VALUE;
         value->value.real_ = 0.0;

         result = tok.skipToken(U_CONSTANT_TO_PARAM("ull"));
         }
      break;

      case 't':
      case 'f':
         {
         result = (c == 't' ? tok.skipToken(U_CONSTANT_TO_PARAM("rue"))
                            : tok.skipToken(U_CONSTANT_TO_PARAM("alse")));

         if (result)
            {
            value->type_       = BOOLEAN_VALUE;
            value->value.bool_ = (c == 't');
            }
         }
      break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '-':
         {
         bool breal;

         if ((result = tok.skipNumber(breal)))
            {
            if (breal)
               {
               value->type_       = REAL_VALUE;
               value->value.real_ = strtod(start, 0);

               U_INTERNAL_DUMP("value.real_ = %.16g", value->value.real_)
               }
            else if (c == '-')
               {
               value->type_      = INT_VALUE;
               value->value.int_ = strtol(start, 0, 10);

               U_INTERNAL_DUMP("value.int_ = %d", value->value.int_)
               }
            else
               {
               value->type_       = UINT_VALUE;
               value->value.uint_ = strtoul(start, 0, 10);

               U_INTERNAL_DUMP("value.uint_ = %u", value->value.uint_)
               }
            }
         }
      break;

      case '"':
         {
         value->type_    = STRING_VALUE;
         const char* ptr = tok.getPointer();
         const char* end = tok.getEnd();
         const char* last = u_find_char(ptr, end, '"');
         uint32_t sz      = (last ? last - ptr : 0);

         U_INTERNAL_DUMP("sz = %u", sz)

         if (sz)
            {
            value->value.ptr_ = U_NEW(UString(sz));

            result = UEscape::decode(ptr, sz, *(value->getString()));
            }
         else
            {
            value->value.ptr_ = U_NEW(UString());

            result = true;
            }

         if (last < end) tok.setPointer(last+1);

         U_INTERNAL_DUMP("value.ptr_ = %.*S", U_STRING_TO_TRACE(*(value->getString())))
         }
      break;

      case '[':
         {
         value->type_      = ARRAY_VALUE;
         value->value.ptr_ = U_NEW(UVector<UValue*>);

         UValue* item;

         while (true)
            {
            tok.skipSpaces();

            c = tok.next();

            if (c == ']' ||
                c == '\0') break;

            if (c != ',') tok.back();

            item = U_NEW(UValue);

            if (readValue(tok, item) == false)
               {
               delete item;

               U_RETURN(false);
               }

            value->getArray()->push(item);
            }

         result = true;

         uint32_t sz = value->getArray()->size();

         U_INTERNAL_DUMP("sz = %u", sz)

         if (sz) value->getArray()->reserve(sz);
         }
      break;

      case '{':
         {
         value->type_      = OBJECT_VALUE;
         value->value.ptr_ = U_NEW(UHashMap<UValue*>);

         value->getObject()->allocate();

         UValue  name;
         UValue* item;

         while (true)
            {
            tok.skipSpaces();

            c = tok.next();

            if (c == '}' ||
                c == '\0') break;

            if (c != ',') tok.back();

            if (readValue(tok, &name) == false) U_RETURN(false);

            tok.skipSpaces();

            if (tok.next()      != ':' ||
                name.isString() == false)
               {
               U_RETURN(false);
               }

            item = U_NEW(UValue);

            if (readValue(tok, item) == false)
               {
               delete item;

               U_RETURN(false);
               }

            value->getObject()->insert(*(name.getString()), item);

            name.clear();
            }

         result = true;

         U_DUMP("hash map size = %u", value->getObject()->size())
         }
      break;

      default:
         result = false;
      break;
      }

   U_RETURN(result);
}

bool UValue::parse(const UString& document)
{
   U_TRACE(0, "UValue::parse(%.*S)", U_STRING_TO_TRACE(document))

   UTokenizer tok(document);

   tok.skipSpaces();

   char c = tok.current();

   if ((c == '['   ||
        c == '{' ) && 
       readValue(tok, this))
      {
      U_RETURN(true);
      }

   U_RETURN(false);
}

// STREAM

U_EXPORT istream& operator>>(istream& is, UValue& v)
{
   U_TRACE(0, "UValue::operator>>(%p,%p)", &is, &v)

   UString doc(U_CAPACITY);

   (void) doc.getline(is, (char)EOF);

   (void) v.parse(doc);

   return is;
}

U_EXPORT ostream& operator<<(ostream& os, UValue& v)
{
   U_TRACE(0, "UValue::operator<<(%p,%p)", &os, &v)

   os << v.output();

   return os;
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
