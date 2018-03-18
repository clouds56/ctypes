cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

file(GLOB sources src/*.cc include/*.h)
add_library(ctypes STATIC ${sources})
target_include_directories(ctypes PUBLIC include)
set_property(TARGET ctypes PROPERTY POSITION_INDEPENDENT_CODE ON)

file(GLOB test_sources samples/test*.cc samples/ext.h)

add_library(test_ctypes SHARED ${test_sources})
target_link_libraries(test_ctypes ctypes)

add_executable(main samples/main.cc)
target_link_libraries(main ctypes test_ctypes)
