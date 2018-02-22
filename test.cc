#include <iostream>
#include "packedfunc.h"

using namespace ctypes;

int test_packedfunc() {
  static PackedFunc &packedfunc_hello = Registry<PackedFunc>::Register("hello");
  std::cout << "PackedFunc::ListNames: ";
  for (auto i : Registry<PackedFunc>::ListNames()) {
    std::cout << i << ", ";
  }
  std::cout << std::endl;
  return 0;
}

int test() {
  return test_packedfunc();
}
