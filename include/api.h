#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define CTI_EXPORT
#define OUT

#define CTI_SUCCESS 0

typedef void* func_handle;

CTI_EXPORT int CTIRegistryListNames(const char* tag, OUT int* ret_size, OUT const char*** ret_names);

CTI_EXPORT int CTIRegistryGet(const char* tag, const char* name, OUT func_handle* ret_handle);

#ifdef __cplusplus
}
#endif
