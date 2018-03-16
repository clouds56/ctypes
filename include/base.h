#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <cstring>

class log_stream {
public:
  log_stream() : o(std::cerr), valid(false) {};
  log_stream(log_stream&& ls) noexcept : o(ls.o), valid(ls.valid) {
    ls.valid = false;
  }
  explicit log_stream(std::ostream& o) : o(o), valid(true) {};
  ~log_stream() {
    if (valid) {
      o << std::endl;
    }
  }
  template <typename T>
  log_stream&& operator<<(T&& i) && {
    if (valid) {
      o << std::forward<T>(i);
    }
    return std::move(*this);
  }
  std::ostream& o;
  bool valid;
};

#define CHECK_(c, b) check(__FILE__, __LINE__, c, b)
#define CHECK(b) CHECK_(#b, b)
#define CHECK_EQ(p, q) (CHECK_(#p " == " #q, (p) == (q)) << "get " << (p) << " expect " << (q) << " ")
#define CHECK_LE(p, q) (CHECK_(#p " <= " #q, (p) <= (q)) << "get " << (p) << " expect " << (q) << " ")

#define FATAL() fatal(__FILE__, __LINE__)
inline const char* filename_trim(const char* file) {
  const char* const i = strrchr(file, '/');
  if (i == file) {
    return file;
  }
  return i+1;
}
inline log_stream line_info(std::ostream& o, const char* file, int line) {
  return log_stream(o) << filename_trim(file) << ":" << line << " ";
}
inline log_stream fatal(const char* file, int line) {
  return line_info(std::cerr, file, line);
}

inline log_stream check(const char* file, int line, const char* condition, bool b) {
  if (!b) {
    return line_info(std::cerr, file, line) << "check [" << condition << "] failed ";
  }
  return log_stream();
}
