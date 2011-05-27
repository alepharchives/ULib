// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    soap_parser.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/xml/soap/soap_fault.h>
#include <ulib/xml/soap/soap_object.h>
#include <ulib/xml/soap/soap_parser.h>
#include <ulib/xml/soap/soap_encoder.h>

const UString* USOAPParser::str_true;
const UString* USOAPParser::str_fault;
const UString* USOAPParser::str_xmlns;
const UString* USOAPParser::str_version11;
const UString* USOAPParser::str_mustUnderstand;

void USOAPParser::str_allocate()
{
   U_TRACE(0, "USOAPParser::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_true,0)
   U_INTERNAL_ASSERT_EQUALS(str_fault,0)
   U_INTERNAL_ASSERT_EQUALS(str_xmlns,0)
   U_INTERNAL_ASSERT_EQUALS(str_version11,0)
   U_INTERNAL_ASSERT_EQUALS(str_mustUnderstand,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("true") },
      { U_STRINGREP_FROM_CONSTANT("Fault") },
      { U_STRINGREP_FROM_CONSTANT("xmlns") },
      { U_STRINGREP_FROM_CONSTANT("mustUnderstand") },
      { U_STRINGREP_FROM_CONSTANT("http://schemas.xmlsoap.org/soap/envelope/") }
   };

   U_NEW_ULIB_OBJECT(str_true,           U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_fault,          U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_xmlns,          U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_mustUnderstand, U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_version11,      U_STRING_FROM_STRINGREP_STORAGE(4));
}

USOAPParser::~USOAPParser()
{
   U_TRACE_UNREGISTER_OBJECT(0, USOAPParser)

   clearData();

#ifdef U_SOAP_NAMESPACE
   XMLNStoURN.deallocate();
#endif
}

bool USOAPParser::parse(const UString& msg)
{
   U_TRACE(0, "USOAPParser::parse(%.*S)", U_STRING_TO_TRACE(msg))

   initParser();

   if (UXMLParser::parse(msg))
      {
      // If we succeeded, get the name of the method being called. This of course assumes only
      // one method in the body, and that there are no objects outside of the serialization root.
      // This method will need an override if this assumption is invalid

      U_INTERNAL_ASSERT_POINTER(body)

      method              = body->childAt(0);
      envelope.methodName = method->elem()->getAccessorName();

      // load the parameters for the method to execute

      UString param;

      for (uint32_t i = 0, num_arguments = method->numChild(); i < num_arguments; ++i)
         {
         param = method->childAt(i)->elem()->getValue();

         // check if parameter optional

         if (param.empty() == false) envelope.arg->push_back(param);
         }

      U_RETURN(true);
      }

   U_RETURN(false);
}

UString USOAPParser::getFaultResponse()
{
   U_TRACE(0, "USOAPParser::getFaultResponse()")

   U_INTERNAL_ASSERT_POINTER(method)

   URPCFault fault;

   fault.setFaultCode();

                                 fault.getFaultReason() = (*envelope.arg)[0];
   if (envelope.arg->size() > 1) fault.getDetail()      = (*envelope.arg)[1];

   UString retval(U_CAPACITY);

   fault.encode(retval);

   U_RETURN_STRING(retval);
}

UString USOAPParser::processMessage(const UString& msg, URPCObject& object, bool& bContainsFault)
{
   U_TRACE(0, "USOAPParser::processMessage(%.*S,%p,%p,%b)", U_STRING_TO_TRACE(msg), &object, &bContainsFault)

   U_ASSERT(msg.empty() == false)

   UString retval;

   clearData();

   if (msg.find(*str_version11) != U_NOT_FOUND)
      {
      zero();

      retval         = USOAPEncoder::encodeVersionMismatch();
      bContainsFault = true;
      }
   else if (parse(msg))
      {
      U_ASSERT(envelope.methodName != *str_fault)

      retval = object.processMessage(envelope, bContainsFault);
      }
   else
      {
      int line = ::XML_GetCurrentLineNumber(m_parser),
          coln = ::XML_GetCurrentColumnNumber(m_parser);

      object.setFailed();

      URPCMethod::pFault->getFaultReason() = UXMLParser::getErrorMessage();

      U_INTERNAL_DUMP("UXMLParser: %.*S (%d,%d) ", U_STRING_TO_TRACE(URPCMethod::pFault->getFaultReason()), line, coln)

      URPCMethod::pFault->setDetail("The fault occurred near position (line: %d col: %d) within the message", line, coln);

      bContainsFault = true;
      retval         = URPCMethod::encoder->encodeFault(URPCMethod::pFault);
      }

   U_RETURN_STRING(retval);
}

void USOAPParser::startElement(const XML_Char* name, const XML_Char** attrs)
{
   U_TRACE(0, "USOAPParser::startElement(%S,%p)", name, attrs)

   U_DUMP_ATTRS(attrs)

   UXMLAttribute* attribute;
   UString str((void*)name), namespaceName, accessorName, value;

   UXMLElement::splitNamespaceAndName(str, namespaceName, accessorName);

   if (flag_state == 0)
      {
      U_INTERNAL_ASSERT(u_rmatch(U_STRING_TO_PARAM(str), U_CONSTANT_TO_PARAM(":Envelope")))

      flag_state = 1;
      }

   U_DUMP("flag_state = %d ptree = %p ptree->parent() = %p ptree->numChild() = %u ptree->depth() = %u",
           flag_state, ptree, ptree->parent(), ptree->numChild(), ptree->depth())

   current = U_NEW(UXMLElement(str, accessorName, namespaceName));
   ptree   = ptree->push(current);

   if (flag_state <= 2)
      {
      if (flag_state == 1 &&
          u_rmatch(U_STRING_TO_PARAM(str), U_CONSTANT_TO_PARAM(":Header")))
         {
         header     = ptree;
         flag_state = 2;
         }
      else if (u_rmatch(U_STRING_TO_PARAM(str), U_CONSTANT_TO_PARAM(":Body")))
         {
         body       = ptree;
         flag_state = 3;
         }
      }

   U_INTERNAL_DUMP("flag_state = %d", flag_state)

   while (*attrs)
      {
        str.replace(*attrs++);
      value.replace(*attrs++);

      UXMLElement::splitNamespaceAndName(str, namespaceName, accessorName);

      attribute = U_NEW(UXMLAttribute(str, accessorName, namespaceName, value));

      current->addAttribute(attribute);

      // check if anybody has mustUnderstand set to true

      if (flag_state   == 2 &&
          accessorName == *str_mustUnderstand)
         {
         envelope.mustUnderstand = (value == *str_true);

         U_INTERNAL_DUMP("envelope.mustUnderstand = %b", envelope.mustUnderstand)
         }

      // set the name of namespace qualified element information (gSOAP)

      if (flag_state    == 1 &&
          namespaceName == *str_xmlns &&
           accessorName == *URPCMethod::str_ns)
         {
         envelope.nsName = value;

         U_INTERNAL_DUMP("envelope.nsName = %.*S", U_STRING_TO_TRACE(envelope.nsName))
         }

      // Manage the names and URNs of any and all namespaces found when parsing the message.
      // If duplicate namespace names are used with different URNs, only the last one found
      // will appear in the set of values.

#  ifdef U_SOAP_NAMESPACE
      if (namespaceName == *str_xmlns) XMLNStoURN.insert(accessorName, value);
#  endif
      }
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* USOAPParser::dump(bool reset) const
{
   URPCParser::dump(false);

   *UObjectIO::os << '\n';

   UXMLParser::dump(false);

   *UObjectIO::os << '\n'
                  << "flag_state                                        " << flag_state         << '\n'
#              ifdef U_SOAP_NAMESPACE
                  << "XMLNStoURN      (UHashMap<UString>                " << (void*)&XMLNStoURN << ")\n"
#              endif
                  << "tree            (UTree<UXMLElement*>              " << (void*)&tree       << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
