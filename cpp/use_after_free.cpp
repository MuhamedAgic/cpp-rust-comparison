#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

void start_worker_unsafe() {
    int local_data = 42;

    // Pass a reference to a stack variable to a thread
    std::thread t([&local_data]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        // BUG: local_data is gone! Reads garbage or crashes.
        std::cout << "Data: " << local_data << std::endl; 
    });

    t.detach(); 
    // `local_data` destroyed here when function returns
}

void start_worker_safe_thread_local() {
    std::vector<std::thread> threads;
    thread_local int local_data = 42;

    for (int i = 0; i < 5; i++) {
        // the thread receive its own independent copy
        std::thread t([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::cout << "Data (at " << &local_data << "): " << local_data << std::endl; // Safe: points to thread's own copy
        });
        threads.push_back(std::move(t));
        // `local_data` goes out of scope here, but the thread has its own copy
    }
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

int main() {
    start_worker_unsafe();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    start_worker_safe_thread_local();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

}