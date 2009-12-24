// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    ldap.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/ldap/ldap.h>

struct timeval ULDAP::timeOut = { 10, 0 }; /* 10 second connection/search timeout */

ULDAPEntry::ULDAPEntry(int num_names, const char** names, int num_entry)
{
   U_TRACE_REGISTER_OBJECT(0, ULDAPEntry, "%d,%p,%d", num_names, names, num_entry)

   U_INTERNAL_ASSERT_EQUALS(names[num_names], 0)

   U_DUMP_ATTRS(names)

   n_attr    = num_names;
   n_entry   = num_entry;
   dn        = U_MALLOC_N(num_entry,             char*);
   attr_val  = U_MALLOC_N(num_entry * num_names, UString*);
   attr_name = names;

   (void) memset(dn,       0, (num_entry             * sizeof(char*)));
   (void) memset(attr_val, 0, (num_entry * num_names * sizeof(UString*)));
}

ULDAPEntry::~ULDAPEntry()
{
   U_TRACE_UNREGISTER_OBJECT(0, ULDAPEntry)

   int i = 0, j, k = 0;

   for (; i < n_entry; ++i)
      {
      if (dn[i])
         {
         U_INTERNAL_DUMP("dn[%d]: %S", i, dn[i])

         ldap_memfree(dn[i]);

         for (j = 0; j < n_attr; ++j, ++k)
            {
            if (attr_val[k])
               {
               U_INTERNAL_DUMP("ULDAPEntry(%d): %S = %.*S", k, attr_name[j], U_STRING_TO_TRACE(*attr_val[k]))

               delete attr_val[k];
               }
            }
         }
      }

   U_FREE_N(dn,       n_entry,          char*);
   U_FREE_N(attr_val, n_entry * n_attr, UString*);
}

void ULDAPEntry::set(char* attribute, char** values, int index_entry)
{
   U_TRACE(0, "ULDAPEntry::set(%S,%p,%d)", attribute, values, index_entry)

   U_INTERNAL_ASSERT_MINOR(index_entry, n_entry)

   int k = index_entry * n_attr;

   for (int j = 0; j < n_attr; ++j, ++k)
      {
      if (strcmp(attr_name[j], attribute) == 0)
         {
         U_INTERNAL_DUMP("ULDAPEntry(%d): %S", k, attr_name[j])

         attr_val[k] = U_NEW(UString((void*)values[0]));

         for (j = 1; values[j] != 0; ++j)
            {
            attr_val[k]->append(U_CONSTANT_TO_PARAM("; "));
            attr_val[k]->append(values[j]);
            }

         U_INTERNAL_DUMP("value = %.*S", U_STRING_TO_TRACE(*attr_val[k]))

         break;
         }
      }
}

UString ULDAPEntry::getString(int index_names, int index_entry)
{
   U_TRACE(0, "ULDAPEntry::getString(%d,%d)", index_names, index_entry)

   U_INTERNAL_ASSERT_MINOR(index_names, n_attr)
   U_INTERNAL_ASSERT_MINOR(index_entry, n_entry)

   int k = (index_entry * n_attr) + index_names;

   if (attr_val[k])
      {
      UString str = *(attr_val[k]);

      U_RETURN_STRING(str);
      }

   U_RETURN_STRING(UString::getStringNull());
}

const char* ULDAPEntry::getCStr(int index_names, int index_entry)
{
   U_TRACE(0, "ULDAPEntry::getCStr(%d,%d)", index_names, index_entry)

   U_INTERNAL_ASSERT_MINOR(index_names, n_attr)
   U_INTERNAL_ASSERT_MINOR(index_entry, n_entry)

   int k = (index_entry * n_attr) + index_names;

   if (attr_val[k])
      {
      const char* str = attr_val[k]->c_str();

      U_RETURN(str);
      }

   U_RETURN("");
}

ULDAP::~ULDAP()
{
   U_TRACE_UNREGISTER_OBJECT(0, ULDAP)

   if (ludpp) U_SYSCALL_VOID(ldap_free_urldesc, "%p", ludpp);

   if (ld)
      {
      if (searchResult) U_SYSCALL(ldap_msgfree, "%p", searchResult);

      U_SYSCALL(ldap_unbind_s, "%p", ld);

#  ifdef HAVE_LDAP_SSL_H
   // if (isSecure) U_SYSCALL_NO_PARAM(ldapssl_client_deinit);
#  endif
      }
}

const char* ULDAP::error()
{
   U_TRACE(0, "ULDAP::error()")

   U_CHECK_MEMORY

   static const char* errlist[] = {
   "LDAP_SUCCESS",                        //  0 0x00
   "LDAP_OPERATIONS_ERROR",               //  1 0x01
   "LDAP_PROTOCOL_ERROR",                 //  2 0x02
   "LDAP_TIMELIMIT_EXCEEDED",             //  3 0x03
   "LDAP_SIZELIMIT_EXCEEDED",             //  4 0x04
   "LDAP_COMPARE_FALSE",                  //  5 0x05
   "LDAP_COMPARE_TRUE",                   //  6 0x06
   "LDAP_STRONG_AUTH_NOT_SUPPORTED",      //  7 0x07
   "LDAP_STRONG_AUTH_REQUIRED",           //  8 0x08
   "LDAP_PARTIAL_RESULTS",                //  9 0x09
   "LDAP_REFERRAL",                       // 10
   "LDAP_ADMINLIMIT_EXCEEDED",            // 11
   "LDAP_UNAVAILABLE_CRITICAL_EXTENSION", // 12
   "LDAP_CONFIDENTIALITY_REQUIRED",       // 13
   "LDAP_SASL_BIND_IN_PROGRESS",          // 14
   "",                                    // 15
   "LDAP_NO_SUCH_ATTRIBUTE",              // 16 0x10
   "LDAP_UNDEFINED_TYPE",                 // 17 0x11
   "LDAP_INAPPROPRIATE_MATCHING",         // 18 0x12
   "LDAP_CONSTRAINT_VIOLATION",           // 19 0x13
   "LDAP_TYPE_OR_VALUE_EXISTS",           // 20 0x14
   "LDAP_INVALID_SYNTAX",                 // 21 0x15
   "",                                    // 22
   "",                                    // 23
   "",                                    // 24
   "",                                    // 25
   "",                                    // 26
   "",                                    // 27
   "",                                    // 28
   "",                                    // 29
   "",                                    // 30
   "",                                    // 31
   "LDAP_NO_SUCH_OBJECT",                 // 32 0x20
   "LDAP_ALIAS_PROBLEM",                  // 33 0x21
   "LDAP_INVALID_DN_SYNTAX",              // 34 0x22
   "LDAP_IS_LEAF",                        // 35 0x23
   "LDAP_ALIAS_DEREF_PROBLEM",            // 36 0x24
   "",                                    // 37
   "",                                    // 38
   "",                                    // 39
   "",                                    // 40
   "",                                    // 41
   "",                                    // 42
   "",                                    // 43
   "",                                    // 44
   "",                                    // 45
   "",                                    // 46
   "",                                    // 47
   "LDAP_INAPPROPRIATE_AUTH",             // 48 0x30
   "LDAP_INVALID_CREDENTIALS",            // 49 0x31
   "LDAP_INSUFFICIENT_ACCESS",            // 50 0x32
   "LDAP_BUSY",                           // 51 0x33
   "LDAP_UNAVAILABLE",                    // 52 0x34
   "LDAP_UNWILLING_TO_PERFORM",           // 53 0x35
   "LDAP_LOOP_DETECT",                    // 54 0x36
   "",                                    // 55
   "",                                    // 56
   "",                                    // 57
   "",                                    // 58
   "",                                    // 59
   "",                                    // 60
   "",                                    // 61
   "",                                    // 62
   "",                                    // 63
   "LDAP_NAMING_VIOLATION",               // 64 0x40
   "LDAP_OBJECT_CLASS_VIOLATION",         // 65 0x41
   "LDAP_NOT_ALLOWED_ON_NONLEAF",         // 66 0x42
   "LDAP_NOT_ALLOWED_ON_RDN",             // 67 0x43
   "LDAP_ALREADY_EXISTS",                 // 68 0x44
   "LDAP_NO_OBJECT_CLASS_MODS",           // 69 0x45
   "LDAP_RESULTS_TOO_LARGE",              // 70 0x46
   "LDAP_AFFECTS_MULTIPLE_DSAS",          // 71
   "",                                    // 72
   "",                                    // 73
   "",                                    // 74
   "",                                    // 75
   "",                                    // 76
   "",                                    // 77
   "",                                    // 78
   "",                                    // 79
   "LDAP_OTHER",                          // 80 0x50
   "LDAP_SERVER_DOWN",                    // 81 0x51
   "LDAP_LOCAL_ERROR",                    // 82 0x52
   "LDAP_ENCODING_ERROR",                 // 83 0x53
   "LDAP_DECODING_ERROR",                 // 84 0x54
   "LDAP_TIMEOUT",                        // 85 0x55
   "LDAP_AUTH_UNKNOWN",                   // 86 0x56
   "LDAP_FILTER_ERROR",                   // 87 0x57
   "LDAP_USER_CANCELLED",                 // 88 0x58
   "LDAP_PARAM_ERROR",                    // 89 0x59
   "LDAP_NO_MEMORY"                       // 90 0x5a
   "LDAP_CONNECT_ERROR",                  // 91 0x5b
   "LDAP_NOT_SUPPORTED",                  // 92 0x5c
   "LDAP_CONTROL_NOT_FOUND",              // 93 0x5d
   "LDAP_NO_RESULTS_RETURNED",            // 94 0x5e
   "LDAP_MORE_RESULTS_TO_RETURN",         // 95 0x5f
   "LDAP_CLIENT_LOOP",                    // 96 0x60
   "LDAP_REFERRAL_LIMIT_EXCEEDED"         // 97 0x61
   };

   static char buffer[1024];

   char* descr = ldap_err2string(result);

#ifdef HAVE_LDAP_SSL_H
   /*
    * get a meaningful error string back from the security library
    * this function should be called, if ldap_err2string doesn't
    * identify the error code.
    */

   if (descr == 0) descr = (char*)ldapssl_err2string(result);
#endif

   (void) sprintf(buffer, "%s (%d, %s)", (result >= 0 && result < 97 ? errlist[result] : ""), result, descr);

   U_RETURN(buffer);
}

/* Initialize the LDAP session */

bool ULDAP::init(const char* url)
{
   U_TRACE(1, "ULDAP::init(%S)", url)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_EQUALS(ludpp,0)

   result = U_SYSCALL(ldap_url_parse, "%S,%p", url, &ludpp);

   if (result != LDAP_SUCCESS)
      {
#  ifdef DEBUG
      char buffer[1024];

      switch (result)
         {
         case  LDAP_URL_ERR_MEM:
            (void) sprintf(buffer, "LDAP_URL_ERR_MEM (%d, %s)", result, "Cannot allocate memory space");
         break;

         case  LDAP_URL_ERR_PARAM:
            (void) sprintf(buffer, "LDAP_URL_ERR_PARAM (%d, %s)", result, "Invalid parameter");
         break;

         case  LDAP_URL_ERR_BADSCOPE:
            (void) sprintf(buffer, "LDAP_URL_ERR_BADSCOPE (%d, %s)", result, "Invalid or missing scope string");
         break;

#     ifdef HAVE_LDAP_SSL_H
         case  LDAP_URL_ERR_NOTLDAP:
            (void) sprintf(buffer, "LDAP_URL_ERR_NOTLDAP (%d, %s)", result, "URL doesn't begin with \"ldap://\"");
         break;

         case  LDAP_URL_ERR_NODN:
            (void) sprintf(buffer, "LDAP_URL_ERR_NODN (%d, %s)", result, "URL has no DN (required)");
         break;

         case  LDAP_URL_UNRECOGNIZED_CRITICAL_EXTENSION:
            (void) sprintf(buffer, "LDAP_URL_UNRECOGNIZED_CRITICAL_EXTENSION (%d, %s)", result, "");
         break;
#     else
         case  LDAP_URL_ERR_BADSCHEME:
            (void) sprintf(buffer, "LDAP_URL_ERR_BADSCHEME (%d, %s)", result, "URL doesnt begin with \"ldap[s]://\"");
         break;

         case  LDAP_URL_ERR_BADENCLOSURE:
            (void) sprintf(buffer, "LDAP_URL_ERR_BADENCLOSURE (%d, %s)", result, "URL is missing trailing \">\"");
         break;

         case  LDAP_URL_ERR_BADURL:
            (void) sprintf(buffer, "LDAP_URL_ERR_BADURL (%d, %s)", result, "Invalid URL");
         break;

         case  LDAP_URL_ERR_BADHOST:
            (void) sprintf(buffer, "LDAP_URL_ERR_BADHOST (%d, %s)", result, "Host port is invalid");
         break;

         case  LDAP_URL_ERR_BADATTRS:
            (void) sprintf(buffer, "LDAP_URL_ERR_BADATTRS (%d, %s)", result, "Invalid or missing attributes");
         break;

         case  LDAP_URL_ERR_BADFILTER:
            (void) sprintf(buffer, "LDAP_URL_ERR_BADFILTER (%d, %s)", result, "Invalid or missing filter");
         break;

         case  LDAP_URL_ERR_BADEXTS:
            (void) sprintf(buffer, "LDAP_URL_ERR_BADEXTS (%d, %s)", result, "Invalid or missing extensions");
         break;
#     endif

         default:
            (void) sprintf(buffer, "??? (%d, %s)", result, "");
         break;
         }

      U_INTERNAL_DUMP("ldap_url_parse() failed - %s", buffer);
#  endif

      U_RETURN(false);
      }

   /* Get the URL scheme (either ldap or ldaps) */

#ifdef HAVE_LDAP_SSL_H
   if (memcmp(url, U_CONSTANT_TO_PARAM("ldaps:")) == 0)
      {
      /* Making encrypted connection */

      isSecure = true;

      /*
       * Initialize the ssl library. The first parameter of
       * ldapssl_client_init is a certificate file. However, when used
       * the file must be a DER encoded file. 0 is passed in for the
       * certificate file because ldapssl_set_verify_mode will be used
       * to specify no server certificate verification.
       * ldapssl_client_init is an application level initialization not a
       * thread level initilization and should be done once.
       */

      result = U_SYSCALL(ldapssl_client_init, "%p,%p", 0,  /* DER encoded cert file */
                                                       0); /* reserved, use 0 */

      if (result != LDAP_SUCCESS) U_RETURN(false);

#  ifdef LDAPSSL_VERIFY_NONE
      /*
       * Configure the LDAP SSL library to not verify the server certificate.
       * The default is LDAPSSL_VERIFY_SERVER which validates all servers
       * against the trusted certificates normally passed in
       * during ldapssl_client_init or ldapssl_add_trusted_cert.
       *
       * WARNING:  Setting the verify mode to LDAPSSL_VERIFY_NONE turns off
       * server certificate verification.  This means all servers are
       * considered trusted.  This should be used only in controlled
       * environments where encrypted communication between servers and
       * clients is desired but server verification is not necessary.
       */

      result = U_SYSCALL(ldapssl_set_verify_mode, "%d", LDAPSSL_VERIFY_NONE);

      if (result != LDAP_SUCCESS)
         {
      // U_SYSCALL_NO_PARAM(ldapssl_client_deinit);

         U_RETURN(false);
         }
#  endif

      /* create a LDAP session handle that is enabled for ssl connection */

      ld = (LDAP*) U_SYSCALL(ldapssl_init, "%S,%d,%d", ludpp->lud_host,   /* host name */
                        (ludpp->lud_port ? ludpp->lud_port : LDAPS_PORT), /* port number */
                        1);                                               /* 0 - clear text, 1 - enable for ssl */

      if (ld == 0)
         {
      // U_SYSCALL_NO_PARAM(ldapssl_client_deinit);

         U_RETURN(false);
         }
      }
   else
#endif
      {
      /* Making clear text connection */

      if (init(ludpp->lud_host, ludpp->lud_port) == false) U_RETURN(false);
      }

   U_RETURN(true);
}

void ULDAP::get(ULDAPEntry& e)
{
   U_TRACE(1, "ULDAP::get(%p)", &e)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(ld)
   U_INTERNAL_ASSERT_POINTER(searchResult)

   /* Go through the search results by checking entries */

   int k = 0;
   char** values;
   char* attribute;

   LDAPMessage* entry;
   BerElement* ber = 0;

   for (entry = ldap_first_entry(ld, searchResult); entry != 0;
        entry = ldap_next_entry( ld, entry), ++k)
      {
      e.dn[k] = ldap_get_dn(ld, entry);

      U_INTERNAL_DUMP("dn[%d]: %S", k, e.dn[k])

      for (attribute = ldap_first_attribute(ld, entry, &ber); attribute != 0;
           attribute = ldap_next_attribute( ld, entry,  ber))
         {
         values = ldap_get_values(ld, entry, attribute); /* Get values */

         U_INTERNAL_ASSERT_POINTER(values)

         e.set(attribute, values, k);

         ldap_value_free(values);

         ldap_memfree(attribute);
         }

      ber_free(ber, 0);
      }
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* ULDAPEntry::dump(bool reset) const
{
   *UObjectIO::os << "dn                 " << (void*)dn        << '\n'
                  << "n_attr             " << n_attr           << '\n'
                  << "n_entry            " << n_entry          << '\n'
                  << "attr_val           " << (void*)attr_val  << '\n'
                  << "attr_name          " << (void*)attr_name;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

const char* ULDAP::dump(bool reset) const
{
   *UObjectIO::os << "ld                 " << (void*)ld           << '\n'
                  << "ludpp              " << (void*)ludpp        << '\n'
                  << "result             " << result              << '\n'
                  << "isSecure           " << isSecure            << '\n'
                  << "pTimeOut           " << (void*)pTimeOut     << '\n'
                  << "searchResult       " << (void*)searchResult;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
