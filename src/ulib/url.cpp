// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    url.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/url.h>

UString* Url::str_ftp;
UString* Url::str_smtp;
UString* Url::str_pop3;
UString* Url::str_http;
UString* Url::str_https;

void Url::str_allocate()
{
   U_TRACE(0, "Url::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_ftp,0)
   U_INTERNAL_ASSERT_EQUALS(str_smtp,0)
   U_INTERNAL_ASSERT_EQUALS(str_pop3,0)
   U_INTERNAL_ASSERT_EQUALS(str_http,0)
   U_INTERNAL_ASSERT_EQUALS(str_https,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("ftp") },
      { U_STRINGREP_FROM_CONSTANT("smtp") },
      { U_STRINGREP_FROM_CONSTANT("pop3") },
      { U_STRINGREP_FROM_CONSTANT("http") },
      { U_STRINGREP_FROM_CONSTANT("https") }
   };

   U_NEW_ULIB_OBJECT(str_ftp,   U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_smtp,  U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_pop3,  U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_http,  U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_https, U_STRING_FROM_STRINGREP_STORAGE(4));
}

UString Url::getService()
{
   U_TRACE(0, "Url::getService()")

   UString srv;

   if (service_end > 0)
      {
      srv = url.substr(0U, (uint32_t)service_end);
      }

   U_RETURN_STRING(srv);
}

void Url::setService(const char* service, uint32_t n)
{
   U_TRACE(0, "Url::setService(%S,%u)", service, n)

   U_INTERNAL_ASSERT_POINTER(service)

   if (service_end > 0)
      {
      (void) url.replace(0, service_end, service, n);
      }
   else
      {
      (void) url.insert(0, U_CONSTANT_TO_PARAM("://"));
      (void) url.insert(0, service, n);
      }

   findpos();
}

UString Url::getUser()
{
   U_TRACE(0, "Url::getUser()")

   UString usr;

   if (user_begin < user_end)
      {
      usr = url.substr(user_begin, user_end - user_begin);
      }

   U_RETURN_STRING(usr);
}

bool Url::setUser(const char* user, uint32_t n)
{
   U_TRACE(0, "Url::setUser(%S,%u)", user, n)

   U_INTERNAL_ASSERT_POINTER(user)

   // Only posible if there is a url

   if (host_begin < host_end)
      {
      if (user_begin < user_end)
         {
         (void) url.replace(user_begin, user_end - user_begin, user, n);
         }
      else
         {
         (void) url.insert(user_begin, 1, '@');
         (void) url.insert(user_begin, user, n);
         }

      findpos();

      U_RETURN(true);
      }

   U_RETURN(false);
}

UString Url::getHost()
{
   U_TRACE(0, "Url::getHost()")

   UString host;

   if (host_begin < host_end) host = url.substr(host_begin, host_end - host_begin);

   U_RETURN_STRING(host);
}

void Url::setHost(const char* host_, uint32_t n)
{
   U_TRACE(0, "Url::setHost(%S,%u)", host_, n)

   U_INTERNAL_ASSERT_POINTER(host_)

   if (host_begin < host_end)
      {
      (void) url.replace(host_begin, host_end - host_begin, host_, n);
      }
   else
      {
      (void) url.insert(host_begin, host_, n);
      }

   findpos();
}

uint32_t Url::getPort(char* buffer, uint32_t size)
{
   U_TRACE(0, "Url::getPort(%p,%u)", buffer, size)

   U_INTERNAL_ASSERT_MAJOR(size,5)
   U_INTERNAL_ASSERT_POINTER(buffer)

   if (host_end < path_begin)
      {
      size = path_begin - host_end - 1;

      if (size <= 5)
         {
         (void) url.copy(buffer, size, host_end + 1);

         buffer[size] = '\0';
         }

      U_RETURN(size);
      }

   U_RETURN(0);
}

int Url::getPort()
{
   U_TRACE(0, "Url::getPort()")

   char buffer[6] = { '\0', '\0', '\0', '\0', '\0', '\0' };

   if (getPort(buffer, sizeof(buffer)))
      {
      int port = atoi(buffer);

      U_RETURN(port);
      }

   if (service_end > 0)
      {
      UString tmp = url.substr(0U, (uint32_t)service_end);

      if (tmp.equal(*str_http))  U_RETURN(80);
      if (tmp.equal(*str_https)) U_RETURN(443);
      if (tmp.equal(*str_ftp))   U_RETURN(21);
      if (tmp.equal(*str_smtp))  U_RETURN(25);
      if (tmp.equal(*str_pop3))  U_RETURN(110);
      }

   U_RETURN(-1);
}

bool Url::setPort(uint32_t port)
{
   U_TRACE(0, "Url::setPort(%u)", port)

   char buffer[10];

   // Only posible if there is a url

   if ((port <= 0xFFFF) && (host_begin < host_end))
      {
      buffer[0] = ':';

      (void) sprintf(buffer + 1, "%u", port);

      (void) url.replace(host_end, path_begin - host_end, buffer);

      findpos();

      U_RETURN(true);
      }

   U_RETURN(false);
}

UString Url::getPath()
{
   U_TRACE(0, "Url::getPath()")

   UString path(U_CAPACITY);

   if (path_begin < path_end)
      {
      uint32_t copy_size = path_end - path_begin;

      path.reserve(copy_size);

      decode(url.c_pointer(path_begin), copy_size, path);
      }
   else
      {
      path.push_back('/');
      }

   U_RETURN_STRING(path);
}

void Url::setPath(const char* path, uint32_t n)
{
   U_TRACE(0, "Url::setPath(%S,%u)", path, n)

   U_INTERNAL_ASSERT_POINTER(path)

   if (path_begin < path_end)
      {
      if (*path != '/')
         {
         ++path_begin;
         }

      (void) url.replace(path_begin, path_end - path_begin, path, n);
      }
   else
      {
      if (*path != '/')
         {
         (void) url.insert(path_begin, 1, '/');

         ++path_begin;
         }

      (void) url.insert(path_begin, path, n);
      }

   findpos();
}

UString Url::getQuery()
{
   U_TRACE(0, "Url::getQuery()")

   UString query;
   int end = url.size() - 1;

   if (path_end < end) query = url.substr(path_end + 1, end - path_end);

   U_RETURN_STRING(query);
}

bool Url::setQuery(const char* query_, uint32_t n)
{
   U_TRACE(0, "Url::setQuery(%S,%u)", query_, n)

   U_INTERNAL_ASSERT_POINTER(query_)

   if (prepeare_for_query())
      {
      if (*query_ == '?') ++query_;

      (void) url.replace(path_end + 1, url.size() - path_end - 1, query_, n);

      U_RETURN(true);
      }

   U_RETURN(false);
}

UString Url::getFile()
{
   U_TRACE(0, "Url::getFile()")

   UString file  = getPath(),
           query = getQuery();

   if (query.empty() == false)
      {
      file.push_back('?');
      file.append(query);
      }

   U_RETURN_STRING(file);
}

void Url::findpos()
{
   U_TRACE(0, "Url::findpos()")

   int temp;

   service_end = U_STRING_FIND(url,0,"//");

   if (service_end < 0)
      {
      service_end = 0;
      user_begin  = service_end;
      }
   else
      {
      user_begin = service_end + 2;

      --service_end;
      }

   path_begin = url.find('/', user_begin);

   if (path_begin < 0)
      {
      path_begin = url.size();
      }

   if (service_end == 0 && path_begin && (url.at(path_begin-1) == ':'))
      {
      temp = url.find('.');

      if ((temp < 0) || (temp > path_begin))
         {
         service_end = path_begin - 1;
         user_begin  = service_end;
         user_end    = user_begin;
         }
      }

   user_end = url.find('@', user_begin);

   if ((user_end < 0) || (user_end > path_begin))
      {
      user_end   = user_begin;
      host_begin = user_end;
      }
   else
      {
      host_begin = user_end + 1;
      }

   // find ipv6 adresses

   temp = url.find('[', host_begin);

   if ((temp >= 0) && (temp < path_begin))
      {
      host_end = url.find(']', temp);

      if (host_end < path_begin) ++host_end;
      else                         host_end = host_begin;
      }
   else
      {
      host_end = url.find(':', host_begin);

      if ((host_end < 0) || (host_end > path_begin)) host_end = path_begin;
      }

   path_end = url.find('?', path_begin);

   if (path_end < path_begin) path_end = url.size();

   query = path_end;
}

U_NO_EXPORT bool Url::prepeare_for_query()
{
   U_TRACE(0, "Url::prepeare_for_query()")

   // Only posible if there is a url

   if (host_begin < host_end)
      {
      if (path_begin == path_end)
         {
         (void) url.insert(path_begin, 1, '/');

         ++path_end;
         }

      if (path_end == (int)url.size()) url.push_back('?');

      U_RETURN(true);
      }

   U_RETURN(false);
}

void Url::addQuery(const char* entry, const char* value)
{
   U_TRACE(0, "Url::addQuery(%S,%S)", entry, value)

   if (prepeare_for_query() && entry)
      {
      uint32_t v_size = 0, b_size = strlen(entry), e_size = b_size;

      if (value) v_size = strlen(value);

      if (e_size < v_size) b_size = v_size;

      b_size *= 3;
      char buffer[b_size];

      uint32_t result = u_url_encode((const unsigned char*)entry, e_size, (unsigned char*)buffer, 0);

      if (*url.rbegin() != '?') url.push_back('&');

      url.append(buffer, result);

      if (value)
         {
         url.push_back('=');

         result = u_url_encode((const unsigned char*)value, v_size, (unsigned char*)buffer, U_RFC2231);

         url.append(buffer, result);
         }
      }
}

U_NO_EXPORT bool Url::next_query_pos(int& entry_start, int& entry_end, int& value_start, int& value_end)
{
   U_TRACE(0, "Url::next_query_pos(%p,%p,%p,%p)", &entry_start, &entry_end, &value_start, &value_end)

   int end = url.size();

   if ((query < end) && (query >= path_end))
      {
      entry_start = ++query;

      value_end = url.find_first_of("&?", entry_start, 2);

      if (value_end < 0) query = value_end = end;
      else               query = value_end;

      value_start = url.find('=', entry_start);

      if ((value_start < entry_start) || (value_start > value_end))
         {
         value_start = entry_end = value_end;
         }
      else
         {
         entry_end = value_start;

         ++value_start;
         }

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool Url::firstQuery(UString& entry, UString& value)
{
   U_TRACE(0, "Url::firstQuery(%.*S,%.*S)", U_STRING_TO_TRACE(entry), U_STRING_TO_TRACE(value))

   if (path_end < (int)url.size())
      {
      query = path_end;

      return nextQuery(entry, value);
      }

   U_RETURN(false);
}

bool Url::nextQuery(UString& entry, UString& value)
{
   U_TRACE(0, "Url::nextQuery(%.*S,%.*S)", U_STRING_TO_TRACE(entry), U_STRING_TO_TRACE(value))

   int entry_start, entry_end, value_start, value_end;

   if (next_query_pos(entry_start, entry_end, value_start, value_end))
      {
      decode(url.c_pointer(entry_start), entry_end - entry_start, entry);
      decode(url.c_pointer(value_start), value_end - value_start, value);

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool Url::findQuery(UString& entry, UString& value)
{
   U_TRACE(0, "Url::findQuery(%.*S,%.*S)", U_STRING_TO_TRACE(entry), U_STRING_TO_TRACE(value))

   if (path_end < (int)url.size())
      {
      query = path_end;

      UString e(1000), v(1000);

      while (nextQuery(e, v))
         {
         if (entry == e ||
             value == v)
            {
            entry = e;
            value = v;

            U_RETURN(true);
            }
         }
      }

   U_RETURN(false);
}

// STREAM

U_EXPORT istream& operator>>(istream& is, Url& u)
{
   U_TRACE(0+256, "Url::operator>>(%p,%p)", &is, &u)

   is >> u.url;

   return is;
}

U_EXPORT ostream& operator<<(ostream& os, const Url& u)
{
   U_TRACE(0+256, "Url::operator<<(%p,%p)", &os, &u)

   os << u.url;

   return os;
}

// DEBUG

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* Url::dump(bool reset) const
{
   *UObjectIO::os << "service_end  " << service_end << '\n'
                  << "user_begin   " << user_begin  << '\n'
                  << "user_end     " << user_end    << '\n'
                  << "host_begin   " << host_begin  << '\n'
                  << "host_end     " << host_end    << '\n'
                  << "path_begin   " << path_begin  << '\n'
                  << "path_end     " << path_end    << '\n'
                  << "query        " << query       << '\n'
                  << "url (UString " << (void*)&url << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
