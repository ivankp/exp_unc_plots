#ifndef IVANP_MATH_HH
#define IVANP_MATH_HH

namespace ivanp { namespace math {

template <typename T> [[ gnu::const ]]
constexpr auto sq(T x) noexcept { return x*x; }
template <typename T, typename... TT> [[ gnu::const ]]
constexpr auto sq(T x, TT... xx) noexcept { return sq(x)+sq(xx...); }

template <typename T1, typename T2>
inline void smaller(T1& x, const T2& y) noexcept { if (y < x) x = y; }
template <typename T1, typename T2>
inline void larger (T1& x, const T2& y) noexcept { if (x < y) x = y; }

}}

#endif
