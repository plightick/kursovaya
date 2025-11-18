#pragma once

#include <string>
#include <random>
#include <sstream>
#include <iomanip>

namespace utils {

inline std::string generateNumericId(std::size_t digits) {
    thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, 9);
    std::string out;
    out.reserve(digits);
    for (std::size_t i = 0; i < digits; ++i) {
        out.push_back(static_cast<char>('0' + dist(rng)));
    }
    return out;
}

inline std::string trim(const std::string &s) {
    std::size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    std::size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

inline std::string weakHash(const std::string &input) {
    std::hash<std::string> hasher;
    auto h = hasher(input);
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << h;
    return ss.str();
}

}


