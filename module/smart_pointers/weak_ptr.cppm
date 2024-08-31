export module tinystd:weak_ptr;

import std;
import :unique_ptr;
import :control_block;
import :shared_ptr;

namespace tinystd
{

export class enable_shared_from_this;

export template <non_array T>
class [[clang::trivial_abi]] weak_ptr
{
public:
    using element_type = T;

    // Constructors
    weak_ptr() noexcept : m_ptr{nullptr}, m_cb{nullptr} {}

    weak_ptr(weak_ptr const & other) noexcept
        : m_ptr{other.m_ptr}
        , m_cb{other.m_cb}
    {
        if (m_cb) m_cb->increment_weak();
    }

    weak_ptr(weak_ptr&& other) noexcept
        : m_ptr{std::exchange(other.m_ptr, nullptr)}
        , m_cb{std::exchange(other.m_cb, nullptr)}
    {
    }

    template <pointer_convertible_to<T> U>
    weak_ptr(weak_ptr<U> const & other) noexcept
        : m_ptr{other.m_ptr}
        , m_cb{other.m_cb}
    {
        if (m_cb) m_cb->increment_weak();
    }

    template <pointer_convertible_to<T> U>
    weak_ptr(weak_ptr<U>&& other) noexcept
        : m_ptr{std::exchange(other.m_ptr, nullptr)}
        , m_cb{std::exchange(other.m_cb, nullptr)}
    {
    }

    template <pointer_convertible_to<T> U>
    weak_ptr(shared_ptr<U> const & other) noexcept
        : m_ptr{other.m_ptr}
        , m_cb{other.m_cb}
    {
        if (m_cb) m_cb->increment_weak();
    }

    // Destructor
    ~weak_ptr() noexcept
    {
        if (m_cb) m_cb->decrement_weak();
    }

    // Assignments
    auto
    operator=(weak_ptr rhs) noexcept -> weak_ptr&
    {
        swap(rhs);
        return *this;
    }

    template <pointer_convertible_to<T> U>
    auto
    operator=(weak_ptr<U> rhs) noexcept -> weak_ptr&
    {
        weak_ptr(std::move(rhs)).swap(rhs);
        return *this;
    }

    template <pointer_convertible_to<T> U>
    auto
    operator=(shared_ptr<U> const & rhs) noexcept -> weak_ptr&
    {
        reset();
        m_ptr = rhs.m_ptr;
        m_cb  = rhs.m_cb;
        if (m_cb) m_cb->increment_weak();
        return *this;
    }

    // Modifiers
    void
    reset() noexcept
    {
        if (m_cb)
        {
            m_cb->decrement_weak();
            m_cb  = nullptr;
            m_ptr = nullptr;
        }
    }

    void
    swap(weak_ptr& rhs)
    {
        std::swap(m_ptr, rhs.m_ptr);
        std::swap(m_cb, rhs.m_cb);
    }

    // Observers
    [[nodiscard]] auto
    use_count() const noexcept -> long
    {
        return m_cb ? m_cb->shared_count() : 0;
    }

    [[nodiscard]] auto
    expired() const noexcept -> bool
    {
        return use_count() == 0;
    }

    [[nodiscard]] auto
    lock() const noexcept -> shared_ptr<T>
    {
        return m_cb && m_cb->increment_shared_if_not_zero()
                 ? shared_ptr<T>{m_ptr, m_cb}
                 : shared_ptr<T>{};
    }

private:
    T*             m_ptr;
    control_block* m_cb;

    weak_ptr(T* ptr, control_block* cb) noexcept : m_ptr{ptr}, m_cb{cb} {}

    template <non_array U>
    friend class weak_ptr;

    friend class enable_shared_from_this;
};

export template <typename T>
void
swap(weak_ptr<T>& lhs, weak_ptr<T>& rhs) noexcept
{
    lhs.swap(rhs);
}

} // namespace tinystd