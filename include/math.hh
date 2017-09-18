#ifndef IVANP_MATH_HH
#define IVANP_MATH_HH

namespace ivanp { namespace math {

template <typename T> [[ gnu::const ]]
constexpr auto sq(T x) noexcept { return x*x; }
template <typename T, typename... TT> [[ gnu::const ]]
constexpr auto sq(T x, TT... xx) noexcept { return sq(x)+sq(xx...); }

}}

#endif
