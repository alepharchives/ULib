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
#include <ulib/utility/string_ext.h>

const UString* Url::str_ftp;
const UString* Url::str_ldap;
const UString* Url::str_ldaps;
const UString* Url::str_smtp;
const UString* Url::str_pop3;
const UString* Url::str_http;
const UString* Url::str_https;

void Url::str_allocate()
{
   U_TRACE(0, "Url::str_allocate()")

   U_INTERNAL_ASSERT_EQUALS(str_ftp,0)
   U_INTERNAL_ASSERT_EQUALS(str_ldap,0)
   U_INTERNAL_ASSERT_EQUALS(str_ldaps,0)
   U_INTERNAL_ASSERT_EQUALS(str_smtp,0)
   U_INTERNAL_ASSERT_EQUALS(str_pop3,0)
   U_INTERNAL_ASSERT_EQUALS(str_http,0)
   U_INTERNAL_ASSERT_EQUALS(str_https,0)

   static ustringrep stringrep_storage[] = {
      { U_STRINGREP_FROM_CONSTANT("ftp") },
      { U_STRINGREP_FROM_CONSTANT("ldap") },
      { U_STRINGREP_FROM_CONSTANT("ldaps") },
      { U_STRINGREP_FROM_CONSTANT("smtp") },
      { U_STRINGREP_FROM_CONSTANT("pop3") },
      { U_STRINGREP_FROM_CONSTANT("http") },
      { U_STRINGREP_FROM_CONSTANT("https") }
   };

   U_NEW_ULIB_OBJECT(str_ftp,   U_STRING_FROM_STRINGREP_STORAGE(0));
   U_NEW_ULIB_OBJECT(str_ldap,  U_STRING_FROM_STRINGREP_STORAGE(1));
   U_NEW_ULIB_OBJECT(str_ldaps, U_STRING_FROM_STRINGREP_STORAGE(2));
   U_NEW_ULIB_OBJECT(str_smtp,  U_STRING_FROM_STRINGREP_STORAGE(3));
   U_NEW_ULIB_OBJECT(str_pop3,  U_STRING_FROM_STRINGREP_STORAGE(4));
   U_NEW_ULIB_OBJECT(str_http,  U_STRING_FROM_STRINGREP_STORAGE(5));
   U_NEW_ULIB_OBJECT(str_https, U_STRING_FROM_STRINGREP_STORAGE(6));
}

// gcc: call is unlikely and code size would grow

void Url::set(const UString& x)
{
   U_TRACE(0, "Url::set(%.*S)", U_STRING_TO_TRACE(x))

   url = x;

   findpos();
}

UString Url::getService() const
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

      if (size &&
          size <= 5)
         {
         (void) url.copy(buffer, size, host_end + 1);

         buffer[size] = '\0';

         U_RETURN(size);
         }
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
      if (tmp.equal(*str_ldap))  U_RETURN(389);
      if (tmp.equal(*str_ldaps)) U_RETURN(636);
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

UString Url::getPath()
{
   U_TRACE(0, "Url::getPath()")

   UString path(U_CAPACITY);

   if (path_begin < path_end) decode(url.c_pointer(path_begin), path_end - path_begin, path);
   else                       path.push_back('/');

   U_RETURN_STRING(path);
}

UString Url::getQuery()
{
   U_TRACE(0, "Url::getQuery()")

   int _end = url.size() - 1;

   if (path_end < _end)
      {
      uint32_t sz = _end - path_end;

      UString _query(sz);

      decode(url.c_pointer(path_end + 1), sz, _query);

      U_RETURN_STRING(_query);
      }

   U_RETURN_STRING(UString::getStringNull());
}

uint32_t Url::getQuery(UVector<UString>& vec)
{
   U_TRACE(0, "Url::getQuery(%p)", &vec)

   uint32_t n = vec.size(), result;

   UString _query = getQuery();

   if (_query.empty() == false) (void) UStringExt::getNameValueFromData(_query, vec, U_CONSTANT_TO_PARAM("&"), true);

   result = (vec.size() - n);

   U_RETURN(result);
}

bool Url::setQuery(const char* query_, uint32_t n)
{
   U_TRACE(0, "Url::setQuery(%S,%u)", query_, n)

   U_INTERNAL_ASSERT_POINTER(query_)

   if (prepeareForQuery())
      {
      if (*query_ == '?') ++query_;

      (void) url.replace(path_end + 1, url.size() - path_end - 1, query_, n);

      U_RETURN(true);
      }

   U_RETURN(false);
}

bool Url::setQuery(UVector<UString>& vec)
{
   U_TRACE(0, "Url::setQuery(%p)", &vec)

   U_INTERNAL_ASSERT_EQUALS(vec.empty(),false)

   if (prepeareForQuery())
      {
      UString name, value;

      for (int32_t i = 0, n  = vec.size(); i < n; ++i)
         {
         name  = vec[i++];
         value = vec[i];

         addQuery(U_STRING_TO_PARAM(name), U_STRING_TO_PARAM(value));
         }

      U_RETURN(true);
      }

   U_RETURN(false);
}

UString Url::getFile()
{
   U_TRACE(0, "Url::getFile()")

   UString file;

   if (path_begin < path_end) file = url.substr(path_begin);
   else                       file.push_back('/');

   U_RETURN_STRING(file);
}

void Url::findpos()
{
   U_TRACE(0, "Url::findpos()")

   // proto://[user[:password]@]hostname[:port]/[path]?[query]

   service_end = U_STRING_FIND(url, 0, "//");

   if (service_end < 0)
      {
      service_end = user_begin = user_end = host_begin = host_end = path_begin = path_end = query = 0;

      return;
      }

   U_INTERNAL_ASSERT(u_isURL(U_STRING_TO_PARAM(url)))

   user_begin = service_end + 2;

   --service_end; // cut ':'

   path_begin = url.find('/', user_begin);

   if (path_begin < 0)
      {
      path_begin = url.find('?', user_begin);

      if (path_begin < 0) path_begin = url.size();
      }

   int temp;

   if (service_end == 0 &&
       path_begin       &&
       (url.c_char(path_begin-1) == ':'))
      {
      temp = url.find('.');

      if (temp < 0 ||
          temp > path_begin)
         {
         service_end = path_begin - 1;
         user_begin  = service_end;
         user_end    = user_begin;
         }
      }

   user_end = url.find('@', user_begin);

   if (user_end < 0 ||
       user_end > path_begin)
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

   if (temp >= 0 &&
       temp < path_begin)
      {
      host_end = url.find(']', temp);

      if (host_end < path_begin) ++host_end;
      else                         host_end = host_begin;
      }
   else
      {
      host_end = url.find(':', host_begin);

      if (host_end < 0 ||
          host_end > path_begin)
         {
         host_end = path_begin;
         }
      }

   path_end = url.find('?', path_begin);

   if (path_end < path_begin) path_end = url.size();

   query = path_end;

   U_INTERNAL_DUMP("service_end = %d user_begin = %d user_end = %d host_begin = %d host_end = %d path_begin = %d path_end = %d query = %d",
                    service_end,     user_begin,     user_end,     host_begin,     host_end,     path_begin,     path_end,     query)
}

U_NO_EXPORT bool Url::prepeareForQuery()
{
   U_TRACE(0, "Url::prepeareForQuery()")

   // NB: Only posible if there is a url

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

void Url::addQuery(const char* entry, uint32_t entry_len, const char* value, uint32_t value_len)
{
   U_TRACE(0, "Url::addQuery(%.*S,%u,%.*S,%u)", entry_len, entry, entry_len, value_len, value, value_len)

   U_INTERNAL_ASSERT_POINTER(entry)

   if (prepeareForQuery())
      {
      uint32_t v_size = 0, b_size = entry_len, e_size = b_size;

      if (value) v_size = value_len;

      if (e_size < v_size) b_size = v_size;

      b_size *= 3;

      UString buffer(b_size);

      encode(entry, e_size, buffer);

      if (*url.rbegin() != '?') url.push_back('&');

      (void) url.append(buffer);

      if (value)
         {
         url.push_back('=');

         encode(value, v_size, buffer, U_RFC2231);

         (void) url.append(buffer);
         }
      }
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
