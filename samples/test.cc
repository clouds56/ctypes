#include <iostream>
#include <numeric>
#include "packedfunc.h"
#include "ext.h"

using namespace ctypes;

static auto &packedfunc_hello = Registry<PackedFunc>::Register("hello")
    .set_body([](PackedFunc::Args args, PackedFunc::RetValue *rv) {
      rv->reset(([](int a, int b) -> int { return a+b; }) (args[0], args[1]));
    });


static auto &packedfunc_append_str = Registry<PackedFunc>::Register("append_str")
    .set_body([](PackedFunc::Args args, PackedFunc::RetValue *rv) {
      rv->reset(([](std::string a, std::string b) -> std::string { return a+" "+b; }) (args[0], args[1]));
    });

static auto &packedfunc_test_append_str = Registry<PackedFunc>::Register("test_append_str")
    .set_body([](PackedFunc::Args args, PackedFunc::RetValue *rv) {
      rv->reset(([](PackedFunc func, std::string a, std::string b) -> std::string { return func(a, b); }) (args[0], args[1], args[2]));
    });

static auto &packedfunc_vector_add = Registry<PackedFunc>::Register("vector_add")
    .set_body([](PackedFunc::Args args, PackedFunc::RetValue *rv) {
      rv->reset(([](std::vector<std::vector<int>> a, std::vector<int> b) -> std::vector<std::vector<int>> {
        int t = std::accumulate(b.begin(), b.end(), 0);
        for(auto& x: a){for(auto& i: x){i += t;};} return a;
      }) (args[0], args[1]));
    });

static auto &packedfunc_ext_new = Registry<PackedFunc>::Register("ext_new")
    .set_body([](PackedFunc::Args args, PackedFunc::RetValue *rv) {
      ext::test* p = new ext::test;
      std::cerr << "new ext::test " << p << std::endl;
      rv->reset(p/*, false*/);
    });

static auto &packedfunc_ext_release = Registry<PackedFunc>::Register("ext_release")
    .set_body([](PackedFunc::Args args, PackedFunc::RetValue *rv) {
      ext::test* p = args[0];
      std::cerr << "release ext::test " << p << std::endl;
      delete p;
      rv->reset(0);
    });

static auto &packedfunc_ext_get = Registry<PackedFunc>::Register("ext_get")
    .set_body([](PackedFunc::Args args, PackedFunc::RetValue *rv) {
      ext::test* p = args[0];
      rv->reset(p->name);
    });

static auto &packedfunc_ext_transform = Registry<PackedFunc>::Register("ext_transform")
    .set_body([](PackedFunc::Args args, PackedFunc::RetValue *rv) {
      ext::test* p = args[0];
      p->name += "!";
      rv->reset(p/*, false*/);
    });

static auto &packedfunc_test_all = Registry<PackedFunc>::Register("test_all")
    .set_body([](PackedFunc::Args args, PackedFunc::RetValue *rv) {
      rv->reset(([](std::vector<PackedFunc> ts) -> std::vector<int> {
        std::vector<int> r;
        std::transform(ts.begin(), ts.end(), std::back_inserter(r), [](auto f) {
          return f();
        });
        return r;
      }) (args[0]));
    });

int test_packedfunc() {
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
  std::string hello_result = Registry<PackedFunc>::Get("append_str")->operator()("hello", "world");
  std::cout << "append_str: " << hello_result << std::endl;

  return 0;
}

int test_func() {
  std::string hello_result = Registry<PackedFunc>::Get("test_append_str")->operator()(*Registry<PackedFunc>::Get("append_str"), "append", "str");
  std::cout << "test_append_str: " << hello_result << std::endl;

  return 0;
}

int test_vector() {
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

int test_ext() {
  ext::test* t = Registry<PackedFunc>::Get("ext_new")->operator()();
  ext::test* hello_result = Registry<PackedFunc>::Get("ext_transform")->operator()(t);
  CHECK_EQ(t, hello_result);
  std::cout << "ext_transform: " << t->name << std::endl;
  Registry<PackedFunc>::Get("ext_release")->operator()(t);
  return 0;
}

int test_all() {
  std::vector<std::function<int()>> ts = { test_packedfunc, test_str, test_func, test_vector, test_ext };
  std::vector<PackedFunc> packed_ts;
  std::transform(ts.begin(), ts.end(), std::back_inserter(packed_ts), [](auto f) -> PackedFunc{
    return PackedFunc{[f](PackedFunc::Args, PackedFunc::RetValue *rv) {
      rv->reset(f());
    }};
  });
  std::vector<int> hello_result = Registry<PackedFunc>::Get("test_all")->operator()(
      PackedManagedVector::create(packed_ts));
  std::cout << "test_all: [";
  for (auto i : hello_result) {
    std::cout << i << ",";
  }
  std::cout << "]" << std::endl;

  return 0;
}

int test() {
  using TestFunc = std::function<int()>;
  //auto not_compiled = _Registry<PackedFunc>();
  // Note: static variable in function would not be initialized until called.
  std::vector<TestFunc> ts = { test_all };
  return std::accumulate(ts.begin(), ts.end(), 0, [](int cur, auto f) {
    return cur + f();
  });
}
