// ir_session.h

#ifndef IR_SESSION_H
#define IR_SESSION_H 1

#include <ulib/debug/crono.h>
#include <ulib/utility/data_session.h>

#include "cquery.h"

#define IR_SESSION (*(IRDataSession*)UHTTP::data_session)

class IRDataSession : public UDataSession {
public:
   UString QUERY;
   UVector<WeightWord*>* vec;
   uint32_t FOR_PAGE;
   char timebuf[9];

   // COSTRUTTORE

   IRDataSession()
      {
      U_TRACE_REGISTER_OBJECT(5, IRDataSession, "")

      vec        = 0;
      FOR_PAGE   = 0;
      timebuf[0] = 0;
      }

   virtual ~IRDataSession()
      {
      U_TRACE_UNREGISTER_OBJECT(5, IRDataSession)

      if (vec) delete vec;
      }

   // method VIRTUAL to define

   virtual void clear()
      {
      U_TRACE(5, "IRDataSession::clear()")

      UDataSession::clear();

      QUERY.clear();

      FOR_PAGE   = 0;
      timebuf[0] = 0;

      if (vec)
         {
         delete vec;
                vec = 0;
         }
      }

   virtual void fromDataSession(UDataSession& data_session)
      {
      U_TRACE(5, "IRDataSession::fromDataSession(%p)", &data_session)

      UDataSession::fromDataSession(data_session);

      U_INTERNAL_ASSERT_EQUALS(vec,0)

      vec      = WeightWord::duplicate((*(IRDataSession*)&data_session).vec);
      QUERY    =                       (*(IRDataSession*)&data_session).QUERY;
      FOR_PAGE =                       (*(IRDataSession*)&data_session).FOR_PAGE;

      (void) u_mem_cpy(timebuf, (*(IRDataSession*)&data_session).timebuf, sizeof(timebuf));
      }

   virtual UDataSession* toDataSession()
      {
      U_TRACE(5, "IRDataSession::toDataSession()")

      UDataSession* ptr = U_NEW(IRDataSession);

      ptr->creation = creation;

      (*(IRDataSession*)ptr).vec      = WeightWord::duplicate(vec);
      (*(IRDataSession*)ptr).QUERY    = QUERY;
      (*(IRDataSession*)ptr).FOR_PAGE = FOR_PAGE;

      (void) u_mem_cpy((*(IRDataSession*)ptr).timebuf, timebuf, sizeof(timebuf));

      U_RETURN_POINTER(ptr,UDataSession);
      }

   virtual void fromStream(istream& is)
      {
      U_TRACE(5, "IRDataSession::fromStream(%p)", &is)

      UDataSession::fromStream(is);

      U_INTERNAL_ASSERT_EQUALS(is.peek(), '{')

      is.get(); // skip '{'

      is >> timebuf
         >> FOR_PAGE;

      is.get(); // skip ' '

      QUERY.get(is);

      U_INTERNAL_ASSERT_EQUALS(vec,0)

      vec = WeightWord::fromStream(is);
      }

   virtual void toStream(ostream& os)
      {
      U_TRACE(5, "IRDataSession::toStream(%p)", &os)

      UDataSession::toStream(os);

      os.put('{');
      os.put(' ');
      os << timebuf;
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
      *UObjectIO::os << "timebuf                     ";

      char buffer[20];

      UObjectIO::os->write(buffer, u_sn_printf(buffer, sizeof(buffer), "%S", timebuf));

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

#endif
