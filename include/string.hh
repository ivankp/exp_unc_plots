#ifndef IVANP_STRING_HH
#define IVANP_STRING_HH

#include <string>
#include <cstring>
#include <sstream>

namespace ivanp {

namespace detail {

template <typename S, typename T>
inline void cat_impl(S& s, const T& t) { s << t; }

template <typename S, typename T, typename... TT>
inline void cat_impl(S& s, const T& t, const TT&... tt) {
  s << t;
  cat_impl(s,tt...);
}

}

template <typename... TT>
inline std::string cat(const TT&... tt) {
  std::stringstream ss;
  detail::cat_impl(ss,tt...);
  return ss.str();
}

template <typename Str, unsigned N>
inline bool starts_with(const Str& str, const char(&prefix)[N]) {
  for (unsigned i=0; i<N-1; ++i)
    if (str[i]!=prefix[i]) return false;
  return true;
}

template <unsigned N>
inline bool ends_with(const char* str, const char(&suffix)[N]) {
  const unsigned len = strlen(str);
  if (len<N-1) return false;
  return starts_with(str+(len-N+1),suffix);
}
template <unsigned N>
inline bool ends_with(const std::string& str, const char(&suffix)[N]) {
  return ends_with<N>(str.c_str(),suffix);
}

struct less_sz {
  bool operator()(const char* a, const char* b) const noexcept {
    return strcmp(a,b) < 0;
  }
};

}

#endif

