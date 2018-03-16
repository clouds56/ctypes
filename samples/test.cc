#include <iostream>
#include "packedfunc.h"

using namespace ctypes;

int test_packedfunc() {
  static auto &packedfunc_hello = Registry<PackedFunc>::Register("hello")
    .set_body([](PackedFunc::PackedArgs args, PackedFunc::PackedRetValue *rv) {
      rv->reset(([](int a, int b) -> int { return a+b; }) (args[0], args[1]));
    });
  std::cout << "PackedFunc::ListNames: ";
  for (auto i : Registry<PackedFunc>::ListNames()) {
    std::cout << i << ", ";
  }
  std::cout << std::endl;
  int hello_result = Registry<PackedFunc>::Get("hello")->operator()(1, 2);
  std::cout << "hello 1+2=" << hello_result << std::endl;

  return 0;
}

int test_str() {
  static auto &packedfunc_hello = Registry<PackedFunc>::Register("append_str")
      .set_body([](PackedFunc::PackedArgs args, PackedFunc::PackedRetValue *rv) {
        rv->reset(([](std::string a, std::string b) -> std::string { return a+" "+b; }) (args[0], args[1]));
      });
  std::string hello_result = Registry<PackedFunc>::Get("append_str")->operator()("hello", "world");
  std::cout << "append_str: " << hello_result << std::endl;

  return 0;
}

int test() {
  //auto not_compiled = _Registry<PackedFunc>();
  return test_packedfunc() || test_str();
}
