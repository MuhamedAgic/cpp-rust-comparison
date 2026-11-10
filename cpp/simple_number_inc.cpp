#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

void wrong_example() {
    int counter = 0;

    auto increment = [&counter]() {
        for (int i = 0; i < 100'000; ++i) {
            // BUG: Multiple threads mutating without atomics or locks.
            // Undefined Behavior in C++ (Data Race).
            counter++; 
        }
    };

    std::thread t1(increment);
    std::thread t2(increment);

    t1.join();
    t2.join();

    // Result is unpredictable (rarely 200,000)
    std::cout << "Counter: " << counter << std::endl;
}

void valid_example() {
    int counter = 0;
    std::mutex m;

    auto increment = [&counter, &m]() {
        for (int i = 0; i < 100'000; ++i) {
            std::lock_guard<std::mutex> lg(m);
            counter++; 
        } // lg out of scope, m released
    };

    std::thread t1(increment);
    std::thread t2(increment);

    t1.join();
    t2.join();

    std::cout << "Counter: " << counter << std::endl;
}

void valid_example_atomics() {
    std::atomic<int> counter = 0;

    auto increment = [&counter]() {
        for (int i = 0; i < 100'000; ++i) {
            counter.fetch_add(1, std::memory_order_relaxed);
        } 
    };

    std::thread t1(increment);
    std::thread t2(increment);

    t1.join();
    t2.join();

    std::cout << "Counter: " << counter << std::endl;
}

int main() {
    wrong_example();
    valid_example();
    valid_example_atomics();
}