use std::thread;
use std::time::Duration;

// fn start_worker_broken() {
//     let local_data = 42;

//     // Trying to reference a local variable in a background thread
//     thread::spawn(|| {
//         thread::sleep(Duration::from_millis(50));
//         println!("Data: {}", local_data);
//     });
// }


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

fn main() {
    start_worker_valid();
}