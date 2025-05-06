#pragma once

#include "Common.hpp"
#include "HashTable.hpp"

struct EdgeHash {
    HashTable hash_table {};
    EdgeHash(size_t num): hash_table { 1u << std::bit_floor(static_cast<uint32>(num)), static_cast<uint32>(num) } {}

    template<typename FuncType>
    void AddConcurrent(int32 edge_index, FuncType&& GetPosition);
    template<typename FuncType1, typename FuncType2>
        requires std::invocable<FuncType1, int32> && std::same_as<std::invoke_result_t<FuncType1, int32>, Vector3f>
    void ForAllMatching(int32 edge_index, bool need_add, FuncType1&& GetPosition, FuncType2&& Function);
};

FORCEINLINE static uint32 HashPosition(const Vector3f& position) {
    auto ToUint = [](float f) {
        union {
            float  f;
            uint32 i;
        } u = { f };
        return f == 0.0 ? 0u : u.i; // 兼容-0.0，确保零值哈希一致
    };

    // 将位置的三个浮点数坐标映射到一维哈希key
    return Murmur32({ ToUint(position.x), ToUint(position.y), ToUint(position.z) });
}

FORCEINLINE static uint32 Cycle3(uint32 value) {
    uint32 value_mod3      = value % 3;
    uint32 next_value_mod3 = (1 << value_mod3) & 3;
    return value - value_mod3 + next_value_mod3;
}

template<typename FuncType>
FORCEINLINE void EdgeHash::AddConcurrent(int32 edge_index, FuncType&& GetPosition) {
    // 根据边索引获取坐标和其相邻坐标
    const Vector3f position0 = GetPosition(edge_index);
    const Vector3f position1 = GetPosition(Cycle3(edge_index));

    // 将两个顶点的坐标分别映射为一维的哈希值
    uint32 hash0 = HashPosition(position0);
    uint32 hash1 = HashPosition(position1);

    // 继续将二者的哈希值映射为一个哈希值
    uint32 hash = Murmur32({ hash0, hash1 });

    // 将哈希值映射到边索引
    hash_table.AddConcurrent(hash, edge_index);
}

// 匹配所有与自己共享顶点但是方向相反的边
template<typename FuncType1, typename FuncType2>
    requires std::invocable<FuncType1, int32> && std::same_as<std::invoke_result_t<FuncType1, int32>, Vector3f>
FORCEINLINE void
EdgeHash::ForAllMatching(int32 edge_index, bool need_add, FuncType1&& GetPosition, FuncType2&& Function) {
    // 根据边索引获取坐标和其相邻坐标
    const Vector3f position0 = GetPosition(edge_index);
    const Vector3f position1 = GetPosition(Cycle3(edge_index));

    // 将两个顶点的坐标分别映射为一维的哈希值
    uint32 hash0 = HashPosition(position0);
    uint32 hash1 = HashPosition(position1);

    // 继续将二者的哈希值映射为一个哈希值
    uint32 hash = Murmur32({ hash0, hash1 });

    // 从头节点开始遍历该哈希桶所有边
    for (uint32 other_edge_index = hash_table.First(hash); hash_table.IsValid(other_edge_index);
         other_edge_index        = hash_table.Next(other_edge_index)) {
        // 匹配和当前边共享顶点但是方向相反的边，即两个三角形共享一条边
        if (position0 == GetPosition(Cycle3(other_edge_index)) && position1 == GetPosition(other_edge_index)) {
            Function(edge_index, other_edge_index);
        }
    }

    // 如果有需要就加入到哈希表中
    if (need_add) {
        hash_table.Add(Murmur32({ hash1, hash0 }), edge_index);
    }
}