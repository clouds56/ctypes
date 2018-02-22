#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <map>

#define FATAL() fatal(__FILE__, __LINE__)
inline std::ostream& fatal(const char* file, int line) {
  return std::cerr << file << ":" << line << " ";
}
