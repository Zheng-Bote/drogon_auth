/**
 * SPDX-FileComment: Lock-free Ring Buffer for Auth Events
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <atomic>
#include <array>
#include <optional>

namespace drogon_auth::utils {

template <typename T, size_t Size>
class LockFreeRingBuffer {
public:
    LockFreeRingBuffer() : head_(0), tail_(0) {}

    bool push(const T& item) {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % Size;
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            // Buffer is full (overwrite could be implemented, but here we just return false or overwrite)
            // Let's implement overwrite for a logging ring buffer
            head_.store((head_.load(std::memory_order_relaxed) + 1) % Size, std::memory_order_release);
        }
        
        buffer_[current_tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() {
        size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return std::nullopt; // Empty
        }
        
        T item = buffer_[current_head];
        head_.store((current_head + 1) % Size, std::memory_order_release);
        return item;
    }
    
    // Non-consuming read of all current items (useful for a snapshot)
    std::vector<T> snapshot() const {
        std::vector<T> result;
        size_t current_head = head_.load(std::memory_order_acquire);
        size_t current_tail = tail_.load(std::memory_order_acquire);
        
        size_t count = (current_tail >= current_head) ? (current_tail - current_head) : (Size - current_head + current_tail);
        result.reserve(count);
        
        size_t idx = current_head;
        while (idx != current_tail) {
            result.push_back(buffer_[idx]);
            idx = (idx + 1) % Size;
        }
        return result;
    }

private:
    std::array<T, Size> buffer_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
};

} // namespace drogon_auth::utils
