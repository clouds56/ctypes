#include "api.h"
#include "packedfunc.h"

using namespace ctypes;

static_assert(sizeof(packedvalue_handle) == sizeof(PackedValue));

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

int CTIPackedFuncCall(const void* handle, int num_args, const unsigned* type_codes, const packedvalue_handle* values,
    OUT unsigned* ret_type, OUT packedvalue_handle* ret_val) {
  PackedFunc::FuncCall(handle, num_args, static_cast<const PackedType*>(type_codes), reinterpret_cast<const PackedValue*>(values),
      static_cast<PackedType*>(ret_type), reinterpret_cast<PackedValue*>(ret_val));
  return CTI_SUCCESS;
}
