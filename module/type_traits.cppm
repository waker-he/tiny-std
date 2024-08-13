export module tinystd:type_traits;

import std;

namespace tinystd
{

export template <class T, class U>
using like_t = decltype(std::forward_like<T>(std::declval<U>()));

static_assert(std::same_as<like_t<int const &, double>, double const &>);
static_assert(std::same_as<like_t<int&, double>, double&>);
static_assert(std::same_as<like_t<int&&, double>, double&&>);

} // namespace tinystd