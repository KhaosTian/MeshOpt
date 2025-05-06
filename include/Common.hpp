#pragma once

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstdint>
#include <cmath>

#include <iostream>
#include <memory>
#include <algorithm>
#include <string>
#include <bit>
#include <type_traits>

#include <vector>
#include <map>
#include <unordered_map>

#include "VectorMath.hpp"

#define DEBUG_BREAK() __debugbreak()

#define RESTRICT __restrict

#define NOMINMAX

using int16  = int16_t;
using int32  = int32_t;
using int64  = int64_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using ErrorHandler = void (*)(const char*, const char*, int, const char*);

FORCEINLINE static void DefaultErrorHandler(const char* expr, const char* file, int line, const char* msg = nullptr) {
    std::fprintf(stderr, "Assertion Failed:\n");
    std::fprintf(stderr, "  Expression: %s\n", expr);
    std::fprintf(stderr, "  File:       %s\n", file);
    std::fprintf(stderr, "  Line:       %d\n", line);
    if (msg) {
        std::fprintf(stderr, "  Message:    %s\n", msg);
    }
    std::fflush(stderr);
}

static ErrorHandler g_ErrorHandler = &DefaultErrorHandler;

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            if (g_ErrorHandler) { \
                g_ErrorHandler(#expr, __FILE__, __LINE__, nullptr); \
            } \
            DEBUG_BREAK(); \
            std::abort(); \
        } \
    } while (0)

FORCEINLINE static uint32 MurmurFinalize32(uint32 hash) {
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    return hash;
}

FORCEINLINE static uint32 Murmur32(std::initializer_list<uint32> init_list) {
    uint32 hash = 0;
    for (auto element: init_list) {
        element *= 0xcc9e2d51;
        element = (element << 15) | (element >> (32 - 15));
        element *= 0x1b873593;

        hash ^= element;
        hash = (hash << 13) | (hash >> (32 - 13));
        hash = hash * 5 + 0xe6546b64;
    }

    return MurmurFinalize32(hash);
}