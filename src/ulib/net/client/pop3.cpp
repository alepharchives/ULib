// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    pop3.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/net/client/pop3.h>
#include <ulib/utility/socket_ext.h>

#define U_POP3_OK    "+OK"
#define U_POP3_ERR   "-ERR"
#define U_POP3_EOD   ".\r\n"
#define U_POP3_EODML "\r\n.\r\n"

/*
#define TEST
*/

#ifdef TEST
#  include <ulib/file.h>
#endif

UPop3Client::~UPop3Client()
{
   U_TRACE_UNREGISTER_OBJECT(0, UPop3Client)
}

U_NO_EXPORT const char* UPop3Client::status()
{
   U_TRACE(0, "UPop3Client::status()")

   const char* descr1;
   const char* descr2;

   switch (state)
      {
      case INIT:                 descr1 = "INIT";           break;
      case AUTHORIZATION:        descr1 = "AUTHORIZATION";  break;
      case TRANSACTION:          descr1 = "TRANSACTION";    break;
      case UPDATE:               descr1 = "UPDATE";         break;
      default:                   descr1 = "???";            break;
      }

   switch (response)
      {
      case OK:                   descr2 = "OK";                      break;
      case BAD_STATE:            descr2 = "bad state";               break;
      case UNAUTHORIZED:         descr2 = "not authenticated";       break;
      case CANT_LIST:            descr2 = "LIST error";              break;
      case NO_SUCH_MESSAGE:      descr2 = "no such message";         break;
      case CAPA_NOT_SUPPORTED:   descr2 = "CAPA not supported";      break;
      case STLS_NOT_SUPPORTED:   descr2 = "STARTTLS not supported";  break;
      case UIDL_NOT_SUPPORTED:   descr2 = "UIDL not supported";      break;
      default:                   descr2 = "???";                     break;
      }

   static char _buffer[128];

   (void) sprintf(_buffer, "%s - (%d, %s)", descr1, response, descr2);

   U_RETURN(_buffer);
}

bool UPop3Client::_connectServer(const UString& server, int port, uint32_t timeoutMS)
{
   U_TRACE(0, "UPop3Client::_connectServer(%.*S,%d,%u)", U_STRING_TO_TRACE(server), port, timeoutMS)

#ifdef USE_LIBSSL
   U_INTERNAL_ASSERT(Socket::isSSL())
   ((USSLSocket*)this)->setActive(false);
#endif

   if (Socket::connectServer(server, port))
      {
      (void) USocket::setTimeoutRCV(timeoutMS);

      if (USocketExt::readLineReply(this, buffer) > 0 &&
          U_STRNEQ(buffer.data(), U_POP3_OK))
         {
         state    = AUTHORIZATION;
         response = OK;

         U_DUMP("status() = %S", status())

         U_RETURN(true);
         }
      }

   U_RETURN(false);
}

// Send a command to the POP3 server and wait for a response eventually with eod or size fixed...

U_NO_EXPORT bool UPop3Client::syncCommand(int eod, const char* format, ...)
{
   U_TRACE(0, "UPop3Client::syncCommand(%d,%S)", eod, format)

   U_DUMP("status() = %S", status())

   va_list argp;
   va_start(argp, format);

   end = USocketExt::vsyncCommand(this, buffer.data(), buffer.capacity(), format, argp);

   va_end(argp);

   buffer.size_adjust(U_max(end,0));

   if (U_STRNCMP(buffer.data(), U_POP3_OK)) U_RETURN(false);

   if (eod != -1)
      {
      pos = buffer.find('\n') + 1;

      if (eod)
         {
         // adjust how many bytes read...

         eod += (U_CONSTANT_SIZE(U_POP3_EOD) - (buffer.size() - pos));

         if (eod > 0)
            {
            bool esito = USocketExt::read(this, buffer, eod);

            if (esito == false) U_RETURN(false);
            }

         end = buffer.size() - U_CONSTANT_SIZE(U_POP3_EOD);
         }
      else
         {
         end = U_STRING_FIND(buffer, pos, U_POP3_EOD);

         if (end == (int)U_NOT_FOUND)
            {
            end = USocketExt::readWhileNotToken(this, buffer, U_CONSTANT_TO_PARAM(U_POP3_EOD));

            if (end == (int)U_NOT_FOUND) U_RETURN(false);
            }
         }

      U_INTERNAL_DUMP("pos = %d end = %d", pos, end)
      }

   response = OK;

   U_RETURN(true);
}

U_NO_EXPORT bool UPop3Client::syncCommandML(const UString& req, int* vpos, int* vend)
{
   U_TRACE(0, "UPop3Client::syncCommandML(%.*S,%p,%p)", U_STRING_TO_TRACE(req), vpos, vend)

   U_DUMP("status() = %S", status())

   uint32_t sz = req.size();

   end = Socket::send(req.data(), sz);

   if (USocket::checkIO(end) == false) U_RETURN(false);

   bool ok;
   int i = 0;

   pos = end = 0;
   buffer.setEmpty();

   if (vpos)
      {
      (void) memset(vpos, 0xff, num_msg * sizeof(int));
      (void) memset(vend, 0xff, num_msg * sizeof(int));
      }

   do {
      ok = USocketExt::read(this, buffer);

      if (ok == false) U_RETURN(false);

      U_INTERNAL_DUMP("buffer.size() = %u", buffer.size())

      do {
         if (buffer.size() < (uint32_t)(end + U_CONSTANT_SIZE(U_POP3_OK))) break;

      // end = U_STRING_FIND(buffer, end, U_POP3_OK);

         U_INTERNAL_ASSERT(U_STRNEQ(buffer.c_pointer(end), U_POP3_OK))

         if (vpos)
            {
            if (vpos[i] == (int)U_NOT_FOUND)
               {
               vpos[i] = buffer.find('\n', end);

               if (vpos[i] == (int)U_NOT_FOUND) break;

               vpos[i] += 1;
               }

            vend[i] = U_STRING_FIND(buffer, end, U_POP3_EODML);

            U_INTERNAL_DUMP("vpos[%i] = %d vend[%i] = %d", i, vpos[i], i, vend[i])

            if (vend[i] == (int)U_NOT_FOUND) break;

            vend[i] += U_CONSTANT_SIZE(U_CRLF);

            end = vend[i] + U_CONSTANT_SIZE(U_POP3_EOD);
            }
         else
            {
            pos = buffer.find('\n', end);

            if (pos == (int)U_NOT_FOUND) break;

            end = ++pos;
            }

         if (++i == num_msg)
            {
#        ifdef TEST
            UFile::writeTo("pop3.data", buffer);
#        endif

            U_INTERNAL_ASSERT_EQUALS((uint32_t)end,buffer.size())

            goto end;
            }
         }
      while (true);
      }
   while (true);

end:
   response = OK;

   U_RETURN(true);
}

bool UPop3Client::startTLS()
{
   U_TRACE(0, "UPop3Client::startTLS()")

#ifdef USE_LIBSSL
   U_INTERNAL_ASSERT(Socket::isSSL())

        if (state != AUTHORIZATION) response = BAD_STATE;
   else if (syncCommand(-1, "STLS"))
      {
          ((USSLSocket*)this)->setActive(true);
      if (((USSLSocket*)this)->secureConnection(USocket::iSockDesc)) U_RETURN(true);
          ((USSLSocket*)this)->setActive(false);

      if (USocketExt::readLineReply(this, buffer) > 0 &&
          U_STRNEQ(buffer.data(), U_POP3_ERR))
         {
         response = STLS_NOT_SUPPORTED;

         U_DUMP("status() = %S", status())
         }
      }
#endif

   U_RETURN(false);
}

bool UPop3Client::login(const char* user, const char* passwd)
{
   U_TRACE(0, "UPop3Client::login(%S,%S)", user, passwd)

   if (state != AUTHORIZATION) response = BAD_STATE;
   else
      {
      if (syncCommand(-1, "USER %s", user) &&
          syncCommand(-1, "PASS %s", passwd))
         {
         state           = TRANSACTION;
         USocket::iState = USocket::LOGIN;

         U_RETURN(true);
         }

      response = UNAUTHORIZED;
      }

   U_RETURN(false);
}

int UPop3Client::getCapabilities(UVector<UString>& vec)
{
   U_TRACE(0, "UPop3Client::getCapabilities(%p)", &vec)

   if (syncCommand(0, "CAPA"))
      {
      (void) capa.replace(0, capa.size(), buffer, pos, end - pos);

      uint32_t n = vec.split(capa, U_CRLF);

      U_RETURN(n);
      }

   response = CAPA_NOT_SUPPORTED;

   U_RETURN(-1);
}

int UPop3Client::getUIDL(UVector<UString>& vec)
{
   U_TRACE(0, "UPop3Client::getUIDL(%p)", &vec)

   if (state != TRANSACTION)
      {
      response = BAD_STATE;
      }
   else if (syncCommand(0, "UIDL"))
      {
      UString r;
      const char* p;
      uint32_t  n      = vec.size();
      const char* s    = buffer.c_pointer(pos);
      const char* _end = buffer.c_pointer(end);

      while (s < _end)
         {
         // skip white space

         if (u__isspace(*s))
            {
            ++s;

            continue;
            }

         s = u_delimit_token(s, &p, _end, 0, 0); // n-esimo
         s = u_delimit_token(s, &p, _end, 0, 0); // uidl

         r = UString((void*)p, s - p);

         vec.push(r);

         ++s;
         }

      n = vec.size() - n;

      U_RETURN(n);
      }
   else
      {
      response = UIDL_NOT_SUPPORTED;
      }

   U_RETURN(-1);
}

int UPop3Client::getSizeMessage(uint32_t n)
{
   U_TRACE(0, "UPop3Client::getSizeMessage(%u)", n)

   if (state == TRANSACTION)
      {
      if ((n ? syncCommand(-1, "LIST %u", n) :
               syncCommand(-1, "STAT")))
         {
         char* ptr = buffer.c_pointer(sizeof(U_POP3_OK));

         num_msg      = strtol(ptr, (char**)&ptr, 10);
         int size_msg = atoi(ptr);

         U_INTERNAL_DUMP("num_msg = %d size_msg = %d", num_msg, size_msg)

         U_RETURN(size_msg);
         }

      response = CANT_LIST;
      }
   else
      {
      response = BAD_STATE;
      }

   U_RETURN(-1);
}
 
// Execute an pop3 session

UString UPop3Client::getHeader(uint32_t n)
{
   U_TRACE(0, "UPop3Client::getHeader(%u)", n)

   if (state == TRANSACTION)
      {
      if (syncCommand(0, "TOP %u 0", n))
         {
         UString result((void*)buffer.c_pointer(pos), end - pos);

         U_RETURN_STRING(result);
         }

      response = NO_SUCH_MESSAGE;
      }
   else
      {
      response = BAD_STATE;
      }

   U_RETURN_STRING(UString::getStringNull());
}

UString UPop3Client::getMessage(uint32_t n)
{
   U_TRACE(0, "UPop3Client::getMessage(%u)", n)

   int size_msg = getSizeMessage(n);

   if (size_msg > 0)
      {
      (void) buffer.reserve(size_msg);

      if (syncCommand(size_msg, "RETR %u", n))
         {
         UString result((void*)buffer.c_pointer(pos), size_msg);

         U_RETURN_STRING(result);
         }

      response = NO_SUCH_MESSAGE;
      }

   U_RETURN_STRING(UString::getStringNull());
}

bool UPop3Client::deleteMessage(uint32_t n)
{
   U_TRACE(0, "UPop3Client::deleteMessage(%u)", n)

   if (state == TRANSACTION)
      {
      if (syncCommand(-1, "DELE %u", n)) U_RETURN(true);

      response = NO_SUCH_MESSAGE;
      }
   else
      {
      response = BAD_STATE;
      }

   U_RETURN(false);
}

// PIPELINING

int UPop3Client::getAllHeader(UVector<UString>& vec)
{
   U_TRACE(0, "UPop3Client::getAllHeader(%p)", &vec)

   int size_msg = getSizeMessage(0);

   if (size_msg > 0)
      {
      int i = 2;
      UString req(U_max(U_CAPACITY, 11U*num_msg));

                                req.snprintf(    "TOP  1 0\r\n");
      for (; i <= num_msg; ++i) req.snprintf_add("TOP %d 0\r\n", i);

      (void) buffer.reserve(size_msg);

      int vpos[8192], vend[8192];

      if (syncCommandML(req, vpos, vend))
         {
         UString r;

         for (i = 0; i < num_msg; ++i)
            {
            r = UString((void*)buffer.c_pointer(vpos[i]), vend[i] - vpos[i]);

            vec.push(r);
            }

         U_RETURN(num_msg);
         }

      response = CANT_LIST;
      }

   U_RETURN(-1);
}

int UPop3Client::getAllMessage(UVector<UString>& vec)
{
   U_TRACE(0, "UPop3Client::getAllMessage(%p)", &vec)

   int size_msg = getSizeMessage(0);

   if (size_msg > 0)
      {
      int i = 2;
      UString req(U_max(U_CAPACITY, 10U*num_msg));

                                req.snprintf(    "RETR  1\r\n");
      for (; i <= num_msg; ++i) req.snprintf_add("RETR %d\r\n", i);

      (void) buffer.reserve(size_msg + (num_msg * (sizeof(U_POP3_OK) + sizeof("Message follows") + sizeof(U_POP3_EODML))));

      int vpos[8192], vend[8192];

      if (syncCommandML(req, vpos, vend))
         {
         UString r;

         for (i = 0; i < num_msg; ++i)
            {
            r = UString((void*)buffer.c_pointer(vpos[i]), vend[i] - vpos[i]);

            vec.push(r);
            }

         U_RETURN(num_msg);
         }

      response = CANT_LIST;
      }

   U_RETURN(-1);
}

bool UPop3Client::deleteAllMessage()
{
   U_TRACE(0, "UPop3Client::deleteAllMessage()")

   int size_msg = getSizeMessage(0);

   if (size_msg > 0)
      {
      int i = 2;
      UString req(U_max(U_CAPACITY, 10U*num_msg));
      uint32_t size = num_msg * (sizeof(U_POP3_OK) + sizeof(" message deleted"));

                                req.snprintf(    "DELE  1\r\n");
      for (; i <= num_msg; ++i) req.snprintf_add("DELE %d\r\n", i);

      (void) buffer.reserve(size);

      if (syncCommandML(req, 0, 0)) U_RETURN(true);
      }

   U_RETURN(false); 
}

bool UPop3Client::reset()
{
   U_TRACE(0, "UPop3Client::reset()")

   if (syncCommand(-1, "RSET"))
      {
      U_RETURN(true);
      }

   U_RETURN(false);
}

// Quit an pop3 session

bool UPop3Client::quit()
{
   U_TRACE(0, "UPop3Client::quit()")

   if (syncCommand(-1, "QUIT"))
      {
      state = UPDATE;

      U_RETURN(true);
      }

   U_RETURN(false);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UPop3Client::dump(bool _reset) const
{
   U_CHECK_MEMORY

   Socket::dump(false);

   *UObjectIO::os << '\n'
                  << "pos                           " << pos             << '\n'
                  << "end                           " << end             << '\n'
                  << "state                         " << state           << '\n'
                  << "num_msg                       " << num_msg         << '\n'
                  << "response                      " << response        << '\n'
                  << "capa            (UString      " << (void*)&capa    << ")\n"
                  << "buffer          (UString      " << (void*)&buffer  << ')';

   if (_reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
