// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    header.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/url.h>
#include <ulib/mime/header.h>
#include <ulib/utility/uhttp.h>

const UString* UMimeHeader::str_name;
const UString* UMimeHeader::str_ascii;
const UString* UMimeHeader::str_charset;
const UString* UMimeHeader::str_msg_rfc;
const UString* UMimeHeader::str_boundary;
const UString* UMimeHeader::str_filename;
const UString* UMimeHeader::str_txt_xml;
const UString* UMimeHeader::str_txt_plain;
const UString* UMimeHeader::str_mime_version;
const UString* UMimeHeader::str_content_transfer_encoding;

void UMimeHeader::str_allocate()
{
   U_TRACE(0+256, "UMimeHeader::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_name,0)
   U_INTERNAL_ASSERT_EQUALS(str_ascii,0)
   U_INTERNAL_ASSERT_EQUALS(str_charset,0)
   U_INTERNAL_ASSERT_EQUALS(str_msg_rfc,0)
   U_INTERNAL_ASSERT_EQUALS(str_boundary,0)
   U_INTERNAL_ASSERT_EQUALS(str_filename,0)
   U_INTERNAL_ASSERT_EQUALS(str_txt_xml,0)
   U_INTERNAL_ASSERT_EQUALS(str_txt_plain,0)
   U_INTERNAL_ASSERT_EQUALS(str_mime_version,0)
   U_INTERNAL_ASSERT_EQUALS(str_content_transfer_encoding,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("name") },
      { U_STRINGREP_FROM_CONSTANT("charset") },
      { U_STRINGREP_FROM_CONSTANT("us-ascii") },
      { U_STRINGREP_FROM_CONSTANT("boundary") },
      { U_STRINGREP_FROM_CONSTANT("filename") },
      { U_STRINGREP_FROM_CONSTANT("text/xml") },
      { U_STRINGREP_FROM_CONSTANT("text/plain") },
      { U_STRINGREP_FROM_CONSTANT("MIME-Version") },
      { U_STRINGREP_FROM_CONSTANT("message/rfc822") },
      { U_STRINGREP_FROM_CONSTANT("Content-Transfer-Encoding") }
   };

   U_NEW_ULIB_OBJECT(str_name,                      U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_charset,                   U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_ascii,                     U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_boundary,                  U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_filename,                  U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_txt_xml,                   U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_txt_plain,                 U_STRING_FROM_STRINGREP_STORAGE(6));
   U_NEW_ULIB_OBJECT(str_mime_version,              U_STRING_FROM_STRINGREP_STORAGE(7));
   U_NEW_ULIB_OBJECT(str_msg_rfc,                   U_STRING_FROM_STRINGREP_STORAGE(8));
   U_NEW_ULIB_OBJECT(str_content_transfer_encoding, U_STRING_FROM_STRINGREP_STORAGE(9));
}

//=============================================================================
// parsing an UString for MIME headers.
// 
// Mime-type headers comprise a keyword followed by ":" followed
// by whitespace and the value. If a line begins with witespace it is
// a continuation of the preceeding line.
//
// MIME headers are delimited by an empty line.
//=============================================================================

uint32_t UMimeHeader::parse(const char* ptr, uint32_t len)
{
   U_TRACE(0, "UMimeHeader::parse(%.*S,%u)", len, ptr, len)

   if (*ptr == '\n') U_RETURN(0); // line empty: we have reached the MIME headers delimiter

   bool cr;
   const char* pkv;
   UString key, value;
   const char* prev = ptr;
   uint32_t n = table.size();
   const char* _end = ptr + len;

   U_INTERNAL_DUMP("n = %u", n)

   // Until we reach the data part of the response we know we are dealing with iso-8859-1 characters

   while (prev < _end)
      {
      U_INTERNAL_DUMP("prev = %.*S", 80, prev)

      ptr = (const char*) memchr(prev, '\n', _end - prev);

      U_INTERNAL_DUMP("ptr  = %.*S", 80, ptr)

      if (ptr == 0) ptr = _end; // we have reached the MIME headers end without line empty...
      else
         {
         len = (ptr - prev);

         U_INTERNAL_DUMP("len  = %u", len)

         if (len == 0 || u_isWhiteSpace(prev, len)) break; // line empty: we have reached the MIME headers delimiter...
         }

      cr = (ptr[-1] == '\r');

      U_INTERNAL_DUMP("cr   = %b", cr)

      // Check for continuation of preceding header

      if (u__isspace(*prev))
         {
         do { ++prev; } while (prev < _end && u__isspace(*prev));

         U_INTERNAL_ASSERT_MINOR(prev,ptr)

         if (value.empty() == false)
            {
            value.append(prev, ptr - prev - cr);

            table.replaceAfterFind(value);
            }
         }
      else
         {
         pkv = (const char*) memchr(prev, ':', _end - prev);

         if (pkv == 0 ||
             pkv >= ptr)
            {
            break;
            }

         (void) key.assign(prev, pkv - prev);

         do { ++pkv; } while (pkv < _end && u__isspace(*pkv));

         (void) value.assign(pkv, ptr - pkv - cr);

         // Check for duplication of header

         if (containsHeader(key) == false) table.insertAfterFind(key, value);
         else
            {
            UStringRep* rep = table.elem();

            if (value.equal(rep) == false)
               {
               UString duplicate(rep->size() + 4U + key.size() + 2U);

               duplicate.snprintf("%.*s%s%.*s: ", U_STRING_TO_TRACE(*rep), (cr ? U_CRLF : U_LF), U_STRING_TO_TRACE(key));

               U_INTERNAL_DUMP("duplicate = %.*S", U_STRING_TO_TRACE(duplicate))

               table.replaceAfterFind(duplicate);
               }
            }
         }

      prev = ptr + 1;
      }

   U_RETURN(table.size() - n);
}

uint32_t UMimeHeader::parse(const UString& buffer)
{
   U_TRACE(0, "UMimeHeader::parse(%.*S)", U_STRING_TO_TRACE(buffer))

   header = buffer;

   uint32_t n = parse(buffer.data(), buffer.size());

   U_RETURN(n);
}

// inlining failed in call to 'UMimeHeader::removeHeader(UString const&)': call is unlikely and code size would grow

void UMimeHeader::removeHeader(const UString& key)
{
   U_TRACE(0, "UMimeHeader::removeHeader(%.*S)", U_STRING_TO_TRACE(key))

   if (containsHeader(key)) table.eraseAfterFind();
}

uint32_t UMimeHeader::getAttributeFromKeyValue(const UString& key_value, UVector<UString>& name_value)
{
   U_TRACE(0, "UMimeHeader::getAttributeFromKeyValue(%.*S,%p)", U_STRING_TO_TRACE(key_value), &name_value)

   uint32_t n = 0;

   if (key_value.empty() == false)
      {
      uint32_t pos = key_value.find(';'); // Content-Disposition: form-data; name="input_file"; filename="/tmp/4dcd39e8-2a84-4242-b7bc-ca74922d26e1"

      if (pos != U_NOT_FOUND)
         {
         // NB: we must use substr() because of the possibility of unQuote()...

         n = name_value.split(key_value.substr(pos + 1), " =;"); // NB: I can't use also '"' char...

         if ((n & 1)) // NB: it is even...
            {
            (void) name_value.pop();

            n -= 1;
            }
         }
      }

   U_RETURN(n);
}

UString UMimeHeader::getValueAttributeFromKeyValue(const UString& name, UVector<UString>& name_value, bool ignore_case)
{
   U_TRACE(0, "UMimeHeader::getValueAttributeFromKeyValue(%.*S,%p,%b)", U_STRING_TO_TRACE(name), &name_value, ignore_case)

   UString value;

   for (int32_t i = 0, n = name_value.size(); i < n; ++i)
      {
      if (name_value[i++].equal(name, ignore_case))
         {
         value = name_value[i];

         if (value.isQuoted()) value.unQuote();

         break;
         }
      }

   U_INTERNAL_DUMP("value = %.*S", U_STRING_TO_TRACE(value))

   U_RETURN_STRING(value);
}

UString UMimeHeader::getValueAttributeFromKeyValue(const UString& value, const UString& name, bool ignore_case)
{
   U_TRACE(0, "UMimeHeader::getValueAttributeFromKeyValue(%.*S,%.*S,%b)", U_STRING_TO_TRACE(value), U_STRING_TO_TRACE(name), ignore_case)

   UString result;
   UVector<UString> name_value;

   if (getAttributeFromKeyValue(value, name_value)) result = getValueAttributeFromKeyValue(name, name_value, ignore_case);

   U_RETURN_STRING(result);
}

bool UMimeHeader::getNames(const UString& cdisposition, UString& name, UString& filename)
{
   U_TRACE(0, "UMimeHeader::getNames(%.*S,%p,%p)", U_STRING_TO_TRACE(cdisposition), &name, &filename)

   bool result = false;
   UVector<UString> name_value;

   // Content-Disposition: form-data; name="input_file"; filename="/tmp/4dcd39e8-2a84-4242-b7bc-ca74922d26e1"

   if (getAttributeFromKeyValue(cdisposition, name_value))
      {
      for (int32_t i = 0, n = name_value.size(); i < n; i += 2)
         {
         if (name_value[i].equal(*str_name, false))
            {
            name = name_value[i+1];

            U_INTERNAL_DUMP("name_value[%d] = %.*S", i+1, U_STRING_TO_TRACE(name))

            if (name.isQuoted()) name.unQuote();

            U_INTERNAL_DUMP("name = %.*S", U_STRING_TO_TRACE(name))
            }
         else if (name_value[i].equal(*str_filename, false))
            {
            filename = name_value[i+1];

            U_INTERNAL_DUMP("name_value[%d] = %.*S", i+1, U_STRING_TO_TRACE(filename))

            if (filename.empty() == false)
               {
               if (filename.isQuoted()) filename.unQuote();

               // This is hairy: Netscape and IE don't encode the filenames
               // The RFC says they should be encoded, so I will assume they are

               uint32_t len = filename.size();

               UString filename_decoded(len);

               Url::decode(filename.data(), len, filename_decoded);

               filename = filename_decoded;

               U_INTERNAL_DUMP("filename = %.*S", U_STRING_TO_TRACE(filename))

               result = true;
               }
            }
         }
      }

   U_RETURN(result);
}

UString UMimeHeader::getCharSet(const UString& content_type)
{
   U_TRACE(0, "UMimeHeader::getCharSet(%.*S)", U_STRING_TO_TRACE(content_type))

   U_INTERNAL_ASSERT(content_type)

   UString charset = getValueAttributeFromKeyValue(content_type, *str_charset, false);

   // For some RFC, encoding is us-ascii if it's not specifief in header.

   if (charset.empty()) charset = *str_ascii;

   U_INTERNAL_DUMP("charset = %.*S", U_STRING_TO_TRACE(charset))

   U_RETURN_STRING(charset);
}

UString UMimeHeader::shortContentType(const UString& content_type)
{
   U_TRACE(0, "UMimeHeader::shortContentType(%.*S)", U_STRING_TO_TRACE(content_type))

   U_INTERNAL_ASSERT(content_type)

   uint32_t pos = content_type.find(';');

   if (pos != U_NOT_FOUND)
      {
      UString result = content_type.substr(0U, pos);

      U_RETURN_STRING(result);
      }

   U_RETURN_STRING(content_type);
}

void UMimeHeader::writeHeaders(UString& buffer)
{
   U_TRACE(0, "UMimeHeader::writeHeaders(%.*S)", U_STRING_TO_TRACE(buffer))

   UStringRep* key;
   UStringRep* value;
   uint32_t len1, len2;
   UString tmp(U_CAPACITY);

   if (table.first())
      {
      do {
         key   = table.key();
         value = table.elem();

         len1 =   key->size();
         len2 = value->size();

         tmp.setBuffer(len1 + 2 + len2 + 2);

         tmp.snprintf("%.*s: %.*s\r\n", len1, key->data(), len2, value->data());

         buffer += tmp;
         }
      while (table.next());
      }

   U_INTERNAL_DUMP("buffer = %.*S", U_STRING_TO_TRACE(buffer))
}

UString UMimeHeader::getHeaders()
{
   U_TRACE(0, "UMimeHeader::getHeaders()")

   if (empty() == false)
      {
      UString buffer(U_CAPACITY);

      writeHeaders(buffer);

      U_RETURN_STRING(buffer);
      }

   U_RETURN_STRING(UString::getStringNull());
}

// read from socket

bool UMimeHeader::readHeader(USocket* socket, UString& data)
{
   U_TRACE(0, "UMimeHeader::readHeader(%p,%p)", socket, &data)

   U_ASSERT(empty())

   bool result = (UHTTP::readHeader(socket, data) &&
                  UHTTP::findEndHeader(     data)
                     ? parse(data.c_pointer(u_http_info.startHeader), u_http_info.szHeader) > 0
                     : false);

   if (result) header = data;

   U_RETURN(result);
}

// STREAM

U_EXPORT ostream& operator<<(ostream& os, UMimeHeader& h)
{
   U_TRACE(0+256, "UMimeHeader::operator<<(%p,%p)", &os, &h)

   if (h.table.first())
      {
      UStringRep* key;
      UStringRep* value;

      do {
         key   = h.table.key();
         value = h.table.elem();

         os.write(key->data(), key->size());
         os.write(": ", 2);
         os.write(value->data(), value->size());
         os.write(U_CONSTANT_TO_PARAM(U_CRLF));
         }
      while (h.table.next());
      }

   os.write(U_CONSTANT_TO_PARAM(U_CRLF));

   return os;
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UMimeHeader::dump(bool reset) const
{
   *UObjectIO::os << "table     (UHashMap " << (void*)&table  << ")\n"
                  << "header    (UString  " << (void*)&header << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
