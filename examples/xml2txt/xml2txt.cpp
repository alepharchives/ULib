// xml2txt.cpp

#include <ulib/file.h>
#include <ulib/base/utility.h>
#include <ulib/container/vector.h>
#include <ulib/xml/expat/xml_parser.h>

#undef  PACKAGE
#define PACKAGE "xml2txt"
#undef  ARGS
#define ARGS "XML"

#define U_OPTIONS \
"purpose \"Expects xml as input, outputs text only\"\n" \
"option t tag     1 \"list of tag separated by comma to use as filter\" \"\"\n" \
"option x exclude 0 \"the tag listed are excluded\" \"\"\n"

#include <ulib/application.h>

#define U_NOT_TO_PRINT ((active == false && excluded == false) || \
                        (active == true  && excluded == true))

static UVector<UString>* taglist;

class Xml2Txt : public UXMLParser {
public:

   uint32_t tagn;
   bool active, excluded;

   virtual void startElement(const XML_Char* name, const XML_Char** attrs)
      {
      U_TRACE(0, "Xml2Txt::startElement(%S,%p)", name, attrs)

      U_DUMP_ATTRS(attrs)

      U_INTERNAL_DUMP("active = %b", active)

      if (active == false)
         {
         if (taglist)
            {
            tagn   = taglist->find(UString(name));
            active = (tagn != U_NOT_FOUND);
            }

         U_INTERNAL_DUMP("active = %b", active)
         }

      if (U_NOT_TO_PRINT) return;

      if (attrs) for (int i = 0; attrs[i]; ++i) std::cout << ' ' << attrs[i];
      }

   virtual void characterData(const XML_Char* str, int len)
      {
      U_TRACE(5, "Xml2Txt::characterData(%.*S,%d)", len, str, len)

      U_INTERNAL_DUMP("active = %b", active)

      if (U_NOT_TO_PRINT) return;

      while (u_isspace(*str) && len > 0)
         {
         ++str;
         --len;
         }

      if (len > 0) std::cout.write(str, len);
      }

   virtual void endElement(const XML_Char* name)
      {
      U_TRACE(0, "Xml2Txt::endElement(%S)", name)

      U_INTERNAL_DUMP("active = %b", active)

      if (U_NOT_TO_PRINT == false) std::cout.put('\n');

      if (active && taglist && taglist->at(tagn) == name)
         {
         active = false;

         U_INTERNAL_DUMP("active = false")
         }
      }
};

class Application : public UApplication {
public:

   ~Application()
      {
      if (taglist) delete taglist;
      }

   void run(int argc, char* argv[], char* env[])
      {
      U_TRACE(5, "Application::run(%d,%p,%p)", argc, argv, env)

      UApplication::run(argc, argv, env);

      // manage options

      converter.excluded = false;

      if (UApplication::isOptions())
         {
         cfg_str = opt['t'];

         if (opt['x'].empty() == false) converter.excluded = true;
         }

      if (cfg_str.empty()) converter.active = true;
      else
         {
         converter.active = false;

         taglist = new UVector<UString>(cfg_str, ',');
         }

      // excluded

      // manage arg operation

      // parse xml

      converter.initParser(false, 0);

      UString xml(argv[optind]);

      if (converter.parse(UFile::contentOf(xml)) == false)
         {
         UApplication::exit_value = 1;

         U_ERROR("xml parsing error: %s", converter.getErrorMessage()); 
         }
      }

private:
   UString cfg_str;
   Xml2Txt converter;
};

U_MAIN(Application)
