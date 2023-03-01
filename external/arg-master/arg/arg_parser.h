#pragma once

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <libgen.h> // basename

#include "arg_helper.h"

namespace arg {

typedef std::vector<std::string>::iterator iterator;

struct desc {
  const std::string& value;
  explicit desc(const std::string& _value): value(_value) {}
};

struct parser {
  typedef std::vector<std::string> str_vec;

  const std::string usage;
  const int min_arg_count;
  const int max_arg_count;
  std::string prog_name;
  bool show_help;
  std::string indent;

  str_vec arg_list;
  str_vec help_messages;

  parser(int argc, char** argv, const std::string& usage_ = "",
         int min_ = -1, int max_ = -1):
    usage(usage_),
    min_arg_count(min_),
    max_arg_count(std::max(min_arg_count,max_)),
    prog_name(basename(argv[0])),
    show_help(false), indent("    ")
  {
    // initialize argument vector
    for (int i=1; i<argc; i++) arg_list.push_back(argv[i]);

    // define default help option
    def(show_help, "help", "false", "Show this help and exit.");
  }

  void check() {
    // help flag passed?
    if (show_help) help();

    // check invalid option
    for (iterator it = arg_list.begin(); it != arg_list.end(); ++it) {
      if ("--" == *it) {
        // remove first "--"
        erase(it);
        // don't check after "--"
        break;
      } else if ("--" == (*it).substr(0,2)) {
        // found unknown option
        helper::util::error("unknown option '" + *it + "'!");
      }
    }

    // check the number of arguments
    if (min_arg_count >= 0 && static_cast<int>(arg_list.size()) < min_arg_count) {
      // show error message
      std::ostringstream message;
      message <<
        "the number of arguments is less than expected! (expected at least: "
        << min_arg_count << ", given: " << arg_list.size() << ")";
      helper::util::error(message.str(), false);

      // show usage and exit
      std::cerr << std::endl;
      show_usage();
      std::exit(1);
    }

    if (max_arg_count >= 0 && static_cast<int>(arg_list.size()) > max_arg_count) {
      // show error message
      std::ostringstream message;
      message <<
        "the number of arguments is greater than expected! (expected at most: "
        << min_arg_count << ", given: " << arg_list.size() << ")";
      helper::util::error(message.str(), false);

      // show usage and exit
      std::cerr << std::endl;
      show_usage();
      std::exit(1);
    }
  }

  // show help message and then exit
  void help() {
    show_usage();
    std::cerr << std::endl;
    show_options();
    std::exit(1);
  }

  void show_usage() {
    std::cerr << "USAGE" << std::endl;
    std::cerr << indent << prog_name << " " << usage << std::endl;
  }

  void show_options() {
    std::cerr << "OPTIONS" << std::endl;
    for (iterator it = help_messages.begin();
         it != help_messages.end(); it++) {
      std::cerr << *it;
    }
  }

  // container delegations
  iterator begin() { return arg_list.begin(); }
  iterator end() { return arg_list.end(); }
  iterator erase(const iterator& pos) {
    return arg_list.erase(pos);
  }
  iterator erase(const iterator& first, const iterator& last) {
    return arg_list.erase(first, last);
  }
  size_t size() {
    return arg_list.size();
  }
  const std::string& operator[](size_t i) {
    if (i >= arg_list.size())
      helper::util::error("index is out of range!");
    return arg_list[i];
  }

  // Throws: std::invalid_argument
  template<typename T>
  T as(int i) {
    return helper::option<T>::cast(arg_list.at(i));
  }

  template<typename T>
  bool parse(T& var, const std::string& name) {
    for (iterator it = arg_list.begin(); it != arg_list.end(); ++it) {
      if (name == *it) {
        // get value string
        iterator value_it = helper::option<T>::require_value() ? it+1 : it;
        if (value_it == arg_list.end())
          helper::util::error("the option '" + name + "' requires value!");
        // cast string to value
        try {
          var = helper::option<T>::cast(*value_it);
        } catch (const std::invalid_argument& ex) {
          std::string opt_desc = helper::option<T>::name();
          if (!opt_desc.empty()) opt_desc = " " + opt_desc;
          helper::util::error(std::string(ex.what()) + "! given '" + *value_it +
              "' to the option '" + name + opt_desc + "'");
        }
        erase(it, value_it+1);
        return true; // match
      } else if ("--" == *it) {
        // ignore options after "--"
        break;
      }
    }
    return false;
  }

  template<typename T>
  void gen_help(const std::string& long_name,
      const std::string& defval, const std::string& desc)
  {
    std::ostringstream str;

    // display option name
    str << indent << long_name << " " << helper::option<T>::name();

    // display default value
    str << "   (default: " << defval << ")";
    str << std::endl;

    // display message
    if (!desc.empty()) {
      str << indent << indent << desc << std::endl;
      str << std::endl;
    }

    help_messages.push_back(str.str());
  }

  template<typename T>
  void def(T& var, const std::string& var_name,
      const std::string& defval, const std::string& desc = "")
  {
    std::string long_name = "--" + helper::util::replace_all(var_name, '_', '-');

    try {
      var = helper::option<T>::cast(defval);
    } catch (const std::invalid_argument& ex) {
      std::string opt_desc = helper::option<T>::name();
      if (!opt_desc.empty()) opt_desc = " " + opt_desc;
      helper::util::error(std::string(ex.what()) + "! given '" + defval +
          "' for the default value of '" + long_name + opt_desc + "'");
    }

    parse(var, long_name);

    gen_help<T>(long_name, defval, desc);
  }
  
  void def(bool& var, const std::string& var_name,
      const std::string& defval, const std::string& desc = "")
  {
    // option name
    bool negative = var_name.find("no_", 0) == 0;
    std::string opt_name = helper::util::replace_all(var_name, '_', '-');

    // parse default value
    bool var_defval;
    if (defval == "true")       var_defval = true;
    else if (defval == "false") var_defval = false;
    else helper::util::error("default value is invalid");

    std::string long_name_help;

    if (!var_defval && !negative)
    {
      // if default value is false

      std::string long_name_t = "--" + opt_name;
      long_name_help = long_name_t;

      // parse option
      bool var_t;
      bool parse_t = parse(var_t, long_name_t);

      if (parse_t)
        var = true;
      else
        var = var_defval;
    }
    else
    {
      if (negative) {
        opt_name = opt_name.substr(3); // remove "no-"
      }

      std::string long_name_t = "--"      + opt_name;
      std::string long_name_f = "--no-"   + opt_name;
      long_name_help          = "--[no-]" + opt_name;

      // parse option
      bool var_t, var_f;
      bool parse_t = parse(var_t, long_name_t);
      bool parse_f = parse(var_f, long_name_f);

      if (parse_t && parse_f)
        helper::util::error("both '" + long_name_t + "' and '" + long_name_f + "' were specified!");
      else if (parse_t)
        var = true;
      else if (parse_f)
        var = false;
      else
        var = var_defval;

      var ^= negative;
    }

    // help
    gen_help<bool>(long_name_help, var_defval ? "true" : "false", desc);
  }
};

}
