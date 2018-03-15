#pragma once

#include "base.h"

namespace ctypes {

template <typename T>
struct Registry {
public:
  using Type = T;

private:
  struct Manager {
    static Manager* Global() {
      static Manager inst;
      return &inst;
    }

    std::map<std::string, Type*> fmap;
    std::mutex mutex;
  };

protected:
  Registry() = default;
  std::string name_;

public:
  Registry(const Registry&) = delete;

  static Registry& Register(const std::string &name) {
    Manager* m = Manager::Global();
    std::lock_guard<std::mutex> _lg(m->mutex);
    auto it = m->fmap.find(name);
    if (it == m->fmap.end()) {
      Type* r = new Type();
      r->name_ = name;
      m->fmap[name] = r;
      return *r;
    } else {
      FATAL() << "Global registry " << name << " is already registered in " << Type::RegistryName;
      return *it->second;
    }
  }

  static const Registry* Get(const std::string &name) {
    Manager* m = Manager::Global();
    std::lock_guard<std::mutex> _lg(m->mutex);
    auto it = m->fmap.find(name);
    if (it == m->fmap.end()) {
      return nullptr;
    } else {
      return m->fmap[name];
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

  Type* operator->() {
    return static_cast<Type*>(this);
  }

  inline std::string name() const {
    return name_;
  }
};

}
