module;

// To use hazard pointer, we must include unordered_flat_set, otherwise we get
// compiler error saying that SIMD intrinsics are not found.
#include <boost/unordered/unordered_flat_set.hpp>
export module tinystd:control_block;

import std;
import :manual_lifetime;
import :hazard_pointer;

namespace tinystd
{

class control_block
{
public:
    using count_type = std::uint32_t;

    control_block() noexcept : m_shared_count{1}, m_weak_count{1} {}
    virtual ~control_block() noexcept = default;

    void
    increment_shared() noexcept
    {
        m_shared_count.fetch_add(1, std::memory_order_relaxed);
    }

    void
    increment_weak() noexcept
    {
        m_weak_count.fetch_add(1, std::memory_order_relaxed);
    }

    // returns whether the increment succeeds
    bool
    increment_shared_if_not_zero() noexcept
    {
        auto old_cnt = m_shared_count.load(std::memory_order_relaxed);
        do {
            if (old_cnt == 0) return false;
        } while (!m_shared_count.compare_exchange_weak(
            old_cnt, old_cnt + 1, std::memory_order_relaxed
        ));
        return true;
    }

    void
    decrement_shared() noexcept
    {
        if (m_shared_count.fetch_sub(1, std::memory_order_release) == 1)
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete_obj();
            decrement_weak();
        }
    }

    void
    decrement_weak() noexcept
    {
        if (m_weak_count.fetch_sub(1, std::memory_order_relaxed) == 1)
        {
            // We can make the hazard pointer non-intrusive here by increasing
            // the weak_count when using hazard pointer and setting the custom
            // deleter to decrement_weak(), but that involves additional
            // atomic operations and might not worth the non-intrusive property.
            hazard_pointer<control_block>::retire(this);
            // ::delete this;
        }
    }

    auto
    shared_count() const noexcept -> count_type
    {
        return m_shared_count.load(std::memory_order_relaxed);
    }

    virtual auto
    get_ptr() noexcept -> void* = 0;

protected:
    virtual void
    delete_obj() noexcept = 0;

private:
    std::atomic<count_type> m_shared_count; // #shared
    std::atomic<count_type> m_weak_count;   // #weak + (#shared != 0)
};

template <typename T>
class control_block_with_ptr : public control_block
{
public:
    control_block_with_ptr() noexcept : m_ptr{nullptr} {}
    control_block_with_ptr(T* ptr) noexcept : m_ptr{ptr} {}
    void
    delete_obj() noexcept override
    {
        ::delete m_ptr;
    }

    auto
    get_ptr() noexcept -> void* override
    {
        return m_ptr;
    }

private:
    T* m_ptr;
};

template <typename T>
class control_block_with_obj : public control_block
{
public:
    template <typename... Args>
    control_block_with_obj(Args&&... args) noexcept
    {
        m_obj.emplace(std::forward<Args>(args)...);
    }

    void
    delete_obj() noexcept override
    {
        m_obj.destroy();
    }

    auto
    get_ptr() noexcept -> void* override
    {
        return std::addressof(m_obj.get());
    }

private:
    [[no_unique_address]] manual_lifetime<T> m_obj;
};

} // namespace tinystd