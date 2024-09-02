module;
#include <new> // `std::hardware_destructive_interference_size` not available in std module

export module tinystd:waitfree_spsc_queue;

import std;

namespace tinystd
{

// if the capacity passed to waitfree_spsc_queue constructor is not power of 2,
// convert it to power of 2 by finding the smallest pow2 >= capacity
constexpr auto
smallest_greater_pow2(std::size_t num) noexcept -> std::size_t
{
    std::size_t pow2 = 1;
    while (pow2 < num) pow2 *= 2;
    return pow2;
}

export template <typename T>
    requires std::is_nothrow_move_constructible_v<T>
class waitfree_spsc_queue
{
public:
    using size_type    = std::size_t;
    using element_type = T;

    // constructor
    waitfree_spsc_queue(size_type capacity)
        : m_capacity{smallest_greater_pow2(capacity)}
        , m_mask{m_capacity - 1}
        , m_buffer{std::allocator<T>{}.allocate(m_capacity)}
        , m_push_cursor{0}
        , m_cached_pop_cursor{0}
        , m_pop_cursor{0}
        , m_cached_push_cursor{0}
    {
    }

    // no copy/move semantics
    waitfree_spsc_queue(waitfree_spsc_queue const &) = delete;
    auto
    operator=(waitfree_spsc_queue const &) = delete;

    // destructor
    ~waitfree_spsc_queue() noexcept
    {
        while (pop().has_value());
        std::allocator<T>{}.deallocate(m_buffer, m_capacity);
    }

    // returns whether the emplace succeeds, it will fail if queue is full
    template <typename... Args>
    auto
    emplace(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>) -> bool
    {
        // In push operation, we need the value of push_cursor to
        // - check if queue is full
        // - construct the object at the position pointed by push_cursor
        // To avoid doing the atomic load twice, we first load the push_cursor.
        auto push_cursor = m_push_cursor.load(std::memory_order_relaxed);

        if (push_cursor - m_cached_pop_cursor == m_capacity)
        {
            // pop_cursor might be incremented, we reload pop_cursor into
            // cached_pop_cursor and check if it is full again.
            //
            // Use std::memory_order_acquire here to avoid overwrite the object
            // before pop is complete.
            m_cached_pop_cursor = m_pop_cursor.load(std::memory_order_acquire);
            if (push_cursor - m_cached_pop_cursor == m_capacity)
            {
                return false;
            }
        }

        std::construct_at(at(push_cursor), std::forward<Args>(args)...);

        // release constructed object, ensure the object acquired by pop is not
        // in corrupted state
        m_push_cursor.store(push_cursor + 1, std::memory_order_release);
        return true;
    }

    // returns empty optional if queue is empty
    auto
    pop() noexcept -> std::optional<T>
    {
        std::optional<T> res;

        // similar to push, load pop_cursor first and double-check empty
        auto pop_cursor = m_pop_cursor.load(std::memory_order_relaxed);
        if (pop_cursor == m_cached_push_cursor)
        {
            // acquire the object constructed by push
            m_cached_push_cursor =
                m_push_cursor.load(std::memory_order_acquire);
            if (pop_cursor == m_cached_push_cursor) { return res; }
        }

        auto popped = at(pop_cursor);
        res.emplace(std::move(*popped));
        std::destroy_at(popped);

        m_pop_cursor.store(pop_cursor + 1, std::memory_order_release);
        return res;
    }

private:
    size_type const m_capacity;
    size_type const m_mask;
    T* const        m_buffer;

    alignas(std::hardware_destructive_interference_size
    ) std::atomic<size_type> m_push_cursor;

    // used by pushing thread to check if queue is full, only used by pushing
    // thread so it does not need to be atomic
    alignas(std::hardware_destructive_interference_size
    ) size_type m_cached_pop_cursor;

    alignas(std::hardware_destructive_interference_size
    ) std::atomic<size_type> m_pop_cursor;

    // opposite of cached_pop_cursor
    alignas(std::hardware_destructive_interference_size
    ) size_type m_cached_push_cursor;

    static_assert(std::atomic<size_type>::is_always_lock_free);

    [[nodiscard]] auto
    at(size_type cursor) const noexcept -> T*
    {
        return m_buffer + (cursor & m_mask);
    }
};

} // namespace tinystd