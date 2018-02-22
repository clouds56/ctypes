#pragma once

#include "base.h"

namespace ctypes {

template <typename T>
struct Registry {
  using Type = T;
  static Type& Register(const std::string &name) {
    Manager* m = Manager::Global();
    std::lock_guard<std::mutex> _lg(m->mutex);
    auto it = m->fmap.find(name);
    if (it == m->fmap.end()) {
      Type* r = new Type();
      r->name_ = name;
      m->fmap[name] = r;
      return *r;
    } else {
      FATAL() << "Global registry " << name << " is already registered in " << T::RegistryName;
      return *it->second;
    }
  }

  static const Type* Get(const std::string &name) {
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

  struct Manager {
    static Manager* Global() {
      static Manager inst;
      return &inst;
    }

    std::map<std::string, Type*> fmap;
    std::mutex mutex;
  };

  std::string name_;
};

}
