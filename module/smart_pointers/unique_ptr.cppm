export module tinystd:unique_ptr;

import std;

namespace tinystd
{

template <typename From, typename To>
concept pointer_convertible_to = std::convertible_to<From*, To*>;

template <typename T>
concept non_array = !std::is_array_v<T>;

export template <non_array T>
class [[clang::trivial_abi]] unique_ptr
{
public:
    // ctors
    constexpr unique_ptr() noexcept : m_ptr(nullptr) {}
    constexpr unique_ptr(std::nullptr_t) : unique_ptr() {}

    template <pointer_convertible_to<T> U>
    constexpr unique_ptr(U* ptr) noexcept : m_ptr(ptr)
    {
    }

    unique_ptr(unique_ptr const &) = delete;

    template <pointer_convertible_to<T> U>
    constexpr unique_ptr(unique_ptr<U>&& other) noexcept
        : m_ptr{other.release()}
    {
    }
    constexpr unique_ptr(unique_ptr&& other) noexcept : m_ptr{other.release()}
    {
    }

    // assignment
    constexpr auto
    operator=(std::nullptr_t) noexcept -> unique_ptr&
    {
        reset();
        return *this;
    }

    unique_ptr&
    operator=(unique_ptr const &) = delete;

    template <pointer_convertible_to<T> U>
    constexpr auto
    operator=(unique_ptr<U>&& other) noexcept -> unique_ptr&
    {
        reset(other.release());
        return *this;
    }
    constexpr auto
    operator=(unique_ptr&& other) noexcept -> unique_ptr&
    {
        reset(other.release());
        return *this;
    }

    // dtor
    constexpr ~unique_ptr() noexcept
    {
        if (m_ptr) ::delete m_ptr;
    }

    // observers
    [[nodiscard]] constexpr auto
    get() const noexcept -> T*
    {
        return m_ptr;
    }

    constexpr
    operator bool() const noexcept
    {
        return m_ptr != nullptr;
    }

    // modifiers
    [[nodiscard]] constexpr auto
    release() noexcept -> T*
    {
        return std::exchange(m_ptr, nullptr);
    }

    constexpr void
    reset(T* new_ptr = nullptr) noexcept
    {
        T* old_ptr = std::exchange(m_ptr, new_ptr);
        if (old_ptr) ::delete old_ptr;
    }

    constexpr void
    swap(unique_ptr& other) noexcept
    {
        std::swap(m_ptr, other.m_ptr);
    }

    [[nodiscard]] constexpr auto
    operator*() const noexcept(noexcept(*std::declval<T*>())) -> T&
    {
        return *m_ptr;
    }

    [[nodiscard]] constexpr auto
    operator->() const noexcept -> T*
    {
        return m_ptr;
    }

private:
    T* m_ptr;
};

export template <typename T>
constexpr void
swap(unique_ptr<T>& lhs, unique_ptr<T>& rhs) noexcept
{
    lhs.swap(rhs);
}

export template <typename T, typename... Args>
[[nodiscard]] constexpr auto
make_unique(Args&&... args) -> unique_ptr<T>
{
    return unique_ptr<T>(::new T(std::forward<Args>(args)...));
}


} // namespace tinystd