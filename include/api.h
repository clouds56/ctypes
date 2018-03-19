#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define CTI_EXPORT
#define OUT

#define CTI_SUCCESS 0

typedef void* func_handle;
// typedef void* packedvalue_handle;
typedef union {
  int64_t v_int64;
  double v_float64;
  void* v_voidp;
  const char* v_str;
} packedvalue_handle;

CTI_EXPORT int CTIRegistryListNames(const char* tag, OUT size_t* ret_size, OUT const char*** ret_names);

CTI_EXPORT int CTIRegistryGet(const char* tag, const char* name, OUT func_handle* ret_handle);

CTI_EXPORT int CTIPackedFuncCall(const void* handle, size_t num_args, const unsigned* type_codes, const packedvalue_handle* values,
    OUT unsigned* ret_type, OUT packedvalue_handle* ret_val);

#ifdef __cplusplus
}
#endif
