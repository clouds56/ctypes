#include <iostream>
#include "packedfunc.h"

using namespace ctypes;

int test_packedfunc() {
  static auto &packedfunc_hello = Registry<PackedFunc>::Register("hello")
    .set_body([](PackedFunc::PackedArgs args, PackedFunc::PackedRetValue *rv) {
      *rv = PackedFunc::PackedRetValue();
      rv->value.v_int64 = ([](int a, int b) -> int { return a+b; }) (args[0], args[1]);
      rv->type_code = PackedFunc::PackedArg::kInt64;
    });
  std::cout << "PackedFunc::ListNames: ";
  for (auto i : Registry<PackedFunc>::ListNames()) {
    std::cout << i << ", ";
  }
  std::cout << std::endl;
  int hello_result = Registry<PackedFunc>::Get("hello")->operator()(1, 2);
  std::cout << "hello 1+2=" << hello_result << std::endl;

  //auto not_compiled = _Registry<PackedFunc>();
  return 0;
}

int test() {
  return test_packedfunc();
}
