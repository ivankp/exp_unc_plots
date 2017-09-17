#ifndef IVANP_EXP_UNC_DATA_READER_HH
#define IVANP_EXP_UNC_DATA_READER_HH

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "ordered_map.hh"
#include "string_view.hh"

struct var {
  std::vector<std::string> bin_edges;
  ordered_map<std::vector<std::string>> vals;
  using all_t = ordered_map<var>;
  static all_t all;
  static void check();
};

std::ostream& operator<<(std::ostream& out, const var::all_t& vars);
std::istream& operator>>(std::istream& in, var::all_t& vars);

#endif
