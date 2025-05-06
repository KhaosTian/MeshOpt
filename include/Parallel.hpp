#pragma once

#include "Common.hpp"

template<typename FuncType>
static inline void ParallelFor(const std::string& message, size_t count, int32 batch_size, FuncType&& Function) {
    for (auto index = 0; index < count; ++index) {
        Function(index);
    }
}
