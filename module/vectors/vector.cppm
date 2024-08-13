export module tinystd:vector;

import std;
import :small_vector;

namespace tinystd
{

export template <typename T>
using vector = small_vector<T, 0>;

} // namespace tinystd