#include <iostream>
#include "packedfunc.h"

using namespace ctypes;

int test_packedfunc() {
  static auto &packedfunc_hello = Registry<PackedFunc>::Register("hello")
    .set_body([](PackedFunc::PackedArgs args, PackedFunc::PackedRetValue *rv) {
      *rv = PackedFunc::PackedRetValue();
    });
  std::cout << "PackedFunc::ListNames: ";
  for (auto i : Registry<PackedFunc>::ListNames()) {
    std::cout << i << ", ";
  }
  Registry<PackedFunc>::Get("hello")->call(PackedFunc::PackedArgs(0, nullptr, nullptr));

  //auto not_compiled = _Registry<PackedFunc>();

  std::cout << std::endl;
  return 0;
}

int test() {
  return test_packedfunc();
}
