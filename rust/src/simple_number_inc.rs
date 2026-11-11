use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;
use std::thread;

#[cfg(feature = "show_broken")]
fn broken() {
    let mut counter = 0;

    // We try to borrow `counter` mutably in two threads
    let handle1 = thread::spawn(|| {
        for _ in 0..100_000 {
            counter += 1;
        }
    });

    let handle2 = thread::spawn(|| {
        for _ in 0..100_000 {
            counter += 1;
        }
    });

    handle1.join().unwrap();
    handle2.join().unwrap();
}

fn valid() {
    let counter = Arc::new(AtomicUsize::new(0));

    let c1 = Arc::clone(&counter);
    let t1 = thread::spawn(move || {
        for _ in 0..100_000 {
            c1.fetch_add(1, Ordering::Relaxed);
        }
    });

    let c2 = Arc::clone(&counter);
    let t2 = thread::spawn(move || {
        for _ in 0..100_000 {
            c2.fetch_add(1, Ordering::Relaxed);
        }
    });

    t1.join().unwrap();
    t2.join().unwrap();

    println!("Counter: {}", counter.load(Ordering::SeqCst)); // Exactly 200,000
}

fn main() {
    valid();
}