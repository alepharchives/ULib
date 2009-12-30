// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    url.h - Set and get the parts of an URL
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_URL_H
#define ULIB_URL_H 1

#include <ulib/string.h>
#include <ulib/base/coder/url.h>

// URI: <scheme>:<scheme-specific-part>
// URL: <scheme>://<user>:<password>@<host>:<port>/<url-path>

// ------------------------------------------
// Scheme_Name Description
// ------------------------------------------
// ftp         File Transfer Protocol
// http        Hypertext Transfer Protocol
// news        USENET news
// nntp        USENET news using NNTP access
// wais        Wide Area Information Servers
// gopher      The Gopher Protocol
// mailto      Electronic mail address
// telnet      Reference to interactive sessions
// prospero    Prospero Directory Service
// ------------------------------------------

// URN: "urn:" <NID> ":" <NSS>
// NID specifies the Namespace ID and NSS specifies the Namespace Specific String.
// URN does not resolve to a unique, physical location. URNs serve as persistent resource identifiers.

/**
   @class Url - proto://[user[:password]@]hostname[:port]/[path]

   Represents a <em>Uniform Resource Locator</em> (URL). This class provides the capability to parse
   and manipulate URL strings.

   <h4>The URL Format</h4>
   A URL is a string representation of a resource that is available via the Internet. The format of URLs
   is formally defined in the IETF RFC 1738 which is available online at http://www.ietf.org/rfc/rfc1738.txt
   (which is itself a URL!) The URL syntax is dependent upon the scheme. In general, absolute URL are written as follows:
   @c <scheme>:<scheme-specific-part>
   A URL contains the name of the scheme being used (<scheme>) followed by a colon and then a string
   (the <scheme-specific-part>) whose interpretation depends on the scheme. The URL syntax does not
   require that the scheme-specific-part is common among all URL, however, many forms of URL do share
   a common syntax for representing hierarchical relationships. This "generic URL" syntax consists of
   a sequence of four main components:
   @c <scheme>://<authority><path>?<query>
   The @b scheme is often the name of a network protocol which can be used to retrieve the resource
   from the Internet. The words @a protocol and @a scheme are used interchangeably within this document.
   The @b authority is comprised of three sub-components:
   @c <userInfo@><host><:port>
   The @b path is comprised of everything following the authority up to the query part. In contrast to
   the description in RFC 1738, this class includes the "/" separator between the authority part and the
   path as part of the path.  The following examples are supported:

   host
   /path
   host:port/path
   service:/path
   service://host
   service://path
   service://host:port
   [12]user@bogus.example.com
   service://[11]user@bogus.example.com/path
   service://host:port/path?query=1&query=2&query=3
   service://[10]user@bogus.example.com:port/path?query=1&query=2
*/

#define U_URL_TO_PARAM(url) (url).getUrlData(),(url).getUrlDataLen()
#define U_URL_TO_TRACE(url) (url).getUrlDataLen(),(url).getUrlData()

template <class T> class UVector;

class U_EXPORT Url {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   static UString* str_ftp;
   static UString* str_smtp;
   static UString* str_pop3;
   static UString* str_http;
   static UString* str_https;

   /** Constructor of the class.
    *
    * This constructor creates an empty class.
    */

   Url()
      {
      U_TRACE_REGISTER_OBJECT(0, Url, "", 0)

      if (str_ftp == 0) str_allocate();

      service_end = user_begin = user_end = host_begin = host_end = path_begin = path_end = query = -1;
      }

   /** Constructor of the class.
    *
    * This constructor copy the url from the string.
    *
    * @param x Reference to a string with an url
    */

   Url(const UString& x) : url(x)
      {
      U_TRACE_REGISTER_OBJECT(0, Url, "%.*S", U_STRING_TO_TRACE(x))

      if (str_ftp == 0) str_allocate();

      findpos();
      }

   /** Constructor of the class.
    *
    * This constructor set the url from the char buffer. 
    *
    * @param t Pointer to a char buffer with an url
    */

   Url(const char* t, uint32_t tlen) : url(t, tlen)
      {
      U_TRACE_REGISTER_OBJECT(0, Url, "%S,%u", t, tlen)

      if (str_ftp == 0) str_allocate();

      findpos();
      }

   /** Destructor of the class.
   */

   ~Url()
      {
      U_TRACE_UNREGISTER_OBJECT(0, Url)
      }

   // ASSEGNAZIONI

   void set(const Url& u)
      {
      service_end = u.service_end;
      user_begin  = u.user_begin;
      user_end    = u.user_end;
      host_begin  = u.host_begin;
      host_end    = u.host_end;
      path_begin  = u.path_begin;
      path_end    = u.path_end;
      query       = u.query;
      }

   Url(const Url& u) : url(u.url)
      {
      U_MEMORY_TEST_COPY(u)

      set(u);
      }

   Url& operator=(const Url& u)
      {
      U_MEMORY_TEST_COPY(u)

      url = u.url;

      set(u);

      return *this;
      }

   void set(const UString& x)
      {
      U_TRACE(0, "Url::set(%.*S)", U_STRING_TO_TRACE(x))

      url = x;

      findpos();
      }

   UString   get() const { return url; }
   bool    empty() const { return url.empty(); }

   const char* getUrlData() const    { return url.data(); }
   uint32_t    getUrlDataLen() const { return url.size(); }

   /** This methode returns the specified service of the url.
    *
    * If there is no service specified the buffer will be empty.
    * The service has no ':' char at the end!
    *
    * @return str.
    */

   UString getService();

   bool isHTTP()
      {
      U_TRACE(0, "Url::isHTTP()")

      U_INTERNAL_ASSERT_POINTER(str_http)

      bool result = (getService() == *str_http);

      U_RETURN(result);
      }

   bool isHTTPS()
      {
      U_TRACE(0, "Url::isHTTPS()")

      U_INTERNAL_ASSERT_POINTER(str_https)

      bool result = (getService() == *str_https);

      U_RETURN(result);
      }

   /** This methode set the service of the url.
    *
    * The should no ':' char at the end!
    *
    * @param service Service to set.
    */

   void setService(const char* service, uint32_t n);

   /** This methode returns the user identifier part of the UserInfo part of this URL.
    *
    * This method assumes that UserInfo is structured like this:
    * @c <userid>:<password>
    * If there is no user specified the buffer will be empty.
    *
    * @return str.
    */

   UString getUser();

   /** This methode set the user of the url.
    *
    * @param user User to set.
    */

   bool setUser(const char* user, uint32_t n);

   /** This methode erase the user from the url.
   */

   void eraseUser()
      {
      U_TRACE(0, "Url::eraseUser()")

      if (user_begin < user_end)
         {
         (void) url.erase(user_begin, user_end + 1);

         findpos();
         }
      }

   /** Checks if there is a host specified.
    *
    * @retval true  It has a host.
    * @retval false There is no host, so it will be a local file.
    */

   bool isLocalFile()
      {
      U_TRACE(0, "Url::isLocalFile()")

      bool result = (host_begin < host_end ? true : false); // Is there a host ?

      U_RETURN(result);
      }

   /** This methode returns the host name part of the URL.
    *
    * Not all URLs contain a host name, but those that do specify the host as part of the authority segment which is contained
    * between the @c '//' and the following @c '/' or @c '?' characters.
    * The host name is a sub-string of the authority part, with user and port number information removed.
    * For example, the following URL's host is @c www.elcel.com : @c http://user:password@www.elcel.com:80/index.html
    * If there is no host specified the buffer will be empty.
    *
    * @return str.
    */

   UString getHost();

   /** This methode set the host.
    *
    * @param host  host to set.
    * @param n     len string host to set.
    */

   void setHost(const char* host_, uint32_t n);

   /** This returns the port number from the URL or -1 if no port number is present.
    *
    * The port number is usually contained within the authority part of the URL and is
    * separated from the host by a colon character. For example, the following URL has
    * a port number of 81: @c http://www.acme.org:81
    * If there is no port, translate the service to a port number. 
    *
    * @retval 0..65535 specified port.
    * @retval       -1 no port specified.
    */

   int getPort();

   /** This methode get the port.
   */

   uint32_t getPort(char* buffer, uint32_t size);

   /** Set the port number.
    *
    * @warning only possible if a url is specified.
    */

   bool setPort(uint32_t port);

   /** This methode returns the path for this URL.
    *
    * The path consists of the file name part of the URL without any query information.
    *
    * For example, the path for the following URL is @a '/search'.
    * @c http://www.google.com/search?q=xml
    * If there is no path specified the buffer contains '/'. 
    *
    * @return str.
    */

   UString getPath();

   /** This methode set the path of the url.
    *
    * If the first char is not an '/' it will be added. 
    *
    * @param path Path to set.
    */

   void setPath(const char* path, uint32_t n);

   /** Returns the file name for this URL. The file name consists of the path plus the query (if present).
    *
    * For example, the file name for the following URL is @a '/search?q=xml'.
    * @c http://www.google.com/search?q=xml
    */

   UString getFile();

   /** This methode check the existence of the query from the url.
    *
    * @return bool.
    */

   bool isQuery()
      {
      U_TRACE(0, "Url::isQuery()")

      bool result = (path_end < (int)(url.size() - 1));

      U_RETURN(result);
      }

   /** This methode returns the portion of the file after (but not including) '?' which represents the start of a query string.
    *
    * If there is no query specified the buffer will be empty.
    *
    * @return str.
    */

   UString  getQuery();
   uint32_t getQuery(UVector<UString>& vec);

   /** This methode set the query of the url.
    *
    * @param query Query to set.
    */

   bool setQuery(UVector<UString>& vec);
   bool setQuery(const char* query, uint32_t n);

   /** This methode erase the query from the url.
   */

   void eraseQuery()
      {
      U_TRACE(0, "Url::eraseQuery()")

      if (path_end < (int)url.size())
         {
         (void) url.erase(path_end);

         query = path_end;
         }
      }

   /** This methode add's a new entry to the query.
    *
    * The entry and the value will first encoded and then added to the url.
    * To seperate the entry's the '&' character is been used.
    *
    * @param entry      Name of the entry.
    * @param entry_len  len string entry = 0 No query will be added.
    * @param value      Value of the entry.
    * @param value_len  len string value = 0 only the entry will be added.
    */

   void addQuery(const char* entry, uint32_t entry_len, const char* value, uint32_t value_len);

   /** This methode get the first query entry and decode it.
    *
    * @param entry Buffer for the name of the entry.
    * @param value Buffer for the value of the entry.
    *
    * @retval true The first entry found.
    * @retval false No entry found.
    *
    * @see nextQuery, addQuery;
    */

   bool firstQuery(UString& entry, UString& value);

   /** This methode get the next query entry and decode it.
    *
    * @param entry Buffer for the name of the entry.
    * @param value Buffer for the value of the entry.
    *
    * @retval true The first entry found.
    * @retval false No entry found. 
    *
    * @see firstQuery, addQuery;
    */

   bool nextQuery(UString& entry, UString& value);

   /** This methode search for the query entry and decode it.
    *
    * @param entry Buffer for the name of the entry.
    * @param value Buffer for the value of the entry.
    *
    * @retval true entry found.
    * @retval false No entry found.
    *
    * @see nextQuery, addQuery;
    */

   bool findQuery(UString& entry, UString& value);

   /** Converts a Unicode string into the MIME @c x-www-form-urlencoded format.
    *
    * To convert a String, each Unicode character is examined in turn:
    * - The ASCII characters 'a' through 'z', 'A' through 'Z', '0' through '9', and ".", "-", "*", "_" remain the same.
    *   - The space character ' '(U+20) is converted into a plus sign '+'.
    *   - All other characters are converted into their UTF-8 equivalent and the subsequent bytes are encoded
    *   as the 3-byte string "%xy", where xy is the two-digit hexadecimal representation of the byte.
    *
    * @param input  string to encode
    * @param len    size of the encoded string
    * @param buffer buffer for the encoded string. The size of the buffer has to be 3 * len
    */

   static void encode(const char* input, uint32_t len, UString& buffer, const char* extra_enc_chars = 0)
      {
      U_TRACE(0, "Url::encode(%.*S,%u,%p,%S)", len, input, len, &buffer, extra_enc_chars)

      buffer.size_adjust(u_url_encode((const unsigned char*)input, len, (unsigned char*)buffer.data(), extra_enc_chars));

      U_INTERNAL_DUMP("buffer.size() = %u", buffer.size())
      }

   static void encode(const UString& input, UString& buffer, const char* extra_enc_chars = 0)
      { encode(input.data(), input.size(), buffer, extra_enc_chars); }

   /** Decode a string
    *
    * @param input  string to decode
    * @param len    size of the encoded string
    * @param buffer buffer for the decoded string. The size of the buffer has to be at minimum len
    */

   static void decode(const char* input, uint32_t len, UString& buffer, bool no_line_break = false)
      {
      U_TRACE(0, "Url::decode(%.*S,%u,%p,%b)", len, input, len, &buffer, no_line_break)

      buffer.size_adjust(u_url_decode(input, len, (unsigned char*)buffer.data(), no_line_break));

      U_INTERNAL_DUMP("buffer.size() = %u", buffer.size())
      }

   static void decode(const UString& input, UString& buffer, bool no_line_break = false)
      { decode(U_STRING_TO_PARAM(input), buffer, no_line_break); }

   static void str_allocate();

   // STREAM

   friend U_EXPORT istream& operator>>(istream& is,       Url& u);
   friend U_EXPORT ostream& operator<<(ostream& os, const Url& u);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UString url;      // Content string - not first for object dump...
   int service_end,  // End position of the service.
       user_begin,   // begin position of the user.
       user_end,     // end position of the user.
       host_begin,   // begin position of the host.
       host_end,     // end position of the host.
       path_begin,   // begin position of the path.
       path_end,     // end position of the path.
       query;        // start position of the last readed query entry.

   /** scanns the structure of the url and is updating the position attributs of the class.
   */
   void findpos();

private:
   /** prepeats the string to add a query.
   */
   bool prepeareForQuery() U_NO_EXPORT;

   /** get the positions of the next query entry.
   */
   bool nextQueryPos(int& entry_start, int& entry_end, int& value_start, int& value_end) U_NO_EXPORT;
};

#endif
