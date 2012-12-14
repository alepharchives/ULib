// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    header.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_MIMEHEADER_H
#define U_MIMEHEADER_H 1

#include <ulib/net/socket.h>
#include <ulib/container/hash_map.h>

/*
   Mime-type headers comprise a keyword followed by ":" followed by whitespace and the value.
   If a line begins with whitespace it is a continuation of the preceeding line.
   MIME headers are delimited by an empty line.  Well-known header fields are those documented
   in RFC-822 (standard email), RFC-1036 (USENET messages), RFC-2045 (MIME messages), and possibly other RFCs.

   Text&         Subject()                { return (Text&)           FieldBody("Subject"); }
   Text&         Comments()               { return (Text&)           FieldBody("Comments"); }
   Text&         InReplyTo()              { return (Text&)           FieldBody("In-Reply-To"); }
   Text&         Keywords()               { return (Text&)           FieldBody("Keywords"); }
   Text&         Encrypted()              { return (Text&)           FieldBody("Encrypted"); }
   Text&         Received()               { return (Text&)           FieldBody("Received"); }
   Text&         References()             { return (Text&)           FieldBody("References"); }
   MsgId&        MessageId()              { return (MsgId&)          FieldBody("Message-Id"); }
   MsgId&        ResentMessageId()        { return (MsgId&)          FieldBody("Resent-Message-Id"); }
   DateTime&     Date()                   { return (DateTime&)       FieldBody("Date"); }
   DateTime&     ResentDate()             { return (DateTime&)       FieldBody("Resent-Date"); }
   Address&      ReturnPath()             { return (Address&)        FieldBody("Return-Path"); }
   AddressList&  Bcc()                    { return (AddressList&)    FieldBody("Bcc"); }
   AddressList&  Cc()                     { return (AddressList&)    FieldBody("Cc"); }
   AddressList&  ReplyTo()                { return (AddressList&)    FieldBody("Reply-To"); }
   AddressList&  ResentBcc()              { return (AddressList&)    FieldBody("Resent-Bcc"); }
   AddressList&  ResentCc()               { return (AddressList&)    FieldBody("Resent-Cc"); }
   AddressList&  ResentReplyTo()          { return (AddressList&)    FieldBody("Resent-Reply-To"); }
   AddressList&  ResentTo()               { return (AddressList&)    FieldBody("Resent-To"); }
   AddressList&  To()                     { return (AddressList&)    FieldBody("To"); }
   Mailbox&      ResentSender()           { return (Mailbox&)        FieldBody("Resent-Sender"); }
   Mailbox&      Sender()                 { return (Mailbox&)        FieldBody("Sender"); }
   MailboxList&  From()                   { return (MailboxList&)    FieldBody("From"); }
   MailboxList&  ResentFrom()             { return (MailboxList&)    FieldBody("Resent-From"); }

   RFC-822 fields

   Text& Approved()                       { return (Text&)           FieldBody("Approved"); }
   Text& Control()                        { return (Text&)           FieldBody("Control"); }
   Text& Distribution()                   { return (Text&)           FieldBody("Distribution"); }
   Text& Expires()                        { return (Text&)           FieldBody("Expires"); }
   Text& FollowupTo()                     { return (Text&)           FieldBody("Followup-To"); }
   Text& Lines()                          { return (Text&)           FieldBody("Lines"); }
   Text& Newsgroups()                     { return (Text&)           FieldBody("Newsgroups"); }
   Text& Organization()                   { return (Text&)           FieldBody("Organization"); }
   Text& Path()                           { return (Text&)           FieldBody("Path"); }
   Text& Summary()                        { return (Text&)           FieldBody("Summary"); }
   Text& Xref()                           { return (Text&)           FieldBody("Xref"); }

   RFC-1036 fields (USENET messages)

   Text&       MimeVersion()              { return (Text&)           FieldBody("MIME-Version"); }
   Text&       ContentDescription()       { return (Text&)           FieldBody("Content-Description"); }
   MsgId&      ContentId()                { return (MsgId&)          FieldBody("Content-Id"); }
   Mechanism&  ContentTransferEncoding()  { return (Mechanism&)      FieldBody("Content-Transfer-Encoding"); }
   MediaType&  ContentType()              { return (MediaType&)      FieldBody("Content-Type"); }

   RFC-2045 fields

   DispositionType& ContentDisposition()  { return (DispositionType&) FieldBody("Content-Disposition"); }
*/

class UHTTP;

class U_EXPORT UMimeHeader {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   static const UString* str_name;
   static const UString* str_ascii;
   static const UString* str_charset;
   static const UString* str_boundary;
   static const UString* str_filename;
   static const UString* str_txt_xml;
   static const UString* str_txt_plain;
   static const UString* str_msg_rfc;
   static const UString* str_mime_version;
   static const UString* str_content_transfer_encoding;

   static void str_allocate();

   // COSTRUTTORI

   UMimeHeader() : table(false) // ignore case...
      {
      U_TRACE_REGISTER_OBJECT(0, UMimeHeader, "")

      table.allocate();

      if (         str_name == 0)          str_allocate();
      if (USocket::str_host == 0) USocket::str_allocate();
      }

   ~UMimeHeader()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UMimeHeader)

      table.clear();
      table.deallocate();
      }

   // Wrapper for table

   void clear()                  {        table.clear(); header.clear(); }
   void deallocate()             {        table.deallocate(); }
   bool empty() const            { return table.empty(); }
   bool ignoreCase() const       { return table.ignoreCase(); }
   void setIgnoreCase(bool flag) {        table.setIgnoreCase(flag); }

   UString      erase(const UString& key) { return table.erase(key); }
   UString operator[](const UString& key) { return table[key]; }

   // VARIE

   uint32_t parse(const UString& buffer);
   uint32_t parse(const char* ptr, uint32_t n);

   void   removeHeader(const UString& key);
   bool containsHeader(const UString& key) { return table.find(key); }
   UString   getHeader(const UString& key) { return table[key]; }

   // Sets a header field, overwriting any existing value

   void setHeader(const UString& key, const UString& value)
      {
      U_TRACE(0, "UMimeHeader::setHeader(%.*S,%.*S)", U_STRING_TO_TRACE(key), U_STRING_TO_TRACE(value))

      if (containsHeader(key)) table.replaceAfterFind(value);
      else                     table.insertAfterFind(key, value);
      }

   bool setHeaderIfAbsent(const UString& key, const UString& value)
      {
      U_TRACE(0, "UMimeHeader::setHeaderIfAbsent(%.*S,%.*S)", U_STRING_TO_TRACE(key), U_STRING_TO_TRACE(value))

      if (containsHeader(key) == false)
         {
         table.insertAfterFind(key, value);

         U_RETURN(true);
         }

      U_RETURN(false);
      }

   // Writes all the headers to the supplied buffer

   UString getHeaders();
   void    writeHeaders(UString& buffer);

   // Host

   UString getHost()
      {
      U_TRACE(0, "UMimeHeader::getHost()")

      U_ASSERT(empty() == false)

      UString host = getHeader(*USocket::str_host);

      U_RETURN_STRING(host);
      }

   // Connection: close

   bool isClose()
      {
      U_TRACE(0, "UMimeHeader::isClose()")

      U_ASSERT(empty() == false)

      bool result = (getHeader(*USocket::str_connection) == *USocket::str_close);

      U_RETURN(result);
      }

   // Transfer-Encoding: chunked

   bool isChunked()
      {
      U_TRACE(0, "UMimeHeader::isChunked()")

      U_ASSERT(empty() == false)

      bool result = (getHeader(*USocket::str_Transfer_Encoding) == *USocket::str_chunked);

      U_RETURN(result);
      }

   // Cookie

   UString getCookie()
      {
      U_TRACE(0, "UMimeHeader::getCookie()")

      U_ASSERT(empty() == false)

      UString cookie = getHeader(*USocket::str_cookie);

      U_RETURN_STRING(cookie);
      }

   // Set-Cookie

   bool isSetCookie()
      {
      U_TRACE(0, "UMimeHeader::isSetCookie()")

      U_ASSERT(empty() == false)

      bool result = containsHeader(*USocket::str_setcookie);

      U_RETURN(result);
      }

   // Location

   UString getLocation()
      {
      U_TRACE(0, "UMimeHeader::getLocation()")

      U_ASSERT(empty() == false)

      UString location = getHeader(*USocket::str_location);

      U_RETURN_STRING(location);
      }

   // Refresh

   UString getRefresh()
      {
      U_TRACE(0, "UMimeHeader::getRefresh()")

      U_ASSERT(empty() == false)

      UString refresh = getHeader(*USocket::str_refresh);

      U_RETURN_STRING(refresh);
      }

   // Mime

   UString getMimeVersion()
      {
      U_TRACE(0, "UMimeHeader::getMimeVersion()")

      U_ASSERT(empty() == false)

      UString value = getHeader(*str_mime_version);

      U_RETURN_STRING(value);
      }

   bool isMime()
      {
      U_TRACE(0, "UMimeHeader::isMime()")

      bool result = ((table.empty()            == false) &&
                     (getMimeVersion().empty() == false));

      U_RETURN(result);
      }

   // Content types: "multipart"   / [ "mixed", "alternative", "digest", "parallel", "signed", "encrypted", "report", "form-data" ],
   //                "message"     / [ "rfc822", "disposition-notification" ],
   //                "image"       / [ "jpeg", "gif" ],
   //                "audio"       / [ "basic" ],
   //                "video"       / [ "mpeg" ],
   //                "application" / [ "postscript", "octet-stream", "pgp-signature", "pgp-encrypted", "pgp-clearsigned",
   //                                  "pkcs7-signature", "pkcs7-mime", "ms-tnef", "x-www-form-urlencoded" ]
   //                "text"        / [ "plain" (RFC-1521), "richtext" (RFC-1341), "enriched", "html", "xvcard", "vcal",
   //                                  "rtf", "xml" ],

   UString getContentType()
      {
      U_TRACE(0, "UMimeHeader::getContentType()")

      U_ASSERT(empty() == false)

      UString content_type = getHeader(*USocket::str_content_type);

      U_RETURN_STRING(content_type);
      }

   static bool isContentType(const UString& content_type, const char* type, uint32_t len, bool ignore_case = false)
      {
      U_TRACE(0, "UMimeHeader::isContentType(%.*S,%.*S,%u,%b)", U_STRING_TO_TRACE(content_type), len, type, len, ignore_case)

      if (content_type.empty() == false)
         {
         bool result = (ignore_case ? strncasecmp(content_type.data(), type, len)
                                    :     strncmp(content_type.data(), type, len)) == 0;

         U_RETURN(result);
         }

      U_RETURN(false);
      }

   static bool isType(const UString& content_type, const char* type, uint32_t len)
      {
      U_TRACE(0, "UMimeHeader::isType(%.*S,%.*S,%u)", U_STRING_TO_TRACE(content_type), len, type, len)

      if (content_type.empty() == false)
         {
         uint32_t pos = content_type.find('/');

         if (pos                               != U_NOT_FOUND &&
             content_type.find(type, pos, len) != U_NOT_FOUND)
            {
            U_RETURN(true);
            }
         }

      U_RETURN(false);
      }

   static UString getCharSet(const UString& content_type);        // get charset/content-type info
   static UString shortContentType(const UString& content_type);

   // VARIE

   static bool isMessage(const UString& content_type)
      { return isContentType(content_type, U_CONSTANT_TO_PARAM("message")); }

   static bool isMultipart(const UString& content_type)
      { return isContentType(content_type, U_CONSTANT_TO_PARAM("multipart")); }

   static bool isApplication(const UString& content_type)
      { return isContentType(content_type, U_CONSTANT_TO_PARAM("application")); }

   static bool isXML(const UString& content_type)
      { return isContentType(content_type, U_STRING_TO_PARAM(*str_txt_xml)); }

   static bool isText(const UString& content_type)
      { return isContentType(content_type, U_STRING_TO_PARAM(*str_txt_plain)); }

   static bool isRFC822(const UString& content_type)
      { return isContentType(content_type, U_STRING_TO_PARAM(*str_msg_rfc)); }

   static bool isPKCS7(const UString& ctype)             { return isType(ctype, U_CONSTANT_TO_PARAM("pkcs7")); }
   static bool isURLEncoded(const UString& ctype)        { return isType(ctype, U_CONSTANT_TO_PARAM("urlencoded")); }
   static bool isMultipartFormData(const UString& ctype) { return isContentType(ctype, U_CONSTANT_TO_PARAM("multipart/form-data")); }

   // VARIE

   static UString getBoundary(const UString& content_type)
      {
      U_TRACE(0, "UMimeHeader::getBoundary(%.*S)", U_STRING_TO_TRACE(content_type))

      UString boundary = getValueAttributeFromKeyValue(content_type, *str_boundary, false);

      U_RETURN_STRING(boundary);
      }

   UString getValueAttributeFromKey(const UString& key, const UString& name)
      {
      U_TRACE(0, "UMimeHeader::getValueAttributeFromKey(%.*S,%.*S)", U_STRING_TO_TRACE(key), U_STRING_TO_TRACE(name))

      U_ASSERT(empty() == false)

      UString value = getHeader(key);

      if (value.empty() == false) value = getValueAttributeFromKeyValue(value, name, false);

      U_RETURN_STRING(value);
      }

   static uint32_t      getAttributeFromKeyValue(const UString& key_value, UVector<UString>& name_value);
   static UString  getValueAttributeFromKeyValue(const UString& name_attr, UVector<UString>& name_value, bool ignore_case);
   static UString  getValueAttributeFromKeyValue(const UString& key_value, const UString& name_attr, bool ignore_case);

   // Disposition type (Content-Disposition header field, see RFC-1806): "inline", "attachment"

   UString getContentDisposition()
      {
      U_TRACE(0, "UMimeHeader::getContentDisposition()")

      U_ASSERT(empty() == false)

      UString content_disposition = getHeader(*USocket::str_content_disposition);

      U_RETURN_STRING(content_disposition);
      }

   static bool getNames(const UString& cdisposition, UString& name, UString& filename);

   static UString getFileName(const UString& cdisposition) { return getValueAttributeFromKeyValue(cdisposition, *str_filename, false); }

   // read from socket

   bool readHeader(USocket* socket, UString& data);

   // STREAM

   friend U_EXPORT ostream& operator<<(ostream& os, UMimeHeader& h);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UString header;
   UHashMap<UString> table;

private:
   UMimeHeader(const UMimeHeader&)            {}
   UMimeHeader& operator=(const UMimeHeader&) { return *this; }

   friend class UHTTP;
};

#endif
