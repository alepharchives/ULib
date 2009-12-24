// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    mysql.cpp
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/mysql/mysql.h>

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

const char* UMySQL::dump(bool reset) const
{
   *UObjectIO::os << "connPtr " << (void*)connPtr;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

const char* UMySQLRow::dump(bool reset) const
{
   *UObjectIO::os << "m_row     " << (void*)m_row << '\n'
                  << "m_nFields " << m_nFields;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

const char* UMySQLSet::dump(bool reset) const
{
   *UObjectIO::os << "res        " << (void*)res << '\n'
                  << "m_nFields  " << m_nFields  << '\n'
                  << "m_rowCount " << m_rowCount;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return 0;
}

#endif
