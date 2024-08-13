export module tinystd:vector_mixin;

import std;
import :type_traits;

namespace tinystd
{

template <typename T>
concept nothrow_relocatable =
    __is_trivially_relocatable(T) || std::is_nothrow_move_constructible_v<T>;

template <typename T>
concept sbo_can_be_constexpr =
    std::is_trivially_constructible_v<T> && std::is_trivially_destructible_v<T>;

template <typename T>
constexpr void
uninitialized_copy(T* ifirst, T* ilast, T* ofirst)
{
    if consteval
    {
        for (; ifirst != ilast; ++ifirst, ++ofirst)
        {
            std::construct_at(ofirst, *ifirst);
        }
    }
    else { std::uninitialized_copy(ifirst, ilast, ofirst); }
}

template <typename T>
constexpr void
relocate(T* ifirst, T* ilast, T* ofirst) noexcept(nothrow_relocatable<T>)
{
    if consteval
    {
        for (; ifirst != ilast; ++ifirst, ++ofirst)
        {
            std::construct_at(ofirst, std::move(*ifirst));
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                std::destroy_at(ifirst);
            }
        }
    }
    else
    {
        if constexpr (__is_trivially_relocatable(T))
        {
            std::memcpy(ofirst, ifirst, sizeof(T) * (ilast - ifirst));
        }
        else
        {
            T* oCurr = ofirst;
            T* iCurr = ifirst;
            try
            {
                // unitialized_move_if_noexcept
                for (; iCurr != ilast; ++iCurr, ++oCurr)
                {
                    // might throw here
                    std::construct_at(oCurr, std::move_if_noexcept(*iCurr));
                }

                // destroy all objects in input range
                std::destroy(ifirst, ilast);
            }
            catch (...)
            {
                std::destroy(ofirst, oCurr);
                throw;
            }
        }
    }
}

export template <class T>
struct vector_mixin
{
    using size_type      = std::size_t;
    using iterator       = T*;
    using const_iterator = T const *;

    template <class Self>
    [[nodiscard]] constexpr auto
    operator[](this Self&& self, std::integral auto i) noexcept
        -> like_t<Self, T>
    {
        return std::forward_like<Self>(*(self.data() + i));
    }

    template <class Self>
    [[nodiscard]] constexpr auto
    begin(this Self&& self) noexcept
        -> std::conditional_t<std::is_const_v<Self>, const_iterator, iterator>
    {
        return self.data();
    }

    template <class Self>
    [[nodiscard]] constexpr auto
    end(this Self&& self) noexcept
        -> std::conditional_t<std::is_const_v<Self>, const_iterator, iterator>
    {
        return self.data() + self.size();
    }

    [[nodiscard]] constexpr auto
    empty(this auto&& self) noexcept -> bool
    {
        return self.size() == 0;
    }
};

} // namespace tinystd