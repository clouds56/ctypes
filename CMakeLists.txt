cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

file(GLOB sources src/*.cc include/*.h)
add_library(ctypes STATIC ${sources})
target_include_directories(ctypes PUBLIC include)

add_executable(main samples/main.cc samples/test.cc samples/test_c.cc)
target_link_libraries(main ctypes)
