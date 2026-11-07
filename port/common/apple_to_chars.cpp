// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the port takes from Apple's libc++ that macOS before 13.3 lacks: std::to_chars for float,
// double and long double, which std::format of a floating-point value calls (the log, the errors,
// every formatted number), std::__hash_memory (std::hash of a string or a number) and
// std::__libcpp_verbose_abort (a failed library check). The build turns off libc++'s availability
// markup (_LIBCPP_DISABLE_AVAILABILITY, CMakeLists.txt), which refuses only the first, and this file
// stands in for the library's: found in the program, they are used on newer systems too, so the
// output is the same everywhere. A libc++ that needs more shows it on macOS 12 as "dyld: missing
// symbol called" (tools/remote-mac.sh runs a build there).
//
// The shortest form is the fewest digits that read back as the same value, found with snprintf
// and strtod.

#include <charconv>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>

namespace {

template <class T>
T read_back(const char* text) {
    if constexpr (sizeof(T) == sizeof(float)) {
        return std::strtof(text, nullptr);
    } else {
        return std::strtod(text, nullptr);
    }
}

std::to_chars_result put(char* first, char* last, const std::string& text) {
    if (static_cast<size_t>(last - first) < text.size()) {
        return {last, std::errc::value_too_large};
    }
    std::memcpy(first, text.data(), text.size());
    return {first + text.size(), std::errc{}};
}

// Not a finite number: spelled as to_chars does.
bool non_finite(double value, std::string& out) {
    if (std::isnan(value)) {
        out = std::signbit(value) ? "-nan" : "nan";
    } else if (std::isinf(value)) {
        out = std::signbit(value) ? "-inf" : "inf";
    } else {
        return false;
    }
    return true;
}

// printf's %a without the "0x" that to_chars leaves out.
std::string hex_text(const char* printed) {
    const bool negative = printed[0] == '-';
    return std::string(negative ? "-" : "") + (printed + (negative ? 3 : 2));
}

// The fewest significant digits (no point) and the decimal exponent of the first, of |value|.
template <class T>
void shortest(T value, std::string& digits, int& exponent) {
    const int max_digits = sizeof(T) == sizeof(float) ? 9 : 17;
    char text[40];
    for (int count = 1; count <= max_digits; ++count) {
        std::snprintf(text, sizeof text, "%.*e", count - 1, static_cast<double>(std::fabs(value)));
        if (read_back<T>(text) == std::fabs(value)) {
            break;
        }
    }
    digits.clear();
    const char* p = text;
    for (; *p && *p != 'e'; ++p) {
        if (*p != '.') {
            digits += *p;
        }
    }
    exponent = std::atoi(p + 1);
}

std::string scientific(const std::string& digits, int exponent) {
    std::string out = digits.substr(0, 1);
    if (digits.size() > 1) {
        out += "." + digits.substr(1);
    }
    char tail[16];
    std::snprintf(tail, sizeof tail, "e%c%02d", exponent < 0 ? '-' : '+', std::abs(exponent));
    return out + tail;
}

std::string fixed(const std::string& digits, int exponent) {
    const size_t count = digits.size();
    if (exponent < 0) {
        return "0." + std::string(static_cast<size_t>(-exponent - 1), '0') + digits;
    }
    const size_t whole = static_cast<size_t>(exponent) + 1;
    if (whole >= count) {
        return digits + std::string(whole - count, '0');
    }
    return digits.substr(0, whole) + "." + digits.substr(whole);
}

// format null: the shorter of fixed and scientific, fixed on a tie.
template <class T>
std::to_chars_result shortest_to_chars(
    char* first, char* last, T value, const std::chars_format* format
) {
    std::string out;
    if (non_finite(value, out)) {
        return put(first, last, out);
    }
    if (format && *format == std::chars_format::hex) {
        char text[64];
        std::snprintf(text, sizeof text, "%a", static_cast<double>(value));
        return put(first, last, hex_text(text));
    }
    std::string digits = "0";
    int exponent = 0;
    if (value != 0) {
        shortest(value, digits, exponent);
    }
    const std::string sci = scientific(digits, exponent);
    const std::string fix = fixed(digits, exponent);
    if (!format) {
        out = fix.size() <= sci.size() ? fix : sci;
    } else if (*format == std::chars_format::scientific) {
        out = sci;
    } else if (*format == std::chars_format::fixed) {
        out = fix;
    } else {  // general: %g's rule, with as many digits as the shortest form has
        out = exponent < -4 || exponent >= static_cast<int>(digits.size()) ? sci : fix;
    }
    return put(first, last, std::string(std::signbit(value) ? "-" : "") + out);
}

template <class T>
std::to_chars_result precise_to_chars(
    char* first, char* last, T value, std::chars_format format, int precision
) {
    std::string out;
    if (non_finite(value, out)) {
        return put(first, last, out);
    }
    const char* spec = "%.*g";
    if (format == std::chars_format::fixed) {
        spec = "%.*f";
    } else if (format == std::chars_format::scientific) {
        spec = "%.*e";
    } else if (format == std::chars_format::hex) {
        spec = "%.*a";
    }
    // A double's %f is up to about 310 digits before the point; a longer result is reported as a
    // buffer too short is.
    char text[800];
    const int length =
        std::snprintf(text, sizeof text, spec, precision, static_cast<double>(value));
    if (length < 0 || length >= static_cast<int>(sizeof text)) {
        return {last, std::errc::value_too_large};
    }
    return put(
        first,
        last,
        format == std::chars_format::hex ? hex_text(text)
                                         : std::string(text, static_cast<size_t>(length))
    );
}

}  // namespace

std::to_chars_result std::to_chars(char* first, char* last, float value) {
    return shortest_to_chars<float>(first, last, value, nullptr);
}

std::to_chars_result std::to_chars(char* first, char* last, double value) {
    return shortest_to_chars<double>(first, last, value, nullptr);
}

std::to_chars_result std::to_chars(char* first, char* last, float value, std::chars_format format) {
    return shortest_to_chars<float>(first, last, value, &format);
}

std::to_chars_result std::to_chars(
    char* first, char* last, double value, std::chars_format format
) {
    return shortest_to_chars<double>(first, last, value, &format);
}

std::to_chars_result std::to_chars(
    char* first, char* last, float value, std::chars_format format, int precision
) {
    return precise_to_chars<float>(first, last, value, format, precision);
}

std::to_chars_result std::to_chars(
    char* first, char* last, double value, std::chars_format format, int precision
) {
    return precise_to_chars<double>(first, last, value, format, precision);
}

// As libc++ does: a long double is written as the double it rounds to.
std::to_chars_result std::to_chars(char* first, char* last, long double value) {
    return std::to_chars(first, last, static_cast<double>(value));
}

std::to_chars_result std::to_chars(
    char* first, char* last, long double value, std::chars_format format
) {
    return std::to_chars(first, last, static_cast<double>(value), format);
}

std::to_chars_result std::to_chars(
    char* first, char* last, long double value, std::chars_format format, int precision
) {
    return std::to_chars(first, last, static_cast<double>(value), format, precision);
}

// The hash libc++'s headers use where the system's library has none.
size_t std::__hash_memory(__attribute__((noescape)) const void* data, size_t size) noexcept {
    return std::__murmur2_or_cityhash<size_t>()(data, size);
}

void std::__libcpp_verbose_abort(const char* format, ...) noexcept {
    va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    std::abort();
}
