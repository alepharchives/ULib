// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    smtp.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_SMTP_CLIENT_H
#define U_SMTP_CLIENT_H 1

#ifdef HAVE_SSL
#  include <ulib/ssl/net/sslsocket.h>
#  define Socket USSLSocket
#  ifdef U_NO_SSL
#  undef U_NO_SSL
#  endif
#else
#  include <ulib/net/tcpsocket.h>
#  define Socket UTCPSocket
#  define Socket UTCPSocket
#  ifndef U_NO_SSL
#  define U_NO_SSL
#  endif
#endif

/**
   @class USmtpClient
   
   @brief Creates and manages a client connection with a remote SMTP server.
*/

/* How to execute an smtp transaction:
 *
 *   1. Establish the connection
 *   2. Read greeting from remote smtp (expect 220)
 *   3. Handshake with the SMTP server
 *         send "HELO"                 (expect 250)
 *         send "MAIL"                 (expect 250)
 *         send "RCPT"                 (expect 250)
 *   4. Write
 *         send "DATA"                 (expect 354)
 *         ....................
 *         send "<CRLF>.<CRLF>"        (expect 250)
 *   5. Quit
 *         send "QUIT"                 (expect 221 then disconnect)
 */

class UFileConfig;

class U_EXPORT USmtpClient : public Socket {
public:

   enum SMTPClientStatus {
      INIT     =  50,    // not logged in yet
      LOG_IN   = 100,    // logged in, got 220
      READY    = 150,    // sent HELO, got 250
      SENTFROM = 200,    // sent MAIL FROM:, got 250
      SENTTO   = 250,    // sent RCTP TO:, got 250
      DATA     = 300,    // DATA sent, got 354
      FINISHED = 350,    // finished sending data, got 250
      QUIT     = 400,    // sent QUIT, got 221
      LOG_OUT  = 450,    // finished, logged out
      CERROR   = 500     // didn't finish, had error or connection drop
      };

   enum SMTPServerStatus {
      NONE                        =   0, // null
      CONNREFUSED                 =   1, // No-one listening on the remote address
      GREET                       = 220, // greeting from server
      GOODBYE                     = 221, // server acknolages quit
      SUCCESSFUL                  = 250, // command successful
      READYDATA                   = 354, // server ready to receive data
      UNAVAILABLE                 = 450, // service not available
      ERR_PROCESSING              = 451, // error in processing
      INSUFFICIENT_SYSTEM_STORAGE = 452, // error insufficient system storage
      SERROR                      = 501, // error
      BAD_SEQUENCE_OF_COMMAND     = 503, // error bad sequence of command
      NOT_IMPLEMENT               = 504, // not implemented
      MAILBOX_UNAVAILABLE         = 550, // error mailbox unavailable
      EXCEED_STORAGE_ALLOCATION   = 552, // error exceed storage allocation
      MAILBOX_NAME_NOT_ALLOWED    = 553, // error mailbox name not alloweD
      TRANSACTION_FAILED          = 554  // error transaction failed
      };

   static const UString* str_empty;
   static const UString* str_address;
   static const UString* str_subject;
   static const UString* str_domainName;
   static const UString* str_TO_ADDRESS;
   static const UString* str_SMTP_SERVER;
   static const UString* str_SENDER_ADDRESS;
   static const UString* str_REPLY_TO_ADDRESS;

   static void str_allocate();

   /**
   Constructs a new USmtpClient with default values for all properties
   */

   USmtpClient(bool bSocketIsIPv6 = false) : Socket(bSocketIsIPv6)
      {
      U_TRACE_REGISTER_OBJECT(0, USmtpClient, "%b", bSocketIsIPv6)

      state    = INIT;
      response = NONE;

      if (str_empty == 0) str_allocate();
      }

   virtual ~USmtpClient();

   /**
   function called to established a socket connection with the SMTP network server
   */

   bool connectServer(UFileConfig& file,     int port = 25, uint32_t timeout = U_TIMEOUT_MS);
   bool connectServer(const UString& server, int port = 25, uint32_t timeout = U_TIMEOUT_MS);

   /**
   Execute an smtp transaction
   */

   char* status();

   UString getSenderAddress() const    { return senderAddress; }
   UString getRecipientAddress() const { return rcptoAddress; }

   bool startTLS();
   bool sendMessage(bool secure = false);

   void setDomainName(      const UString& name)      { domainName     = name; }
   void setMessageBody(     const UString& message)   { messageBody    = message; }
   void setMessageHeader(   const UString& header)    { messageHeader  = header; }
   void setSenderAddress(   const UString& sender);
   void setMessageSubject(  const UString& subject)   { messageSubject = subject; }
   void setRecipientAddress(const UString& recipient) { rcptoAddress   = recipient; }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   UString domainName, senderAddress, rcptoAddress, messageSubject, messageBody, messageHeader;
   SMTPClientStatus state;
   int response;

   static char buffer[128];

private:
   void setStateFromResponse() U_NO_EXPORT;
   bool syncCommand(const char* format, ...) U_NO_EXPORT; // Send a command to the SMTP server and wait for a response

   USmtpClient(const USmtpClient&) : Socket(false) {}
   USmtpClient& operator=(const USmtpClient&)      { return *this; }
};

#endif
