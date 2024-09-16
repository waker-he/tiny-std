export module tinystd:function;

import std;

namespace tinystd
{


export template <class>
class function;

template <typename F, typename R, typename... Args>
concept function_constructible =
    (!std::same_as<std::decay_t<F>, function<R(Args...)>>)
    && std::copyable<std::decay_t<F>> && std::invocable<F, Args...>
    && std::convertible_to<std::invoke_result_t<F, Args...>, R>;

export template <typename R, typename... Args>
class function<R(Args...)>
{
    constexpr static std::size_t buffer_size = sizeof(void*);

    using clone_t  = void (*)(char const *, char*);
    using delete_t = void (*)(char*);
    using invoke_t = R (*)(char*, Args...);

public:
    function() noexcept : m_invoke{nullptr} {}
    function(std::nullptr_t) noexcept : function() {}

    function(function const & other)
        : m_clone{other.m_clone}
        , m_delete{other.m_delete}
        , m_invoke{other.m_invoke}
    {
        if (other) { other.m_clone(other.m_buf.data(), m_buf.data()); }
    }

    function(function&& other) noexcept
        : m_clone{other.m_clone}
        , m_delete{other.m_delete}
        , m_invoke{other.m_invoke}
    {
        if (other)
        {
            m_buf = other.m_buf;
            other.reset();
        }
    }

    template <function_constructible<R, Args...> F>
    function(F&& f)
    {
        using func_t                  = std::decay_t<F>;
        constexpr static bool inplace = sizeof(func_t) <= buffer_size;
        if constexpr (inplace)
        {
            std::construct_at(
                reinterpret_cast<func_t*>(m_buf.data()), std::forward<F>(f)
            );
        }
        else
        {
            std::construct_at(
                reinterpret_cast<func_t**>(m_buf.data()),
                ::new func_t{std::forward<F>(f)}
            );
        }

        m_clone = [](char const * src, char* dst)
        {
            if constexpr (inplace)
            {
                std::construct_at(
                    reinterpret_cast<func_t*>(dst),
                    *reinterpret_cast<func_t const *>(src)
                );
            }
            else
            {
                auto& ptr = *reinterpret_cast<func_t**>(dst);
                ptr =
                    new func_t{**reinterpret_cast<func_t const * const *>(src)};
            }
        };
        m_delete = [](char* buf)
        {
            if constexpr (inplace) { std::destroy_at(get_ptr<func_t>(buf)); }
            else { delete get_ptr<func_t>(buf); }
        };
        m_invoke = [](char* buf, Args... args) -> R
        {
            return std::invoke(
                *get_ptr<func_t>(buf), std::forward<Args>(args)...
            );
        };
    }

    ~function() noexcept
    {
        if (*this) m_delete(m_buf.data());
    }

    // Can be assigned from nullptr, lvalue/rvalue of function or any
    // function_constructible type.
    auto
    operator=(function f) noexcept -> function&
    {
        swap(f);
        return *this;
    }

    // UB if *this does not sotre a callable target, user is responsible for
    // checking before invoking. Since target might be stored in buffer, this
    // call operator cannot be const.
    auto
    operator()(Args... args) -> R
    {
        return m_invoke(m_buf.data(), std::forward<Args>(args)...);
    }

    operator bool() const noexcept { return m_invoke != nullptr; }

    void
    reset() noexcept
    {
        m_invoke = nullptr;
    }

    void
    swap(function& rhs) noexcept
    {
        std::swap(m_buf, rhs.m_buf);
        std::swap(m_clone, rhs.m_clone);
        std::swap(m_delete, rhs.m_delete);
        std::swap(m_invoke, rhs.m_invoke);
    }

private:
    alignas(void*) std::array<char, buffer_size> m_buf;
    clone_t  m_clone;
    delete_t m_delete;
    invoke_t m_invoke;

    template <typename F>
    static auto
    get_ptr(char* buf) noexcept -> F*
    {
        if constexpr (sizeof(F) <= buffer_size)
        {
            return reinterpret_cast<F*>(buf);
        }
        else { return *reinterpret_cast<F**>(buf); }
    }
};

export template <typename R, typename... Args>
void
swap(function<R(Args...)>& lhs, function<R(Args...)>& rhs) noexcept
{
    lhs.swap(rhs);
}

export template <typename R, typename... Args>
bool
operator==(function<R(Args...)> const & f, std::nullptr_t) noexcept
{
    return !static_cast<bool>(f);
}

} // namespace tinystd