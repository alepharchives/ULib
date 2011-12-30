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
"extern \"C\" {\n" \
"extern U_EXPORT int runDynamicPage(UClientImage_Base* client_image);\n" \
"       U_EXPORT int runDynamicPage(UClientImage_Base* client_image)\n" \
"{\n" \
"\tU_TRACE(0, \"::runDynamicPage(%%p)\", client_image)\n" \
"\t\n" \
"%s" \
"%s" \
"%s" \
"%s" \
"\t\n" \
"%.*s" \
"\t\n" \
"\tU_RETURN(0);\n" \
"} }\n"

class Application : public UApplication {
public:

   void run(int argc, char* argv[], char* env[])
      {
      U_TRACE(5, "Application::run(%d,%p,%p)", argc, argv, env)

      UApplication::run(argc, argv, env);

      const char* ptr;
      const char* filename = argv[1];

      if (filename == 0) U_ERROR("filename not specified...");

      UString usp = UFile::contentOf(filename);

      if (usp.empty()) U_ERROR("filename not valid...");

      bool bflag         = false;
      uint32_t endHeader = u_findEndHeader(U_STRING_TO_PARAM(usp));

      U_INTERNAL_DUMP("endHeader = %u u_line_terminator_len = %d", endHeader, u_line_terminator_len)

      // NB: we check for HTTP Header and Content-Type...

      if (endHeader                                              == U_NOT_FOUND ||
          U_STRING_FIND_EXT(usp, 0, "Content-Type: ", endHeader) == U_NOT_FOUND)
         {
         ptr = usp.data();

         // NB: we check for <h(1|tml)> (HTML without HTTP headers..)

         while (u_isspace(*ptr)) ++ptr; // skip space...

         if (         ptr[0]  != '<' ||
            u_toupper(ptr[1]) != 'H')
            {
            bflag = true;
            }
         }

      // Anything that is not enclosed in <!-- ... --> tags is assumed to be HTML

      UTokenizer t(usp);
      t.setGroup(U_CONSTANT_TO_PARAM("<!--->"));

      const char* ptr1 = "";
      const char* ptr2 = "";
      const char* ptr3 = "";
      const char* directive;
      uint32_t n, distance, pos, size;
      UString token, declaration, buffer(U_CAPACITY), output(U_CAPACITY);
      bool bgroup, bcout = false, binit = false, breset = false, bend = false;

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

            size = pos - distance;

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

         U_INTERNAL_DUMP("directive = %10s", directive)

         if (U_STRNEQ(directive, "declaration"))
            {
            U_ASSERT(declaration.empty()) // NB: <!--#declaration ... --> must be at the beginning...

            n = token.size() - U_CONSTANT_SIZE("declaration") - 2;

            declaration = UStringExt::trim(directive + U_CONSTANT_SIZE("declaration"), n);

            (void) declaration.append(U_CONSTANT_TO_PARAM("\n\t\n"));

            // NB: to avoid trailing \n...

            ptr = t.getPointer();

            while (u_isspace(*ptr)) ++ptr; // skip space...

            t.setPointer(ptr);

            binit  = (declaration.find("static void usp_init()")  != U_NOT_FOUND); // usp_init  (    Server-wide hooks)...
            breset = (declaration.find("static void usp_reset()") != U_NOT_FOUND); // usp_reset (Connection-wide hooks)...
            bend   = (declaration.find("static void usp_end()")   != U_NOT_FOUND); // usp_end
            }
         else if (U_STRNEQ(directive, "code"))
            {
            n = token.size() - U_CONSTANT_SIZE("code") - 2;

            token = UStringExt::trim(directive + U_CONSTANT_SIZE("code"), n);

            (void) output.append(U_CONSTANT_TO_PARAM("\n\t"));
            (void) output.append(token);
            (void) output.append(U_CONSTANT_TO_PARAM("\n\t\n"));
            }
         else if (U_STRNEQ(directive, "puts"))
            {
            n = token.size() - U_CONSTANT_SIZE("puts") - 2;

            token = UStringExt::trim(directive + U_CONSTANT_SIZE("puts"), n);

            buffer.snprintf("\n\t(void) UClientImage_Base::wbuffer->append(%.*s);\n\t", U_STRING_TO_TRACE(token));

            (void) output.append(buffer);
            }
         else if (U_STRNEQ(directive, "cout"))
            {
            n = token.size() - U_CONSTANT_SIZE("cout") - 2;

            token = UStringExt::trim(directive + U_CONSTANT_SIZE("cout"), n);

            if (bcout == false)
               {
               bcout = true;

               (void) output.append(U_CONSTANT_TO_PARAM("\n\tuint32_t usp_sz;"
                                                        "\n\tchar usp_buffer[10 * 4096];"
                                                        "\n\t"));
               }

            buffer.snprintf("\n\tusp_sz = UObject2String(%.*s, usp_buffer, sizeof(usp_buffer));"
                            "\n\t(void) UClientImage_Base::wbuffer->append(usp_buffer, usp_sz);\n\t", U_STRING_TO_TRACE(token));

            (void) output.append(buffer);
            }
         }

      UString result(200U + sizeof(USP_TEMPLATE) + declaration.size() + output.size());

      if (binit  == false &&
          bend   == false &&
          breset == false)
         {
         ptr1 = "\n\tif (client_image == 0 || client_image == (UClientImage_Base*)-1 || client_image == (UClientImage_Base*)-2) U_RETURN(0);\n";
         }
      else
         {
         if (binit)  ptr1 = "\n\tif (client_image == 0)         { usp_init();  U_RETURN(0); }\n"; // usp_init  (    Server-wide hooks)...
         if (breset) ptr2 = "\n\tif (client_image == (void*)-1) { usp_reset(); U_RETURN(0); }\n"; // usp_reset (Connection-wide hooks)...
         if (bend)   ptr3 = "\n\tif (client_image == (void*)-2) { usp_end();   U_RETURN(0); }\n"; // usp_end
         }

      buffer.snprintf("%.*s.cpp", u_str_len(filename) - 4, filename);

      result.snprintf(USP_TEMPLATE,
                      U_STRING_TO_TRACE(buffer),
                      U_STRING_TO_TRACE(declaration),
                      ptr1,
                      ptr2,
                      ptr3,
                      (bflag  == false ? ""
                                       : "\n\tUClientImage_Base::wbuffer->snprintf(\"Content-Type: \" U_CTYPE_HTML \"\\r\\n\\r\\n\");\n"),
                      U_STRING_TO_TRACE(output));

      (void) UFile::writeTo(buffer, UStringExt::removeEmptyLine(result));
      }

private:
};

U_MAIN(Application)
