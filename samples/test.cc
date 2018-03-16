#include <iostream>
#include <numeric>
#include "packedfunc.h"

using namespace ctypes;

int test_packedfunc() {
  static auto &packedfunc_hello = Registry<PackedFunc>::Register("hello")
      .set_body([](PackedFunc::Args args, PackedFunc::RetValue *rv) {
        rv->reset(([](int a, int b) -> int { return a+b; }) (args[0], args[1]));
      });
  std::cout << Registry<PackedFunc>::RegistryName() << "::ListNames: ";
  for (const auto& i : Registry<PackedFunc>::ListNames()) {
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

int test_vector() {
  static auto &packedfunc_hello = Registry<PackedFunc>::Register("vector_add")
      .set_body([](PackedFunc::Args args, PackedFunc::RetValue *rv) {
        rv->reset(([](std::vector<std::vector<int>> a, std::vector<int> b) -> std::vector<std::vector<int>> {
          int t = std::accumulate(b.begin(), b.end(), 0);
          for(auto& x: a) for(auto& i: x){i += t;}; return a;
        }) (args[0], args[1]));
      });
  std::vector<std::vector<int>> hello_result = Registry<PackedFunc>::Get("vector_add")->operator()(
      PackedManagedVector::create(std::vector<std::vector<int>>{{1, 2, 3}, {4}}),
      PackedManagedVector::create(std::vector<int>{1,2,3,4}));
  std::cout << "vector_add: [";
  for (const auto& x : hello_result) {
    std::cout << "[";
    for (auto i : x) {
      std::cout << i << ",";
    }
    std::cout << "], ";
  }
  std::cout << "]" << std::endl;

  return 0;
}

int test() {
  using TestFunc = std::function<int()>;
  //auto not_compiled = _Registry<PackedFunc>();
  // Note: static variable in function would not be initialized until called.
  std::vector<TestFunc> ts = { test_packedfunc, test_str, test_func, test_vector };
  return std::accumulate(ts.begin(), ts.end(), 0, [](int cur, auto f) {
    return cur + f();
  });
}
