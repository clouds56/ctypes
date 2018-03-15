#pragma once
#include <functional>
#include <utility>
#include "registry.h"

namespace ctypes {

struct PackedFunc {
  struct PackedArgs;
  struct PackedRetValue;
  static const std::string RegistryName;
  using FType = std::function<void (PackedArgs args, PackedRetValue* rv)>;

  PackedFunc() = default;
  explicit PackedFunc(FType body) : body_(std::move(body)) { }

  FType body_;

  struct PackedArg {
    unsigned type_code;
    union Value {
      int64_t v_int64;
      double v_float64;
      void* v_voidp;
      const char* v_str;
    } value;
    enum TypeCode {
      kUnknown = 0,
      kInt64 = 1,
      kFloat64 = 2,
      kPtr = 3,
      kStr = 4,
    };

    PackedArg(unsigned type_code, Value value) : type_code(type_code), value(value) {}
  };

  struct PackedArgSetter {

  };

  struct PackedArgs {
    PackedArgs(int num_args, unsigned *type_codes, PackedArg::Value *values) {
      for (int i = 0; i < num_args; i++) {
        args.emplace_back(type_codes[i], values[i]);
      }
    }

    std::vector<PackedArg> args;
  };

  struct PackedRetValue : PackedArg {
    PackedRetValue() : PackedArg(PackedArg::kUnknown, PackedArg::Value{.v_voidp = nullptr}) {}
  };

  PackedRetValue call(const PackedArgs &args) {
    PackedRetValue rv;
    return rv;
  }

  static void FuncCall(void* handle, unsigned* type_codes, PackedArg::Value* values, int num_args, PackedArg::Value* ret_val, unsigned* ret_type) {
    PackedArgs args(num_args, type_codes, values);
    PackedRetValue rv = reinterpret_cast<PackedFunc*>(handle)->call(args);
    *ret_type = rv.type_code;
    *ret_val = rv.value;
  }
};

template<>
struct _Registry<PackedFunc> : public _Registry_Base<PackedFunc> {
public:
  using Type = PackedFunc;
  using typename _Registry_Base<Type>::RegistryType;
protected:
  Type content_;
  _Registry() = default;
public:
  RegistryType& set_body(PackedFunc::FType content) {
    content_ = PackedFunc(std::move(content));
    // TODO: dynamic_cast is not that safe here?
    return *dynamic_cast<RegistryType*>(this);
  }

  PackedFunc* get() override {
    return &content_;
  }
};

}
