#include <cmath>
#include <iomanip>
#include <tuple>

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include "termcolor.hpp"

#include "reader.hh"
#include "program_options.hh"
#include "error.hh"
#include "math.hh"

namespace tc = termcolor;

#define TEST(var) \
  std::cerr << tc::cyan << #var << tc::reset << " = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;

using namespace ivanp;
using namespace ivanp::math;

template <typename T> const T& as_const(const T& x) { return x; }

template <typename Res, typename S>
bool match_any(const S& str, const Res& res) {
  return std::any_of( res.begin(), res.end(),
    [&](const auto& re){ return regex_match(str,re); } );
}

template <typename V, typename F>
auto operator|(const V& v, F&& f) {
  std::vector<decltype(f(*v.begin()))> out;
  out.reserve(v.size());
  for (const auto& x : v) out.emplace_back(f(x));
  return out;
}

double stod(const std::string& str) {
  try {
    return boost::lexical_cast<double>(str);
  } catch (const boost::bad_lexical_cast& e) {
    throw error("cannot interpret \"",str,"\" as double");
  }
}

unsigned prec = 8;
std::string dtos(double x) {
  return cat(std::fixed,std::setprecision(prec),x);
}

std::ostream& operator<<(std::ostream& out, const std::exception& e) {
  return out << tc::red << e.what() << tc::reset;
}

int main(int argc, char* argv[]) {
  std::vector<const char*> ifnames;
  std::vector<const char*> add, rm, exclude;
  const char* ofname = nullptr;
  bool sym = false, invert_add = false;
  std::tuple<unsigned,const char*> top {0,"others"};
  boost::optional<double> tol;

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
      (top,"--top","keep top n contributions, combine others\n"
                   "n:name or n, default name is \"others\"")
      (exclude,"--exclude","fields that won't participate")
      (prec,"--prec","double to string precision, default is 8")
      (tol,"--tol","fractional tolerance when comparing binning")
      .parse(argc,argv)) return 0;

      if (!invert_add && add.size()==1) throw error(
        "--add takes at least 2 arguments");
  } catch (const std::exception& e) {
    cerr << e << endl;
    return 1;
  }

  try { // READ =====================================================
    if (ifnames.empty()) std::cin >> var_t::all;
    else for (unsigned i=0, n=ifnames.size(); i<n; ++i) {
      if (!i) std::ifstream(ifnames[i]) >> var_t::all;
      else { // replace if from subsequent files
        var_t::all_t new_vars;
        std::ifstream(ifnames[i]) >> new_vars;
        for (auto& var2 : new_vars) {
          auto& var1 = var_t::all[var2.first];

          if ( tol ?
            !std::equal(
              var1.bin_edges.begin(), var1.bin_edges.end(),
              var2.second.bin_edges.begin(), var2.second.bin_edges.end(),
              [&](const auto& s1, const auto& s2){
                const auto x1 = ::stod(s1), x2 = ::stod(s2);
                if (x1==x2) return true;
                return std::abs(1.-x1/x2) < *tol;
              }
            ) : ( var1.bin_edges != var2.second.bin_edges )
          ) throw error(
            "different binning for \"",var2.first,"\" in file ",ifnames[i]);

          for (auto&& val2 : var2.second.vals)
            var1.vals[val2.first] = std::move(val2.second);
        }
      }
    }
    var_t::check();
  } catch (const std::exception& e) {
    cerr << e << endl;
    return 1;
  }

  // ================================================================
  if (!rm.empty()) {
    std::vector<boost::regex> res;
    res.reserve(rm.size());
    for (const char* str : rm) res.emplace_back(str);
    for (auto& var : var_t::all) {
      auto& vals = var.second.vals;
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
    for (auto& var : var_t::all)
      for (auto& val : var.second.vals)
        for (auto& s : val.second) {
          const auto d = s.find(',');
          if (d<s.size()) {
            const bool pm1 = (s[0]=='+' || s[0]=='-');
            const bool pm2 = (s[d+1]=='+' || s[d+1]=='-');
            auto s1 = s.substr(pm1,d-pm1);
            auto s2 = s.substr(d+1+pm2);
            double u1, u2;
            try {
              u1 = ::stod(s1);
              u2 = ::stod(s2);
            } catch (const std::exception& e) {
              cerr << e << endl;
              return 1;
            }
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
    for (auto& var : var_t::all) {
      auto& vals = var.second.vals;
      const unsigned nbins = var.second.bin_edges.size()-1;
      std::vector<double> sumd(nbins,0.);
      auto last = vals.end();
      for (auto it=vals.begin(); it!=last; ) {
        if (match_any(it->first, res) != invert_add) {
          for (unsigned i=0; i<nbins; ++i) {
            try {
              sumd[i] += sq(::stod(it->second[i]));
            } catch (const std::exception& e) {
              cerr << e << endl;
              return 1;
            }
          }
          if (strcmp(it->first.c_str(),add.front())) {
            it = vals.erase(it);
            last = vals.end();
            continue;
          } else {
            it->second.clear();
          }
        }
        ++it;
      }
      auto& sum = vals[add.front()];
      sum.reserve(nbins);
      for (double d : sumd) // convert back to strings
        sum.emplace_back(dtos(std::sqrt(d)));
    }
  }

  // ================================================================
  if (std::get<0>(top)) {
    const auto ntop = std::get<0>(top);
    std::vector<boost::regex> res;
    res.reserve(exclude.size());
    for (const char* str : exclude) res.emplace_back(str);
    for (auto& var : var_t::all) { // outer loop
      auto& vals = var.second.vals;
      // parse xsec values
      std::vector<double> xsec;
      try {
        xsec = as_const(vals)["xsec"] |
          [](const auto& s){ return ::stod(s); };
      } catch (const std::exception& e) {
        cerr << e << endl;
        return 1;
      }
      const auto nbins = var.second.bin_edges.size()-1;
      using iter = decltype(vals.begin());
      // map iterator, values in bins, impact metric
      std::vector<std::tuple< iter, std::vector<double>, double >> fields;
      fields.reserve(vals.size());
      std::vector<const std::string*> order; // preserve fields' order
      order.reserve(exclude.size()+ntop+1);
      for (auto it=vals.begin(); it!=vals.end(); ++it) {
        // exclude accordingly specified fields
        if (match_any(it->first, res) || it->first=="xsec") {
          order.push_back(&it->first);
          continue;
        }
        fields.emplace_back(it,nbins,0.);
        const auto& bins = it->second;
        for (unsigned i=0; i<nbins; ++i) {
          double x;
          try {
            x = ::stod(bins[i]);
          } catch (const std::exception& e) {
            cerr << e << endl;
            return 1;
          }
          // cache parsed numbers
          std::get<1>(fields.back())[i] = x;
          // use sum of fractional uncertainties as impact metric
          std::get<2>(fields.back()) += x/xsec[i];
        }
      }

      // select top contributions (descending)
      std::partial_sort( fields.begin(), fields.begin()+ntop, fields.end(),
        [](const auto& a, const auto& b){
          return std::get<2>(a) > std::get<2>(b); });
      // re-sort back minor contributions by order (backward)
      std::sort( fields.begin()+ntop, fields.end(),
        [](const auto& a, const auto& b){
          return std::get<0>(a) > std::get<0>(b); });

      // keep order of top contributions
      for (unsigned i=0; i<ntop; ++i) {
        TEST( std::get<2>(fields[i]) ) // test impact metric
        order.push_back(&std::get<0>(fields[i])->first);
      }

      // sum others in quadrature and erase contributions
      std::vector<double> sumd(nbins,0.);
      for (auto it=fields.begin()+ntop; it!=fields.end(); ++it) {
        for (unsigned i=0; i<nbins; ++i)
          sumd[i] += sq( std::get<1>(*it)[i] );
        vals.erase(std::get<0>(*it));
      }

      // add others to vars
      auto& sum = vals[std::get<1>(top)];
      sum.reserve(nbins);
      for (double d : sumd) // convert back to strings
        sum.emplace_back(dtos(std::sqrt(d)));

      // apply order
      vals.sort([
          f = [&order](const std::string* sp){
            return std::find(order.begin(),order.end(),sp);
          }
        ](const auto& a, const auto& b){
          return f(&a.first) < f(&b.first);
        });
    }
  }

  // ================================================================
  if (!ofname) cout << var_t::all;
  else std::ofstream(ofname) << var_t::all;
}
