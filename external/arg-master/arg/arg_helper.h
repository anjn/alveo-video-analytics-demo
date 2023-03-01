#pragma once

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept> // std::invalid_argument
#include <cerrno>    // errno
#include <climits>   // LONG_MAX, LONGMIN

namespace arg {
namespace helper {

template<typename T>
struct option {
};

struct util {
  static long long int parse_integer(const std::string& value) {
    const char* str = value.c_str();
    char* endptr;
    if (value.empty())
      throw std::invalid_argument("empty");
    errno = 0;
#if __cplusplus >= 201103L && !defined(__MSYS__)
    long long int v = std::strtoll(str, &endptr, 0);
#else
    long long int v = std::strtol(str, &endptr, 0);
#endif
    if (errno == EINVAL)
      throw std::invalid_argument("invalid input");
    if (errno == ERANGE)
      throw std::invalid_argument("out of range");
    if (endptr != str+value.size())
      throw std::invalid_argument("invalid input");
    return v;
  }

  template<typename T>
  static std::vector<T> parse_list(std::string str) {
    typedef std::string::iterator itor;
    std::vector<T> result;
    itor head, pos;
    head = pos = str.begin();
    do {
      if (pos == str.end() || *pos == ',') {
        result.push_back(option<T>::cast(std::string(head,pos)));
        head = pos+1;
      }
    } while (pos++ != str.end());
    return result;
  }

  static std::string replace_all(const std::string& str, char from, char to) {
    std::string res = str;
    for (int i=0, n=res.size(); i<n; i++) if (res[i] == from) res[i] = to;
    return res;
  }

  template<typename T>
  static void error(const T& message, bool exit=true) {
    std::cerr << "error: " << message << std::endl;
    if (exit) std::exit(1);
  }
};

template<>
struct option<bool> {
  static bool cast(const std::string& value) { return true; }
  static bool require_value() { return false; }
  static std::string name() { return ""; }
};

template<>
struct option<int> {
  static int cast(const std::string& value) {
    long long v = util::parse_integer(value);
    if (v > INT_MAX || v < INT_MIN) throw std::invalid_argument("out of range");
    return (int) v;
  }
  static bool require_value() { return true; }
  static std::string name() { return "<integer>"; }
};

template<>
struct option<long int> {
  static long int cast(const std::string& value) {
    long int v = util::parse_integer(value);
    if (v > LONG_MAX || v < LONG_MIN) throw std::invalid_argument("out of range");
    return (long int) v;
  }
  static bool require_value() { return true; }
  static std::string name() { return "<integer>"; }
};

template<>
struct option<long long int> {
  static long long int cast(const std::string& value) { return util::parse_integer(value); }
  static bool require_value() { return true; }
  static std::string name() { return "<integer>"; }
};

template<>
struct option<double> {
  static double cast(const std::string& value) { return std::atof(value.c_str()); }
  static bool require_value() { return true; }
  static std::string name() { return "<double>"; }
};

template<>
struct option<std::string> {
  static std::string cast(const std::string& value) { return value; }
  static bool require_value() { return true; }
  static std::string name() { return "<string>"; }
};

template<typename U>
struct option<std::vector<U> > {
  static std::vector<U> cast(const std::string& value) {
    return util::parse_list<U>(value);
  }
  static bool require_value() { return true; }
  static std::string name() { return option<U>::name() + "[," + option<U>::name() + "...]"; }
};

template<typename U, typename V>
struct option<std::pair<U,V> > {
  static std::pair<U,V> cast(const std::string& value) {
    std::pair<U,V> v;

    // find '='
    size_t pos = value.find('=', 0);
    if (pos == std::string::npos || pos == value.size()-1)
      throw std::invalid_argument("invalid input");

    // split key&value and cast
    v.first  = option<U>::cast(value.substr(0,pos));
    v.second = option<V>::cast(value.substr(pos+1));

    return v;
  }
  static bool require_value() { return true; }
  static std::string name() { return option<U>::name() + "=" + option<V>::name(); }
};

}
}
