#include <iostream>
#include <vector>
#include <thread>
#include <memory>
#include <mutex>
#include <cassert>

struct BankAccountThreadUnsafe {
    int balance{0};
    std::vector<int> transactions;

    // BUG: No locking! Two threads call this concurrently.
    void deposit(int amount) {
        balance += amount;              // Data race: lost updates
        transactions.push_back(amount); // Data race on heap buffer: UB / heap corruption!
    }
};

void unsafe_example() {
    auto account = std::make_shared<BankAccountThreadUnsafe>();

    std::thread t1([account]() {
        for (int i = 0; i < 10'000; ++i) {
            account->deposit(10);
        }
    });

    std::thread t2([account]() {
        for (int i = 0; i < 10'000; ++i) {
            account->deposit(20);
        }
    });

    t1.join();
    t2.join();

    std::cout << "--- C++ (WRONG / UNPROTECTED) RESULT ---\n";
    std::cout << "Expected balance : 300000\n";
    std::cout << "Actual balance   : " << account->balance << "\n";
    std::cout << "Total deposits   : " << account->transactions.size() << "\n";
    std::cout << "Note: Often crashes with double-free or outputs balance != 300000.\n\n";
}


class BankAccountThreadSafe {
public:
    explicit BankAccountThreadSafe(int initial_balance = 0) : balance(initial_balance) {}

    void deposit(int amount) {
        // Explicit lock required: developer must remember this step
        std::lock_guard<std::mutex> lock(mtx);
        balance += amount;
        transactions.push_back(amount);
    }

    int get_balance() {
        std::lock_guard<std::mutex> lock(mtx);
        return balance;
    }

    size_t get_transaction_count() {
        std::lock_guard<std::mutex> lock(mtx);
        return transactions.size();
    }

private:
    int balance{0};
    std::vector<int> transactions;
    std::mutex mtx; // Sits beside the data; not wrapped around it
};

void safe_example() {
    auto account = std::make_shared<BankAccountThreadSafe>(0);

    std::thread t1([account]() {
        for (int i = 0; i < 10'000; ++i) {
            account->deposit(10);
        }
    });

    std::thread t2([account]() {
        for (int i = 0; i < 10'000; ++i) {
            account->deposit(20);
        }
    });

    t1.join();
    t2.join();

    std::cout << "--- C++ (CORRECT / SYNCHRONIZED) RESULT ---\n";
    std::cout << "Expected balance : 300000\n";
    std::cout << "Actual balance   : " << account->get_balance() << "\n";
    std::cout << "Total deposits   : " << account->get_transaction_count() << "\n";

    assert(account->get_balance() == 300'000);
    assert(account->get_transaction_count() == 20'000);
    std::cout << "Status           : Synchronized successfully.\n";
}


int main() {
    unsafe_example();
    safe_example();
}