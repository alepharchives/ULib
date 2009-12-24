// product.h

#ifndef product_H
#define product_H

#include <ulib/dynamic/plugin.h>

#include <iostream>

class Product {
public:
				Product() {}
	virtual ~Product() { cout << "distruttore Product" << '\n'; }

	virtual void print(std::ostream& os) const = 0;

	friend std::ostream& operator<<(std::ostream& os, const Product& base);
};

std::ostream& operator<<(std::ostream& os, const Product& base) { base.print(os); return os; }

#endif
