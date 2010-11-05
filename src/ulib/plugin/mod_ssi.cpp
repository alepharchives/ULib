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

#include <ulib/command.h>
#include <ulib/tokenizer.h>
#include <ulib/file_config.h>
#include <ulib/utility/uhttp.h>
#include <ulib/plugin/mod_ssi.h>
#include <ulib/net/server/server.h>
#include <ulib/utility/string_ext.h>

// Server Side Include (SSI) commands are executed by the server as it parses your HTML file.
// Server side includes can be used to include the value of various server environment variables
// within your HTML such as the local date and time. One might use a server side include to add
// a signature file to an HTML file or company logo. 

U_CREAT_FUNC(mod_ssi, USSIPlugIn)

const UString* USSIPlugIn::str_var;
const UString* USSIPlugIn::str_cmd;
const UString* USSIPlugIn::str_cgi;
const UString* USSIPlugIn::str_file;
const UString* USSIPlugIn::str_value;
const UString* USSIPlugIn::str_bytes;
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
const UString* USSIPlugIn::str_SSI_EXT_MASK;
const UString* USSIPlugIn::str_errmsg_default;
const UString* USSIPlugIn::str_timefmt_default;

void USSIPlugIn::str_allocate()
{
   U_TRACE(0, "USSIPlugIn::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_var,0)
   U_INTERNAL_ASSERT_EQUALS(str_cmd,0)
   U_INTERNAL_ASSERT_EQUALS(str_cgi,0)
   U_INTERNAL_ASSERT_EQUALS(str_file,0)
   U_INTERNAL_ASSERT_EQUALS(str_value,0)
   U_INTERNAL_ASSERT_EQUALS(str_bytes,0)
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
   U_INTERNAL_ASSERT_EQUALS(str_SSI_EXT_MASK,0)
   U_INTERNAL_ASSERT_EQUALS(str_errmsg_default,0)
   U_INTERNAL_ASSERT_EQUALS(str_timefmt_default,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("var") },
      { U_STRINGREP_FROM_CONSTANT("cmd") },
      { U_STRINGREP_FROM_CONSTANT("cgi") },
      { U_STRINGREP_FROM_CONSTANT("file") },
      { U_STRINGREP_FROM_CONSTANT("value") },
      { U_STRINGREP_FROM_CONSTANT("bytes") },
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
      { U_STRINGREP_FROM_CONSTANT("SSI_EXT_MASK") },
      { U_STRINGREP_FROM_CONSTANT("Error") },
      { U_STRINGREP_FROM_CONSTANT("%A, %d-%b-%Y %H:%M:%S GMT") }
   };

   U_NEW_ULIB_OBJECT(str_var,             U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_cmd,             U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_cgi,             U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_file,            U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_value,           U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_bytes,           U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_abbrev,          U_STRING_FROM_STRINGREP_STORAGE(6));
   U_NEW_ULIB_OBJECT(str_errmsg,          U_STRING_FROM_STRINGREP_STORAGE(7));
   U_NEW_ULIB_OBJECT(str_virtual,         U_STRING_FROM_STRINGREP_STORAGE(8));
   U_NEW_ULIB_OBJECT(str_timefmt,         U_STRING_FROM_STRINGREP_STORAGE(9));
   U_NEW_ULIB_OBJECT(str_sizefmt,         U_STRING_FROM_STRINGREP_STORAGE(10));
   U_NEW_ULIB_OBJECT(str_DATE_GMT,        U_STRING_FROM_STRINGREP_STORAGE(11));
   U_NEW_ULIB_OBJECT(str_USER_NAME,       U_STRING_FROM_STRINGREP_STORAGE(12));
   U_NEW_ULIB_OBJECT(str_DATE_LOCAL,      U_STRING_FROM_STRINGREP_STORAGE(13));
   U_NEW_ULIB_OBJECT(str_DOCUMENT_URI,    U_STRING_FROM_STRINGREP_STORAGE(14));
   U_NEW_ULIB_OBJECT(str_DOCUMENT_NAME,   U_STRING_FROM_STRINGREP_STORAGE(15));
   U_NEW_ULIB_OBJECT(str_LAST_MODIFIED,   U_STRING_FROM_STRINGREP_STORAGE(16));
   U_NEW_ULIB_OBJECT(str_SSI_EXT_MASK,    U_STRING_FROM_STRINGREP_STORAGE(17));
   U_NEW_ULIB_OBJECT(str_errmsg_default,  U_STRING_FROM_STRINGREP_STORAGE(18));
   U_NEW_ULIB_OBJECT(str_timefmt_default, U_STRING_FROM_STRINGREP_STORAGE(19));
}

U_NO_EXPORT UString USSIPlugIn::getInclude(const UString& include, int include_level)
{
   U_TRACE(0, "USSIPlugIn::getInclude(%.*S,%d)", U_STRING_TO_TRACE(include), include_level)

   UString content = include;

   const char* suffix = UHTTP::file->getSuffix();

   if (u_dosmatch_with_OR(suffix, UHTTP::file->getSuffixLen(suffix), U_STRING_TO_PARAM(ssi_ext_mask), 0))
      {
      if (include_level < 16) content = processSSIRequest(content, include_level + 1);
      else
         {
         U_SRV_LOG("SSI #include level is too deep (%.*S)", U_FILE_TO_TRACE(*UHTTP::file));
         }
      }

   U_RETURN_STRING(content);
}

U_NO_EXPORT UString USSIPlugIn::processSSIRequest(const UString& content, int include_level)
{
   U_TRACE(0, "USSIPlugIn::processSSIRequest(%.*S,%d)", U_STRING_TO_TRACE(content), include_level)

   int op;
   UString tmp; // NB: must be here to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...
   int32_t i, n;
   bool bgroup, bfile;
   const char* directive;
   UVector<UString> name_value;
   uint32_t distance, pos, size;
   UString token, name, value, pathname, include, directory, output(U_CAPACITY);

   (directory = UHTTP::getDirectoryURI()).duplicate();

   UTokenizer t(content);
   t.setGroup(U_CONSTANT_TO_PARAM("<!--->"));

   enum { SSI_NONE, SSI_INCLUDE, SSI_EXEC, SSI_ECHO, SSI_CONFIG, SSI_FLASTMOD, SSI_FSIZE, SSI_PRINTENV, SSI_SET };

   while (true)
      {
      // Find next SSI tag

      distance = t.getDistance();
      pos      = content.find("<!--#", distance);

      if (pos)
         {
         if (pos != U_NOT_FOUND) t.setDistance(pos);
         else
            {
            pos = content.size();

            t.setDistance(pos);
            }

         size = pos - distance;

         if (size) (void) output.append(content.substr(distance, size)); // plain html block
         }

      if (t.next(token, &bgroup) == false) break;

      U_INTERNAL_ASSERT(bgroup)

      U_INTERNAL_DUMP("token = %.*S", U_STRING_TO_TRACE(token))

      directive = token.c_pointer((i = 2)); // "-#"...

      U_INTERNAL_DUMP("directive = %10s", directive)

      // Check element

      op = SSI_NONE;

      if (U_STRNEQ(directive, "include "))
         {
         op = SSI_INCLUDE;
         i += U_CONSTANT_SIZE("include ");
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

      switch (op)
         {
         case SSI_PRINTENV:
            (void) output.append(U_STRING_TO_PARAM(environment));
         break;

         case SSI_SET:
            {
            for (i = 0; i < n; i += 4)
               {
               if (name_value[i]   == *str_var &&
                   name_value[i+2] == *str_value)
                  {
                  (void) environment.append(name_value[i+1]);
                  (void) environment.append(1U, '=');
                  (void) environment.append(name_value[i+3]);
                  (void) environment.append(1U, '\n');
                  }
               }
            }
         break;

         case SSI_CONFIG:
            {
            for (i = 0; i < n; i += 2)
               {
               name  = name_value[i];
               value = name_value[i+1];

                    if (name == *str_errmsg)  (errmsg  = value).duplicate();
               else if (name == *str_timefmt) (timefmt = value).duplicate();
               else if (name == *str_sizefmt) use_size_abbrev = (value == *str_abbrev);
               }
            }
         break;

         case SSI_ECHO:
            {
            for (i = 0; i < n; i += 2)
               {
               name  = name_value[i];
               value = name_value[i+1];

               if (name == *str_var)
                  {
                       if (value == *str_DATE_GMT)      (void) output.append(UDate::strftime(timefmt.data(), u_now.tv_sec));
                  else if (value == *str_DATE_LOCAL)    (void) output.append(UDate::strftime(timefmt.data(), u_now.tv_sec + u_now_adjust));
                  else if (value == *str_LAST_MODIFIED) (void) output.append(UDate::strftime(timefmt.data(), last_modified));
                  else if (value == *str_USER_NAME)     (void) output.append(u_user_name, u_user_name_len);
                  else if (value == *str_DOCUMENT_URI)  (void) output.append(U_HTTP_URI_TO_PARAM);
                  else if (value == *str_DOCUMENT_NAME) (void) output.append(docname);
                  else
                     {
                     // check if it is a cgi-var

                     pos = environment.find(value);

                     if (pos == U_NOT_FOUND) (void) output.append(U_CONSTANT_TO_PARAM("(none)"));
                     else
                        {
                        pos += value.size() + 1;

                        distance = environment.find('\n', pos);

                        if (environment.c_char(distance-1) == '"') --distance;

                        (void) output.append(environment.c_pointer(pos), distance - pos);
                        }
                     }
                  }
               }
            }
         break;

         case SSI_FSIZE:
         case SSI_INCLUDE:
         case SSI_FLASTMOD:
            {
            pathname.clear();

            if (n == 2)
               {
               name = name_value[0];

                    if (name == *str_file)    pathname = directory                       + '/' + name_value[1];
               else if (name == *str_virtual) pathname = UServer_Base::getDocumentRoot() +       name_value[1];
               }

            bfile = false;

            if (pathname.empty() == false)
               {
               UHTTP::file->setPath(pathname);

               if (UHTTP::isFileInCache() &&
                   UHTTP::isDataFromCache(false))
                  {
                  bfile   = true;
                  include = UHTTP::getDataFromCache(false, false);
                  }
               else if (UHTTP::file->open())
                  {
                  bfile = true;

                  UHTTP::file->fstat();

                  include = UHTTP::file->getContent();
                  }
               }

            if (bfile)
               {
                    if (op == SSI_FSIZE)   (void) output.append(UStringExt::numberToString(UHTTP::file->getSize(), use_size_abbrev));
               else if (op == SSI_INCLUDE) (void) output.append(getInclude(include, include_level));
               else                        (void) output.append(UDate::strftime(timefmt.data(), UHTTP::file->st_mtime)); // SSI_FLASTMOD
               }
            else
               {
               (void) output.append(errmsg);
               }
            }
         break;

         case SSI_EXEC:
            {
            if (n == 2)
               {
               name = name_value[0];

                    if (name == *str_cmd) (void) output.append(UCommand::outputCommand(name_value[1], 0, -1, UServices::getDevNull()));
               else if (name == *str_cgi)
                  {
                  UHTTP::pathname->snprintf("%w%.*s", U_STRING_TO_TRACE(name_value[1]));

                  if (UHTTP::checkForCGIRequest() &&
                      UHTTP::processCGIRequest((UCommand*)0, &environment, false))
                     {
                     (void) output.append(U_STRING_TO_PARAM(*UClientImage_Base::wbuffer));
                     }
                  }
               }
            }
         break;
         }
      }

   U_RETURN_STRING(output);
}

// Server-wide hooks

int USSIPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "USSIPlugIn::handlerConfig(%p)", &cfg)

   // --------------------------------------------------------------------------------------------------------------
   // SSI_EXT_MASK  mask (DOS regexp) of special filename extension that recognize an SSI-enabled HTML file (*.shtml)
   // --------------------------------------------------------------------------------------------------------------

   if (cfg.loadTable()) ssi_ext_mask = cfg[*str_SSI_EXT_MASK];

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// Connection-wide hooks

int USSIPlugIn::handlerRequest()
{
   U_TRACE(0, "USSIPlugIn::handlerRequest()")

   if (UHTTP::isHTTPRequestNotFound()  == false &&
       UHTTP::isHTTPRequestForbidden() == false &&
       ssi_ext_mask.empty()            == false &&
       u_dosmatch_with_OR(U_HTTP_URI_TO_PARAM, U_STRING_TO_PARAM(ssi_ext_mask), 0))
      {
      uint32_t pos;
      const char* ptr;
      UString body, header(U_CAPACITY);

      // init

      errmsg          = *str_errmsg_default;
      timefmt         = *str_timefmt_default;
      environment     = UHTTP::getCGIEnvironment() + *UHTTP::penvironment;
      use_size_abbrev = true;

      (docname = UHTTP::getDocumentName()).duplicate();

      // read the SSI file

      bool deflate = (U_http_is_accept_deflate == '1'), bappend = deflate;

      if (UHTTP::isHTTPRequestAlreadyProcessed())
         {
         if (deflate) bappend = (UHTTP::isDataFromCache(true) == false);

         body          = UHTTP::getDataFromCache(false, false);
         header        = UHTTP::getDataFromCache(true, (bappend == false));
         last_modified = UHTTP::file_data->mtime;
         }
      else
         {
         body          = UHTTP::file->getContent();
         last_modified = UHTTP::file->st_mtime;
         }

      if (bappend) (void) header.append(U_CONSTANT_TO_PARAM("Content-Encoding: deflate\r\n"));

      if (UHTTP::isHTTPRequestAlreadyProcessed() == false) UHTTP::getFileMimeType(UHTTP::file->getSuffix(), 0, header, UHTTP::file->getSize());

      header = UHTTP::getHTTPHeaderForResponse(HTTP_OK, header);

      U_INTERNAL_DUMP("header = %.*S", U_STRING_TO_TRACE(header))

      pos = header.find(*USocket::str_content_length);

      U_INTERNAL_DUMP("pos = %.*S", 20, header.c_pointer(pos))

      U_INTERNAL_ASSERT_DIFFERS(pos, U_NOT_FOUND)

      ptr = header.c_pointer(pos + USocket::str_content_length->size() + 2);

      // process SSI file

                   *UClientImage_Base::body = processSSIRequest(body, 0);
      if (deflate) *UClientImage_Base::body = UStringExt::deflate(*UClientImage_Base::body);

      // NB: adjusting the size of response...

      (void) UHTTP::checkHTTPContentLength(ptr, UClientImage_Base::body->size(), header);

      U_INTERNAL_DUMP("header = %.*S", U_STRING_TO_TRACE(header))

      *UClientImage_Base::wbuffer = header;

      UHTTP::setHTTPRequestProcessed();
      }

   U_RETURN(U_PLUGIN_HANDLER_GO_ON);
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* USSIPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "last_modified         " << last_modified        << '\n'
                  << "use_size_abbrev       " << use_size_abbrev      << '\n'
                  << "errmsg       (UString " << (void*)&errmsg       << ")\n"
                  << "timefmt      (UString " << (void*)&timefmt      << ")\n"
                  << "docname      (UString " << (void*)&docname      << ")\n"
                  << "environment  (UString " << (void*)&environment  << ")\n"
                  << "ssi_ext_mask (UString " << (void*)&ssi_ext_mask << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
