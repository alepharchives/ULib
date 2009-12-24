// test_plugin.cpp

#include "plugin/product.h"

int
U_EXPORT main (int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   UPlugIn<void*>::setPluginDirectory(argv[1]);

   Product* b1 = UPlugIn<Product*>::create(argv[2], strlen(argv[2]));

   if (b1 == 0) cout << "object with key " << argv[2] << " cannot be created" << endl;
   else         cout << *b1 << endl;

   Product* b2 = UPlugIn<Product*>::create(argv[3], strlen(argv[3]));

   if (b2 == 0) cout << "object with key " << argv[3] << " cannot be created" << endl;
   else         cout << *b2 << endl;

   U_ASSERT(UPlugIn<Product*>::empty() == false)

   delete b2;
   delete b1;

   UPlugIn<void*>::clear();
}
