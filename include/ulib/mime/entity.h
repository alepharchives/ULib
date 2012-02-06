// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    entity.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MIME_ENTITY_H
#define U_MIME_ENTITY_H 1

#include <ulib/mime/header.h>
#include <ulib/container/vector.h>

// UMimeEntity -- class representing a MIME entity
//
// RFC-2045 defines an entity as either a message or a body part, both of which have a collection of headers and a body
//
// MIME headers comprise a keyword followed by ":" followed by whitespace and the value.
// If a line begins with whitespace it is a continuation of the preceeding line.
// MIME headers are delimited by an empty line
//
// The only reliable way to determine the type of body is to access the Content-Type header field from the
// headers object of the entity that contains it. For this reason, a body should always be part of a entity.
// Only types "multipart" and "message" need to be parsed, and in all content types, the body contains a
// string of characters
//
// If the content type is 'message'   then the body contains an encapsulated message
// If the content type is 'multipart' then the body contains one or more body parts

class UHTTP;
class UMimeMultipart;

class U_EXPORT UMimeEntity {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   UMimeEntity();
   UMimeEntity(const UString& _data, uint32_t startHeader = 0);

   UMimeEntity(UMimeEntity& item) : data(item.data), content_type(item.content_type), content(item.content)
      {
      U_TRACE_REGISTER_OBJECT(0, UMimeEntity, "%p", &item)

      U_INTERNAL_ASSERT_POINTER(item.header)

      endHeader   = item.endHeader;
      startHeader = item.startHeader;

      // passaggio di consegne...

      header      = item.header;
                    item.header = 0;
      }

   ~UMimeEntity()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UMimeEntity)

      if (header) delete header;
      }

   // VARIE

   UMimeHeader* getHeader() const { return header; }

   bool isEmpty()
      {
      U_TRACE(0, "UMimeEntity::isEmpty()")

      bool result = content.empty();

      U_RETURN(result);
      }

   void setEmpty();

   bool parse();
   bool parse(const UString& _data)
      {
      U_TRACE(0, "UMimeEntity::parse(%.*S)", U_STRING_TO_TRACE(_data))

      U_ASSERT(data.empty() == true)

      data = _data;

      (void) parse();

      U_RETURN(parse_result);
      }

   bool isParsingOk()
      {
      U_TRACE(0, "UMimeEntity::isParsingOk()")

      U_INTERNAL_ASSERT_POINTER(header)

      U_RETURN(parse_result);
      }

   UString getData() const        { return data; }
   UString getBody() const        { return data.substr(endHeader); }
   UString getContent() const     { return content; }
   UString getContentType() const { return content_type; }
   UString getMimeVersion() const { return header->getMimeVersion(); }

   // Content types: "multipart"   / [ "mixed", "alternative", "digest", "parallel", "signed", "encrypted", "report", "form-data" ],
   //                "message"     / [ "rfc822", "disposition-notification" ],
   //                "image"       / [ "jpeg", "gif" ],
   //                "audio"       / [ "basic" ],
   //                "video"       / [ "mpeg" ],
   //                "application" / [ "postscript", "octet-stream", "pgp-signature", "pgp-encrypted", "pgp-clearsigned",
   //                                  "pkcs7-signature", "pkcs7-mime", "ms-tnef", "x-www-form-urlencoded" ]
   //                "text"        / [ "plain" (RFC-1521), "richtext" (RFC-1341), "enriched", "html", "xvcard", "vcal",
   //                                  "rtf", "xml" ],

   bool isMime() { return header->isMime(); }

   bool isXML() const __pure;
   bool isText() const __pure;
   bool isPKCS7() const __pure;
   bool isRFC822() const __pure;
   bool isMessage() const __pure;
   bool isMultipart() const __pure;
   bool isURLEncoded() const __pure;
   bool isApplication() const __pure;
   bool isMultipartFormData() const __pure;

   UString getCharSet() const        { return UMimeHeader::getCharSet(content_type); } // get charset/content-type info
   UString shortContentType() const  { return UMimeHeader::shortContentType(content_type); }

   UString getValueAttributeFromKey(const UString& key, const UString& name) const
      { return header->getValueAttributeFromKey(key, name); }

   bool isType(       const char* type, uint32_t len) const { return UMimeHeader::isType(       content_type, type, len); }
   bool isContentType(const char* type, uint32_t len) const { return UMimeHeader::isContentType(content_type, type, len); }

   // Disposition type (Content-Disposition header field, see RFC-1806): "inline", "attachment"

   UString getContentDisposition() const { return header->getContentDisposition(); }

   bool isAttachment() const
      {
      U_TRACE(0, "UMimeEntity::isAttachment()")

      UString value = getContentDisposition();

      bool result = (value.empty()                         == false &&
                     U_STRING_FIND(value, 0, "attachment") != U_NOT_FOUND);

      U_RETURN(result);
      }

   bool isBodyMessage() const
      {
      U_TRACE(0, "UMimeEntity::isBodyMessage()")

      bool result = (isText()                                                                                        &&
                     UMimeHeader::getValueAttributeFromKeyValue(content_type, *UMimeHeader::str_name, false).empty() &&
                     isAttachment() == false);

      U_RETURN(result);
      }

   UString getFileName() const { return UMimeHeader::getFileName(getContentDisposition()); }

   // read with socket

   bool readBody(USocket* socket);
   bool readHeader(USocket* socket);

   // STREAM

   friend ostream& operator<<(ostream& os, const UMimeEntity& e)
      {
      U_TRACE(0+256, "UMimeEntity::operator<<(%p,%p)", &os, &e)

      os << *(e.header);

      os.write(e.content.data(), e.content.size());

      return os;
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UMimeHeader* header;
   uint32_t startHeader, endHeader;
   UString data, content_type, content;
   bool parse_result;

   void clear()
      {
      U_TRACE(0, "UMimeEntity::clear()")

      if (header)
         {
         startHeader = 0;

         delete header;
                header = 0;
         }

      content.clear();
      }

   bool checkContentType()
      {
      U_TRACE(0, "UMimeEntity::checkContentType()")

      U_INTERNAL_ASSERT_POINTER(header)

      content_type = header->getContentType();

      bool result = (content_type.empty() == false);

      U_RETURN(result);
      }

   void decodeBody();

private:
   UMimeEntity& operator=(const UMimeEntity&) { return *this; }

   friend class UHTTP;
   friend class UMimeMultipart;
};

// If the content type is 'message' then the body contains an encapsulated message.

class U_EXPORT UMimeMessage : public UMimeEntity {
public:

   // COSTRUTTORI

   UMimeMessage(UMimeEntity& item) : UMimeEntity(item), rfc822(content)
      {
      U_TRACE_REGISTER_OBJECT(0, UMimeMessage, "%p", &item)

      U_ASSERT(UMimeEntity::isMessage())
      }

   UMimeMessage(const UString& _data) : UMimeEntity(_data), rfc822(content)
      {
      U_TRACE_REGISTER_OBJECT(0, UMimeMessage, "%.*S", U_STRING_TO_TRACE(_data))
      }

   ~UMimeMessage()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UMimeMessage)
      }

   // VARIE

   UMimeEntity& getRFC822() { return rfc822; }

   // STREAM

   friend ostream& operator<<(ostream& os, const UMimeMessage& m)
      {
      U_TRACE(0+256, "UMimeMessage::operator<<(%p,%p)", &os, &m)

      os << *(m.header) << m.rfc822;

      return os;
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UMimeEntity rfc822;

private:
   UMimeMessage(const UMimeMessage&) : UMimeEntity() {}
   UMimeMessage& operator=(const UMimeMessage&)      { return *this; }
};

// If the content type is 'multipart' then the body contains one or more body parts.

class U_EXPORT UMimeMultipart : public UMimeEntity {
public:

   // COSTRUTTORI

   UMimeMultipart() : UMimeEntity()
      {
      U_TRACE_REGISTER_OBJECT(0, UMimeMultipart, "")
      }

   UMimeMultipart(UMimeEntity& item) : UMimeEntity(item)
      {
      U_TRACE_REGISTER_OBJECT(0, UMimeMultipart, "%p", &item)

      U_ASSERT(UMimeEntity::isMultipart())

      init();
      }

   UMimeMultipart(const UString& _data) : UMimeEntity(_data)
      {
      U_TRACE_REGISTER_OBJECT(0, UMimeMultipart, "%.*S", U_STRING_TO_TRACE(_data))

      init();
      }

   ~UMimeMultipart()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UMimeMultipart)
      }

   // VARIE

   bool parse();
   void clear();

   void setContent(const UString& _content)
      {
      U_TRACE(0, "UMimeMultipart::setContent(%.*S)", U_STRING_TO_TRACE(_content))

      U_ASSERT(_content.empty() == false)

      UMimeEntity::content = _content;
      }

   void setBoundary(const UString& _boundary)
      {
      U_TRACE(0, "UMimeMultipart::setBoundary(%.*S)", U_STRING_TO_TRACE(_boundary))

      U_ASSERT(_boundary.empty() == false)

      boundary = _boundary;
      }

   UString getBoundary() const { return boundary; }
   UString getPreamble() const { return preamble; } 
   UString getEpilogue() const { return epilogue; }

   bool isDigest() const { return UMimeEntity::isType(U_CONSTANT_TO_PARAM("digest")); }

   // manage parts

   uint32_t getNumBodyPart() const             { return bodypart.size(); }

   UMimeEntity* operator[](uint32_t pos) const { return bodypart.at(pos); }

   UVector<UMimeEntity*>& getBodyPart() { return bodypart; }

   // STREAM

   friend U_EXPORT ostream& operator<<(ostream& os, const UMimeMultipart& ml);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   const char* buf;
   const char* bbuf;
   uint32_t blen, endPos, boundaryStart, boundaryEnd;
   UString boundary, preamble, epilogue;
   UVector<UMimeEntity*> bodypart;
   bool isFinal;

   void init()
      {
      U_TRACE(0, "UMimeMultipart::init()")

      boundary = UMimeHeader::getBoundary(content_type);

      (void) parse();
      }

                 bool findBoundary(uint32_t pos) U_NO_EXPORT;
   static inline bool isOnlyWhiteSpaceOrDashesUntilEndOfLine(const char* current, const char* end) U_NO_EXPORT;

private:
   UMimeMultipart(const UMimeMultipart&) : UMimeEntity() {}
   UMimeMultipart& operator=(const UMimeMultipart&)      { return *this; }
};

#endif
