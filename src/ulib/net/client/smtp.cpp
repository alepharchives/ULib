// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    smtp.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file_config.h>
#include <ulib/mime/multipart.h>
#include <ulib/net/client/smtp.h>
#include <ulib/utility/string_ext.h>
#include <ulib/utility/socket_ext.h>

char USmtpClient::buffer[128];

const UString* USmtpClient::str_empty;
const UString* USmtpClient::str_address;
const UString* USmtpClient::str_subject;
const UString* USmtpClient::str_domainName;
const UString* USmtpClient::str_TO_ADDRESS;
const UString* USmtpClient::str_SMTP_SERVER;
const UString* USmtpClient::str_SENDER_ADDRESS;
const UString* USmtpClient::str_REPLY_TO_ADDRESS;

void USmtpClient::str_allocate()
{
   U_TRACE(0, "USmtpClient::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_empty,0)
   U_INTERNAL_ASSERT_EQUALS(str_address,0)
   U_INTERNAL_ASSERT_EQUALS(str_subject,0)
   U_INTERNAL_ASSERT_EQUALS(str_domainName,0)
   U_INTERNAL_ASSERT_EQUALS(str_TO_ADDRESS,0)
   U_INTERNAL_ASSERT_EQUALS(str_SMTP_SERVER,0)
   U_INTERNAL_ASSERT_EQUALS(str_SENDER_ADDRESS,0)
   U_INTERNAL_ASSERT_EQUALS(str_REPLY_TO_ADDRESS,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("empty") },
      { U_STRINGREP_FROM_CONSTANT("user@host.ext") },
      { U_STRINGREP_FROM_CONSTANT("(no subject)") },
      { U_STRINGREP_FROM_CONSTANT("somemachine.nowhere.org") },
      { U_STRINGREP_FROM_CONSTANT("TO_ADDRESS") },
      { U_STRINGREP_FROM_CONSTANT("SMTP_SERVER") },
      { U_STRINGREP_FROM_CONSTANT("SENDER_ADDRESS") },
      { U_STRINGREP_FROM_CONSTANT("REPLY_TO_ADDRESS") }
   };

   U_NEW_ULIB_OBJECT(str_empty,            U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_address,          U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_subject,          U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_domainName,       U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_TO_ADDRESS,       U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_SMTP_SERVER,      U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_SENDER_ADDRESS,   U_STRING_FROM_STRINGREP_STORAGE(6));
   U_NEW_ULIB_OBJECT(str_REPLY_TO_ADDRESS, U_STRING_FROM_STRINGREP_STORAGE(7));
}

USmtpClient::USmtpClient(bool bSocketIsIPv6) : Socket(bSocketIsIPv6)
{
   U_TRACE_REGISTER_OBJECT(0, USmtpClient, "%b", bSocketIsIPv6)

   state    = INIT;
   response = NONE;

   if (str_empty == 0) str_allocate();
}

USmtpClient::~USmtpClient()
{
   U_TRACE_UNREGISTER_OBJECT(0, USmtpClient)
}

char* USmtpClient::status()
{
   U_TRACE(0, "USmtpClient::status()")

   const char* descr;

   switch (response)
      {
      case CONNREFUSED:                   descr = 0;                                   break; //   1
      case GREET:                         descr = "greeting from server";              break; // 200
      case GOODBYE:                       descr = "server acknolages quit";            break; // 221
      case SUCCESSFUL:                    descr = "command successful";                break; // 250
      case READYDATA:                     descr = "server ready to receive data";      break; // 354
      case ERR_PROCESSING:                descr = "error in processing";               break; // 451
      case UNAVAILABLE:                   descr = "service not available";             break; // 450
      case INSUFFICIENT_SYSTEM_STORAGE:   descr = "error insufficient system storage"; break; // 452
      case SERROR:                        descr = "error";                             break; // 501
      case BAD_SEQUENCE_OF_COMMAND:       descr = "error bad sequence of command";     break; // 503
      case NOT_IMPLEMENT:                 descr = "not implemented";                   break; // 504
      case MAILBOX_UNAVAILABLE:           descr = "error mailbox unavailable";         break; // 550
      case EXCEED_STORAGE_ALLOCATION:     descr = "error exceed storage allocation";   break; // 552
      case MAILBOX_NAME_NOT_ALLOWED:      descr = "error mailbox name not allowed";    break; // 553
      case TRANSACTION_FAILED:            descr = "error transaction failed";          break; // 554
      default:                            descr = "???";                               break;
      }

   if (descr) (void) sprintf(buffer, "(%d, %s)", response, descr);

   U_RETURN(buffer);
}

bool USmtpClient::_connectServer(const UString& server, int port, uint32_t timeoutMS)
{
   U_TRACE(0, "USmtpClient::_connectServer(%.*S,%d,%u)", U_STRING_TO_TRACE(server), port, timeoutMS)

#ifdef USE_LIBSSL
   U_INTERNAL_ASSERT(Socket::isSSL())
   ((USSLSocket*)this)->setActive(false);
#endif

   if (Socket::connectServer(server, port) == false)
      {
      response = CONNREFUSED;

      (void) u_sn_printf(buffer, sizeof(buffer), "Sorry, couldn't connect to server '%.*s:%d'%R", U_STRING_TO_TRACE(server), port, 0); // NB: the last argument (0) is necessary...
      }
   else
      {
      (void) USocket::setTimeoutRCV(timeoutMS);

      response = USocketExt::readMultilineReply(this);

      U_DUMP("status() = %S", status())

      if (response == GREET)
         {
         state = LOG_IN;

         U_RETURN(true);
         }
      }

   U_RETURN(false);
}

bool USmtpClient::_connectServer(UFileConfig& cfg, int port, uint32_t timeoutMS)
{
   U_TRACE(0, "USmtpClient::_connectServer(%p,%d,%u)", &cfg, port, timeoutMS)

   U_ASSERT_EQUALS(cfg.empty(), false)

   // ----------------------------------------------------------------------------------------------------------------------
   // USmtpClient - configuration parameters
   // ----------------------------------------------------------------------------------------------------------------------
   // SMTP_SERVER       host name or ip address for server
   //
   //       TO_ADDRESS
   //   SENDER_ADDRESS
   // REPLY_TO_ADDRESS
   // ----------------------------------------------------------------------------------------------------------------------

   setSenderAddress(   cfg[*str_SENDER_ADDRESS]);
   setRecipientAddress(cfg[*str_TO_ADDRESS]);

   if (_connectServer(cfg[*str_SMTP_SERVER], port, timeoutMS))
      {
      UString replyToAddress = cfg[*str_REPLY_TO_ADDRESS];

      if (replyToAddress.empty() == false)
         {
         U_ASSERT(UStringExt::isEmailAddress(replyToAddress))

         UString tmp(10U + replyToAddress.size());

         tmp.snprintf("Reply-To: %.*s", U_STRING_TO_TRACE(replyToAddress));

         setMessageHeader(tmp);
         }

      U_RETURN(true);
      }

   U_RETURN(false);
}

void USmtpClient::setSenderAddress(const UString& sender)
{
   U_TRACE(0, "USmtpClient::setSenderAddress(%.*S)", U_STRING_TO_TRACE(sender))

   uint32_t index = sender.find('<');

   if (index == U_NOT_FOUND)
      {
      senderAddress = sender;

      return;
      }

   ++index;

   uint32_t n = sender.find('>', index);

   if (n != U_NOT_FOUND) n -= index;

   UString tmp = UStringExt::simplifyWhiteSpace(sender.data() + index, n);

   n = 0;

   while (true)
      {
      index = tmp.find(' ', n);

      if (index == U_NOT_FOUND) break;

      n = index + 1; // take one side
      }

   senderAddress.replace(tmp.data() + n, tmp.size() - n);

   index = senderAddress.find('@');

   // won't go through without a local mail system

   if (index == U_NOT_FOUND) senderAddress.append(U_CONSTANT_TO_PARAM("@localhost"));
}

U_NO_EXPORT void USmtpClient::setStateFromResponse()
{
   U_TRACE(0, "USmtpClient::setStateFromResponse()")

   switch (response)
      {
      case USocket::BROKEN: state = CERROR; break;
      case GREET:           state = LOG_IN; break;
      case GOODBYE:         state = QUIT;   break;
      case READYDATA:       state = DATA;   break;

      case SUCCESSFUL:
         {
         switch (state)
            {
            case LOG_IN:   state = READY;    break;
            case READY:    state = SENTFROM; break;
            case SENTFROM: state = SENTTO;   break;
            case DATA:     state = FINISHED; break;

            case INIT:
            case QUIT:
            case SENTTO:
            case FINISHED:
            case LOG_OUT:
            case CERROR:
            default:       state = CERROR;   break;
            }
         }
      break;

      default: state = CERROR; break;
      }

   U_INTERNAL_DUMP("state = %d", state)
}

// Send a command to the SMTP server and wait for a response

U_NO_EXPORT bool USmtpClient::syncCommand(const char* format, ...)
{
   U_TRACE(0, "USmtpClient::syncCommand(%S)", format)

   va_list argp;
   va_start(argp, format);

   response = USocketExt::vsyncCommandML(this, format, argp);

   va_end(argp);

   setStateFromResponse();

   U_RETURN(true);
}

bool USmtpClient::startTLS()
{
   U_TRACE(0, "USmtpClient::startTLS()")

#ifdef USE_LIBSSL
   U_INTERNAL_ASSERT(Socket::isSSL())

   if (syncCommand("STARTTLS") &&
       response == GREET)
      {
          ((USSLSocket*)this)->setActive(true);
      if (((USSLSocket*)this)->secureConnection(USocket::iSockDesc)) U_RETURN(true);
          ((USSLSocket*)this)->setActive(false);
      }
#endif

   U_RETURN(false);
}

// Execute an smtp transaction

bool USmtpClient::sendMessage(bool secure)
{
   U_TRACE(0, "USmtpClient::sendMessage(%b)", secure)

   U_INTERNAL_ASSERT_EQUALS(state,LOG_IN)

   if (domainName.empty())
      {
      /*
      struct utsname uts;
      uname(&uts);
      domainName = uts.nodename;
      */

      domainName = *str_domainName;
      }

   if (secure)
      {
      (void) syncCommand("ehlo %.*s", U_STRING_TO_TRACE(domainName));

      if (response != SUCCESSFUL                ||
          strstr(u_buffer, "250-STARTTLS") == 0 ||
          startTLS() == false)
         {
         U_RETURN(false);
         }

      (void) syncCommand("ehlo %.*s", U_STRING_TO_TRACE(domainName));
      }
   else
      {
      (void) syncCommand("helo %.*s", U_STRING_TO_TRACE(domainName));
      }

   if (response != SUCCESSFUL) U_RETURN(false);

   U_INTERNAL_ASSERT_EQUALS(state,READY)

   if (senderAddress.empty()) senderAddress = *str_address;

   U_ASSERT(UStringExt::isEmailAddress(senderAddress))

   (void) syncCommand("mail from: %.*s", U_STRING_TO_TRACE(senderAddress));

   if (response != SUCCESSFUL) U_RETURN(false);

   U_INTERNAL_ASSERT_EQUALS(state,SENTFROM)

   if (rcptoAddress.empty()) rcptoAddress = *str_address;

   U_ASSERT(UStringExt::isEmailAddress(rcptoAddress))

   (void) syncCommand("rcpt to: %.*s", U_STRING_TO_TRACE(rcptoAddress));

   if (response != SUCCESSFUL) U_RETURN(false);

   U_INTERNAL_ASSERT_EQUALS(state,SENTTO)

   (void) syncCommand("data", 0);

   if (response != READYDATA) U_RETURN(false);

   U_INTERNAL_ASSERT_EQUALS(state,DATA)

   if (messageSubject.empty()) messageSubject = *str_subject;

        if           (messageBody.empty()) messageBody = *str_empty;
   else if (U_STRNCMP(messageBody.data(), "MIME-Version: "))
      {
      u_line_terminator = U_CRLF;

      messageBody = UMimeMultipartMsg::section(messageBody, "", UMimeMultipartMsg::AUTO, "", "", "MIME-Version: 1.0");
      }

   UString msg(rcptoAddress.size() + messageSubject.size() + messageHeader.size() + messageBody.size() + 32U);

   msg.snprintf("To: %.*s\r\n"
                "Subject: %.*s\r\n"
                "%.*s\r\n"
                "%.*s\r\n"
                ".\r\n",
                U_STRING_TO_TRACE(rcptoAddress),
                U_STRING_TO_TRACE(messageSubject),
                U_STRING_TO_TRACE(messageHeader),
                U_STRING_TO_TRACE(messageBody));

   response = (USocketExt::write(this, msg) ? USocketExt::readMultilineReply(this) : (int)USocket::BROKEN);

   setStateFromResponse();

   if (response != SUCCESSFUL) U_RETURN(false);

   U_INTERNAL_ASSERT_EQUALS(state,FINISHED)

   (void) syncCommand("quit", 0);

   if (response != GOODBYE) U_RETURN(false);

   U_INTERNAL_ASSERT_EQUALS(state,QUIT)

   U_RETURN(true);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* USmtpClient::dump(bool reset) const
{
   UTCPSocket::dump(false);

   *UObjectIO::os << '\n'
                  << "state                         " << state                  << '\n'
                  << "response                      " << response               << '\n'
                  << "domainName      (UString      " << (void*)&domainName     << ")\n"
                  << "messageBody     (UString      " << (void*)&messageBody    << ")\n"
                  << "rcptoAddress    (UString      " << (void*)&rcptoAddress   << ")\n"
                  << "messageHeader   (UString      " << (void*)&messageHeader  << ")\n"
                  << "senderAddress   (UString      " << (void*)&senderAddress  << ")\n"
                  << "messageSubject  (UString      " << (void*)&messageSubject << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
