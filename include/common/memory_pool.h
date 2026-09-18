#pragma once

#include <cstddef>
#include <vector>
#include <utility>
#include <new>

namespace itch {

template <typename T, size_t BlockSize = 4096>
class MemoryPool {
public:
    MemoryPool() : free_list_(nullptr) {
        allocate_block();
    }

    ~MemoryPool() {
        for (void* block : blocks_) {
            ::operator delete(block);
        }
    }

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    template <typename... Args>
    T* allocate(Args&&... args) {
        if (!free_list_) {
            allocate_block();
        }

        Node* node = free_list_;
        free_list_ = free_list_->next;

        T* obj = reinterpret_cast<T*>(node);
        new (obj) T(std::forward<Args>(args)...);
        return obj;
    }

    void deallocate(T* obj) {
        if (!obj) return;

        obj->~T();
        Node* node = reinterpret_cast<Node*>(obj);
        node->next = free_list_;
        free_list_ = node;
    }

private:
    union Node {
        Node* next;
        alignas(alignof(T)) char storage[sizeof(T)];
    };

    std::vector<void*> blocks_;
    Node* free_list_;

    void allocate_block() {
        size_t bytes = sizeof(Node) * BlockSize;
        void* block = ::operator new(bytes);
        blocks_.push_back(block);

        Node* nodes = static_cast<Node*>(block);
        for (size_t i = 0; i < BlockSize - 1; ++i) {
            nodes[i].next = &nodes[i + 1];
        }
        nodes[BlockSize - 1].next = free_list_;
        free_list_ = nodes;
    }
};

}
