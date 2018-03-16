#pragma once
#include <functional>
#include <utility>
#include <memory>
#include "registry.h"

namespace ctypes {

struct PackedFunc;

namespace PackedTypeCode {
enum PackedType_ {
  kUnknown = 0,
  kInt64 = 1,
  kFloat64 = 2,
  kPtr = 3,
  kStr = 4,
  kFunc = 5,
};
}

using PackedType = unsigned;

union PackedValue {
  int64_t v_int64;
  double v_float64;
  void* v_voidp;
  const char* v_str;
  const PackedFunc* v_func;
};

struct PackedArg {
  PackedType type_code;
  PackedValue value;
};

struct PackedFunc {
  struct Args;
  struct RetValue;
  using FType = std::function<void (Args args, RetValue* rv)>;

  PackedFunc() = default;
  explicit PackedFunc(FType body) : body_(std::move(body)) { }

  FType body_;

  struct Arg {
  protected:
    PackedArg content_;
  public:
    Arg(PackedType type_code, PackedValue value) : content_(PackedArg{type_code, value}) {}

    PackedType type_code() const { return content_.type_code; }
    PackedValue value() const { return content_.value; }

    template<typename T,
        typename = typename std::enable_if<
            std::is_integral<T>::value>::type>
    static Arg from(T value) {
      // TODO: check numeric_limits
      return {PackedTypeCode::kInt64, PackedValue{.v_int64 = value}};
    }
    static Arg from(int64_t value) {
      return {PackedTypeCode::kInt64, PackedValue{.v_int64 = value}};
    }
    static Arg from(uint64_t value) {
      CHECK_LE(value, static_cast<uint64_t>(std::numeric_limits<int>::max()));
      return {PackedTypeCode::kPtr, PackedValue{.v_int64 = static_cast<int64_t>(value)}};
    }
    static Arg from(double value) {
      return {PackedTypeCode::kFloat64, PackedValue{.v_float64 = value}};
    }
    static Arg from(const std::string& value) {
      return {PackedTypeCode::kStr, PackedValue{.v_str = value.c_str()}};
    }
    static Arg from(const char* value) {
      return {PackedTypeCode::kStr, PackedValue{.v_str = value}};
    }
    static Arg from(const PackedFunc& value) {
      return {PackedTypeCode::kFunc, PackedValue{.v_func = &value}};
    }

    operator int64_t() {
      CHECK_EQ(type_code(), PackedTypeCode::kInt64);
      return value().v_int64;
    }

    operator uint64_t() {
      CHECK_EQ(type_code(), PackedTypeCode::kInt64);
      CHECK_LE(static_cast<uint64_t>(value().v_int64), static_cast<uint64_t>(std::numeric_limits<int>::max()));
      return static_cast<uint64_t>(value().v_int64);
    }

    operator int() {
      CHECK_EQ(type_code(), PackedTypeCode::kInt64);
      CHECK_LE(value().v_int64, std::numeric_limits<int>::max());
      return value().v_int64;
    }

    operator double() {
      CHECK_EQ(type_code(), PackedTypeCode::kFloat64);
      return value().v_float64;
    }

    operator std::string() {
      CHECK_EQ(type_code(), PackedTypeCode::kStr);
      return value().v_str;
    }

    operator PackedFunc() {
      CHECK_EQ(type_code(), PackedTypeCode::kFunc);
      return *value().v_func;
    }
  };

  struct Args {
    Args(const std::vector<Arg>& args) : args(args) { }
    Args(int num_args, PackedType* type_codes, PackedValue* values) {
      for (int i = 0; i < num_args; i++) {
        args.emplace_back(type_codes[i], values[i]);
      }
    }

    std::vector<Arg> args;

    Arg& operator [](size_t i) {
      return args[i];
    }
  };

  struct Manager {
    using PtrType = std::unique_ptr<void, Manager>;
    using Deleter = std::function<void(void*)>;
    explicit Manager(Deleter deleter) : deleter(std::move(deleter)) { }
    template <typename T>
    void operator ()(T* p) {
      deleter(static_cast<void*>(p));
    }
    template <typename T>
    static Deleter deleter_for() {
      return [](void*p){ return std::default_delete<T>()(reinterpret_cast<T*>(p)); };
    }
    static std::unique_ptr<void, Manager> make(const char* str, int len=-1) {
      return std::unique_ptr<void, Manager>(new std::string(str, len), Manager(deleter_for<std::string>()));
    };
    template <typename T>
    static std::unique_ptr<void, Manager> make(const T& v) {
      return std::unique_ptr<void, Manager>(new T(v), Manager(deleter_for<T>()));
    };
    static std::unique_ptr<void, Manager> make(nullptr_t) {
      return std::unique_ptr<void, Manager>(nullptr, Manager(deleter_for<nullptr_t>()));
    };
    Deleter deleter;
  };

  struct RetValue : public Arg {
  private:
    Manager::PtrType p;
  public:
    RetValue() :
        Arg(PackedTypeCode::kUnknown, PackedValue{.v_voidp = nullptr}),
        p(Manager::make(nullptr)) {}


    RetValue& switch_to(PackedType type_code, PackedValue value, bool managed=false) {
      if (!managed) {
        p = Manager::make(nullptr);
      }
      content_.type_code = type_code;
      content_.value = value;
      return *this;
    };

    template<typename T,
        typename = typename std::enable_if<
            std::is_integral<T>::value>::type>
    RetValue& reset(T value) {
      // TODO: check numeric_limits
      return switch_to(PackedTypeCode::kInt64, PackedValue{.v_int64 = value});
    }
    RetValue& reset(int64_t value) {
      return switch_to(PackedTypeCode::kInt64, PackedValue{.v_int64 = value});
    }
    RetValue& reset(uint64_t value) {
      CHECK_LE(value, static_cast<uint64_t>(std::numeric_limits<int>::max()));
      return switch_to(PackedTypeCode::kInt64, PackedValue{.v_int64 = static_cast<int64_t>(value)});
    }
    RetValue& reset(double value) {
      return switch_to(PackedTypeCode::kFloat64, PackedValue{.v_float64 = value});
    }
    RetValue& reset(const char* value, bool copy=true) {
      if (copy) {
        p = Manager::make(value);
        value = reinterpret_cast<std::string*>(p.get())->c_str();
      }
      return switch_to(PackedTypeCode::kStr, PackedValue{.v_str = value}, copy);
    }
    RetValue& reset(const std::string& value, bool copy=true) {
      const char* value_ = value.c_str();
      if (copy) {
        p = Manager::make(value);
        value_ = reinterpret_cast<std::string*>(p.get())->c_str();
      }
      return switch_to(PackedTypeCode::kStr, PackedValue{.v_str = value_}, copy);
    }
    RetValue& reset(const PackedFunc& value, bool copy=true) {
      const PackedFunc* value_ = &value;
      if (copy) {
        p = Manager::make(value);
        value_ = reinterpret_cast<PackedFunc*>(p.get());
      }
      return switch_to(PackedTypeCode::kFunc, PackedValue{.v_func = value_}, copy);
    }
    RetValue& reset(RetValue&& other) {
      p = std::move(other.p);
      return switch_to(other.content_.type_code, other.content_.value);
    }
  };

  RetValue call_packed(const Args &args) const {
    RetValue rv;
    body_(args, &rv);
    return std::move(rv);
  }

  template <typename... ArgTps>
  RetValue operator()(ArgTps&& ...args) const {
    const int c_num_args = sizeof...(ArgTps);
    std::vector<Arg> packed_args{ Arg::from(std::forward<ArgTps>(args))... };
    return call_packed(Args(packed_args));
  }

  static void FuncCall(const void* handle, PackedType* type_codes, PackedValue* values, int num_args, PackedValue* ret_val, PackedType* ret_type) {
    Args args(num_args, type_codes, values);
    // TODO: better memory manager
    static thread_local RetValue rv;
    rv.reset(reinterpret_cast<const PackedFunc*>(handle)->call_packed(args));
    *ret_type = rv.type_code();
    *ret_val = rv.value();
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

template <>
const std::string Registry<PackedFunc>::RegistryName_ = "PackedFunc";

}
