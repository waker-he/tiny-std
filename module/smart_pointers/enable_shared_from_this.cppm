export module tinystd:enable_shared_from_this;

import std;
import :control_block;
import :shared_ptr;
import :weak_ptr;

namespace tinystd
{

export class [[clang::trivial_abi]] enable_shared_from_this
{
public:
    template <class Self>
    [[nodiscard]] auto
    shared_from_this(this Self&& self
    ) -> shared_ptr<std::remove_reference_t<Self>>
    {
        if (!self.m_cb) throw std::runtime_error{"bad shared_from_this"};
        self.m_cb->increment_shared();
        return shared_ptr<std::remove_reference_t<Self>>(
            std::addressof(self), self.m_cb
        );
    }

    template <class Self>
    [[nodiscard]] auto
    weak_from_this(this Self&& self) -> weak_ptr<std::remove_reference_t<Self>>
    {
        if (!self.m_cb) throw std::runtime_error{"bad weak_from_this"};
        self.m_cb->increment_weak();
        return weak_ptr<std::remove_reference_t<Self>>(
            std::addressof(self), self.m_cb
        );
    }

protected:
    enable_shared_from_this() noexcept : m_cb{nullptr} {}
    enable_shared_from_this(enable_shared_from_this const &) noexcept
        : m_cb{nullptr}
    {
    }
    ~enable_shared_from_this() = default;
    auto
    operator=(enable_shared_from_this const &) noexcept
        -> enable_shared_from_this&
    {
        return *this;
    }

private:
    control_block* m_cb;

    template <non_array T>
    friend class shared_ptr;

    template <typename T, typename... Args>
    friend auto
    make_shared(Args&&...) -> shared_ptr<T>;
};

} // namespace tinystd