set (CMAKE_CXX_STANDARD 17)

set(sources registry.cc registry.h packedfunc.h)

add_executable(main main.cc ${sources})
