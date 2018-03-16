#pragma once
#include <functional>
#include <utility>
#include <memory>
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
  public:
    union Value {
      int64_t v_int64;
      double v_float64;
      void* v_voidp;
      const char* v_str;
    };
    enum TypeCode {
      kUnknown = 0,
      kInt64 = 1,
      kFloat64 = 2,
      kPtr = 3,
      kStr = 4,
    };

  protected:
    unsigned type_code_;
    Value value_;

  public:
    PackedArg(unsigned type_code, Value value) : type_code_(type_code), value_(value) {}

    unsigned type_code() const { return type_code_; }
    Value value() const { return value_; }

    struct Setter {
      template<typename T,
          typename = typename std::enable_if<
              std::is_integral<T>::value>::type>
      static PackedArg from(T value) {
        // TODO: check numeric_limits
        return {kInt64, PackedArg::Value{.v_int64 = value}};
      }
      static PackedArg from(int64_t value) {
        return {kInt64, PackedArg::Value{.v_int64 = value}};
      }
      static PackedArg from(uint64_t value) {
        CHECK_LE(value, static_cast<uint64_t>(std::numeric_limits<int>::max()));
        return {kPtr, PackedArg::Value{.v_int64 = static_cast<int64_t>(value)}};
      }
      static PackedArg from(double value) {
        return {kFloat64, PackedArg::Value{.v_float64 = value}};
      }
      static PackedArg from(const std::string& value) {
        return {kStr, PackedArg::Value{.v_str = value.c_str()}};
      }
      static PackedArg from(const char* value) {
        return {kStr, PackedArg::Value{.v_str = value}};
      }
    };

    operator int64_t() {
      CHECK_EQ(type_code_, kInt64);
      return value_.v_int64;
    }

    operator uint64_t() {
      CHECK_EQ(type_code_, kInt64);
      CHECK_LE(static_cast<uint64_t>(value_.v_int64), static_cast<uint64_t>(std::numeric_limits<int>::max()));
      return static_cast<uint64_t>(value_.v_int64);
    }

    operator int() {
      CHECK_EQ(type_code_, kInt64);
      CHECK_LE(value_.v_int64, std::numeric_limits<int>::max());
      return value_.v_int64;
    }

    operator double() {
      CHECK_EQ(type_code_, kFloat64);
      return value_.v_float64;
    }

    operator std::string() {
      CHECK_EQ(type_code_, kStr);
      return value_.v_str;
    }
  };

  struct PackedArgSetter {

  };

  struct PackedArgs {
    PackedArgs(const std::vector<PackedArg>& args) : args(args) { }
    PackedArgs(int num_args, unsigned *type_codes, PackedArg::Value *values) {
      for (int i = 0; i < num_args; i++) {
        args.emplace_back(type_codes[i], values[i]);
      }
    }

    std::vector<PackedArg> args;

    PackedArg& operator [](size_t i) {
      return args[i];
    }
  };

  struct PackedManager {
    using Deleter = std::function<void(void*)>;
    explicit PackedManager(Deleter deleter) : deleter(std::move(deleter)) { }
    template <typename T>
    void operator ()(T* p) {
      deleter(static_cast<void*>(p));
    }
    template <typename T>
    static Deleter deleter_for() {
      return [](void*p){ return std::default_delete<T>()(reinterpret_cast<T*>(p)); };
    }
    static std::unique_ptr<void, PackedManager> make(const char* str, int len=-1) {
      return std::unique_ptr<void, PackedManager>(new std::string(str, len), PackedManager(deleter_for<std::string>()));
    };
    static std::unique_ptr<void, PackedManager> make(const std::string& str) {
      return std::unique_ptr<void, PackedManager>(new std::string(str), PackedManager(deleter_for<std::string>()));
    };
    static std::unique_ptr<void, PackedManager> make(nullptr_t) {
      return std::unique_ptr<void, PackedManager>(nullptr, PackedManager(deleter_for<nullptr_t>()));
    };
    Deleter deleter;
  };

  struct PackedRetValue : public PackedArg {
    PackedRetValue() :
        PackedArg(PackedArg::kUnknown, PackedArg::Value{.v_voidp = nullptr}),
        p(PackedManager::make(nullptr)) {}

    template<typename T,
        typename = typename std::enable_if<
            std::is_integral<T>::value>::type>
    PackedRetValue& reset(T value) {
      // TODO: check numeric_limits
      type_code_ = kInt64;
      value_.v_int64 = value;
      return *this;
    }
    PackedRetValue& reset(int64_t value) {
      type_code_ = kInt64;
      value_.v_int64 = value;
      return *this;
    }
    PackedRetValue& reset(uint64_t value) {
      CHECK_LE(value, static_cast<uint64_t>(std::numeric_limits<int>::max()));
      type_code_ = kInt64;
      value_.v_int64 = static_cast<uint64_t>(value);
      return *this;
    }
    PackedRetValue& reset(double value) {
      type_code_ = kFloat64;
      value_.v_float64 = value;
      return *this;
    }
    PackedRetValue& reset(const char* value, bool copy=true) {
      type_code_ = kStr;
      if (copy) {
        p = PackedManager::make(value);
      }
      value_.v_str = reinterpret_cast<std::string*>(p.get())->c_str();
      return *this;
    }
    PackedRetValue& reset(const std::string& value, bool copy=true) {
      type_code_ = kStr;
      if (copy) {
        p = PackedManager::make(value);
      }
      value_.v_str = reinterpret_cast<std::string*>(p.get())->c_str();
      return *this;
    }
    std::unique_ptr<void, PackedManager> p;
  };

  PackedRetValue call_packed(const PackedArgs &args) const {
    PackedRetValue rv;
    body_(args, &rv);
    return std::move(rv);
  }

  template <typename... Args>
  PackedRetValue operator()(Args&& ...args) const {
    const int c_num_args = sizeof...(Args);
    std::vector<PackedArg> packed_args{ PackedArg::Setter::from(std::forward<Args>(args))... };
    return call_packed(PackedArgs(packed_args));
  }

  static void FuncCall(const void* handle, unsigned* type_codes, PackedArg::Value* values, int num_args, PackedArg::Value* ret_val, unsigned* ret_type) {
    PackedArgs args(num_args, type_codes, values);
    // TODO: better memory manager
    static thread_local PackedRetValue rv = reinterpret_cast<const PackedFunc*>(handle)->call_packed(args);
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

}