// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    rpc_gen_method.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/net/rpc/rpc_object.h>
#include <ulib/net/rpc/rpc_envelope.h>

UString* URPCGenericMethod::str_command_fault;
UString* URPCGenericMethod::str_command_not_started;

void URPCGenericMethod::str_allocate()
{
   U_TRACE(0, "URPCGenericMethod::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_command_fault,0)
   U_INTERNAL_ASSERT_EQUALS(str_command_not_started,0)

   static ustringrep stringrep_storage[] = {
       { U_STRINGREP_FROM_CONSTANT("command failed") },
       { U_STRINGREP_FROM_CONSTANT("command not started") }
   };

   U_NEW_ULIB_OBJECT(str_command_fault,       U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_command_not_started, U_STRING_FROM_STRINGREP_STORAGE(1));
}

bool URPCGenericMethod::execute(URPCEnvelope& theCall)
{
   U_TRACE(0, "URPCGenericMethod::execute(%p)", &theCall)

   U_ASSERT(theCall.getMethodName() == getMethodName())

   UString input;
   UString* pinput;
   UString* poutput;
   int32_t old_ncmd = command->getNumArgument();
   uint32_t i = 0, num_arguments = theCall.getNumArgument();

   U_INTERNAL_DUMP("old_ncmd = %u num_arguments = %u", old_ncmd, num_arguments)

   if (URPCObject::isStdInput(response_type))
      {
      U_INTERNAL_ASSERT_MAJOR(num_arguments, 0)

      input = theCall.getArgument(i++);

      U_ASSERT(input.empty() == false)

      pinput = &input;
      }
   else
      {
      pinput = 0;
      }

   if (URPCObject::isSuccessOrFailure(response_type))
      {
      poutput = 0;
      }
   else
      {
      response.setEmpty();

      poutput = &response;
      }

   for (; i < num_arguments; ++i) command->addArgument(theCall.getArgumentCStr(i));

   bool result = command->execute(pinput, poutput);

   command->setNumArgument(old_ncmd, true); // we need to free strndup() malloc...

   if (result == false)
      {
      UCommand::setMsgError(command->getCommand());

      URPCObject::dispatcher->setFailed();

      URPCMethod::pFault->setDetail();
      URPCMethod::pFault->setFaultCode();
      URPCMethod::pFault->getFaultReason() = *(UCommand::isStarted() ? str_command_fault
                                                                     : str_command_not_started);
      URPCMethod::pFault->encode(response);

      U_RETURN(false);
      }

   if (URPCObject::isSuccessOrFailure(response_type)) response.replace(UCommand::exit_value == 0 ? '1' : '0');

   U_RETURN(true);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* URPCGenericMethod::dump(bool reset) const
{
   URPCMethod::dump(false);

   *UObjectIO::os << '\n'
                  << "response_type            " << (void*)response_type << '\n'
                  << "command     (UCommand    " << (void*)command       << ")\n"
                  << "response    (UString     " << (void*)&response     << ")\n"
                  << "method_name (UString     " << (void*)&method_name  << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
