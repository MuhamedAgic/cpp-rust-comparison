use std::thread;
use std::time::Duration;

#[cfg(feature = "show_broken")]
fn start_worker_broken() {
    let local_data = 42;

    // Trying to reference a local variable in a background thread
    thread::spawn(|| {
        thread::sleep(Duration::from_millis(50));
        println!("Data: {}", local_data);
    });
}


fn start_worker_valid() {
    let local_data = 42;

    // Scoped threads: the compiler verifies all threads terminate before exiting scope
    thread::scope(|s| {
        s.spawn(|| {
            // Perfectly safe to borrow local stack data here!
            println!("Data: {}", local_data);
        });
    }); // Automatically waits for spawned threads here
}

#[cfg(feature = "show_broken")]
fn vector_reallocation() {
    let mut readings = vec![10, 20, 30];

    // Borrow a reference to the first element
    let first_reading = &readings[0];

    // Attempt 1: Pass the reference to a spawned thread
    let reader = thread::spawn(move || {
        thread::sleep(Duration::from_millis(50));
        println!("Reading: {}", *first_reading);
    });

    // Attempt 2: Mutate the vector while `first_reading` is still borrowed
    for i in 0..100 {
        readings.push(i);
    }

    reader.join().unwrap();
}

fn vector_safe_copy() {
    let mut readings = vec![10, 20, 30];

    // Copy the integer value (not a pointer into the Vec's heap buffer)
    let first_reading = readings[0];

    let reader = thread::spawn(move || {
        thread::sleep(Duration::from_millis(50));
        println!("Reading: {}", first_reading); // Safe: owns its own i32
    });

    // Safe to reallocate `readings` now; no pointers into its buffer exist
    for i in 0..100 {
        readings.push(i);
    }

    reader.join().unwrap();
}

fn main() {
    start_worker_valid();
    vector_safe_copy();
}