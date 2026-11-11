use std::sync::{Arc, Mutex};
use std::thread;

struct BankAccount {
    balance: i32,
    transactions: Vec<i32>,
}

impl BankAccount {
    fn new(initial_balance: i32) -> Self {
        Self {
            balance: initial_balance,
            transactions: Vec::new(),
        }
    }

    fn deposit(&mut self, amount: i32) {
        self.balance += amount;
        self.transactions.push(amount);
    }
}

#[cfg(feature = "show_broken")]
fn deposit_wrong_no_mutex() {
    // Shared ownership across threads, but NO Mutex
    let account = Arc::new(BankAccount::new(0));

    let acc1 = Arc::clone(&account);
    let handle1 = thread::spawn(move || {
        for _ in 0..10_000 {
            acc1.deposit(10); // COMPILER ERROR!
        }
    });

    let acc2 = Arc::clone(&account);
    let handle2 = thread::spawn(move || {
        for _ in 0..10_000 {
            acc2.deposit(20); // COMPILER ERROR!
        }
    });

    handle1.join().unwrap();
    handle2.join().unwrap();
}

#[cfg(feature = "show_broken")]
fn deposit_wrong_borrow() {
    let mut account = BankAccount::new(0);

    let handle1 = thread::spawn(|| {
        for _ in 0..10_000 {
            account.deposit(10); // COMPILER ERROR 1: May outlive function + mutable borrow
        }
    });

    let handle2 = thread::spawn(|| {
        for _ in 0..10_000 {
            account.deposit(20); // COMPILER ERROR 2: Second mutable borrow!
        }
    });

    handle1.join().unwrap();
    handle2.join().unwrap();
}


fn deposit_example() {
    // 1. Arc provides shared ownership across threads; Mutex guards internal data
    let account = Arc::new(Mutex::new(BankAccount::new(0)));

    // 2. Clone Arc reference for Thread 1
    let acc1 = Arc::clone(&account);
    let handle1 = thread::spawn(move || {
        for _ in 0..10_000 {
            // lock() yields a MutexGuard. In Rust, you CANNOT touch BankAccount
            // without holding this guard.
            let mut acc = acc1.lock().unwrap();
            acc.deposit(10);
        } // `acc` drops here, unlocking automatically via RAII
    });

    // 3. Clone Arc reference for Thread 2
    let acc2 = Arc::clone(&account);
    let handle2 = thread::spawn(move || {
        for _ in 0..10_000 {
            let mut acc = acc2.lock().unwrap();
            acc.deposit(20);
        }
    });

    // 4. Wait for both threads to finish
    handle1.join().unwrap();
    handle2.join().unwrap();

    // 5. Inspect final result
    let final_acc = account.lock().unwrap();
    println!("--- RUST RESULT ---");
    println!("Expected balance : {}", 10_000 * 10 + 10_000 * 20); // 300,000
    println!("Actual balance   : {}", final_acc.balance);
    println!("Total deposits   : {}", final_acc.transactions.len());

    assert_eq!(final_acc.balance, 300_000);
    assert_eq!(final_acc.transactions.len(), 20_000);
    println!("Status           : Guaranteed correct at compile time!\n");
}

fn main() {
    deposit_example();
}