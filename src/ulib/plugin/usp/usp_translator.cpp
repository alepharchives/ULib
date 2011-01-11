// =======================================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//		usp_translator.cpp - the translator .usp => .cpp for plugin dynamic page for UServer
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

#define U_DYNAMIC_PAGE_TEMPLATE \
"#include <ulib/all.h>\n" \
"\n" \
"#define U_DYNAMIC_PAGE_APPEND(string)              (void)UClientImage_Base::wbuffer->append(string)\n" \
"\n" \
"#define U_DYNAMIC_PAGE_OUTPUT(fmt,args...)			 (UClientImage_Base::_buffer->snprintf(fmt , ##args),UClientImage_Base::wbuffer->append(*UClientImage_Base::_buffer))\n" \
"\n" \
"#define U_DYNAMIC_PAGE_OUTPUT_ENCODED(fmt,args...) (UClientImage_Base::_buffer->snprintf(fmt , ##args),UXMLEscape::encode(*UClientImage_Base::_buffer,*UClientImage_Base::_encoded),UClientImage_Base::wbuffer->append(*UClientImage_Base::_encoded))\n" \
"\n" \
"#define U_DYNAMIC_PAGE_GET_FORM_VALUE(n)                UHTTP::getFormValue(*UClientImage_Base::_value, n)\n" \
"\n" \
"#define U_DYNAMIC_PAGE_OUTPUT_FORM_VALUE(fmt,n)         (U_DYNAMIC_PAGE_GET_FORM_VALUE(n),U_DYNAMIC_PAGE_OUTPUT(fmt,U_STRING_TO_TRACE(*UClientImage_Base::_value)))\n" \
"#define U_DYNAMIC_PAGE_OUTPUT_ENCODED_FORM_VALUE(fmt,n) (U_DYNAMIC_PAGE_GET_FORM_VALUE(n),U_DYNAMIC_PAGE_OUTPUT_ENCODED(fmt,U_STRING_TO_TRACE(*UClientImage_Base::_value)))\n" \
"\n" \
"%.*s\n" \
"extern \"C\" {\n" \
"extern U_EXPORT void runDynamicPage(UClientImage_Base* client_image);\n" \
"       U_EXPORT void runDynamicPage(UClientImage_Base* client_image)\n" \
"{\n"	\
"  U_TRACE(0, \"::runDynamicPage(%%p)\", client_image)\n" \
"\n" \
"  if (client_image == 0 || client_image == (UClientImage_Base*)-1 || client_image == (UClientImage_Base*)-2) return;\n" \
"\n" \
"  U_INTERNAL_ASSERT_POINTER(UClientImage_Base::_value)\n" \
"  U_INTERNAL_ASSERT_POINTER(UClientImage_Base::_buffer)\n" \
"  U_INTERNAL_ASSERT_POINTER(UClientImage_Base::rbuffer)\n" \
"  U_INTERNAL_ASSERT_POINTER(UClientImage_Base::wbuffer)\n" \
"  U_INTERNAL_ASSERT_POINTER(UClientImage_Base::_encoded)\n" \
"  U_INTERNAL_ASSERT_EQUALS( UClientImage_Base::pClientImage, client_image)\n" \
"%s" \
"\n" \
"%.*s} }\n"

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

		bool bcontent_type = true;
		uint32_t endHeader = u_findEndHeader(U_STRING_TO_PARAM(usp));

		U_INTERNAL_DUMP("endHeader = %u u_line_terminator_len = %d", endHeader, u_line_terminator_len)

		// NB: we check for HTTP Header and Content-Type...

		if (endHeader															  == U_NOT_FOUND ||
			 U_STRING_FIND_EXT(usp, 0, "Content-Type: ", endHeader) == U_NOT_FOUND)
			{
			const char* ptr = usp.data();

			// NB: we check for <h(1|tml)> (HTML without HTTP headers..)

			if (			 ptr[0]  != '<' ||
				u_toupper(ptr[1]) != 'H')
				{
				bcontent_type = false;
				}
			}

		UTokenizer t(usp);
		t.setGroup(U_CONSTANT_TO_PARAM("<%%>"));

		bool bgroup;
		const char* directive;
		uint32_t n, distance, pos, size;
		UString token, declaration, buffer(U_CAPACITY), output(U_CAPACITY);

		while (true)
			{
			distance = t.getDistance();
			pos      = usp.find("<%", distance);

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

					buffer.snprintf("(void) UClientImage_Base::wbuffer->append(U_CONSTANT_TO_PARAM(%.*s));\n", U_STRING_TO_TRACE(tmp));

					(void) output.append(buffer);
					}
				}

			if (t.next(token, &bgroup) == false) break;

			U_INTERNAL_ASSERT(bgroup)

			U_INTERNAL_DUMP("token = %.*S", U_STRING_TO_TRACE(token))

			directive = token.c_pointer(1);
			n			 = token.size() - 1;

			U_INTERNAL_DUMP("directive = %10s", directive)

			switch (token.first_char())
				{
				case '!': // <%! ... %>
					{
					declaration = UStringExt::trim(directive, n);
					}
				break;

				case '=': // <%= ... %>
					{
					token = UStringExt::trim(directive, n);

					buffer.snprintf("UClientImage_Base::_buffer->snprintf(\"%%.*s\", %.*s);\n"
										 "(void) UClientImage_Base::wbuffer->append(*UClientImage_Base::_buffer);\n", U_STRING_TO_TRACE(token));

					(void) output.append(buffer);
					}
				break;

				default: // plain code block <% ... %>
					{
					(void) output.append(UStringExt::trim(token));
					(void) output.push_back('\n');
					}
				break;
				}
			}

		UString result(100U + sizeof(U_DYNAMIC_PAGE_TEMPLATE) + declaration.size() + output.size());

		result.snprintf(U_DYNAMIC_PAGE_TEMPLATE,
							 U_STRING_TO_TRACE(declaration),
							 (bcontent_type ? "" : "\nUClientImage_Base::wbuffer->snprintf(\"Content-Type: \" U_CTYPE_HTML \"\\r\\n\\r\\n\");\n"),
							 U_STRING_TO_TRACE(output));


		buffer.snprintf("%.*s.cpp", u_strlen(filename) - 4, filename);

		(void) UFile::writeTo(buffer, result);
		}

private:
};

U_MAIN(Application)
