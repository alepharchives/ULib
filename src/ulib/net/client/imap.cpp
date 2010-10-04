// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    imap.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/tokenizer.h>
#include <ulib/net/client/imap.h>
#include <ulib/utility/socket_ext.h>

#define U_IMAP_OK  "* OK"
#define U_IMAP_ERR "* BAD"

UString* UImapClient::str_list;
UString* UImapClient::str_marked;
UString* UImapClient::str_unseen;
UString* UImapClient::str_recent;
UString* UImapClient::str_uidnext;
UString* UImapClient::str_unmarked;
UString* UImapClient::str_noselect;
UString* UImapClient::str_asterisk;
UString* UImapClient::str_messages;
UString* UImapClient::str_noinferiors;
UString* UImapClient::str_uidvalidity;

UString* UImapClient::str_seen;
UString* UImapClient::str_draft;
UString* UImapClient::str_flags;
UString* UImapClient::str_exists;
UString* UImapClient::str_recent1;
UString* UImapClient::str_flagged;
UString* UImapClient::str_deleted;
UString* UImapClient::str_answered;
UString* UImapClient::str_asterisk1;
UString* UImapClient::str_read_write;
UString* UImapClient::str_permanentflags;

UString* UImapClient::str_has_children;
UString* UImapClient::str_has_no_children;
UString* UImapClient::str_message_disposition_notification;
UString* UImapClient::str_junk;
UString* UImapClient::str_no_junk;
UString* UImapClient::str_Forwarded;

void UImapClient::str_allocate()
{
   U_TRACE(0, "UImapClient_Base::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_list,0)
   U_INTERNAL_ASSERT_EQUALS(str_marked,0)
   U_INTERNAL_ASSERT_EQUALS(str_unseen,0)
   U_INTERNAL_ASSERT_EQUALS(str_recent,0)
   U_INTERNAL_ASSERT_EQUALS(str_uidnext,0)
   U_INTERNAL_ASSERT_EQUALS(str_unmarked,0)
   U_INTERNAL_ASSERT_EQUALS(str_noselect,0)
   U_INTERNAL_ASSERT_EQUALS(str_asterisk,0)
   U_INTERNAL_ASSERT_EQUALS(str_messages,0)
   U_INTERNAL_ASSERT_EQUALS(str_noinferiors,0)
   U_INTERNAL_ASSERT_EQUALS(str_uidvalidity,0)

   U_INTERNAL_ASSERT_EQUALS(str_seen,0)
   U_INTERNAL_ASSERT_EQUALS(str_draft,0)
   U_INTERNAL_ASSERT_EQUALS(str_flags,0)
   U_INTERNAL_ASSERT_EQUALS(str_exists,0)
   U_INTERNAL_ASSERT_EQUALS(str_recent1,0)
   U_INTERNAL_ASSERT_EQUALS(str_flagged,0)
   U_INTERNAL_ASSERT_EQUALS(str_deleted,0)
   U_INTERNAL_ASSERT_EQUALS(str_answered,0)
   U_INTERNAL_ASSERT_EQUALS(str_asterisk1,0)
   U_INTERNAL_ASSERT_EQUALS(str_read_write,0)
   U_INTERNAL_ASSERT_EQUALS(str_permanentflags,0)

   U_INTERNAL_ASSERT_EQUALS(str_has_children,0)
   U_INTERNAL_ASSERT_EQUALS(str_has_no_children,0)
   U_INTERNAL_ASSERT_EQUALS(str_message_disposition_notification,0)
   U_INTERNAL_ASSERT_EQUALS(str_junk,0)
   U_INTERNAL_ASSERT_EQUALS(str_no_junk,0)
   U_INTERNAL_ASSERT_EQUALS(str_Forwarded,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("LIST") },
      { U_STRINGREP_FROM_CONSTANT("\\Marked") },
      { U_STRINGREP_FROM_CONSTANT("UNSEEN") },
      { U_STRINGREP_FROM_CONSTANT("RECENT") },
      { U_STRINGREP_FROM_CONSTANT("UIDNEXT") },
      { U_STRINGREP_FROM_CONSTANT("\\Unmarked") },
      { U_STRINGREP_FROM_CONSTANT("\\Noselect") },
      { U_STRINGREP_FROM_CONSTANT("*") },
      { U_STRINGREP_FROM_CONSTANT("MESSAGES") },
      { U_STRINGREP_FROM_CONSTANT("\\Noinferiors") },
      { U_STRINGREP_FROM_CONSTANT("UIDVALIDITY") },

      { U_STRINGREP_FROM_CONSTANT("\\Seen") },
      { U_STRINGREP_FROM_CONSTANT("\\Draft") },
      { U_STRINGREP_FROM_CONSTANT("FLAGS") },
      { U_STRINGREP_FROM_CONSTANT("EXISTS") },
      { U_STRINGREP_FROM_CONSTANT("\\Recent") },
      { U_STRINGREP_FROM_CONSTANT("\\Flagged") },
      { U_STRINGREP_FROM_CONSTANT("\\Deleted") },
      { U_STRINGREP_FROM_CONSTANT("\\Answered") },
      { U_STRINGREP_FROM_CONSTANT("\\*") },
      { U_STRINGREP_FROM_CONSTANT("READ-WRITE") },
      { U_STRINGREP_FROM_CONSTANT("PERMANENTFLAGS") },
      { U_STRINGREP_FROM_CONSTANT("\\HasChildren") },
      { U_STRINGREP_FROM_CONSTANT("\\HasNoChildren") },
      { U_STRINGREP_FROM_CONSTANT("$MDNSent") },
      { U_STRINGREP_FROM_CONSTANT("Junk") },
      { U_STRINGREP_FROM_CONSTANT("NonJunk"), },
      { U_STRINGREP_FROM_CONSTANT("$Forwarded") }
   };

   U_NEW_ULIB_OBJECT(str_list,        U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_marked,      U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_unseen,      U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_recent,      U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_uidnext,     U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_unmarked,    U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_noselect,    U_STRING_FROM_STRINGREP_STORAGE(6));
   U_NEW_ULIB_OBJECT(str_asterisk,    U_STRING_FROM_STRINGREP_STORAGE(7));
   U_NEW_ULIB_OBJECT(str_messages,    U_STRING_FROM_STRINGREP_STORAGE(8));
   U_NEW_ULIB_OBJECT(str_noinferiors, U_STRING_FROM_STRINGREP_STORAGE(9));
   U_NEW_ULIB_OBJECT(str_uidvalidity, U_STRING_FROM_STRINGREP_STORAGE(10));

   U_NEW_ULIB_OBJECT(str_seen,            U_STRING_FROM_STRINGREP_STORAGE(11));
   U_NEW_ULIB_OBJECT(str_draft,           U_STRING_FROM_STRINGREP_STORAGE(12));
   U_NEW_ULIB_OBJECT(str_flags,           U_STRING_FROM_STRINGREP_STORAGE(13));
   U_NEW_ULIB_OBJECT(str_exists,          U_STRING_FROM_STRINGREP_STORAGE(14));
   U_NEW_ULIB_OBJECT(str_recent1,         U_STRING_FROM_STRINGREP_STORAGE(15));
   U_NEW_ULIB_OBJECT(str_flagged,         U_STRING_FROM_STRINGREP_STORAGE(16));
   U_NEW_ULIB_OBJECT(str_deleted,         U_STRING_FROM_STRINGREP_STORAGE(17));
   U_NEW_ULIB_OBJECT(str_answered,        U_STRING_FROM_STRINGREP_STORAGE(18));
   U_NEW_ULIB_OBJECT(str_asterisk1,       U_STRING_FROM_STRINGREP_STORAGE(19));
   U_NEW_ULIB_OBJECT(str_read_write,      U_STRING_FROM_STRINGREP_STORAGE(20));
   U_NEW_ULIB_OBJECT(str_permanentflags,  U_STRING_FROM_STRINGREP_STORAGE(21));

   U_NEW_ULIB_OBJECT(str_has_children,    U_STRING_FROM_STRINGREP_STORAGE(22));
   U_NEW_ULIB_OBJECT(str_has_no_children, U_STRING_FROM_STRINGREP_STORAGE(23));

   U_NEW_ULIB_OBJECT(str_message_disposition_notification, U_STRING_FROM_STRINGREP_STORAGE(24));
   U_NEW_ULIB_OBJECT(str_junk,                             U_STRING_FROM_STRINGREP_STORAGE(25));
   U_NEW_ULIB_OBJECT(str_no_junk,                          U_STRING_FROM_STRINGREP_STORAGE(26));
   U_NEW_ULIB_OBJECT(str_Forwarded,                        U_STRING_FROM_STRINGREP_STORAGE(27));
}

U_NO_EXPORT const char* UImapClient::status()
{
   U_TRACE(0, "UImapClient::status()")

   const char* descr1;
   const char* descr2;

   switch (state)
      {
      case LOGOUT:                  descr1 = "LOGOUT";            break;
      case NOT_AUTHENTICATED:       descr1 = "NOT_AUTHENTICATED"; break;
      case AUTHENTICATED:           descr1 = "AUTHENTICATED";     break;
      case SELECTED:                descr1 = "SELECTED";          break;
      default:                      descr1 = "???";               break;
      }

   switch (response)
      {
      case IMAP_SESSION_OK:         descr2 = "OK";                   break;
      case IMAP_SESSION_CONTINUED:  descr2 = "to be continued...";   break;
      case IMAP_SESSION_BAD:        descr2 = "bad state";            break;
      default:                      descr2 = "???";                  break;
      }

   static char _buffer[128];

   (void) sprintf(_buffer, "%s - (%d, %s)", descr1, response, descr2);

   U_RETURN(_buffer);
}

bool UImapClient::connectServer(const UString& server, int port, uint32_t timeoutMS)
{
   U_TRACE(0, "UImapClient::connectServer(%.*S,%d,%u)", U_STRING_TO_TRACE(server), port, timeoutMS)

#ifdef HAVE_SSL
   U_INTERNAL_ASSERT(Socket::isSSL())
   ((USSLSocket*)this)->setActive(false);
#endif

   if (Socket::connectServer(server, port))
      {
      (void) USocket::setTimeoutRCV(timeoutMS);

      if (USocketExt::readLineReply(this, buffer) > 0 &&
          U_STRNEQ(buffer.data(), U_IMAP_OK))
         {
         response = IMAP_SESSION_OK;

         U_DUMP("status() = %S", status())

         U_RETURN(true);
         }
      }

   U_RETURN(false);
}

// Send a command to the IMAP server and wait for a response...

U_NO_EXPORT bool UImapClient::syncCommand(const char* format, ...)
{
   U_TRACE(0, "UImapClient::syncCommand(%S)", format)

   U_DUMP("status() = %S", status())

   buffer.setEmpty();

   va_list argp;
   va_start(argp, format);

   end = USocketExt::vsyncCommandToken(this, buffer, format, argp);

   va_end(argp);

   U_INTERNAL_DUMP("end = %d", end)

   if (U_STRNCMP(buffer.c_pointer(end), "OK")) U_RETURN(false);

   response = IMAP_SESSION_OK;

   U_RETURN(true);
}

bool UImapClient::startTLS()
{
   U_TRACE(0, "UImapClient::startTLS()")

#ifdef HAVE_SSL
   U_INTERNAL_ASSERT(Socket::isSSL())

   if (state == NOT_AUTHENTICATED)
      {
      if (syncCommand("STARTTLS"))
         {
             ((USSLSocket*)this)->setActive(true);
         if (((USSLSocket*)this)->secureConnection(USocket::getFd())) U_RETURN(true);
             ((USSLSocket*)this)->setActive(false);
         }
      }

   response = IMAP_SESSION_BAD;
#endif

   U_RETURN(false);
}

bool UImapClient::login(const char* user, const char* passwd)
{
   U_TRACE(0, "UImapClient::login(%S,%S)", user, passwd)

   if (state == NOT_AUTHENTICATED)
      {
      if (syncCommand("LOGIN %s %s", user, passwd))
         {
         state           = AUTHENTICATED;
         USocket::iState = USocket::LOGIN;

         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

U_NO_EXPORT void UImapClient::setEnd()
{
   U_TRACE(0, "UImapClient::setEnd()")

   U_ASSERT(buffer.empty() == false)

   const char* ptr1;
   const char* ptr2;

   // "\r\nU0001 " <- "OK ..."

   end -= 2;

   for (ptr1 = ptr2 = buffer.c_pointer(end); !u_isspace(*ptr1); --ptr1) {}

   // "\r" -> "\nU0001 OK ..."

   end -= (ptr2 - ptr1) + 1;

   U_INTERNAL_DUMP("end = %d ptr1 = %.*S", end, 9, ptr1)
}

int UImapClient::getCapabilities(UVector<UString>& vec)
{
   U_TRACE(0, "UImapClient::getCapabilities(%p)", &vec)

   if (syncCommand("CAPABILITY"))
      {
      setEnd();

      int pos = sizeof("* CAPABILITY");

      U_INTERNAL_ASSERT_MAJOR(end,pos)

      (void) capa.replace(0, capa.size(), buffer, pos, end - pos);

      uint32_t n = vec.split(capa);

      U_RETURN(n);
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(-1);
}

// Logout an imap session

bool UImapClient::logout()
{
   U_TRACE(0, "UImapClient::logout()")

   if (syncCommand("LOGOUT"))
      {
      state = LOGOUT;

      U_RETURN(true);
      }

   U_RETURN(false);
}

/* (LIST | LSUB) command representation
typedef struct ListResponse {
   UString name, hierarchyDelimiter;
   bool marked, unmarked, noSelect, noInferiors;
} ListResponse;
*/

bool UImapClient::list(const UString& ref, const UString& wild, UVector<ListResponse*>& vec, bool subscribedOnly)
{
   U_TRACE(0, "UImapClient::list(%.*S,%.*S,%p,%b)", U_STRING_TO_TRACE(ref), U_STRING_TO_TRACE(wild), &vec, subscribedOnly)

   if (state >= AUTHENTICATED)
      {
      if (syncCommand("%s \"%.*s\" %.*s",
               (subscribedOnly ? "LSUB" : "LIST"),
               U_STRING_TO_TRACE(ref), U_STRING_TO_TRACE(wild)))
         {
         setEnd();

         UString attributes;
         ListResponse* elem;
         UString x = buffer.substr(0U, (uint32_t)end);
         UVector<UString> v1(x);

         for (uint32_t i = 0, length = v1.size(); i < length; ++i)
            {
            U_ASSERT(v1[i]   == *str_asterisk)
            U_ASSERT(v1[i+1] == *str_list)

            i += 2;
            attributes = v1[i++];
            elem = U_NEW(ListResponse);

            elem->marked      = elem->unmarked =
            elem->noSelect    = elem->noInferiors =
            elem->hasChildren = elem->hasNoChildren = false;

            U_ASSERT_EQUALS(attributes[0],'(')

            if (attributes.size() > 2)
               {
               attributes.unQuote();

               UVector<UString> v2(attributes);

               U_INTERNAL_DUMP("attributes = %.*S", U_STRING_TO_TRACE(attributes))

               // (\\HasNoChildren \\HasChildren \\Marked \\Unmarked \\Noselect \\Noinferiors)

               for (uint32_t n = 0, l = v2.size(); n < l; ++n)
                  {
                  if (!elem->hasNoChildren &&
                      v2[n] == *str_has_no_children)
                     {
                     elem->hasNoChildren = true;
                     }
                  else if (!elem->hasChildren &&
                           v2[n] == *str_has_children)
                     {
                     elem->hasChildren = true;
                     }
                  else if (!elem->marked &&
                      v2[n] == *str_marked)
                     {
                     elem->marked = true;
                     }
                  else if (!elem->unmarked &&
                           v2[n] == *str_unmarked)
                     {
                     elem->unmarked = true;
                     }
                  else if (!elem->noSelect &&
                           v2[n] == *str_noselect)
                     {
                     elem->noSelect = true;
                     }
                  else if (!elem->noInferiors &&
                           v2[n] == *str_noinferiors)
                     {
                     elem->noInferiors = true;
                     }
                  else
                     {
                     U_ERROR("Unknow tag response for LIST command, exit..", 0);
                     }
                  }
               }

            elem->hierarchyDelimiter = v1[i++];
            elem->name               = v1[i];

            elem->name.duplicate();
            elem->hierarchyDelimiter.duplicate();

            vec.push_back(elem);
            }

         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

/* STATUS command representation
typedef struct StatusInfo {
   long messageCount, recentCount, nextUID, uidValidity, unseenCount;
   bool hasMessageCount, hasRecentCount, hasNextUID, hasUIDValidity, hasUnseenCount;
} StatusInfo;
*/

bool UImapClient::status(const UString& mailboxName, StatusInfo& retval, int items)
{
   U_TRACE(0, "UImapClient::status(%.*S,%p,%d)", U_STRING_TO_TRACE(mailboxName), &retval, items)

   if (state >= AUTHENTICATED)
      {
      if (syncCommand("STATUS %.*s (%s %s %s %s %s)",
               U_STRING_TO_TRACE(mailboxName),
               (items & MESSAGE_COUNT  ? "MESSAGES"    : ""),
               (items & RECENT_COUNT   ? "RECENT"      : ""),
               (items & NEXT_UID       ? "UIDNEXT"     : ""),
               (items & UID_VALIDITY   ? "UIDVALIDITY" : ""),
               (items & UNSEEN         ? "UNSEEN"      : "")))
         {
         setEnd();

         (void) memset(&retval, 0, sizeof(StatusInfo));

         const char* ptr1 = buffer.data();
         const char* ptr2 = ptr1 + sizeof("* STATUS") + mailboxName.size();

         while (*ptr2 != '(') ++ptr2;

         uint32_t length, i = (ptr2 - ptr1);

         UString x = buffer.substr(i, end - i);
         UVector<UString> vec(x);

         for (i = 0, length = vec.size(); i < length; ++i)
            {
            if (!retval.hasMessageCount &&
                vec[i] == *str_messages)
               {
               retval.messageCount    = vec[++i].strtol();
               retval.hasMessageCount = true;
               }
            else if (!retval.hasRecentCount &&
                     vec[i] == *str_recent)
               {
               retval.recentCount    = vec[++i].strtol();
               retval.hasRecentCount = true;
               }
            else if (!retval.hasNextUID &&
                     vec[i] == *str_uidnext)
               {
               retval.nextUID    = vec[++i].strtol();
               retval.hasNextUID = true;
               }
            else if (!retval.hasUIDValidity &&
                     vec[i] == *str_uidvalidity)
               {
               retval.uidValidity    = vec[++i].strtol();
               retval.hasUIDValidity = true;
               }
            else if (!retval.hasUnseenCount &&
                     vec[i] == *str_unseen)
               {
               retval.unseenCount    = vec[++i].strtol();
               retval.hasUnseenCount = true;
               }
            else
               {
               U_ERROR("Unknow tag response for STATUS command, exit..", 0);
               }
            }

         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

/* (SELECT | EXAMINE) command representation
typedef struct MailboxInfo {
   bool readWrite;
   StatusInfo status;
   int flags, permanentFlags;
   bool flagsAvailable, permanentFlagsAvailable, readWriteAvailable;
} MailboxInfo;

enum MailboxFlag {
   SEEN      = 1 << 0,
   ANSWERED  = 1 << 1,
   FLAGGED   = 1 << 2,
   DELETED   = 1 << 3,
   DRAFT     = 1 << 4,
   RECENT    = 1 << 5,
   ASTERISK  = 1 << 6,
   MDNSent   = 1 << 7,
   Junk      = 1 << 8,
   NonJunk   = 1 << 9,
   Forwarded = 1 << 10
};
*/

U_NO_EXPORT void UImapClient::setFlag(int& _flags, UVector<UString>& vec)
{
   U_TRACE(0, "UImapClient::setFlag(%B,%p)", _flags, &vec)

   for (uint32_t i = 0, length = vec.size(); i < length; ++i)
      {
      // * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen $MDNSent Junk NonJunk)\r\n

      U_INTERNAL_DUMP("vec[%d] = %.*S", i, U_STRING_TO_TRACE(vec[i]))

      if (!(_flags & ANSWERED) &&
          vec[i] == *str_answered)
         {
         _flags |= ANSWERED;
         }
      else if (!(_flags & FLAGGED) &&
               vec[i] == *str_flagged)
         {
         _flags |= FLAGGED;
         }
      else if (!(_flags & DRAFT) &&
               vec[i] == *str_draft)
         {
         _flags |= DRAFT;
         }
      else if (!(_flags & DELETED) &&
               vec[i] == *str_deleted)
         {
         _flags |= DELETED;
         }
      else if (!(_flags & SEEN) &&
               vec[i] == *str_seen)
         {
         _flags |= SEEN;
         }
      else if (!(_flags & RECENT) &&
               vec[i] == *str_recent1)
         {
         _flags |= RECENT;
         }
      else if (!(_flags & ASTERISK) &&
               vec[i] == *str_asterisk1)
         {
         _flags |= ASTERISK;
         }
      else if (!(_flags & MDNSent) &&
               vec[i] == *str_message_disposition_notification)
         {
         _flags |= MDNSent;
         }
      else if (!(_flags & Junk) &&
               vec[i] == *str_junk)
         {
         _flags |= Junk;
         }
      else if (!(_flags & NonJunk) &&
               vec[i] == *str_no_junk)
         {
         _flags |= NonJunk;
         }
      else if (!(_flags & Forwarded) &&
               vec[i] == *str_Forwarded)
         {
         _flags |= Forwarded;
         }
      else
         {
         U_ERROR("Unknow tag response for SELECT command, exit..", 0);
         }
      }

   U_INTERNAL_DUMP("flags = %B", _flags)
}

U_NO_EXPORT void UImapClient::setMailBox(MailboxInfo& retval)
{
   U_TRACE(0, "UImapClient::setMailBox(%p)", &retval)

   setEnd();

   (void) memset(&retval, 0, sizeof(MailboxInfo));

   /* Example
   -------------------------------------------------------------------------------
   U0005 SELECT INBOX\r\n
   -------------------------------------------------------------------------------
   * FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen)\r\n
   * OK [PERMANENTFLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen \\*)]  \r\n
   * 29 EXISTS\r\n
   * 28 RECENT\r\n
   * OK [UNSEEN 1]  \r\n
   * OK [UIDVALIDITY 1148569720]  \r\n
   * OK [UIDNEXT 5774]  \r\n
   U0005 OK [READ-WRITE] Completed\r\n
   -------------------------------------------------------------------------------
   */

   uint32_t n;
   UString line;
   const char* ptr1;
   const char* ptr2;
   UVector<UString> vec;
   UString l, x = buffer.substr(0U, (uint32_t)end);
   UTokenizer tok(x, U_CRLF);

   while (tok.next(line, '\n')) // '\n'
      {
      U_ASSERT_EQUALS(line[0],'*')

      if (line[2] == 'O') // OK ...
         {
         U_ASSERT_EQUALS(line[3],'K')
         U_ASSERT_EQUALS(line[4],' ')

         n = sizeof("* OK");

         U_ASSERT_EQUALS(line[n],'[')

         ptr1 = line.c_pointer(++n);

         if (U_STRNEQ(ptr1, "PERMANENTFLAGS"))
            {
            n += sizeof("PERMANENTFLAGS");

            U_ASSERT_EQUALS(line[n],'(')

            ptr1 = ptr2 = line.c_pointer(++n);

            while (*ptr2 != ')') ++ptr2;

            l = line.substr(n, ptr2 - ptr1);
            n = vec.split(l);

            setFlag(retval.permanentFlags, vec);

            retval.permanentFlagsAvailable = true;

            vec.clear();
            }
         else
            {
            ptr2 = ptr1;

            while (*ptr2 != ']') ++ptr2;

            l = line.substr(n, ptr2 - ptr1);
            n = vec.split(l);

            U_INTERNAL_ASSERT_EQUALS(n,2)

            if (!retval.status.hasUnseenCount &&
                vec[0] == *str_unseen)
               {
               retval.status.unseenCount    = vec[1].strtol();
               retval.status.hasUnseenCount = true;
               }
            else if (!retval.status.hasNextUID &&
                     vec[0] == *str_uidnext)
               {
               retval.status.nextUID    = vec[1].strtol();
               retval.status.hasNextUID = true;
               }
            else if (!retval.status.hasUIDValidity &&
                     vec[0] == *str_uidvalidity)
               {
               retval.status.uidValidity    = vec[1].strtol();
               retval.status.hasUIDValidity = true;
               }
            else
               {
               U_ERROR("Unknow tag response for SELECT command, exit..", 0);
               }

            vec.clear();
            }
         }
      else if (line[2] == 'F') // FLAGS ...
         {
         U_ASSERT_EQUALS(line[3],'L')
         U_ASSERT_EQUALS(line[4],'A')
         U_ASSERT_EQUALS(line[5],'G')
         U_ASSERT_EQUALS(line[6],'S')
         U_ASSERT_EQUALS(line[7],' ')

         n = sizeof("* FLAGS");

         U_ASSERT_EQUALS(line[n],'(')

         ptr1 = ptr2 = line.c_pointer(++n);

         while (*ptr2 != ')') ++ptr2;

         l = line.substr(n, ptr2 - ptr1);
         n = vec.split(l);

         setFlag(retval.flags, vec);

         retval.flagsAvailable = true;

         vec.clear();
         }
      else // EXISTS | RECENT
         {
         l = line.substr(2U);
         n = vec.split(l);

         U_INTERNAL_ASSERT_EQUALS(n,2)

         if (!retval.status.hasMessageCount &&
             vec[1] == *str_exists)
            {
            retval.status.messageCount    = vec[0].strtol();
            retval.status.hasMessageCount = true;
            }
         else if (!retval.status.hasRecentCount &&
                  vec[1] == *str_recent)
            {
            retval.status.recentCount    = vec[0].strtol();
            retval.status.hasRecentCount = true;
            }
         else
            {
            U_ERROR("Unknow tag response for SELECT command, exit..", 0);
            }

         vec.clear();
         }
      }

   ptr1 = buffer.c_pointer(end + sizeof(U_IMAP_OK));

   while (*ptr1 != '[') ++ptr1;

   if (U_STRNEQ(++ptr1, "READ-WRITE")) retval.readWriteAvailable = retval.readWrite = true;
}

bool UImapClient::selectMailbox(const UString& name, MailboxInfo& retval)
{
   U_TRACE(0, "UImapClient::selectMailbox(%.*S,%p)", U_STRING_TO_TRACE(name), &retval)

   U_ASSERT(name.empty() == false)

   if (state >= AUTHENTICATED)
      {
      // Don't re-select.

      if (state    == SELECTED &&
          selected == name)
         {
         U_RETURN(true);
         }

      if (syncCommand("SELECT %.*s", U_STRING_TO_TRACE(name)))
         {
         setMailBox(retval);

         state    = SELECTED;
         selected = name;

         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

bool UImapClient::examineMailbox(const UString& name, MailboxInfo& retval)
{
   U_TRACE(0, "UImapClient::examineMailbox(%.*S,%p)", U_STRING_TO_TRACE(name), &retval)

   U_ASSERT(name.empty() == false)

   if (state >= AUTHENTICATED)
      {
      if (syncCommand("EXAMINE %.*s", U_STRING_TO_TRACE(name)))
         {
         setMailBox(retval);

         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

bool UImapClient::createMailbox(const UString& name)
{
   U_TRACE(0, "UImapClient::createMailbox(%.*S)", U_STRING_TO_TRACE(name))

   U_ASSERT(name.empty() == false)

   if (state >= AUTHENTICATED)
      {
      if (syncCommand("CREATE %.*s", U_STRING_TO_TRACE(name)))
         {
         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

/**
* Attempt to remove the new mailbox with the given name.
*/

bool UImapClient::removeMailbox(const UString& name)
{
   U_TRACE(0, "UImapClient::removeMailbox(%.*S)", U_STRING_TO_TRACE(name))

   U_ASSERT(name.empty() == false)

   if (state >= AUTHENTICATED)
      {
      if (syncCommand("DELETE %.*s", U_STRING_TO_TRACE(name)))
         {
         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

/**
* Attempt to rename the new mailbox with the given name.
*/

bool UImapClient::renameMailbox(const UString& from, const UString& to)
{
   U_TRACE(0, "UImapClient::renameMailbox(%.*S,%.*S)", U_STRING_TO_TRACE(from), U_STRING_TO_TRACE(to))

   U_ASSERT(to.empty() == false)
   U_ASSERT(from.empty() == false)

   if (state >= AUTHENTICATED)
      {
      if (syncCommand("RENAME %.*s %.*s", U_STRING_TO_TRACE(from), U_STRING_TO_TRACE(to)))
         {
         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

/**
* Attempt to subscribe to the new mailbox with the given name. @see RFC
*/

bool UImapClient::subscribeMailbox(const UString& name)
{
   U_TRACE(0, "UImapClient::subscribeMailbox(%.*S)", U_STRING_TO_TRACE(name))

   U_ASSERT(name.empty() == false)

   if (state >= AUTHENTICATED)
      {
      if (syncCommand("SUBSCRIBE %.*s", U_STRING_TO_TRACE(name)))
         {
         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

/**
* Attempt to unsubscribe from the new mailbox with the given name. @see RFC
*/

bool UImapClient::unsubscribeMailbox(const UString& name)
{
   U_TRACE(0, "UImapClient::unsubscribeMailbox(%.*S)", U_STRING_TO_TRACE(name))

   U_ASSERT(name.empty() == false)

   if (state >= AUTHENTICATED)
      {
      if (syncCommand("UNSUBSCRIBE %.*s", U_STRING_TO_TRACE(name)))
         {
         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

bool UImapClient::appendMessage(const UString& mailboxName, const UString& messageData, int _flags, const char* date)
{
   U_TRACE(0, "UImapClient::appendMessage(%.*S,%.*S,%d,%S)",
                        U_STRING_TO_TRACE(mailboxName), U_STRING_TO_TRACE(messageData), _flags, date)

   U_INTERNAL_ASSERT_POINTER(date)
   U_ASSERT(mailboxName.empty() == false)
   U_ASSERT(messageData.empty() == false)

   if (state >= AUTHENTICATED)
      {
      if (syncCommand("APPEND %.*s %s%s%s%s%s%s%s%s %s\r\n%.*s",
               U_STRING_TO_TRACE(mailboxName),
               (_flags            ? "("           : ""),
               (_flags & SEEN     ? "\\Seen"      : ""),
               (_flags & ANSWERED ? " \\Answered" : ""),
               (_flags & FLAGGED  ? " \\Flagged"  : ""),
               (_flags & DELETED  ? " \\Deleted"  : ""),
               (_flags & DRAFT    ? " \\Draft"    : ""),
               (_flags & RECENT   ? " \\Recent"   : ""),
               (_flags            ? ")"           : ""),
               date,
               U_STRING_TO_TRACE(messageData)))
         {
         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

bool UImapClient::expunge(int* _ret)
{
   U_TRACE(0, "UImapClient::expunge(%p)", _ret)

   if (state == SELECTED)
      {
      if (syncCommand("EXPUNGE"))
         {
         if (_ret)
            {
            setEnd();

            /* Example
            -------------------------------------------------------------------------------
            U0005 EXPUNGE\r\n
            -------------------------------------------------------------------------------
            * 3 EXPUNGE\r\n
            * 3 EXPUNGE\r\n
            * 5 EXPUNGE\r\n
            * 8 EXPUNGE\r\n
            U0005 OK EXPUNGE Completed\r\n
            -------------------------------------------------------------------------------
            */

            UString line;
            UString x = buffer.substr(0U, (uint32_t)end);
            UTokenizer tok(x, U_CRLF);

            while (tok.next(line, '\n')) // '\n'
               {
               U_ASSERT_EQUALS(line[0],'*')

               *_ret++ = strtol(line.c_pointer(2), 0, 0);
               }
            }

         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

bool UImapClient::search(int* _ret, const UString& spec, const char* charSet, bool usingUID)
{
   U_TRACE(0, "UImapClient::search(%p,%.*S,%S,%b)", _ret, U_STRING_TO_TRACE(spec), charSet, usingUID)

   U_INTERNAL_ASSERT_POINTER(_ret)
   U_ASSERT(spec.empty() == false)
   U_INTERNAL_ASSERT_POINTER(charSet)

   if (state == SELECTED)
      {
      if (syncCommand("%s SEARCH %s %.*s",
               (usingUID ? "UID" : ""),
               charSet,
               U_STRING_TO_TRACE(spec)))
         {
         setEnd();

         /* Example
         -------------------------------------------------------------------------------
         U0005 SEARCH TEXT "string not in mailbox"\r\n
         -------------------------------------------------------------------------------
         * SEARCH 2 84 882\r\n
         U0005 OK SEARCH Completed\r\n
         -------------------------------------------------------------------------------
         */

         U_ASSERT_EQUALS(buffer[0],'*')

         UString word;
         UString x = buffer.substr(sizeof("* SEARCH"), end);
         UTokenizer tok(x);

         while (tok.next(word, ' '))
            {
            *_ret++ = strtol(word.data(), 0, 0);
            }

         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

bool UImapClient::fetch(UVector<UString>& vec, int start, int _end, const UString& spec, bool usingUID)
{
   U_TRACE(0, "UImapClient::fetch(%p,%d,%d,%.*S,%b)", &vec, start, _end, U_STRING_TO_TRACE(spec), usingUID)

   U_ASSERT(spec.empty() == false)

   if (state == SELECTED)
      {
      if (syncCommand("%s FETCH %d:%d %.*s",
               (usingUID ? "UID" : ""),
               start, _end,
               U_STRING_TO_TRACE(spec)))
         {
         setEnd();

         /* Example
         -------------------------------------------------------------------------------
         U0005 FETCH 2:4 (FLAGS BODY[HEADER.FIELDS (DATE FROM)])\r\n
         -------------------------------------------------------------------------------
         * 2 FETCH ....
         * 3 FETCH ....
         * 4 FETCH ....
         U0005 OK FETCH Completed\r\n
         -------------------------------------------------------------------------------
         */

         U_ASSERT_EQUALS(buffer[0],'*')

         bool bgroup;
         UString data;
         UTokenizer tok(buffer);
         const char* ptr1 = buffer.data();
         const char* ptr2 = buffer.c_pointer(_end);

         tok.setGroup("()");

         while (true)
            {
            ptr1 += sizeof("* FETCH");

            if  (ptr1 > ptr2) break;

            while (*ptr1 != '(') ++ptr1;

            tok.setPointer(ptr1);

            if (tok.next(data, &bgroup) && bgroup) vec.push_back(data);

            ptr1 += data.size();
            }

         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

bool UImapClient::setFlags(int start, int _end, int style, int _flags, bool usingUID)
{
   U_TRACE(0, "UImapClient::setFlags(%d,%d,%d,%B,%b)", start, _end, style, _flags, usingUID)

   if (state == SELECTED)
      {
      if (syncCommand("%s STORE %d:%d %sFLAGS.SILENT (%s%s%s%s%s%s)",
               (usingUID ? "UID" : ""),
               start, _end,
               (style & ADD       ? "+"           :
                style & REMOVE    ? "-"           : ""),
               (_flags & SEEN     ? "\\Seen"      : ""),
               (_flags & ANSWERED ? " \\Answered" : ""),
               (_flags & FLAGGED  ? " \\Flagged"  : ""),
               (_flags & DELETED  ? " \\Deleted"  : ""),
               (_flags & DRAFT    ? " \\Draft"    : ""),
               (_flags & RECENT   ? " \\Recent"   : "")))
         {
         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

bool UImapClient::copy(int start, int _end, const UString& to, bool usingUID)
{
   U_TRACE(0, "UImapClient::copy(%d,%d,%.*S,%b)", start, _end, U_STRING_TO_TRACE(to), usingUID)

   U_ASSERT(to.empty() == false)

   if (state == SELECTED)
      {
      if (syncCommand("%s COPY %d:%d %.*s",
               (usingUID ? "UID" : ""),
               start, _end,
               U_STRING_TO_TRACE(to)))
         {
         U_RETURN(true);
         }
      }

   response = IMAP_SESSION_BAD;

   U_RETURN(false);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UImapClient::dump(bool reset) const
{
   UTCPSocket::dump(false);

   *UObjectIO::os << '\n'
                  << "end                           " << end              << '\n'
                  << "state                         " << state            << '\n'
                  << "response                      " << response         << '\n'
                  << "capa            (UString      " << (void*)&capa     << ")\n"
                  << "buffer          (UString      " << (void*)&buffer   << ")\n"
                  << "selected        (UString      " << (void*)&selected << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
