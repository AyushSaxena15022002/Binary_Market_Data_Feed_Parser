#pragma once

#include <atomic>
#include <cstddef>
#include <new>
#include <utility>

namespace itch {

// Lock-free Single Producer Single Consumer circular queue.
// Capacity MUST be a power of 2.
// Producer owns head_, consumer owns tail_.
template <typename T, size_t Capacity>
class SPSCQueue {
public:
    SPSCQueue() : head_(0), tail_(0) {
        static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    }

    ~SPSCQueue() {
        // Destroy any remaining unconsumed objects
        size_t head = head_.load(std::memory_order_relaxed);
        size_t tail = tail_.load(std::memory_order_relaxed);
        while (tail != head) {
            T* obj = reinterpret_cast<T*>(&buffer_[tail & (Capacity - 1)]);
            obj->~T();
            tail++;
        }
    }

    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    // Enqueue by constructing in-place. Returns false if full.
    template <typename... Args>
    bool try_enqueue(Args&&... args) {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next_head = head + 1;

        // Queue is full when next_head - tail == Capacity (all slots occupied)
        if (next_head - tail_.load(std::memory_order_acquire) >= Capacity) {
            return false;
        }

        T* obj = reinterpret_cast<T*>(&buffer_[head & (Capacity - 1)]);
        new (obj) T(std::forward<Args>(args)...);
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    // Dequeue into item. Returns false if empty.
    bool try_dequeue(T& item) {
        size_t tail = tail_.load(std::memory_order_relaxed);

        if (tail == head_.load(std::memory_order_acquire)) {
            return false;
        }

        T* obj = reinterpret_cast<T*>(&buffer_[tail & (Capacity - 1)]);
        item = std::move(*obj);
        obj->~T();
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    bool empty() const {
        return tail_.load(std::memory_order_acquire) == head_.load(std::memory_order_acquire);
    }

    size_t size() const {
        return head_.load(std::memory_order_acquire) - tail_.load(std::memory_order_acquire);
    }

private:
    // Separate cache lines to avoid false sharing between producer and consumer
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;

    struct alignas(alignof(T)) Slot {
        char storage[sizeof(T)];
    };

    Slot buffer_[Capacity];
};

}
