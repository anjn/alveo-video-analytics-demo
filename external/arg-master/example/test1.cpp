#include <cstdio>

#include "arg/arg.h"

// Custom data type option

struct geometry {
  int w, h;
};

namespace arg {
namespace helper {

template<>
struct option<geometry> {
  static geometry cast(const std::string& value) {
    // find 'x'
    size_t pos = value.find('x', 0);
    if (pos == std::string::npos || pos == value.size()-1)
      throw std::invalid_argument("invalid input");

    geometry v;
    v.w = option<int>::cast(value.substr(0,pos));
    v.h = option<int>::cast(value.substr(pos+1));
    return v;
  }
  static bool require_value() { return true; }
  static std::string name() { return "<width>x<height>"; }
};

}
}

int main(int argc, char** argv) {
  geometry size;

  arg_begin("INT1 INT2", 2, 2);
  arg_i(int_option, 0, "An option which takes an integer value.");
  arg_b(bool_option1, false, "An option which takes a bool value.");
  arg_b(bool_option2, true, "An option which takes a bool value.");
  arg_b(no_bool_option3, false, "An option which takes a bool value.");
  arg_b(no_bool_option4, true, "An option which takes a bool value.");
  arg_g(size, 320x240, "An option which takes a size (WxH).");
  arg_end;

  for (int i=0; i<args.size(); i++) {
    try {
      std::cout << "args[" << i << "] = " << args.as<int>(i) << std::endl;
    } catch (const std::invalid_argument& e) {
      std::cerr << "argument is not integer!" << std::endl;
      std::exit(1);
    }
  }

  std::cout << "int_option = " << int_option << std::endl;
  std::cout << "bool_option1 = " << bool_option1 << std::endl;
  std::cout << "bool_option2 = " << bool_option2 << std::endl;
  std::cout << "no_bool_option3 = " << no_bool_option3 << std::endl;
  std::cout << "no_bool_option4 = " << no_bool_option4 << std::endl;
  std::cout << "size.w = " << size.w << std::endl;
  std::cout << "size.h = " << size.h << std::endl;
}
