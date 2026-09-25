#pragma once
// This file provides compatibility with the original Arduino code the library is based on.

#include <time.h>
#include <stdint.h>
#include <random>
#include <iostream>

class SerialOut {
public:
    void print(const std::string &s) {
        std::cout << s;
    }
    void print(long int i) {
        std::cout << i;
    }
    void println(const std::string &s) {
        std::cout << s << std::endl;
    }
    void println(long int i) {
        std::cout << i << std::endl;
    }
    //void print(double d) {
    //    std::cout << d << std::endl;
    //}
};
inline SerialOut Serial;

inline uint64_t millis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
}

inline static std::mt19937 &rng() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

inline long random(long max) {
    if (max <= 0) {
        return 0;
    }

    std::uniform_int_distribution<long> dist(0, max - 1);
    return dist(rng());
}

inline long random(long min, long max) {
    if (min >= max) {
        return min;
    }

    std::uniform_int_distribution<long> dist(min, max - 1);
    return dist(rng());
}
