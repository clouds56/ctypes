#include "api.h"
#include "packedfunc.h"

using namespace ctypes;

int CTIRegistryListNames(const char* tag, OUT int* ret_size, OUT const char *** ret_names) {
  static thread_local std::vector<std::string> names;
  static thread_local std::vector<const char*> names_p;
  CHECK_EQ(tag, Registry<PackedFunc>::RegistryName_);
  names = Registry<PackedFunc>::ListNames();
  names_p.clear();
  names_p.reserve(names.size());
  std::transform(names.begin(), names.end(), std::back_inserter(names_p), [](const auto& s){
    return s.c_str();
  });
  *ret_size = names_p.size();
  *ret_names = names_p.data();
  return CTI_SUCCESS;
}

int CTIRegistryGet(const char* tag, const char* name, OUT func_handle* ret_handle) {
  CHECK_EQ(tag, Registry<PackedFunc>::RegistryName_);
  *ret_handle = Registry<PackedFunc>::Get(name);
  return CTI_SUCCESS;
}
