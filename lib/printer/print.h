#include <Arduino.h>

void println(){}

// --- Base cases ---

// Generic type: uses normal Serial.println
template <typename T>
void println(const T &value) {
    Serial.println(value);
}

// float specialization
template <>
void println<float>(const float &value) {
    Serial.println(value, 6);   // 6 decimal places
}

// double specialization
template <>
void println<double>(const double &value) {
    Serial.println(value, 10);  // 10 decimal places
}

template <typename T, typename... Args>
void println(const T &first, const Args&... rest) {
    // Generic type
    Serial.print(first);
    Serial.print(' ');
    println(rest...);
}

template <typename... Args>
void println(const float &first, const Args&... rest) {
    Serial.print(first, 6);
    Serial.print(' ');
    println(rest...);
}

template <typename... Args>
void println(const double &first, const Args&... rest) {
    Serial.print(first, 10);
    Serial.print(' ');
    println(rest...);
}

