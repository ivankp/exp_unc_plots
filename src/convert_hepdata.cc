#include "reader.hh"
#include "program_options.hh"
#include "termcolor.hpp"

using std::cout;
using std::cerr;
using std::endl;
namespace tc = termcolor;
using ivanp::starts_with;

std::ostream& operator<<(std::ostream& out, const std::exception& e) {
  return out << tc::red << e.what() << tc::reset;
}

bool read_input(std::istream& in) {
  bool reading_variable = false;
  unsigned line_n = 0;
  for (std::string line; std::getline(in,line); ) {
    ++line_n;
    if (!reading_variable) {
      if (ivanp::starts_with(line,"*dataset:")) {
        const auto var_name = view(line,line.rfind('/')+1);
        if (!var_t::all.emplace(var_name)) {
          cerr << tc::yellow << "Line " << line_n
               << ": repeated variable:" << tc::reset << " "
               << var_name << endl;
          continue;
        }
        reading_variable = true;
      }
    } else {
      auto& x = var_t::all.back();
      const bool star = starts_with(line,"*");
      if ( star && x.second.bin_edges.empty()) continue;
      if (!star && !line.empty()) { // parse bin information
        const auto d1 = line.find(';');
        if (d1==std::string::npos) {
          cerr << tc::red << "Line " << line_n
               << ": expected \';\'" << tc::reset << endl;
          return 1;
        }
        auto chunk = view(line,0,d1);

        const auto min = peal_head(chunk);
        const auto to  = peal_head(chunk);
        const auto max = peal_head(chunk);

        if (!min || !max || to!="TO") {
          cerr << tc::red << "Line " << line_n
               << ": unexpected bin definition:" << tc::reset << ' '
               << view(line,0,d1) << endl;
          return 1;
        }

        if (x.second.bin_edges.empty()) x.second.bin_edges.emplace_back(min);
        else if (x.second.bin_edges.back()!=min) {
          cerr << tc::red << "Line " << line_n
               << ": mismatch in bin edges:" << tc::reset
               << " in \"" << x.first << "\" " << x.second.bin_edges.back()
               << " and " << min << endl;
          return 1;
        }
        x.second.bin_edges.emplace_back(max);

        const auto d2 = line.find('(',d1+1);
        if (d2==std::string::npos) {
          cerr << tc::red << "Line " << line_n
               << ": expected \'(\'" << tc::reset << endl;
          return 1;
        }
        chunk = view(line,d1+1,d2-d1-1);

        const auto xsec = peal_head(chunk);
        const auto pm   = peal_head(chunk);
        const auto stat = peal_head(chunk);

        if (!xsec || !stat || pm!="+-") {
          cerr << tc::red << "Line " << line_n
               << ": unexpected bin definition:" << tc::reset << ' '
               << view(line,d1+1,d2-d1-1) << endl;
          return 1;
        }
        x.second.vals["xsec"].emplace_back(xsec);
        x.second.vals["stat"].emplace_back(stat);

        size_t first = d2 + 1, last = line.find(',',first+1);
        for (bool eol = false; !eol; ) {
          if (last==std::string::npos) eol = true;
          else if (!starts_with(line.c_str()+last+1,"DSYS=")) {
            last = line.find(',',last+1);
            if (last==std::string::npos) eol = true;
            else continue;
          }
          if (eol) {
            last = line.rfind(')');
            if (last==std::string::npos) {
              cerr << tc::red << "Line " << line_n << ":" << tc::reset
                   << " missing closing \')\'" << endl;
              return 1;
            } else eol = true;
          }


          first += 5; // DSYS=
          chunk = view(line,first,last-first); // view

          const auto d = chunk.find(':');
          x.second.vals[std::string(chunk.substr(d+1))]
            .emplace_back(chunk.substr(0,d));

          if (!eol) {
            first = last + 1;
            last = line.find(',',first+1);
          }
        }

      } else reading_variable = false;
    }
  }
  return 0;
}

int main(int argc, char* argv[]) {
  std::vector<const char*> ifnames;
  const char* ofname = nullptr;

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifnames,'i',"input file name",pos())
      (ofname,'o',"output file name")
      .parse(argc,argv)) return 0;
  } catch (const std::exception& e) {
    cerr << e << endl;
    return 1;
  }

  if (ifnames.empty()) {
    if (read_input(std::cin)) return 1;
  } else for (const char* fname : ifnames) {
    std::ifstream in(fname);
    if (read_input(in)) return 1;
  }

  try {
    var_t::check();
  } catch (const std::exception& e) {
    cerr << e << endl;
    return 1;
  }

  if (!ofname) cout << var_t::all;
  else std::ofstream(ofname) << var_t::all;
}
