// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mod_ssi.cpp - this is a plugin SSI for userver
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/url.h>
#include <ulib/date.h>
#include <ulib/command.h>
#include <ulib/tokenizer.h>
#include <ulib/file_config.h>
#include <ulib/utility/uhttp.h>
#include <ulib/utility/escape.h>
#include <ulib/utility/services.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/xml_escape.h>
#include <ulib/utility/string_ext.h>
#include <ulib/net/server/plugin/mod_ssi.h>

// Server Side Include (SSI) commands are executed by the server as it parses your HTML file.
// Server side includes can be used to include the value of various server environment variables
// within your HTML such as the local date and time. One might use a server side include to add
// a signature file to an HTML file or company logo. 

#ifdef HAVE_MODULES
U_CREAT_FUNC(mod_ssi, USSIPlugIn)
#endif

int      USSIPlugIn::alternative_response;
bool     USSIPlugIn::use_size_abbrev;
time_t   USSIPlugIn::last_modified;
UString* USSIPlugIn::errmsg;
UString* USSIPlugIn::timefmt;
UString* USSIPlugIn::docname;
UString* USSIPlugIn::body;
UString* USSIPlugIn::header;
UString* USSIPlugIn::environment;
UString* USSIPlugIn::alternative_include;

const UString* USSIPlugIn::str_expr;
const UString* USSIPlugIn::str_var;
const UString* USSIPlugIn::str_cmd;
const UString* USSIPlugIn::str_cgi;
const UString* USSIPlugIn::str_file;
const UString* USSIPlugIn::str_value;
const UString* USSIPlugIn::str_bytes;
const UString* USSIPlugIn::str_direct;
const UString* USSIPlugIn::str_abbrev;
const UString* USSIPlugIn::str_errmsg;
const UString* USSIPlugIn::str_virtual;
const UString* USSIPlugIn::str_timefmt;
const UString* USSIPlugIn::str_sizefmt;
const UString* USSIPlugIn::str_DATE_GMT;
const UString* USSIPlugIn::str_USER_NAME;
const UString* USSIPlugIn::str_DATE_LOCAL;
const UString* USSIPlugIn::str_DOCUMENT_URI;
const UString* USSIPlugIn::str_DOCUMENT_NAME;
const UString* USSIPlugIn::str_LAST_MODIFIED;
const UString* USSIPlugIn::str_SSI_AUTOMATIC_ALIASING;
const UString* USSIPlugIn::str_errmsg_default;
const UString* USSIPlugIn::str_timefmt_default;
const UString* USSIPlugIn::str_encoding;
const UString* USSIPlugIn::str_encoding_none;
const UString* USSIPlugIn::str_encoding_url;
const UString* USSIPlugIn::str_encoding_entity;
const UString* USSIPlugIn::str_servlet;

void USSIPlugIn::str_allocate()
{
   U_TRACE(0, "USSIPlugIn::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_expr,0)
   U_INTERNAL_ASSERT_EQUALS(str_var,0)
   U_INTERNAL_ASSERT_EQUALS(str_cmd,0)
   U_INTERNAL_ASSERT_EQUALS(str_cgi,0)
   U_INTERNAL_ASSERT_EQUALS(str_file,0)
   U_INTERNAL_ASSERT_EQUALS(str_value,0)
   U_INTERNAL_ASSERT_EQUALS(str_bytes,0)
   U_INTERNAL_ASSERT_EQUALS(str_direct,0)
   U_INTERNAL_ASSERT_EQUALS(str_abbrev,0)
   U_INTERNAL_ASSERT_EQUALS(str_errmsg,0)
   U_INTERNAL_ASSERT_EQUALS(str_virtual,0)
   U_INTERNAL_ASSERT_EQUALS(str_timefmt,0)
   U_INTERNAL_ASSERT_EQUALS(str_sizefmt,0)
   U_INTERNAL_ASSERT_EQUALS(str_DATE_GMT,0)
   U_INTERNAL_ASSERT_EQUALS(str_USER_NAME,0)
   U_INTERNAL_ASSERT_EQUALS(str_DATE_LOCAL,0)
   U_INTERNAL_ASSERT_EQUALS(str_DOCUMENT_URI,0)
   U_INTERNAL_ASSERT_EQUALS(str_DOCUMENT_NAME,0)
   U_INTERNAL_ASSERT_EQUALS(str_LAST_MODIFIED,0)
   U_INTERNAL_ASSERT_EQUALS(str_SSI_AUTOMATIC_ALIASING,0)
   U_INTERNAL_ASSERT_EQUALS(str_errmsg_default,0)
   U_INTERNAL_ASSERT_EQUALS(str_timefmt_default,0)
   U_INTERNAL_ASSERT_EQUALS(str_encoding,0)
   U_INTERNAL_ASSERT_EQUALS(str_encoding_none,0)
   U_INTERNAL_ASSERT_EQUALS(str_encoding_url,0)
   U_INTERNAL_ASSERT_EQUALS(str_encoding_entity,0)
   U_INTERNAL_ASSERT_EQUALS(str_servlet,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("expr") },
      { U_STRINGREP_FROM_CONSTANT("var") },
      { U_STRINGREP_FROM_CONSTANT("cmd") },
      { U_STRINGREP_FROM_CONSTANT("cgi") },
      { U_STRINGREP_FROM_CONSTANT("file") },
      { U_STRINGREP_FROM_CONSTANT("value") },
      { U_STRINGREP_FROM_CONSTANT("bytes") },
      { U_STRINGREP_FROM_CONSTANT("direct") },
      { U_STRINGREP_FROM_CONSTANT("abbrev") },
      { U_STRINGREP_FROM_CONSTANT("errmsg") },
      { U_STRINGREP_FROM_CONSTANT("virtual") },
      { U_STRINGREP_FROM_CONSTANT("timefmt") },
      { U_STRINGREP_FROM_CONSTANT("sizefmt") },
      { U_STRINGREP_FROM_CONSTANT("DATE_GMT") },
      { U_STRINGREP_FROM_CONSTANT("USER_NAME") },
      { U_STRINGREP_FROM_CONSTANT("DATE_LOCAL") },
      { U_STRINGREP_FROM_CONSTANT("DOCUMENT_URI") },
      { U_STRINGREP_FROM_CONSTANT("DOCUMENT_NAME") },
      { U_STRINGREP_FROM_CONSTANT("LAST_MODIFIED") },
      { U_STRINGREP_FROM_CONSTANT("SSI_AUTOMATIC_ALIASING") },
      { U_STRINGREP_FROM_CONSTANT("Error") },
      { U_STRINGREP_FROM_CONSTANT("%A, %d-%b-%Y %H:%M:%S GMT") },
      { U_STRINGREP_FROM_CONSTANT("encoding") },
      { U_STRINGREP_FROM_CONSTANT("none") },
      { U_STRINGREP_FROM_CONSTANT("url") },
      { U_STRINGREP_FROM_CONSTANT("entity") },
      { U_STRINGREP_FROM_CONSTANT("servlet") }
   };

   U_NEW_ULIB_OBJECT(str_expr,                   U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_var,                    U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_cmd,                    U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_cgi,                    U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_file,                   U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_value,                  U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_bytes,                  U_STRING_FROM_STRINGREP_STORAGE(6));
   U_NEW_ULIB_OBJECT(str_direct,                 U_STRING_FROM_STRINGREP_STORAGE(7));
   U_NEW_ULIB_OBJECT(str_abbrev,                 U_STRING_FROM_STRINGREP_STORAGE(8));
   U_NEW_ULIB_OBJECT(str_errmsg,                 U_STRING_FROM_STRINGREP_STORAGE(9));
   U_NEW_ULIB_OBJECT(str_virtual,                U_STRING_FROM_STRINGREP_STORAGE(10));
   U_NEW_ULIB_OBJECT(str_timefmt,                U_STRING_FROM_STRINGREP_STORAGE(11));
   U_NEW_ULIB_OBJECT(str_sizefmt,                U_STRING_FROM_STRINGREP_STORAGE(12));
   U_NEW_ULIB_OBJECT(str_DATE_GMT,               U_STRING_FROM_STRINGREP_STORAGE(13));
   U_NEW_ULIB_OBJECT(str_USER_NAME,              U_STRING_FROM_STRINGREP_STORAGE(14));
   U_NEW_ULIB_OBJECT(str_DATE_LOCAL,             U_STRING_FROM_STRINGREP_STORAGE(15));
   U_NEW_ULIB_OBJECT(str_DOCUMENT_URI,           U_STRING_FROM_STRINGREP_STORAGE(16));
   U_NEW_ULIB_OBJECT(str_DOCUMENT_NAME,          U_STRING_FROM_STRINGREP_STORAGE(17));
   U_NEW_ULIB_OBJECT(str_LAST_MODIFIED,          U_STRING_FROM_STRINGREP_STORAGE(18));
   U_NEW_ULIB_OBJECT(str_SSI_AUTOMATIC_ALIASING, U_STRING_FROM_STRINGREP_STORAGE(19));
   U_NEW_ULIB_OBJECT(str_errmsg_default,         U_STRING_FROM_STRINGREP_STORAGE(20));
   U_NEW_ULIB_OBJECT(str_timefmt_default,        U_STRING_FROM_STRINGREP_STORAGE(21));
   U_NEW_ULIB_OBJECT(str_encoding,               U_STRING_FROM_STRINGREP_STORAGE(22));
   U_NEW_ULIB_OBJECT(str_encoding_none,          U_STRING_FROM_STRINGREP_STORAGE(23));
   U_NEW_ULIB_OBJECT(str_encoding_url,           U_STRING_FROM_STRINGREP_STORAGE(24));
   U_NEW_ULIB_OBJECT(str_encoding_entity,        U_STRING_FROM_STRINGREP_STORAGE(25));
   U_NEW_ULIB_OBJECT(str_servlet,                U_STRING_FROM_STRINGREP_STORAGE(26));
}

USSIPlugIn::USSIPlugIn()
{
   U_TRACE_REGISTER_OBJECT(0, USSIPlugIn, "")

   errmsg  = U_NEW(UString);
   timefmt = U_NEW(UString);
   docname = U_NEW(UString);

   if (str_SSI_AUTOMATIC_ALIASING == 0) str_allocate();
}

USSIPlugIn::~USSIPlugIn()
{
   U_TRACE_UNREGISTER_OBJECT(0, USSIPlugIn)

   delete errmsg;
   delete timefmt;
   delete docname;

   if (body)
      {
      delete body;  
      delete header;
      delete alternative_include;
      }

   if (environment) delete environment;
}

void USSIPlugIn::setAlternativeRedirect(const char* fmt, ...)
{
   U_TRACE(0, "USSIPlugIn::setAlternativeRedirect(%S)", fmt)

   UString format(U_CAPACITY), buffer(U_CAPACITY);

   format.snprintf(U_CTYPE_HTML "\r\nRefresh: 0; url=%s\r\n", fmt);

   va_list argp;
   va_start(argp, fmt);

   buffer.vsnprintf(format.data(), argp);

   va_end(argp);

   U_http_is_connection_close = U_YES;
   u_http_info.nResponseCode  = HTTP_OK;

   UHTTP::setHTTPResponse(&buffer, 0);

   alternative_response = 1;
}

void USSIPlugIn::setAlternativeResponse(const UString& _body)
{
   U_TRACE(0, "USSIPlugIn::setAlternativeResponse(%.*S)", U_STRING_TO_TRACE(_body))

   alternative_response = 1;

   U_http_is_connection_close = U_YES;

   if (_body.empty())
      {
      u_http_info.nResponseCode = HTTP_NO_CONTENT;

      UHTTP::setHTTPResponse(0, 0);
      }
   else
      {
      u_http_info.nResponseCode = HTTP_OK;

      UHTTP::setHTTPResponse(UHTTP::str_ctype_html, &_body);
      }
}

void USSIPlugIn::setAlternativeInclude(const UString& tmpl, const char* title_txt,  const char* head_html, const char* body_style, ...)
{
   U_TRACE(0, "USSIPlugIn::setAlternativeInclude(%.*S,%S,%S,%S)", U_STRING_TO_TRACE(tmpl), title_txt, head_html, body_style)

   alternative_include->setBuffer(tmpl.size() + 256 * 1024);

   va_list argp;
   va_start(argp, body_style);

   alternative_include->vsnprintf(tmpl.data(), argp);

   va_end(argp);

   u_http_info.nResponseCode = HTTP_NO_CONTENT;

   UString buffer(U_CAPACITY);

   U_INTERNAL_ASSERT_POINTER(title_txt)

                   buffer.snprintf(    "'TITLE_TXT=%s'\n", title_txt);
   if (head_html)  buffer.snprintf_add("'HEAD_HTML=%s'\n", head_html);
   if (body_style) buffer.snprintf_add("BODY_STYLE=%s\n",  body_style);

   (void) UClientImage_Base::environment->append(buffer);
}

void USSIPlugIn::setMessagePage(const UString& tmpl, const char* title_txt, const char* message)
{
   U_TRACE(0, "USSIPlugIn::setMessagePage(%.*S,%S,%S)", U_STRING_TO_TRACE(tmpl), title_txt, message)

   setAlternativeInclude(tmpl, title_txt, 0, 0, title_txt, message);
}

void USSIPlugIn::setMessagePageWithVar(const UString& tmpl, const char* title_txt, const char* fmt, ...)
{
   U_TRACE(0, "USSIPlugIn::setMessagePageWithVar(%.*S,%S,%S)", U_STRING_TO_TRACE(tmpl), title_txt, fmt)

   UString buffer(U_CAPACITY);

   va_list argp;
   va_start(argp, fmt);

   buffer.vsnprintf(fmt, argp);

   va_end(argp);

   setMessagePage(tmpl, title_txt, buffer.data());
}

U_NO_EXPORT UString USSIPlugIn::getPathname(const UString& name, const UString& value, const UString& directory)
{
   U_TRACE(0, "USSIPlugIn::getPathname(%.*S,%.*S,%.*S)", U_STRING_TO_TRACE(name), U_STRING_TO_TRACE(value), U_STRING_TO_TRACE(directory))

   /* "include file" looks in the current directory (the name must not start with /, ., or ..) and "include virtual" starts
    * in the root directory of your kiosk (so the name must start with "/".) You might want to make a "/includes" directory
    * in your Kiosk and then you can say "include virtual=/includes/file.txt" from any page. The "virtual" and "file"
    * parameters are also used with "fsize" and "flastmod". With either method, you can only reference files that are within
    * your Kiosk directory (apart if "direct"...)
    */

   UString pathname;

        if (name == *str_direct)  pathname =                                         value;
   else if (name == *str_file)    pathname = directory                       + '/' + value;
   else if (name == *str_virtual) pathname = UServer_Base::getDocumentRoot() +       value;

   U_RETURN_STRING(pathname);
}

U_NO_EXPORT UString USSIPlugIn::getInclude(const UString& include, int include_level)
{
   U_TRACE(0, "USSIPlugIn::getInclude(%.*S,%d)", U_STRING_TO_TRACE(include), include_level)

   UString content = include;

   U_INTERNAL_DUMP("file = %.*S", U_FILE_TO_TRACE(*UHTTP::file))

   if (u_endsWith(U_FILE_TO_PARAM(*UHTTP::file), U_CONSTANT_TO_PARAM(".shtml")))
      {
      if (include_level < 16) content = processSSIRequest(content, include_level + 1);
      else
         {
         U_SRV_LOG("SSI #include level is too deep (%.*S)", U_FILE_TO_TRACE(*UHTTP::file));
         }
      }

   U_RETURN_STRING(content);
}

U_NO_EXPORT bool USSIPlugIn::callService(const UString& name, const UString& value)
{
   U_TRACE(0, "USSIPlugIn::callService(%.*S,%.*S)", U_STRING_TO_TRACE(name), U_STRING_TO_TRACE(value))

   if (name == *str_cmd)
      {
      static int fd_stderr = UServices::getDevNull("/tmp/ssi.err");

      *UClientImage_Base::wbuffer = UCommand::outputCommand(value, 0, -1, fd_stderr);
      }
   else
      {
      U_ASSERT(name == *str_cgi ||
               name == *str_servlet)

      if (UHTTP::isHttpPOST() &&
          name == *str_cgi)
         {
         *UClientImage_Base::body = U_HTTP_BODY(*UClientImage_Base::request);
         }

      if (UHTTP::callService(value) == false ||
          alternative_response)
         {
         // 1 => response already complete (nothing to do)

         alternative_response = 1;

         U_RETURN(false);
         }
      }

   U_RETURN(true);
}

U_NO_EXPORT UString USSIPlugIn::processSSIRequest(const UString& content, int include_level)
{
   U_TRACE(0, "USSIPlugIn::processSSIRequest(%.*S,%d)", U_STRING_TO_TRACE(content), include_level)

   U_INTERNAL_ASSERT(content)

   UString tmp; // NB: must be here to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...
   int32_t i, n;
   const char* directive;
   UVector<UString> name_value;
   enum {E_NONE, E_URL, E_ENTITY} encode;
   int op, if_level = 0, if_is_false_level = 0;
   uint32_t distance, pos, size, sz = content.size();
   bool bgroup, bfile, bvar, if_is_false = false, if_is_false_endif = false;
   UString token, name, value, pathname, include, directory, output(U_CAPACITY), x, encoded;

   (directory = UHTTP::getDirectoryURI()).duplicate();

   UTokenizer t(content);
   t.setGroup(U_CONSTANT_TO_PARAM("<!--->"));

   enum { SSI_NONE, SSI_INCLUDE, SSI_EXEC, SSI_ECHO, SSI_CONFIG, SSI_FLASTMOD,
          SSI_FSIZE, SSI_PRINTENV, SSI_SET, SSI_IF, SSI_ELSE, SSI_ELIF, SSI_ENDIF };

   do {
      // Find next SSI tag

      distance = t.getDistance();
      pos      = content.find("<!--#", distance);

      if (pos)
         {
         if (pos == U_NOT_FOUND) pos = sz;

         t.setDistance(pos);

         size = pos - distance;

         if (size &&
             if_is_false == false)
            {
            (void) output.append(content.substr(distance, size)); // plain html block
            }
         }

      if (t.next(token, &bgroup) == false) break;

      U_INTERNAL_ASSERT(bgroup)

      U_INTERNAL_DUMP("token = %.*S", U_STRING_TO_TRACE(token))

      U_ASSERT(token.size() >= 2) 

      directive = token.c_pointer((i = 2)); // "-#"...

      U_INTERNAL_DUMP("directive = %.*S", 10, directive)

      /**
       * <!--#element attribute=value attribute=value ... -->
       *
       * echo         DONE
       *   var        DONE
       *   encoding   DONE
       * set          DONE
       *   var        DONE
       *   value      DONE
       * exec         DONE
       *   cmd        DONE
       *   cgi        DONE
       *   servlet    DONE
       * include      DONE
       *   file       DONE
       *   direct     DONE
       *   virtual    DONE
       * fsize        DONE
       *   file       DONE
       *   direct     DONE
       *   virtual    DONE
       * flastmod     DONE
       *   file       DONE
       *   direct     DONE
       *   virtual    DONE
       * config       DONE
       *   errmsg     DONE
       *   sizefmt    DONE
       *   timefmt    DONE
       * printenv     DONE
       *
       * if           DONE
       * elif         DONE
       * else         DONE
       * endif        DONE
       *
       * expressions
       * &&, ||       DONE
       * comp         DONE
       * ${...}       DONE
       * $...         DONE
       * '...'        DONE
       * ( ... )      DONE
       */

      // Check element

      op = SSI_NONE;

      if (U_STRNEQ(directive, "include "))
         {
         op = SSI_INCLUDE;
         i += U_CONSTANT_SIZE("include ");
         }
      else if (U_STRNEQ(directive, "if "))
         {
         op = SSI_IF;
         i += U_CONSTANT_SIZE("if ");
         }
      else if (U_STRNEQ(directive, "else "))
         {
         op = SSI_ELSE;
         i += U_CONSTANT_SIZE("else ");
         }
      else if (U_STRNEQ(directive, "elif "))
         {
         op = SSI_ELIF;
         i += U_CONSTANT_SIZE("elif ");
         }
      else if (U_STRNEQ(directive, "endif "))
         {
         op = SSI_ENDIF;
         i += U_CONSTANT_SIZE("endif ");
         }
      else if (U_STRNEQ(directive, "exec "))
         {
         op = SSI_EXEC;
         i += U_CONSTANT_SIZE("exec ");
         }
      else if (U_STRNEQ(directive, "echo "))
         {
         op = SSI_ECHO;
         i += U_CONSTANT_SIZE("echo ");
         }
      else if (U_STRNEQ(directive, "config "))
         {
         op = SSI_CONFIG;
         i += U_CONSTANT_SIZE("config ");
         }
      else if (U_STRNEQ(directive, "flastmod "))
         {
         op = SSI_FLASTMOD;
         i += U_CONSTANT_SIZE("flastmod ");
         }
      else if (U_STRNEQ(directive, "fsize "))
         {
         op = SSI_FSIZE;
         i += U_CONSTANT_SIZE("fsize ");
         }
      else if (U_STRNEQ(directive, "printenv "))
         {
         op = SSI_PRINTENV;
         i += U_CONSTANT_SIZE("printenv ");
         }
      else if (U_STRNEQ(directive, "set "))
         {
         op = SSI_SET;
         i += U_CONSTANT_SIZE("set ");
         }

      U_INTERNAL_DUMP("op = %d", op)

      n = token.size() - i;

      if (n)
         {
#     ifdef DEBUG // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...
          name.clear();
         value.clear();
#     endif

         name_value.clear();

         tmp = UStringExt::simplifyWhiteSpace(token.substr(i, n));

         n = UStringExt::getNameValueFromData(tmp, name_value, U_CONSTANT_TO_PARAM(" "));
         }

#  ifdef DEBUG // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...
      token.clear();
#  endif

      U_INTERNAL_DUMP("if_is_false = %b if_is_false_level = %d if_level = %d if_is_false_endif = %b",
                       if_is_false,     if_is_false_level,     if_level,     if_is_false_endif)

      switch (op)
         {
         /*
         <!--#if expr="${Sec_Nav}" -->
            <!--#include virtual="secondary_nav.txt" -->
         <!--#elif expr="${Pri_Nav}" -->
            <!--#include virtual="primary_nav.txt" -->
         <!--#endif -->
         */

         case SSI_IF:
         case SSI_ELIF:
            {
            if (n != 2) U_ERROR("SSI: syntax error for %S statement", op == SSI_IF ? "if" : "elif");

            name = name_value[0];

            if (name != *str_expr) U_ERROR("SSI: unknow attribute %S for %S statement", U_STRING_TO_TRACE(name), op == SSI_IF ? "if" : "elif");

            value = name_value[1];

            if (op == SSI_IF)
               {
               if ( if_is_false       == false &&
                   (if_is_false_level == 0 || (if_level < if_is_false_level)))
                  {
                  if_is_false = (UStringExt::evalExpression(value, *UClientImage_Base::environment).empty()
                                             ? (if_is_false_level = if_level, true)
                                             : false);
                  }
               }
            else
               {
               --if_level;

               if (if_level == if_is_false_level)
                  {
                  if (if_is_false &&
                      if_is_false_endif == false)
                     {
                     if_is_false = (UStringExt::evalExpression(value, *UClientImage_Base::environment).empty()
                                             ? (if_is_false_level = if_level, true)
                                             : false);
                     }
                  else
                     {
                     if_is_false_level = if_level;
                     if_is_false = if_is_false_endif = true;
                     }
                  }
               }

            ++if_level;
            }
         break;

         case SSI_ELSE:
            {
            --if_level;

            if (if_is_false)
               {
               if (if_level          == if_is_false_level &&
                   if_is_false_endif == false)
                  {
                  if_is_false = false;
                  }
               }
            else
               {
               if_is_false       = true;
               if_is_false_level = if_level;
               }

            ++if_level;
            }
         break;

         case SSI_ENDIF:
            {
            --if_level;

            if (if_level == if_is_false_level) if_is_false = if_is_false_endif = false;
            }
         break;
         }

      U_INTERNAL_DUMP("if_is_false = %b if_is_false_level = %d if_level = %d if_is_false_endif = %b",
                       if_is_false,     if_is_false_level,     if_level,     if_is_false_endif)

      if (if_is_false) continue;

      switch (op)
         {
         /*
         <!--#printenv -->
         */
         case SSI_PRINTENV: (void) output.append(U_STRING_TO_PARAM(*UClientImage_Base::environment)); break;

         /*
         <!--#config sizefmt="bytes" -->
         <!--#config timefmt="%y %m %d" -->
         <!--#config errmsg="SSI command failed!" -->
         */
         case SSI_CONFIG:
            {
            for (i = 0; i < n; i += 2)
               {
               name  = name_value[i];
               value = name_value[i+1];

                    if (name == *str_errmsg)  (*errmsg  = value).duplicate();
               else if (name == *str_timefmt) (*timefmt = value).duplicate();
               else if (name == *str_sizefmt) use_size_abbrev = (value == *str_abbrev);
               }
            }
         break;

         /*
         <!--#echo var=$BODY_STYLE -->
         <!--#echo [encoding="..."] var="..." [encoding="..."] var="..." ... -->
         */
         case SSI_ECHO:
            {
            encode = E_NONE;

            for (i = 0; i < n; i += 2)
               {
               name  = name_value[i];
               value = name_value[i+1];

               if (name == *str_encoding)
                  {
                       if (value == *str_encoding_none)   encode = E_NONE;
                  else if (value == *str_encoding_url)    encode = E_URL;
                  else if (value == *str_encoding_entity) encode = E_ENTITY;
                  }
               else if (name == *str_var)
                  {
                  U_INTERNAL_ASSERT_MAJOR(u_user_name_len,0)

                  /**
                   * DATE_GMT       The current date in Greenwich Mean Time.
                   * DATE_LOCAL     The current date in the local time zone.
                   * LAST_MODIFIED  The last modification date of the document requested by the user.
                   * USER_NAME      Contains the owner of the file which included it.
                   * DOCUMENT_NAME  The filename (excluding directories) of the document requested by the user.
                   * DOCUMENT_URI   The URL path of the document requested by the user. Note that in the case of
                   *                nested include files, this is not then URL for the current document.
                   */

                       if (value == *str_DATE_GMT)      x = UTimeDate::strftime(timefmt->data(), u_now->tv_sec);
                  else if (value == *str_DATE_LOCAL)    x = UTimeDate::strftime(timefmt->data(), u_now->tv_sec, true);
                  else if (value == *str_LAST_MODIFIED) x = UTimeDate::strftime(timefmt->data(), last_modified);
                  else if (value == *str_USER_NAME)     (void) x.assign(u_user_name, u_user_name_len);
                  else if (value == *str_DOCUMENT_URI)  (void) x.assign(U_HTTP_URI_TO_PARAM);
                  else if (value == *str_DOCUMENT_NAME) x = *docname;
                  else
                     {
                     x = UStringExt::expandEnvironmentVar(value, UClientImage_Base::environment);
                     }

                  if (x.empty()) continue;

                  switch (encode)
                     {
                     case E_NONE:
                        {
                        encoded = x;
                        }
                     break;

                     case E_URL:
                        {
                        encoded.setBuffer(x.size() * 3);

                        Url::encode(x, encoded);
                        }
                     break;

                     case E_ENTITY:
                        {
                        encoded.setBuffer(x.size());

                        UXMLEscape::encode(x, encoded);
                        }
                     break;
                     }

                  (void) output.append(encoded);
                  }

               x.clear();
               }
            }
         break;

         /*
         <!--#fsize file="script.pl" -->
         <!--#flastmod virtual="index.html" -->
         */
         case SSI_FSIZE:
         case SSI_FLASTMOD:
            {
            if (n == 2)
               {
               name     = name_value[0];
               value    = name_value[1];
               pathname = getPathname(name, value, directory);
               }

            bfile = false;

            if (pathname.empty() == false)
               {
               UHTTP::file->setPath(pathname, UClientImage_Base::environment);

                    if (UHTTP::isFileInCache()) bfile = true;
               else if (UHTTP::file->open())
                  {
                  bfile = true;

                  UHTTP::file->fstat();
                  }

               pathname.clear();
               }

                 if (bfile == false)     (void) output.append(*errmsg);
            else if (op    == SSI_FSIZE) (void) output.append(UStringExt::numberToString(UHTTP::file->getSize(), use_size_abbrev));
            else                         (void) output.append(UTimeDate::strftime(timefmt->data(), UHTTP::file->st_mtime)); // SSI_FLASTMOD
            }
         break;

         /*
         <!--#include file=footer.html -->
         */
         case SSI_INCLUDE:
            {
            if (n == 2)
               {
               name     = name_value[0];
               value    = name_value[1];
               pathname = getPathname(name, value, directory);
               }

            if (pathname.empty() == false)
               {
               if (alternative_include->empty() == false &&
                   pathname.equal(U_CONSTANT_TO_PARAM("$FILE_BODY_SHTML")))
                  {
                  (void) output.append(*alternative_include);
                                        alternative_include->clear();

                  break;
                  }

               UHTTP::file->setPath(pathname, UClientImage_Base::environment);

               if (UHTTP::isFileInCache() &&
                   UHTTP::isDataFromCache())
                  {
                  include = UHTTP::getDataFromCache(false, false);
                  }
               else if (UHTTP::file->open())
                  {
                  UHTTP::file->fstat();

                  include = UHTTP::file->getContent();
                  }
               }

            if (include.empty()) x = *errmsg;
            else
               {
               x = getInclude(include, include_level);

               include.clear();
               }

            (void) output.append(x);

                                 x.clear();
                          pathname.clear();
            UHTTP::file->getPath().clear();
            }
         break;

         /*
         <!--#exec cmd="ls -l" -->
         <!--#exec cgi=/cgi-bin/foo.cgi -->
         <!--#exec servlet=/servlet/mchat -->
         */
         case SSI_EXEC:
            {
            if (n == 2)
               {
               if (callService(name_value[0], name_value[1]) == false) U_RETURN_STRING(UString::getStringNull());

               (void) output.append(*UClientImage_Base::wbuffer);
               }
            }
         break;

         /*
         <!--#set cmd="env -l" -->
         <!--#set var="foo" value="bar" -->
         <!--#set cgi=$VIRTUAL_HOST/cgi-bin/main.bash -->
         */
         case SSI_SET:
            {
            bvar = false;

            for (i = 0; i < n; i += 4)
               {
               if (name_value[i]   == *str_var &&
                   name_value[i+2] == *str_value)
                  {
                  bvar = true;

                  (void) UClientImage_Base::environment->append(name_value[i+1]);
                  (void) UClientImage_Base::environment->append(1U, '=');
                  (void) UClientImage_Base::environment->append(UStringExt::expandEnvironmentVar(name_value[i+3], UClientImage_Base::environment));
                  (void) UClientImage_Base::environment->append(1U, '\n');
                  }
               }

            if (bvar == false)
               {
               name  = name_value[0];
               value = name_value[1];

               if (callService(name, value) == false) U_RETURN_STRING(UString::getStringNull());

               if (name != *str_servlet)
                  {
                  // NB: check if there is an alternative response to elaborate...

                  UString rfile = UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("FILE_RESPONSE_HTML"), UClientImage_Base::wbuffer);

                  U_INTERNAL_DUMP("FILE_RESPONSE_HTML(%u) = %.*S", rfile.size(), U_STRING_TO_TRACE(rfile))

                  if (rfile.empty() == false)
                     {
                     // 2 => body response in var 'header' (take from from file of name: var environment 'FILE_RESPONSE_HTML')

                     alternative_response = 2;

                     *header = UFile::contentOf(rfile);

                     // NB: with an alternative response we cannot use the cached header response...

                     U_RETURN_STRING(UString::getStringNull());
                     }

                  UString rheader = UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("HTTP_RESPONSE_HEADER"), UClientImage_Base::wbuffer);

                  U_INTERNAL_DUMP("response_header(%u) = %.*S", rheader.size(), U_STRING_TO_TRACE(rheader))

                  if (rheader.empty() == false)
                     {
                     // 3 => var environment 'HTTP_RESPONSE_HEADER' inserted in header response (var 'header')

                     alternative_response = 3;

                     uint32_t hsz = rheader.size();

                     UString _tmp(hsz);

                     (void) UEscape::decode(rheader, _tmp);

                     // NB: we cannot use directly the body attribute because is bounded at the param 'content'...

                     UString rbody = UStringExt::getEnvironmentVar(U_CONSTANT_TO_PARAM("HTTP_RESPONSE_BODY"), UClientImage_Base::wbuffer);

                     U_INTERNAL_DUMP("response_body(%u) = %.*S", rbody.size(), U_STRING_TO_TRACE(rbody))

                     if (rbody.empty()) (void) header->insert(0, _tmp);
                     else
                        {
                        // 4 => var environment 'HTTP_RESPONSE_HEADER' set as header response (var 'header') and
                        //      var environment 'HTTP_RESPONSE_BODY'   set as body   response (var 'body')

                        alternative_response = 4;

                        *body = rbody;

                        // NB: with an alternative body response we cannot use the cached header response...

                        (void) header->replace(_tmp);

                        U_RETURN_STRING(UString::getStringNull());
                        }

                     UClientImage_Base::wbuffer->erase(0, U_CONSTANT_SIZE("HTTP_RESPONSE_HEADER=") + hsz + 3);
                     }

                  (void) UClientImage_Base::environment->append(*UClientImage_Base::wbuffer);
                  }
               }
            }
         break;
         }
      }
   while (t.atEnd() == false);

   U_INTERNAL_DUMP("if_is_false = %b if_is_false_level = %d if_level = %d if_is_false_endif = %b",
                    if_is_false,     if_is_false_level,     if_level,     if_is_false_endif)

   if (if_is_false || if_is_false_endif || if_level || if_is_false_level) U_ERROR("SSI: syntax error for conditional statement");

   U_RETURN_STRING(output);
}

// Server-wide hooks

int USSIPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "USSIPlugIn::handlerConfig(%p)", &cfg)

   // --------------------------------------------------------------------------------------------------------------
   // ENVIRONMENT             path of file configuration environment for SSI
   //
   // SSI_AUTOMATIC_ALIASING  special SSI HTML file that is recognized automatically as alias of all uri request
   //                         without suffix (generally cause navigation directory not working)
   // --------------------------------------------------------------------------------------------------------------

   if (cfg.loadTable())
      {
      UString x = cfg[*str_SSI_AUTOMATIC_ALIASING];

      U_INTERNAL_ASSERT_EQUALS(UHTTP::global_alias, 0)

      if (x.empty() == false)
         {
         if (x.first_char() != '/') (void) x.insert(0, '/');

         UHTTP::global_alias = U_NEW(UString(x));
         }

      x = cfg[*UServer_Base::str_ENVIRONMENT];

      if (x.empty() == false)
         {
         environment = U_NEW(UString(UStringExt::prepareForEnvironmentVar(UFile::contentOf(x))));

         const char* home = U_SYSCALL(getenv, "%S", "HOME");

         if (home) environment->snprintf_add("HOME=%s\n", home);

         U_ASSERT_EQUALS(environment->isBinary(), false)
         }
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

int USSIPlugIn::handlerInit()
{
   U_TRACE(0, "USSIPlugIn::handlerInit()")

   U_INTERNAL_ASSERT_EQUALS(body,0)
   U_INTERNAL_ASSERT_EQUALS(header,0)

   body                = U_NEW(UString);
   header              = U_NEW(UString);
   alternative_include = U_NEW(UString);

   U_SRV_LOG("initialization of plugin success");

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int USSIPlugIn::handlerRequest()
{
   U_TRACE(0, "USSIPlugIn::handlerRequest()")

   U_INTERNAL_DUMP("method = %.*S uri = %.*S", U_HTTP_METHOD_TO_TRACE, U_HTTP_URI_TO_TRACE)

   bool bcache = u_is_ssi();

   U_DUMP("bcache = %b", bcache)

   if (bcache                           ||
       (UHTTP::file->isSuffix(".shtml") &&
        UHTTP::isHTTPRequestNotFound() == false))
      {
      // init

      *UClientImage_Base::environment = UHTTP::getCGIEnvironment();

      if (environment) (void) UClientImage_Base::environment->append(*environment);

      if (UClientImage_Base::environment->empty())
         {
         UHTTP::setHTTPBadRequest();

         U_RETURN(U_PLUGIN_HANDLER_ERROR);
         }

      *errmsg              = *str_errmsg_default;
      *timefmt             = *str_timefmt_default;
      last_modified        = UHTTP::file->st_mtime;
      use_size_abbrev      = true;
      alternative_response = 0;

#  ifdef U_HTTP_CACHE_REQUEST
      U_http_no_cache = true;
#  endif

      header->setBuffer(U_CAPACITY);

      (*docname = UHTTP::getDocumentName()).duplicate();

      // read the SSI file

      if (bcache == false) *body = UHTTP::file->getContent();
      else
         {
         U_INTERNAL_DUMP("UClientImage_Base::body(%u) = %.*S", UClientImage_Base::body->size(), U_STRING_TO_TRACE(*UClientImage_Base::body))

         U_ASSERT(UHTTP::isDataFromCache())
         U_INTERNAL_ASSERT_POINTER(UHTTP::file_data->array)
         U_INTERNAL_ASSERT_EQUALS(UHTTP::file_data->array->size(),2)

         (void) header->append(UHTTP::getDataFromCache(true, false)); // NB: after now 'file_data' can change...

         *body = (UHTTP::isHttpGETorHEAD() && UClientImage_Base::body->empty() == false
                                           ? *UClientImage_Base::body
                                           : UHTTP::getDataFromCache(false, false));
         }

      // process the SSI file

      UString output = processSSIRequest(*body, 0);

      U_INTERNAL_DUMP("alternative_response = %d output(%u) = %.*S", alternative_response, output.size(), U_STRING_TO_TRACE(output))

      if (alternative_response == 0)
         {
         U_INTERNAL_ASSERT(output)

#     ifdef USE_PAGE_SPEED
         UHTTP::page_speed->minify_html("USSIPlugIn::handlerRequest()", output);
#     endif

         U_INTERNAL_DUMP("U_http_is_accept_gzip = %C", U_http_is_accept_gzip)

         if (U_http_is_accept_gzip)
            {
            output = UStringExt::deflate(output, true);

            (void) header->insert(0, U_CONSTANT_TO_PARAM("Content-Encoding: gzip\r\n"));
            }

         if (bcache)
            {
            // NB: adjusting the size of response...

            (void) UHTTP::checkHTTPContentLength(*header, output.size());
            }
         else
            { u_mime_index = U_ssi;

            (void) header->append(UHTTP::getHeaderMimeType(0, U_CTYPE_HTML, output.size(), 0));
            }

         u_http_info.nResponseCode = HTTP_OK;

         *UClientImage_Base::wbuffer = UHTTP::getHTTPHeaderForResponse(*header, false);
         *UClientImage_Base::body    = output;
         }
      else if (alternative_response > 1)
         {
         // -----------------------------------------------------------------------------------------------------
         // 1 => response already complete (nothing to do)
         // ------------------------------------------------------------------------------------------------------
         // 2 => body response in var 'header' (take from from file of name: var environment 'FILE_RESPONSE_HTML')
         // ------------------------------------------------------------------------------------------------------
         // 3 => var environment 'HTTP_RESPONSE_HEADER' inserted in header response (var 'header')
         // ------------------------------------------------------------------------------------------------------
         // 4 => var environment 'HTTP_RESPONSE_HEADER'      set as header response (var 'header') and
         //      var environment 'HTTP_RESPONSE_BODY'        set as body   response (var 'body')
         // ------------------------------------------------------------------------------------------------------

         U_INTERNAL_DUMP("header(%u) = %.*S", header->size(), U_STRING_TO_TRACE(*header))
         U_INTERNAL_DUMP("  body(%u) = %.*S",   body->size(), U_STRING_TO_TRACE(*body))

         (void) UClientImage_Base::wbuffer->replace(*header);

         if (alternative_response != 2)
            {
            (void) UClientImage_Base::wbuffer->append(U_CONSTANT_TO_PARAM(U_CRLF));
            (void) UClientImage_Base::wbuffer->append(alternative_response == 3 ? output : *body);
            }

         (void) UHTTP::processCGIOutput();
         }

      UHTTP::setHTTPRequestProcessed();
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* USSIPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "last_modified                " << last_modified               << '\n'
                  << "use_size_abbrev              " << use_size_abbrev             << '\n'
                  << "alternative_response         " << alternative_response        << '\n'
                  << "body                (UString " << (void*)body                 << ")\n"
                  << "errmsg              (UString " << (void*)errmsg               << ")\n"
                  << "header              (UString " << (void*)header               << ")\n"
                  << "timefmt             (UString " << (void*)timefmt              << ")\n"
                  << "docname             (UString " << (void*)docname              << ")\n"
                  << "environment         (UString " << (void*)environment          << ")\n"
                  << "alternative_include (UString " << (void*)alternative_include  << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
