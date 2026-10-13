#include "scoped_timer.h"
#include "utils.h"

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

using u64 = uint64_t;
using u32 = uint32_t;

constexpr int vec_size = 10'000'000;
const auto n_hw_treads = std::thread::hardware_concurrency();


template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<typename Numeric>
std::vector<std::pair<u32, u32>> get_divided_vector_parts(const std::vector<Numeric>& vec) {
    std::vector<std::pair<u32, u32>> parts;
    auto one_nth = std::floor(vec.size() / n_hw_treads);
    auto curr = 0;
    while (curr <= vec.size()) {
        auto curr_range_end = curr + one_nth;
        if (curr_range_end > vec.size()) {
            --curr_range_end;
        }
        parts.push_back(std::pair<u32, u32>(curr, curr_range_end));
        if (curr == 0) {
            ++curr;
        }
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
std::vector<Numeric> create_vec(u32 n) {
    std::vector<Numeric> vec(n); // reserve n elements
    std::iota(vec.begin(), vec.end(), 1); // {1, 2, 3, ..., n}
    return vec;
}

// ==================================== SEQUENTIAL ==================================== 
template<typename Numeric>
[[nodiscard]] 
u64 sum_vec_seq(const std::vector<Numeric>& vec) {
    std::cout << "Using std::accumulate \n";
    return std::accumulate(
        vec.begin(), 
        vec.end(), 
        0
    );
}

template<typename Numeric>
[[nodiscard]] 
u64 sum_vec_seq_v2(const std::vector<Numeric>& vec) {
    std::cout << "Basic for loop \n";
    u64 sum = 0;
    for (const auto& elem : vec) {
        sum += elem;
    }
    return sum;
}

// ==================================== EXECUTION POLICIES ==================================== 
template<typename T, typename Numeric> requires std::is_execution_policy_v<T>
[[nodiscard]] 
u64 sum_vec_par(const std::vector<Numeric>& vec, const T& execution_policy) {
    std::cout << "Using execution policy: " << demangle<T>() << "\n";
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
[[nodiscard]] 
u64 sum_vec_part(
    const std::vector<Numeric>& vec,
    const u32 begin_range,
    const u32 end_range
) {
    u64 part_sum = {};
    for (int i = begin_range; i < end_range; i++) {
        std::lock_guard<std::mutex> m_lock_guard(myMutex);
        part_sum += vec[i];
    }
    return part_sum;
}


int main() {
    auto vec = create_vec<u32>(vec_size);

    {
        ScopedTimer t;
        auto res = sum_vec_seq(vec);
        std::cout << "Sum result:" << res << "\n";
    }

    {
        ScopedTimer t;
        auto res = sum_vec_seq_v2(vec);
        std::cout << "Sum result:" << res << "\n";
    }

    {
        ScopedTimer t;
        auto res = sum_vec_par(vec, std::execution::seq);
        std::cout << "Sum result:" << res << "\n";
    }

    {
        ScopedTimer t;
        auto res = sum_vec_par(vec, std::execution::par);
        std::cout << "Sum result:" << res << "\n";
    }

    {
        ScopedTimer t;
        auto res = sum_vec_par(vec, std::execution::par_unseq);
        std::cout << "Sum result:" << res << "\n";
    }

    {
        ScopedTimer t;
        auto res = sum_vec_with_lock(vec);
        std::cout << "Sum result:" << res << "\n";
    }

    {
        ScopedTimer t;
        auto res = sum_vec_with_atomic(vec);
        std::cout << "Sum result:" << res << "\n";
    }

    std::cout << "\nDividing in parts\n\n";

    auto vec_parts = get_divided_vector_parts(vec);
    std::cout << "Parts:\n";
    print_vec_parts(vec_parts);





    return 0;
}