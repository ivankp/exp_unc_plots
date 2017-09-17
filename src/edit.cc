#include "reader.hh"
#include "program_options.hh"
#include "terminal_colors.hh"
#include "error.hh"

#define TEST(var) \
  std::cout << TC_CYN #var TC_RST " = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;
using ivanp::starts_with;

int main(int argc, char* argv[]) {
  std::vector<const char*> ifnames;
  const char* ofname = nullptr;
  bool symm = false;

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifnames,'i',"input file name",pos())
      (ofname,'o',"output file name")
      (symm,{"-s","--symm"},"symmetrize uncertainties (take larger)")
      .parse(argc,argv)) return 0;
  } catch (const std::exception& e) {
    cerr << TC_RED << e.what() << TC_RST << endl;
    return 1;
  }

  try {
    if (ifnames.empty()) std::cin >> var::all;
    else for (const char* fname : ifnames)
      std::ifstream(fname) >> var::all;
    var::check();
  } catch (const std::exception& e) {
    cerr << TC_RED << e.what() << TC_RST << endl;
    return 1;
  }

  if (symm) {
    for (auto& x : var::all)
      for (auto& v : x.second.vals)
        for (auto& s : v.second)
          if (s[0]=='+' || s[0]=='-') {
            const auto d = s.find(',');
            if (d>=s.size()) continue;
            if (s[d+1]=='-' || s[d+1]=='+') {
              auto s1 = s.substr(1,d-1);
              auto s2 = s.substr(d+2);
              const auto u1 = std::stod(s1);
              const auto u2 = std::stod(s2);
              s = (u1>u2 ? std::move(s1) : std::move(s2));
            }
          }
  }

  if (!ofname) cout << var::all;
  else std::ofstream(ofname) << var::all;
}
