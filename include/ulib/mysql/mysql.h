// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mysql.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_MySQL_H
#define ULIB_MySQL_H 1

#include <ulib/string.h>

extern "C" {
#include <mysql/mysql.h>
}

/**
   @class UMySQL

   @brief This class is a wrapper around the mysql C API library.
*/

class U_EXPORT UMySQL {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

    UMySQL()
      {
      U_TRACE_REGISTER_OBJECT(0, UMySQL, "")

      connPtr = (MYSQL*) U_SYSCALL(mysql_init, "%p", 0);
      }

   ~UMySQL()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UMySQL)

      if (connPtr) U_SYSCALL_VOID(mysql_close, "%p", connPtr);
      }

   // VARIE

   bool connect(const char* hostName, const char* dbName, const char* username, const char* password)
      {
      U_TRACE(1, "UMySQL::connect(%S,%S,%S,%S)", hostName, dbName, username, password)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_POINTER(connPtr)

      MYSQL* result = (MYSQL*) U_SYSCALL(mysql_real_connect, "%p,%S,%S,%S,%S,%u,%S,%lu",
                                         connPtr, hostName, dbName, username, password,
                                         0,     // default port
                                         NULL,  // default socket name
                                         0);    // connection flag

#  ifdef DEBUG
      if (result == 0) U_DUMP("error = (%d, %s)", getLastErrorCode(), getLastErrorString())
#  endif

      U_RETURN(result != 0);
      }

   // ERROR

   int getLastErrorCode() const
      {
      U_TRACE(1, "UMySQL::getLastErrorCode()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_POINTER(connPtr)

      int result = U_SYSCALL(mysql_errno, "%p", connPtr);

      U_RETURN(result);
      }

   const char* getLastErrorString() const
      {
      U_TRACE(1, "UMySQL::getLastErrorString()")

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_POINTER(connPtr)

      const char* result = U_SYSCALL(mysql_error, "%p", connPtr);

      U_RETURN(result);
      }

   // QUERY

   template <typename ProcessQuery> typename ProcessQuery::ReturnType query(const char* sql)
      {
      U_TRACE(1, "UMySQL::query(%S)", sql)

      U_CHECK_MEMORY

      U_INTERNAL_ASSERT_POINTER(connPtr)

      if (U_SYSCALL(mysql_query, "%p,%S", connPtr, sql)) return ProcessQuery::returnInFail();

      return ProcessQuery::deepQuery(connPtr);
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   MYSQL* connPtr;

private:
   UMySQL(const UMySQL&)            {}
   UMySQL& operator=(const UMySQL&) { return *this; }
};

// The thin wrapper of MYSQL_ROW

class UMySQLRow {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

    UMySQLRow(MYSQL_RES* ptr, my_ulonglong nfields) : m_nFields(nfields)
      {
      U_TRACE_REGISTER_OBJECT(0, UMySQLRow, "%p,%u", ptr, nfields)

      U_INTERNAL_ASSERT_POINTER(ptr)

      m_row = (MYSQL_ROW) U_SYSCALL(mysql_fetch_row, "%p", ptr);
      }

   ~UMySQLRow()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UMySQLRow)
      }

   // VARIE

   operator bool() const { return (m_row != 0); }

   my_ulonglong numberOfFields() const { return m_nFields; }

   char* operator[](unsigned index)
      {
      U_TRACE(0, "UMySQLRow::operator[](%u)", index)

      U_INTERNAL_ASSERT_MINOR(index, m_nFields)

      U_RETURN(m_row[index]);
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

private:
   MYSQL_ROW m_row;
   my_ulonglong m_nFields;
};

// The thin wrapper of MYSQL_RES

class UMySQLSet {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // COSTRUTTORI

   void init()
      {
      U_TRACE(1, "UMySQLSet::init()")

      if (res)
         {
         m_nFields  = mysql_num_fields(res);
         m_rowCount = mysql_num_rows(res);
         }
      else
         {
         m_rowCount = m_nFields = 0;
         }

      U_INTERNAL_DUMP("m_rowCount = %lu, m_nFields = %lu", m_rowCount, m_nFields)
      }

   UMySQLSet(MYSQL* ptr)
      {
      U_TRACE_REGISTER_OBJECT(0, UMySQLSet, "%p", ptr)

      U_INTERNAL_ASSERT_POINTER(ptr)

      res = (MYSQL_RES*) U_SYSCALL(mysql_store_result, "%p", ptr);

      init();
      }

   UMySQLSet(MYSQL_RES* ptr = 0) : res(ptr)
      {
      U_TRACE_REGISTER_OBJECT(0, UMySQLSet, "%p", ptr)

      init();
      }

   ~UMySQLSet()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UMySQLSet)

      if (res) U_SYSCALL_VOID(mysql_free_result, "%p", res);
      }

   // VARIE

   inline operator bool() const { return (res != 0); }

   my_ulonglong numberOfFields() const { return  m_nFields; }
   my_ulonglong numberOfRecords() const { return m_rowCount; }

   UMySQLRow* getNextRow()
      {
      U_TRACE(0, "UMySQLSet::getNextRow()")

      U_INTERNAL_ASSERT_POINTER(res)

      UMySQLRow* result = U_NEW(UMySQLRow(res, m_nFields));

      U_RETURN_POINTER(result,UMySQLRow);
      }

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

private:
   MYSQL_RES* res;
   my_ulonglong m_rowCount, m_nFields;
};

// Declare and define the ProcessQueryPolicy

struct UNoData // The policy for the not data query such as "update", "insert" statement
{
   typedef bool ReturnType;

   static ReturnType returnInFail()          { return false; }
   static ReturnType deepQuery(MYSQL* ptr)   { return true; }
};

struct UWithData // The policy for the query with entire dataset
{
   typedef UMySQLSet* ReturnType;

   static ReturnType returnInFail()        { return U_NEW(UMySQLSet()); }
   static ReturnType deepQuery(MYSQL* ptr) { return U_NEW(UMySQLSet(ptr)); }
};

// The policy to check one field of the first record
// which is normally used in checking if the valid record exist

struct UCheckOneRecord
{
   typedef UString ReturnType;

   static ReturnType returnInFail() { return UString::getStringNull(); }

   static ReturnType deepQuery(MYSQL* ptr)
      {
      U_TRACE(0, "UCheckOneRecord::deepQuery(%p)", ptr)

      UString value;
      UMySQLSet rset(ptr);

      if (rset.numberOfRecords())
         {
         UMySQLRow* row = rset.getNextRow();

         if (row->numberOfFields()) value.replace((*row)[0]);

         delete row;
         }

      U_RETURN_STRING(value);
      }
};

#endif
