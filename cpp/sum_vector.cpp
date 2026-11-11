#include "utils/scoped_timer.h"
#include "utils/utils.h"

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
#include <numeric>
#include <execution>
#include <mutex>
#include <cstdint>
#include <concepts>
#include <cmath>
#include <utility>
#include <random>
#include <future>

using u64 = uint64_t;
using u32 = uint32_t;

constexpr int vec_size = 100'000'000;
const auto n_hw_treads = std::thread::hardware_concurrency();


template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<typename Numeric>
std::vector<std::pair<u32, u32>> get_divided_vector_parts(const std::vector<Numeric>& vec) {
    std::vector<std::pair<u32, u32>> parts;
    auto one_nth = std::floor(vec.size() / n_hw_treads);
    auto curr = 0;
    while (curr < vec.size()) {
        auto curr_range_end = curr + one_nth;
        if (curr_range_end > vec.size()) {
            --curr_range_end;
        }
        parts.push_back(std::pair<u32, u32>(curr, curr_range_end));
        curr += one_nth;
    }
    return parts;
}

template<typename Numeric>
void print_vec_parts(const std::vector<std::pair<Numeric, Numeric>>& vec) {
    for (const auto& elem : vec) {
        std::cout << "{" << elem.first << ", " << elem.second << "}\n";
    }
}

// ==================================== FIXTURES ==================================== 


    
template<typename Numeric>
[[nodiscard]] 
std::vector<Numeric> create_vec() {
    std::vector<Numeric> vec;
    vec.reserve(vec_size);

    std::mt19937 engine;
    std::uniform_int_distribution<> uniform_dist(1,10);
    for (long long i = 0; i < vec_size; ++i) {
        vec.emplace_back(uniform_dist(engine));
    }

    return vec;
}

// ==================================== SEQUENTIAL ==================================== 
template<typename Numeric>
[[nodiscard]] 
u64 sum_vec_seq(const std::vector<Numeric>& vec) {
    return std::accumulate(
        vec.begin(), 
        vec.end(), 
        0
    );
}

template<typename Numeric>
[[nodiscard]] 
u64 sum_vec_seq_v2(const std::vector<Numeric>& vec) {
    u64 sum = 0;
    for (const auto& elem : vec) {
        sum += elem;
    }
    return sum;
}

// ==================================== EXECUTION POLICIES ==================================== 
template<typename T, typename Numeric> requires std::is_execution_policy_v<T>
[[nodiscard]] 
u64 sum_vec(const std::vector<Numeric>& vec, const T& execution_policy) {
    return std::reduce(
        execution_policy,
        vec.begin(), 
        vec.end(), 
        0
    );
}

// ==================================== LOCK PROTECTION (SINGLE THREADED) ==================================== 
template<typename Numeric>
[[nodiscard]] 
u64 sum_vec_with_lock(const std::vector<Numeric>& vec) {
    std::mutex myMutex;
    u64 sum = {};
    for (auto i: vec) {
        std::lock_guard<std::mutex> myLockGuard(myMutex);
        sum += i;
    }
    return sum;
}

// ==================================== ATOMIC PROTECTION (SINGLE THREADED) ==================================== 
template<typename Numeric>
[[nodiscard]] 
u64 sum_vec_with_atomic(const std::vector<Numeric>& vec) {
    std::atomic<u64> sum {0};
    for (auto i: vec) {
        sum += i;
    }
    return sum;
}

// ==================================== LOCK PROTECTION (MULTI THREADED) ==================================== 
std::mutex myMutex;

template<typename Numeric> 
void sum_vec_part_with_lock(
    u64& sum,
    const std::vector<Numeric>& vec,
    const u32 begin_range,
    const u32 end_range
) {
    u64 part_sum = {};
    for (int i = begin_range; i < end_range; i++) {
        part_sum += vec[i];
    }
    std::lock_guard<std::mutex> m_lock_guard(myMutex);
    sum += part_sum;
}

template<typename Numeric>
[[nodiscard]] 
u64 sum_vec_multi_threaded_with_lock(
    const std::vector<Numeric>& vec,
    std::vector<std::pair<u32, u32>>& parts
) {
    u64 sum = 0;
    std::vector<std::thread> worker_threads;
    worker_threads.reserve(n_hw_treads);

    for (int i = 0; i < n_hw_treads; i++) {
        std::thread worker = std::thread(sum_vec_part_with_lock<Numeric>, std::ref(sum), std::cref(vec), parts[i].first, parts[i].second);
        worker_threads.emplace_back(std::move(worker));
    }
    for (auto& worker : worker_threads) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    return sum;
}

// ==================================== ATOMIC PROTECTION (MULTI THREADED) ==================================== 

// 1. NTTP Concept: Constrain a value 'Order' to be std::memory_order
template<std::memory_order Order>
concept ValidMemoryOrder = true;

template<typename Numeric, std::memory_order MemoryOrder>
void sum_vec_part_with_atomic(
    std::atomic<u64>& sum,
    const std::vector<Numeric>& vec,
    const u32 begin_range,
    const u32 end_range
) {
    u64 part_sum = {};
    for (int i = begin_range; i < end_range; i++) {
        part_sum += vec[i];
    }
    sum.fetch_add(part_sum, MemoryOrder);
}

template<std::memory_order MemoryOrder, typename Numeric>
[[nodiscard]] 
u64 sum_vec_multi_threaded_with_atomics(
    const std::vector<Numeric>& vec,
    std::vector<std::pair<u32, u32>>& parts
) {
    std::atomic<u64> sum = 0;
    std::vector<std::thread> worker_threads;
    worker_threads.reserve(n_hw_treads);

    for (int i = 0; i < n_hw_treads; i++) {
        std::thread worker = std::thread(
            sum_vec_part_with_atomic<Numeric, MemoryOrder>, 
            std::ref(sum), 
            std::cref(vec), 
            parts[i].first, 
            parts[i].second        
        );
        worker_threads.emplace_back(std::move(worker));
    }
    for (auto& worker : worker_threads) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    return sum.load(MemoryOrder);
}

// ==================================== USING STD ASYNC ==================================== 
template<typename Numeric>
[[nodiscard]] 
u64 sum_vec_part_async(
    const std::vector<Numeric>& vec,
    const u32 begin_range,
    const u32 end_range
) {
    u64 part_sum = 0;
    for (u32 i = begin_range; i < end_range; ++i) {
        part_sum += vec[i];
    }
    return part_sum;
}

template<typename Numeric>
[[nodiscard]] 
u64 sum_vec_async(
    const std::vector<Numeric>& vec,
    const std::vector<std::pair<u32, u32>>& parts
) {
    std::vector<std::future<u64>> futures;
    futures.reserve(n_hw_treads);

    // Launch async task on each hardware thread
    for (u32 i = 0; i < n_hw_treads; ++i) {
        futures.push_back(
            std::async(
                std::launch::async, 
                sum_vec_part_async<Numeric>, 
                std::cref(vec), 
                parts[i].first, 
                parts[i].second
            )
        );
    }

    // Accumulate results from all futures (get() blocks until the task completes)
    u64 total_sum = 0;
    for (auto& fut : futures) {
        total_sum += fut.get();
    }

    return total_sum;
}


// ==================================== STD::LATCH SYNCHRONIZATION ==================================== 

#include <latch>
template<typename Numeric>
[[nodiscard]] 
u64 sum_vec_latch(
    const std::vector<Numeric>& vec,
    const std::vector<std::pair<u32, u32>>& parts
) {
    std::vector<u64> partial_sums(n_hw_treads, 0);
    std::latch work_done(n_hw_treads);

    for (u32 i = 0; i < n_hw_treads; ++i) {
        std::thread([&, i]() {
            u64 local = 0;
            for (u32 idx = parts[i].first; idx < parts[i].second; ++idx) {
                local += vec[idx];
            }
            partial_sums[i] = local;
            work_done.count_down(); // Decrement latch count
        }).detach(); // Detach thread safely since latch governs completion
    }

    work_done.wait(); // Main thread waits until latch reaches 0
    return std::accumulate(partial_sums.begin(), partial_sums.end(), 0ULL);
}


// ==================================== MAIN ==================================== 
int main() {
    auto vec = create_vec<u32>();

    {
        ScopedTimer t;
        std::cout << "\nSequential std::accumulate\n";
        auto res = sum_vec_seq(vec);
        std::cout << "Sum result: " << res << "\n";
    }

    {
        ScopedTimer t;
        std::cout << "\nSequential for loop\n";
        auto res = sum_vec_seq_v2(vec);
        std::cout << "Sum result: " << res << "\n";
    }

    {
        ScopedTimer t;
        std::cout << "\nUsing execution policy: std::execution::seq\n";
        auto res = sum_vec(vec, std::execution::seq);
        std::cout << "Sum result: " << res << "\n";
    }

    {
        ScopedTimer t;
        std::cout << "\nUsing execution policy: std::execution::par\n";
        auto res = sum_vec(vec, std::execution::par);
        std::cout << "Sum result: " << res << "\n";
    }

    {
        ScopedTimer t;
        std::cout << "\nUsing execution policy: std::execution::par_unseq\n";
        auto res = sum_vec(vec, std::execution::par_unseq);
        std::cout << "Sum result: " << res << "\n";
    }

    {
        ScopedTimer t;
        std::cout << "\nSingle threaded with lock protection\n";
        auto res = sum_vec_with_lock(vec);
        std::cout << "Sum result: " << res << "\n";
    }

    {
        ScopedTimer t;
        std::cout << "\nSingle threaded with atomic protection\n";
        auto res = sum_vec_with_atomic(vec);
        std::cout << "Sum result: " << res << "\n";
    }

    std::cout << "\nDividing in parts\n\n";

    auto vec_parts = get_divided_vector_parts(vec);
    std::cout << "Parts:\n";
    print_vec_parts(vec_parts);

    {
        ScopedTimer t;
        std::cout << "\n\nMulti threaded with lock protection\n";
        auto res = sum_vec_multi_threaded_with_lock(vec, vec_parts);
        std::cout << "Sum result: " << res << "\n";
    }

    {
        ScopedTimer t;
        std::cout << "\n\nMulti threaded with atomic protection (sequential consistency)\n";
        auto res = sum_vec_multi_threaded_with_atomics<std::memory_order_seq_cst>(vec, vec_parts);
        std::cout << "Sum result: " << res << "\n";
    }

    {
        ScopedTimer t;
        std::cout << "\n\nMulti threaded with atomic protection (acquire)\n";
        auto res = sum_vec_multi_threaded_with_atomics<std::memory_order_acquire>(vec, vec_parts);
        std::cout << "Sum result: " << res << "\n";
    }

    {
        ScopedTimer t;
        std::cout << "\n\nMulti threaded with atomic protection (consume)\n";
        auto res = sum_vec_multi_threaded_with_atomics<std::memory_order_consume>(vec, vec_parts);
        std::cout << "Sum result: " << res << "\n";
    }

    {
        ScopedTimer t;
        std::cout << "\n\nMulti threaded with atomic protection (relaxed semantic)\n";
        auto res = sum_vec_multi_threaded_with_atomics<std::memory_order_relaxed>(vec, vec_parts);
        std::cout << "Sum result: " << res << "\n";
    }

    {
        ScopedTimer t;
        std::cout << "\n\nUsing std::async and all possible hardware threads\n";
        auto res = sum_vec_async(vec, vec_parts);
        std::cout << "Sum result: " << res << "\n";
    }

    {
        ScopedTimer t;
        std::cout << "\n\nLatch\n";
        auto res = sum_vec_latch(vec, vec_parts);
        std::cout << "Sum result: " << res << "\n";
    }

    return 0;
}