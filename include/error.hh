#ifndef IVANP_ERROR_HH
#define IVANP_ERROR_HH

#include <stdexcept>
#include "string.hh"

namespace ivanp {

struct error : std::runtime_error {
  using std::runtime_error::runtime_error;
  template <typename... Args>
  error(const Args&... args): std::runtime_error(ivanp::cat(args...)) { };
};

}

#endif
