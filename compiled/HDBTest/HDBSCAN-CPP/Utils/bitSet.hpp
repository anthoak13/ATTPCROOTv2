#ifndef BITSET_HPP
#define BITSET_HPP

#include <vector>
class bitSet {
private:
   std::vector<bool> _bits;

public:
   bool get(int pos);

   void set(int pos);

   void ensure(int pos);
};

#endif
