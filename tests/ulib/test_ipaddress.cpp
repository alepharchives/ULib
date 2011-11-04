// test_ipaddress.cpp

#include <ulib/net/ipaddress.h>

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   u_init_ulib_hostname();

   UIPAddress x;
   char address[16];
   UString name = UString(argv[1]), domain, name_domain, name_ret;

   if (argv[2] && argv[2][0])
      {
      domain      = argv[2];
      name_domain = name + '.' + domain;
      }

   U_ASSERT( x.setHostName(U_STRING_FROM_CONSTANT("pippo1")) == false )

   // Test code for matching IP masks

   UIPAllow a;

   U_ASSERT( a.parseMask(U_STRING_FROM_CONSTANT("127.0.0.5")) == true )

   x.setHostName(U_STRING_FROM_CONSTANT("127.0.0.5"));

   U_ASSERT( a.isAllowed(x.getInAddr()) == true )

   x.setHostName(U_STRING_FROM_CONSTANT("127.0.0.0"));

   U_ASSERT( a.isAllowed(x.getInAddr()) == false )

   x.setHostName(U_STRING_FROM_CONSTANT("127.0.0.2"));

   U_ASSERT( a.isAllowed(x.getInAddr()) == false )

   U_ASSERT( a.parseMask(U_STRING_FROM_CONSTANT("127.0.0.1/8")) == true )

   U_ASSERT( a.isAllowed(x.getInAddr()) == true )

   U_ASSERT( a.parseMask(U_STRING_FROM_CONSTANT("10.113.0.0/16")) == true )
        
   x.setHostName(U_STRING_FROM_CONSTANT("10.113.45.67"));

   U_ASSERT( a.isAllowed(x.getInAddr()) == true )

   x.setHostName(U_STRING_FROM_CONSTANT("10.11.45.67"));

   U_ASSERT( a.isAllowed(x.getInAddr()) == false )

   x.setHostName(U_STRING_FROM_CONSTANT("127.0.0.1"));

   U_ASSERT( a.isAllowed(x.getInAddr()) == false )

   U_ASSERT( a.parseMask(U_STRING_FROM_CONSTANT("1.2.3.4/0")) == true )
        
   x.setHostName(U_STRING_FROM_CONSTANT("4.3.2.1"));

   U_ASSERT( a.isAllowed(x.getInAddr()) == true )

   U_ASSERT( a.parseMask(U_STRING_FROM_CONSTANT("1.2.3.4/40")) == false )
        
   U_ASSERT( a.parseMask(U_STRING_FROM_CONSTANT("1.2.3.4.5.6.7/8")) == false )
        
   U_ASSERT( a.parseMask(U_STRING_FROM_CONSTANT("1.2.3.4/8")) == true )
        
   x.setHostName(U_STRING_FROM_CONSTANT("4.3.2.1"));

   U_ASSERT( a.isAllowed(x.getInAddr()) == false )

   U_ASSERT( a.parseMask(U_STRING_FROM_CONSTANT("192.168.1.64/28")) == true )
        
   x.setHostName(U_STRING_FROM_CONSTANT("192.168.1.70"));

   U_ASSERT( a.isAllowed(x.getInAddr()) == true )

   x.setHostName(U_STRING_FROM_CONSTANT("192.168.1.7"));

   U_ASSERT( a.isAllowed(x.getInAddr()) == false )

   bool esito = x.setHostName(name);

   U_ASSERT( esito == true )

   if (esito)
      {
      (void) u_memcpy(address, x.get_in_addr(), x.getInAddrLength());

      x.setAddress(address);

      name_ret = x.getHostName();

      U_INTERNAL_DUMP("name_ret = %.*S", U_STRING_TO_TRACE(name_ret))

      U_ASSERT( name_ret.equalnocase(name) || name_ret.equalnocase(name_domain) )

      cout << name;
      }
}
