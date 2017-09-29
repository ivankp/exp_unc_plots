#include <array>
#include <memory>
#include <cmath>

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
#include "math.hh"

#define TEST(var) \
  std::cerr << tc::cyan << #var << tc::reset << " = " << var << std::endl;

#define STR_IMPL(x) #x
#define STR(x) STR_IMPL(x)

using std::cout;
using std::cerr;
using std::endl;
namespace tc = termcolor;
using namespace ivanp;
using namespace ivanp::math;

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

auto make_replacer(const char* filename) {
  std::unordered_map<std::string,std::string> map;
  std::ifstream f(filename);
  for (std::string line; std::getline(f,line); ) {
    const auto d1 = line.find(' ');
    const auto d2 = line.find_first_not_of(' ',d1);
    map.emplace(line.substr(0,d1),line.substr(d2));
  }
  return [map=std::move(map)](const std::string& name){
    try { return map.at(name); }
    catch (...) { return name; }
  };
}

// template <typename V1, typename V2>
// V1&& operator+=(V1&& v1, const V2& v2) {
//   auto it1 = v1.begin();
//   auto it2 = v2.begin();
//   for ( ; it1 != v1.end(); ++it1, ++it2) {
//     (*it1) = std::sqrt(sq(*it1,*it2));
//   }
//   return std::forward<V1>(v1);
// }

int main(int argc, char* argv[]) {
  std::string ofname;
  const char *ifname,
             *vars_tex = STR(CONFIG) "/vars.tex",
             *unc_tex = STR(CONFIG) "/unc.tex",
             *style_file = STR(CONFIG) "/blue.sty",
             *ylabel = "",
             *ranges_file = nullptr;
  std::array<float,4> margins { 0.1, 0.035, 0.13, 0.03 };
  float yoffset = 0.7;
  bool burst = false;

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifname,'i',"input file name",req(),pos())
      (ofname,'o',"output file name",req())
      (burst,"--burst","put each plot in it's own file")
      (style_file,{"-s","--style"},"style file "+cat('[',style_file,']'))
      (vars_tex,"--vars-tex","file with latex for variables' names\n"+
        cat("default: ",vars_tex))
      (unc_tex,"--unc-tex","file with latex for uncertainties' names\n"+
        cat("default: ",unc_tex))
      (ranges_file,"--ranges","file with Y ranges")
      (margins,{"-m","--margins"},"canvas margins "+
        cat("l[",std::get<0>(margins),"]:"
            "r[",std::get<1>(margins),"]:"
            "b[",std::get<2>(margins),"]:"
            "t[",std::get<3>(margins),"]"))
      (ylabel,'y',"Y-axis label")
      (yoffset,"--y-offset","Y-axis label offset "+cat('[',yoffset,']'))
      .parse(argc,argv,true)) return 0;
  } catch (const std::exception& e) {
    cerr << e << endl;
    return 1;
  }

  // read formatting files ==========================================
  const auto var_name = make_replacer(vars_tex);
  const auto unc_name = make_replacer(unc_tex);

  struct style_t {
    Color_t fill_color, line_color;
    Style_t line_style;
    style_t(const std::string& s) {
      std::stringstream(s) >> fill_color >> line_color >> line_style;
    }
  };
  std::vector<style_t> styles;
  { std::ifstream f(style_file);
    for (std::string line; std::getline(f,line); ) {
      if (line.empty() || line[0]=='#') continue;
      styles.emplace_back(line);
    }
  }
  if (styles.empty()) {
    cerr << tc::red << "empty file:" << tc::reset << ' ' << style_file << endl;
    return 1;
  }
  auto style = [&styles,i=0u](TH1* h) mutable {
    if (i==styles.size()) i = 0u;
    h->SetFillColor(styles[i].fill_color);
    h->SetLineColor(styles[i].line_color);
    h->SetLineStyle(styles[i].line_style);
    ++i;
  };

  std::unordered_map<std::string,double> ranges;
  { std::ifstream f(ranges_file);
    for (std::string line; std::getline(f,line); ) {
      const auto d1 = line.find(' ');
      const auto d2 = line.find_first_not_of(' ',d1);
      ranges.emplace(line.substr(0,d1),::stod(line.substr(d2)));
    }
  }

  // ================================================================
  // read input file
  std::ifstream(ifname) >> var_t::all;

  TH1::AddDirectory(false);

  TCanvas canv;
  canv.SetMargin(std::get<0>(margins),std::get<1>(margins),
                 std::get<2>(margins),std::get<3>(margins));

  gPad->SetTickx();
  gPad->SetTicky();

  if (!burst) ofname += '(';
  bool first_page = true;
  unsigned page_back_cnt = var_t::all.size();
  // LOOP ===========================================================
  for (const auto& var : var_t::all) {
    --page_back_cnt;
    cout << var.first << '\n';

    const auto bins = var.second.bin_edges | ::stod;
    const auto xsec = var.second.vals["xsec"] | ::stod;
    const auto nbins = bins.size()-1;
    const auto nbands = var.second.vals.size()-1;

    struct band {
      using type = TH1D;
      type *h1, *h2;
      const std::string* name;
      band(TH1D* h, const std::string& name)
      : h1(h), h2(static_cast<type*>(h1->Clone())), name(&name) { }
      ~band() { delete h1; delete h2; }
    };
    std::vector<band> bands;
    bands.reserve(nbands);

    // convert string to double and fill first histogram ------------
    for (const auto& val : var.second.vals) {
      if (val.first=="xsec") continue;

      auto* h = new band::type("","",bins.size()-1,bins.data());
      h->SetStats(0);
      h->SetMarkerStyle(0);
      h->SetLineWidth(1); // gives legend color boxes outlines
      style(h);
      bands.emplace_back(h,val.first);

      auto* arr = bands.back().h1->GetArray() + 1;
      // fill with squares to sum in quadrature
      for (const auto& x : val.second) (*arr) = sq(::stod(x)), ++arr;
    }

    TAxis *xa = bands.back().h1->GetXaxis(),
          *ya = bands.back().h1->GetYaxis();
    xa->SetTitle(var_name(var.first).c_str());
    xa->SetTitleOffset(0.95);
    ya->SetTitleOffset(yoffset);
    ya->SetTitle(ylabel);
    xa->SetTitleSize(0.06);
    xa->SetLabelSize(0.05);
    ya->SetTitleSize(0.065);
    ya->SetLabelSize(0.05);

    TLegend leg(0.14, 0.165, 0.92, 0.285);
    leg.SetLineWidth(0);
    leg.SetFillColor(0);
    leg.SetFillStyle(0);
    leg.SetTextSize(0.041);
    leg.SetNColumns(2);

    // sum and add to legend ----------------------------------------
    leg.AddEntry(bands[0].h1,unc_name(*bands[0].name).c_str(),"f");
    for (unsigned i=1; i<nbands; ++i) {
      auto* arr1 = bands[i-1].h1->GetArray();
      auto* arr2 = bands[i  ].h1->GetArray();
      for (unsigned j=nbins; j; --j) arr2[j] += arr1[j];
      leg.AddEntry( // make legend entry
        bands[i].h1,
        cat("#oplus ",unc_name(*bands[i].name)).c_str(),
        "f");
    }

    // reflect, take sqrt, divide by xsec ---------------------------
    for (const auto& b : bands) {
      auto* arr1 = b.h1->GetArray();
      auto* arr2 = b.h2->GetArray();
      for (unsigned j=nbins; j; --j)
        arr2[j] = -(arr1[j] = std::sqrt(arr1[j])/xsec[j-1]);
    }

    // set Y-range --------------------------------------------------
    { double max = 0.;
      const auto it = ranges.find(var.first);
      if (it!=ranges.end()) max = it->second;
      else {
        auto* h = bands.back().h1;
        const auto* arr = h->GetArray();
        for (unsigned j=nbins; j; --j) larger(max,arr[j]);
        max *= 1.65;
      }
      ya->SetRangeUser(-max,max);
    }

    // draw ---------------------------------------------------------
    for (auto it=bands.rbegin(), last=bands.rend(); it!=last; ++it) {
      it->h1->Draw(it!=bands.rbegin() ? "SAME" : "");
      it->h2->Draw("SAME");
    }
    gPad->RedrawAxis();
    leg.Draw();

    TLatex l;
    l.SetTextColor(1);
    l.SetNDC();
    l.SetTextFont(72);
    l.DrawLatex(0.15,0.83,"ATLAS");
    l.SetTextFont(42);
    l.DrawLatex(0.27,0.83,"Internal");
    // l.DrawLatex(0.255,0.83,"Preliminary");
    l.SetTextFont(42);
    l.DrawLatex(0.15,0.89,
      "#it{H} #rightarrow #gamma#gamma, "
      "#sqrt{#it{s}} = 13 TeV, 36.1 fb^{-1}, "
      "m_{H} = 125.09 GeV"
    );
    l.SetTextFont(42);

    if (!burst && !page_back_cnt) ofname += ')';
    canv.Print(ofname.c_str(),("Title:"+var.first).c_str());
    if (!burst && first_page) ofname.pop_back(), first_page = false;
  }
  // ================================================================
}
