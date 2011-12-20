// ir_session.h

#ifndef IR_SESSION_H
#define IR_SESSION_H 1

#include <ulib/utility/uhttp_session.h>

#include "cquery.h"

class IRDataSession : public UDataSession {
public:
   UString QUERY;
   UVector<WeightWord*>* vec;
   uint32_t FOR_PAGE;
   char TIME[9];

   // COSTRUTTORE

   IRDataSession()
      {
      U_TRACE_REGISTER_OBJECT(5, IRDataSession, "")

      vec      = 0;
      TIME[0]  = 0;
      FOR_PAGE = 0;
      }

   virtual ~IRDataSession()
      {
      U_TRACE_UNREGISTER_OBJECT(5, IRDataSession)

      if (vec) delete vec;
      }

   // method VIRTUAL to define

   virtual void fromStream(istream& is)
      {
      U_TRACE(5, "IRDataSession::fromStream(%p)", &is)

      U_INTERNAL_ASSERT_EQUALS(is.peek(), '{')

      UDataSession::fromStream(is);

      is.get(); // skip '{'

      is >> TIME
         >> FOR_PAGE;

      is.get(); // skip ' '

      QUERY.get(is);

      uint32_t vsize;
         is >> vsize;

      is.get(); // skip ' '

      // load filenames

      UVector<UString> vtmp(vsize);
                 is >> vtmp;

      WeightWord::clear();
      WeightWord::allocVector(vsize);

      UPosting::word_freq = 0;

      for (uint32_t i = 0, sz = vtmp.size(); i < sz; ++i)
         {
         *UPosting::filename = vtmp[i];

         WeightWord::push();
         }

      vec = WeightWord::vec;
            WeightWord::vec = 0;
      }

   virtual void toStream(ostream& os)
      {
      U_TRACE(5, "IRDataSession::toStream(%p)", &os)

      UDataSession::toStream(os);

      os.put('{');
      os.put(' ');
      os << TIME;
      os.put(' ');
      os << FOR_PAGE;
      os.put(' ');
      QUERY.write(os);
      os.put(' ');
      os << vec->size();
      os.put(' ');
      os << *vec;
      os.put(' ');
      os.put('}');
      }

   // STREAMS

   friend istream& operator>>(istream& is, IRDataSession& d)
      {
      U_TRACE(5, "IRDataSession::operator>>(%p,%p)", &is, &d)

      return operator>>(is, *(UDataSession*)&d);
      }

   friend ostream& operator<<(ostream& os, const IRDataSession& d)
      {
      U_TRACE(5, "IRDataSession::operator<<(%p,%p)", &os, &d)

      return operator<<(os, *(UDataSession*)&d);
      }

#ifdef DEBUG
#  include <ulib/internal/objectIO.h>

   const char* dump(bool usp_reset) const
      {
      *UObjectIO::os << "TIME                        ";

      char buffer[20];

      UObjectIO::os->write(buffer, u_sn_printf(buffer, sizeof(buffer), "%S", TIME));

      *UObjectIO::os << '\n'
                     << "FOR_PAGE                    " << FOR_PAGE      << '\n'
                     << "QUERY (UString              " << (void*)&QUERY << ")\n"
                     << "vec   (UVector<WeightWord*> " << (void*)vec    << ')';

      if (usp_reset)
         {
         UObjectIO::output();

         return UObjectIO::buffer_output;
         }

      return 0;
      }
#endif
};

class IRSession : public UHTTPSession {
public:

   IRSession() : UHTTPSession(U_NEW(IRDataSession))
      {
      U_TRACE_REGISTER_OBJECT(5, UHTTPSession, "")
      }

   ~IRSession()
      {
      U_TRACE_UNREGISTER_OBJECT(5, IRSession)
      }

   void set(const UString& _QUERY, uint32_t _FOR_PAGE)
      {
      U_TRACE(5, "IRSession::set(%.*S,%u)", U_STRING_TO_TRACE(_QUERY), _FOR_PAGE)

      ((IRDataSession*)data_session)->QUERY    = _QUERY;
      ((IRDataSession*)data_session)->FOR_PAGE = _FOR_PAGE;
      }

#ifdef DEBUG
   const char* dump(bool reset) const { return UHTTPSession::dump(reset); }
#endif

private:
   IRSession(const IRSession&) : UHTTPSession(0) {}
   IRSession& operator=(const IRSession&)        { return *this; }
};

#endif
