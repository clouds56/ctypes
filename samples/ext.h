#pragma once

#include <string>
#include "packedfunc.h"

namespace ext {

struct test {
  std::string name = "run";
};

template <typename T>
struct PackedExtension {
  static constexpr ctypes::PackedType type_code = ctypes::PackedTypeCode::kUnknown;
};

template <>
struct PackedExtension<test> {
  static constexpr ctypes::PackedType type_code = 32;
};

}

namespace ctypes {

namespace PackedTypeCode {

template <typename T>
struct TypeCode<T, typename std::enable_if_t<is_ext<ext::PackedExtension<T>::type_code>::value>>
    : _TypeCode<ext::PackedExtension<T>::type_code, kPtr> { };
}

}
