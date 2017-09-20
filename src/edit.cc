#include <cmath>
#include <iomanip>

#include <boost/regex.hpp>

#include "reader.hh"
#include "program_options.hh"
#include "terminal_colors.hh"
#include "error.hh"
#include "math.hh"

#define TEST(var) \
  std::cout << TC_CYN #var TC_RST " = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;

using namespace ivanp;
using namespace ivanp::math;


template <typename...> struct bad_type;

template <typename T> const T& as_const(const T& x) { return x; }

template <typename Res, typename S>
bool match_any(const S& str, const Res& res) {
  return std::any_of( res.begin(), res.end(),
    [&](const auto& re){ return regex_match(str,re); } );
}

int main(int argc, char* argv[]) {
  std::vector<const char*> ifnames;
  std::vector<const char*> add, rm, exclude;
  const char* ofname = nullptr;
  bool sym = false, invert_add = false;
  unsigned top = 0;

  try {
    using namespace ivanp::po;
    using ivanp::po::error;
    if (program_options()
      (ifnames,'i',"input file name",pos())
      (ofname,'o',"output file name")
      (rm,"--rm","remove these fields")
      (sym,"--sym","symmetrize uncertainties (take larger)")
      (add,"--add","sum these fields in quadrature",
        [&](const char* str, auto& x){
          if (!x.empty() && invert_add) throw error(
            "only one of the --add or --add-except options can be used");
          x.push_back(str);
        })
      (add,"--add-except","sum all fields in quadrature except these\n"
                          "first value is the name of the sum\n"
                          "only one of the two options may be used\n"
                          "regex can be used here",
        [&](const char* str, auto& x){
          if (x.empty()) invert_add = true;
          else if (!invert_add) throw error(
            "only one of the --add or --add-except options may be used");
          x.push_back(str);
        })
      (top,"--top","keep top n contributions, combine others")
      (exclude,"--exclude","fields that won't participate")
      .parse(argc,argv)) return 0;

      if (add.size()==1) throw error(
        "--add or --add-except options take at least 2 arguments");
  } catch (const std::exception& e) {
    cerr << TC_RED << e.what() << TC_RST << endl;
    return 1;
  }

  try { // READ =====================================================
    if (ifnames.empty()) std::cin >> var::all;
    else for (unsigned i=0, n=ifnames.size(); i<n; ++i) {
      if (!i) std::ifstream(ifnames[i]) >> var::all;
      else { // replace if from subsequent files
        var::all_t new_vars;
        std::ifstream(ifnames[i]) >> new_vars;
        for (auto& x2 : new_vars) {
          auto& x1 = var::all[x2.first];
          if (x1.bin_edges != x2.second.bin_edges) throw error(
            "different binning for \"",x2.first,"\" in file ",ifnames[i]);
          for (auto&& v2 : x2.second.vals)
            x1.vals[v2.first] = std::move(v2.second);
        }
      }
    }
    var::check();
  } catch (const std::exception& e) {
    cerr << TC_RED << e.what() << TC_RST << endl;
    return 1;
  }

  // ================================================================
  if (!rm.empty()) {
    std::vector<boost::regex> res;
    res.reserve(rm.size());
    for (const char* str : rm) res.emplace_back(str);
    for (auto& x : var::all) {
      auto& vals = x.second.vals;
      auto last = vals.end();
      for (auto it=vals.begin(); it!=last; ) {
        if (match_any(it->first, res)) {
          it = vals.erase(it);
          last = vals.end();
          continue;
        }
        ++it;
      }
    }
  }

  // ================================================================
  if (sym) {
    for (auto& x : var::all)
      for (auto& v : x.second.vals)
        for (auto& s : v.second) {
          const auto d = s.find(',');
          if (d<s.size()) {
            const bool pm1 = (s[0]=='+' || s[0]=='-');
            const bool pm2 = (s[d+1]=='+' || s[d+1]=='-');
            auto s1 = s.substr(pm1,d-pm1);
            auto s2 = s.substr(d+1+pm2);
            const auto u1 = std::stod(s1);
            const auto u2 = std::stod(s2);
            s = (u1>u2 ? std::move(s1) : std::move(s2));
          } else if (s[0]=='-' || s[0]=='+') {
            s.erase(0,1);
          }
        }
  }

  // ================================================================
  if (!add.empty()) {
    std::vector<boost::regex> res;
    res.reserve(add.size()-1);
    for (const char* str : add) {
      if (str==add.front()) continue;
      res.emplace_back(str);
    }
    for (auto& x : var::all) {
      auto& vals = x.second.vals;
      const unsigned n = x.second.bin_edges.size()-1;
      std::vector<double> sumd(n,0.);
      auto last = vals.end();
      for (auto it=vals.begin(); it!=last; ) {
        if (match_any(it->first, res) != invert_add) {
          for (unsigned i=0; i<n; ++i) {
            try {
              sumd[i] += sq(stod(it->second[i]));
            } catch (const std::exception& e) {
              cerr << TC_RED << e.what() << ":" TC_RST " "
                   << it->second[i] << endl;
              return 1;
            }
          }
          it = vals.erase(it);
          last = vals.end();
          continue;
        }
        ++it;
      }
      auto& sum = vals[add.front()];
      sum.reserve(n);
      for (double d : sumd)
        sum.emplace_back( cat(std::fixed,std::setprecision(8),std::sqrt(d)) );
    }
  }

  // ================================================================
  if (top) {
    std::vector<boost::regex> res;
    res.reserve(exclude.size());
    for (const char* str : exclude) res.emplace_back(str);
    for (auto& x : var::all) {
      const auto& xsec = as_const(x.second.vals)["xsec"];
      auto& vals = x.second.vals;
      std::vector<std::pair<decltype(vals.begin()),double>> fields;
      fields.reserve(vals.size());
      for (auto it=vals.begin(); it!=vals.end(); ++it) {
        if (match_any(it->first, res) || it->first=="xsec") continue;
        fields.emplace_back(it,0.);
        const auto& s = it->second;
        for (unsigned i=0, n=s.size(); i<n; ++i)
          fields.back().second += stod(s[i])/stod(xsec[i]);
      }

      std::partial_sort( fields.begin(), fields.begin()+top, fields.end(),
        [](const auto& a, const auto& b){ return b.second < a.second; });
      std::sort( fields.begin()+top, fields.end(),
        [](const auto& a, const auto& b){ return ((b.first) < (a.first)); });

      cout << '\n';
      for (auto& p : fields) {
        cout << p.first->first << endl;
      }
      cout << '\n';

    }
  }

  // ================================================================
  if (!ofname) cout << var::all;
  else std::ofstream(ofname) << var::all;
}
