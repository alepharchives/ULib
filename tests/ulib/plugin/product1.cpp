// product1.cpp

#include "product.h"

class U_EXPORT Product1 : public Product {
public:
            Product1() {}
   virtual ~Product1() { cout << "distruttore Product1" << '\n'; }

   virtual void print(std::ostream& os) const { os << "I am Product1" << endl; }
};

U_CREAT_FUNC(Product1)
