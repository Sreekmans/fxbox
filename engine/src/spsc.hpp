// Lock-free single-producer / single-consumer ring buffer.
// Producer: control thread. Consumer: realtime audio thread.
// No allocation after construction; push/pop are wait-free.
#pragma once
#include <atomic>
#include <array>
#include <cstddef>

namespace fxbox {

template <typename T, size_t N>
class SpscQueue {
    static_assert((N & (N - 1)) == 0, "N must be a power of two");
public:
    bool push(const T& v) {
        const size_t w = write_.load(std::memory_order_relaxed);
        const size_t r = read_.load(std::memory_order_acquire);
        if (w - r >= N) return false;           // full
        buf_[w & (N - 1)] = v;
        write_.store(w + 1, std::memory_order_release);
        return true;
    }
    bool pop(T& out) {
        const size_t r = read_.load(std::memory_order_relaxed);
        const size_t w = write_.load(std::memory_order_acquire);
        if (r == w) return false;               // empty
        out = buf_[r & (N - 1)];
        read_.store(r + 1, std::memory_order_release);
        return true;
    }
private:
    std::array<T, N> buf_{};
    alignas(64) std::atomic<size_t> write_{0};
    alignas(64) std::atomic<size_t> read_{0};
};

} // namespace fxbox
