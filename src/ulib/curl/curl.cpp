// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    curl.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/curl/curl.h>

bool   UCURL::inited;
char   UCURL::errorBuffer[CURL_ERROR_SIZE];
UCURL* UCURL::first;
CURLM* UCURL::multiHandle;

const char* UCURL::error()
{
   U_TRACE(0, "UCURL::error()")

   U_CHECK_MEMORY

   static const char* errlist[] = {
      "CURLE_OK",                         //  0 0x00
      "CURLE_UNSUPPORTED_PROTOCOL",       //  1 0x01
      "CURLE_FAILED_INIT",                //  2 0x02
      "CURLE_URL_MALFORMAT",              //  3 0x03
      "CURLE_URL_MALFORMAT_USER",         //  4 0x04
      "CURLE_COULDNT_RESOLVE_PROXY",      //  5 0x05
      "CURLE_COULDNT_RESOLVE_HOST",       //  6 0x06
      "CURLE_COULDNT_CONNECT",            //  7 0x07
      "CURLE_FTP_WEIRD_SERVER_REPLY",     //  8 0x08
      "CURLE_FTP_ACCESS_DENIED",          //  9 0x09
      "CURLE_FTP_USER_PASSWORD_INCORRECT",// 10
      "CURLE_FTP_WEIRD_PASS_REPLY",       // 11
      "CURLE_FTP_WEIRD_USER_REPLY",       /* 12 */
      "CURLE_FTP_WEIRD_PASV_REPLY",       /* 13 */
      "CURLE_FTP_WEIRD_227_FORMAT",       /* 14 */
      "CURLE_FTP_CANT_GET_HOST",          /* 15 */
      "CURLE_FTP_CANT_RECONNECT",         /* 16 */
      "CURLE_FTP_COULDNT_SET_BINARY",     /* 17 */
      "CURLE_PARTIAL_FILE",               /* 18 */
      "CURLE_FTP_COULDNT_RETR_FILE",      /* 19 */
      "CURLE_FTP_WRITE_ERROR",            /* 20 */
      "CURLE_FTP_QUOTE_ERROR",            /* 21 */
      "CURLE_HTTP_RETURNED_ERROR",        /* 22 */
      "CURLE_WRITE_ERROR",                /* 23 */
      "CURLE_MALFORMAT_USER",             /* 24 - NOT USED */
      "CURLE_FTP_COULDNT_STOR_FILE",      /* 25 - failed FTP upload */
      "CURLE_READ_ERROR",                 /* 26 - could open/read from file */
      "CURLE_OUT_OF_MEMORY",              /* 27 */
      "CURLE_OPERATION_TIMEOUTED",        /* 28 - the timeout time was reached */
      "CURLE_FTP_COULDNT_SET_ASCII",      /* 29 - TYPE A failed */
      "CURLE_FTP_PORT_FAILED",            /* 30 - FTP PORT operation failed */
      "CURLE_FTP_COULDNT_USE_REST",       /* 31 - the REST command failed */
      "CURLE_FTP_COULDNT_GET_SIZE",       /* 32 - the SIZE command failed */
      "CURLE_HTTP_RANGE_ERROR",           /* 33 - RANGE "command" didn't work */
      "CURLE_HTTP_POST_ERROR",            /* 34 */
      "CURLE_SSL_CONNECT_ERROR",          /* 35 - wrong when connecting with SSL */
      "CURLE_BAD_DOWNLOAD_RESUME",        /* 36 - couldn't resume download */
      "CURLE_FILE_COULDNT_READ_FILE",     /* 37 */
      "CURLE_LDAP_CANNOT_BIND",           /* 38 */
      "CURLE_LDAP_SEARCH_FAILED",         /* 39 */
      "CURLE_LIBRARY_NOT_FOUND",          /* 40 */
      "CURLE_FUNCTION_NOT_FOUND",         /* 41 */
      "CURLE_ABORTED_BY_CALLBACK",        /* 42 */
      "CURLE_BAD_FUNCTION_ARGUMENT",      /* 43 */
      "CURLE_BAD_CALLING_ORDER",          /* 44 - NOT USED */
      "CURLE_INTERFACE_FAILED",           /* 45 - CURLOPT_INTERFACE failed */
      "CURLE_BAD_PASSWORD_ENTERED",       /* 46 - NOT USED */
      "CURLE_TOO_MANY_REDIRECTS" ,        /* 47 - catch endless re-direct loops */
      "CURLE_UNKNOWN_TELNET_OPTION",      /* 48 - User specified an unknown option */
      "CURLE_TELNET_OPTION_SYNTAX" ,      /* 49 - Malformed telnet option */
      "CURLE_OBSOLETE",                   /* 50 - NOT USED */
      "CURLE_SSL_PEER_CERTIFICATE",       /* 51 - peer's certificate wasn't ok */
      "CURLE_GOT_NOTHING",                /* 52 - when this is a specific error */
      "CURLE_SSL_ENGINE_NOTFOUND",        /* 53 - SSL crypto engine not found */
      "CURLE_SSL_ENGINE_SETFAILED",       /* 54 - can not set SSL crypto engine as default */
      "CURLE_SEND_ERROR",                 /* 55 - failed sending network data */
      "CURLE_RECV_ERROR",                 /* 56 - failure in receiving network data */
      "CURLE_SHARE_IN_USE",               /* 57 - share is in use */
      "CURLE_SSL_CERTPROBLEM",            /* 58 - problem with the local certificate */
      "CURLE_SSL_CIPHER",                 /* 59 - couldn't use specified cipher */
      "CURLE_SSL_CACERT",                 /* 60 - problem with the CA cert (path?) */
      "CURLE_BAD_CONTENT_ENCODING",       /* 61 - Unrecognized transfer encoding */
      "CURLE_LDAP_INVALID_URL",           /* 62 - Invalid LDAP URL */
      "CURLE_FILESIZE_EXCEEDED",          /* 63 - Maximum file size exceeded */
      "CURLE_FTP_SSL_FAILED",             /* 64 - Requested FTP SSL level failed */
      "CURLE_SEND_FAIL_REWIND",           /* 65 - Sending the data requires a rewind that failed */
      "CURLE_SSL_ENGINE_INITFAILED",      /* 66 - failed to initialise ENGINE */
      "CURLE_LOGIN_DENIED",               /* 67 - user, password or similar was not accepted and we failed to login */
      "CURLE_TFTP_NOTFOUND",              /* 68 - file not found on server */
      "CURLE_TFTP_PERM",                  /* 69 - permission problem on server */
      "CURLE_TFTP_DISKFULL",              /* 70 - out of disk space on server */
      "CURLE_TFTP_ILLEGAL",               /* 71 - Illegal TFTP operation */
      "CURLE_TFTP_UNKNOWNID",             /* 72 - Unknown transfer ID */
      "CURLE_TFTP_EXISTS",                /* 73 - File already exists */
      "CURLE_TFTP_NOSUCHUSER",            /* 74 - No such user */
   };

   static char buffer[CURL_ERROR_SIZE+32];

   (void) sprintf(buffer, "%s (%d, %s)", (result >= 0 && result < CURL_LAST ? errlist[result] : ""), result, errorBuffer);

   U_RETURN(buffer);
}

U_NO_EXPORT void UCURL::setup()
{
   U_TRACE(1, "UCURL::setup()")

   U_INTERNAL_ASSERT_EQUALS(inited,false)

   inited = true;

   CURLcode result = (CURLcode) U_SYSCALL(curl_global_init, "%d", CURL_GLOBAL_NOTHING);

   U_INTERNAL_ASSERT_EQUALS(result,CURLE_OK)

   multiHandle = U_SYSCALL_NO_PARAM(curl_multi_init);

   U_INTERNAL_ASSERT_POINTER(multiHandle)

#ifdef HAVE_SSL
   ULib_init_openssl();
#endif
}

U_NO_EXPORT size_t UCURL::writeFunction(void* ptr, size_t size, size_t nmemb, void* obj)
{
   U_TRACE(0, "UCURL::writeFunction(%p,%lu,%lu,%p)", ptr, size, nmemb, obj)

   size_t len = size * nmemb;

   (void) ((UCURL*)obj)->response.reserve(((UCURL*)obj)->response.size() + len);

   ((UCURL*)obj)->response.append((const char*)ptr, len);

   U_RETURN(len);
}

UCURL::UCURL() : response(U_CAPACITY)
{
   U_TRACE_REGISTER_OBJECT(0, UCURL, "")

   added     = false;
   formPost  = 0;
   formLast  = 0;

   if (!inited) setup();

   easyHandle = U_SYSCALL_NO_PARAM(curl_easy_init);

   U_INTERNAL_ASSERT_POINTER(easyHandle)

#ifdef DEBUG
   setOption(CURLOPT_VERBOSE, 1L);
#endif
   setOption(CURLOPT_ERRORBUFFER,   (long)UCURL::errorBuffer);
   setOption(CURLOPT_NOSIGNAL,      1L);
   setOption(CURLOPT_WRITEFUNCTION, (long)UCURL::writeFunction);
   setOption(CURLOPT_WRITEDATA,     (long)this);

   // insert into list

   next  = first;
   first = this;
}

UCURL::~UCURL()
{
   U_TRACE_UNREGISTER_OBJECT(0, UCURL)

   U_INTERNAL_ASSERT_POINTER(easyHandle)

   if (added) removeHandle();

   U_SYSCALL_VOID(curl_easy_cleanup, "%p", easyHandle);

   // remove from list

   UCURL* entry =  first;
   UCURL** ptr  = &first;

   do {
      if (entry == this)
         {
         *ptr = entry->next;

         break;
         }

      ptr = &(*ptr)->next;
      }
   while ((entry = *ptr));
}

void UCURL::infoComplete()
{
   U_TRACE(1, "UCURL::infoComplete()")

   U_CHECK_MEMORY

   if (result != CURLE_OK)
      {
      const char* err = error();

      U_INTERNAL_DUMP("status = %.*S", 512, err)

      U_WARNING("File transfer terminated with error from libcurl %s", err);
      }

   if (formPost)
      {
      U_SYSCALL_VOID(curl_formfree, "%p", formPost);

      formPost = 0;
      formLast = 0;
      }

   if (added) removeHandle();
}

bool UCURL::perform()
{
   U_TRACE(1, "UCURL::perform()")

   UCURL* entry;
   CURLMcode result;
   int msgs_in_queue;
   CURLMsg* pendingMsg;
   int activeTransfers = 0;

   if (inited == false) setup();

   while (true)
      {
      result = (CURLMcode) U_SYSCALL(curl_multi_perform, "%p,%p", multiHandle, &activeTransfers);

      U_INTERNAL_DUMP("activeTransfers = %d", activeTransfers)

      if (result != CURLM_CALL_MULTI_PERFORM) break;
      }

   if (result == CURLM_OK)
      {
      while (true)
         {
         pendingMsg = (CURLMsg*) U_SYSCALL(curl_multi_info_read, "%p,%p", multiHandle, &msgs_in_queue);

         if (!pendingMsg) break;

         // search into list

         for (entry = first; entry; entry = entry->next)
            {
            if (entry->easyHandle == pendingMsg->easy_handle)
               {
               entry->infoComplete();

               break;
               }
            }

         if (msgs_in_queue <= 0) break;
         }

      U_RETURN(true);
      }
   else
      {
      U_WARNING("Error while doing multi_perform from libcurl %d : %s", result, errorBuffer);

      U_RETURN(false);
      }
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UCURL::dump(bool reset) const
{
   *UObjectIO::os << "added             " << added              << '\n'
                  << "result            " << result             << '\n'
                  << "inited            " << inited             << '\n'
                  << "formPost          " << (void*)formPost    << '\n'
                  << "formLast          " << (void*)formLast    << '\n'
                  << "easyHandle        " << (void*)easyHandle  << '\n'
                  << "multiHandle       " << (void*)multiHandle << '\n'
                  << "response (UString " << (void*)&response   << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
