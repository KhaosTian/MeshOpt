#pragma once

#include "Common.h"

struct Adjacency {
    // 存储每个边的一个直接邻接边，-1表示没有直接邻接
    std::vector<int32> direct;

    // 存储额外的邻接关系，当一个边有多个邻接边时使用
    std::multimap<int32, int32> extended;

    Adjacency(size_t num);
    void AddUnique(int32 key, int32 value);
    void Link(int32 edge_index0, int32 edge_index1);

    template<typename FuncType>
    void ForAll(int32 edge_index, FuncType&& Function) const;
};

// 遍历指定边的所有邻接边，并对每个邻接对应用给定函数
template<typename FuncType>
FORCEINLINE void Adjacency::ForAll(int32 edge_index, FuncType&& Function) const {
    // 首先检查Direct数组中的直接邻接
    int32 adj_index = direct[edge_index];
    if (adj_index >= 0) {
        // 对直接邻接应用函数
        Function(edge_index, adj_index);
    }

    // 然后检查Extended中的所有邻接
    auto [begin, end] = extended.equal_range(edge_index);
    for (auto& it = begin; it != end; ++it) {
        // 对每个额外邻接应用函数
        Function(edge_index, it->second);
    }
}

FORCEINLINE Adjacency::Adjacency(size_t num) {
    // 初始化Direct数组，所有值设为-1表示尚未连接
    direct.resize(num, -1);
}

// 向Extended映射中添加键值对，确保不重复添加
FORCEINLINE void Adjacency::AddUnique(int32 key, int32 value) {
    // 获取指定键的所有已存在值
    auto [begin, end] = extended.equal_range(key);
    bool found        = false;

    // 检查是否已存在相同的键值对
    for (auto& it = begin; it != end; ++it) {
        if (it->second == value) {
            found = true;
            break;
        }
    }

    // 仅在不存在时添加新的键值对
    if (!found) {
        extended.insert({ key, value });
    }
}

// 在两个边之间建立邻接连接
// 如果两个边都没有直接邻接边，则使用Direct数组存储它们之间的关系。
// 否则，使用Extended多重映射存储它们之间的关系。
FORCEINLINE void Adjacency::Link(int32 edge_index0, int32 edge_index1) {
    // 如果两个边都没有直接邻接边，使用Direct数组连接它们
    if (direct[edge_index0] < 0 && direct[edge_index1] < 0) {
        direct[edge_index0] = edge_index1;
        direct[edge_index1] = edge_index0;
    } else {
        // 否则使用Extended存储它们之间的连接关系
        AddUnique(edge_index0, edge_index1);
        AddUnique(edge_index1, edge_index0);
    }
}