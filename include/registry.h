#pragma once

#include "base.h"

namespace ctypes {

template <typename T>
struct Registry;

template <typename T>
struct _Registry_Base {
protected:
  _Registry_Base() = default;
public:
  using Type = T;
  using RegistryType = Registry<Type>;
  virtual Type* get() = 0;
};

template <typename T>
struct _Registry : public _Registry_Base<T> {
public:
  using Type = T;
  using typename _Registry_Base<Type>::RegistryType;
protected:
  Type content_;
  // TODO: is it possible make this always protected?
  _Registry() = default;
public:
  RegistryType& set_content(Type content) {
    content_ = std::move(content);
    return *dynamic_cast<RegistryType*>(this);
  }
  Type* get() override {
    return &content_;
  }
};

template <typename T>
struct Registry : public _Registry<T> {
private:
  struct Manager {
    static Manager* Global() {
      static Manager inst;
      return &inst;
    }

    std::map<std::string, Registry*> fmap;
    std::mutex mutex;
  };

  std::string name_;

public:
  static const std::string RegistryName_;
  static std::string RegistryName() {
    return "Registry<" + RegistryName_ + ">";
  }
  using Type = T;
  static Registry& Register(const std::string &name) {
    Manager* m = Manager::Global();
    std::lock_guard<std::mutex> _lg(m->mutex);
    auto it = m->fmap.find(name);
    if (it == m->fmap.end()) {
      auto r = new Registry();
      r->name_ = name;
      m->fmap[name] = r;
      return *r;
    } else {
      FATAL() << "Global registry " << name << " is already registered in " << RegistryName_;
      return *it->second;
    }
  }

  static Type* Get(const std::string &name) {
    Manager* m = Manager::Global();
    std::lock_guard<std::mutex> _lg(m->mutex);
    auto it = m->fmap.find(name);
    if (it == m->fmap.end()) {
      return nullptr;
    } else {
      return it->second->get();
    }
  }

  static std::vector<std::string> ListNames() {
    Manager* m = Manager::Global();
    std::lock_guard<std::mutex> _lg(m->mutex);
    std::vector<std::string> keys;
    keys.reserve(m->fmap.size());
    for (const auto &kv : m->fmap) {
      keys.push_back(kv.first);
    }
    return keys;
  }

  inline std::string name() const {
    return name_;
  }
};

}
