#include <array>

#include <boost/lexical_cast.hpp>

#include <TCanvas.h>
#include <TAxis.h>
#include <TColor.h>
#include <TH1.h>
#include <TLegend.h>
#include <TLatex.h>

#include "termcolor.hpp"

#include "reader.hh"
#include "program_options.hh"

#define TEST(var) \
  std::cerr << tc::cyan << #var << tc::reset << " = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;
namespace tc = termcolor;
using namespace ivanp;

std::ostream& operator<<(std::ostream& out, const std::exception& e) {
  return out << tc::red << e.what() << tc::reset;
}

double stod(const std::string& str) {
  try {
    return boost::lexical_cast<double>(str);
  } catch (const boost::bad_lexical_cast& e) {
    throw ivanp::error("cannot interpret \"",str,"\" as double");
  }
}

template <typename V, typename F>
auto operator|(const V& v, F&& f) {
  std::vector<decltype(f(*v.begin()))> out;
  out.reserve(v.size());
  for (const auto& x : v) out.emplace_back(f(x));
  return out;
}

int main(int argc, char* argv[]) {
  std::string ofname;
  const char *ifname,
             *var_tex = "config/var.tex",
             *ylabel = "";
  std::array<float,4> margins { 0.1, 0.035, 0.13, 0.03 };

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifname,'i',"input file name",req(),pos())
      (ofname,'o',"output file name",req())
      (var_tex,"--var-tex","file with latex for variables' names\n"+
        cat("default: ",var_tex))
      (margins,{"-m","--margins"},"canvas margins "+
        cat("l[",std::get<0>(margins),"]:"
            "r[",std::get<1>(margins),"]:"
            "b[",std::get<2>(margins),"]:"
            "t[",std::get<3>(margins),"]"))
      (ylabel,'y',"Y-axis label")
      .parse(argc,argv,true)) return 0;
  } catch (const std::exception& e) {
    cerr << e << endl;
    return 1;
  }

  // read input file
  std::ifstream(ifname) >> var_t::all;

  std::unordered_map<std::string,std::string> tex;
  { std::ifstream f(var_tex);
    for (std::string line; std::getline(f,line); ) {
      const auto d = line.find(' ');
      tex.emplace(line.substr(0,d),line.substr(d+1));
    }
  }

  TCanvas canv;
  canv.SetMargin(std::get<0>(margins),std::get<1>(margins),
                 std::get<2>(margins),std::get<3>(margins));

  gPad->SetTickx();
  gPad->SetTicky();

  ofname += '(';
  bool first_page = true;
  unsigned page_back_cnt = var_t::all.size();
  for (const auto& var : var_t::all) {
    --page_back_cnt;
    cout << var.first << '\n';

    const auto bin_edges = var.second.bin_edges | ::stod;
    const auto xsec = var.second.vals["xsec"] | ::stod;

    for (const auto& val : var.second.vals) {
    }

    if (!page_back_cnt) ofname += ')';
    canv.Print(ofname.c_str(),("Title:"+var.first).c_str());
    if (first_page) ofname.pop_back(), first_page = false;
  }
}
