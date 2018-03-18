#pragma once

#include <functional>
#include <utility>
#include <memory>
#include "registry.h"

namespace ctypes {

struct PackedFunc;
struct PackedVector;

using PackedType = unsigned;

namespace PackedTypeCode {
enum PackedType_ {
  kUnknown = 0,
  kInt64 = 1,
  kFloat64 = 2,
  kPtr = 3,
  kStr = 4,
  kFunc = 5,
  kVector = 6,
};

enum PackedExtType_ {
  kExtStart = 32,
};

template <unsigned _code>
struct is_ext {
  static constexpr bool value = _code >= kExtStart;
};

template <unsigned _code, unsigned _transform_code=_code, bool _copy_default=true>
struct _TypeCode {
  static constexpr PackedType code() { return _code; }
  static constexpr PackedType transform_code() { return _transform_code; }
  static constexpr bool copy() { return _copy_default; }
};

template <typename T, typename Enabled = void>
struct TypeCode : _TypeCode<kUnknown> { };

template <typename T>
struct TypeCode<T, typename std::enable_if_t<std::is_integral<T>::value>>
    : _TypeCode<kUnknown, kInt64> { };

template <> struct TypeCode<int64_t> : _TypeCode<kInt64> { };
template <> struct TypeCode<uint64_t> : _TypeCode<kInt64> { };
template <> struct TypeCode<double> : _TypeCode<kFloat64> { };
template <> struct TypeCode<const char*> : _TypeCode<kStr> { };
template <> struct TypeCode<std::string> : _TypeCode<kUnknown, kStr> { };
template <> struct TypeCode<PackedFunc> : _TypeCode<kFunc, kFunc> { };
template <typename T> struct TypeCode<std::vector<T>> : _TypeCode<kVector, kVector> { };

template <typename T>
struct is_ext_type : is_ext<TypeCode<T>::code()> {};

template <typename T>
struct Extension {
  static constexpr ctypes::PackedType type_code = ctypes::PackedTypeCode::kUnknown;
  static constexpr bool copy_ret = false;
};

template <typename T, typename = bool>
constexpr bool Extension_copy = Extension<void>::copy_ret;

template <typename T>
constexpr bool Extension_copy<T, decltype(Extension<T>::copy_ret)> = Extension<T>::copy_ret;

template <typename T>
struct TypeCode<T, typename std::enable_if_t<is_ext<Extension<T>::type_code>::value>>
    : _TypeCode<Extension<T>::type_code, kPtr, Extension_copy<T>> { };

}

union PackedValue {
  int64_t v_int64;
  double v_float64;
  void* v_voidp;
  const char* v_str;
  const PackedFunc* v_func;
  const PackedVector* v_vec;
};

struct PackedArg {
  PackedType type_code;
  PackedValue value;
};

struct PackedVector {
  PackedValue* data;
  size_t size;
  PackedType type_code;
};

struct PackedManagedVector {
  using ManagedType = std::vector<PackedValue>;
  using ManagedNestedType = std::vector<PackedManagedVector>;
  PackedVector content{};
  ManagedType* ptr = nullptr;
  ManagedNestedType* nested_ptr = nullptr;
  PackedManagedVector(const PackedManagedVector&) = delete;
  PackedManagedVector(PackedManagedVector&& other) noexcept : content(other.content), ptr(other.ptr), nested_ptr(other.nested_ptr) {
    other.ptr = nullptr;
    other.nested_ptr = nullptr;
  }
  PackedManagedVector(PackedType type_code, ManagedType* ptr, ManagedNestedType* nested_ptr=nullptr) : ptr(ptr), nested_ptr(nested_ptr) {
    content.size = ptr->size();
    content.data = ptr->data();
    content.type_code = type_code;
  }

  ~PackedManagedVector() {
    delete ptr;
    delete nested_ptr;
  }

  template <typename T>
  static PackedManagedVector create(const std::vector<T>& vec) {
    const PackedType type_code = PackedTypeCode::TypeCode<T>::transform_code();
    static_assert(type_code != PackedTypeCode::kUnknown);
    static_assert(type_code != PackedTypeCode::kVector);
    auto ptr = std::make_unique<ManagedType>();
    ptr->reserve(vec.size());
    std::transform(vec.begin(), vec.end(), std::back_inserter(*ptr), [](auto&& i){ return transform(i); });
    return PackedManagedVector(type_code, ptr.release());
  }

  template <typename T>
  static PackedManagedVector create(const std::vector<std::vector<T>>& vec) {
    const PackedType type_code = PackedTypeCode::TypeCode<std::vector<T>>::transform_code();
    static_assert(type_code == PackedTypeCode::kVector);
    auto ptr = std::make_unique<ManagedType>();
    auto nested_ptr = std::make_unique<ManagedNestedType>();;
    ptr->reserve(vec.size());
    nested_ptr->reserve(vec.size());
    for (const auto &i : vec) {
      nested_ptr->push_back(create(i));
      ptr->push_back(transform(nested_ptr->back().content));
    }
    return PackedManagedVector(type_code, ptr.release(), nested_ptr.release());
  }
  template <typename T>
  static PackedValue transform(T&& i);
//  template <typename T>
//  static PackedValue transform(const std::vector<T>& i);
};

struct PackedFunc {
  struct Args;
  struct RetValue;
  using FType = std::function<void (Args args, RetValue* rv)>;

  PackedFunc() = default;
  explicit PackedFunc(FType body) : body_(body) { }

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
    static Arg from(const PackedVector& value) {
      return {PackedTypeCode::kVector, PackedValue{.v_vec = &value}};
    }
    static Arg from(const PackedManagedVector& value) {
      return {PackedTypeCode::kVector, PackedValue{.v_vec = &value.content}};
    }
    template <typename T, unsigned _type_code = PackedTypeCode::TypeCode<T>::code(), typename = std::enable_if_t<PackedTypeCode::is_ext<_type_code>::value> >
    static Arg from(const T* value) {
      static_assert(PackedTypeCode::TypeCode<T>::transform_code() == PackedTypeCode::kPtr);
      return {_type_code, PackedValue{.v_voidp = const_cast<T*>(value)}};
    }

    template<typename T,
        typename = typename std::enable_if<
            std::is_integral<T>::value>::type>
    operator T() {
      // TODO: check numeric_limits
      CHECK_EQ(type_code(), PackedTypeCode::kInt64);
      return value().v_int64;
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
      return static_cast<int>(value().v_int64);
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

    operator PackedVector() {
      CHECK_EQ(type_code(), PackedTypeCode::kVector);
      return *value().v_vec;
    }

    template <typename T>
    operator std::vector<T>() {
      CHECK_EQ(type_code(), PackedTypeCode::kVector);
      std::vector<T> r;
      CHECK_EQ(value().v_vec->type_code, PackedTypeCode::TypeCode<T>::transform_code());
      r.reserve(value().v_vec->size);
      for (size_t i = 0; i < value().v_vec->size; i++) {
        r.push_back(Arg(value().v_vec->type_code, value().v_vec->data[i]).operator T());
      }
      return r;
    };

    template <typename T, typename = std::enable_if_t<PackedTypeCode::is_ext_type<T>::value> >
    operator T*() {
      const PackedType _type_code = PackedTypeCode::TypeCode<T>::code();
      static_assert(PackedTypeCode::TypeCode<T>::transform_code() == PackedTypeCode::kPtr);
      CHECK_EQ(type_code(), _type_code);
      return reinterpret_cast<T*>(value().v_voidp);
    }
  };

  struct Args {
    Args(const std::vector<Arg>& args) : args(args) { }
    Args(int num_args, const PackedType* type_codes, const PackedValue* values) {
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
    using Copy = std::function<void*(const void*)>;
    using Deleter = std::function<void(void*)>;
    explicit Manager(Deleter deleter) : deleter(std::move(deleter)) { }
    template <typename T>
    void operator ()(T* p) {
      deleter(static_cast<void*>(p));
    }

    template <typename T>
    inline static Deleter deleter_for() {
      return [](void*p){ return std::default_delete<T>()(reinterpret_cast<T*>(p)); };
    }
    template <typename T>
    inline static Copy copy_for() {
      return [](const void*p) { return new T(*reinterpret_cast<const T*>(p)); };
    }

    static std::unique_ptr<void, Manager> make(const char* str, int len=-1) {
      return std::unique_ptr<void, Manager>(new std::string(str, len), Manager(deleter_for<std::string>()));
    };
    template <typename T>
    static std::unique_ptr<void, Manager> make(const T& v, Deleter deleter = deleter_for<T>()) {
      return std::unique_ptr<void, Manager>(new T(v), Manager(deleter));
    };
    template <typename T>
    static std::unique_ptr<void, Manager> make(const T* v, Deleter deleter = deleter_for<T>()) {
      return std::unique_ptr<void, Manager>(new T(*v), Manager(deleter));
    };
    template <typename T>
    static std::unique_ptr<void, Manager> make(const T& v, Copy copy, Deleter deleter = deleter_for<T>()) {
      return std::unique_ptr<void, Manager>(copy(&v), Manager(deleter));
    };
    template <typename T>
    static std::unique_ptr<void, Manager> make(const T* v, Copy copy, Deleter deleter = deleter_for<T>()) {
      return std::unique_ptr<void, Manager>(copy(v), Manager(deleter));
    };
    static std::unique_ptr<void, Manager> make(std::nullptr_t) {
      return std::unique_ptr<void, Manager>(nullptr, Manager(deleter_for<std::nullptr_t>()));
    };
    static std::unique_ptr<void, Manager> make(PackedManagedVector vec) {
      return std::unique_ptr<void, Manager>(new PackedManagedVector(std::move(vec)), Manager(deleter_for<PackedManagedVector>()));
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
    template <typename T>
    RetValue& reset(const std::vector<T>& value, bool copy_=true) {
      const bool copy = true;
      static_assert(copy);
      const PackedVector* value_ = nullptr;
      if (copy) {
        p = Manager::make(PackedManagedVector::create(value));
        value_ = &reinterpret_cast<PackedManagedVector*>(p.get())->content;
      }
      return switch_to(PackedTypeCode::kVector, PackedValue{.v_vec = value_}, copy);
    }
    template <typename T, unsigned _type_code = PackedTypeCode::TypeCode<T>::code(), typename = std::enable_if_t<PackedTypeCode::is_ext<_type_code>::value> >
    RetValue& reset(const T* value, bool copy=PackedTypeCode::TypeCode<T>::copy()) {
      if (copy) {
        p = Manager::make(value);
        value = reinterpret_cast<T*>(p.get());
      }
      return switch_to(_type_code, PackedValue{.v_voidp = const_cast<T*>(value)}, copy);
    }
    RetValue& reset(RetValue&& other) {
      p = std::move(other.p);
      return switch_to(other.content_.type_code, other.content_.value, p!=nullptr);
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

  static void FuncCall(const void* handle, int num_args, const PackedType* type_codes, const PackedValue* values, PackedType* ret_type, PackedValue* ret_val) {
    Args args(num_args, type_codes, values);
    // TODO: better memory manager
    static thread_local RetValue rv;
    rv.reset(reinterpret_cast<const PackedFunc*>(handle)->call_packed(args));
    *ret_type = rv.type_code();
    *ret_val = rv.value();
  }
};

template <typename T>
PackedValue PackedManagedVector::transform(T&& i) {
  return PackedFunc::Arg::from(i).value();
}

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

template <> const std::string Registry<PackedFunc>::RegistryName_;

}
