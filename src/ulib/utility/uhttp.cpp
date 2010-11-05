// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    uhttp.cpp - HTTP utility
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/internal/common.h>

/*
#ifndef DEBUG
# undef U_DUMP
# undef U_TRACE
# undef U_INTERNAL_DUMP
# undef U_INTERNAL_TRACE

# define U_INTERNAL_TRACE(format, args...)                                                \
   { char u_internal_buf[16 * 1024]; (void) sprintf(u_internal_buf, format"\n" , ##args); \
     (void) write(STDERR_FILENO, u_internal_buf, strlen(u_internal_buf)); }

# define U_DUMP(args...)            U_INTERNAL_TRACE(args)
# define U_TRACE(level,args...)     U_INTERNAL_TRACE(args)
# define U_INTERNAL_DUMP(args...)   U_INTERNAL_TRACE(args)
// --------------
// TODO subst
// --------------
// %b    => %d
// %S    => %s
// %.2S  => %.2s
// %.8S  => %.8s
// %.*S  => %.*s
// %#.*S => %.*s
// %.20S => %.20s
// --------------
#endif
*/

#include <ulib/url.h>
#include <ulib/date.h>
#include <ulib/file.h>
#include <ulib/command.h>
#include <ulib/tokenizer.h>
#include <ulib/json/value.h>
#include <ulib/mime/entity.h>
#include <ulib/utility/uhttp.h>
#include <ulib/mime/multipart.h>
#include <ulib/utility/base64.h>
#include <ulib/base/coder/url.h>
#include <ulib/utility/services.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/socket_ext.h>
#include <ulib/utility/string_ext.h>
#include <ulib/container/hash_map.h>

#ifdef HAVE_MAGIC
#  include <ulib/magic/magic.h>
#endif

#ifdef SYS_INOTIFY_H_EXISTS_AND_WORKS
#  include <sys/inotify.h>
#elif defined(LINUX) || defined(__LINUX__) || defined(__linux__)
#  ifdef HAVE_SYS_INOTIFY_H
#  undef HAVE_SYS_INOTIFY_H
#  endif
#  include <ulib/replace/inotify-nosys.h>
#endif

#ifdef __MINGW32__
#define U_LEN_SUFFIX 4 // .dll
#else
#define U_LEN_SUFFIX 3 // .so
#endif

int                               UHTTP::inotify_wd;
char                              UHTTP::cgi_dir[U_PATH_MAX];
bool                              UHTTP::virtual_host;
bool                              UHTTP::digest_authentication;
void*                             UHTTP::argument;
UFile*                            UHTTP::file;
UValue*                           UHTTP::json;
UString*                          UHTTP::alias;
UString*                          UHTTP::tmpdir;
UString*                          UHTTP::qcontent;
UString*                          UHTTP::pathname;
UString*                          UHTTP::request_uri;
UString*                          UHTTP::penvironment;
UString*                          UHTTP::cache_file_mask;
UString*                          UHTTP::htpasswd;
UString*                          UHTTP::htdigest;
UCommand*                         UHTTP::pcmd;
uhttpinfo                         UHTTP::http_info;
UStringRep*                       UHTTP::pkey;
const char*                       UHTTP::ptrH;
const char*                       UHTTP::ptrC;
const char*                       UHTTP::ptrT;
const char*                       UHTTP::ptrL;
const char*                       UHTTP::ptrR;
const char*                       UHTTP::ptrI;
const char*                       UHTTP::ptrA;
UMimeMultipart*                   UHTTP::formMulti;
UVector<UString>*                 UHTTP::form_name_value;
UVector<UIPAllow*>*               UHTTP::vallow_IP;

         UHTTP::UServletPage*     UHTTP::page;
UHashMap<UHTTP::UServletPage*>*   UHTTP::pages;

         UHTTP::UFileCacheData*   UHTTP::file_data;
UHashMap<UHTTP::UFileCacheData*>* UHTTP::cache_file;

const UString* UHTTP::str_origin;
const UString* UHTTP::str_favicon;
const UString* UHTTP::str_frm_body;
const UString* UHTTP::str_htpasswd;
const UString* UHTTP::str_htdigest;
const UString* UHTTP::str_indexhtml;
const UString* UHTTP::str_websocket;
const UString* UHTTP::str_ctype_tsa;
const UString* UHTTP::str_frm_header;
const UString* UHTTP::str_ctype_html;
const UString* UHTTP::str_ctype_soap;
const UString* UHTTP::str_frm_websocket;
const UString* UHTTP::str_websocket_key1;
const UString* UHTTP::str_websocket_key2;
const UString* UHTTP::str_websocket_prot;

void UHTTP::str_allocate()
{
   U_TRACE(0, "UHTTP::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_favicon,0)
   U_INTERNAL_ASSERT_EQUALS(str_htdigest,0)
   U_INTERNAL_ASSERT_EQUALS(str_htpasswd,0)
   U_INTERNAL_ASSERT_EQUALS(str_indexhtml,0)
   U_INTERNAL_ASSERT_EQUALS(str_ctype_tsa,0)
   U_INTERNAL_ASSERT_EQUALS(str_ctype_html,0)
   U_INTERNAL_ASSERT_EQUALS(str_ctype_soap,0)
   U_INTERNAL_ASSERT_EQUALS(str_frm_header,0)
   U_INTERNAL_ASSERT_EQUALS(str_frm_body,0)
   U_INTERNAL_ASSERT_EQUALS(str_origin,0)
   U_INTERNAL_ASSERT_EQUALS(str_websocket,0)
   U_INTERNAL_ASSERT_EQUALS(str_websocket_key1,0)
   U_INTERNAL_ASSERT_EQUALS(str_websocket_key2,0)
   U_INTERNAL_ASSERT_EQUALS(str_websocket_prot,0)
   U_INTERNAL_ASSERT_EQUALS(str_frm_websocket,0)

   static ustringrep stringrep_storage[] = {
   { U_STRINGREP_FROM_CONSTANT("favicon.ico") },
   { U_STRINGREP_FROM_CONSTANT(".htdigest") },
   { U_STRINGREP_FROM_CONSTANT(".htpasswd") },
   { U_STRINGREP_FROM_CONSTANT("index.html") },
   { U_STRINGREP_FROM_CONSTANT("application/timestamp-reply") },
   { U_STRINGREP_FROM_CONSTANT(U_CTYPE_HTML) },
   { U_STRINGREP_FROM_CONSTANT("application/soap+xml; charset=\"utf-8\"") },
   { U_STRINGREP_FROM_CONSTANT("HTTP/1.%c %d %s\r\n"
                               "Server: ULib/1.0\r\n"
                               "%.*s"
                               "%.*s") },
   { U_STRINGREP_FROM_CONSTANT("<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
                               "<html><head>\r\n"
                               "<title>%d %s</title>\r\n"
                               "</head><body>\r\n"
                               "<h1>%s</h1>\r\n"
                               "<p>%.*s</p>\r\n"
                               "<hr>\r\n"
                               "<address>ULib Server</address>\r\n"
                               "</body></html>\r\n") },
   { U_STRINGREP_FROM_CONSTANT("Origin") },
   { U_STRINGREP_FROM_CONSTANT("Upgrade: WebSocket") },
   { U_STRINGREP_FROM_CONSTANT("Sec-WebSocket-Key1") },
   { U_STRINGREP_FROM_CONSTANT("Sec-WebSocket-Key2") },
   { U_STRINGREP_FROM_CONSTANT("Sec-WebSocket-Protocol") },
   { U_STRINGREP_FROM_CONSTANT("HTTP/1.1 101 Web Socket Protocol Handshake\r\n"
                               "Upgrade: WebSocket\r\n"
                               "Connection: Upgrade\r\n"
                               "WebSocket-Origin: %.*s\r\n"
                               "WebSocket-Location: ws://%.*s%.*s\r\n"
                               "%.*s"
                               "\r\n") }
   };

   U_NEW_ULIB_OBJECT(str_favicon,        U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_htdigest,       U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_htpasswd,       U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_indexhtml,      U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_ctype_tsa,      U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_ctype_html,     U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_ctype_soap,     U_STRING_FROM_STRINGREP_STORAGE(6));
   U_NEW_ULIB_OBJECT(str_frm_header,     U_STRING_FROM_STRINGREP_STORAGE(7));
   U_NEW_ULIB_OBJECT(str_frm_body,       U_STRING_FROM_STRINGREP_STORAGE(8));
   U_NEW_ULIB_OBJECT(str_origin,         U_STRING_FROM_STRINGREP_STORAGE(9));
   U_NEW_ULIB_OBJECT(str_websocket,      U_STRING_FROM_STRINGREP_STORAGE(10));
   U_NEW_ULIB_OBJECT(str_websocket_key1, U_STRING_FROM_STRINGREP_STORAGE(11));
   U_NEW_ULIB_OBJECT(str_websocket_key2, U_STRING_FROM_STRINGREP_STORAGE(12));
   U_NEW_ULIB_OBJECT(str_websocket_prot, U_STRING_FROM_STRINGREP_STORAGE(13));
   U_NEW_ULIB_OBJECT(str_frm_websocket,  U_STRING_FROM_STRINGREP_STORAGE(14));
}

UHTTP::UFileCacheData::UFileCacheData()
{
   U_TRACE_REGISTER_OBJECT(0, UFileCacheData, "")

   U_INTERNAL_ASSERT_POINTER(file)

   wd    = -1;
   size  = file->st_size;
   mode  = file->st_mode;
   mtime = file->st_mtime;
   array = 0;

#ifdef HAVE_SYS_INOTIFY_H
   U_INTERNAL_ASSERT_POINTER(UServer_Base::handler_event)

   int                  flags  = IN_ONLYDIR | IN_CREATE | IN_DELETE;
   if (cache_file_mask) flags |= IN_MODIFY;

   if (u_ftw_ctx.is_directory ||
       file->dir())
      {
      wd = U_SYSCALL(inotify_add_watch, "%d,%s,%u", UServer_Base::handler_event->fd, pathname->c_str(), flags);
      }
#endif

   U_INTERNAL_DUMP("size = %u mtime = %ld dir() = %b u_ftw_ctx.is_directory = %b", size, mtime, S_ISDIR(mode), u_ftw_ctx.is_directory)

   U_INTERNAL_ASSERT_POINTER(cache_file)

   cache_file->insert(*pathname, this);
}

UHTTP::UFileCacheData::~UFileCacheData()
{
   U_TRACE_UNREGISTER_OBJECT(0, UFileCacheData)

   if (array) delete array;

#ifdef HAVE_SYS_INOTIFY_H
   U_INTERNAL_ASSERT_POINTER(UServer_Base::handler_event)

   if (wd != -1) (void) U_SYSCALL(inotify_rm_watch, "%d,%d", UServer_Base::handler_event->fd, wd);
#endif
}

U_NO_EXPORT void UHTTP::getInotifyPathDirectory(UStringRep* key, void* value)
{
   U_TRACE(0, "UHTTP::getInotifyPathDirectory(%.*S,%p)", U_STRING_TO_TRACE(*key), value)

   file_data = (UFileCacheData*)value;

   U_INTERNAL_ASSERT_POINTER(file_data)

   if (file_data->wd == inotify_wd)
      {
      pkey->str     = key->str;
      pkey->_length = key->_length;

      cache_file->stopCallForAllEntry();
      }
}

U_NO_EXPORT void UHTTP::checkInotifyForCache(int wd, char* name, uint32_t len)
{
   U_TRACE(0, "UHTTP::checkInotifyForCache(%d,%.*S,%u)", wd, len, name, len)

   U_INTERNAL_ASSERT_POINTER(cache_file)

   if (wd        != inotify_wd ||
       file_data == 0          ||
       wd        != file_data->wd)
      {
      inotify_wd = wd;

      cache_file->callForAllEntry(getInotifyPathDirectory);
      }

   U_INTERNAL_ASSERT_POINTER(file_data)

   if (file_data->wd != wd) file_data = 0;
   else
      {
      char buffer[U_PATH_MAX];

      pkey->_length = u_snprintf(buffer, sizeof(buffer), "%.*s/%.*s", U_STRING_TO_TRACE(*pkey), len, name);
      pkey->str     = buffer;

      file_data = (*cache_file)[pkey];
      }
}

U_NO_EXPORT void UHTTP::in_CREATE()
{
   U_TRACE(0, "UHTTP::in_CREATE()")

   if (file_data == 0)
      {
      u_buffer_len = u_snprintf(u_buffer, sizeof(u_buffer), "./%.*s", U_STRING_TO_TRACE(*pkey));

      checkFileForCache();

      u_buffer_len = 0;
      }
}

U_NO_EXPORT void UHTTP::in_DELETE()
{
   U_TRACE(0, "UHTTP::in_DELETE()")

   if (file_data)
      {
      cache_file->eraseAfterFind();

      file_data = 0;
      }
}

#define IN_BUFLEN (1024 * (sizeof(struct inotify_event) + 16))

void UHTTP::in_READ()
{
   U_TRACE(1, "UHTTP::in_READ()")

   /*
   struct inotify_event {
      int wd;           // The watch descriptor
      uint32_t mask;    // Watch mask
      uint32_t cookie;  // A cookie to tie two events together
      uint32_t len;     // The length of the filename found in the name field
      char name[];      // The name of the file, padding to the end with NULs
   }
   */

#ifdef HAVE_SYS_INOTIFY_H
   union uuinotify_event {
                      char*  p;
      struct inotify_event* ip;
   };

   uint32_t len;
   char buffer[IN_BUFLEN];
   union uuinotify_event event;
   int i = 0, length = U_SYSCALL(read, "%d,%p,%u", UServer_Base::handler_event->fd, buffer, IN_BUFLEN);  

   while (i < length)
      {
      event.p = buffer+i;

      if (event.ip->len)
         {
         U_INTERNAL_DUMP("The %s %s(%u) was %s", (event.ip->mask & IN_ISDIR  ? "directory" : "file"), event.ip->name, event.ip->len,
                                                 (event.ip->mask & IN_CREATE ? "created"   :
                                                  event.ip->mask & IN_DELETE ? "deleted"   :
                                                  event.ip->mask & IN_MODIFY ? "modified"  : "???"))

         u_ftw_ctx.is_directory = (event.ip->mask & IN_ISDIR);

         // NB: The length contains any potential padding that is, the result of strlen() on the name field may be smaller than len...

         for (len = event.ip->len; event.ip->name[len-1] == '\0'; --len) {}

         U_INTERNAL_ASSERT_EQUALS(len,strlen(event.ip->name))

         checkInotifyForCache(event.ip->wd, event.ip->name, len);

         if (event.ip->mask & IN_CREATE) in_CREATE();
         else
            {
                 if (event.ip->mask & IN_DELETE) in_DELETE();
            else if (event.ip->mask & IN_MODIFY)
               {
               if (file_data &&
                   isDataFromCache(false))
                  {
                  in_DELETE();
                  in_CREATE();
                  }
               }
            }
         }

      i += sizeof(struct inotify_event) + event.ip->len;
      }
#endif
}

void UHTTP::ctor(UEventFd* handler_event)
{
   U_TRACE(0, "UHTTP::ctor(%p)", handler_event)

   str_allocate();

   if (        Url::str_ftp  == 0)         Url::str_allocate();
   if (UMimeHeader::str_name == 0) UMimeHeader::str_allocate();

   U_INTERNAL_ASSERT_EQUALS(file,0)
   U_INTERNAL_ASSERT_EQUALS(alias,0)
   U_INTERNAL_ASSERT_EQUALS(tmpdir,0)
   U_INTERNAL_ASSERT_EQUALS(qcontent,0)
   U_INTERNAL_ASSERT_EQUALS(pathname,0)
   U_INTERNAL_ASSERT_EQUALS(formMulti,0)
   U_INTERNAL_ASSERT_EQUALS(request_uri,0)
   U_INTERNAL_ASSERT_EQUALS(penvironment,0)
   U_INTERNAL_ASSERT_EQUALS(form_name_value,0)

   file            = U_NEW(UFile);
   pcmd            = U_NEW(UCommand);
   pkey            = UStringRep::create(0U, 100U, 0);
   alias           = U_NEW(UString(U_CAPACITY));
   tmpdir          = U_NEW(UString(U_PATH_MAX));
   qcontent        = U_NEW(UString);
   pathname        = U_NEW(UString(U_CAPACITY));
   formMulti       = U_NEW(UMimeMultipart);
   request_uri     = U_NEW(UString);
   penvironment    = U_NEW(UString(U_CAPACITY));
   form_name_value = U_NEW(UVector<UString>);

   U_INTERNAL_ASSERT_POINTER(USocket::str_host)
   U_INTERNAL_ASSERT_POINTER(USocket::str_range)
   U_INTERNAL_ASSERT_POINTER(USocket::str_connection)
   U_INTERNAL_ASSERT_POINTER(USocket::str_content_type)
   U_INTERNAL_ASSERT_POINTER(USocket::str_content_length)
   U_INTERNAL_ASSERT_POINTER(USocket::str_accept_encoding)
   U_INTERNAL_ASSERT_POINTER(USocket::str_if_modified_since)

   ptrH = USocket::str_host->c_pointer(1);              // "Host"
   ptrR = USocket::str_range->c_pointer(1);             // "Range"
   ptrC = USocket::str_connection->c_pointer(1);        // "Connection"
   ptrT = USocket::str_content_type->c_pointer(1);      // "Content-Type"
   ptrL = USocket::str_content_length->c_pointer(1);    // "Content-Length"
   ptrA = USocket::str_accept_encoding->c_pointer(1);   // "Accept-Encoding"
   ptrI = USocket::str_if_modified_since->c_pointer(1); // "If-Modified-Since"

   // USP (ULib Servlet Page)

   U_INTERNAL_ASSERT_EQUALS(page,0)
   U_INTERNAL_ASSERT_EQUALS(pages,0)

   if (UFile::chdir("usp", true))
      {
      const char* ptr;
      UVector<UString> vec;
      UString name, key(100U);
      char buffer[U_PATH_MAX];

      pages = U_NEW(UHashMap<UHTTP::UServletPage*>);

      pages->allocate();

      for (uint32_t i = 0, n = UFile::listContentOf(vec); i < n; ++i)
         {
         name = vec[i];
         ptr  = name.data();
         page = U_NEW(UHTTP::UServletPage);

         // NB: dlopen() fail if not prefix with "./"...

         (void) snprintf(buffer, U_PATH_MAX, "./%.*s", U_STRING_TO_TRACE(name));

         if (page->UDynamic::load(buffer)) page->runDynamicPage = (vPFpv) (*page)["runDynamicPage"];

         if (page->runDynamicPage)
            {
            (void) snprintf(buffer, U_PATH_MAX, "%.*s.usp", name.size() - U_LEN_SUFFIX, ptr);

            (void) u_canonicalize_pathname(buffer);

            (void) key.replace(buffer);

            pages->insert(key, page);

            U_SRV_LOG("USP found: usp/%s, USP service registered (URI): /usp/%.*s", ptr, U_STRING_TO_TRACE(key));
            }
         else
            {
            U_SRV_LOG("USP load failed: usp/%.*s.usp", name.size() - U_LEN_SUFFIX, ptr);

            delete page;
            }
         }

      (void) UFile::chdir(0, true);

      if (pages->empty() == false) callRunDynamicPage(0); // call init for all usp modules...
      else
         {
         pages->deallocate();

         delete pages;
                pages = 0;
         }

      page = 0;
      }

   // CACHE FILE SYSTEM

   U_INTERNAL_ASSERT_EQUALS(cache_file,0)

   cache_file = U_NEW(UHashMap<UFileCacheData*>);

   cache_file->allocate();

   if (handler_event)
      {
      UServer_Base::handler_event = handler_event;

      // INIT INOTIFY FOR CACHE FILE SYSTEM

#ifdef HAVE_SYS_INOTIFY_H
#  ifdef HAVE_INOTIFY_INIT1
      handler_event->fd = U_SYSCALL(inotify_init1, "%d", IN_NONBLOCK | IN_CLOEXEC);
#  else
      handler_event->fd = U_SYSCALL_NO_PARAM(inotify_init);

      (void) U_SYSCALL(fcntl, "%d,%d,%d", handler_event->fd, F_SETFL, O_NONBLOCK | O_CLOEXEC);
#  endif

      if (handler_event->fd == -1)
         {
         U_ERROR("Inode based directory notification failed");
         }
      else
         {
         U_SRV_LOG("Inode based directory notification enabled");
         }
#endif
      }

   (void) UServices::setFtw(0);

   u_ftw_ctx.call              = UHTTP::checkFileForCache;
   u_ftw_ctx.sort_by           = u_ftw_ino_cmp;
   u_ftw_ctx.call_if_directory = true;

   u_ftw();

   u_buffer_len = 0;

   uint32_t sz = cache_file->size();

   U_INTERNAL_DUMP("cache size = %u", sz)

   sz += (sz / 100) * 25;

   cache_file->reserve(sz);

   // manage authorization data...

   file_data = (*cache_file)[*str_htpasswd];

   if (file_data &&
       isDataFromCache(false))
      {
      htpasswd = U_NEW(UString(getDataFromCache(false,false)));

   // U_SRV_LOG("file data users permission: '%.*s' loaded", U_STRING_TO_TRACE(*str_htpasswd));
      }

   file_data = (*cache_file)[*str_htdigest];

   if (file_data &&
       isDataFromCache(false))
      {
      htdigest = U_NEW(UString(getDataFromCache(false,false)));

   // U_SRV_LOG("file data users permission: '%.*s' loaded", U_STRING_TO_TRACE(*str_htdigest));
      }

   UServices::generateKey(); // For ULIB facility request TODO stateless session cookies... 
}

void UHTTP::dtor()
{
   U_TRACE(0, "UHTTP::dtor()")

   if (cache_file_mask) delete cache_file_mask;

   if (file)
      {
      delete file;
      delete pcmd;
      delete pkey;
      delete alias;
      delete tmpdir;
      delete qcontent;
      delete pathname;
      delete formMulti;
      delete request_uri;
      delete penvironment;
      delete form_name_value;

      if (htpasswd)     delete htpasswd;
      if (htdigest)     delete htdigest;
      if (vallow_IP)    delete vallow_IP;
      if (vRewriteRule) delete vRewriteRule;

      // CACHE FILE SYSTEM

      /*
#  if defined(HAVE_SYS_INOTIFY_H) && defined(DEBUG)
      UStringRep* rep = UObject2StringRep(cache_file);
      (void) UFile::writeToTmpl("/tmp/cache_file.%P", UString(rep));
      delete rep;
#  endif
      */

             cache_file->clear();
             cache_file->deallocate();
      delete cache_file;

      // inotify: Inode based directory notification...

#  ifdef HAVE_SYS_INOTIFY_H
      if (UServer_Base::handler_event &&
          UServer_Base::handler_event->fd != -1)
         {
         (void) U_SYSCALL(close, "%d", UServer_Base::handler_event->fd);
         }
#  endif

      // USP (ULib Servlet Page)

      if (pages)
         {
         U_INTERNAL_ASSERT_EQUALS(pages->empty(),false)

         callRunDynamicPage(-2); // call end for all usp modules...

         pages->clear();
         pages->deallocate();

         delete pages;
         }
      }
}

UString UHTTP::getDocumentName()
{
   U_TRACE(0, "UHTTP::getDocumentName()")

   U_INTERNAL_ASSERT_POINTER(file)

   UString document = UStringExt::basename(file->getPath());

   U_RETURN_STRING(document);
}

UString UHTTP::getDirectoryURI()
{
   U_TRACE(0, "UHTTP::getDirectoryURI()")

   U_INTERNAL_ASSERT_POINTER(file)

   UString directory = UStringExt::dirname(file->getPath());

   U_RETURN_STRING(directory);
}

UString UHTTP::getRequestURI(bool bquery)
{
   U_TRACE(0, "UHTTP::getRequestURI(%b)", bquery)

   // NB: there may be an ALIAS...

   U_INTERNAL_ASSERT_POINTER(request_uri)

   UString uri(U_CAPACITY);

   if (request_uri->empty())
      {
      if (bquery &&
          http_info.query_len)
         {
         (void) uri.assign(U_HTTP_URI_QUERY_TO_PARAM);
         }
      else
         {
         (void) uri.assign(U_HTTP_URI_TO_PARAM);
         }
      }
   else
      {
      uri = *request_uri;

      if (bquery &&
          http_info.query_len)
         {
         uri.push_back('?');

         (void) uri.append(U_HTTP_QUERY_TO_PARAM);
         }
      }

   U_RETURN_STRING(uri);
}

/* read HTTP message
======================================================================================
Read the request line and attached headers. A typical http request will take the form:
======================================================================================
GET / HTTP/1.1
Host: 127.0.0.1
User-Agent: Mozilla/5.0 (compatible; Konqueror/3.1; Linux; it, en_US, en)
Accept-Encoding: x-gzip, x-deflate, gzip, deflate, identity
Accept-Charset: iso-8859-1, utf-8;q=0.5, *;q=0.5
Accept-Language: it, en
Connection: Keep-Alive
<empty line>
======================================================================================
Read the result line and attached headers. A typical http response will take the form:
======================================================================================
HTTP/1.1 200 OK
Date: Wed, 25 Oct 2000 16:54:02 GMT
Age: 57718
Server: Apache/1.3b5
Last-Modified: Sat, 23 Sep 2000 15:00:57 GMT
Accept-Ranges: bytes
Etag: "122b12-2d8c-39ccc5a9"
Content-Type: text/html
Content-Length: 11660
<empty line>
.......<the data>
====================================================================================
*/

bool UHTTP::isHTTPRequest(const char* ptr)
{
   U_TRACE(0, "UHTTP::isHTTPRequest(%.*S)", 30, ptr)

   while (u_isspace(*ptr)) ++ptr; // skip space...

   unsigned char c = u_toupper(ptr[0]);

   if (c != 'G' && // GET
       c != 'P' && // POST/PUT
       c != 'D' && // DELETE
       c != 'H')   // HEAD
      {
      U_RETURN(false);
      }

   if (c == 'G') // GET
      {
      if (U_STRNCASECMP(ptr, "GET ")) U_RETURN(false);
      }
   else if (c == 'P') // POST/PUT
      {
      if (U_STRNCASECMP(ptr, "POST ") &&
          U_STRNCASECMP(ptr, "PUT "))
         {
         U_RETURN(false);
         }
      }
   else if (c == 'D') // DELETE
      {
      if (U_STRNCASECMP(ptr, "DELETE ")) U_RETURN(false);
      }
   else // HEAD
      {
      if (U_STRNCASECMP(ptr, "HEAD ")) U_RETURN(false);
      }

   U_RETURN(true);
}

bool UHTTP::scanfHTTPHeader(const char* ptr)
{
   U_TRACE(0, "UHTTP::scanfHTTPHeader(%.*S)", 30, ptr)

   /**
    * Check if HTTP response or request
    *
    * The default is GET for input requests and POST for output requests
    *
    * Other possible alternatives are:
    *  - PUT
    *  - HEAD
    *  - DELETE
    *  ---- NOT implemented -----
    *  - TRACE
    *  - OPTIONS
    *  --------------------------
    *
    * See http://ietf.org/rfc/rfc2616.txt for further information about HTTP request methods
    **/

   const char* start;
   unsigned char c = ptr[0];

   if (c == 'G') goto bypass;

   while (u_isspace(*ptr)) ++ptr; // RFC 2616 4.1 "servers SHOULD ignore any empty line(s) received where a Request-Line is expected."

   c = u_toupper(ptr[0]);

   if (c != 'G' && // GET
       c != 'P' && // POST/PUT
       c != 'D' && // DELETE
       c != 'H')   // HEAD or response
      {
      U_RETURN(false);
      }

bypass:
   // try to parse the request line: GET / HTTP/1.n

   start = http_info.method = ptr;

   if (c == 'G') // GET
      {
      U_http_method_type   = HTTP_GET;
      http_info.method_len = 3;

      U_INTERNAL_ASSERT_EQUALS(U_STRNCASECMP(http_info.method, "GET "), 0)
      }
   else if (c == 'P') // POST/PUT
      {
      if (u_toupper(ptr[1]) == 'O')
         {
         U_http_method_type   = HTTP_POST;
         http_info.method_len = 4;

         U_INTERNAL_ASSERT_EQUALS(U_STRNCASECMP(http_info.method, "POST "), 0)
         }
      else
         {
         U_http_method_type   = HTTP_PUT;
         http_info.method_len = 3;

         U_INTERNAL_ASSERT_EQUALS(U_STRNCASECMP(http_info.method, "PUT "), 0)
         }
      }
   else if (c == 'D') // DELETE
      {
      U_http_method_type   = HTTP_DELETE;
      http_info.method_len = 6;

      U_INTERNAL_ASSERT_EQUALS(U_STRNCASECMP(http_info.method, "DELETE "), 0)
      }
   else // HEAD or response
      {
      if (ptr[1] == 'T')
         {
         // try to parse the response line: HTTP/1.n nnn <ssss>

         if (U_STRNCMP(ptr, "HTTP/1.")) U_RETURN(false);

         ptr += U_CONSTANT_SIZE("HTTP/1.") + 2;

         http_info.nResponseCode = strtol(ptr, (char**)&ptr, 10);

         U_INTERNAL_DUMP("nResponseCode = %d", http_info.nResponseCode)

         goto end;
         }

      U_http_method_type   = HTTP_HEAD;
      http_info.method_len = 4;

      U_INTERNAL_ASSERT_EQUALS(U_STRNCASECMP(http_info.method, "HEAD "), 0)
      }

   U_INTERNAL_DUMP("method = %.*S method_type = %C", U_HTTP_METHOD_TO_TRACE, U_http_method_type)

   ptr += http_info.method_len;

   while (u_isspace(*++ptr)) {} // RFC 2616 19.3 "[servers] SHOULD accept any amount of SP or HT characters between [Request-Line] fields"

   http_info.uri = ptr;

   while (true)
      {
      c = *ptr;

      if (c == ' ') break;

      /* check uri for invalid characters (NB: \n can happen because openssl base64...) */

      if (c <=  32 ||
          c == 127 ||
          c == 255)
         {
         U_WARNING("invalid character %C in URI %.*S", c, ptr - http_info.uri, http_info.uri);

         U_RETURN(false);
         }

      if (c == '?')
         {
         http_info.uri_len = ptr - http_info.uri;
         http_info.query   = ++ptr;

         continue;
         }

      ++ptr;
      }

   if (http_info.query)
      {
      http_info.query_len = ptr - http_info.query;

      U_INTERNAL_DUMP("query = %.*S", U_HTTP_QUERY_TO_TRACE)
      }
   else
      {
      http_info.uri_len = ptr - http_info.uri;
      }

   U_INTERNAL_DUMP("uri = %.*S", U_HTTP_URI_TO_TRACE)

   while (u_isspace(*++ptr)) {} // RFC 2616 19.3 "[servers] SHOULD accept any amount of SP or HT characters between [Request-Line] fields"

   if (U_STRNCMP(ptr, "HTTP/1.")) U_RETURN(false);

   ptr += U_CONSTANT_SIZE("HTTP/1.");

   U_http_version = ptr[0];

   U_INTERNAL_DUMP("U_http_version = %C", U_http_version)

end:
   for (c = u_line_terminator[0]; *ptr != (char)c; ++ptr) {}

   http_info.startHeader = ptr - start;

   U_INTERNAL_DUMP("startHeader = %u", http_info.startHeader)

   U_INTERNAL_ASSERT(U_STRNEQ(ptr, U_LF) || U_STRNEQ(ptr, U_CRLF))

   U_RETURN(true);
}

bool UHTTP::readHTTPHeader(USocket* s, UString& rbuffer)
{
   U_TRACE(0, "UHTTP::readHTTPHeader(%p,%.*S)", s, U_STRING_TO_TRACE(rbuffer))

// U_ASSERT(rbuffer.empty())
   U_INTERNAL_ASSERT(s->isConnected())
   U_INTERNAL_ASSERT_EQUALS(http_info.szHeader,0)
   U_INTERNAL_ASSERT_EQUALS(http_info.endHeader,0)
   U_INTERNAL_ASSERT_EQUALS(http_info.startHeader,0)

   const char* ptr;
   int timeoutMS = -1;
   uint32_t endHeader = 0, count = 0;

start:
   U_INTERNAL_DUMP("startHeader = %u", http_info.startHeader)

   if ((endHeader == U_NOT_FOUND ||
        rbuffer.size() <= http_info.startHeader) &&
       USocketExt::read(s, rbuffer, U_SINGLE_READ, timeoutMS) == false)
      {
      U_RETURN(false);
      }

   ptr       = rbuffer.c_pointer(http_info.startHeader);
   endHeader = u_findEndHeader(ptr, rbuffer.remain(ptr));

   if (endHeader == U_NOT_FOUND)
      {
      if (rbuffer.isBinary()) U_RETURN(false);

      // NB: attacked by a "slow loris"... http://lwn.net/Articles/337853/

      if (count++ > 5) U_RETURN(false);

      timeoutMS = 3 * 1000;

      goto start;
      }

   // NB: endHeader comprende anche la blank line...

   endHeader += http_info.startHeader;

   U_INTERNAL_DUMP("endHeader = %u", endHeader)

   // NB: http_info.startHeader is needed for loop...

   if (scanfHTTPHeader(rbuffer.c_pointer(http_info.endHeader)) == false) U_RETURN(false);

   http_info.startHeader += http_info.endHeader;

   U_INTERNAL_DUMP("startHeader = %u", http_info.startHeader)

   // check if HTTP response line: HTTP/1.n 100 Continue

   if (http_info.nResponseCode == HTTP_CONTINUE)
      {
      /*
      --------------------------------------------------------------------------------------------------------
      During the course of an HTTP 1.1 client sending a request to a server, the server might respond with
      an interim "100 Continue" response. This means the server has received the first part of the request,
      and can be used to aid communication over slow links. In any case, all HTT 1.1 clients must handle the
      100 response correctly (perhaps by just ignoring it). The "100 Continue" response is structured like
      any HTTP response, i.e. consists of a status line, optional headers, and a blank line. Unlike other
      responses, it is always followed by another complete, final response. Example:
      --------------------------------------------------------------------------------------------------------
      HTTP/1.0 100 Continue
      [blank line here]
      HTTP/1.0 200 OK
      Date: Fri, 31 Dec 1999 23:59:59 GMT
      Content-Type: text/plain
      Content-Length: 42
      some-footer: some-value
      another-footer: another-value
      [blank line here]
      abcdefghijklmnoprstuvwxyz1234567890abcdef
      --------------------------------------------------------------------------------------------------------
      To handle this, a simple HTTP 1.1 client might read one response from the socket;
      if the status code is 100, discard the first response and read the next one instead.
      --------------------------------------------------------------------------------------------------------
      */

      U_INTERNAL_ASSERT(U_STRNEQ(rbuffer.c_pointer(http_info.startHeader), U_LF2) ||
                        U_STRNEQ(rbuffer.c_pointer(http_info.startHeader), U_CRLF2))

      http_info.startHeader += u_line_terminator_len * 2;
      http_info.endHeader    = http_info.startHeader;

      goto start;
      }

   http_info.startHeader += u_line_terminator_len;
   http_info.endHeader    = endHeader; // NB: endHeader comprende anche la blank line...
   http_info.szHeader     = endHeader - http_info.startHeader;

   U_INTERNAL_DUMP("szHeader = %u startHeader = %.*S endHeader = %.*S", http_info.szHeader,
                        20, rbuffer.c_pointer(http_info.startHeader),
                        20, rbuffer.c_pointer(http_info.endHeader))

   U_RETURN(true);
}

bool UHTTP::readHTTPBody(USocket* s, UString& rbuffer, UString& body)
{
   U_TRACE(0, "UHTTP::readHTTPBody(%p,%.*S,%.*S)", s, U_STRING_TO_TRACE(rbuffer), U_STRING_TO_TRACE(body))

   U_ASSERT(body.empty())
   U_INTERNAL_ASSERT(s->isConnected())
   U_INTERNAL_ASSERT_MAJOR(http_info.szHeader,0)

   uint32_t body_byte_read;

   if (http_info.clength == 0)
      {
            char* out;
      const char* inp;
      const char* chunk_terminator;
      uint32_t count, chunkSize, chunk_terminator_len,
               chunk = U_STRING_FIND_EXT(rbuffer, http_info.startHeader, "Transfer-Encoding: chunked", http_info.szHeader);

      U_INTERNAL_DUMP("chunk = %u", chunk)

      if (chunk == U_NOT_FOUND)
         {
         U_ASSERT(isHttpPOST())

         if (U_http_version == '1') U_RETURN(false); // HTTP/1.1 compliance: no missing Content-Length on POST requests

         // NB: attacked by a "slow loris"... http://lwn.net/Articles/337853/

         count = 0;

         do { if (count++ > 5) U_RETURN(false); } while (USocketExt::read(s, rbuffer, U_SINGLE_READ, 3 * 1000)); // wait max 3 sec for other data...

         http_info.clength = (rbuffer.size() - http_info.endHeader);

         U_INTERNAL_DUMP("http_info.clength = %u", http_info.clength)

         if (http_info.clength == 0) U_RETURN(false);

         goto end;
         }

      /* ----------------------------------------------------------------------------------------------------------
      If a server wants to start sending a response before knowing its total length (like with long script output),
      it might use the simple chunked transfer-encoding, which breaks the complete response into smaller chunks and
      sends them in series. You can identify such a response because it contains the "Transfer-Encoding: chunked"
      header. All HTTP 1.1 clients must be able to receive chunked messages. A chunked message body contains a
      series of chunks, followed by a line with a single "0" (zero), followed by optional footers (just like headers),
      and a blank line. Each chunk consists of two parts: a line with the size of the chunk data, in hex, possibly
      followed by a semicolon and extra parameters you can ignore (none are currently standard), and ending with
      CRLF. the data itself, followed by CRLF. An example:

      HTTP/1.1 200 OK
      Date: Fri, 31 Dec 1999 23:59:59 GMT
      Content-Type: text/plain
      Transfer-Encoding: chunked
      [blank line here]
      1a; ignore-stuff-here
      abcdefghijklmnopqrstuvwxyz
      10
      1234567890abcdef
      0
      some-footer: some-value
      another-footer: another-value
      [blank line here]

      Note the blank line after the last footer. The length of the text data is 42 bytes (1a + 10, in hex),
      and the data itself is abcdefghijklmnopqrstuvwxyz1234567890abcdef. The footers should be treated like
      headers, as if they were at the top of the response. The chunks can contain any binary data, and may
      be much larger than the examples here. The size-line parameters are rarely used, but you should at
      least ignore them correctly. Footers are also rare, but might be appropriate for things like checksums
      or digital signatures
      -------------------------------------------------------------------------------------------------------- */

      // manage buffered read

      if (u_line_terminator_len == 1)
         {
         chunk_terminator     = U_LF2;
         chunk_terminator_len = U_CONSTANT_SIZE(U_LF2);
         }
      else
         {
         chunk_terminator     = U_CRLF2;
         chunk_terminator_len = U_CONSTANT_SIZE(U_CRLF2);
         }

      count = rbuffer.find(chunk_terminator, http_info.endHeader, chunk_terminator_len);

      if (count == U_NOT_FOUND) count = USocketExt::read(s, rbuffer, chunk_terminator, chunk_terminator_len);

      if (count == U_NOT_FOUND) U_RETURN(false);

      count += chunk_terminator_len; // NB: il messaggio comprende anche la blank line...

      U_INTERNAL_DUMP("count = %u", count)

      http_info.clength = (count - http_info.endHeader);

      U_INTERNAL_DUMP("http_info.clength = %u", http_info.clength)

      body.setBuffer(http_info.clength);

      inp = rbuffer.c_pointer(http_info.endHeader);
      out = body.data();

      while (true)
         {
         // Decode the hexadecimal chunk size into an understandable number

         chunkSize = strtol(inp, 0, 16);

         U_INTERNAL_DUMP("chunkSize = %u inp[0] = %C", chunkSize, inp[0])

         U_INTERNAL_ASSERT(u_isxdigit(*inp))

         // The last chunk is followed by zero or more trailers, followed by a blank line

         if (chunkSize == 0)
            {
            body.size_adjust(body.distance(out));

            U_INTERNAL_DUMP("body = %.*S", U_STRING_TO_TRACE(body))

            break;
            }

         // discard the rest of the line

         // inp = memchr(inp, '\n', http_info.clength);
         // if (inp == 0) U_RETURN(false);

         while (*inp++ != '\n') {}

         (void) U_SYSCALL(memcpy, "%p,%p,%u", out, inp, chunkSize);

         inp += chunkSize + u_line_terminator_len;
         out += chunkSize;

         U_INTERNAL_ASSERT(inp <= (rbuffer.c_pointer(count)))
         }

      U_RETURN(true);
      }

   body_byte_read = (rbuffer.size() - http_info.endHeader);

   U_INTERNAL_DUMP("rbuffer.size() = %u body_byte_read = %u Content-Length = %u", rbuffer.size(), body_byte_read, http_info.clength)

   if (http_info.clength > body_byte_read)
      {
      UString* pbuffer;

      if (http_info.clength > (64 * 1024 * 1024) && // 64M
          UFile::mkTempStorage(body, http_info.clength))
         {
         (void) U_SYSCALL(memcpy, "%p,%p,%u", body.data(), rbuffer.c_pointer(http_info.endHeader), body_byte_read);

         body.size_adjust(body_byte_read);

         pbuffer = &body;
         }
      else
         {
         pbuffer = &rbuffer;
         }

      // NB: wait max 3 sec for other data...

      if (USocketExt::read(s, *pbuffer, http_info.clength - body_byte_read, 3 * 1000) == false) U_RETURN(false);

      U_INTERNAL_DUMP("pbuffer = %.*S", U_STRING_TO_TRACE(*pbuffer))
      }

end:
   if (body.empty()) body = rbuffer.substr(http_info.endHeader, http_info.clength);

   U_RETURN(true);
}

bool UHTTP::readHTTPRequest()
{
   U_TRACE(0, "UHTTP::readHTTPRequest()")

   if (http_info.method) resetHTTPInfo();

   if (readHTTPHeader(UClientImage_Base::socket, *UClientImage_Base::rbuffer) == false) U_RETURN(false);

   // manage buffered read (pipelining)

   USocketExt::size_message = http_info.endHeader;

   U_INTERNAL_DUMP("size_message = %u", USocketExt::size_message)

   // --------------------------------
   // check in header request for:
   // --------------------------------
   // "Host: ..."
   // "Range: ..."
   // "Connection: ..."
   // "Content-Type: ..."
   // "Content-Length: ..."
   // "Accept-Encoding: ..."
   // "If-Modified-Since: ..."
   // --------------------------------

   static const char header_req[] = {
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0, // ' ',    '!',    '\"',   '#',    '$',    '%',    '&',    '\'',
      0,   0,   0,   0,   0,   0,   0,   0, // '(',    ')',    '*',    '+',    ',',    '-',    '.',    '/',
      0,   0,   0,   0,   0,   0,   0,   0, // '0',    '1',    '2',    '3',    '4',    '5',    '6',    '7',
      0,   0,   0,   0,   0,   0,   0,   0, // '8',    '9',    ':',    ';',    '<',    '=',    '>',    '?',
      0, 'A',   0, 'C',   0,   0,   0,   0, // '@',    'A',    'B',    'C',    'D',    'E',    'F',    'G',
    'H', 'I',   0,   0,   0,   0,   0,   0, // 'H',    'I',    'J',    'K',    'L',    'M',    'N',    'O',
      0,   0, 'R',   0,   0,   0,   0,   0, // 'P',    'Q',    'R',    'S',    'T',    'U',    'V',    'W',
      0,   0,   0,   0,   0,   0,   0,   0, // 'X',    'Y',    'Z',    '[',    '\\',   ']',    '^',    '_',
      0, 'A',   0, 'C',   0,   0,   0,   0, // '`',    'a',    'b',    'c',    'd',    'e',    'f',    'g',
    'H',   0,   0,   0,   0,   0,   0,   0, // 'h',    'i',    'j',    'k',    'l',    'm',    'n',    'o',
      0,   0, 'R',   0,   0,   0,   0,   0, // 'p',    'q',    'r',    's',    't',    'u',    'v',    'w',
      0,   0,   0,   0,   0,   0,   0,   0  // 'x',    'y',    'z',    '{',    '|',    '}',    '~',    '\177'
   };

   U_INTERNAL_ASSERT_DIFFERS(ptrH, 0)
   U_INTERNAL_ASSERT_DIFFERS(ptrR, 0)
   U_INTERNAL_ASSERT_DIFFERS(ptrC, 0)
   U_INTERNAL_ASSERT_DIFFERS(ptrT, 0)
   U_INTERNAL_ASSERT_DIFFERS(ptrL, 0)
   U_INTERNAL_ASSERT_DIFFERS(ptrA, 0)
   U_INTERNAL_ASSERT_DIFFERS(ptrI, 0)

   char c;
   const char* p;
   const char* ptr    = UClientImage_Base::rbuffer->data();
   const char* start  = ptr;
   uint32_t pos, pos1 = http_info.startHeader, pos2, l, char_r = (u_line_terminator_len == 2);

   while (pos1 < http_info.endHeader)
      {
   // U_INTERNAL_DUMP("rbuffer = %.*S", 80, rbuffer.c_pointer(pos1))

      pos2 = UClientImage_Base::rbuffer->find('\n', pos1);

      if (pos2 == U_NOT_FOUND) pos2 = http_info.endHeader;

      if ((c = header_req[(uint32_t)ptr[pos1]]))
         {
         pos = UClientImage_Base::rbuffer->find(':', ++pos1);

         if (pos != U_NOT_FOUND)
            {
            U_INTERNAL_ASSERT_MINOR(pos,pos2)

            p = ptr + pos1;
            l = pos - pos1;

            do { ++pos; } while (pos < http_info.endHeader && u_isspace(ptr[pos]));

            if (c == 'H' &&
                l == 3   && // sizeof("ost")
                memcmp(ptrH, p, l) == 0)
               {
               http_info.host     = (ptr - (ptrdiff_t)start) + (ptrdiff_t)pos;
               http_info.host_len = pos2 - pos - char_r;

               U_INTERNAL_DUMP("http_info.host = %.*S", http_info.host_len, start + (ptrdiff_t)http_info.host)
               }
            else if (c == 'A'                    &&
                     l == 14                     && // sizeof("ccept-Encoding")
                     memcmp(ptrA,   p,   6) == 0 && // 6 -> sizeof("ccept-")
                     u_toupper(p[6]) == 'E'      &&
                     memcmp(ptrA+7, p+7, 7) == 0 && // 7 -> sizeof(       "ncoding")
                     u_find(ptr + pos, 30, U_CONSTANT_TO_PARAM("deflate")) != 0)
               {
#           ifdef HAVE_LIBZ
               U_http_is_accept_deflate = '1';
#           endif

               U_INTERNAL_DUMP("U_http_is_accept_deflate = %C", U_http_is_accept_deflate)
               }
            else if (c == 'C')
               {
               if (l == 9 && // sizeof("onnection")
                   memcmp(ptrC, p, l) == 0)
                  {
                  U_INTERNAL_DUMP("Connection: = %.*S", pos2 - pos - 1, UClientImage_Base::rbuffer->c_pointer(pos))

                  U_INTERNAL_ASSERT_EQUALS(c, 'C')

                  p = ptr + pos;

                  if (U_MEMCMP(p, "close") == 0)
                     {
                     U_http_is_connection_close = U_YES;

                     U_INTERNAL_DUMP("U_http_is_connection_close = %d", U_http_is_connection_close)
                     }
                  else if (U_STRNCASECMP(p, "keep-alive") == 0)
                     {
                     U_http_keep_alive = '1';

                     U_INTERNAL_DUMP("U_http_keep_alive = %C", U_http_keep_alive)
                     }
                  else if (U_MEMCMP(p, "Upgrade") == 0)
                     {
                     U_http_upgrade = '1';

                     U_INTERNAL_DUMP("U_http_upgrade = %C", U_http_upgrade)

                     if (getHTTPHeaderValuePtr(*str_websocket)) // web socket
                        {
                        if (getHTTPHeaderValuePtr(*str_websocket_key1))
                           {
                           U_ASSERT_POINTER(getHTTPHeaderValuePtr(*str_websocket_key2))

                           http_info.clength = 8;
                           }
                        }
                     }
                  }
               else if (l == 11                     && // 11 -> sizeof("ontent-Type")
                        memcmp(ptrT,   p,   7) == 0 && //  7 -> sizeof("ontent-")
                        u_toupper(p[7]) == 'T'      &&
                        memcmp(ptrT+8, p+8, 3) == 0)   //  3 -> sizeof(        "ype")
                  {
                  U_INTERNAL_ASSERT_EQUALS(c, 'C')

                  http_info.content_type     = (ptr - (ptrdiff_t)start) + (ptrdiff_t)pos;
                  http_info.content_type_len = pos2 - pos - char_r;

                  U_INTERNAL_DUMP("Content-Type: = %.*S", http_info.content_type_len, start + (ptrdiff_t)http_info.content_type)
                  }
               else if (l == 13                     && // 13 -> sizeof("ontent-Length")
                        memcmp(ptrL,   p,   7) == 0 && //  7 -> sizeof("ontent-")
                        u_toupper(p[7]) == 'L'      &&
                        memcmp(ptrL+8, p+8, 5) == 0)   //  5 -> sizeof(        "ength")
                  {
                  U_INTERNAL_ASSERT_EQUALS(c, 'C')

                  http_info.clength = (uint32_t) strtoul(ptr + pos, 0, 0);

                  U_INTERNAL_DUMP("Content-Length: = %.*S http_info.clength = %u", 10, ptr + pos, http_info.clength)
                  }
               }
            else if (c == 'I' &&
                     l == 16  && // sizeof("f-Modified-Since")
                     memcmp(ptrI, p, l) == 0)
               {
               http_info.if_modified_since = UDate::getSecondFromTime(ptr + pos, true);

               U_INTERNAL_DUMP("If-Modified-Since = %ld", http_info.if_modified_since)
               }
            else if (c == 'R' &&
                     l == 4   && // sizeof("ange")
                     memcmp(ptrR, p, l) == 0 &&
                     U_STRNEQ(ptr + pos, "bytes="))
               {
               http_info.range     = (ptr - (ptrdiff_t)start) + (ptrdiff_t)pos + U_CONSTANT_SIZE("bytes=");
               http_info.range_len = pos2 - pos - char_r                       - U_CONSTANT_SIZE("bytes=");

               U_INTERNAL_DUMP("Range = %.*S", http_info.range_len, start + (ptrdiff_t)http_info.range)
               }
            }
         }

      pos1 = pos2 + 1;
      }

   // NB: we can have a possible resize of read buffer string...

   bool rbuffer_resize = false;
   uint32_t method_pos, uri_pos, query_pos;

   if (http_info.clength || isHttpPOST())
      {
      method_pos = (http_info.method - start);
         uri_pos = (http_info.uri    - start);
       query_pos = (http_info.query  - start);

      if (readHTTPBody(UClientImage_Base::socket, *UClientImage_Base::rbuffer, *UClientImage_Base::body) == false) U_RETURN(false);

      rbuffer_resize = (start != UClientImage_Base::rbuffer->data());

      U_INTERNAL_DUMP("rbuffer_resize = %b", rbuffer_resize)
      }

   // NB: we can have a possible resize of read buffer string...

   if (rbuffer_resize)
      {
      start = UClientImage_Base::rbuffer->data();

      http_info.method = start + (ptrdiff_t)method_pos;
      http_info.uri    = start + (ptrdiff_t)   uri_pos;

      U_INTERNAL_DUMP("method = %.*S", U_HTTP_METHOD_TO_TRACE)
      U_INTERNAL_DUMP("uri    = %.*S", U_HTTP_URI_TO_TRACE)

      if (http_info.query_len)
         {
         http_info.query = start + (ptrdiff_t)query_pos;

         U_INTERNAL_DUMP("query = %.*S", U_HTTP_QUERY_TO_TRACE)
         }
      }

   if (http_info.host_len)          http_info.host          += (ptrdiff_t)start;
   if (http_info.range_len)         http_info.range         += (ptrdiff_t)start;
   if (http_info.content_type_len)  http_info.content_type  += (ptrdiff_t)start;

   // manage buffered read (pipelining)

   USocketExt::size_message += http_info.clength;

   U_INTERNAL_DUMP("size_message = %u", USocketExt::size_message)

   U_RETURN(true);
}

const char* UHTTP::getHTTPHeaderValuePtr(const UString& name)
{
   U_TRACE(0, "UHTTP::getHTTPHeaderValuePtr(%.*S)", U_STRING_TO_TRACE(name))

   uint32_t header_line = UClientImage_Base::rbuffer->find(name, http_info.startHeader, http_info.szHeader);

// if (header_line == U_NOT_FOUND) header_line = UClientImage_Base::rbuffer->findnocase(name, http_info.startHeader, http_info.szHeader); 

   if (header_line == U_NOT_FOUND) U_RETURN((const char*)0);

   U_INTERNAL_DUMP("header_line = %.*S", 20, UClientImage_Base::rbuffer->c_pointer(header_line))

   const char* ptr_header_value = UClientImage_Base::rbuffer->c_pointer(header_line + name.size() + 2);

   U_RETURN(ptr_header_value);
}

// Accept-Language: en-us,en;q=0.5
// ----------------------------------------------------
// take only the first 2 character (it, en, de fr, ...)

const char* UHTTP::getAcceptLanguage()
{
   U_TRACE(0, "UHTTP::getAcceptLanguage()")

   const char* ptr = getHTTPHeaderValuePtr(*USocket::str_accept_language);

   const char* accept_language = (ptr ? ptr : "en");

   U_INTERNAL_DUMP("accept_language = %.2S", ptr)

   U_RETURN_POINTER(accept_language,const char);
}

void UHTTP::getHTTPInfo(const UString& request, UString& method, UString& uri)
{
   U_TRACE(0, "UHTTP::getHTTPInfo(%.*S,%p,%p)", U_STRING_TO_TRACE(request), &method, &uri)

   U_INTERNAL_DUMP("http_info.uri_len = %u", http_info.uri_len)

   if (http_info.uri_len == 0)
      {
      if (scanfHTTPHeader(request.data()) == false)
         {
         method.clear();
            uri.clear();

         return;
         }

      http_info.startHeader += u_line_terminator_len;
      }

   (void) method.assign(U_HTTP_METHOD_TO_PARAM);
   (void)    uri.assign(U_HTTP_URI_QUERY_TO_PARAM);
}

// param: "[ data expire path domain secure HttpOnly ]"
// ----------------------------------------------------------------------------------------------------------------------------
// string -- Data to put into the cookie        -- must
// int    -- Lifetime of the cookie in HOURS    -- must (0 -> valid until browser exit)
// string -- Path where the cookie can be used  --  opt
// string -- Domain which can read the cookie   --  opt
// bool   -- Secure mode                        --  opt
// bool   -- Only allow HTTP usage              --  opt
// ----------------------------------------------------------------------------------------------------------------------------
// RET: Set-Cookie: ulib_sid=data&expire&HMAC-MD5(data&expire); expires=expire(GMT); path=path; domain=domain; secure; HttpOnly

UString UHTTP::setHTTPCookie(const UString& param)
{
   U_TRACE(0, "UHTTP::setHTTPCookie(%.*S)", U_STRING_TO_TRACE(param))

   /*
   Set-Cookie: NAME=VALUE; expires=DATE; path=PATH; domain=DOMAIN_NAME; secure

   NAME=VALUE
   ------------------------------------------------------------------------------------------------------------------------------------
   This string is a sequence of characters excluding semi-colon, comma and white space. If there is a need to place such data
   in the name or value, some encoding method such as URL style %XX encoding is recommended, though no encoding is defined or required.
   This is the only required attribute on the Set-Cookie header.
   ------------------------------------------------------------------------------------------------------------------------------------

   expires=DATE
   ------------------------------------------------------------------------------------------------------------------------------------
   The expires attribute specifies a date string that defines the valid life time of that cookie. Once the expiration date has been
   reached, the cookie will no longer be stored or given out.
   The date string is formatted as: Wdy, DD-Mon-YYYY HH:MM:SS GMT
   expires is an optional attribute. If not specified, the cookie will expire when the user's session ends.

   Note: There is a bug in Netscape Navigator version 1.1 and earlier. Only cookies whose path attribute is set explicitly to "/" will
   be properly saved between sessions if they have an expires attribute.
   ------------------------------------------------------------------------------------------------------------------------------------

   domain=DOMAIN_NAME
   ------------------------------------------------------------------------------------------------------------------------------------
   When searching the cookie list for valid cookies, a comparison of the domain attributes of the cookie is made with the Internet
   domain name of the host from which the URL will be fetched. If there is a tail match, then the cookie will go through path matching
   to see if it should be sent. "Tail matching" means that domain attribute is matched against the tail of the fully qualified domain
   name of the host. A domain attribute of "acme.com" would match host names "anvil.acme.com" as well as "shipping.crate.acme.com".

   Only hosts within the specified domain can set a cookie for a domain and domains must have at least two (2) or three (3) periods in
   them to prevent domains of the form: ".com", ".edu", and "va.us". Any domain that fails within one of the seven special top level
   domains listed below only require two periods. Any other domain requires at least three. The seven special top level domains are:
   "COM", "EDU", "NET", "ORG", "GOV", "MIL", and "INT".

   The default value of domain is the host name of the server which generated the cookie response.
   ------------------------------------------------------------------------------------------------------------------------------------

   path=PATH
   ------------------------------------------------------------------------------------------------------------------------------------
   The path attribute is used to specify the subset of URLs in a domain for which the cookie is valid. If a cookie has already passed
   domain matching, then the pathname component of the URL is compared with the path attribute, and if there is a match, the cookie is
   considered valid and is sent along with the URL request. The path "/foo" would match "/foobar" and "/foo/bar.html". The path "/" is
   the most general path.

   If the path is not specified, it as assumed to be the same path as the document being described by the header which contains the
   cookie.

   secure
   ------------------------------------------------------------------------------------------------------------------------------------
   If a cookie is marked secure, it will only be transmitted if the communications channel with the host is a secure one. Currently
   this means that secure cookies will only be sent to HTTPS (HTTP over SSL) servers.

   If secure is not specified, a cookie is considered safe to be sent in the clear over unsecured channels. 
   ------------------------------------------------------------------------------------------------------------------------------------

   HttpOnly cookies are a Microsoft extension to the cookie standard. The idea is that cookies marked as httpOnly cannot be accessed
   from JavaScript. This was implemented to stop cookie stealing through XSS vulnerabilities. This is unlike many people believe not
   a way to stop XSS vulnerabilities, but a way to stop one of the possible attacks (cookie stealing) that are possible through XSS.
   */

   time_t expire;
   UVector<UString> vec(param);
   UString set_cookie(U_CAPACITY);

   if (vec.empty())
      {
      expire = u_now.tv_sec - U_ONE_DAY_IN_SECOND;

      set_cookie.snprintf("Set-Cookie: ulib_sid=deleted; expires=%#D", expire);
      }
   else
      {
      uint32_t n = vec.size();

      U_INTERNAL_ASSERT_RANGE(2, n, 6)

      long num_hours = vec[1].strtol();

      expire = (num_hours ? u_now.tv_sec + (num_hours * 60L * 60L) : 0L);

      // HMAC-MD5(data&expire)

      UString item, token = UServices::generateToken(vec[0], expire);

      set_cookie.snprintf("Set-Cookie: ulib_sid=%.*s", U_STRING_TO_TRACE(token));

      if (num_hours) set_cookie.snprintf_add("; expires=%#D", expire);

      for (uint32_t i = 2; i < n; ++i)
         {
         item = vec[i];

         if (item.empty() == false)
            {
            switch (i)
               {
               case 2: set_cookie.snprintf_add("; path=%.*s",   U_STRING_TO_TRACE(item)); break; // path
               case 3: set_cookie.snprintf_add("; domain=%.*s", U_STRING_TO_TRACE(item)); break; // domain
               case 4: (void) set_cookie.append(U_CONSTANT_TO_PARAM("; secure"));         break; // secure
               case 5: (void) set_cookie.append(U_CONSTANT_TO_PARAM("; HttpOnly"));       break; // HttpOnly
               }
            }
         }
      }

   U_INTERNAL_DUMP("set_cookie = %.*S", U_STRING_TO_TRACE(set_cookie))

   U_RETURN_STRING(set_cookie);
}

UString UHTTP::getHTTPCookie()
{
   U_TRACE(1, "UHTTP::getHTTPCookie()")

   const char* cookie_ptr = getHTTPHeaderValuePtr(*USocket::str_cookie);

   if (cookie_ptr)
      {
      uint32_t cookie_len = 0;

      for (char c = u_line_terminator[0]; cookie_ptr[cookie_len] != c; ++cookie_len) {}

      U_INTERNAL_DUMP("cookie = %.*S", cookie_len, cookie_ptr)

      if (U_STRNEQ(cookie_ptr, "ulib_sid="))
         {
         const char* token = cookie_ptr + U_CONSTANT_SIZE("ulib_sid=");

         UString data = UServices::getTokenData(token);

         U_RETURN_STRING(data);
         }

      if (U_http_sh_script == false)
         {
         UString result(cookie_ptr, cookie_len);

         U_RETURN_STRING(result);
         }
      }

   U_RETURN_STRING(UString::getStringNull());
}

const char* UHTTP::getHTTPStatusDescription(uint32_t nResponseCode)
{
   U_TRACE(0, "UHTTP::getHTTPStatusDescription(%u)", nResponseCode)

   const char* descr;

   switch (nResponseCode)
      {
      // 1xx indicates an informational message only
      case HTTP_CONTINUE:           descr = "Continue";                        break;
      case HTTP_SWITCH_PROT:        descr = "Switching Protocol";              break;
   // case 102:                     descr = "HTTP Processing";                 break;

      // 2xx indicates success of some kind
      case HTTP_OK:                 descr = "OK";                              break;
      case HTTP_CREATED:            descr = "Created";                         break;
      case HTTP_ACCEPTED:           descr = "Accepted";                        break;
      case HTTP_NOT_AUTHORITATIVE:  descr = "Non-Authoritative Information";   break;
      case HTTP_NO_CONTENT:         descr = "No Content";                      break;
      case HTTP_RESET:              descr = "Reset Content";                   break;
      case HTTP_PARTIAL:            descr = "Partial Content";                 break;
   // case 207:                     descr = "Webdav Multi-status";             break;

      // 3xx Redirection - Further action must be taken in order to complete the request
      case HTTP_MULT_CHOICE:        descr = "Multiple Choices";                break;
      case HTTP_MOVED_PERM:         descr = "Moved Permanently";               break;
      case HTTP_MOVED_TEMP:         descr = "Moved Temporarily";               break;
   // case HTTP_FOUND:              descr = "Found [Elsewhere]";               break;
      case HTTP_SEE_OTHER:          descr = "See Other";                       break;
      case HTTP_NOT_MODIFIED:       descr = "Not Modified";                    break;
      case HTTP_USE_PROXY:          descr = "Use Proxy";                       break;
      case HTTP_TEMP_REDIR:         descr = "Temporary Redirect";              break;

      // 4xx indicates an error on the client's part
      case HTTP_BAD_REQUEST:        descr = "Bad Request";                     break;
      case HTTP_UNAUTHORIZED:       descr = "Authorization Required";          break;
      case HTTP_PAYMENT_REQUIRED:   descr = "Payment Required";                break;
      case HTTP_FORBIDDEN:          descr = "Forbidden";                       break;
      case HTTP_NOT_FOUND:          descr = "Not Found";                       break;
      case HTTP_BAD_METHOD:         descr = "Method Not Allowed";              break;
      case HTTP_NOT_ACCEPTABLE:     descr = "Not Acceptable";                  break;
      case HTTP_PROXY_AUTH:         descr = "Proxy Authentication Required";   break;
      case HTTP_CLIENT_TIMEOUT:     descr = "Request Time-out";                break;
      case HTTP_CONFLICT:           descr = "Conflict";                        break;
      case HTTP_GONE:               descr = "Gone";                            break;
      case HTTP_LENGTH_REQUIRED:    descr = "Length Required";                 break;
      case HTTP_PRECON_FAILED:      descr = "Precondition Failed";             break;
      case HTTP_ENTITY_TOO_LARGE:   descr = "Request Entity Too Large";        break;
      case HTTP_REQ_TOO_LONG:       descr = "Request-URI Too Long";            break;
      case HTTP_UNSUPPORTED_TYPE:   descr = "Unsupported Media Type";          break;
      case HTTP_REQ_RANGE_NOT_OK:   descr = "Requested Range not satisfiable"; break;
      case HTTP_EXPECTATION_FAILED: descr = "Expectation Failed";              break;
   // case 422:                     descr = "Unprocessable Entity";            break;
   // case 423:                     descr = "Locked";                          break;
   // case 424:                     descr = "Failed Dependency";               break;
   // case 425:                     descr = "No Matching Vhost";               break;
   // case 426:                     descr = "Upgrade Required";                break;
   // case 449:                     descr = "Retry With Appropriate Action";   break;

      // 5xx indicates an error on the server's part
      case HTTP_INTERNAL_ERROR:     descr = "Internal Server Error";           break;
      case HTTP_NOT_IMPLEMENTED:    descr = "Not Implemented";                 break;
      case HTTP_BAD_GATEWAY:        descr = "Bad Gateway";                     break;
      case HTTP_UNAVAILABLE:        descr = "Service Unavailable";             break;
      case HTTP_GATEWAY_TIMEOUT:    descr = "Gateway Time-out";                break;
      case HTTP_VERSION:            descr = "HTTP Version Not Supported";      break;
   // case 506:                     descr = "Variant also varies";             break;
   // case 507:                     descr = "Insufficient Storage";            break;
   // case 510:                     descr = "Not Extended";                    break;

      default:                      descr = "Code unknown";                    break;
      }

   U_RETURN(descr);
}

const char* UHTTP::getHTTPStatus()
{
   U_TRACE(0, "UHTTP::getHTTPStatus()")

   static char buffer[128];

   (void) sprintf(buffer, "(%d, %s)", http_info.nResponseCode, getHTTPStatusDescription(http_info.nResponseCode));

   U_RETURN(buffer);
}

void UHTTP::getTimeIfNeeded(bool all_http_version)
{
   U_TRACE(0, "UHTTP::getTimeIfNeeded(%b)", all_http_version)

   if (ULog::isSysLog() ||
       UServer_Base::isLog() == false)
      {
      // HTTP 1.1 want header "Date: ..."

      if (U_http_version == '1' || all_http_version) u_check_now_adjust();
      }
}

U_NO_EXPORT UString UHTTP::getHTMLDirectoryList()
{
   U_TRACE(0, "UHTTP::getHTMLDirectoryList()")

   UString buffer(4000U);

   buffer.snprintf("<html><head><title></title></head>"
                   "<body><h1>Index of directory: %.*s</h1><hr>"
                     "<table><tr>"
                        "<td><a href=\"/%.*s/..\"><img align=\"absbottom\" border=\"0\" src=\"/icons/menu.gif\"> Up one level</a></td>"
                        "<td></td>"
                        "<td></td>"
                     "</tr>", U_FILE_TO_TRACE(*file), U_FILE_TO_TRACE(*file));

   if (UFile::chdir(file->getPathRelativ(), true))
      {
      bool is_dir;
      UVector<UString> vec;
      UString item, size, entry(4000U), value_encoded(U_CAPACITY);
      uint32_t pos = buffer.size(), n = UFile::listContentOf(vec);

      if (n > 1) vec.sort();

      for (uint32_t i = 0; i < n; ++i)
         {
         item = vec[i];

         pathname->setBuffer(file->getPathRelativLen() + 1 + item.size());

         pathname->snprintf("%.*s/%.*s", U_FILE_TO_TRACE(*file), U_STRING_TO_TRACE(item));

         file_data = (*cache_file)[*pathname];

         U_INTERNAL_ASSERT_POINTER(file_data)

         is_dir = S_ISDIR(file_data->mode);

         Url::encode(item, value_encoded);

         size = UStringExt::numberToString(file_data->size, true);

         entry.snprintf("<tr>"
                           "<td><a href=\"/%.*s/%.*s\"><img align=\"absbottom\" border=\"0\" src=\"/icons/%s.gif\"> %.*s</a></td>"
                           "<td align=\"right\" valign=\"bottom\">%.*s</td>"
                           "<td align=\"right\" valign=\"bottom\">%#4D</td>"
                         "</tr>",
                         U_FILE_TO_TRACE(*file), U_STRING_TO_TRACE(value_encoded), (is_dir ? "menu" : "gopher-unknown"), U_STRING_TO_TRACE(item),
                         U_STRING_TO_TRACE(size),
                         file_data->mtime);

         if (is_dir)
            {
            (void) buffer.insert(pos, entry);

            pos += entry.size();
            }
         else
            {
            (void) buffer.append(entry);
            }
         }

      (void) UFile::chdir(0, true);

      (void) buffer.append(U_CONSTANT_TO_PARAM("</table><hr><address>ULib Server</address>\r\n</body></html>"));
      }

   U_INTERNAL_DUMP("buffer(%u) = %.*S", buffer.size(), U_STRING_TO_TRACE(buffer))

   U_RETURN_STRING(buffer);
}

U_NO_EXPORT void UHTTP::resetForm(bool brmdir)
{
   U_TRACE(0, "UHTTP::resetForm(%b)", brmdir)

   U_INTERNAL_ASSERT_POINTER(tmpdir)
   U_INTERNAL_ASSERT_POINTER(qcontent)
   U_INTERNAL_ASSERT_POINTER(formMulti)
   U_INTERNAL_ASSERT_POINTER(form_name_value)

   if (qcontent->empty() == false) qcontent->clear();
   else
      {
      // clean temporary directory, if any...

      if (tmpdir->empty() == false)
         {
         if (brmdir) (void) UFile::rmdir(tmpdir->data(), true);

         tmpdir->setEmpty();
         }
      else if (json)
         {
         delete json;
                json = 0;
         }

      if (formMulti->isEmpty() == false) formMulti->clear();
      }

   form_name_value->clear();
}

// retrieve information on specific HTML form elements
// (such as checkboxes, radio buttons, and text fields, or uploaded files)

void UHTTP::getFormValue(UString& buffer, uint32_t n)
{
   U_TRACE(0, "UHTTP::getFormValue(%.*S,%u)", U_STRING_TO_TRACE(buffer), n)

   U_INTERNAL_ASSERT_POINTER(form_name_value)

   if (n >= form_name_value->size())
      {
      buffer.setEmpty();

      return;
      }

   (void) buffer.replace((*form_name_value)[n]);
}

uint32_t UHTTP::processHTTPForm()
{
   U_TRACE(0, "UHTTP::processHTTPForm()")

   uint32_t n = 0;

   if (isHttpGETorHEAD())
      {
      *qcontent = UString(U_HTTP_QUERY_TO_PARAM);

      goto get_name_value;
      }

   // ------------------------------------------------------------------------
   // POST
   // ------------------------------------------------------------------------
   // Content-Type: application/x-www-form-urlencoded OR multipart/form-data...
   // ------------------------------------------------------------------------

   U_ASSERT(isHttpPOST())

   if (U_HTTP_CTYPE_STRNEQ("application/x-www-form-urlencoded"))
      {
      *qcontent = *UClientImage_Base::body;

      goto get_name_value;
      }

   if (U_HTTP_CTYPE_STRNEQ("application/jsonrequest"))
      {
      json = U_NEW(UValue);

      (void) json->parse(*UClientImage_Base::body);

      U_RETURN(1);
      }

   // multipart/form-data (FILE UPLOAD)

   U_INTERNAL_ASSERT(U_HTTP_CTYPE_STRNEQ("multipart/form-data"))

   {
   UString boundary, tmp = UClientImage_Base::body->substr(2);
   UTokenizer(tmp, u_line_terminator).next(boundary, (bool*)0);

   formMulti->setBoundary(boundary);
   formMulti->setContent(*UClientImage_Base::body);

   if (formMulti->parse() == false) U_RETURN(0);
   }

   // create temporary directory with files upload...

   {
   tmpdir->snprintf("%s/formXXXXXX", u_tmpdir);

   if (UFile::mkdtemp(*tmpdir) == false) U_RETURN(0);

   UMimeEntity* item;
   UString content, name, filename, basename;

   for (uint32_t i = 0, j = formMulti->getNumBodyPart(); i < j; ++i)
      {
      item    = (*formMulti)[i];
      content = item->getContent();

      // Content-Disposition: form-data; name="input_file"; filename="/tmp/4dcd39e8-2a84-4242-b7bc-ca74922d26e1"

      if (UMimeHeader::getNames(item->getContentDisposition(), name, filename))
         {
         // NB: we can't reuse the same string (filename) to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...

         basename = UStringExt::basename(filename);

         pathname->setBuffer(tmpdir->size() + 1 + basename.size());

         pathname->snprintf("%.*s/%.*s", U_STRING_TO_TRACE(*tmpdir), U_STRING_TO_TRACE(basename));

         (void) UFile::writeTo(*pathname, content);

         content = *pathname;
         }

      form_name_value->push_back(name);
      form_name_value->push_back(content);
      }

   n = form_name_value->size();

   U_RETURN(n);
   }

get_name_value:

   if (qcontent->empty() == false) n = UStringExt::getNameValueFromData(*qcontent, *form_name_value);

   U_RETURN(n);
}

// set HTTP main error message
// --------------------------------------------------------------------------------------------------------------------------------------

UString UHTTP::getHTTPHeaderForResponse(int nResponseCode, const UString& content)
{
   U_TRACE(0, "UHTTP::getHTTPHeaderForResponse(%d,%.*S)", nResponseCode, U_STRING_TO_TRACE(content))

   U_INTERNAL_ASSERT_MAJOR(nResponseCode,0)

   // NB: All 1xx (informational), 204 (no content), and 304 (not modified) responses MUST not include a body...

#ifdef DEBUG
   if ((nResponseCode >= 100  &&
        nResponseCode <  200) ||
        nResponseCode == 204  ||
        nResponseCode == 304)
      {
      U_ASSERT(content.empty())
      }
#endif

   uint32_t sz;
   const char* ptr;

   if (nResponseCode == HTTP_NOT_IMPLEMENTED)
      {
      ptr =                 "Allow: GET, HEAD, POST, PUT, DELETE\r\nContent-Length: 0\r\n\r\n";
      sz  = U_CONSTANT_SIZE("Allow: GET, HEAD, POST, PUT, DELETE\r\nContent-Length: 0\r\n\r\n");
      }
   else
      {
      // ...all other responses must include an entity body or a Content-Length header field defined with a value of zero (0)

      if ((sz = content.size())) ptr = content.data();
      else
         {
         ptr =                 "Content-Length: 0\r\n\r\n";
         sz  = U_CONSTANT_SIZE("Content-Length: 0\r\n\r\n");
         }
      }

   U_INTERNAL_DUMP("U_http_version = %C U_http_keep_alive = %C U_http_is_connection_close = %d",
                    U_http_version,     U_http_keep_alive,     U_http_is_connection_close)

   /*
   Max Keep-Alive Requests

   Description: Specifies the maximum number of requests that can be served through a Keep-Alive (Persistent) session.
                Connection will be closed once this limit is reached.

   Syntax: Integer number

   Tips: [Performance] Set it to a resonable high value (256). Value <= 1 will disable Keep-Alive.
   */

   if (UServer_Base::isMaxKeepAlive())
      {
      U_http_keep_alive          = 0;
      U_http_is_connection_close = U_YES;
      }

   UString tmp(300U + sz), connection(200U);

   // HTTP/1.1 compliance: Sends Date on every requests...

   if (U_http_version == '1') connection.snprintf("Date: %D\r\n", 0);
   else
      {
      // HTTP/1.0 compliance: if not Keep-Alive we force close...

      if (U_http_keep_alive == '\0') U_http_is_connection_close = U_YES;
      else
         {
         // ...to indicate that it desires a multiple-request session

         (void) connection.append(U_CONSTANT_TO_PARAM("Connection: keep-alive\r\n"));

         /*
         Keep-Alive Timeout

         Description: Specifies the maximum idle time between requests from a Keep-Alive connection. If no new request is received during
                      this period of time, the connection will be closed.

         Syntax:      Integer number

         Tips: [Security & Performance] We recommend you to set the value just long enough to handle all requests for a single page view.
               It is unnecessary to keep connection alive for an extended period of time. A smaller value can reduce idle connections, increase
               capacity to service more users and guard against DoS attacks. 2-5 seconds is a reasonable range for most applications.
         */

         if (UServer_Base::getReqTimeout())
            {
            // ...to indicate that the session is being kept alive for a maximum of x requests and a per-request timeout of x seconds

            connection.snprintf_add("Keep-Alive: max=%d, timeout=%d\r\n", UServer_Base::getMaxKeepAlive(), UServer_Base::getReqTimeout());
            }
         }
      }

   if (U_http_is_connection_close            == U_YES &&
       UClientImage_Base::checkForPipeline() == false)
      {
      (void) connection.append(U_CONSTANT_TO_PARAM("Connection: close\r\n"));
      }

   tmp.snprintf(str_frm_header->data(),
                (U_http_version ? U_http_version : '0'),
                nResponseCode, getHTTPStatusDescription(nResponseCode),
                U_STRING_TO_TRACE(connection),
                sz, ptr);

   U_INTERNAL_DUMP("tmp(%u) = %.*S", tmp.size(), U_STRING_TO_TRACE(tmp))

   U_RETURN_STRING(tmp);
}

void UHTTP::setHTTPResponse(int nResponseCode, const UString* content_type, const UString* body)
{
   U_TRACE(0, "UHTTP::setHTTPResponse(%d,%p,%p)", nResponseCode, content_type, body)

   U_INTERNAL_ASSERT_MAJOR(nResponseCode,0)

   UString tmp(U_CAPACITY);

   if (content_type)
      {
      if (body) tmp.snprintf("Content-Length: %u\r\n", body->size());

      (void) tmp.append(U_CONSTANT_TO_PARAM("Content-Type: "));
      (void) tmp.append(*content_type);
      (void) tmp.append(U_CONSTANT_TO_PARAM("\r\n\r\n"));
      }

   if (body) *UClientImage_Base::body = *body;
   else       UClientImage_Base::body->clear(); // clean body to avoid writev() in response...

   *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(nResponseCode, tmp);

   U_INTERNAL_DUMP("UClientImage_Base::wbuffer(%u) = %.*S", UClientImage_Base::wbuffer->size(), U_STRING_TO_TRACE(*UClientImage_Base::wbuffer))
   U_INTERNAL_DUMP("UClientImage_Base::body(%u)    = %.*S", UClientImage_Base::body->size(),    U_STRING_TO_TRACE(*UClientImage_Base::body))
}

void UHTTP::setHTTPForbidden()
{
   U_TRACE(0, "UHTTP::setHTTPForbidden()")

   U_http_is_connection_close = U_YES;

   UString msg(100U + http_info.uri_len), body(500U + http_info.uri_len);

   msg.snprintf("You don't have permission to access %.*s on this server", U_HTTP_URI_TO_TRACE);

   const char* status = getHTTPStatusDescription(HTTP_FORBIDDEN);

   body.snprintf(str_frm_body->data(),
                  HTTP_FORBIDDEN, status,
                  status,
                  U_STRING_TO_TRACE(msg));

   setHTTPResponse(HTTP_FORBIDDEN, str_ctype_html, &body);
}

void UHTTP::setHTTPNotFound()
{
   U_TRACE(0, "UHTTP::setHTTPNotFound()")

   U_http_is_connection_close = U_YES;

   UString msg(100U + http_info.uri_len), body(500U + http_info.uri_len);

   msg.snprintf("The requested URL %.*s was not found on this server", U_HTTP_URI_TO_TRACE);

   const char* status = getHTTPStatusDescription(HTTP_NOT_FOUND);

   body.snprintf(str_frm_body->data(),
                  HTTP_NOT_FOUND, status,
                  status,
                  U_STRING_TO_TRACE(msg));

   setHTTPResponse(HTTP_NOT_FOUND, str_ctype_html, &body);
}

/* see: http://sebastians-pamphlets.com/the-anatomy-of-http-redirects-301-302-307/
 * ------------------------------------------------------------------------------------------------------------------
 * HTTP/1.0
 * ------------------------------------------------------------------------------------------------------------------
 * 302 Moved Temporarily
 *
 * The requested resource resides temporarily under a different URL. Since the redirection may be altered on occasion,
 * the client should continue to use the Request-URI for future requests. The URL must be given by the Location field
 * in the response. Unless it was a HEAD request, the Entity-Body of the response should contain a short note with a
 * hyperlink to the new URI(s).
 * ------------------------------------------------------------------------------------------------------------------
 * HTTP/1.1
 * ------------------------------------------------------------------------------------------------------------------
 * 302 Found [Elsewhere]
 *
 * The requested resource resides temporarily under a different URI. Since the redirection might be altered on occasion,
 * the client SHOULD continue to use the Request-URI for future requests. This response is only cacheable if indicated
 * by a Cache-Control or Expires header field. The temporary URI SHOULD be given by the Location field in the response.
 * Unless the request method was HEAD, the entity of the response SHOULD contain a short hypertext note with a hyperlink
 * to the new URI(s).
 *
 * 307 Temporary Redirect
 *
 * The requested resource resides temporarily under a different URI. Since the redirection MAY be altered on occasion,
 * the client SHOULD continue to use the Request-URI for future requests. This response is only cacheable if indicated by
 * a Cache-Control or Expires header field. The temporary URI SHOULD be given by the Location field in the response. Unless
 * the request method was HEAD, the entity of the response SHOULD contain a short hypertext note with a hyperlink to the new
 * URI(s), since many pre-HTTP/1.1 user agents do not understand the 307 status. Therefore, the note SHOULD contain the
 * information necessary for a user to repeat the original request on the new URI.
 */

void UHTTP::setHTTPRedirectResponse(UString& ext, const char* ptr_location, uint32_t len_location)
{
   U_TRACE(0, "UHTTP::setHTTPRedirectResponse(%.*S,%.*S,%u)", U_STRING_TO_TRACE(ext), len_location, ptr_location, len_location)

   U_ASSERT_EQUALS(u_find(ptr_location,len_location,"\n",1),0)

   // NB: firefox chiede conferma all'utente con 307

   int nResponseCode =                                            HTTP_MOVED_TEMP;
// int nResponseCode = (U_http_version == '1' ? HTTP_TEMP_REDIR : HTTP_MOVED_TEMP);

   if (U_http_is_connection_close == U_MAYBE) U_http_is_connection_close = U_YES;

   UString tmp(U_CAPACITY), msg(100U + len_location), body(500U + len_location);

   msg.snprintf("The document has moved <a href=\"%.*s\">here</a>", len_location, ptr_location);

   const char* status = getHTTPStatusDescription(nResponseCode);

   body.snprintf(str_frm_body->data(),
                  nResponseCode, status,
                  status,
                  U_STRING_TO_TRACE(msg));

   (void) tmp.assign(U_CONSTANT_TO_PARAM(U_CTYPE_HTML "\r\nLocation: "));
   (void) tmp.append(ptr_location, len_location);

   if (ext.empty() == false)
      {
      (void) tmp.append(U_CONSTANT_TO_PARAM("\r\n"));
      (void) tmp.append(UStringExt::trim(ext));

#  ifdef DEBUG
      ext.clear(); // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...
#  endif
      }

   setHTTPResponse(nResponseCode, &tmp, &body);
}

void UHTTP::setHTTPBadRequest()
{
   U_TRACE(0, "UHTTP::setHTTPBadRequest()")

   U_http_is_connection_close = U_YES;

   UString msg(100U + http_info.uri_len), body(500U + http_info.uri_len);

   msg.snprintf("Your requested URL %.*s was a request that this server could not understand", U_HTTP_URI_TO_TRACE);

   const char* status = getHTTPStatusDescription(HTTP_BAD_REQUEST);

   body.snprintf(str_frm_body->data(),
                  HTTP_BAD_REQUEST, status,
                  status,
                  U_STRING_TO_TRACE(msg));

   setHTTPResponse(HTTP_BAD_REQUEST, str_ctype_html, &body);
}

void UHTTP::setHTTPUnAuthorized()
{
   U_TRACE(0, "UHTTP::setHTTPUnAuthorized()")

#ifdef HAVE_SSL
   UString ext(100U), body(500U);

   body.snprintf(str_frm_body->data(),
                  HTTP_UNAUTHORIZED, getHTTPStatusDescription(HTTP_UNAUTHORIZED),
                  "Sorry, Password Required",
                  U_CONSTANT_TO_TRACE("An account (with a password) is required to view the page that you requested"));

   (void) ext.assign(U_CONSTANT_TO_PARAM(U_CTYPE_HTML "\r\nWWW-Authenticate: "));

   if (digest_authentication)        ext.snprintf_add("Digest qop=\"auth\", nonce=\"%ld\", algorithm=MD5,", u_now.tv_sec);
   else                       (void) ext.append(U_CONSTANT_TO_PARAM("Basic"));

   (void) ext.append(U_CONSTANT_TO_PARAM(" realm=\"" U_HTTP_REALM "\""));

   setHTTPResponse(HTTP_UNAUTHORIZED, &ext, &body);
#else
   setHTTPForbidden();
#endif
}

void UHTTP::setHTTPInternalError()
{
   U_TRACE(0, "UHTTP::setHTTPInternalError()")

   U_http_is_connection_close = U_YES;

   UString body(1000U);

   const char* status = getHTTPStatusDescription(HTTP_INTERNAL_ERROR);

   body.snprintf(str_frm_body->data(),
                 HTTP_INTERNAL_ERROR, status,
                 status,
                 U_CONSTANT_TO_TRACE("The server encountered an internal error or misconfiguration "
                                     "and was unable to complete your request. Please contact the server "
                                     "administrator, and inform them of the time the error occurred, and "
                                     "anything you might have done that may have caused the error. More "
                                     "information about this error may be available in the server error log"));

   setHTTPResponse(HTTP_INTERNAL_ERROR, str_ctype_html, &body);
}

void UHTTP::setHTTPServiceUnavailable()
{
   U_TRACE(0, "UHTTP::setHTTPServiceUnavailable()")

   U_http_is_connection_close = U_YES;

   UString body(500U);

   const char* status = getHTTPStatusDescription(HTTP_UNAVAILABLE);

   body.snprintf(str_frm_body->data(),
                  HTTP_UNAVAILABLE, status,
                  status,
                  U_CONSTANT_TO_TRACE("Sorry, the service you requested is not available at this moment. "
                                      "Please contact the server administrator and inform them about this"));

   setHTTPResponse(HTTP_UNAVAILABLE, str_ctype_html, &body);
}

void UHTTP::setHTTPCgiResponse(int nResponseCode, bool header_content_length, bool header_content_type, bool content_encoding)
{
   U_TRACE(0, "UHTTP::setHTTPCgiResponse(%d,%b,%b,%b)", nResponseCode, header_content_length, header_content_type, content_encoding)

   U_ASSERT_EQUALS(UClientImage_Base::wbuffer->empty(), false)

   UString tmp(4000U);

   U_INTERNAL_DUMP("http_info.clength = %u", http_info.clength)

   if (http_info.clength == 0)
      {
      // NB: no body...it's ok Content-Length: 0...
      }
   else if (header_content_length)
      {
      // NB: there is body...it's KO Content-Length: 0...

      (void) tmp.assign(U_CONSTANT_TO_PARAM("X-Powered-By: ULib/1.0\r\n"));
      }
   else
      {
      if (content_encoding                ||
          http_info.clength        <= 100 ||
          U_http_is_accept_deflate == '\0')
         {
         tmp.snprintf("Content-Length: %u\r\n"
                      "%s",
                      http_info.clength,
                      (header_content_type ? "" : "Content-Type: " U_CTYPE_HTML "\r\n\r\n"));
         }
      else
         {
         const char* ptr;
         uint32_t sz, endHeader = UClientImage_Base::wbuffer->size() - http_info.clength;

         U_INTERNAL_DUMP("endHeader = %u", endHeader)

         UString content  = UClientImage_Base::wbuffer->substr(endHeader),
                 compress = UStringExt::deflate(content);

         http_info.clength = compress.size();

         if (endHeader)
            {
            // NB: endHeader comprende anche la blank line...
         
            ptr = UClientImage_Base::wbuffer->data();
            sz  = endHeader;
            }
         else
            {
            ptr =                 U_CRLF;
            sz  = U_CONSTANT_SIZE(U_CRLF);
            }

         tmp.snprintf("%s"
                      "Content-Length: %u\r\n"
                      "Content-Encoding: deflate\r\n"
                      "%.*s",
                      (header_content_type ? "" : "Content-Type: " U_CTYPE_HTML "\r\n"),
                      http_info.clength,
                      sz, ptr);

#     ifdef DEBUG
         content.clear(); // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...
#     endif

         *UClientImage_Base::wbuffer = compress;
         }
      }

   *UClientImage_Base::body    = *UClientImage_Base::wbuffer;
   *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(nResponseCode, tmp);
}

// --------------------------------------------------------------------------------------------------------------------------------------

U_NO_EXPORT bool UHTTP::openFile()
{
   U_TRACE(0, "UHTTP::openFile()")

   bool result;

   if (file->dir())
      {
      // NB: cgi-bin and usp are forbidden...

      result = (u_endsWith(U_FILE_TO_PARAM(*file), U_CONSTANT_TO_PARAM("usp"))     == false &&
                u_endsWith(U_FILE_TO_PARAM(*file), U_CONSTANT_TO_PARAM("cgi-bin")) == false);

      if (result)
         {
         // Check if there is an index file (index.html) in the directory...

         pathname->setBuffer(file->getPathRelativLen() + 1 + str_indexhtml->size());

         pathname->snprintf("%.*s/%.*s", U_FILE_TO_TRACE(*file), U_STRING_TO_TRACE(*str_indexhtml));

         file_data = (*cache_file)[*pathname];

         if (file_data)
            {
            if (isDataFromCache(false))
               {
               bool deflate = ((U_http_is_accept_deflate == '1') && isDataFromCache(true));

               if (isHttpHEAD() == false) *UClientImage_Base::body    = getDataFromCache(false, deflate);
                                          *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(HTTP_OK, getDataFromCache(true, deflate));

               U_RETURN(false);
               }

            file->setPath(*pathname);

            file->st_size  = file_data->size;
            file->st_mode  = file_data->mode;
            file->st_mtime = file_data->mtime;

            result = (file->regular() && file->open());

            goto end;
            }

         /* Nope, no index file, so it's an actual directory request */

         result = (file->getPathRelativLen() > 1); // NB: '/' alias '.' is forbidden...
         }
      }
   else
      {
      result = (file->regular()                             &&
                file->isPathRelativ(*str_htpasswd) == false && // NB: '.htpasswd' is forbidden
                file->isPathRelativ(*str_htdigest) == false && // NB: '.htdigest' is forbidden
                file->open());
      }

end:
   if (result == false) setHTTPForbidden(); // set forbidden error response...

   U_RETURN(result);
}

U_NO_EXPORT bool UHTTP::processHTTPAuthorization()
{
   U_TRACE(0, "UHTTP::processHTTPAuthorization()")

   bool result = false;

#ifdef HAVE_SSL
   const char* ptr = getHTTPHeaderValuePtr(*USocket::str_authorization);

   if (ptr == 0) U_RETURN(false);

   if (digest_authentication)
      {
      if (U_STRNCMP(ptr, "Digest ")) U_RETURN(false);

      ptr += U_CONSTANT_SIZE("Digest ");
      }
   else
      {
      if (U_STRNCMP(ptr, "Basic "))  U_RETURN(false);

      ptr += U_CONSTANT_SIZE("Basic ");
      }

   UString content, tmp = UClientImage_Base::rbuffer->substr(UClientImage_Base::rbuffer->distance(ptr));
   UTokenizer t(tmp, u_line_terminator);

   if (t.next(content, (bool*)0) == false) U_RETURN(false);

   UString user(100U);

   if (digest_authentication)
      {
      // Authorization: Digest username="s.casazza", realm="Protected Area", nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093", uri="/",
      //                       response="a74c1cb52877766bb0781d12b653d1a7", qop=auth, nc=00000001, cnonce="73ed2d9694b46324", algorithm=MD5

      UVector<UString> name_value;
      UString name, value, realm, nonce, uri, response, qop, nc, cnonce;

      for (int32_t i = 0, n = UStringExt::getNameValueFromData(content, name_value, U_CONSTANT_TO_PARAM(", \t")); i < n; i += 2)
         {
         name  = name_value[i];
         value = name_value[i+1];

         U_INTERNAL_DUMP("name = %.*S value = %.*S", U_STRING_TO_TRACE(name), U_STRING_TO_TRACE(value))

         switch (name.c_char(0))
            {
            case 'u':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("username")))
                  {
                  U_ASSERT(user.empty())

                  user = value;
                  }
               else if (name.equal(U_CONSTANT_TO_PARAM("uri")))
                  {
                  U_ASSERT(uri.empty())

                  // NB: there may be an ALIAS...

                  if (value != getRequestURI(true)) U_RETURN(false);

                  uri = value;
                  }
               }
            break;

            case 'r':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("realm")))
                  {
                  U_ASSERT(realm.empty())

                  realm = value;
                  }
               else if (name.equal(U_CONSTANT_TO_PARAM("response")))
                  {
                  U_ASSERT(response.empty())

                  if (value.size() != 32) U_RETURN(false);

                  response = value;
                  }
               }
            break;

            case 'n':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("nonce")))
                  {
                  U_ASSERT(nonce.empty())

                  // XXX: Due to a bug in MSIE (version=??), we do not check for authentication timeout...

                  if ((u_now.tv_sec - value.strtol()) > 3600) U_RETURN(false);

                  nonce = value;
                  }
               else if (name.equal(U_CONSTANT_TO_PARAM("nc")))
                  {
                  U_ASSERT(nc.empty())

                  nc = value;
                  }
               }
            break;

            case 'q':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("qop")))
                  {
                  U_ASSERT(qop.empty())

                  qop = value;
                  }
               }
            break;

            case 'c':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("cnonce")))
                  {
                  U_ASSERT(cnonce.empty())

                  cnonce = value;
                  }
               }
            break;

            case 'a':
               {
               if (name.equal(U_CONSTANT_TO_PARAM("algorithm")))
                  {
                  if (value.equal("MD5") == false) U_RETURN(false);
                  }
               }
            break;
            }
         }

      UString  a2(4 + 1 + uri.size()),       //     method : uri
              ha2(33U),                      // MD5(method : uri)
              ha1 = getUserHA1(user, realm), // MD5(user : realm : password)
              a3(200U), ha3(33U);

      // MD5(method : uri)

      a2.snprintf("%.*s:%.*s", U_HTTP_METHOD_TO_TRACE, U_STRING_TO_TRACE(uri));

      UServices::generateDigest(U_HASH_MD5, 0, a2, ha2, false);

      // MD5(HA1 : nonce : nc : cnonce : qop : HA2)

      a3.snprintf("%.*s:%.*s:%.*s:%.*s:%.*s:%.*s", U_STRING_TO_TRACE(ha1), U_STRING_TO_TRACE(nonce),
                                                   U_STRING_TO_TRACE(nc),  U_STRING_TO_TRACE(cnonce),
                                                   U_STRING_TO_TRACE(qop), U_STRING_TO_TRACE(ha2));

      UServices::generateDigest(U_HASH_MD5, 0, a3, ha3, false);

      result = (ha3 == response);
      }
   else
      {
      // Authorization: Basic cy5jYXNhenphOnN0ZWZhbm8x==

      UString buffer(100U);

      if (UBase64::decode(content, buffer))
         {
         U_INTERNAL_DUMP("buffer = %.*S", U_STRING_TO_TRACE(buffer))

         t.setData(buffer);
         t.setDelimiter(":");

         UString password(100U);

         if (t.next(user,     (bool*)0) &&
             t.next(password, (bool*)0) &&
             isUserAuthorized(user, password))
            {
            result = true;
            }
         }
      }

   U_SRV_LOG("request authorization for user %.*S %s", U_STRING_TO_TRACE(user), result ? "success" : "failed");
#endif

   U_RETURN(result);
}

UString UHTTP::getUserHA1(const UString& user, const UString& realm)
{
   U_TRACE(0, "UHTTP::getUserHA1(%.*S,%.*S)", U_STRING_TO_TRACE(user), U_STRING_TO_TRACE(realm))

   UString ha1;

   if (htdigest)
      {
      // s.casazza:Protected Area:...............\n

      UString line(100U);

      line.snprintf("%.*s:%.*s:", U_STRING_TO_TRACE(user), U_STRING_TO_TRACE(realm));

      uint32_t pos = htdigest->find(line);

      if (pos != U_NOT_FOUND)
         {
         pos += line.size();
         ha1  = htdigest->substr(pos, 32);

         U_INTERNAL_ASSERT_EQUALS(htdigest->c_char(pos + 32),'\n')
         }
      }

   U_RETURN_STRING(ha1);
}

bool UHTTP::isUserAuthorized(const UString& user, const UString& password)
{
   U_TRACE(0, "UHTTP::isUserAuthorized(%.*S,%.*S)", U_STRING_TO_TRACE(user), U_STRING_TO_TRACE(password))

#ifdef HAVE_SSL
   if (htpasswd)
      {
      UString line(100U), output(100U);

      UServices::generateDigest(U_HASH_SHA1, 0, password, output, true);

      line.snprintf("%.*s:{SHA}%.*s\n", U_STRING_TO_TRACE(user), U_STRING_TO_TRACE(output));

      if (htpasswd->find(line) != U_NOT_FOUND) U_RETURN(true);
      }
#endif

   U_RETURN(false);
}

bool UHTTP::checkUriProtected()
{
   U_TRACE(0, "UHTTP::checkUriProtected()")

   if (vallow_IP)
      {
      bool ok = UIPAllow::isAllowed(UClientImage_Base::socket->remoteIPAddress().getInAddr(), *vallow_IP);

      if (ok &&
          UClientImage_Base::setRealIP())
         {
         ok = UIPAllow::isAllowed(UClientImage_Base::getRemoteIP(), *vallow_IP);
         }

      if (ok == false)
         {
         U_SRV_LOG("URI_PROTECTED: request %.*S denied by access list", U_HTTP_URI_TO_TRACE);

         setHTTPForbidden();

         U_RETURN(false);
         }
      }

   // check if it's OK via authentication (digest|basic)

   if (processHTTPAuthorization() == false)
      {
      setHTTPUnAuthorized();

      U_RETURN(false);
      }

   U_RETURN(true);
}

void UHTTP::getFileMimeType(const char* suffix, const char* content_type, UString& ext, uint32_t size)
{
   U_TRACE(0, "UHTTP::getFileMimeType(%S,%S,%.*S,%u)", suffix, content_type, U_STRING_TO_TRACE(ext), size)

   U_INTERNAL_ASSERT_POINTER(file)

   if (suffix == 0) goto end;

   U_INTERNAL_DUMP("suffix = %.*S", 4, suffix)

   U_INTERNAL_ASSERT_EQUALS(suffix[0], '.')

   content_type = UFile::getMimeType(++suffix);

   if (content_type == 0)
      {
#  ifdef HAVE_MAGIC
      content_type = file->getMimeType();
#  endif

      goto end;
      }

   if (UFile::mime_index == U_css)
      {
      U_INTERNAL_ASSERT(U_STRNEQ(content_type, "text/css"))
      U_INTERNAL_ASSERT(u_endsWith(U_FILE_TO_PARAM(*file), U_CONSTANT_TO_PARAM(".css")))

      (void) ext.append("Content-Style-Type: text/css\r\n");

      /*
      if (u_endsWith(U_FILE_TO_PARAM(*file), U_CONSTANT_TO_PARAM(".compressed.css")))
         {
         (void) ext.append(U_CONSTANT_TO_PARAM("Content-Encoding: gzip\r\n"));
         }
      */
      }
   else if (UFile::mime_index == U_js)
      {
      U_INTERNAL_ASSERT(U_STRNEQ(content_type, "text/javascript"))
      U_INTERNAL_ASSERT(u_endsWith(U_FILE_TO_PARAM(*file), U_CONSTANT_TO_PARAM(".js")))

      (void) ext.append("Content-Script-Type: text/javascript\r\n");

      /*
      if (u_endsWith(U_FILE_TO_PARAM(*file), U_CONSTANT_TO_PARAM(".compressed.js")))
         {
         (void) ext.append(U_CONSTANT_TO_PARAM("Content-Encoding: gzip\r\n"));
         }
      */
      }

end:
   if (size ||
       content_type)
      {
      if (content_type == 0) content_type = "application/octet-stream";

      ext.snprintf_add("Content-Type: %s\r\n", content_type);
      }

   U_INTERNAL_ASSERT_MAJOR(ext.capacity(),300)

   ext.snprintf_add("Content-Length: %u\r\n"
                    "Accept-Ranges: bytes\r\n"
                    "Last-Modified: %#D\r\n"
                    "\r\n",
                    size,
                    file->st_mtime);
}

void UHTTP::checkFileForCache()
{
   U_TRACE(0, "UHTTP::checkFileForCache()")

   U_INTERNAL_DUMP("u_buffer(%u) = %.*S", u_buffer_len, u_buffer_len, u_buffer)

   U_INTERNAL_ASSERT_POINTER(pathname)
   U_INTERNAL_ASSERT_EQUALS(u_buffer[0],'.')

   if (u_buffer_len <= 2) return;

   (void) pathname->replace(u_buffer+2, u_buffer_len-2);

   file->setPath(*pathname);

   file_data = (file->stat() ? U_NEW(UFileCacheData) : 0);

   if (file_data == 0) return;

   int ratio;
   bool cached;
   uint32_t size;
   const char* suffix;
   UString content, ext(U_CAPACITY);

   if (file->isPathRelativ(*str_favicon)  ||
       file->isPathRelativ(*str_htpasswd) ||
       file->isPathRelativ(*str_htdigest))
      {
      goto next;
      }

   if (cache_file_mask == 0   ||
       u_ftw_ctx.is_directory ||
       file->dir()            || // NB: can be a simbolic link to a directory...
       u_dosmatch_with_OR(U_FILE_TO_PARAM(*file), U_STRING_TO_PARAM(*cache_file_mask), 0) == false)
      {
      return;
      }

next:
   file_data->array = U_NEW(UVector<UString>(4U));

   content = file->getContent();

   if (content.empty()) return;

   file_data->array->push_back(content);

   U_INTERNAL_ASSERT_MAJOR(file_data->size, 0)

   suffix = file->getSuffix();

   getFileMimeType(suffix, 0, ext, file_data->size);

   U_INTERNAL_DUMP("ext = %.*S", U_STRING_TO_TRACE(ext))

   file_data->array->push_back(ext);

   ratio = 100;

#ifdef HAVE_LIBZ
   content = UStringExt::deflate(content);

   size = content.size();

   ratio = (size * 100U) / file_data->size;

   U_INTERNAL_DUMP("ratio = %d", ratio)

   if (ratio > 85) ratio = 100; // NB: almeno il 15%...
   else
      {
      ext.setBuffer(300U);

      (void) ext.assign(U_CONSTANT_TO_PARAM("Content-Encoding: deflate\r\n"));

      getFileMimeType(suffix, 0, ext, size);

      file_data->array->push_back(content);
      file_data->array->push_back(ext);
      }
#endif

   cached = (ratio          < 50 ||         // enough compression...
             content.size() < (32 * 1024)); // for major size it is better to use sendfile()...

   U_SRV_LOG("file %scached: %S - %u bytes - (%d%%) compressed ratio%s",
                     cached ? "" : "not", pathname->data(), file_data->size, 100 - ratio,
                     cached ? "" : " (size exceeded)");
}

bool UHTTP::isFileInCache()
{
   U_TRACE(0, "UHTTP::isFileInCache()")

   pkey->str     = file->getPathRelativ();
   pkey->_length = file->getPathRelativLen();

   file_data = (*cache_file)[pkey];

   if (file_data)
      {
      file->st_size  = file_data->size;
      file->st_mode  = file_data->mode;
      file->st_mtime = file_data->mtime;

      U_INTERNAL_DUMP("st_size = %I st_mtime = %ld dir() = %b", file->st_size, file->st_mtime, file->dir())

      U_RETURN(true);
      }

   U_RETURN(false);
}

U_NO_EXPORT void UHTTP::processFileCache()
{
   U_TRACE(0, "UHTTP::processFileCache()")

   if (isDataFromCache(false) == false)
      {
      U_http_request_check = '2'; // need processing...

      return;
      }

   if (checkHTTPGetRequestIfModified())
      {
      int nResponseCode = HTTP_OK;
      uint32_t start = 0, size = file_data->size;
      UString ext = getDataFromCache(true, false);

      U_INTERNAL_DUMP("ext = %.*S", U_STRING_TO_TRACE(ext))

      // The Range: header is used with a GET request.
      // For example assume that will return only a portion (let's say the first 32 bytes) of the requested resource...
      //
      // Range: bytes=0-31

      if (http_info.range_len &&
          checkHTTPGetRequestIfRange(UString::getStringNull()))
         {
         if (checkHTTPGetRequestForRange(start, size, ext, getDataFromCache(false, false)) == false) goto end;

         nResponseCode = HTTP_PARTIAL;
         }
      else if (U_http_is_accept_deflate)
         {
         if (isDataFromCache(true))
            {
                                                            ext = getDataFromCache(true,  true);
            if (isHttpHEAD() == false) *UClientImage_Base::body = getDataFromCache(false, true);

            goto next;
            }

         U_http_is_accept_deflate = '2';

         U_INTERNAL_DUMP("U_http_is_accept_deflate = %C", U_http_is_accept_deflate)
         }

      if (isHttpHEAD() == false) *UClientImage_Base::body    = getDataFromCache(false, false).substr(start, size);
next:                            *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(nResponseCode, ext);
      }

end:
   U_http_request_check = '4'; // already processed...
}

U_NO_EXPORT void UHTTP::checkPath()
{
   U_TRACE(0, "UHTTP::checkPath()")

   U_INTERNAL_DUMP("pathname = %.*S", U_STRING_TO_TRACE(*pathname))

   if (u_canonicalize_pathname(pathname->data())) pathname->size_adjust_force(); // NB: can be referenced by file...

   if (UServer_Base::isFileInsideDocumentRoot(*pathname) == false) // like chroot()...
      {
      U_http_request_check = '1'; // NB: special value for forbidden error response...
      }
   else
      {
      // special case: '/' alias '.'...

      if (pathname->size() == u_cwd_len)
         {
         U_INTERNAL_ASSERT_EQUALS(http_info.uri[0], '/')

         file->setRoot();

         U_http_request_check = '2'; // need processing...
         }
      else
         {
         file->setPath(*pathname);

         if (isFileInCache()) U_http_request_check = '3'; // in cache...

#     ifdef HAVE_SYS_INOTIFY_H
         return; // NB: there is inotify, we are sure that the file don't exists...
#     endif

         if (file->stat()) U_http_request_check = '2'; // need processing...
         }
      }
}

// REWRITE RULE

UVector<UHTTP::RewriteRule*>* UHTTP::vRewriteRule;

U_NO_EXPORT void UHTTP::processRewriteRule()
{
   U_TRACE(0, "UHTTP::processRewriteRule()")

#ifdef HAVE_PCRE
   uint32_t pos, len;
   UHTTP::RewriteRule* rule;
   UString uri(U_HTTP_URI_TO_PARAM), new_uri;

   for (uint32_t i = 0, n = vRewriteRule->size(); i < n; ++i)
      {
      rule    = (*vRewriteRule)[i];
      new_uri = rule->key.replace(uri, rule->replacement);

      if (rule->key.matched())
         {
         pos = new_uri.find('?');
         len = (pos == U_NOT_FOUND ? new_uri.size() : pos);

         pathname->setBuffer(u_cwd_len + len);

         pathname->snprintf("%w%.*s", len, new_uri.data());

         U_SRV_LOG("REWRITE_RULE_NF: URI request changed to: %.*s", U_STRING_TO_TRACE(new_uri));

         checkPath();

         if (isHTTPRequestNeedProcessing())
            {
            request_uri->clear();

            (void) alias->replace(new_uri);

            setHTTPUri(alias->data(), len);

            if (pos != U_NOT_FOUND)
               {
               const char* ptr = alias->c_pointer(len+1);

               setHTTPQuery(ptr, alias->remain(ptr));
               }
            }

         break;
         }
      }
#else
   U_SRV_LOG("REWRITE_RULE_NF: pcre support is missing, please install libpcre and the headers and recompile ULib...");
#endif
}

bool UHTTP::checkForCGIRequest()
{
   U_TRACE(0, "UHTTP::checkForCGIRequest()")

   uint32_t uri_len = pathname->size() - u_cwd_len;

   int lsz = uri_len - U_CONSTANT_SIZE(".sh");

   if (lsz < 0) U_RETURN(false);

   uint32_t sz      = uri_len - 1;
   const char* uri  = pathname->c_pointer(u_cwd_len);
   const char*  ptr = uri +  sz;
   const char* lptr = uri + lsz;

   U_INTERNAL_ASSERT_POINTER(ptr)

   if (*ptr != '/') // NB: check if ends with '/'...
      {
      while (*ptr != '/')
         {
         --sz;
         --ptr;
         }

      while (*ptr == '/')
         {
         --sz;
         --ptr;
         }

      if (memcmp(ptr - U_CONSTANT_SIZE("/cgi-bin/") + 2, "/cgi-bin/", U_CONSTANT_SIZE("/cgi-bin/")) == 0)
         {
         lsz = uri_len - 1;

         (void) U_SYSCALL(memcpy, "%p,%S,%u", cgi_dir, uri + 1, lsz);

         cgi_dir[lsz] = '\0';
         cgi_dir[ sz] = '\0';

         U_INTERNAL_DUMP("cgi_dir = %S", cgi_dir)
         U_INTERNAL_DUMP("cgi_doc = %S", cgi_dir + u_strlen(cgi_dir) + 1)
         }
      }

   U_http_sh_script = U_STRNEQ(lptr, ".sh");

        if (U_http_sh_script)         http_info.interpreter = U_PATH_SHELL;
   else if (U_STRNEQ(lptr-1, ".php")) http_info.interpreter = "php-cgi";
   else if (U_STRNEQ(lptr,   ".pl"))  http_info.interpreter = "perl";
   else if (U_STRNEQ(lptr,   ".py"))  http_info.interpreter = "python";
   else if (U_STRNEQ(lptr,   ".rb"))  http_info.interpreter = "ruby";

   U_INTERNAL_DUMP("http_info.interpreter = %S", http_info.interpreter)

   bool result = (http_info.interpreter || cgi_dir[0]);

   U_RETURN(result);
}

void UHTTP::checkHTTPRequest()
{
   U_TRACE(0, "UHTTP::checkHTTPRequest()")

   // ...process the HTTP message

   U_INTERNAL_ASSERT(isHTTPRequest())

   U_INTERNAL_DUMP("U_http_request_check = %c", U_http_request_check)

   U_INTERNAL_ASSERT_EQUALS(U_http_request_check, '\0')

   U_INTERNAL_DUMP("method = %.*S uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_HTTP_URI_TO_TRACE)

   // check if dynamic page (ULib Servlet Page)

   if (isUSPRequest() &&
       U_HTTP_URI_STRNEQ("/usp/"))
      {
      U_http_request_check = '4'; // already processed...

      U_INTERNAL_ASSERT_POINTER(pages)
      U_INTERNAL_ASSERT_EQUALS(pages->empty(),false)

      uint32_t n = U_min(5, http_info.uri_len); // 5 => U_CONSTANT_SIZE("/usp/")

      pkey->str     = http_info.uri     + n;
      pkey->_length = http_info.uri_len - n;

      U_INTERNAL_DUMP("pkey = %.*S", U_STRING_TO_TRACE(*pkey))

      page = (*pages)[pkey];

      if (page == 0)
         {
         U_SRV_LOG("USP request %.*S NOT available...", U_HTTP_URI_TO_TRACE);

         setHTTPServiceUnavailable(); // set Service Unavailable error response...

         return;
         }

      U_INTERNAL_ASSERT_POINTER(page->runDynamicPage)

      // retrieve information on specific HTML form elements
      // (such as checkboxes, radio buttons, and text fields), or uploaded files

      n = processHTTPForm();

      UClientImage_Base::wbuffer->setBuffer(U_CAPACITY);

      page->runDynamicPage(UClientImage_Base::pClientImage);

      if (n) resetForm(true);

      (void) processCGIOutput();

      return;
      }

   pathname->setBuffer(u_cwd_len + http_info.uri_len);

   pathname->snprintf("%w%.*s", U_HTTP_URI_TO_TRACE);

   U_INTERNAL_ASSERT_DIFFERS(pathname->size(), u_cwd_len)

   checkPath();

   if (U_http_request_check == '\0') // NB: not found...
      {
      // URI request can be URL encoded...

      uint32_t len1 = pathname->size() - u_cwd_len,
               len2 = u_url_decode(U_HTTP_URI_TO_PARAM, (unsigned char*)pathname->c_pointer(u_cwd_len), true);

      if (len1 != len2)
         {
         // NB: pathname is referenced by file...

         pathname->size_adjust_force(u_cwd_len + len2);

         file->path_relativ_len = pathname->size() - u_cwd_len;

         U_INTERNAL_DUMP("file = %.*S", U_FILE_TO_TRACE(*file))

         checkPath();
         }
      }

   // NB: apply rewrite rule if specified and file forbidden or do not exist...

   if (U_http_request_check <= '1' && vRewriteRule) processRewriteRule();

   U_INTERNAL_DUMP("U_http_request_check = %C http_info.flag = %.8S", U_http_request_check, http_info.flag)

   // check if it's in cache...

   if (isHTTPRequestInFileCache()) processFileCache();

   // check if cgi request or if we need some interpreter...

   if (isHTTPRequestNeedProcessing()) (void) checkForCGIRequest();
}

// USP (ULib Servlet Page)

U_NO_EXPORT void UHTTP::_callRunDynamicPage(UStringRep* key, void* value)
{
   U_TRACE(0, "UHTTP::_callRunDynamicPage(%.*S,%p)", U_STRING_TO_TRACE(*key), value)

   UHTTP::UServletPage* _page = (UHTTP::UServletPage*)value;

   U_INTERNAL_ASSERT_POINTER(_page)
   U_INTERNAL_ASSERT_POINTER(_page->runDynamicPage)

   // ------------------------------
   // argument value for usp mudule:
   // ------------------------------
   //  0 -> init
   // -1 -> reset
   // -2 -> destroy
   // ------------------------------

   _page->runDynamicPage(argument);
}

void UHTTP::callRunDynamicPage(int arg)
{
   U_TRACE(0, "UHTTP::callRunDynamicPage(%d)", arg)

   U_INTERNAL_ASSERT_POINTER(pages)
   U_INTERNAL_ASSERT_EQUALS(pages->empty(),false)

   // call for all usp modules...

   argument = (void*)arg;

   pages->callForAllEntry(_callRunDynamicPage);
}

// manage CGI

UString UHTTP::getCGIEnvironment()
{
   U_TRACE(0, "UHTTP::getCGIEnvironment()")

   char c = u_line_terminator[0];

   // Accept-Language: en-us,en;q=0.5

   uint32_t    accept_language_len = 0;
   const char* accept_language_ptr = getHTTPHeaderValuePtr(*USocket::str_accept_language);

   if (accept_language_ptr)
      {
      while (accept_language_ptr[accept_language_len] != c) ++accept_language_len;

      U_INTERNAL_DUMP("accept_language = %.*S", accept_language_len, accept_language_ptr)
      }

   // Referer: http://www.cgi101.com/class/ch3/text.html

   uint32_t    referer_len = 0;
   const char* referer_ptr = getHTTPHeaderValuePtr(*USocket::str_referer);

   if (referer_ptr)
      {
      while (referer_ptr[referer_len] != c) ++referer_len;

      U_INTERNAL_DUMP("referer = %.*S", referer_len, referer_ptr)
      }

   // User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)

   uint32_t    user_agent_len = 0;
   const char* user_agent_ptr = getHTTPHeaderValuePtr(*USocket::str_user_agent);

   if (user_agent_ptr)
      {
      while (user_agent_ptr[user_agent_len] != c) ++user_agent_len;

      U_INTERNAL_DUMP("user_agent = %.*S", user_agent_len, user_agent_ptr)
      }

   /*
   Set up CGI environment variables.
   The following table describes common CGI environment variables that the server creates (some of these are not available with some servers):

   CGI server variable  Description
   -------------------------------------------------------------------------------------------------------------------------------------------
   SERVER_SOFTWARE
      Name and version of the information server software answering the request (and running the gateway). Format: name/version.
   SERVER_NAME
      Server's hostname, DNS alias, or IP address as it appears in self-referencing URLs.
   GATEWAY_INTERFACE
      CGI specification revision with which this server complies. Format: CGI/revision.
   SERVER_PROTOCOL
      Name and revision of the information protocol this request came in with. Format: protocol/revision.
   SERVER_PORT
      Port number to which the request was sent.
   REQUEST_METHOD
      Method with which the request was made. For HTTP, this is Get, Head, Post, and so on.
   PATH_INFO
      Extra path information, as given by the client. Scripts can be accessed by their virtual pathname, followed by extra information at the end
      of this path. The extra information is sent as PATH_INFO.
   PATH_TRANSLATED
      Translated version of PATH_INFO after any virtual-to-physical mapping.
   SCRIPT_NAME
      Virtual path to the script that is executing; used for self-referencing URLs.
   QUERY_STRING
      Query information that follows the ? in the URL that referenced this script.
   REMOTE_HOST
      Hostname making the request. If the server does not have this information, it sets REMOTE_ADDR and does not set REMOTE_HOST.
   REMOTE_ADDR
      IP address of the remote host making the request.
   AUTH_TYPE
      If the server supports user authentication, and the script is protected, the protocol-specific authentication method used to validate the user.
   AUTH_USER
   REMOTE_USER
      If the server supports user authentication, and the script is protected, the username the user has authenticated as. (Also available as AUTH_USER.)
   REMOTE_IDENT
      If the HTTP server supports RFC 931 identification, this variable is set to the remote username retrieved from the server.
      Use this variable for logging only.
   CONTENT_TYPE
      For queries that have attached information, such as HTTP POST and PUT, this is the content type of the data.
   CONTENT_LENGTH
      Length of the content as given by the client.

   CERT_ISSUER
      Issuer field of the client certificate (O=MS, OU=IAS, CN=user name, C=USA).
   CERT_SUBJECT
      Subject field of the client certificate.
   CERT_SERIALNUMBER
      Serial number field of the client certificate.
   -------------------------------------------------------------------------------------------------------------------------------------------

   The following table describes common CGI environment variables the browser creates and passes in the request header:

   CGI client variable  Description
   -------------------------------------------------------------------------------------------------------------------------------------------
   HTTP_REFERER
      The referring document that linked to or submitted form data.
   HTTP_USER_AGENT
      The browser that the client is currently using to send the request. Format: software/version library/version.
   HTTP_IF_MODIFIED_SINCE
      The last time the page was modified. The browser determines whether to set this variable, usually in response to the server having sent
      the LAST_MODIFIED HTTP header. It can be used to take advantage of browser-side caching.
   -------------------------------------------------------------------------------------------------------------------------------------------

   Example:
   ----------------------------------------------------------------------------------------------------------------------------
   GATEWAY_INTERFACE=CGI/1.1
   QUERY_STRING=
   REMOTE_ADDR=127.0.0.1
   REQUEST_METHOD=GET
   SCRIPT_NAME=/cgi-bin/printenv
   SERVER_NAME=localhost
   SERVER_PORT=80
   SERVER_PROTOCOL=HTTP/1.1
   SERVER_SOFTWARE=Apache

   HTTP_ACCEPT_LANGUAGE="en-us,en;q=0.5"
   PATH="/lib64/rc/sbin:/lib64/rc/bin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin"
   SCRIPT_FILENAME="/var/www/localhost/cgi-bin/printenv"
   HTTP_COOKIE="_saml_idp=dXJuOm1hY2U6dGVzdC5zXRo; _redirect_user_idp=urn%3Amace%3Atest.shib%3Afederation%3Alocalhost;"

   DOCUMENT_ROOT="/var/www/localhost/htdocs"
   HTTP_ACCEPT="text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png"
   HTTP_ACCEPT_CHARSET="ISO-8859-1,utf-8;q=0.7,*;q=0.7"
   HTTP_ACCEPT_ENCODING="gzip,deflate"
   HTTP_ACCEPT_LANGUAGE="en-us,en;q=0.5"
   HTTP_CONNECTION="keep-alive"
   HTTP_HOST="localhost"
   HTTP_KEEP_ALIVE="300"
   HTTP_USER_AGENT="Mozilla/5.0 (X11; U; Linux x86_64; en-US; rv:1.8.1.14) Gecko/20080421 Firefox/2.0.0.14"
   REQUEST_URI="/cgi-bin/printenv"
   REMOTE_PORT="35653"
   SERVER_ADDR="127.0.0.1"
   SERVER_ADMIN="root@localhost"
   SERVER_SIGNATURE="<address>Apache Server at localhost Port 80</address>\n"
   UNIQUE_ID="b032zQoeAYMAAA-@BZwAAAAA"
   ----------------------------------------------------------------------------------------------------------------------------

   http://$SERVER_NAME:$SERVER_PORT$SCRIPT_NAME$PATH_INFO will always be an accessible URL that points to the current script...
   */

   uint32_t len = UClientImage_Base::body->size();
   UString name = USocketExt::getNodeName(), uri = getRequestURI(false),
           ip_server = UServer_Base::getIPAddress(), ip_client = UClientImage_Base::getRemoteIP(),
           buffer(4000U + http_info.query_len + referer_len + user_agent_len);

   buffer.snprintf("CONTENT_LENGTH=%u\n" // The first header must have the name "CONTENT_LENGTH" and a value that is body length in decimal.
                                         // The "CONTENT_LENGTH" header must always be present, even if its value is "0".
                   "REMOTE_ADDR=%.*s\n"  // The IP address of the visitor
                // "REMOTE_HOST=%.*s\n"  // The hostname of the visitor (if your server has reverse-name-lookups on; otherwise this is the IP address again)
                // "REMOTE_PORT=%.*s\n"  // The port the visitor is connected to on the web server
                // "REMOTE_USER=%.*s\n"  // The visitor's username (for .htaccess-protected pages)
                // "SERVER_ADMIN=%.*s\n" // The email address for your server's webmaster
                   "SERVER_NAME=%.*s\n"  // Your server's fully qualified domain name (e.g. www.cgi101.com)
                   "SERVER_ADDR=%.*s\n"
                   "SERVER_PORT=%d\n"    // The port number your server is listening on
                   "SERVER_PROTOCOL=HTTP/1.%c\n"
                   "SCRIPT_NAME=%.*s\n" // The interpreted pathname of the current CGI (relative to the document root)
                   // ext
                   "PWD=%w/%s\n"
                   "DOCUMENT_ROOT=%w\n"         // The root directory of your server
                   "SCRIPT_FILENAME=%w%.*s\n"   // The full pathname of the current CGI (is used by PHP for determining the name of script to execute)
                   // PHP
                   "REQUEST_METHOD=%.*s\n",     // dealing with POST requests
                   len,
                   U_STRING_TO_TRACE(ip_client),
                   U_STRING_TO_TRACE(name),
                   U_STRING_TO_TRACE(ip_server),
                   UServer_Base::port,
                   (U_http_version ? U_http_version : '0'),
                   U_HTTP_URI_TO_TRACE,
                   // ext
                   cgi_dir,
                   U_HTTP_URI_TO_TRACE,
                   // PHP
                   U_HTTP_METHOD_TO_TRACE);

   // The hostname of your server from header's request.
   // The difference between HTTP_HOST and SERVER_NAME is that
   // HTTP_HOST can include the «:PORT» text, and SERVER_NAME only the name

   if (http_info.host_len)         buffer.snprintf_add("HTTP_HOST=%.*s\n", U_HTTP_HOST_TO_TRACE);
   else                            buffer.snprintf_add("HTTP_HOST=%.*s\n", U_STRING_TO_TRACE(ip_server));

   if (referer_len)                buffer.snprintf_add("HTTP_REFERER=%.*s\n", referer_len, referer_ptr); // The URL of the page that called your script
   if (user_agent_len)             buffer.snprintf_add("\"HTTP_USER_AGENT=%.*s\"\n", user_agent_len, user_agent_ptr); // The browser type of the visitor
   if (http_info.query_len)        buffer.snprintf_add("QUERY_STRING=%.*s\n", U_HTTP_QUERY_TO_TRACE); // contains the parameters of the request
   if (http_info.content_type_len) buffer.snprintf_add("\"CONTENT_TYPE=%.*s\"\n", U_HTTP_CTYPE_TO_TRACE);

   // The interpreted pathname of the requested document or CGI (relative to the document root)

   buffer.snprintf_add("REQUEST_URI=%.*s\n", U_STRING_TO_TRACE(uri));

   if (U_http_sh_script)
      {
      // ULIB facility: some env var for shell script...

      if (user_agent_len)
         {
         // check for MSIE in "User-Agent: ...." in header request...

         if (u_find(user_agent_ptr, user_agent_len, U_CONSTANT_TO_PARAM("MSIE"))) (void) buffer.append(U_CONSTANT_TO_PARAM("BROWSER_MSIE=1\n"));
         }

      buffer.snprintf_add("HTTP_ACCEPT_LANGUAGE=%.2s\n", (accept_language_len ? accept_language_ptr : "en"));
      }
   else
      {
      if (accept_language_len) buffer.snprintf_add("\"HTTP_ACCEPT_LANGUAGE=%.*s\"\n", accept_language_len, accept_language_ptr);
      }

#ifdef HAVE_SSL
   if (UClientImage_Base::socket->isSSL())
      {
      X509* x509 = ((USSLSocket*)UClientImage_Base::socket)->getPeerCertificate();

      if (x509)
         {
         UString issuer  = UCertificate::getIssuer(x509),
                 subject = UCertificate::getSubject(x509);

         buffer.snprintf_add("\"SSL_CLIENT_I_DN=%.*s\"\n"
                             "\"SSL_CLIENT_S_DN=%.*s\"\n"
                             "SSL_CLIENT_CERT_SERIAL=%ld\n",
                             U_STRING_TO_TRACE(issuer),
                             U_STRING_TO_TRACE(subject),
                             UCertificate::getSerialNumber(x509));
         }

      (void) buffer.append(U_CONSTANT_TO_PARAM("HTTPS=on\n")); // "on" if the script is being called through a secure server
      }
#endif

   (void) buffer.append(U_CONSTANT_TO_PARAM("GATEWAY_INTERFACE=CGI/1.1\n"
                                            "SERVER_SOFTWARE=" PACKAGE "/" VERSION "\n" // The server software you're using (such as Apache 1.3)
                                            "PATH=/usr/local/bin:/usr/bin:/bin\n" // The system path your server is running under
                                            "REDIRECT_STATUS=200\n"));

   // The visitor's cookie, if one is set
   // -------------------------------------------------------------------------------------------------------------
   // Cookie: _saml_idp=dXJuOm1hY2U6dGVzdC5zXRo; _redirect_user_idp=urn%3Amace%3Atest.shib%3Afederation%3Alocalhost;

   UString cookie = getHTTPCookie();

   U_INTERNAL_DUMP("cookie = %.*S", U_STRING_TO_TRACE(cookie))

   if (cookie.empty() == false)
      {
      (void) buffer.append(U_CONSTANT_TO_PARAM("\"HTTP_COOKIE="));
      (void) buffer.append(cookie);
      (void) buffer.append(U_CONSTANT_TO_PARAM("\"\n"));
      }

   U_ASSERT_EQUALS(buffer.isBinary(), false)

   U_RETURN_STRING(buffer);
}

U_NO_EXPORT bool UHTTP::setCGIShellScript(UString& command)
{
   U_TRACE(0, "UHTTP::setCGIShellScript(%.*S)", U_STRING_TO_TRACE(command))

   // ULIB facility: check if present form data and convert it in parameters for shell script...

   U_INTERNAL_ASSERT(U_http_sh_script)

   char c;
   UString item;
   const char* ptr;
   uint32_t n = processHTTPForm(), sz;

   U_INTERNAL_ASSERT_POINTER(form_name_value)

   for (uint32_t i = 1; i < n; i += 2)
      {
      item = (*form_name_value)[i];

      // check for binary data (invalid)...

      if (item.empty() ||
          item.isBinary())
         {
         c    = '\'';
         ptr  = 0;
         sz   = 0;

         if (item.empty() == false) U_WARNING("Found binary data in form: %.*S", U_STRING_TO_TRACE(item));
         }
      else
         {
         ptr = item.data();
         sz  = item.size();

         // find how to escape the param...

         c = (memchr(ptr, '"', sz) ? '\'' : '"');
         }

      (void) command.reserve(command.size() + sz + 4);

      command.snprintf_add(" %c%.*s%c ", c, sz, ptr, c);
      }

   U_RETURN(n > 0);
}

U_NO_EXPORT bool UHTTP::splitCGIOutput(const char*& ptr1, const char* ptr2, uint32_t endHeader, UString& ext)
{
   U_TRACE(0, "UHTTP::splitCGIOutput(%p,%p,%u,%p)", ptr1, ptr2, endHeader, &ext)

   uint32_t pos = UClientImage_Base::wbuffer->distance(ptr1);

   if (pos) ext = UClientImage_Base::wbuffer->substr(0U, pos);

   ptr1 = (const char*) memchr(ptr2, '\r', endHeader - pos);

   if (ptr1)
      {
      pos = UClientImage_Base::wbuffer->distance(ptr1) + 2; // NB: we cut \r\n...

      U_INTERNAL_ASSERT_MINOR(pos, endHeader)

      uint32_t diff = endHeader - pos;

      U_INTERNAL_DUMP("diff = %u pos = %u endHeader = %u", diff, pos, endHeader)

      if (diff > 4) ext += UClientImage_Base::wbuffer->substr(pos, diff - 4); // NB: we cut \r\n\r\n...

      U_INTERNAL_DUMP("value = %.*S ext = %.*S", (uint32_t)(ptr2int(ptr1) - ptr2int(ptr2)), ptr2, U_STRING_TO_TRACE(ext))

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool UHTTP::processCGIOutput()
{
   U_TRACE(0, "UHTTP::processCGIOutput()")

   /*
   CGI Script Output

   The script sends its output to stdout. This output can either be a document generated by the script, or instructions to the server
   for retrieving the desired output.

   Script naming conventions

   Normally, scripts produce output which is interpreted and sent back to the client. An advantage of this is that the scripts do not
   need to send a full HTTP/1.0 header for every request.

   Some scripts may want to avoid the extra overhead of the server parsing their output, and talk directly to the client. In order to
   distinguish these scripts from the other scripts, CGI requires that the script name begins with nph- if a script does not want the
   server to parse its header. In this case, it is the script's responsibility to return a valid HTTP response to the client.

   Parsed headers

   The output of scripts begins with a small header. This header consists of text lines, in the same format as an HTTP header,
   terminated by a blank line (a line with only a linefeed or CR/LF).

   Any headers which are not server directives are sent directly back to the client. Currently, this specification defines three server
   directives:

   * Content-type: This is the MIME type of the document you are returning.

   * Location:     This is used to specify to the server that you are returning a reference to a document rather than an actual document.
                   If the argument to this is a URL, the server will issue a redirect to the client.

   * Status:       This is used to give the server an HTTP status line to send to the client. The format is nnn xxxxx, where nnn is
                   the 3-digit status code, and xxxxx is the reason string, such as "Forbidden".
   */

   const char* ptr;
   int nResponseCode;
   const char* endptr;
   const char* location;
   uint32_t endHeader, sz = UClientImage_Base::wbuffer->size();
   bool connection_close, header_content_length, header_content_type, content_encoding;

   U_INTERNAL_DUMP("sz = %u", sz)

   if (sz == 0) goto error;

   ptr = UClientImage_Base::wbuffer->data();

   U_INTERNAL_DUMP("ptr = %.*S", U_min(20, sz), ptr)

   // NB: we check for <h(1|tml)> (HTML without HTTP headers..)

   if (          ptr[0]  == '<' &&
       u_toupper(ptr[1]) == 'H')
      {
      goto no_headers;
      }

rescan:
   endHeader = u_findEndHeader(ptr, sz);

   // NB: endHeader comprende anche la blank line...

   U_INTERNAL_DUMP("endHeader = %u u_line_terminator_len = %d", endHeader, u_line_terminator_len)

   if (endHeader == U_NOT_FOUND)
      {
no_headers: // NB: we assume to have HTML without HTTP headers...

#  ifdef HAVE_MAGIC
      U_ASSERT(U_STRNEQ(UMagic::getType(ptr, sz).data(), "text"))
#  endif

      http_info.clength = sz;

      setHTTPCgiResponse(HTTP_OK, false, false, false);

      U_RETURN(true);
      }

   if (u_line_terminator_len == 1)
      {
      UString tmp                 = UStringExt::dos2unix(UClientImage_Base::wbuffer->substr(0U, endHeader), true) +
                                                         UClientImage_Base::wbuffer->substr(    endHeader);
      *UClientImage_Base::wbuffer = tmp;

      sz  = UClientImage_Base::wbuffer->size();
      ptr = UClientImage_Base::wbuffer->data();

      goto rescan;
      }

   U_INTERNAL_ASSERT_EQUALS(u_line_terminator_len, 2)

   endptr            = UClientImage_Base::wbuffer->c_pointer(endHeader);
   nResponseCode     = HTTP_OK;
   connection_close  = header_content_length = header_content_type = content_encoding = false;
   http_info.clength = sz - endHeader;

   U_INTERNAL_DUMP("http_info.clength = %u", http_info.clength)

   while (ptr < endptr)
      {
      U_INTERNAL_DUMP("ptr = %.20S", ptr)

      switch (u_toupper(*ptr))
         {
         case 'H': // response line: HTTP/1.n nnn <ssss>
            {
            if (scanfHTTPHeader(ptr)) // check for script's responsibility to return a valid HTTP response to the client...
               {
               U_INTERNAL_DUMP("wbuffer(%u) = %.*S", UClientImage_Base::wbuffer->size(), U_STRING_TO_TRACE(*UClientImage_Base::wbuffer))

               U_INTERNAL_DUMP("U_http_is_connection_close = %d", U_http_is_connection_close)

               if (U_http_is_connection_close == U_MAYBE)
                  {
                  U_http_is_connection_close = (u_find(ptr + 15, endHeader, U_CONSTANT_TO_PARAM("Connection: close")) ? U_YES : U_NOT);

                  U_INTERNAL_DUMP("U_http_is_connection_close = %d", U_http_is_connection_close)
                  }

               U_RETURN(true);
               }

            goto next;
            }
         break;

         case 'L': // check if is used to specify to the server that you are returning a reference to a document...
            {
            U_INTERNAL_DUMP("check 'Location: ...'")

            if (U_STRNEQ(ptr+1, "ocation: "))
               {
               UString ext;

               location = ptr + U_CONSTANT_SIZE("Location: ");

               if (splitCGIOutput(ptr, location, endHeader, ext))
                  {
                  setHTTPRedirectResponse(ext, location, ptr - location);

                  U_RETURN(true);
                  }

               goto error;
               }

            goto next;
            }
         break;

         case 'X':
            {
            U_INTERNAL_DUMP("check 'X-Sendfile: ...' or 'X-Accel-Redirect: ...'")

            /* X-Sendfile is a special, non-standard HTTP header. At first you might think it is no big deal, but think again.
             * It can be enabled in any CGI, FastCGI or SCGI backend. Basically its job is to instruct the web server to ignore
             * the content of the response and replace it by whatever is specified in the header. The main advantage of this is
             * that it will be server the one serving the file, making use of all its optimizations. It is useful for processing
             * script-output of e.g. php, perl, ruby or any cgi. This is particularly useful because it hands the load to server,
             * all the response headers from the backend are forwarded, the whole application uses a lot less resources and performs
             * several times faster not having to worry about a task best suited for a web server. You retain the ability to check for
             * special privileges or dynamically deciding anything contemplated by your backend logic, you speed up things a lot while
             * having more resources freed, and you can even specify the delivery of files outside of the web server's document root path.
             * Of course, this is to be done solely in controlled environments. In short, it offers a huge performance gain at absolutely
             * no cost. Note that the X-Sendfile feature also supports X-Accel-Redirect header, a similar feature offered by other web
             * servers. This is to allow the migration of applications supporting it without having to make major code rewrites.
             */

            location = ptr+1;

                 if (U_STRNEQ(location, "-Sendfile: "))       location += U_CONSTANT_SIZE("X-Sendfile:");
            else if (U_STRNEQ(location, "-Accel-Redirect: ")) location += U_CONSTANT_SIZE("X-Accel-Redirect:");

            if (location > (ptr+1))
               {
               UString ext;

               if (splitCGIOutput(ptr, location, endHeader, ext))
                  {
                  uint32_t len = ptr - location;

                  pathname->setBuffer(u_cwd_len + 1 + len);

                  if (location[0] == '/') pathname->snprintf(   "%.*s", len, location);
                  else                    pathname->snprintf("%w/%.*s", len, location);

                  if (u_canonicalize_pathname(pathname->data())) pathname->size_adjust_force(); // NB: can be referenced by file...

                  file->setPath(*pathname);

                  if (file->stat())
                     {
                     UString rbuffer_save;
                     bool bext = (ext.empty() == false);

                     U_SRV_LOG("header X-Sendfile found in CGI output: serving file %.*S", U_STRING_TO_TRACE(*pathname));

                     if (bext)
                        {
                        UString name = USocketExt::getNodeName(), req(300U + pathname->size() + ext.size());

                        req.snprintf("GET %.*s HTTP/1.1\r\n" \
                                     "Host: %.*s\r\n" \
                                     "%.*s" \
                                     "\r\n\r\n",
                                     U_STRING_TO_TRACE(*pathname),
                                     U_STRING_TO_TRACE(name),
                                     U_STRING_TO_TRACE(ext));

                        UClientImage_Base::body->clear();
                        UClientImage_Base::wbuffer->clear();

                        // manage buffered read (pipelining)

                        len = USocketExt::size_message;

                        rbuffer_save                = *UClientImage_Base::rbuffer;
                        *UClientImage_Base::rbuffer = req;
                        
                        (void) readHTTPRequest();
                        }

                     processHTTPGetRequest();

                     if (bext)
                        {
                        USocketExt::size_message    = len;
                        *UClientImage_Base::rbuffer = rbuffer_save;
                        }

                     U_RETURN(true);
                     }
                  }

               goto error;
               }

            goto next;
            }
         break;

         case 'S':
            {
            // check if is used to give the server an HTTP status line to send to the client...
            // ---------------------------------------------------------------------------------------------------------------------------------------------
            // Ex: "Status: 503 Service Unavailable\r\nX-Powered-By: PHP/5.2.6-pl7-gentoo\r\nContent-Type: text/html;charset=utf-8\r\n\r\n<!DOCTYPE html"...

            U_INTERNAL_DUMP("check 'Status: ...'")

            if (U_STRNEQ(ptr+1, "tatus: "))
               {
               location = ptr + U_CONSTANT_SIZE("Status: ");

               nResponseCode = strtol(location, 0, 0);

               U_INTERNAL_DUMP("nResponseCode = %d", nResponseCode)

               location = (const char*) memchr(location, '\n', sz);

               if (location)
                  {
                  uint32_t diff = (location - ptr) + 1; // NB: we cut also \n...

                  U_INTERNAL_ASSERT_MINOR(diff,512)
                  U_INTERNAL_ASSERT_MINOR(diff, endHeader)

                  sz        -= diff;
                  endHeader -= diff;

                  U_INTERNAL_DUMP("diff = %u sz = %u endHeader = %u", diff, sz, endHeader)

                  UClientImage_Base::wbuffer->erase(0, diff);

                  U_INTERNAL_DUMP("wbuffer(%u) = %.*S", sz, U_STRING_TO_TRACE(*UClientImage_Base::wbuffer))

                  U_ASSERT_EQUALS(sz, UClientImage_Base::wbuffer->size())

                  ptr    = UClientImage_Base::wbuffer->data();
                  endptr = UClientImage_Base::wbuffer->c_pointer(endHeader);

                  continue;
                  }

               goto error;
               }

            // ULIB facility: check for request TODO timed session cookies...

            U_INTERNAL_DUMP("check 'Set-Cookie: TODO['")

            if (U_STRNEQ(ptr+1, "et-Cookie: TODO["))
               {
               uint32_t pos1,
                        pos2 = U_CONSTANT_SIZE("Set-Cookie: "),
                        n1   = U_CONSTANT_SIZE("TODO[");

               ptr += pos2;
               pos1 = UClientImage_Base::wbuffer->distance(ptr);

               ptr     += n1;
               location = ptr;

               ptr = (const char*) memchr(location, ']', sz);

               if (ptr)
                  {
                  // REQ: Set-Cookie: TODO[ data expire path domain secure HttpOnly ]
                  // ----------------------------------------------------------------------------------------------------------------------------
                  // string -- Data to put into the cookie        -- must
                  // int    -- Lifetime of the cookie in HOURS    -- must (0 -> valid until browser exit)
                  // string -- Path where the cookie can be used  --  opt
                  // string -- Domain which can read the cookie   --  opt
                  // bool   -- Secure mode                        --  opt
                  // bool   -- Only allow HTTP usage              --  opt
                  // ----------------------------------------------------------------------------------------------------------------------------
                  // RET: Set-Cookie: ulib_sid=data&expire&HMAC-MD5(data&expire); expires=expire(GMT); path=path; domain=domain; secure; HttpOnly

                  uint32_t len  = ptr - location;
                  UString param = UClientImage_Base::wbuffer->substr(location, len), set_cookie = setHTTPCookie(param);

                  U_INTERNAL_DUMP("set_cookie = %.*S", U_STRING_TO_TRACE(set_cookie))

                  n1 += len + 1; // ']'

                  uint32_t n2   = set_cookie.size() - pos2,
                           diff = n2 - n1;

                  sz        += diff;
                  endHeader += diff;

                  U_INTERNAL_DUMP("diff = %u sz = %u endHeader = %u", diff, sz, endHeader)

                  U_INTERNAL_ASSERT_MINOR(diff,512)

                  (void) UClientImage_Base::wbuffer->replace(pos1, n1, set_cookie, pos2, n2);

                  U_INTERNAL_DUMP("wbuffer(%u) = %#.*S", UClientImage_Base::wbuffer->size(), U_STRING_TO_TRACE(*UClientImage_Base::wbuffer))

                  U_ASSERT_EQUALS(sz, UClientImage_Base::wbuffer->size())

                  // for next parsing...

                  ptr    = UClientImage_Base::wbuffer->c_pointer(pos1 + n2 + 2);
                  endptr = UClientImage_Base::wbuffer->c_pointer(endHeader);

                  continue;
                  }

               goto error;
               }

            goto next;
            }
         break;

         case 'C':
            {
            // check if is used to specify to the server to close the connection...

            U_INTERNAL_DUMP("check 'Connection: close'")

            if (U_STRNEQ(ptr+1, "onnection: close"))
               {
               connection_close = true;

               ptr += U_CONSTANT_SIZE("Connection: close") + 2;

               continue;
               }

            U_INTERNAL_DUMP("check 'Content-...: ...'")

            if (U_STRNEQ(ptr+1, "ontent-"))
               {
               ptr += U_CONSTANT_SIZE("Content-");

               char c = u_toupper(*ptr++);

               if (c == 'T' &&
                   U_STRNEQ(ptr, "ype: "))
                  {
                  ptr += U_CONSTANT_SIZE("ype: ");

                  header_content_type = true;

                  U_INTERNAL_DUMP("header_content_type = %b", header_content_type)

                  if (U_STRNEQ(ptr, "text/") == false) content_encoding = true;

                  U_INTERNAL_DUMP("content_encoding = %b", content_encoding)
                  }
               else if (c == 'L' &&
                        U_STRNEQ(ptr, "ength: "))
                  {
                  ptr += U_CONSTANT_SIZE("ength: ");

                  header_content_length = true;

                  U_INTERNAL_DUMP("header_content_length = %b", header_content_length)

                  uint32_t pos = UClientImage_Base::wbuffer->distance(ptr);

                  if (checkHTTPContentLength(ptr, http_info.clength, *UClientImage_Base::wbuffer)) // NB: with drupal can happens...!!!
                     {
                     sz     = UClientImage_Base::wbuffer->size();
                     ptr    = UClientImage_Base::wbuffer->c_pointer(pos);
                     endptr = UClientImage_Base::wbuffer->c_pointer(endHeader);
                     }
                  }
               else if (c == 'E' &&
                        U_STRNEQ(ptr, "ncoding: "))
                  {
                  content_encoding = true;

                  ptr += U_CONSTANT_SIZE("ncoding: ");
                  }
               }

            goto next;
            }
         break;

         default:
            {
next:       // for next parsing...

            ptr = (const char*) memchr(ptr, '\n', sz);

            if (ptr)
               {
               ++ptr;

               continue;
               }

            goto error;
            }
         break;
         }
      }

   U_INTERNAL_ASSERT_MAJOR(endHeader, 0)

   setHTTPCgiResponse(nResponseCode, header_content_length, header_content_type, content_encoding);

   if (connection_close) U_http_is_connection_close = U_YES;

   U_RETURN(true);

error:
   U_SRV_LOG("UHTTP::processCGIOutput() failed...");

   setHTTPInternalError(); // set internal error response...

   U_RETURN(false);
}

bool UHTTP::processCGIRequest(UCommand* cmd, UString* penv, bool async)
{
   U_TRACE(0, "UHTTP::processCGIRequest(%p,%p,%b)", cmd, penv, async)

   // process the CGI or script request

   U_INTERNAL_DUMP("method = %.*S method_type = %C uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_http_method_type, U_HTTP_URI_TO_TRACE)

   U_ASSERT(isCGIRequest())

   bool reset_form = false;

   if (cmd) U_http_sh_script = cmd->isShellScript();
   else
      {
      // NB: we can't use relativ path because after we call chdir()...

      UString command(U_CAPACITY), path(U_CAPACITY);

      path.snprintf("%w/%s/%s", cgi_dir, cgi_dir + u_strlen(cgi_dir) + 1);

      if (http_info.interpreter)        command.snprintf("%s %.*s", http_info.interpreter, U_STRING_TO_TRACE(path));
      else                       (void) command.assign(path);                       

      // ULIB facility: check if present form data and convert it in parameters for shell script...

      if (U_http_sh_script) reset_form = setCGIShellScript(command);

      (cmd = pcmd)->setCommand(command);
      }

   if (cmd->checkForExecute() == false)
      {
      // set forbidden error response...

      setHTTPForbidden();

      U_RETURN(false);
      }

#ifdef DEBUG
   static int fd_stderr = UFile::creat("/tmp/processCGIRequest.err", O_WRONLY | O_APPEND, PERM_FILE);
#else
   static int fd_stderr = UServices::getDevNull();
#endif

   if (async                                 &&
       UServer_Base::preforked_num_kids == 0 &&
       UClientImage_Base::checkForPipeline() == false)
      {
      U_INTERNAL_ASSERT_POINTER(UServer_Base::proc)

      // NB: if server no preforked (ex: nodog) process the HTTP CGI request with fork....

      if (UServer_Base::proc->parent()) UServer_Base::proc->wait(); // NB: to avoid fork bomb...

      if (UServer_Base::proc->fork() &&
          UServer_Base::proc->parent())
         {
         if (reset_form) resetForm(false);

         if (cgi_dir[0]) cgi_dir[0] = '\0';

         UClientImage_Base::write_off = true;

         U_RETURN(false);
         }

      if (UServer_Base::proc->child())
         {
         UServer_Base::flag_loop = false;

         if (UServer_Base::isLog())
            {
            u_unatexit(&ULog::close); // NB: needed because all instance try to close the log... (inherits from its parent)

            if (ULog::isMemoryMapped() &&
                ULog::isShared() == false)
               {
               // NB: we need locking to write log...

               UServer_Base::log = 0;
               }
            }
         }
      }

   UString environment;

   if (penv)                environment = *penv;
   if (environment.empty()) environment = getCGIEnvironment();

   cmd->setEnvironment(&environment);

   /* When a url ends by "/cgi-bin/" it is assumed to be a cgi script.
    * The server changes directory to the location of the script and
    * executes it after setting QUERY_STRING and other environment variables.
    */

   if (cgi_dir[0]) (void) UFile::chdir(cgi_dir, true);

   // execute script...

   UString* pinput = (UClientImage_Base::body->empty() ? 0 : UClientImage_Base::body);

   (void) cmd->execute(pinput, UClientImage_Base::wbuffer, -1, fd_stderr);

   if (cgi_dir[0])
      {
      (void) UFile::chdir(0, true);

      cgi_dir[0] = '\0';
      }

   UServer_Base::logCommandMsgError(cmd->getCommand());

   if (reset_form) resetForm(true);

   cmd->reset(penv);

   U_RETURN(true);
}

bool UHTTP::checkHTTPContentLength(const char* ptr, uint32_t length, UString& ext)
{
   U_TRACE(0, "UHTTP::checkHTTPContentLength(%p,%u,%.*S)", ptr, length, U_STRING_TO_TRACE(ext))

   char* nptr;
   uint32_t clength = (uint32_t) strtoul(ptr, &nptr, 0);

   U_INTERNAL_DUMP("clength = %u", clength)

   if (clength != length)
      {
      char bp[12];
      uint32_t pos = ext.distance(ptr);

      (void) ext.replace(pos, (nptr - ptr), bp, u_snprintf(bp, sizeof(bp), "%u", length));

      U_INTERNAL_DUMP("ext(%u) = %#.*S", ext.size(), U_STRING_TO_TRACE(ext))

      U_RETURN(true);
      }

   U_RETURN(false);
}

typedef struct { uint32_t start, end; } HTTPRange;

/**
 * The Range: header is used with a GET request.
 *
 * For example assume that will return only a portion (let's say the first 32 bytes) of the requested resource...
 * Range: bytes=0-31
 *
 * If @end is non-negative, then @start and @end represent the bounds
 * of the range, counting from %0. (Eg, the first 500 bytes would be
 * represented as @start = %0 and @end = %499.)
 *
 * If @end is %-1 and @start is non-negative, then this represents a
 * range starting at @start and ending with the last byte of the
 * requested resource body. (Eg, all but the first 500 bytes would be
 * @start = %500, and @end = %-1.)
 *
 * If @end is %-1 and @start is negative, then it represents a "suffix
 * range", referring to the last -@start bytes of the resource body.
 * (Eg, the last 500 bytes would be @start = %-500 and @end = %-1.)
 *
 * The If-Range: header allows a client to "short-circuit" the request (conditional GET).
 * Informally, its meaning is `if the entity is unchanged, send me the part(s) that I am
 * missing; otherwise, send me the entire new entity'.
 *
 * If-Range: ( entity-tag | HTTP-date )
 *
 * If the client has no entity tag for an entity, but does have a Last-Modified date, it
 * MAY use that date in an If-Range header. (The server can distinguish between a valid
 * HTTP-date and any form of entity-tag by examining no more than two characters.) The If-Range
 * header SHOULD only be used together with a Range header, and MUST be ignored if the request
 * does not include a Range header, or if the server does not support the sub-range operation. 
 *
 */

U_NO_EXPORT bool UHTTP::checkHTTPGetRequestIfRange(const UString& etag)
{
   U_TRACE(0, "UHTTP::checkHTTPGetRequestIfRange(%.*S)", U_STRING_TO_TRACE(etag))

   const char* ptr = getHTTPHeaderValuePtr(*USocket::str_if_range);

   if (ptr)
      {
      if (*ptr == '"') // entity-tag
         {
         if (etag.equal(ptr, etag.size()) == false) U_RETURN(false);
         }
      else // HTTP-date
         {
         time_t since = UDate::getSecondFromTime(ptr, true);

         U_INTERNAL_DUMP("since = %ld", since)
         U_INTERNAL_DUMP("mtime = %ld", file->st_mtime)

         if (file->st_mtime > since) U_RETURN(false);
         }
      }

   U_RETURN(true);
}

U_NO_EXPORT int UHTTP::sortHTTPRange(const void* a, const void* b)
{
   U_TRACE(0, "UHTTP::sortHTTPRange(%p,%p)", a, b)

   HTTPRange* ra = *(HTTPRange**)a;
   HTTPRange* rb = *(HTTPRange**)b;

   U_INTERNAL_DUMP("ra->start = %u ra->end = %u", ra->start, ra->end)
   U_INTERNAL_DUMP("rb->start = %u rb->end = %u", rb->start, rb->end)

   uint32_t diff = ra->start - rb->start;

   U_INTERNAL_DUMP("diff = %u", diff)

   U_RETURN(diff);
}

U_NO_EXPORT bool UHTTP::checkHTTPGetRequestForRange(uint32_t& start, uint32_t& size, UString& ext, const UString& data)
{
   U_TRACE(0, "UHTTP::checkHTTPGetRequestForRange(%u,%u,%.*S,%.*S)", start, size, U_STRING_TO_TRACE(ext), U_STRING_TO_TRACE(data))

   U_INTERNAL_ASSERT_MAJOR(http_info.range_len, 0)

   char* pend;
   HTTPRange* cur;
   HTTPRange* prev;
   const char* spec;
   uint32_t i, n, _end;
   UVector<HTTPRange*> array;
   UString range(http_info.range, http_info.range_len), tmp(100U);

   UVector<UString> range_list(range, ',');

   for (i = 0, n = range_list.size(); i < n; ++i)
      {
      cur  = new HTTPRange;
      spec = range_list[i].data();

      U_INTERNAL_DUMP("spec = %.*S", 10, spec)

      if (*spec == '-')
         {
         cur->start = strtol(spec, &pend, 0) + size;
         cur->end   = size - 1;
         }
      else
         {
         cur->start = strtol(spec, &pend, 0);

         if (*pend == '-') ++pend;

         cur->end = (*pend ? strtol(pend, &pend, 0) : size - 1);
         }

      U_INTERNAL_DUMP("cur->start = %u cur->end = %u", cur->start, cur->end)

      array.push(cur);
      }

   n = array.size();

   if (n > 1)
      {
      array.sort(sortHTTPRange);

      for (i = 1, n = array.size(); i < n; ++i)
         {
         cur  = array[i];
         prev = array[i-1];

         U_INTERNAL_DUMP("prev->start = %u prev->end = %u", prev->start, prev->end)
         U_INTERNAL_DUMP(" cur->start = %u  cur->end = %u",  cur->start,  cur->end)

         if (cur->start <= prev->end)
            {
            prev->end = U_max(prev->end, cur->end);

            array.erase(i);
            }
         }

      n = array.size();
      }

   if (n == 0)
      {
      U_http_is_connection_close = U_YES;

      setHTTPResponse(HTTP_REQ_RANGE_NOT_OK, 0, 0);

      U_RETURN(false);
      }

   if (n == 1)
      {
      // Single range

      cur   = array[0];
      _end  = cur->end;
      start = cur->start;

      if (_end >= size) _end = size - 1;

      U_INTERNAL_DUMP("start = %u _end = %u", start, _end)

      U_INTERNAL_ASSERT(start <= _end)
      U_INTERNAL_ASSERT_RANGE(start,_end,size-1)

      tmp.snprintf("Content-Range: bytes %u-%u/%u\r\n", start, _end, size);

      size = _end - start + 1;

      if (ext.empty() == false)
         {
         uint32_t pos = ext.find(*USocket::str_content_length);

         U_INTERNAL_ASSERT_DIFFERS(pos, U_NOT_FOUND)

         U_INTERNAL_DUMP("pos = %.*S", 20, ext.c_pointer(pos))

         const char* ptr = ext.c_pointer(pos + USocket::str_content_length->size() + 2);

         (void) checkHTTPContentLength(ptr, size, ext);
         }

      (void) ext.insert(0, tmp);

      U_INTERNAL_DUMP("ext = %.*S", U_STRING_TO_TRACE(ext))

      U_RETURN(true);
      }

   /* Multiple ranges, so build a multipart/byteranges response
   --------------------------
   GET /index.html HTTP/1.1
   Host: www.unirel.com
   User-Agent: curl/7.21.0 (x86_64-pc-linux-gnu) libcurl/7.21.0 GnuTLS/2.10.0 zlib/1.2.5
   Range: bytes=100-199,500-599

   --------------------------
   HTTP/1.1 206 Partial Content
   Date: Fri, 09 Jul 2010 10:27:52 GMT
   Server: Apache/2.0.49 (Linux/SuSE)
   Last-Modified: Fri, 06 Nov 2009 17:59:33 GMT
   Accept-Ranges: bytes
   Content-Length: 431
   Content-Type: multipart/byteranges; boundary=48af1db00244c25fa


   --48af1db00244c25fa
   Content-type: text/html; charset=ISO-8859-1
   Content-range: bytes 100-199/598

   ............
   --48af1db00244c25fa
   Content-type: text/html; charset=ISO-8859-1
   Content-range: bytes 500-597/598

   ............
   --48af1db00244c25fa--
   --------------------------
   */

   const char* ptr = tmp.data();

   tmp.snprintf("Content-Length: %u", size);

   UMimeMultipartMsg response("byteranges", UMimeMultipartMsg::NONE, ptr, false);

   for (i = 0; i < n; ++i)
      {
      cur   = array[i];
      _end  = cur->end;
      start = cur->start;

      U_INTERNAL_ASSERT(start <= _end)
      U_INTERNAL_ASSERT_RANGE(start,_end,size-1)

      tmp.snprintf("Content-Range: bytes %u-%u/%u", start, _end, size);

      response.add(UMimeMultipartMsg::section(data.substr(start, _end - start + 1), U_CTYPE_HTML, UMimeMultipartMsg::NONE, "", "", ptr));
      }

   UString msg;
   uint32_t content_length     = response.message(msg),
            content_length_pos = msg.find(*USocket::str_content_length);

   U_INTERNAL_ASSERT_DIFFERS(content_length_pos, U_NOT_FOUND)

   U_INTERNAL_DUMP("content_length_pos = %.*S", 20, msg.c_pointer(content_length_pos))

   const char* content_length_ptr = msg.c_pointer(content_length_pos + USocket::str_content_length->size() + 2);

   (void) checkHTTPContentLength(content_length_ptr, content_length, msg);

#ifdef DEBUG
   (void) UFile::writeToTmpl("/tmp/byteranges", msg);
#endif

   *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(HTTP_PARTIAL, msg);

   U_RETURN(false);
}

#define U_NO_Etag                // for me it's enough Last-Modified: ...
#define U_NO_If_Unmodified_Since // I think it's not very much used...

U_NO_EXPORT bool UHTTP::checkHTTPGetRequestIfModified()
{
   U_TRACE(0, "UHTTP::checkHTTPGetRequestIfModified()")

   /*
   The If-Modified-Since: header is used with a GET request. If the requested resource has been modified since the given date,
   ignore the header and return the resource as you normally would. Otherwise, return a "304 Not Modified" response, including
   the Date: header and no message body, like

   HTTP/1.1 304 Not Modified
   Date: Fri, 31 Dec 1999 23:59:59 GMT
   [blank line here]
   */

   if (http_info.if_modified_since)
      {
      U_INTERNAL_DUMP("since = %ld", http_info.if_modified_since)
      U_INTERNAL_DUMP("mtime = %ld", file->st_mtime)

      if (file->st_mtime <= http_info.if_modified_since)
         {
         setHTTPResponse(HTTP_NOT_MODIFIED, 0, 0);

         U_RETURN(false);
         }
      }
#ifndef U_NO_If_Unmodified_Since
   else
      {
      /*
      The If-Unmodified-Since: header is similar, but can be used with any method. If the requested resource has not been modified
      since the given date, ignore the header and return the resource as you normally would. Otherwise, return a "412 Precondition Failed"
      response, like

      HTTP/1.1 412 Precondition Failed
      Date: Fri, 31 Dec 1999 23:59:59 GMT
      [blank line here]
      */

      const char* ptr = getHTTPHeaderValuePtr(*USocket::str_if_unmodified_since);

      if (ptr)
         {
         time_t since = UDate::getSecondFromTime(ptr, true);

         U_INTERNAL_DUMP("since = %ld", since)
         U_INTERNAL_DUMP("mtime = %ld", file->st_mtime)

         if (file->st_mtime > since)
            {
            U_is_connection_close = U_YES;

            setHTTPResponse(HTTP_PRECON_FAILED, 0, 0);

            U_RETURN(false);
            }
         }
      }
#endif

   U_RETURN(true);
}

void UHTTP::processHTTPGetRequest()
{
   U_TRACE(0, "UHTTP::processHTTPGetRequest()")

   U_ASSERT(UClientImage_Base::body->empty())

   // If the browser has to validate a component, it uses the If-None-Match header to pass the ETag back to
   // the origin server. If the ETags match, a 304 status code is returned reducing the response...

   UString etag, ext(U_CAPACITY);

#ifndef U_NO_Etag
   etag = file->etag();

   const char* ptr = getHTTPHeaderValuePtr(*USocket::str_if_none_match);

   if (ptr)
      {
      U_INTERNAL_ASSERT_EQUALS(*ptr, '"') // entity-tag

      if (etag.equal(ptr, etag.size()))
         {
         setHTTPResponse(HTTP_NOT_MODIFIED, 0, 0);

         return;
         }
      }

   ext.snprintf("Etag: %.*s\r\n", U_STRING_TO_TRACE(etag));
#endif

   if (checkHTTPGetRequestIfModified() == false) return;

   UString mmap;
   const char* suffix = 0;
   bool bdir, isSSL = false;
   int nResponseCode = HTTP_OK;
   uint32_t start = 0, size = 0;
   const char* content_type = 0;

   if (openFile() == false) goto end;

   if (file->dir())
      {
      // check if it's OK to do directory listing via authentication (digest|basic)

      if (processHTTPAuthorization() == false)
         {
         setHTTPUnAuthorized();

         return;
         }

      bdir         = true;
      content_type = U_CTYPE_HTML;

      if (isHttpHEAD()) size = getHTMLDirectoryList().size();
      else
         {
         *UClientImage_Base::body = getHTMLDirectoryList();

         if (U_http_is_accept_deflate)
            {
            (void) ext.append(U_CONSTANT_TO_PARAM("Content-Encoding: deflate\r\n"));

            *UClientImage_Base::body = UStringExt::deflate(*UClientImage_Base::body);
            }

         size = UClientImage_Base::body->size();
         }
      }
   else
      {
      bdir = false;
      size = file->getSize();

      if (size &&
          file->memmap(PROT_READ, UClientImage_Base::body))
         {
         suffix = file->getSuffix();

         if (http_info.range_len &&
             checkHTTPGetRequestIfRange(etag))
            {
            // The Range: header is used with a GET request.
            // For example assume that will return only a portion (let's say the first 32 bytes) of the requested resource...
            //
            // Range: bytes=0-31

            if (checkHTTPGetRequestForRange(start, size, ext, *UClientImage_Base::body) == false)
               {
               UClientImage_Base::body->clear(); // NB: this make also the unmmap() of file...

               goto end;
               }

            nResponseCode = HTTP_PARTIAL;
            }
         }
      }

   getFileMimeType(suffix, content_type, ext, size);

   // build response...

   *UClientImage_Base::wbuffer = getHTTPHeaderForResponse(nResponseCode, ext);

#ifdef HAVE_SSL
   if (UClientImage_Base::socket->isSSL()) isSSL = true;
#endif

   // NB: check if we need to send the body with writev()...

   if (bdir        ||
       isSSL       ||
       size < 4096 ||
       isHttpHEAD())
      {
      if (nResponseCode == HTTP_PARTIAL)
         {
         mmap                     = *UClientImage_Base::body;
         *UClientImage_Base::body =  mmap.substr(start, size);
         }
      }
   else
      {
      // NB: we use sendfile()...

      UClientImage_Base::body->clear(); // NB: this make also the unmmap() of file...

      /* On Linux, sendfile() depends on the TCP_CORK socket option to avoid undesirable packet boundaries
       * ------------------------------------------------------------------------------------------------------
       * if we set the TCP_CORK option on the socket, our header packet will be padded with the bulk data
       * and all the data will be transferred automatically in the packets according to size. When finished
       * with the bulk data transfer, it is advisable to uncork the connection by unsetting the TCP_CORK option
       * so that any partial frames that are left can go out. This is equally important to corking. To sum it up,
       * we recommend setting the TCP_CORK option when you're sure that you will be sending multiple data sets
       * together (such as header and a body of HTTP response), with no delays between them. This can greatly
       * benefit the performance of WWW, FTP, and file servers, as well as simplifying your life
       * ------------------------------------------------------------------------------------------------------
       */

      UClientImage_Base::socket->setTcpCork(1U); // NB: must be here, before the first write...
      }

   (void) UClientImage_Base::pClientImage->handlerWrite();
          UClientImage_Base::write_off = true;

   if (UClientImage_Base::body->empty())
      {
      if (size == 0 || isHttpHEAD()) goto end;

      off_t lstart = start;

      if (file->sendfile(UClientImage_Base::socket->getFd(), &lstart, size)) UClientImage_Base::socket->setTcpCork(0U);
      }
#ifdef DEBUG
   else if (nResponseCode == HTTP_PARTIAL) UClientImage_Base::body->clear(); // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...
#endif

end:
   if (file->isOpen()) file->close();
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

U_EXPORT const char* UHTTP::UServletPage::dump(bool reset) const
{
   UDynamic::dump(false);

   *UObjectIO::os << '\n'                                            
                  << "runDynamicPage" << (void*)runDynamicPage << '\n';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

U_EXPORT const char* UHTTP::RewriteRule::dump(bool reset) const
{
   *UObjectIO::os
#              ifdef HAVE_PCRE
                  << "key         (UPCRE   " << (void*)&key         << ")\n"
#              endif
                  << "replacement (UString " << (void*)&replacement << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

U_EXPORT const char* UHTTP::UFileCacheData::dump(bool reset) const
{
   *UObjectIO::os << "wd                      " << wd            << '\n'
                  << "size                    " << size          << '\n'
                  << "mode                    " << mode          << '\n'
                  << "mtime                   " << mtime         << '\n'
                  << "array (UVector<UString> " << (void*)&array << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif

/*
#if defined(U_ALL_CPP) && !defined(DEBUG)
# undef  U_DUMP
# define U_DUMP(args...)
# undef  U_TRACE
# define U_TRACE(level,args...)
# undef  U_INTERNAL_DUMP
# define U_INTERNAL_DUMP(args...)
# undef  U_INTERNAL_TRACE
# define U_INTERNAL_TRACE(args...)
#endif
*/
