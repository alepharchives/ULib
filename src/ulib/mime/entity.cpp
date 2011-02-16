// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    entity.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file.h>
#include <ulib/mime/entity.h>
#include <ulib/utility/uhttp.h>
#include <ulib/utility/base64.h>
#include <ulib/utility/quoted_printable.h>

// gcc - call is unlikely and code size would grow

UMimeEntity::UMimeEntity() : content(U_CAPACITY)
{
   U_TRACE_REGISTER_OBJECT(0, UMimeEntity, "")

   header      = U_NEW(UMimeHeader);
   startHeader = endHeader = 0;
}

UMimeEntity::UMimeEntity(const UString& _data, uint32_t _startHeader) : data(_data)
{
   U_TRACE_REGISTER_OBJECT(0, UMimeEntity, "%.*S,%u", U_STRING_TO_TRACE(_data), _startHeader)

   header      = U_NEW(UMimeHeader);
   startHeader = _startHeader;

   if (parse()) decodeBody();
}

bool UMimeEntity::isXML() const                { return UMimeHeader::isXML(content_type); }
bool UMimeEntity::isText() const               { return UMimeHeader::isText(content_type); }
bool UMimeEntity::isPKCS7() const              { return UMimeHeader::isPKCS7(content_type); }
bool UMimeEntity::isRFC822() const             { return UMimeHeader::isRFC822(content_type); }
bool UMimeEntity::isMessage() const            { return UMimeHeader::isMessage(content_type); }
bool UMimeEntity::isURLEncoded() const         { return UMimeHeader::isURLEncoded(content_type); }
bool UMimeEntity::isApplication() const        { return UMimeHeader::isApplication(content_type); }
bool UMimeEntity::isMultipartFormData() const  { return UMimeHeader::isMultipartFormData(content_type); }

bool UMimeEntity::isMultipart() const
{
   U_TRACE(0, "UMimeEntity::isMultipart()")

   bool result = UMimeHeader::isContentType(content_type, U_CONSTANT_TO_PARAM("multipart"), true); // NB: ignore case - kmail use "Multipart"

   U_RETURN(result);
}

bool UMimeEntity::parse()
{
   U_TRACE(0, "UMimeEntity::parse()")

   U_ASSERT(header->empty())
   U_ASSERT(content.empty())
   U_ASSERT(data.empty() == false)

   /*
   uint32_t skip,
            end1 = U_STRING_FIND(data, 0, U_LF2),
            end2 = U_STRING_FIND(data, 0, U_CRLF);

   if (end1 < end2)
      {
      skip      = 2;
      endHeader = end1;
      }
   else
      {
      skip      = 4;
      endHeader = end2;
      }

   parse_result = (endHeader != U_NOT_FOUND &&
                  (endHeader += skip, header->parse(data.substr(startHeader, endHeader)) > 0));
   */

   UString h;
   const char* str = data.data();

   endHeader = u_findEndHeader(str, data.size());

   if (endHeader != U_NOT_FOUND &&
       (h = data.substr(startHeader, endHeader), header->parse(h)) > 0)
      {
      parse_result = checkContentType();
      }
   else
      {
      parse_result = false;

      const char* ptr  = str;
      const char* _end = data.rep->end();

      while ((ptr < _end) && u_isspace(*ptr)) ++ptr;

      endHeader = ptr - str;
      }

   U_INTERNAL_DUMP("endHeader = %u", endHeader)

   U_RETURN(parse_result);
}

void UMimeEntity::decodeBody()
{
   U_TRACE(0, "UMimeEntity::decodeBody()")

   if (content.empty()) content = getBody();

   uint32_t length = content.size();

   // Content transfer encoding: [ "base64", "quoted-printable", "7bit", "8bit", "binary" ]

   UString tfrEncoding = header->getHeader(*UMimeHeader::str_content_transfer_encoding);

   if (tfrEncoding.empty() == false)
      {
      const char* ptr = tfrEncoding.data();

      if (U_STRNCASECMP(ptr, "base64") == 0)
         {
         UString buffer(length);

         if (UBase64::decode(content.data(), length, buffer))
            {
            content = buffer;

            return;
            }
         }
      else if (U_STRNCASECMP(ptr, "quoted-printable") == 0)
         {
         UString buffer(length);

         if (UQuotedPrintable::decode(content.data(), length, buffer))
            {
            content = buffer;

            return;
            }
         }
   // else if (U_STRNCASECMP(ptr, "binary") == 0) return;
      }
}

// read with socket

bool UMimeEntity::readHeader(USocket* socket)
{
   U_TRACE(0, "UMimeEntity::readHeader(%p)", socket)

   U_ASSERT(header->empty())

   if (header->readHeader(socket, data))
      {
      endHeader   = UHTTP::http_info.endHeader;
      startHeader = UHTTP::http_info.startHeader;

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool UMimeEntity::readBody(USocket* socket)
{
   U_TRACE(0, "UMimeEntity::readBody(%p)", socket)

   U_ASSERT(content.empty())
   U_INTERNAL_ASSERT_DIFFERS(endHeader, U_NOT_FOUND)
   U_INTERNAL_ASSERT_EQUALS(endHeader, UHTTP::http_info.endHeader)

   UHTTP::http_info.clength = header->getHeader(*USocket::str_content_length).strtol();

   if (UHTTP::readHTTPBody(socket, &data, content) &&
       checkContentType())
      {
      decodeBody();

      U_RETURN(true);
      }

   U_RETURN(false);
}

void UMimeEntity::setEmpty()
{
   U_TRACE(0, "UMimeEntity::setEmpty()")

   data.setEmpty();

        content.clear();
   content_type.clear();

   if (header) header->clear();
}

// checks whether [current,end[ matches -*[\r\t ]*(\n|$)

inline bool UMimeMultipart::isOnlyWhiteSpaceOrDashesUntilEndOfLine(const char* current, const char* _end)
{
   U_TRACE(0, "UMimeMultipart::isOnlyWhiteSpaceOrDashesUntilEndOfLine(%S,%p)", current, _end)

   bool dashesStillAllowed = true;

   while (current < _end)
      {
      switch (*current)
         {
         case ' ':
         case '\t':
         case '\r':
            {
            dashesStillAllowed = false;

            ++current;

            continue;
            }

         case '\n': U_RETURN(true);

         case '-':
            {
            if (dashesStillAllowed == false) U_RETURN(false);

            ++current;

            continue;
            }

         default: U_RETURN(false);
         }
      }

   // end of buffer is ok, too:

   U_RETURN(true);
}

U_NO_EXPORT bool UMimeMultipart::findBoundary(uint32_t pos)
{
   U_TRACE(0, "UMimeMultipart::findBoundary(%u)", pos)

   // Search for the first boundary.
   // The leading CR LF ('\n') is part of the boundary, but if there is no preamble, there may be no leading CR LF ('\n').
   // The case of no leading CR LF ('\n') is a special case that will occur only when '-' is the first character of the body

   if (buf[pos]   == '-' &&
       buf[pos+1] == '-' &&
       ((pos+blen+1) < endPos) &&
       (memcmp(&buf[pos+2], bbuf, blen) == 0))
      {
      boundaryStart = pos;

      pos += blen + 2;

      // Check for final boundary

      if (pos+1 < endPos    &&
          buf[pos]   == '-' &&
          buf[pos+1] == '-')
         {
         pos += 2;

         isFinal = true;
         }
      else
         {
         isFinal = false;
         }

      // Advance position past end of line

      while (pos < endPos)
         {
         if (buf[pos] == '\n')
            {
            ++pos;

            break;
            }

         ++pos;
         }

      boundaryEnd = pos;

      U_RETURN(true);
      }

   bool isFound = false;

   while ((pos+blen+2) < endPos)
      {
      // Case of leading LF

      if (buf[pos]   == '\n' &&
          buf[pos+1] == '-'  &&
          buf[pos+2] == '-'  &&
          (memcmp(&buf[pos+3], bbuf, blen) == 0) &&
          isOnlyWhiteSpaceOrDashesUntilEndOfLine(buf+pos+blen+3, buf+endPos))
         {
         boundaryStart = pos;

         pos += blen + 3;

         isFound = true;
         }

      // Case of leading CR LF

      else if (buf[pos]   == '\r' &&
               buf[pos+1] == '\n' &&
               buf[pos+2] == '-'  &&
               ((pos+blen+3) < endPos) &&
               buf[pos+3] == '-' &&
               (memcmp(&buf[pos+4], bbuf, blen) == 0) &&
               isOnlyWhiteSpaceOrDashesUntilEndOfLine(buf+pos+blen+4, buf+endPos))
         {
         boundaryStart = pos;

         pos += blen + 4;

         isFound = true;
         }

      if (isFound)
         {
         // Check for final boundary

         if (pos < endPos && buf[pos] == '-')
            {
            // NOTE: Since we must be fault tolerant for being able to understand messaged that were damaged during
            // transportation we now accept final boundaries ending with "-" instead of "--".

            ++pos;

            isFinal = true;

            // if there *is* the 2nd '-' we of course process it

            if (((pos+1) < endPos) && buf[pos+1] == '-') ++pos;
            }
         else
            {
            isFinal = false;
            }

         // Advance position past end of line

         while (pos < endPos)
            {
            if (buf[pos] == '\n')
               {
               ++pos;

               break;
               }

            ++pos;
            }

         boundaryEnd = pos;

         U_RETURN(true);
         }

      ++pos;
      }

   // Exceptional case: no boundary found

   boundaryStart = boundaryEnd = endPos;

   isFinal = true;

   U_RETURN(false);
}

bool UMimeMultipart::parse()
{
   U_TRACE(0, "UMimeMultipart::parse()")

   U_ASSERT( content.empty() == false)
   U_ASSERT(boundary.empty() == false)

   // Assume the starting position is the beginning of a line

   buf    =  content.data();
   bbuf   = boundary.data();
   blen   = boundary.size();
   endPos =  content.size();

   // Find the preamble

   if (findBoundary(0) == false)
      {
      U_INTERNAL_DUMP("boundaryStart = %u boundaryEnd = %u isFinal = %b", boundaryStart, boundaryEnd, isFinal)

      U_RETURN(parse_result = false);
      }

   U_INTERNAL_DUMP("boundaryStart = %u boundaryEnd = %u isFinal = %b", boundaryStart, boundaryEnd, isFinal)

   if (boundaryStart > 0) preamble = content.substr(0U, (uint32_t)boundaryStart);

   // Find the body parts

   UString part;
   UMimeEntity* item;
   uint32_t pos = boundaryEnd, len;
   bool result, digest = isDigest();

   while (isFinal == false)
      {
      result = findBoundary(pos);

      U_INTERNAL_DUMP("boundaryStart = %u boundaryEnd = %u isFinal = %b", boundaryStart, boundaryEnd, isFinal)

      // NOTE: For enhanced fault tolerance we *accept* a missing last boundary.
      // If no last boundary is found (but at leat a first one was there) we just
      // assume the end of the text ebing the end of the last part.
      // By doing so we can safely parse some buggy MS Outlook clients' messages.

      len  = (result ? (boundaryStart - pos) : (isFinal = true, endPos - pos));
      part = content.substr(pos, len);
      pos  = (result ? boundaryEnd : endPos);

      item = U_NEW(UMimeEntity(part));

      if (item->isParsingOk() == false)
         {
         if (digest)
            {
            // in a digest, the default Content-Type value for a body part is changed from "text/plain" to "message/rfc822"

            item->content_type = *UMimeHeader::str_msg_rfc;
            }
         else
            {
            // implicitly typed plain ASCII text

            item->content_type = *UMimeHeader::str_txt_plain;
            }

         (void) item->header->setHeaderIfAbsent(*USocket::str_content_type, item->content_type);
         }

      bodypart.push(item);
      }

   // Find the epilogue

   if ((len = (endPos - pos)) > 0) epilogue = content.substr(pos, len); 

   U_INTERNAL_DUMP("getNumBodyPart() = %u", getNumBodyPart())

   U_RETURN(parse_result = true);
}

void UMimeMultipart::clear()
{
   U_TRACE(0, "UMimeMultipart::clear()")

   boundary.clear();
   preamble.clear();
   epilogue.clear();

   bodypart.clear();

   UMimeEntity::clear();
}

// STREAMS

U_EXPORT ostream& operator<<(ostream& os, const UMimeMultipart& ml)
{
   U_TRACE(0+256, "UMimeMultipart::operator<<(%p,%p)", &os, &ml)

   os << *(ml.header);

   UMimeEntity* e;

   for (uint32_t i = 0, n = ml.bodypart.size(); i < n; ++i)
      {
      os.write(U_CONSTANT_TO_PARAM("--"));
      os.write(ml.boundary.data(), ml.boundary.size());
      os.write(U_CONSTANT_TO_PARAM(U_CRLF));

      e = ml.bodypart[i];

      os << *(e->getHeader());
      os.write(e->getBody().data(), e->getBody().size());
      os.write(U_CONSTANT_TO_PARAM(U_CRLF));
      }

   os.write(U_CONSTANT_TO_PARAM("--"));
   os.write(ml.boundary.data(), ml.boundary.size());
   os.write(U_CONSTANT_TO_PARAM("--"));
   os.write(U_CONSTANT_TO_PARAM(U_CRLF));

   return os;
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UMimeEntity::dump(bool reset) const
{
   *UObjectIO::os << "endHeader                 " << endHeader             << '\n'
                  << "startHeader               " << startHeader           << '\n'
                  << "parse_result              " << parse_result          << '\n'
                  << "data         (UString     " << (void*)&data          << ")\n"
                  << "header       (UMimeHeader " << (void*)header         << ")\n"
                  << "content      (UString     " << (void*)&content       << ")\n"
                  << "content_type (UString     " << (void*)&content_type  << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

const char* UMimeMessage::dump(bool reset) const
{
   UMimeEntity::dump(false);

   *UObjectIO::os << '\n'
                  << "rfc822       (UMimeEntity " << (void*)&rfc822 << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

const char* UMimeMultipart::dump(bool reset) const
{
    UMimeEntity::dump(false);

   *UObjectIO::os << '\n'
                  << "isFinal                   " << isFinal          << '\n'
                  << "boundaryEnd               " << boundaryEnd      << '\n'
                  << "boundaryStart             " << boundaryStart    << '\n'
                  << "preamble     (UString     " << (void*)&preamble << ")\n"
                  << "epilogue     (UString     " << (void*)&epilogue << ")\n"
                  << "boundary     (UString     " << (void*)&boundary << ")\n"
                  << "bodypart     (UVector     " << (void*)&bodypart << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
