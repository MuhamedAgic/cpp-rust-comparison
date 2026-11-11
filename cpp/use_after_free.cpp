#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>

std::mutex cout_mutex;

void start_worker_unsafe() {
    int local_data = 42;

    for (int i = 0; i < 5; i++) {
        // Pass a reference to a stack variable to a thread
        std::thread t([&local_data]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            // BUG: local_data is gone! Reads garbage or crashes.
            std::lock_guard<std::mutex> lg(cout_mutex);
            std::cout << "Data (at " << &local_data << "): " << local_data << std::endl;
        });
        t.detach(); 
        // `local_data` destroyed here when function returns
    }
}

thread_local int local_data = 42;
// thread_local std::string local_data = "hello";
void start_worker_safe_thread_local() {
    for (int i = 0; i < 5; i++) {
        // the thread receive its own independent copy
        std::thread t([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::lock_guard<std::mutex> lg(cout_mutex);
            std::cout << "Data (at " << &local_data << "): " << local_data << std::endl; // Safe: points to thread's own copy
        });
        t.detach();
    }
}

void vector_reallocation_unsafe() {
    std::vector<int> readings = {10, 20, 30};
    
    // Pass pointer to the first element to a worker thread
    int* first_reading = &readings[0];

    std::thread reader([first_reading]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        // USE-AFTER-FREE: Vector reallocated and freed the old array!
        std::cout << "Reading: " << *first_reading << '\n';
    });

    // Main thread adds elements -> triggers reallocation -> frees old buffer
    for (int i = 0; i < 100; ++i) {
        readings.push_back(i);
    }

    reader.join();
}

void vector_reallocation_safe() {
    std::vector<int> readings = {10, 20, 30};
    
    // Pass copy of the first element to a worker thread
    int first_reading = readings[0];

    std::thread reader([first_reading]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "Reading: " << first_reading << '\n';
    });

    // Main thread adds elements -> triggers reallocation -> frees old buffer
    for (int i = 0; i < 100; ++i) {
        readings.push_back(i);
    }

    reader.join();
}

int main() {
    start_worker_unsafe();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "------------------------------\n";
    start_worker_safe_thread_local();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "------------------------------\n";
    vector_reallocation_unsafe();
    std::cout << "------------------------------\n";
    vector_reallocation_safe();
}