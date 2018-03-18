#pragma once

#include <string>
#include "packedfunc.h"

namespace ext {

struct test {
  std::string name = "run";
};

}

template <>
struct ctypes::PackedTypeCode::Extension<ext::test> {
  static constexpr ctypes::PackedType type_code = 32;
  //static constexpr bool copy_ret = false;
};
