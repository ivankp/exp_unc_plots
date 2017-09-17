#include <iostream>
#include <vector>

#include "program_options.hh"
#include "terminal_colors.hh"

#define TEST(var) \
  std::cout << TC_CYN #var TC_RST " = " << var << std::endl;

using namespace std;

int main(int argc, char* argv[]) {
  vector<const char*> ifnames;

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifnames,'i',"input file name",req(),pos())
      (ofname,'o',"output file name",req())
      .parse(argc,argv,true)) return 0;
  } catch (const std::exception& e) {
    cerr << RED << e.what() << RST << endl;
    return 1;
  }

}
