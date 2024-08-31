export module tinystd:manual_lifetime;

import std;

namespace tinystd
{

template <typename T>
class manual_lifetime
{
public:
    manual_lifetime()                        = default;
    manual_lifetime(manual_lifetime const &) = delete;
    auto
    operator=(manual_lifetime const &) = delete;
    ~manual_lifetime()                 = default;

    template <typename... Args>
    void
    emplace(Args&&... args)
    {
        std::construct_at(
            reinterpret_cast<T*>(m_storage), std::forward<Args>(args)...
        );
    }

    void
    destroy() noexcept
    {
        std::destroy_at(reinterpret_cast<T*>(m_storage));
    }

    auto
    get() noexcept -> T&
    {
        return *reinterpret_cast<T*>(m_storage);
    }

    auto
    get() const noexcept -> T const &
    {
        return *reinterpret_cast<T const *>(m_storage);
    }

private:
    [[no_unique_address]] alignas(T) char m_storage[sizeof(T)];
};

} // namespace tinystd