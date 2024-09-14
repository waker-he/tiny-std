export module tinystd:shared_ptr;

import std;
import :unique_ptr;
import :control_block;

namespace tinystd
{

export template <non_array T>
class weak_ptr;

export class enable_shared_from_this;

export template <non_array T>
class atomic_shared_ptr;

export template <non_array T>
class [[clang::trivial_abi]] shared_ptr
{
public:
    using element_type = T;
    using weak_type    = weak_ptr<T>;

    // Constructors
    shared_ptr() noexcept : m_ptr{nullptr}, m_cb{nullptr} {}
    shared_ptr(std::nullptr_t) noexcept : m_ptr{nullptr}, m_cb{nullptr} {}

    template <pointer_convertible_to<T> U>
    explicit shared_ptr(U* ptr)
        : m_ptr{ptr}
        , m_cb{::new control_block_with_ptr<U>(ptr)}
    {
        if constexpr (std::derived_from<U, enable_shared_from_this>)
        {
            ptr->m_cb = m_cb;
        }
    }

    // copy/move constructors
    shared_ptr(shared_ptr const & other) noexcept
        : m_ptr{other.m_ptr}
        , m_cb{other.m_cb}
    {
        if (m_cb) m_cb->increment_shared();
    }

    shared_ptr(shared_ptr&& other) noexcept
        : m_ptr{std::exchange(other.m_ptr, nullptr)}
        , m_cb{std::exchange(other.m_cb, nullptr)}
    {
    }

    template <pointer_convertible_to<T> U>
    shared_ptr(shared_ptr<U> const & other) noexcept
        : m_ptr{other.m_ptr}
        , m_cb{other.m_cb}
    {
        if (m_cb) m_cb->increment_shared();
    }

    template <pointer_convertible_to<T> U>
    shared_ptr(shared_ptr<U>&& other) noexcept
        : m_ptr{std::exchange(other.m_ptr, nullptr)}
        , m_cb{std::exchange(other.m_cb, nullptr)}
    {
    }

    // constructed from other smart pointers
    template <pointer_convertible_to<T> U>
    shared_ptr(unique_ptr<U>&& uptr)
        : m_ptr{uptr.release()}
        , m_cb{
              m_ptr ? ::new control_block_with_ptr<U>(static_cast<U*>(m_ptr))
                    : nullptr
          }
    {
        if constexpr (std::derived_from<U, enable_shared_from_this>)
        {
            if (m_ptr) m_ptr->m_cb = m_cb;
        }
    }

    // same as weak_ptr<T>::lock() except that it thows if weak_ptr is empty
    template <pointer_convertible_to<T> U>
    shared_ptr(weak_ptr<U> const & wptr)
    {
        shared_ptr(wptr.lock()).swap(*this);
        if (!m_cb) throw std::runtime_error{"bad weak_ptr"};
    }

    // aliasing constructors
    template <typename U>
    shared_ptr(shared_ptr<U> const & other, T* ptr) noexcept
        : m_ptr{ptr}
        , m_cb{other.m_cb}
    {
        if (m_cb) m_cb->increment_shared();
    }

    template <typename U>
    shared_ptr(shared_ptr<U>&& other, T* ptr) noexcept
        : m_ptr{ptr}
        , m_cb{std::exchange(other.m_cb, nullptr)}
    {
        other.m_ptr = nullptr;
    }


    // Destructor
    ~shared_ptr()
    {
        if (m_cb) m_cb->decrement_shared();
    }


    // Assignments
    auto
    operator=(shared_ptr const & rhs) noexcept -> shared_ptr&
    {
        if (m_cb != rhs.m_cb)
        {
            reset();
            m_ptr = rhs.m_ptr;
            m_cb  = rhs.m_cb;
            if (m_cb) m_cb->increment_shared();
        }
        return *this;
    }

    auto
    operator=(shared_ptr&& rhs) noexcept -> shared_ptr&
    {
        if (m_cb != rhs.m_cb)
        {
            reset();
            m_ptr = std::exchange(rhs.m_ptr, nullptr);
            m_cb  = std::exchange(rhs.m_cb, nullptr);
        }
        return *this;
    }

    template <pointer_convertible_to<T> U>
    auto
    operator=(shared_ptr<U> const & rhs) noexcept -> shared_ptr&
    {
        if (m_cb != rhs.m_cb)
        {
            reset();
            m_ptr = rhs.m_ptr;
            m_cb  = rhs.m_cb;
            if (m_cb) m_cb->increment_shared();
        }
        return *this;
    }

    template <pointer_convertible_to<T> U>
    auto
    operator=(shared_ptr<U>&& rhs) noexcept -> shared_ptr&
    {
        if (m_cb != rhs.m_cb)
        {
            reset();
            m_ptr = std::exchange(rhs.m_ptr, nullptr);
            m_cb  = std::exchange(rhs.m_cb, nullptr);
        }
        return *this;
    }

    template <pointer_convertible_to<T> U>
    auto
    operator=(unique_ptr<U>&& rhs) -> shared_ptr&
    {
        reset();
        auto ptr = rhs.release();
        m_ptr    = ptr;
        m_cb     = ::new control_block_with_ptr<U>(ptr);
        return *this;
    }


    // Modifiers
    void
    swap(shared_ptr& other) noexcept
    {
        std::swap(m_ptr, other.m_ptr);
        std::swap(m_cb, other.m_cb);
    }

    void
    reset() noexcept
    {
        if (m_cb)
        {
            m_cb->decrement_shared();
            m_ptr = nullptr;
            m_cb  = nullptr;
        }
    }

    template <pointer_convertible_to<T> U>
    void
    reset(U* ptr)
    {
        reset();
        if (ptr)
        {
            m_ptr = ptr;
            if (ptr) m_cb = ::new control_block_with_ptr<U>(ptr);
        }
    }


    // Observers
    [[nodiscard]] auto
    get() const noexcept -> T*
    {
        return m_ptr;
    }

    auto
    operator*() const noexcept(noexcept(*std::declval<T*>())) -> T&
    {
        return *m_ptr;
    }

    [[nodiscard]] auto
    operator->() const noexcept -> T*
    {
        return m_ptr;
    }

    [[nodiscard]] auto
    use_count() const noexcept -> long
    {
        return m_cb ? m_cb->shared_count() : 0;
    }

    [[nodiscard]] operator bool() const noexcept { return m_cb; }

private:
    T*             m_ptr;
    control_block* m_cb;

    shared_ptr(T* ptr, control_block* cb) noexcept : m_ptr{ptr}, m_cb{cb} {}

    template <non_array U>
    friend class shared_ptr;

    template <non_array U>
    friend class weak_ptr;

    friend class enable_shared_from_this;

    template <non_array U>
    friend class atomic_shared_ptr;

    template <typename U, typename... Args>
    friend auto
    make_shared(Args&&...) -> shared_ptr<U>;
};

export template <typename T>
void
swap(shared_ptr<T>& lhs, shared_ptr<T>& rhs) noexcept
{
    lhs.swap(rhs);
}

export template <typename T, typename... Args>
[[nodiscard]] auto
make_shared(Args&&... args) -> shared_ptr<T>
{
    auto cb_ptr = ::new control_block_with_obj<T>(std::forward<Args>(args)...);
    if constexpr (std::derived_from<T, enable_shared_from_this>)
    {
        static_cast<T*>(cb_ptr->get_ptr())->m_cb = cb_ptr;
    }
    return shared_ptr<T>(static_cast<T*>(cb_ptr->get_ptr()), cb_ptr);
}

export template <typename T1, typename T2>
[[nodiscard]] auto
operator==(shared_ptr<T1> const & lhs, shared_ptr<T2> const & rhs) noexcept
    -> bool
{
    return lhs.get() == rhs.get();
}

export template <typename T1, typename T2>
[[nodiscard]] auto
operator<=>(shared_ptr<T1> const & lhs, shared_ptr<T2> const & rhs) noexcept
    -> std::strong_ordering
{
    return lhs.get() <=> rhs.get();
}

} // namespace tinystd