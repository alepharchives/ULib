// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    dbi.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_DBI_H
#define ULIB_DBI_H 1

#include <ulib/string.h>

extern "C" {
#include <dbi/dbi.h>
}

#include <limits>
#include <sstream>
#include <iomanip>

/**
   @class UDBI

   @brief This class is a wrapper around the DBI C API library (Database Independent Abstraction Layer).
*/

class UDBIRow;
class UDBISet;

struct exec {}; // Special type to start statement execution   using operator,() - syntactic sugar
struct null {}; // Special type to bind a NULL value to column using operator,() - syntactic sugar

// \This function returns a pair of value and NULL flag allowing to bind conditional values easily like
//
// \code
// sql << "insert into foo values(?)" << use(my_int, my_int_is_null);
// \endcode

template<typename T> std::pair<T,bool> use(T ref, bool isnull = false) { return std::pair<T,bool>(ref, isnull); }

class U_EXPORT UDBI {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

    UDBI(const char* driverdir = 0, const char* drivername = "sqlite3"); // ex: "/usr/lib/dbd"
   ~UDBI()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UDBI)

      close();

      U_SYSCALL_VOID_NO_PARAM(dbi_shutdown);
      }

   // VARIE

   dbi_conn getConnection() { return conn; } // Get low level libdbi connection object

   void close()
      {
      U_TRACE(1, "UDBI::close()")

      U_CHECK_MEMORY

      if (conn)
         {
         U_SYSCALL_VOID(dbi_conn_close, "%p", conn);

         conn = 0;
         }
      }

   bool reconnect();
   bool setDirectory(const char* directory = "./");
   bool connect(const char* dbName, const char* hostName = 0, const char* username = 0, const char* password = 0);

   // Get last inserted rowid for sequence \a seq. Some DB require sequence name (postgresql) for other seq is just ignored (mysql, sqlite)

   unsigned long long rowid(char const* seq = 0)
      {
      U_TRACE(1, "UDBI::rowid(%S)", seq)

      U_INTERNAL_ASSERT_POINTER_MSG(conn, "DBI: backend is not open")

      unsigned long long r = U_SYSCALL(dbi_conn_sequence_last, "%p,%S", conn, seq);

      U_RETURN(r);
      }

   // ERROR

   const char* getLastError() const
      {
      U_TRACE(1, "UDBI::getLastError()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_POINTER_MSG(conn, "DBI: backend is not open")

      const char* errmsg_dest;

      (void) U_SYSCALL(dbi_conn_error, "%p,%p", conn, &errmsg_dest);

      U_RETURN(errmsg_dest);
      }

   // QUERY

   // Set query to be prepared for execution, parameters that should be binded
   // should be marked with "?" symbol. Note: when binding string you should not
   // write quites around it as they would be automatically added.
   //
   // For example:
   //
   // \code
   //  sql.query("SELECT * FROM users WHERE name=?")
   //  sql.bind(username);
   //  sql.fetch(rset);
   // \endcode

   void query(const char* str, uint32_t len)
      {
      U_TRACE(0, "UDBI::query(%.*S,%u)", len, str, len)

      pos_read = pos_write = 0;
      complete = ready_for_input = false;

      query_in     = str;
      query_in_len = len;

      escaped_query.setBuffer(len * 3 / 2);

      escape();
      }

   UDBI& operator<<(const char*    q) { query(q, u_strlen(q));       return *this; } // Syntactic sugar for query(q)
   UDBI& operator<<(const UString& q) { query(U_STRING_TO_PARAM(q)); return *this; } // Syntactic sugar for query(q)

   // BIND

   void bind(                   int v, bool isnull = false) { doBind(v, isnull); } // Bind a numeric  parameter at next position in query
   void bind(unsigned           int v, bool isnull = false) { doBind(v, isnull); } // Bind a numeric  parameter at next position in query
   void bind(         long      int v, bool isnull = false) { doBind(v, isnull); } // Bind a numeric  parameter at next position in query
   void bind(unsigned long      int v, bool isnull = false) { doBind(v, isnull); } // Bind a numeric  parameter at next position in query
   void bind(         long long int v, bool isnull = false) { doBind(v, isnull); } // Bind a numeric  parameter at next position in query
   void bind(unsigned long long int v, bool isnull = false) { doBind(v, isnull); } // Bind a numeric  parameter at next position in query
   void bind(     double            v, bool isnull = false) { doBind(v, isnull); } // Bind a numeric  parameter at next position in query
   void bind(long double            v, bool isnull = false) { doBind(v, isnull); } // Bind a numeric  parameter at next position in query

   void bind(const char*            v, bool isnull = false) { bind(UString(v), isnull); }

   void bind(struct tm&             v, bool isnull = false); // Bind a date-time parameter at next position in query
   void bind(const null&            v, bool isnull =  true); // Bind a NULL      parameter at next position in query, isnull is just for consistency
   void bind(const UString&         v, bool isnull = false); // Bind a string    parameter at next position in query

   // Syntactic sugar for bind(v)

   UDBI& operator,(struct tm&     v)    { bind(        v,  false); return *this; } // Syntactic sugar for bind(v)
   UDBI& operator,(const char*    v)    { bind(UString(v), false); return *this; } // Syntactic sugar for bind(v)
   UDBI& operator,(const UString& v)    { bind(        v,  false); return *this; } // Syntactic sugar for bind(v)

   // Syntactic sugar for bind(p.first,p.second), usually used with use() function

   UDBI& operator,(std::pair<const UString&,bool> p) { bind(p.first, p.second); return *this; }

   template<typename T> UDBI& operator,(T v)                 { bind(v, false);          return *this; }
   template<typename T> UDBI& operator,(std::pair<T,bool> p) { bind(p.first, p.second); return *this; }

   // Execute the statement

   void exec();

   // Fetch a single row from query.
   // If no rows where selected returns false,
   // If exactly one row was fetched, returns true.

   bool single(UDBIRow& r);

   // Fetch query result into \a rset

   bool fetch(UDBISet& rset);

   typedef struct exec ExecType;

   UDBI& operator,(UDBIRow& row)      { (void) single(row); return *this; } // Syntactic sugar for fetching a single row
   UDBI& operator,(UDBISet& rset)     { (void) fetch(rset); return *this; } // Syntactic sugar for fetching result
   UDBI& operator,(const ExecType& e) {         exec();     return *this; } // Syntactic sugar for calling exec() function

   // QUERY PROCESS

   template <typename ProcessQuery> typename ProcessQuery::ReturnType Exec()
      {
      U_TRACE(1, "UDBI::Exec()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_POINTER_MSG(conn, "DBI: backend is not open")
      U_INTERNAL_ASSERT_MSG(complete, "DBI: not all parameters are bind");

      const char* ptr = escaped_query.c_str();

      U_INTERNAL_ASSERT_POINTER_MSG(ptr, "DBI: no statement assigned")

      dbi_result res = (dbi_result) U_SYSCALL(dbi_conn_query, "%p,%S", conn, ptr);

      return ProcessQuery::deepQuery(res);
      }

   // Get number of affected rows by the last statement

   unsigned long long affected()
      {
      U_TRACE(1, "UDBI::affected()")

      U_RETURN(affected_rows);
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   dbi_conn conn;
   const char* query_in;
   UString escaped_query;
   unsigned long long affected_rows;
   uint32_t pos_read, pos_write, query_in_len;
   bool ready_for_input, complete;

                        void escape();
   template<typename T> void doBind(T v, bool isnull)
      {
      U_TRACE(0, "UDBI::doBind<T>(%p,%b)", &v, isnull)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_POINTER_MSG(conn, "DBI: backend is not open")
      U_INTERNAL_ASSERT_MSG(ready_for_input, "DBI: more parameters given then inputs in query");

      // The representation of a number in decimal form is more compact then in binary so it should be enough

      if (isnull) (void) escaped_query.append(U_CONSTANT_TO_PARAM("NULL"));
      else
         {
         ostringstream ss;

         ss.imbue(locale::classic());

         if (!numeric_limits<T>::is_integer) ss << setprecision(numeric_limits<T>::digits10+1);

         ss << v;

         string s = ss.str();

         (void) escaped_query.append(s.c_str(), s.size());
         }

      ready_for_input = false;

      escape();
      }

private:
   UDBI(const UDBI&)            {}
   UDBI& operator=(const UDBI&) { return *this; }
};

// This class represents a single row that is fetched from the DB

class U_EXPORT UDBIRow {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UDBIRow(dbi_result res_ = 0) : res(res_)
      {
      U_TRACE_REGISTER_OBJECT(0, UDBIRow, "%p", res_)

      owner   = (res_ != 0);
      current = 0;
      }

   ~UDBIRow()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UDBIRow)

      if (owner)
         {
         U_INTERNAL_ASSERT_POINTER(res)

         U_SYSCALL_VOID(dbi_result_free, "%p", res);
         }
      }

   // VARIE

   dbi_result getResult() { return res; }  // Get underlying libdbi result object. For low level access

   // Check if this row has some data or not

   bool isempty() const  { return (res != 0); }

   operator bool() const { return (res != 0); }

   // Check if the column at position \a idx has NULL value, first column index is 1

   bool isnull(int idx)
      {
      U_TRACE(1, "UDBIRow::isnull(%d)", idx)

      U_INTERNAL_ASSERT_POINTER_MSG(res, "DBI: using unititilized row")

      int r = U_SYSCALL(dbi_result_field_is_null_idx, "%p,%d", res, idx);

      U_INTERNAL_ASSERT_DIFFERS_MSG(r,DBI_FIELD_FLAG_ERROR,"DBI: invalid field")

      U_RETURN(r);
      }

   // Check if the column named \a id has NULL value

   bool isnull(const UString& id)
      {
      U_TRACE(1, "UDBIRow::isnull(%.*S)", U_STRING_TO_TRACE(id))

      U_INTERNAL_ASSERT_POINTER_MSG(res, "DBI: using unititilized row")

      const char* ptr = id.c_str();

      U_INTERNAL_ASSERT_POINTER_MSG(ptr, "DBI: no id")

      int r = U_SYSCALL(dbi_result_field_is_null, "%p,%S", res, ptr);

      U_INTERNAL_ASSERT_DIFFERS_MSG(r,DBI_FIELD_FLAG_ERROR,"DBI: invalid field")

      U_RETURN(r);
      }

   bool operator[](int idx)           { return isnull(idx); } // Syntactic sugar for isnull(idx)
   bool operator[](const UString& id) { return isnull(id); }  // Syntactic sugar for isnull(id)

   // Fetch \a value at position \a pos (starting from 1), returns false if the column has null value

   bool fetch(int pos,          short& value) { return sfetch(pos, value); }
   bool fetch(int pos, unsigned short& value) { return ufetch(pos, value); }
   bool fetch(int pos,          int&   value) { return sfetch(pos, value); }
   bool fetch(int pos, unsigned int&   value) { return ufetch(pos, value); }
   bool fetch(int pos,          long&  value) { return sfetch(pos, value); }
   bool fetch(int pos, unsigned long&  value) { return ufetch(pos, value); }

   bool fetch(int pos,          long long& value);
   bool fetch(int pos, unsigned long long& value);
   bool fetch(int pos, double&             value);

   bool fetch(int pos, float& value)
      {
      U_TRACE(0, "UDBIRow::fetch(%d,%p)", pos, &value)

      U_INTERNAL_ASSERT_POINTER_MSG(res, "DBI: using unititilized row")

      double v;

      if (fetch(pos, v))
         {
         value = static_cast<float>(v);

         U_RETURN(true);
         }

      U_RETURN(false);
      }

   bool fetch(int pos, long double& value)
      {
      U_TRACE(0, "UDBIRow::fetch(%d,%p)", pos, &value)

      U_INTERNAL_ASSERT_POINTER_MSG(res, "DBI: using unititilized row")

      double v;

      if (fetch(pos, v))
         {
         value = v;

         U_RETURN(true);
         }

      U_RETURN(false);
      }

   bool fetch(int pos, UString&   value); // Fetch \a string value at position \a pos (starting from 1), returns false if the column has null value
   bool fetch(int pos, struct tm& value); // Fetch \a   time value at position \a pos (starting from 1), returns false if the column has null value

   // Fetch a value \a v from the row starting with first column. Each next call updates the internal pointer to next column in row

   template<typename T> UDBIRow& operator>>(T& value) { fetch(++current, value); return *this; }

   // Get number of columns in the row

   unsigned int cols()
      {
      U_TRACE(1, "UDBIRow::cols()")

      U_INTERNAL_ASSERT_POINTER_MSG(res, "DBI: using unititilized row")

      unsigned int r = U_SYSCALL(dbi_result_get_numfields, "%p", res);

      U_INTERNAL_ASSERT_DIFFERS_MSG(r,DBI_FIELD_ERROR,"DBI: failed to fetch number of columns")

      U_RETURN(r);
      }

   // Fetch value by column. It fetches the value from column \a col (starting from 1) and returns it

   template<typename T> T get(int col)
      {
      U_TRACE(0, "UDBIRow::get<T>(%d)", col)

      U_INTERNAL_ASSERT_POINTER_MSG(res, "DBI: using unititilized row")

      T v;
      bool result = fetch(col, v);

      U_INTERNAL_ASSERT_MSG(result,"DBI: null value fetch")

      return v;
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

private:
   dbi_result res;
   int current;
   bool owner;

   template<typename T> bool sfetch(int pos, T& value)
      {
      U_TRACE(0, "UDBIRow::sfetch<T>(%d,%p)", pos, &value)

      U_INTERNAL_ASSERT_POINTER_MSG(res, "DBI: using unititilized row")

      long long v;

      if (fetch(pos, v))
         {
         U_INTERNAL_ASSERT_RANGE_MSG(std::numeric_limits<T>::min(),v,std::numeric_limits<T>::max(),"DBI: Bad cast to integer of small size")

         value = static_cast<T>(v);

         U_RETURN(true);
         }

      U_RETURN(false);
      }

   template<typename T> bool ufetch(int pos, T& value)
      {
      U_TRACE(0, "UDBIRow::ufetch<T>(%d,%p)", pos, &value)

      U_INTERNAL_ASSERT_POINTER_MSG(res, "DBI: using unititilized row")

      unsigned long long v;

      if (fetch(pos, v))
         {
         U_INTERNAL_ASSERT_MSG(v <= std::numeric_limits<T>::max(),"DBI: Bad cast to integer of small size")

         value = static_cast<T>(v);

         U_RETURN(true);
         }

      U_RETURN(false);
      }

   UDBIRow(const UDBIRow&)            {}
   UDBIRow& operator=(const UDBIRow&) { return *this; }

   friend class UDBISet;
};

// \brief This class holds query result and allows iterating over its rows

class U_EXPORT UDBISet {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   UDBISet(dbi_result res_ = 0) : res(res_)
      {
      U_TRACE_REGISTER_OBJECT(0, UDBISet, "%p", res_)
      }

   ~UDBISet()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UDBISet)

      if (res) U_SYSCALL_VOID(dbi_result_free, "%p", res);
      }

   // VARIE

   dbi_result getResult() { return res; }  // Get underlying libdbi result object. For low level access

   // Check if this set has some data or not

   bool isempty() const  { return (res != 0); }

   operator bool() const { return (res != 0); }

   // Get number of rows in the returned result

   unsigned long long rows()
      {
      U_TRACE(1, "UDBISet::rows()")

      U_INTERNAL_ASSERT_POINTER_MSG(res, "DBI: no result assigned")

      unsigned long long r = U_SYSCALL(dbi_result_get_numrows, "%p", res);

      U_INTERNAL_ASSERT_DIFFERS_MSG(r,DBI_FIELD_ERROR,"DBI: failed to fetch number of rows")

      U_RETURN(r);
      }

   // Get number of columns in the returned result

   unsigned int cols()
      {
      U_TRACE(1, "UDBISet::cols()")

      U_INTERNAL_ASSERT_POINTER_MSG(res, "DBI: no result assigned")

      unsigned int r = U_SYSCALL(dbi_result_get_numfields, "%p", res);

      U_INTERNAL_ASSERT_DIFFERS_MSG(r,DBI_FIELD_ERROR,"DBI: failed to fetch number of columns")

      U_RETURN(r);
      }

   // Fetch next row and store it into \a r. Returns false if no more rows remain.

   bool next(UDBIRow& r);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

private:
   dbi_result res;

   friend class UDBI;

   UDBISet(const UDBISet&)            {}
   UDBISet& operator=(const UDBISet&) { return *this; }
};

// \brief Transaction scope guard
//
// It automatically rollbacks the transaction during stack unwind if it wasn't committed

class U_EXPORT UDBITransaction {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // Begin transaction on session \a s

   UDBITransaction(UDBI& s) : sql(s)
      {
      U_TRACE_REGISTER_OBJECT(0, UDBITransaction, "%p", &s)

      commited = false;

      sql << "BEGIN",exec();
      }

   // If commit wasn't called, calls rollback()

   ~UDBITransaction()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UDBITransaction)

      if (!commited) sql << "ROLLBACK",exec();
      }

   // Commit translation to database

   void commit()
      {
      U_TRACE(0, "UDBITransaction::commit()")

      sql << "COMMIT",exec();

      commited = true;
      }

   // Rollbacks the transaction

   void rollback()
      {
      U_TRACE(0, "UDBITransaction::rollback()")

      sql << "ROLLBACK",exec();

      commited = true;
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

private:
   UDBI& sql;
   bool commited;

   UDBITransaction(const UDBITransaction&);
   UDBITransaction& operator=(const UDBITransaction&) { return *this; }
};

// Declare and define the ProcessQueryPolicy

struct U_EXPORT UNoData
{
   // The policy for the not data query such as "update", "insert" statement

   static bool deepQuery(dbi_result res)
      {
      U_TRACE(0, "UNoData::deepQuery(%p)", res)

      if (res)
         {
         U_SYSCALL_VOID(dbi_result_free, "%p", res);

         U_RETURN(true);
         }

      U_RETURN(false);
      }
};

struct U_EXPORT UWithData
{
   // The policy for the query with entire dataset

   static UDBISet* deepQuery(dbi_result res) { return U_NEW(UDBISet(res)); }
};

template <class T> struct U_EXPORT UCheckOneRecord
{
   // The policy to check one field of the first record
   // which is normally used in checking if the valid record exist

   static T deepQuery(dbi_result res)
      {
      U_TRACE(0, "UCheckOneRecord::deepQuery(%p)", res)

      T value;
      UDBISet rset(res);

      if (rset.rows())
         {
         UDBIRow row;

         if (rset.next(row)) value = row.get(0, value);
         }

      return value;
      }
};

#endif
