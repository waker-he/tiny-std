export module tinystd:small_vector;

import std;
import :vector_mixin;

namespace tinystd
{

export template <typename T, std::size_t N>
class small_vector : private vector_mixin<T>
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
    constexpr small_vector() noexcept
        : m_data{get_buffer()}
        , m_sz{0}
        , m_capacity{N}
    {
    }

    constexpr small_vector(small_vector const & other) : m_sz(other.m_sz)
    {
        if (m_sz > N)
        {
            m_capacity = m_sz;
            m_data     = m_alloc.allocate(m_capacity);
        }
        else
        {
            m_capacity = N;
            m_data     = get_buffer();
        }
        uninitialized_copy(other.begin(), other.end(), m_data);
    }

    constexpr small_vector(small_vector&& other)
        noexcept(N == 0 || nothrow_relocatable<T>)
        : m_sz(other.m_sz)
    {
        if (other.objects_on_heap())
        {
            m_capacity = std::exchange(other.m_capacity, N);
            m_data     = std::exchange(other.m_data, other.get_buffer());
        }
        else
        {
            m_capacity = N;
            m_data     = get_buffer();
            relocate(other.begin(), other.end(), m_data); // might throw here
        }
        other.m_sz = 0;
    }

    // assignments
    constexpr auto
    operator=(small_vector rhs)
        noexcept(noexcept(rhs.swap(*this))) -> small_vector&
    {
        swap(rhs);
        return *this;
    }

    // destructor
    constexpr ~small_vector() noexcept
    {
        destroy_all();
        deallocate();
    }

    // access and observers
    template <class Self>
    [[nodiscard]] constexpr auto
    data(this Self&& self
    ) noexcept -> std::conditional_t<std::is_const_v<Self>, T const *, T*>
    {
        return self.m_data;
    }

    [[nodiscard]] constexpr auto
    size() const noexcept -> size_type
    {
        return m_sz;
    }

    [[nodiscard]] constexpr auto
    capacity() const noexcept -> size_type
    {
        return m_capacity;
    }

    // modifiers
    constexpr void
    swap(small_vector& other) noexcept(N == 0 || nothrow_relocatable<T>)
    {
        if constexpr (N == 0)
        {
            std::swap(m_data, other.m_data);
            std::swap(m_sz, other.m_sz);
            std::swap(m_capacity, other.m_capacity);
        }
        else
        {
            if (other.objects_on_heap() && objects_on_heap())
            {
                std::swap(m_data, other.m_data);
                std::swap(m_sz, other.m_sz);
                std::swap(m_capacity, other.m_capacity);
            }
            else
            {
                if constexpr (nothrow_relocatable<T>)
                {
                    // relocate lhs to temp
                    small_vector temp(std::move(*this));

                    // relocate rhs to lhs
                    if (other.objects_on_heap())
                    {
                        m_data =
                            std::exchange(other.m_data, other.get_buffer());
                    }
                    else { relocate(other.begin(), other.end(), m_data); }
                    m_sz       = other.m_sz;
                    m_capacity = other.m_capacity;

                    // relocate temp to rhs
                    if (temp.objects_on_heap())
                    {
                        other.m_data =
                            std::exchange(temp.m_data, temp.get_buffer());
                    }
                    else { relocate(temp.begin(), temp.end(), other.m_data); }
                    other.m_sz       = std::exchange(temp.m_sz, 0);
                    other.m_capacity = std::exchange(temp.m_capacity, N);
                }
                else
                {
                    // return <data, capacity>
                    auto relocate_to_heap = [](small_vector& v
                                            ) -> std::pair<T*, size_type>
                    {
                        T*        new_data;
                        size_type cap;
                        if (v.objects_on_heap())
                        {
                            new_data = v.m_data;
                            cap      = v.m_capacity;
                        }
                        else
                        {
                            new_data = v.m_alloc.allocate(v.m_sz);
                            try
                            {
                                relocate(v.begin(), v.end(), new_data);
                            }
                            catch (...)
                            {
                                v.m_alloc.deallocate(new_data, v.m_sz);
                                throw;
                            }
                            cap = v.m_sz;
                        }
                        return {new_data, cap};
                    };

                    auto [lhs_data, lhs_cap] = relocate_to_heap(*this);
                    try
                    {
                        auto [rhs_data, rhs_cap] = relocate_to_heap(other);
                        m_data                   = rhs_data;
                        m_capacity               = rhs_cap;
                        other.m_data             = lhs_data;
                        other.m_capacity         = lhs_cap;
                        std::swap(m_sz, other.m_sz);
                    }
                    catch (...)
                    {
                        m_data     = lhs_data;
                        m_capacity = lhs_cap;
                        throw;
                    }
                }
            }
        }
    }

    template <typename... Args>
    constexpr auto
    emplace_back(Args&&... args) -> T&
    {
        if (m_sz == m_capacity) grow();
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
    T*        m_data;
    size_type m_sz;
    size_type m_capacity;

    struct emptyS
    {
    };
    using storage_t = std::conditional_t<
        N == 0,
        emptyS,
        std::conditional_t<sbo_can_be_constexpr<T>, T[N], char[N * sizeof(T)]>>;
    [[no_unique_address]] alignas(T) storage_t m_buffer;
    [[no_unique_address]] std::allocator<T> m_alloc;


    [[nodiscard]] constexpr auto
    get_buffer() noexcept -> T*
    {
        if constexpr (N == 0) { return nullptr; }
        else if constexpr (sbo_can_be_constexpr<T>) { return m_buffer; }
        else { return reinterpret_cast<T*>(&m_buffer); }
    }

    [[nodiscard]] constexpr auto
    objects_on_heap() noexcept -> bool
    {
        if constexpr (N == 0) { return true; }
        else { return m_data != get_buffer(); }
    }

    constexpr void
    grow()
    {
        size_type new_cap  = std::max(1ul, m_capacity * 2);
        T*        new_data = m_alloc.allocate(new_cap);
        try
        {
            relocate(begin(), end(), new_data);
        }
        catch (...)
        {
            m_alloc.deallocate(new_data, new_cap);
            throw;
        }
        deallocate();
        m_data     = new_data;
        m_capacity = new_cap;
    }

    constexpr void
    deallocate() noexcept
    {
        if (objects_on_heap() && m_data != nullptr)
        {
            m_alloc.deallocate(m_data, m_capacity);
        }
    }

    constexpr void
    destroy_all() noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            // compiler can optimize this away if trivially destructible,
            // we write it explicitly because ending lifetime twice is not
            // allowed in constexpr evaluation
            std::destroy(begin(), end());
        }
    }
};

export template <typename T, std::size_t N>
constexpr void
swap(small_vector<T, N>& v1, small_vector<T, N>& v2)
    noexcept(noexcept(v1.swap(v2)))
{
    v1.swap(v2);
}


} // namespace tinystd