export module tinystd:atomic_shared_ptr;

import std;
import :unique_ptr;
import :control_block;
import :shared_ptr;
import :hazard_pointer;

namespace tinystd
{

export template <non_array T>
class atomic_shared_ptr
{
public:
    constexpr static bool is_always_lock_free =
        std::atomic<void*>::is_always_lock_free;

    // Constructors
    atomic_shared_ptr() noexcept : m_cb{nullptr} {}
    atomic_shared_ptr(shared_ptr<T> desired) noexcept : m_cb{desired.m_cb}
    {
        desired.m_cb  = nullptr;
        desired.m_ptr = nullptr;
    }
    atomic_shared_ptr(atomic_shared_ptr const &) = delete;

    ~atomic_shared_ptr() noexcept { store(nullptr); }

    // Assignments
    void
    operator=(shared_ptr<T> desired)
    {
        store(std::move(desired));
    }
    auto
    operator=(atomic_shared_ptr const &) = delete;

    [[nodiscard("no side effect")]] auto
    is_lock_free() const noexcept -> bool
    {
        return m_cb.is_lock_free();
    }

    // memory_order does not matter for load operation
    auto
    load([[maybe_unused]] std::memory_order order = std::memory_order_seq_cst)
        const noexcept -> shared_ptr<T>
    {
        auto hp = make_hazard_pointer<control_block>();
        auto cb = hp.protect(m_cb);
        while (cb && !cb->increment_shared_if_not_zero())
        {
            // a store happens after we load m_cb, need to reload
            cb = hp.protect(m_cb);
        }
        return make_shared_from_cb(cb);
    }

    void
    store(
        shared_ptr<T>     desired,
        std::memory_order order = std::memory_order_seq_cst
    ) noexcept
    {
        desired.m_ptr = nullptr;
        auto new_cb   = std::exchange(desired.m_cb, nullptr);
        auto old_cb   = m_cb.exchange(new_cb, order);
        if (old_cb) old_cb->decrement_shared();
    }

    auto
    exchange(
        shared_ptr<T>     desired,
        std::memory_order order = std::memory_order_seq_cst
    ) noexcept -> shared_ptr<T>
    {
        desired.m_ptr = nullptr;
        auto new_cb   = std::exchange(desired.m_cb, nullptr);
        auto old_cb   = m_cb.exchange(new_cb, order);
        return make_shared_from_cb(old_cb);
    }

    [[nodiscard("might have ABA")]] auto
    compare_exchange_weak(
        shared_ptr<T>&    expected,
        shared_ptr<T>&&   desired,
        std::memory_order success,
        std::memory_order failure
    ) noexcept -> bool
    {
        // Avoid changing expected.m_cb when doing CAS on m_cb
        auto expected_cb = expected.m_cb;
        // spurious failure can be expensive as we need a load if failed
        if (m_cb.compare_exchange_strong(
                expected_cb, desired.m_cb, success, failure
            ))
        {
            desired.m_ptr = nullptr;
            desired.m_cb  = nullptr;
            if (expected_cb) expected_cb->decrement_shared();
            return true;
        }
        else
        {
            // if failed, desired won't be moved from and will stay the same.

            // There might be ABA and expected stays the same, so for
            // compare_exchange_strong, we have to use a different algorithm
            // than compare_exchange_weak.
            expected = load();
            return false;
        }
    }

    // To avoid letting user find that it returns false but expected stays the
    // same, we need to use a different algorithm than compare_exchange_weak.
    auto
    compare_exchange_strong(
        shared_ptr<T>&    expected,
        shared_ptr<T>&&   desired,
        std::memory_order success,
        std::memory_order failure
    ) noexcept -> bool
    {
        auto old_expected_cb = expected.m_cb;
        do {
            if (compare_exchange_weak(
                    expected, std::move(desired), success, failure
                ))
            {
                return true;
            }
        } while (old_expected_cb == expected.m_cb);
        // Loops if expected ABAs

        return false;
    }

    [[nodiscard("might have spurious failure")]] auto
    compare_exchange_weak(
        shared_ptr<T>&    expected,
        shared_ptr<T>&&   desired,
        std::memory_order order = std::memory_order_seq_cst
    ) noexcept -> bool
    {
        return compare_exchange_weak(
            expected, std::move(desired), order, order
        );
    }

    auto
    compare_exchange_strong(
        shared_ptr<T>&    expected,
        shared_ptr<T>&&   desired,
        std::memory_order order = std::memory_order_seq_cst
    ) noexcept -> bool
    {
        return compare_exchange_strong(
            expected, std::move(desired), order, order
        );
    }

private:
    std::atomic<control_block*> m_cb;

    static auto
    make_shared_from_cb(control_block* cb) noexcept -> shared_ptr<T>
    {
        if (cb == nullptr) return shared_ptr<T>();
        return shared_ptr<T>(static_cast<T*>(cb->get_ptr()), cb);
    }
};

} // namespace tinystd