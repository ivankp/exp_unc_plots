#ifndef IVANP_BOOST_STRING_VIEW_HH
#define IVANP_BOOST_STRING_VIEW_HH

#include <boost/utility/string_view.hpp>

using string_view = boost::string_view;

inline string_view view(
  const std::string& str,
  string_view::size_type p = 0,
  string_view::size_type n = string_view::npos
) noexcept {
  if (p>=str.size() || n==0) return { };
  if (n>str.size()-p) n = str.size()-p;
  return { str.data()+p, n };
}

inline void ltrim(string_view& sv, const char* ds = " \t") {
  sv.remove_prefix(sv.find_first_not_of(ds));
}

constexpr bool operator!(const string_view& sv) noexcept {
  return sv.empty();
}

string_view peal_head(string_view& sv, const char* ds = " \t");

#endif
