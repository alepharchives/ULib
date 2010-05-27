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

#include <ulib/file.h>
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

   bool result;
   UString input;
   UString* pinput;
   int32_t old_ncmd = command->getNumArgument();
   uint32_t i = 0, num_arguments = theCall.getNumArgument();

   U_INTERNAL_DUMP("old_ncmd = %u num_arguments = %u", old_ncmd, num_arguments)

   if (URPCObject::isStdInput(response_type) == false) pinput = 0;
   else
      {
      U_INTERNAL_ASSERT_MAJOR(num_arguments, 0)

      input = theCall.getArgument(i++);

      U_ASSERT(input.empty() == false)

      pinput = &input;
      }

   for (; i < num_arguments; ++i) command->addArgument(theCall.getArgumentCStr(i));

#ifdef DEBUG
   static int fd_stderr = UFile::creat("/tmp/URPCGenericMethod.err", O_WRONLY | O_APPEND, PERM_FILE);
#else
   static int fd_stderr = UServices::getDevNull();
#endif

   if (URPCObject::isSuccessOrFailure(response_type))
      {
      result = command->executeAndWait(pinput, -1, fd_stderr);
      }
   else
      {
      response.setEmpty();

      result = command->execute(pinput, &response, -1, fd_stderr);
      }

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
                  << "response    (UString     " << (void*)&response     << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
