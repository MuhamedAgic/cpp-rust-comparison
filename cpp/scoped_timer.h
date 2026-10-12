#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string_view>

class ScopedTimer {
public:
    using clock = std::chrono::steady_clock;

    explicit ScopedTimer(std::string_view label = "Elapsed")
        : label_(label), start_(clock::now()) {}

    ~ScopedTimer() noexcept {
        const auto elapsed = clock::now() - start_;

        try {
            std::clog << '[' << label_ << "] "
                      << format(elapsed) << '\n';
        } catch (...) {
            // Destructors should not throw.
        }
    }

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    ScopedTimer(ScopedTimer&&) = delete;
    ScopedTimer& operator=(ScopedTimer&&) = delete;

private:
    template <typename Duration>
    static std::ostream& print_value(
        std::ostream& out,
        Duration duration,
        std::string_view unit,
        int precision = 2)
    {
        return out << std::fixed << std::setprecision(precision)
                   << duration.count() << ' ' << unit;
    }

    static std::string format(clock::duration elapsed) {
        using namespace std::chrono;

        const auto ns = duration_cast<nanoseconds>(elapsed);

        std::ostringstream out;

        if (ns < microseconds(1)) {
            print_value(out, duration<double, std::nano>(ns),
                        "nanoseconds", 0);
        } else if (ns < milliseconds(1)) {
            print_value(out, duration<double, std::micro>(ns),
                        "microseconds");
        } else if (ns < seconds(1)) {
            print_value(out, duration<double, std::milli>(ns),
                        "milliseconds");
        } else if (ns < minutes(1)) {
            print_value(out, duration<double>(ns),
                        "seconds");
        } else if (ns < hours(1)) {
            print_value(out, duration<double, std::ratio<60>>(ns),
                        "minutes");
        } else {
            print_value(out, duration<double, std::ratio<3600>>(ns),
                        "hours");
        }

        return out.str();
    }

    std::string_view label_;
    clock::time_point start_;
};
