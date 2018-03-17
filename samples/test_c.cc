#include "api.h"
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

namespace ctypes {
namespace client {

std::vector<std::string> ListNames(const std::string& tag) {
  const char** names;
  int size;
  CTIRegistryListNames(tag.c_str(), &size, &names);
  std::vector<std::string> ret;
  ret.reserve(size);
  // std::copy(names, names + size, std::back_inserter(ret));
  std::transform(names, names + size, std::back_inserter(ret), [](auto i){ return i; });
  return ret;
}

}
}

using namespace ctypes;

int test_packed() {
  auto names = client::ListNames("PackedFunc");
  std::cout << "client::ListNames: ";
  for (auto i : names) {
    std::cout << i << ",";
  }
  std::cout << std::endl;
  return 0;
}

int test_c() {
  return test_packed();
}
