#include <iostream>
#include "packedfunc.h"

using namespace ctypes;

int test_packedfunc() {
  static auto &packedfunc_hello = Registry<PackedFunc>::Register("hello")
      .set_body([](PackedFunc::Args args, PackedFunc::RetValue *rv) {
        rv->reset(([](int a, int b) -> int { return a+b; }) (args[0], args[1]));
      });
  std::cout << Registry<PackedFunc>::RegistryName() << "::ListNames: ";
  for (auto i : Registry<PackedFunc>::ListNames()) {
    std::cout << i << ", ";
  }
  std::cout << std::endl;
  int hello_result = Registry<PackedFunc>::Get("hello")->operator()(1, 2);
  std::cout << "hello: 1+2=" << hello_result << std::endl;

  return 0;
}

int test_str() {
  static auto &packedfunc_hello = Registry<PackedFunc>::Register("append_str")
      .set_body([](PackedFunc::Args args, PackedFunc::RetValue *rv) {
        rv->reset(([](std::string a, std::string b) -> std::string { return a+" "+b; }) (args[0], args[1]));
      });
  std::string hello_result = Registry<PackedFunc>::Get("append_str")->operator()("hello", "world");
  std::cout << "append_str: " << hello_result << std::endl;

  return 0;
}

int test_func() {
  static auto &packedfunc_hello = Registry<PackedFunc>::Register("test_append_str")
      .set_body([](PackedFunc::Args args, PackedFunc::RetValue *rv) {
        rv->reset(([](PackedFunc func, std::string a, std::string b) -> std::string { return func(a, b); }) (args[0], args[1], args[2]));
      });
  std::string hello_result = Registry<PackedFunc>::Get("test_append_str")->operator()(*Registry<PackedFunc>::Get("append_str"), "append", "str");
  std::cout << "test_append_str: " << hello_result << std::endl;

  return 0;
}

int test() {
  //auto not_compiled = _Registry<PackedFunc>();
  // Note: static variable in function would not be initialized until called.
  return test_packedfunc() || test_str() || test_func();
}
