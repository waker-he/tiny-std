module;

#include <boost/unordered/unordered_flat_set.hpp>
#include <new>
export module tinystd:hazard_pointer;

import std;

namespace tinystd
{

template <typename T>
class hp_slot_list
{
public:
    struct alignas(std::hardware_destructive_interference_size) hp_slot
    {
        // True if this hazard pointer slot is owned by a thread.
        // It is also used to form the synchronizes-with relation between the
        // thread that relinquishes this slot and the thread that acquires this
        // slot so that resources in this slot can be reused by another thread.
        std::atomic<bool> in_use{false};

        // The *actual* "Hazard Pointer" that protects the object that it points
        // to. Other threads scan for the set of all such pointers before they
        // clean up.
        std::atomic<T const *> protected_ptr{nullptr};

        // The list of hp_slots are in the form of linked list, so that when
        // multiple threads are trying to append new hp_slot, they can use CAS
        // operation to avoid data race on the tail.
        std::atomic<hp_slot*> next{nullptr};

        // The list of hp_slot is growing monotonically, once hp_slot is
        // appended, it will never get popped, and also once hp_slot.next is set
        // to a new hp_slot, this next pointer remains as const. These
        // properties of hp_slot list make caching possible.
        //
        // This cached list serves two purposes:
        // 1. To avoid atomic load on the next pointer when scanning the list.
        // 2. Have a rough estimate of number of threads running and let the
        // clean up threshold depends on it.
        std::vector<hp_slot*> hp_slot_list_cache;
        std::span<hp_slot*>   init_slots; // init_slots part of the list cache

        // Local retired list for a thread, protected by in_use.
        // When the size exceeds 2 * numOfThreads (estimated by
        // hp_slot_list_cache.size()), we will perform a cleanup and it is
        // guaranteed that we can clean up at least numOfThreads objects. So
        // there are at most O(numOfThreads ^ 2) unreclaimed objects.
        std::vector<std::unique_ptr<T>> retired_list;

        // During cleanup, the list of hazard pointers will be added to
        // protected_set, this is used to avoid scanning the list of hazard
        // pointers once for every object to reclaim.
        boost::unordered_flat_set<T const *> protected_set;

        // hp_slot will never be destroyed, leaked memory will be reclaimed by
        // OS when the process ends.
        ~hp_slot() = delete;

        struct Owner
        {
            hp_slot* owned_slot;
            Owner() : owned_slot{hp_slot_list<T>::get().acquire_slot()} {}

            ~Owner() noexcept
            {
                owned_slot->in_use.store(false, std::memory_order_release);
            }
        };

        void
        retire(T* ptr)
        {
            retired_list.emplace_back(ptr);
            auto const cleanup_threshold =
                (init_slots.size() + hp_slot_list_cache.size()) * 2;
            if (retired_list.size() > cleanup_threshold)
            {
                // Scanning the hazard pointer list to populate protected_set
                for (auto slot_ptr : init_slots)
                {
                    protected_set.insert(
                        slot_ptr->protected_ptr.load(std::memory_order_acquire)
                    );
                }

                for (auto slot_ptr : hp_slot_list_cache)
                {
                    protected_set.insert(
                        slot_ptr->protected_ptr.load(std::memory_order_acquire)
                    );
                }

                auto slot_ptr = hp_slot_list_cache.empty()
                                  ? init_slots.back()
                                  : hp_slot_list_cache.back();
                do {
                    auto next = slot_ptr->next.load(std::memory_order_acquire);
                    if (!next) break;
                    slot_ptr = next;
                    hp_slot_list_cache.push_back(slot_ptr);
                    protected_set.insert(
                        slot_ptr->protected_ptr.load(std::memory_order_acquire)
                    );
                } while (true);

                // Complete scanning, try to reclaim any object that is not
                // protected
                std::erase_if(
                    retired_list,
                    [&](auto& ptr)
                    { return !protected_set.contains(ptr.get()); }
                );

                // clear all elements while maintaining the memory for next use
                protected_set.clear();
            }
        }
    };

private:
    hp_slot_list() : init_slots(std::thread::hardware_concurrency())
    {
        for (auto& slot_ptr : init_slots) { slot_ptr = ::new hp_slot{}; }

        for (auto slot_ptr : init_slots) { slot_ptr->init_slots = init_slots; }
    }

public:
    // Singleton Access
    static auto
    get() -> hp_slot_list&
    {
        // hp_slots are intentionally leaked here to handle following situation:
        // A detached thread acquires a hp_slot and relinquish it after main
        // thread exits, at which point global static objects would have already
        // been destroyed.
        //
        // It is acceptable because the number of leaked hp_slots is bounded by
        // number of threads.
        alignas(hp_slot_list) static char buf[sizeof(hp_slot_list)];
        static auto*                      list = ::new (&buf) hp_slot_list{};
        // std::construct_at(reinterpret_cast<hp_slot_list*>(&buf));
        return *list;
    }

    ~hp_slot_list() = delete;

    [[nodiscard]] auto
    acquire_slot() -> hp_slot*
    {
        auto try_acquire = [](hp_slot* slot_ptr) -> bool
        {
            return !slot_ptr->in_use.load(std::memory_order_relaxed)
                && !slot_ptr->in_use.exchange(true, std::memory_order_acquire);
        };

        // access init slots
        for (auto slot_ptr : init_slots)
        {
            if (try_acquire(slot_ptr)) { return slot_ptr; }
        }

        // init_slots are all in use, we need to access slots that are added
        // after init, these slots are in the form of linked list.
        auto slot_ptr = init_slots.back();
        do {
            auto next = slot_ptr->next.load(std::memory_order_acquire);
            if (!next) break;
            slot_ptr = next;
            if (try_acquire(slot_ptr)) { return slot_ptr; }
        } while (true);

        // All current slots are in use, we need to allocate a new slot.
        auto tail = slot_ptr;
        slot_ptr  = ::new hp_slot{};
        slot_ptr->in_use.store(true, std::memory_order_relaxed);
        slot_ptr->init_slots = init_slots;

        hp_slot* next = nullptr;
        while (!tail->next.compare_exchange_weak(
            next, slot_ptr, std::memory_order_release, std::memory_order_relaxed
        ))
        {
            tail = next;
            next = nullptr;
        }

        return slot_ptr;
    }

private:
    // This is the initial list of slots with size of
    // std::thread::hardware_concurrency()
    std::vector<hp_slot*> init_slots;
};

// main interface for user
//
export template <typename T>
class hazard_pointer
{
    using hp_slot = hp_slot_list<T>::hp_slot;

public:
    static void
    retire(T* ptr)
    {
        local_slot.owned_slot->retire(ptr);
    }

    // default constructor will not acquire a hp_slot
    hazard_pointer() noexcept : m_slot{nullptr} {}

    hazard_pointer(hazard_pointer&& other) noexcept
        : m_slot{std::exchange(other.m_slot, nullptr)}
    {
    }

    auto
    operator=(hazard_pointer&& other) noexcept -> hazard_pointer&
    {
        if (this != &other)
        {
            if (!empty()) reset_protection();
            m_slot = std::exchange(other.m_slot, nullptr);
        }
        return *this;
    }

    ~hazard_pointer() noexcept
    {
        if (!empty()) reset_protection();
    }

    hazard_pointer(hazard_pointer const &) = delete;
    auto
    operator=(hazard_pointer const &) = delete;

    auto
    empty() const noexcept -> bool
    {
        return m_slot == nullptr;
    }

    void
    swap(hazard_pointer& rhs) noexcept
    {
        std::swap(m_slot, rhs.m_slot);
    }

    // Operations allowed only when non-empty
    auto
    protect(std::atomic<T*> const & src) noexcept -> T*
    {
        auto ptr = src.load(std::memory_order_relaxed);
        while (!try_protect(ptr, src));
        return ptr;
    }

    auto
    try_protect(T*& ptr, std::atomic<T*> const & src) noexcept -> bool
    {
        auto old = ptr;
        reset_protection(old);
        ptr = src.load(std::memory_order_acquire);
        return old == ptr;
    }

    void
    reset_protection(T const * ptr = nullptr) noexcept
    {
        m_slot->protected_ptr.store(ptr, std::memory_order_release);
    }

private:
    hp_slot* m_slot;

    template <typename U>
    friend auto
    make_hazard_pointer() -> hazard_pointer<U>;

    // Initialize on first use.
    inline static thread_local hp_slot::Owner const local_slot{};
};

export template <typename T>
auto
make_hazard_pointer() -> hazard_pointer<T>
{
    hazard_pointer<T> hp;
    hp.m_slot = hazard_pointer<T>::local_slot.owned_slot;
    return hp;
}

export template <typename T>
void
swap(hazard_pointer<T>& lhs, hazard_pointer<T>& rhs) noexcept
{
    return lhs.swap(rhs);
}


} // namespace tinystd