export module tinystd:span;

import std;

namespace tinystd
{

export constexpr std::size_t dynamic_extent =
    std::numeric_limits<std::size_t>::max();

export template <class T, std::size_t Extent = dynamic_extent>
class span
{
public:
    static constexpr std::size_t extent = Extent;

    // member types
    using element_type = T;
    using size_type    = std::size_t;
    using reference    = T&;
    using pointer      = T*;
    using iterator     = T*;

    // span is trivially copyable
    constexpr span(span const &) noexcept = default;
    constexpr auto
    operator=(span const &) noexcept -> span& = default;
    constexpr ~span()                         = default;

    // Regular constructors
    constexpr span() noexcept
        requires((extent == 0) || (extent == dynamic_extent))
        : m_begin{nullptr}
    {
        if constexpr (extent == dynamic_extent) { m_sz = 0; }
    }

    constexpr span(iterator first, size_type count) noexcept : m_begin(first)
    {
        if constexpr (extent == dynamic_extent) { m_sz = count; }
    }

    constexpr span(iterator first, iterator last) noexcept : m_begin(first)
    {
        if constexpr (extent == dynamic_extent) { m_sz = last - first; }
    }

    // Iterators
    [[nodiscard]] constexpr auto
    begin() const noexcept -> iterator
    {
        return data();
    }

    [[nodiscard]] constexpr auto
    end() const noexcept -> iterator
    {
        return data() + size();
    }

    // Element access
    [[nodiscard]] constexpr auto
    data() const noexcept -> T*
    {
        return m_begin;
    }

    [[nodiscard]] constexpr auto
    operator[](std::integral auto idx) const noexcept -> T&
    {
        return *(m_begin + idx);
    }

    // Observers
    [[nodiscard]] constexpr auto
    size() const noexcept -> size_type
    {
        if constexpr (extent == dynamic_extent) { return m_sz; }
        else { return extent; }
    }

    [[nodiscard]] constexpr auto
    empty() const noexcept -> bool
    {
        return size() == 0;
    }

    // Subviews
    template <size_type Offset, size_type Count = dynamic_extent>
        requires((Offset <= Extent)
                 && (Count == dynamic_extent || Extent - Offset >= Count))
    [[nodiscard]] constexpr auto
    subspan() const noexcept -> auto
    {
        if constexpr (Count == dynamic_extent && extent == dynamic_extent)
        {
            // size cannot be determined in compile-time
            return span<T>(m_begin + Offset, size() - Offset);
        }
        else
        {
            constexpr size_type cnt =
                Count == dynamic_extent ? Extent - Offset : Count;
            return span<T, cnt>(m_begin + Offset, cnt);
        }
    }

    [[nodiscard]] constexpr auto
    subspan(size_type offset, size_type count = dynamic_extent) const noexcept
        -> span<T>
    {
        return span<T>(
            m_begin + offset, count == dynamic_extent ? size() - offset : count
        );
    }

    template <size_type Count>
    [[nodiscard]] constexpr auto
    first() const noexcept -> span<T, Count>
    {
        return subspan<0, Count>();
    }

    [[nodiscard]] constexpr auto
    first(size_type count) const noexcept -> span<T>
    {
        return subspan(0, count);
    }

    template <size_type Count>
    [[nodiscard]] constexpr auto
    last() const noexcept -> span<T, Count>
    {
        if constexpr (extent == dynamic_extent)
        {
            return span<T, Count>(m_begin + m_sz - Count, Count);
        }
        else { return subspan<Extent - Count, Count>(); }
    }

    [[nodiscard]] constexpr auto
    last(size_type count) const noexcept -> span<T>
    {
        if constexpr (extent == dynamic_extent)
        {
            return subspan(m_sz - count, count);
        }
        else { return subspan(extent - count, count); }
    }

private:
    struct _empty
    {
    };

    pointer m_begin;
    [[no_unique_address]] std::
        conditional_t<extent == dynamic_extent, size_type, _empty> m_sz;
};

} // namespace tinystd