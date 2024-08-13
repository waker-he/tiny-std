export module tinystd:inplace_vector;

import std;
import :vector_mixin;

namespace tinystd
{

export template <typename T, std::size_t N>
class inplace_vector : private vector_mixin<T>
{
    using base = vector_mixin<T>;

public:
    using typename base::const_iterator;
    using typename base::iterator;
    using typename base::size_type;
    using base::operator[];
    using base::begin;
    using base::empty;
    using base::end;

    // constructors
    constexpr inplace_vector() noexcept : m_sz{0} {}

    constexpr inplace_vector(inplace_vector const & other) : m_sz{other.m_sz}
    {
        uninitialized_copy(other.begin(), other.end(), begin());
    }

    constexpr inplace_vector(inplace_vector&& other)
        noexcept(nothrow_relocatable<T>)
        : m_sz{other.m_sz}
    {
        relocate(other.begin(), other.end(), begin());
        other.m_sz = 0;
    }

    // assignment
    constexpr auto
    operator=(inplace_vector rhs) noexcept -> inplace_vector&
        requires nothrow_relocatable<T>
    {
        swap(rhs);
        return *this;
    }

    // dtor
    constexpr ~inplace_vector() noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            std::destroy(begin(), end());
        }
    }

    // access and observers
    template <class Self>
    [[nodiscard]] constexpr auto
    data(this Self&& self
    ) noexcept -> std::conditional_t<std::is_const_v<Self>, T const *, T*>
    {
        if constexpr (sbo_can_be_constexpr<T>) { return self.m_data; }
        else { return reinterpret_cast<T*>(const_cast<char*>(self.m_data)); }
    }

    [[nodiscard]] constexpr auto
    size() const noexcept -> size_type
    {
        return m_sz;
    }

    [[nodiscard]] constexpr auto
    capacity() const noexcept -> size_type
    {
        return N;
    }

    // modifiers
    constexpr void
    swap(inplace_vector& other) noexcept
        requires nothrow_relocatable<T>
    {
        inplace_vector temp(std::move(*this));
        relocate(other.begin(), other.end(), begin());
        relocate(temp.begin(), temp.end(), other.begin());
        m_sz       = other.m_sz;
        other.m_sz = temp.m_sz;
        temp.m_sz  = 0;
    }

    template <class... Args>
    constexpr auto
    emplace_back(Args&&... args) -> T&
    {
        T& ret = *std::construct_at(end(), std::forward<Args>(args)...);
        ++m_sz;
        return ret;
    }

    constexpr void
    pop_back() noexcept
    {
        --m_sz;
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            std::destroy_at(end());
        }
    }

private:
    using storage_t =
        std::conditional_t<sbo_can_be_constexpr<T>, T[N], char[N * sizeof(T)]>;

    size_type m_sz;
    [[no_unique_address]] alignas(T) storage_t m_data;
};

export template <typename T, std::size_t N>
constexpr void
swap(inplace_vector<T, N>& v1, inplace_vector<T, N>& v2)
    noexcept(noexcept(v1.swap(v2)))
{
    v1.swap(v2);
}

} // namespace tinystd