// product2.cpp

#include "product.h"

class U_EXPORT Product2 : public Product {
public:
            Product2() {}
   virtual ~Product2() { cout << "distruttore Product2" << '\n'; }

   virtual void print(std::ostream& os) const { os << "I am Product2" << endl; }
};

U_CREAT_FUNC(Product2)
