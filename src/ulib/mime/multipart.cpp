// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    multipart.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/mime/multipart.h>
#include <ulib/utility/base64.h>
#include <ulib/utility/quoted_printable.h>

#ifdef USE_LIBMAGIC
#  include <ulib/magic/magic.h>
#endif

#ifndef   RFC2045MIMEMSG
#  define RFC2045MIMEMSG "This is a MIME-formatted message. If you see this text it means that your\n" \
                         "E-mail software does not support MIME-formatted messages.\n"
#endif

#ifndef   RFC2231DOENCODE
#  define RFC2231DOENCODE(c) (strchr("()'\"\\%:;=", (c)) || (c) <= ' ' || (c) >= 127)
#endif

uint32_t    UMimeMultipartMsg::counter;
const char* UMimeMultipartMsg::str_encoding[4] = {
   "7bit",
   "8bit",
   "quoted-printable",
   "base64"
};

inline char* UMimeMultipartMsg::mkboundary()
{
   U_TRACE(0, "UMimeMultipartMsg::mkboundary()")

   boundary_len = u__snprintf(boundary, sizeof(boundary), "%s--=_%u_%4D_%P", u_line_terminator, ++counter);

   char* ptr = boundary + u_line_terminator_len + 2;

   U_RETURN(ptr);
}

UMimeMultipartMsg::UMimeMultipartMsg(const char* type, Encoding encoding, const char* header, bool bRFC2045MIMEMSG)
{
   U_TRACE_REGISTER_OBJECT(0, UMimeMultipartMsg, "%S,%d,%S,%b", type, encoding, header, bRFC2045MIMEMSG)

   U_INTERNAL_ASSERT_POINTER(type)
   U_INTERNAL_ASSERT_POINTER(header)

   char* ptr = mkboundary();

   UString buffer(4000U);

   buffer.snprintf("%s%s"                                            // MIME-Version: 1.0
                   "Content-Type: multipart/%s; boundary=\"%s\"%s"
                   "%s%s%s"                                          // Content-Transfer-Encoding: ...
                   "%s"                                              // \r\n
                   "%s",                                             // RFC2045MIMEMSG
                   (*header ? header            : ""),
                   (*header ? u_line_terminator : ""),
                   type, ptr, u_line_terminator,
                   (encoding == NONE ? "" : "Content-Transfer-Encoding: "),
                   (encoding == NONE ? "" : str_encoding[encoding - 1]),
                   (encoding == NONE ? "" : u_line_terminator),
                   u_line_terminator,
                   (bRFC2045MIMEMSG ? RFC2045MIMEMSG : ""));

   vec_part.push(buffer);
}

uint32_t UMimeMultipartMsg::message(UString& body)
{
   U_TRACE(0, "UMimeMultipartMsg::message(%.*S)", U_STRING_TO_TRACE(body))

   U_ASSERT_MAJOR(vec_part.size(),1)

   char _buf[64];
   uint32_t content_length = vec_part[0].size(),
            len = u__snprintf(_buf, sizeof(_buf), "%s%s", boundary, u_line_terminator);

   body = vec_part.join(_buf, len);

   len = u__snprintf(_buf, sizeof(_buf), "%s--%s", boundary, u_line_terminator);

   (void) body.append(_buf, len);

   content_length = body.size() - content_length;

   U_RETURN(content_length);
}

// Determine encoding as follows:
// -------------------------------------------------------------
// Default to 7bit.
// Use 8bit if high-ascii bytes found.
// Use quoted printable if lines more than 200 characters found.
// Use base64 if a null byte is found
// -------------------------------------------------------------

inline int UMimeMultipartMsg::encodeAutodetect(const UString& content, const char* charset)
{
   U_TRACE(0, "UMimeMultipartMsg::encodeAutodetect(%.*S,%S)", U_STRING_TO_TRACE(content), charset)

   U_INTERNAL_ASSERT_POINTER(charset)

   int l = 0;
   unsigned char ch;
   bool longline = false;
   Encoding encoding = BIT7;
   unsigned char* ptr  = (unsigned char*) content.data();
   unsigned char* _end = (unsigned char*) content.rep->end();

   while (ptr < _end)
      {
      ch = *ptr++;

      if (ch >= 0x80) encoding = BIT8;

      if (ch  < 0x20 &&
          ch != '\t' &&
          ch != '\r' &&
          ch != '\n')
         {
         if (*charset) encoding = QUOTED_PRINTABLE;
         }

      if (ch == 0) U_RETURN(BASE64);

      if      (ch == '\n') l = 0;
      else if (++l > 200)
         {
         longline = true;

         if (*charset) encoding = QUOTED_PRINTABLE;
         }
      }

   if (longline) encoding = (*charset ? QUOTED_PRINTABLE : BASE64);

   U_RETURN(encoding);
}

// Creating a single MIME section
// -----------------------------------------------------------------------------------------------------
// The type option encodes content appropriately, adds the "Content-Type: type" and
// "Content-Transfer-Encoding:" and MIME headers. type can be any valid MIME type, except for multipart
// The encoding option should be specified. It's more efficient to do so
// The charset option sets the MIME charset attribute for text/plain content
// The name option sets the name attribute for Content-Type:
// Additional headers are specified by the header option, doesn't do anything with them except to insert
// the headers into the generated MIME section
// -----------------------------------------------------------------------------------------------------

UString UMimeMultipartMsg::section(const UString& content,
                                   const char* type, Encoding encoding,
                                   const char* charset, const char* name, const char* header)
{
   U_TRACE(0, "UMimeMultipartMsg::section(%.*S,%S,%d,%S,%S,%S)", U_STRING_TO_TRACE(content), type, encoding, charset, name, header)

   U_INTERNAL_ASSERT_POINTER(name)
   U_INTERNAL_ASSERT_POINTER(type)
   U_INTERNAL_ASSERT_POINTER(header)
   U_INTERNAL_ASSERT_POINTER(charset)
   U_INTERNAL_ASSERT_EQUALS(strstr(type,"multipart"),0)

   if (encoding == AUTO) encoding = (Encoding) encodeAutodetect(content, charset);

   if (*type == 0)
      {
#  ifdef USE_LIBMAGIC
      if (UMagic::magic == 0) (void) UMagic::init();

      charset = "";
      type    = UMagic::getType(content).data();
#  else
      type = (encoding == BASE64 ? (charset = "", "application/octet-stream") : "text/plain");
#  endif
      }

   uint32_t length = content.size();

   U_INTERNAL_ASSERT_MAJOR(length,0)

   UString buffer(4000U);

   buffer.snprintf("%s%s"
                   "Content-Type: %s",
                   header, (*header ? u_line_terminator : ""),
                   type);

   if (*charset) buffer.snprintf_add("; charset=\"%s\"", charset);
   if (*name)    buffer.snprintf_add("; name=\"%s\"", name);

   buffer.snprintf_add("%s"      // \r\n
                       "%s%s%s"  // Content-Transfer-Encoding: ...
                       "%s",     // \r\n
                       u_line_terminator,
                       (encoding == NONE ? "" : "Content-Transfer-Encoding: "),
                       (encoding == NONE ? "" : str_encoding[encoding - 1]),
                       (encoding == NONE ? "" : u_line_terminator),
                       u_line_terminator);

   if (encoding == BASE64 ||
       encoding == QUOTED_PRINTABLE)
      {
      UString tmp(length * 4);

      const unsigned char* ptr = (const unsigned char*)content.data();

      if (encoding == BASE64)
         {
         u_base64_max_columns = U_OPENSSL_BASE64_MAX_COLUMN;

         UBase64::encode(ptr, length, tmp);

         u_base64_max_columns = 0;
         }
      else
         {
         UQuotedPrintable::encode(ptr, length, tmp);
         }

      buffer += tmp;
      }
   else
      {
      buffer += content;
      }

   U_RETURN_STRING(buffer);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UMimeMultipartMsg::dump(bool reset) const
{
   *UObjectIO::os << "boundary              " << '"' << boundary         << "\"\n"
                  << "boundary_len          " << boundary_len            << '\n'
                  << "vec_part     (UVector " << (void*)&vec_part        << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
