#pragma once

#include <chrono>

class Stopwatch {
private:
        std::chrono::high_resolution_clock::time_point start_;
        int64_t elapsed_;
public:
        Stopwatch() {
                reset();
        }
        void start() {
                start_ = std::chrono::high_resolution_clock::now();
        }
        void stop() {
                auto s = std::chrono::high_resolution_clock::now();
                std::chrono::high_resolution_clock::duration d = (s - start_);
                elapsed_ += std::chrono::nanoseconds(d).count();
        }
        int64_t elapsed_ns() {
                return elapsed_;
        }
        int64_t elapsed_us() {
                return elapsed_/1000;
        }
        int64_t elapsed_ms() {
                return elapsed_/1000000;
        }
        void reset() {
                elapsed_ = 0;
        }
};
