// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    dbi.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/date.h>
#include <ulib/dbi/dbi.h>

UDBI::UDBI(const char* driverdir, const char* drivername)
{
   U_TRACE_REGISTER_OBJECT(0, UDBI, "%S,%S", driverdir, drivername)

   int ndrivers = U_SYSCALL(dbi_initialize, "%S", driverdir);

   U_INTERNAL_ASSERT_MAJOR(ndrivers,0)

   conn = U_SYSCALL(dbi_conn_new, "%S", drivername);

   U_INTERNAL_ASSERT_POINTER_MSG(conn, "DBI: failed to load backend...")

   (void) U_SYSCALL(dbi_conn_set_option, "%p,%S,%S", conn, "driver", drivername);
}

bool UDBI::setDirectory(const char* directory)
{
   U_TRACE(1, "UDBI::setDirectory(%S)", directory)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(conn)

   const char* backend = (const char*) U_SYSCALL(dbi_conn_get_option, "%p,%S", conn, "driver");

   U_INTERNAL_ASSERT_POINTER(backend)

   char buffer[U_PATH_MAX];

   (void) snprintf(buffer, U_PATH_MAX, "%s_dbdir", backend);

   if (U_SYSCALL(dbi_conn_set_option, "%p,%S,%S", conn, buffer, directory) == 0) U_RETURN(true);

#ifdef DEBUG
   U_DUMP("error = %s", getLastError())
#endif

   U_RETURN(false);
}

bool UDBI::connect(const char* dbName, const char* hostName, const char* username, const char* password)
{
   U_TRACE(1, "UDBI::connect(%S,%S,%S,%S)", dbName, hostName, username, password)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(conn)

   if (                  U_SYSCALL(dbi_conn_set_option, "%p,%S,%S", conn, "dbname",   dbName)   == 0  &&
       (hostName == 0 || U_SYSCALL(dbi_conn_set_option, "%p,%S,%S", conn, "host",     hostName) == 0) &&
       (username == 0 || U_SYSCALL(dbi_conn_set_option, "%p,%S,%S", conn, "username", username) == 0) &&
       (password == 0 || U_SYSCALL(dbi_conn_set_option, "%p,%S,%S", conn, "password", password) == 0) &&
                         U_SYSCALL(dbi_conn_connect,    "%p",       conn)                       == 0)
      {
      U_RETURN(true);
      }

#ifdef DEBUG
   U_DUMP("error = %s", getLastError())
#endif

   U_RETURN(false);
}

bool UDBI::reconnect()
{
   U_TRACE(1, "UDBI::reconnect()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(conn)

   const char* backend = (const char*) U_SYSCALL(dbi_conn_get_driver, "%p", conn);

   close();

   conn = U_SYSCALL(dbi_conn_new, "%S", backend);

   U_INTERNAL_ASSERT_POINTER_MSG(conn, "DBI: failed to load backend...")

   if (U_SYSCALL(dbi_conn_connect, "%p", conn) == 0) U_RETURN(true);

#ifdef DEBUG
   U_DUMP("error = %s", getLastError())
#endif

   U_RETURN(false);
}

void UDBI::escape()
{
   U_TRACE(0+256, "UDBI::escape()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(conn)

   for (; pos_read < query_in_len; ++pos_read)
      {
      if (query_in[pos_read] == '\'')
         {
         escaped_query._append('\'');

         ++pos_read;

         while (query_in[pos_read] != '\'' && pos_read != query_in_len)
            {
            escaped_query._append(query_in[pos_read]);

            ++pos_read;
            }

         if (pos_read == query_in_len) U_ERROR("DBI: unexpected end of query after \"'\"");

         escaped_query._append('\'');

         ++pos_read;

         continue;
         }

      if (query_in[pos_read] == '?')
         {
         ready_for_input = true;

         ++pos_read;

         break;
         }

      escaped_query._append(query_in[pos_read]);
      }

   escaped_query._append();

   U_INTERNAL_DUMP("escaped_query = %.*S", U_STRING_TO_TRACE(escaped_query))

   if (ready_for_input) return;

   if (pos_read == query_in_len)
      {
      complete = true;

      return;
      }

   U_ERROR("DBI: internal dbixx error");
}

void UDBI::bind(const null& v, bool isnull)
{
   U_TRACE(0, "UDBI::bind(%p,%b)", &v, isnull)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(conn)
   U_INTERNAL_ASSERT_MSG(ready_for_input, "DBI: more parameters given then inputs in query");

   (void) escaped_query.append(U_CONSTANT_TO_PARAM("NULL"));

   ready_for_input = false;

   escape();
}

void UDBI::bind(const UString& v, bool isnull)
{
   U_TRACE(1, "UDBI::bind(%.*S,%b)", U_STRING_TO_TRACE(v), isnull)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(conn)
   U_INTERNAL_ASSERT_MSG(ready_for_input, "DBI: more parameters given then inputs in query");

        if (isnull)    (void) escaped_query.append(U_CONSTANT_TO_PARAM("NULL"));
   else if (v.empty()) (void) escaped_query.append(U_CONSTANT_TO_PARAM("''"));
   else
      {
      char* new_str;

      const char* ptr = v.c_str();

      size_t sz = U_SYSCALL(dbi_conn_quote_string_copy, "%p,%S,%p", conn, ptr, &new_str);

      U_INTERNAL_ASSERT_MAJOR(sz,0)

      (void) escaped_query.append(new_str, sz);

      U_SYSCALL_VOID(free, "%p", new_str);
      }

   ready_for_input = false;

   escape();
}

void UDBI::bind(struct tm& v, bool isnull)
{
   U_TRACE(0, "UDBI::bind(%p,%b)", &v, isnull)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER(conn)
   U_INTERNAL_ASSERT_MSG(ready_for_input, "DBI: more parameters given then inputs in query");

   // The representation of a number in decimal form is more compact then in binary so it should be enough

   if (isnull) (void) escaped_query.append(U_CONSTANT_TO_PARAM("NULL"));
   else
      {
      char buffer[32];

      (void) strftime(buffer, sizeof(buffer), "'%Y-%m-%d %H:%M:%S'", &v);

      (void) escaped_query.append(buffer, strlen(buffer));
      }

   ready_for_input = false;

   escape();
}

void UDBI::exec()
{
   U_TRACE(1, "UDBI::exec()")

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER_MSG(conn, "DBI: backend is not open")
   U_INTERNAL_ASSERT_MSG(complete, "DBI: not all parameters are bind");

   const char* ptr = escaped_query.c_str();

   U_INTERNAL_ASSERT_POINTER_MSG(ptr, "DBI: no statement assigned")

   dbi_result res = (dbi_result) U_SYSCALL(dbi_conn_query, "%p,%S", conn, ptr);

   if (res)
      {
      affected_rows = U_SYSCALL(dbi_result_get_numrows_affected, "%p", res);

      U_SYSCALL_VOID(dbi_result_free, "%p", res);
      }
}

bool UDBIRow::fetch(int pos, long long& value)
{
   U_TRACE(1, "UDBIRow::fetch(%d,%p)", pos, &value)

   if (isnull(pos)) U_RETURN(false);

   int type = U_SYSCALL(dbi_result_get_field_type_idx, "%p,%d", res, pos);

   switch (type)
      {
      case DBI_TYPE_DECIMAL:
      case DBI_TYPE_INTEGER:
         value = (long long int) U_SYSCALL(dbi_result_get_longlong_idx, "%p,%d", res, pos);
      break;

      case DBI_TYPE_STRING:
         {
         char const* d = U_SYSCALL(dbi_result_get_string_idx, "%p,%d", res, pos);

         type = sscanf(d, "%lld", &value);

         U_INTERNAL_ASSERT_EQUALS_MSG(type,1,"DBI: bad cast to integer type")
         }
      break;

      default:
         {
         U_INTERNAL_ASSERT_MSG(false,"DBI: bad cast to integer type")

         U_RETURN(false);
         }
      break;
      }

   U_RETURN(true);
}

bool UDBIRow::fetch(int pos, unsigned long long& value)
{
   U_TRACE(1, "UDBIRow::fetch(%d,%p)", pos, &value)

   if (isnull(pos)) U_RETURN(false);

   int type = U_SYSCALL(dbi_result_get_field_type_idx, "%p,%d", res, pos);

   switch (type)
      {
      case DBI_TYPE_DECIMAL:
      case DBI_TYPE_INTEGER:
         value = U_SYSCALL(dbi_result_get_ulonglong_idx, "%p,%d", res, pos);
      break;

      case DBI_TYPE_STRING:
         {
         char const* d = U_SYSCALL(dbi_result_get_string_idx, "%p,%d", res, pos);

         type = sscanf(d, "%llu", &value);

         U_INTERNAL_ASSERT_EQUALS_MSG(type,1,"DBI: bad cast to integer type")
         }
      break;

      default:
         {
         U_INTERNAL_ASSERT_MSG(false,"DBI: bad cast to integer type")

         U_RETURN(false);
         }
      break;
      }

   U_RETURN(true);
}

bool UDBIRow::fetch(int pos, double& value)
{
   U_TRACE(1, "UDBIRow::fetch(%d,%p)", pos, &value)

   if (isnull(pos)) U_RETURN(false);

   int type = U_SYSCALL(dbi_result_get_field_type_idx, "%p,%d", res, pos);

   switch (type)
      {
      case DBI_TYPE_DECIMAL:
         {
         if (DBI_DECIMAL_SIZE8 &
             U_SYSCALL(dbi_result_get_field_attribs_idx, "%p,%d", res, pos)) value = U_SYSCALL(dbi_result_get_double_idx, "%p,%d", res, pos);
         else                                                                value = U_SYSCALL(dbi_result_get_float_idx,  "%p,%d", res, pos);
         }
      break;

      case DBI_TYPE_INTEGER:
         value = U_SYSCALL(dbi_result_get_longlong_idx, "%p,%d", res, pos);
      break;

      case DBI_TYPE_STRING:
         {
         char const* d = U_SYSCALL(dbi_result_get_string_idx, "%p,%d", res, pos);

         value = atof(d);
         }
      break;

      default:
         {
         U_INTERNAL_ASSERT_MSG(false,"DBI: bad cast to double type")

         U_RETURN(false);
         }
      break;
      }

   U_RETURN(true);
}

bool UDBIRow::fetch(int pos, struct tm& value)
{
   U_TRACE(1, "UDBIRow::fetch(%d,%p)", pos, &value)

   if (isnull(pos)) U_RETURN(false);

   int type = U_SYSCALL(dbi_result_get_field_type_idx, "%p,%d", res, pos);

   switch (type)
      {
      case DBI_TYPE_DATETIME:
         {
         time_t v = U_SYSCALL(dbi_result_get_datetime_idx, "%p,%d", res, pos);

         (void) U_SYSCALL(memset, "%p,%d,%u", &value, 0, sizeof(struct tm));

         (void) U_SYSCALL(gmtime_r, "%p,%p", &v, &value);
         }
      break;

      case DBI_TYPE_STRING:
         {
         char const* d = U_SYSCALL(dbi_result_get_string_idx, "%p,%d", res, pos);

         (void) UTimeDate::getSecondFromTime(d, false, "%d-%d-%d %d:%d:%d", &value);
         }
      break;

      default:
         {
         U_INTERNAL_ASSERT_MSG(false,"DBI: bad cast to datetime type")

         U_RETURN(false);
         }
      break;
      }

   U_RETURN(true);
}

bool UDBIRow::fetch(int pos, UString& value)
{
   U_TRACE(1, "UDBIRow::fetch(%d,%p)", pos, &value)

   if (isnull(pos)) U_RETURN(false);

   int type = U_SYSCALL(dbi_result_get_field_type_idx, "%p,%d", res, pos);

   if (type != DBI_TYPE_STRING)
      {
      U_INTERNAL_ASSERT_MSG(false,"DBI: bad cast to string type")

      U_RETURN(false);
      }

   char const* d = U_SYSCALL(dbi_result_get_string_idx, "%p,%d", res, pos);

   if (d)
      {
      (void) value.replace(d);

      U_RETURN(true);
      }

   U_RETURN(false);
}

unsigned long long UDBISet::rows()
{
   U_TRACE(1, "UDBISet::rows()")

   U_INTERNAL_ASSERT_POINTER_MSG(res, "DBI: no result assigned")

   unsigned long long r = U_SYSCALL(dbi_result_get_numrows, "%p", res);

   U_INTERNAL_ASSERT_DIFFERS_MSG(r,DBI_FIELD_ERROR,"DBI: failed to fetch number of rows")

   U_RETURN(r);
}

bool UDBISet::next(UDBIRow& r)
{
   U_TRACE(1, "UDBISet::next(%p)", &r)

   U_INTERNAL_ASSERT_POINTER_MSG(res, "DBI: no result assigned")

   if (U_SYSCALL(dbi_result_next_row, "%p", res))
      {
      if (r.owner)
         {
         U_INTERNAL_ASSERT_POINTER(r.res)

         U_SYSCALL_VOID(dbi_result_free, "%p", r.res);

         r.owner = false;
         }

      r.res     = res;
      r.current = 0;

      U_RETURN(true);
      }

   U_RETURN(false);
}

// Fetch a single row from query.
// If no rows where selected returns false,
// If exactly one row was fetched, returns true.

bool UDBI::single(UDBIRow& r)
{
   U_TRACE(1, "UDBI::single(%p)", &r)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER_MSG(conn, "DBI: backend is not open")
   U_INTERNAL_ASSERT_MSG(complete, "DBI: not all parameters are bind");

   const char* ptr = escaped_query.c_str();

   U_INTERNAL_ASSERT_POINTER_MSG(ptr, "DBI: no statement assigned")

   dbi_result res = (dbi_result) U_SYSCALL(dbi_conn_query, "%p,%S", conn, ptr);

   if (res)
      {
      UDBISet rset(res);

      if (rset.rows() == 1 &&
          rset.next(r))
         {
         rset.res = 0;

         U_RETURN(true);
         }
      }

   U_RETURN(false);
}

// Fetch query result into \a res

bool UDBI::fetch(UDBISet& rset)
{
   U_TRACE(1, "UDBI::fetch(%p)", &rset)

   U_CHECK_MEMORY

   U_INTERNAL_ASSERT_POINTER_MSG(conn, "DBI: backend is not open")
   U_INTERNAL_ASSERT_MSG(complete, "DBI: not all parameters are bind");

   const char* ptr = escaped_query.c_str();

   U_INTERNAL_ASSERT_POINTER_MSG(ptr, "DBI: no statement assigned")

   dbi_result res = (dbi_result) U_SYSCALL(dbi_conn_query, "%p,%S", conn, ptr);

   if (res)
      {
      if (rset.res) U_SYSCALL_VOID(dbi_result_free, "%p", rset.res);

      rset.res = res;

      U_RETURN(true);
      }

   U_RETURN(false);
}

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UDBI::dump(bool reset) const
{
   U_CHECK_MEMORY

   *UObjectIO::os << "conn                   " << (void*)conn           << '\n'
                  << "complete               " << complete              << '\n'
                  << "pos_read               " << pos_read              << '\n'  
                  << "pos_write              " << pos_write             << '\n'
                  << "affected_rows          " << affected_rows         << '\n' 
                  << "ready_for_input        " << ready_for_input       << '\n' 
                  << "query_in      (UString " << (void*)&query_in      << ")\n"
                  << "escaped_query (UString " << (void*)&escaped_query << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

const char* UDBIRow::dump(bool reset) const
{
   U_CHECK_MEMORY

   *UObjectIO::os << "res     " << (void*)res << '\n'
                  << "owner   " << owner      << '\n'
                  << "current " << current;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

const char* UDBISet::dump(bool reset) const
{
   U_CHECK_MEMORY

   *UObjectIO::os << "res " << (void*)res;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

const char* UDBITransaction::dump(bool reset) const
{
   U_CHECK_MEMORY

   *UObjectIO::os << "commited " << commited;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
