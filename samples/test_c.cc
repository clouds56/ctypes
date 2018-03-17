#include "api.h"
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include "packedfunc.h"

namespace ctypes {
namespace client {

#define CHECK_CTI(x) (x)

std::vector<std::string> ListNames(const std::string& tag) {
  const char** names;
  int size;
  CHECK_CTI(CTIRegistryListNames(tag.c_str(), &size, &names));
  std::vector<std::string> ret;
  ret.reserve(size);
  // std::copy(names, names + size, std::back_inserter(ret));
  std::transform(names, names + size, std::back_inserter(ret), [](auto i){ return i; });
  return ret;
}

struct PackedFunc {
  using FType = func_handle;
  FType handle;
  PackedFunc(FType handle) : handle(handle) { }

  struct RetValue {
    unsigned type_code;
    packedvalue_handle value;
    template <typename T>
    operator T() {
      return ctypes::PackedFunc::Arg(type_code, *reinterpret_cast<ctypes::PackedValue*>(&value)).operator T();
    }
  };

  template <typename... ArgTps>
  RetValue operator()(ArgTps&& ...args) {
    const int c_num_args = sizeof...(ArgTps);
    std::vector<ctypes::PackedFunc::Arg> cti_args{ ctypes::PackedFunc::Arg::from(std::forward<ArgTps>(args))... };
    std::vector<PackedType> type_codes;
    std::vector<PackedValue> values;
    type_codes.reserve(c_num_args);
    values.reserve(c_num_args);
    std::transform(cti_args.begin(), cti_args.end(), std::back_inserter(type_codes), [](const auto& i){ return i.type_code(); });
    std::transform(cti_args.begin(), cti_args.end(), std::back_inserter(values), [](const auto& i){ return i.value(); });

    RetValue ret;
    CHECK_CTI(CTIPackedFuncCall(handle, c_num_args, static_cast<const unsigned*>(type_codes.data()), reinterpret_cast<const packedvalue_handle*>(values.data()),
        static_cast<unsigned*>(&ret.type_code), reinterpret_cast<packedvalue_handle*>(&ret.value)));
    return ret;
  }
};

PackedFunc Get(const std::string& tag, const std::string& name) {
  func_handle handle;
  CHECK_CTI(CTIRegistryGet(tag.c_str(), name.c_str(), &handle));
  return PackedFunc(handle);
}

}
}

using namespace ctypes;

static int test_packed() {
  auto names = client::ListNames("PackedFunc");
  std::cout << "client::ListNames: ";
  for (auto i : names) {
    std::cout << i << ",";
  }
  std::cout << std::endl;
  return 0;
}

static int test_func() {
  auto f = client::Get("PackedFunc", "hello");
  std::cout << "hello: " << f(1, 2).operator int() << std::endl;
  return 0;
}

int test_c() {
  return test_packed() || test_func();
}
