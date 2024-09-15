export module tinystd:any;

import std;
import :unique_ptr;

namespace tinystd
{

export class any;

template <typename T>
concept is_any = std::same_as<std::decay_t<T>, any>;

template <typename T>
concept any_constructible = (!is_any<T>) && std::copyable<std::decay_t<T>>;

export class any
{
    struct Concept
    {
        virtual ~Concept() = default;
        [[nodiscard]] constexpr virtual auto
        clone() const -> unique_ptr<Concept> = 0;
        [[nodiscard]] constexpr virtual auto
        type() const noexcept -> std::type_info const & = 0;
    };

    template <std::copyable T>
    struct OwningModel : Concept
    {
        T m_obj;

        template <typename... Args>
        constexpr OwningModel(Args&&... args)
            : m_obj(std::forward<Args>(args)...)
        {
        }

        [[nodiscard]] constexpr auto
        clone() const -> unique_ptr<Concept> override
        {
            return tinystd::make_unique<OwningModel>(m_obj);
        }

        [[nodiscard]] constexpr auto
        type() const noexcept -> std::type_info const & override
        {
            return typeid(T);
        }
    };

public:
    constexpr any() noexcept = default;
    constexpr any(any const & other)
        : m_pimpl(other.m_pimpl ? other.m_pimpl->clone() : nullptr)
    {
    }

    constexpr any(any&& other) noexcept = default;

    // Constructed from any std::copyable type
    // If not constrained, when copy constructing from any&, this constructor
    // will be preferred
    template <any_constructible T>
    constexpr any(T&& obj)
        : m_pimpl(tinystd::make_unique<OwningModel<std::decay_t<T>>>(
              std::forward<T>(obj)
          ))
    {
    }

    constexpr ~any() = default;

    // Assignment
    // Can be assigned from lvalue, rvalue and std::copyable type T
    constexpr auto
    operator=(any rhs) noexcept -> any&
    {
        swap(rhs);
        return *this;
    }

    // Modifiers
    constexpr void
    swap(any& rhs) noexcept
    {
        m_pimpl.swap(rhs.m_pimpl);
    }

    template <any_constructible T, typename... Args>
    constexpr auto
    emplace(Args&&... args) -> T&
    {
        auto ptr =
            tinystd::make_unique<OwningModel<T>>(std::forward<Args>(args)...);
        T& ret  = ptr->m_obj;
        m_pimpl = std::move(ptr);
        return ret;
    }

    constexpr void
    reset() noexcept
    {
        m_pimpl.reset();
    }

    // Observers
    [[nodiscard]] constexpr auto
    has_value() const noexcept -> bool
    {
        return static_cast<bool>(m_pimpl);
    }

    [[nodiscard]] constexpr auto
    type() const noexcept -> std::type_info const &
    {
        return m_pimpl->type();
    }

private:
    unique_ptr<Concept> m_pimpl;

    template <typename T>
    friend constexpr auto
    any_cast(any* operand) noexcept -> T*;

    template <typename T>
    friend constexpr auto
    any_cast(any const * operand) noexcept -> T const *;
};

export constexpr void
swap(any& lhs, any& rhs) noexcept
{
    return lhs.swap(rhs);
}

export template <std::copyable T, typename... Args>
constexpr auto
make_any(Args&&... args) -> any
{
    any a;
    a.emplace<T>(std::forward<Args>(args)...);
    return a;
}

export template <typename T>
constexpr auto
any_cast(any* operand) noexcept -> T*
{
    return operand->type() == typeid(T)
             ? std::addressof(
                   static_cast<any::OwningModel<T>*>(operand->m_pimpl.get())
                       ->m_obj
               )
             : nullptr;
}

export template <typename T>
constexpr auto
any_cast(any const * operand) noexcept -> T const *
{
    return operand->type() == typeid(T)
             ? std::addressof(static_cast<any::OwningModel<T> const *>(
                                  operand->m_pimpl.get()
               )
                                  ->m_obj)
             : nullptr;
}

} // namespace tinystd