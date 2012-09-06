// =======================================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    usp_translator.cpp - the translator .usp => .cpp for plugin dynamic page for UServer
//
// = AUTHOR
//    Stefano Casazza
//
// =======================================================================================

#include <ulib/file.h>
#include <ulib/tokenizer.h>
#include <ulib/utility/escape.h>
#include <ulib/utility/string_ext.h>

#undef  PACKAGE
#define PACKAGE "usp_translator"
#undef  ARGS
#define ARGS "[filename]"

#define U_OPTIONS ""
#define U_PURPOSE "program for dynamic page translation (*.usp => *.cpp)"

#include <ulib/application.h>

#define USP_TEMPLATE \
"// %.*s\n" \
"\t\n" \
"#include <ulib/net/server/usp_macro.h>\n" \
"\t\n" \
"%.*s" \
"\t\n" \
"\t\n" \
"extern \"C\" {\n" \
"extern U_EXPORT int runDynamicPage(UClientImage_Base* client_image);\n" \
"       U_EXPORT int runDynamicPage(UClientImage_Base* client_image)\n" \
"{\n" \
"\tU_TRACE(0, \"::runDynamicPage(%%p)\", client_image)\n" \
"\t\n" \
"%s"   \
"%s"   \
"%s"   \
"\n\tuint32_t usp_sz;\n" \
"\n\tchar usp_buffer[10 * 4096];\n" \
"\n\tbool usp_as_service = (client_image == (UClientImage_Base*)-3);\n" \
"\t\n" \
"%.*s" \
"%.*s" \
"%.*s" \
"%.*s" \
"\n\tU_RETURN(usp_as_service ? 0 : %u);\n" \
"} }\n"

class Application : public UApplication {
public:

   void run(int argc, char* argv[], char* env[])
      {
      U_TRACE(5, "Application::run(%d,%p,%p)", argc, argv, env)

      UApplication::run(argc, argv, env);

      const char* filename = argv[1];

      if (filename == 0) U_ERROR("filename not specified...");

      UString usp = UFile::contentOf(filename);

      if (usp.empty()) U_ERROR("filename not valid...");

      const char* ptr;
      const char* ptr1 = "";
      const char* ptr2 = "";
      const char* ptr3 = "";
      const char* directive;
      uint32_t i, n, distance, pos, size;
      bool bgroup, binit = false, breset = false, bend = false;
      UString token, declaration, http_header, buffer(U_CAPACITY),
              output(U_CAPACITY), output1(U_CAPACITY), xoutput(U_CAPACITY);

      // Anything that is not enclosed in <!-- ... --> tags is assumed to be HTML

      UTokenizer t(usp);
      t.setGroup(U_CONSTANT_TO_PARAM("<!--->"));

      while (true)
         {
         distance = t.getDistance();
         pos      = usp.find("<!--#", distance);

         if (pos)
            {
            if (pos != U_NOT_FOUND) t.setDistance(pos);
            else
               {
               pos = usp.size();

               t.setDistance(pos);

               while (usp.c_char(pos-1) == '\n') --pos; // no trailing \n...
               }

            size = (pos > distance ? pos - distance : 0);

            U_INTERNAL_DUMP("size = %u", size)

            if (size)
               {
               token = usp.substr(distance, size);
         
               // plain html block

               UString tmp(token.size() * 4);

               UEscape::encode(token, tmp, false);

               (void) buffer.reserve(tmp.size() + 100U);

               buffer.snprintf("\n\t(void) UClientImage_Base::wbuffer->append(\n\t\tU_CONSTANT_TO_PARAM(%.*s)\n\t);\n\t", U_STRING_TO_TRACE(tmp));

               (void) output.append(buffer);
               }
            }

         if (t.next(token, &bgroup) == false) break;

         U_INTERNAL_ASSERT(bgroup)

         U_INTERNAL_DUMP("token = %.*S", U_STRING_TO_TRACE(token))

         directive = token.c_pointer(2); // "-#"...

         U_INTERNAL_DUMP("directive(10) = %.*S", 10, directive)

         if (U_STRNEQ(directive, "declaration"))
            {
            U_ASSERT(declaration.empty()) // NB: <!--#declaration ... --> must be at the beginning...

            n = token.size() - U_CONSTANT_SIZE("declaration") - 2;

            declaration = UStringExt::trim(directive + U_CONSTANT_SIZE("declaration"), n);

            binit  = (declaration.find("static void usp_init()")  != U_NOT_FOUND); // usp_init  (    Server-wide hooks)...
            breset = (declaration.find("static void usp_reset()") != U_NOT_FOUND); // usp_reset (Connection-wide hooks)...
            bend   = (declaration.find("static void usp_end()")   != U_NOT_FOUND); // usp_end

            declaration = UStringExt::substitute(declaration, '\n', U_CONSTANT_TO_PARAM("\n\t"));
            }
         else if (U_STRNEQ(directive, "header"))
            {
            U_ASSERT(http_header.empty())

            n = token.size() - U_CONSTANT_SIZE("header") - 2;

            http_header = UStringExt::trim(directive + U_CONSTANT_SIZE("header"), n);
            }
         else if (U_STRNEQ(directive, "session"))
            {
            n = token.size() - U_CONSTANT_SIZE("session") - 2;

            token = UStringExt::trim(directive + U_CONSTANT_SIZE("session"), n);

            (void) output.append(U_CONSTANT_TO_PARAM("\n\t"));
            (void) output.append(token);
            (void) output.append(U_CONSTANT_TO_PARAM("\n\t\n"));

            UString tmp, name;
            UVector<UString> vec(token, "\t\n;");

            for (i = 0, n = vec.size(); i < n; ++i)
               {
#           ifdef DEBUG
               name.clear(); // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...
#           endif
               tmp = UStringExt::trim(vec[i]);
               ptr = tmp.data();

               do { ++ptr; } while (u__isspace(*ptr) == false);
               do { ++ptr; } while (u__isspace(*ptr) == true);

               name = tmp.substr(tmp.distance(ptr));
               pos  = name.find('(');

               size = (pos == U_NOT_FOUND ? name.size() : pos);

               buffer.snprintf("\n\tUSP_SESSION_VAR_GET(%u,%.*s);\n\t", i, size, ptr);

               (void) output.append(buffer);

               output1.snprintf_add("\n\tUSP_SESSION_VAR_PUT(%u,%.*s);\n\t", i, size, ptr);
               }
            }
         else if (U_STRNEQ(directive, "storage"))
            {
            n = token.size() - U_CONSTANT_SIZE("storage") - 2;

            token = UStringExt::trim(directive + U_CONSTANT_SIZE("storage"), n);

            (void) output.append(U_CONSTANT_TO_PARAM("\n\t"));
            (void) output.append(token);
            (void) output.append(U_CONSTANT_TO_PARAM("\n\t\n"));

            UString tmp, name;
            UVector<UString> vec(token, "\t\n;");

            for (i = 0, n = vec.size(); i < n; ++i)
               {
#           ifdef DEBUG
               name.clear(); // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...
#           endif
               tmp = UStringExt::trim(vec[i]);
               ptr = tmp.data();

               do { ++ptr; } while (u__isspace(*ptr) == false);
               do { ++ptr; } while (u__isspace(*ptr) == true);

               name = tmp.substr(tmp.distance(ptr));
               pos  = name.find('(');

               size = (pos == U_NOT_FOUND ? name.size() : pos);

               buffer.snprintf("\n\tUSP_STORAGE_VAR_GET(%u,%.*s);\n\t", i, size, ptr);

               (void) output.append(buffer);

               output1.snprintf_add("\n\tUSP_STORAGE_VAR_PUT(%u,%.*s);\n\t", i, size, ptr);
               }
            }
         else if (U_STRNEQ(directive, "args"))
            {
            n = token.size() - U_CONSTANT_SIZE("args") - 2;

            token = UStringExt::trim(directive + U_CONSTANT_SIZE("args"), n);

            (void) output.append(U_CONSTANT_TO_PARAM("\n\tif (usp_as_service == false) UHTTP::processHTTPForm();\n\t"));

            (void) output1.append(U_CONSTANT_TO_PARAM("\n\tif (UHTTP::form_name_value->size()) UHTTP::resetForm(true);\n\t"));

            UString tmp, name;
            UVector<UString> vec(token, "\t\n;");

            (void) buffer.reserve(token.size() * 100U);

            for (i = 0, n = vec.size(); i < n; ++i)
               {
#           ifdef DEBUG
               name.clear(); // NB: to avoid DEAD OF SOURCE STRING WITH CHILD ALIVE...
#           endif
               tmp  = UStringExt::trim(vec[i]);
               pos  = tmp.find('(');
               name = (pos == U_NOT_FOUND ? tmp : tmp.substr(0U, pos));

               buffer.snprintf("\n\tUString %.*s = USP_FORM_VALUE(%u);\n\t", U_STRING_TO_TRACE(name), i);

               if (pos != U_NOT_FOUND)
                  {
                  ptr  = name.data();
                  size = name.size();

                  buffer.snprintf_add("\n\tif (%.*s.empty()) %.*s = U_STRING_FROM_CONSTANT(%.*s);\n\t",
                                       size, ptr, size, ptr, tmp.size() - pos - 2, tmp.c_pointer(pos + 1));  
                  }

               (void) output.append(buffer);
               }
            }
         else if (U_STRNEQ(directive, "code"))
            {
            n = token.size() - U_CONSTANT_SIZE("code") - 2;

            token = UStringExt::trim(directive + U_CONSTANT_SIZE("code"), n);

            (void) output.append(U_CONSTANT_TO_PARAM("\n\t"));
            (void) output.append(UStringExt::substitute(token, '\n', U_CONSTANT_TO_PARAM("\n\t")));
            (void) output.append(U_CONSTANT_TO_PARAM("\n\t\n"));
            }
         else if (U_STRNEQ(directive, "xcode"))
            {
            n = token.size() - U_CONSTANT_SIZE("xcode") - 2;

            token = UStringExt::trim(directive + U_CONSTANT_SIZE("xcode"), n);

            (void) xoutput.append(U_CONSTANT_TO_PARAM("\n\t"));
            (void) output.append(UStringExt::substitute(token, '\n', U_CONSTANT_TO_PARAM("\n\t")));
            (void) xoutput.append(U_CONSTANT_TO_PARAM("\n\t\n"));
            }
         else if (U_STRNEQ(directive, "puts"))
            {
            n = token.size() - U_CONSTANT_SIZE("puts") - 2;

            token = UStringExt::trim(directive + U_CONSTANT_SIZE("puts"), n);

            (void) buffer.reserve(token.size() + 100U);

            buffer.snprintf("\n\t(void) UClientImage_Base::wbuffer->append(%.*s);\n\t", U_STRING_TO_TRACE(token));

            (void) output.append(buffer);
            }
         else if (U_STRNEQ(directive, "xputs"))
            {
            n = token.size() - U_CONSTANT_SIZE("xputs") - 2;

            token = UStringExt::trim(directive + U_CONSTANT_SIZE("xputs"), n);

            buffer.snprintf("\n\tUSP_PUTS_XML(%.*s);\n\t", U_STRING_TO_TRACE(token));

            (void) output.append(buffer);
            }
         else if (U_STRNEQ(directive, "number"))
            {
            n = token.size() - U_CONSTANT_SIZE("number") - 2;

            token = UStringExt::trim(directive + U_CONSTANT_SIZE("number"), n);

            (void) buffer.reserve(token.size() + 100U);

            buffer.snprintf("\n\t(void) UClientImage_Base::wbuffer->append(UStringExt::numberToString(%.*s));\n\t", U_STRING_TO_TRACE(token));

            (void) output.append(buffer);
            }
         else if (U_STRNEQ(directive, "cout"))
            {
            n = token.size() - U_CONSTANT_SIZE("cout") - 2;

            token = UStringExt::trim(directive + U_CONSTANT_SIZE("cout"), n);

            (void) buffer.reserve(token.size() + 150U);

            buffer.snprintf("\n\tusp_sz = UObject2String(%.*s, usp_buffer, sizeof(usp_buffer));"
                            "\n\t(void) UClientImage_Base::wbuffer->append(usp_buffer, usp_sz);\n\t", U_STRING_TO_TRACE(token));

            (void) output.append(buffer);
            }

         // no trailing \n...

         for (ptr = t.getPointer(); u__islterm(*ptr); ++ptr) {}

         t.setPointer(ptr);
         }

      if (binit  == false &&
          bend   == false &&
          breset == false)
         {
         ptr1 = "\n\tif (client_image == 0 || client_image == (UClientImage_Base*)-1 || client_image == (UClientImage_Base*)-2) U_RETURN(0);\n\t";
         }
      else
         {
         if (binit)  ptr1 = "\n\tif (client_image == 0)         { usp_init();  U_RETURN(0); }\n\t"; // (    Server-wide hooks)...
         else        ptr1 = "\n\tif (client_image == 0)         {              U_RETURN(0); }\n\t";
         if (breset) ptr2 = "\n\tif (client_image == (void*)-1) { usp_reset(); U_RETURN(0); }\n\t"; // (Connection-wide hooks)...
         else        ptr2 = "\n\tif (client_image == (void*)-1) {              U_RETURN(0); }\n\t";
         if (bend)   ptr3 = "\n\tif (client_image == (void*)-2) { usp_end();   U_RETURN(0); }\n\t";
         else        ptr3 = "\n\tif (client_image == (void*)-2) {              U_RETURN(0); }\n\t";
         }

      if (http_header.empty() == false)
         {
         UString header = UStringExt::dos2unix(http_header, true);

         (void) header.append(U_CONSTANT_TO_PARAM("\r\n\r\n"));

         UString tmp(header.size() * 4);

         UEscape::encode(header, tmp, false);

         http_header.setBuffer(tmp.size() + 100U);

         http_header.snprintf("\n\tif (usp_as_service == false) (void) UClientImage_Base::wbuffer->append(\n\t\tU_CONSTANT_TO_PARAM(%.*s)\n\t);\n\t",
                              U_STRING_TO_TRACE(tmp));
         }

      buffer.snprintf("%.*s.cpp", u__strlen(filename) - 4, filename);

      UString result(300U + sizeof(USP_TEMPLATE) + declaration.size() + http_header.size() + output.size() + output1.size() + xoutput.size());

      result.snprintf(USP_TEMPLATE,
                      U_STRING_TO_TRACE(buffer),
                      U_STRING_TO_TRACE(declaration),
                      ptr1,
                      ptr2,
                      ptr3,
                      U_STRING_TO_TRACE(http_header),
                      U_STRING_TO_TRACE(output),
                      U_STRING_TO_TRACE(output1),
                      U_STRING_TO_TRACE(xoutput),
                      (http_header.empty() ? 200 : 0));

      (void) UFile::writeTo(buffer, UStringExt::removeEmptyLine(result));
      }

private:
};

U_MAIN(Application)
