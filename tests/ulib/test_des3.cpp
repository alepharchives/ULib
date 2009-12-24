// test_des3.cpp

#include <ulib/file.h>
#include <ulib/base/ssl/des3.h>

#include <iostream>

static void check(const UString& dati, const char* file)
{
   U_TRACE(5,"check(%p,%S)", dati.data(), file)

   long pos;
   UString buffer1(dati.size() + 32), buffer2(dati.size());

   pos = u_des3_encode( (const unsigned char*)U_STRING_TO_PARAM(dati),    (unsigned char*)buffer1.data());
   buffer1.size_adjust(pos);
   pos = u_des3_decode( (const unsigned char*)U_STRING_TO_PARAM(buffer1), (unsigned char*)buffer2.data());
   buffer2.size_adjust(pos);

   U_INTERNAL_DUMP("buffer1 = %#.*S", U_STRING_TO_TRACE(buffer1))
   U_INTERNAL_DUMP("dati    = %#.*S", U_STRING_TO_TRACE(dati))
   U_INTERNAL_DUMP("buffer2 = %#.*S", U_STRING_TO_TRACE(buffer2))

   /*
   UFile save_file;

   if (save_file.creat("des3.encode"))
      {
      save_file.write(buffer1, true);
      save_file.close();
      }

   if (save_file.creat("des3.decode"))
      {
      save_file.write(buffer2, true);
      save_file.close();
      }
   */

   U_ASSERT( dati == buffer2 )
}

int U_EXPORT main(int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)", argc)

   u_des3_key( argv[1] );

   /*
   u_printf_string_max_length = 256;
   U_INTERNAL_DUMP("inp1_sched = %#.*S", DES_SCHEDULE_SZ, &UDES3::d.inp1_sched)
   U_INTERNAL_DUMP("inp2_sched = %#.*S", DES_SCHEDULE_SZ, &UDES3::d.inp2_sched)
   U_INTERNAL_DUMP("inp3_sched = %#.*S", DES_SCHEDULE_SZ, &UDES3::d.inp3_sched)
   */

   UString filename;

   while (cin >> filename)
      {
      UString dati = UFile::contentOf(filename);

      check(dati, filename.c_str());
      }
}
