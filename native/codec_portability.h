#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <stdexcept>
using BOOL = int;
#define TRUE 1
#define FALSE 0
using std::min;
inline int strcat_s(char *dst, size_t size, const char *src) {
    if (strlen(dst) + strlen(src) >= size) throw std::length_error("protocol buffer overflow");
    strcat(dst, src); return 0;
}
template<size_t N> int strcat_s(char (&dst)[N], const char *src) { return strcat_s(dst, N, src); }
template<size_t N> int strncpy_s(char (&dst)[N], const char *src, size_t count) {
    size_t n = std::min({count, strlen(src), N-1});
    memcpy(dst, src, n); dst[n] = 0; return 0;
}
inline int sprintf_s(char *dst, size_t size, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); int n = vsnprintf(dst, size, fmt, ap); va_end(ap);
    if (n < 0 || size_t(n) >= size) throw std::length_error("protocol format overflow");
    return n;
}
static_assert(sizeof(int) == 4, "Stone Age protocol requires 32-bit int");
inline int strncat_s(char *dst, size_t size, const char *src, size_t count) {
    size_t n = std::min(count, strlen(src));
    size_t used = strlen(dst);
    if (used + n >= size) throw std::length_error("protocol append overflow");
    memcpy(dst + used, src, n); dst[used+n] = 0; return 0;
}
