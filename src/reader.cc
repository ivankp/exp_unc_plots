#include "reader.hh"

using ivanp::error;

var_t::all_t var_t::all;

std::ostream& operator<<(std::ostream& out, const var_t::all_t& vars) {
  for (const auto& x : vars) {
    out << x.first << ".bins:";
    for (const auto& b : x.second.bin_edges) out << ' ' << b;
    out << '\n';
    for (const auto& v : x.second.vals) {
      out << x.first << '.' << v.first << ':';
      for (const auto& b : v.second) out << ' ' << b;
      out << '\n';
    }
    out << std::endl;
  }
  return out;
}

void var_t::check() {
  for (const auto& x : var_t::all) {
    if (x.second.bin_edges.empty()) throw error(
      "no bins for variable \"",x.first,'\"');
    const auto nbins = x.second.bin_edges.size()-1;
    for (const auto& v : x.second.vals) {
      if (v.second.size()!=nbins) throw error(
        v.second.size()," \"",v.first,"\" values for \"",x.first,"\" with ",
        nbins," bins");
    }
  }
}

std::istream& operator>>(std::istream& in, var_t::all_t& vars) {
  unsigned line_n = 0;
  for (std::string line; std::getline(in,line); ) {
    if (std::all_of(line.begin(),line.end(),
          [](char c){ return std::isspace(c); })) continue;
    const auto d1 = line.find('.');
    const auto var_name = view(line,0,d1);
    auto& x = vars[var_name];
    const auto d2 = line.find(':',d1+1);
    const auto field = view(line,d1+1,d2-d1-1);
    std::vector<std::string> *v = nullptr;
    if (field=="bins") {
      if (!x.bin_edges.empty()) throw error(
        "line ",line_n,": "
        "repeated binning for variable \"",var_name,'\"');
      v = &x.bin_edges;
    } else {
      if (!x.vals.emplace(field)) throw error(
        "line ",line_n,": "
        "repeated field \"",field,"\" in variable \"",var_name,'\"');
      v = &x.vals.back().second;
    }
    auto chunk = view(line,d2+1);
    for (;;) {
      const auto head = peal_head(chunk);
      if (!head) break;
      v->emplace_back(head);
    }
  }
  return in;
}
