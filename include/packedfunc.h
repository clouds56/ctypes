#pragma once

#include "registry.h"

namespace ctypes {

struct PackedFunc : public Registry<PackedFunc> {
    static const std::string RegistryName;
};

}
