#pragma once

#include "METIS/metis.h"

#include "Common.hpp"
#include "DisjointSet.hpp"
#include "Math/BoundingBox.hpp"

class GraphPartitioner {
public:
    struct GraphData {
        int32 offset;
        int32 num;

        std::vector<idx_t> adjacency {};
        std::vector<idx_t> adjacency_cost {};
        std::vector<idx_t> adjacency_offset {};
    };

    struct Range {
        uint32 begin;
        uint32 end;

        bool operator<(const Range& other) const { return begin < other.begin; }
    };

    std::vector<Range>  ranges;
    std::vector<uint32> indices;
    std::vector<uint32> sorted_to;

public:
    GraphPartitioner(uint32 num_elements, int32 min_partition_size, int32 max_partition_size);
    GraphData* NewGraph(uint32 num_adjacency) const;

    void AddAdjaceny(GraphData* graph, uint32 adj_index, idx_t cost);
    void AddLocalityLinks(GraphData* graph, uint32 index, idx_t cost);

    template<typename FuncType>
    void BuildLocalityLinks(
        DisjointSet&              disjoint_set,
        const Bounds3f&           bounds,
        const std::vector<int32>& group_indices,
        FuncType&                 GetCenter
    );

    void Partition(GraphData* graph);

    void ParititionStrict(GraphData* graph, bool enable_threaded);

    void BisectGraph(GraphData* graph, GraphData* child_graphs[2]);
    void RecursiveBisectGraph(GraphData* graph);

    uint32 num_elements;
    int32  min_partition_size;
    int32  max_partition_size;

    std::atomic<uint32> num_parition;

    std::vector<idx_t> partition_ids;
    std::vector<int32> swapped_with;

    std::multimap<int32, uint32> locality_links;
};

FORCEINLINE static constexpr uint32 MorotonCode3(uint32 x) {
    // 莫顿码只处理0到1023，即2的10次方，所以使用掩码过滤掉高位的值
    x &= 0x000003ff; //00000000 00000000 00000011 11111111
    x = (x ^ (x << 16)) & 0xff0000ff; //11111111 00000000 00000000 11111111
    x = (x ^ (x << 8)) & 0x0300f00f; //00000011 00000000 11110000 00001111
    x = (x ^ (x << 4)) & 0x030c30c3; //00000011 00001100 00110000 11000011
    x = (x ^ (x << 2)) & 0x09249249; //00001001 00100100 10010010 01001001
    return x;
}

FORCEINLINE static constexpr uint32 ReverseMortonCode3(uint32 x) {
    x &= 0x09249249;
    x = (x ^ (x >> 2)) & 0x030c30c3;
    x = (x ^ (x >> 4)) & 0x0300f00f;
    x = (x ^ (x >> 8)) & 0xff0000ff;
    x = (x ^ (x >> 16)) & 0x000003ff;
    return x;
}

template<class FuncType>
FORCEINLINE static void RadixSort32(uint32* RESTRICT dst, uint32* RESTRICT src, uint32 num, FuncType&& SortKey) {
    // 将莫顿码分割为低10位、中11位和高11位，对应1024个桶，2048个桶，2048个桶
    uint32 histograms[1024 + 2048 + 2048];

    uint32* RESTRICT histogram0 = histograms + 0;
    uint32* RESTRICT histogram1 = histogram0 + 1024;
    uint32* RESTRICT histogram2 = histogram1 + 2048;

    std::memset(histograms, 0, sizeof(histograms)); // 都初始化为0

    // 分桶，桶数组本身就是排序好的0到1023
    {
        const uint32* RESTRICT s = (const uint32* RESTRICT)src;
        // 遍历三角形
        for (auto i = 0; i < num; i++) {
            uint32 key = SortKey(s[i]); // 获取三角形的莫顿码
            // 初始化桶数组histograms，此时每个桶存储的是匹配该桶的元素数量
            histogram0[(key >> 0) & 1023]++; // 00000000 00000000 00000011 11111111
            histogram1[(key >> 10) & 2047]++; // 00000000 00000000 00000111 11111111
            histogram2[(key >> 21) & 2047]++; // 00000000 00000000 00000111 11111111
        }
    }

    // 前缀和，此时桶数组histograms中的元素数量，变为累加和，最后一个桶中是所有元素数量
    {
        uint32 sum0 = 0;
        uint32 sum1 = 0;
        uint32 sum2 = 0;

        // 遍历所有桶构造前缀和
        for (uint32 i = 0, t = 0; i < 2048; i++) {
            // for循环会将前面的元素数量加起来赋值给桶，等于计算了该桶元素在最终排序数组中的的起始索引（-1）
            if (i < 1024) {
                t             = histogram0[i] + sum0;
                histogram0[i] = sum0 - 1;
                sum0          = t;
            }

            t             = histogram1[i] + sum1;
            histogram1[i] = sum1 - 1;
            sum1          = t;

            t             = histogram2[i] + sum2;
            histogram2[i] = sum2 - 1;
            sum2          = t;
        }
    }

    // sort pass 1 对低10位排序
    {
        const uint32* RESTRICT s = (const uint32* RESTRICT)src; // indices，三角形索引数组
        uint32* RESTRICT       d = dst; // sorted_to，排序结果
        // 遍历三角形
        for (uint32 i = 0; i < num; i++) {
            uint32 value = s[i]; // 三角形的索引
            uint32 key   = SortKey(value); // 根据三角形索引得到三角形的莫顿码

            uint32 bucket = (key >> 0) & 1023; // 掩码截取莫顿码的低10位
            uint32 index = ++histogram0[bucket]; // 获取该桶的前缀和，前缀和并加一为下一次遍历到该桶做准备
            d[index] = value; // 将该索引放到排序的位置
        }
    }

    // sort pass 2 对中11位排序，但是基于sort pass 1的排序结果原地排序
    {
        const uint32* RESTRICT s = (const uint32* RESTRICT)dst;
        uint32* RESTRICT       d = src;
        for (uint32 i = 0; i < num; i++) {
            uint32 value = s[i];
            uint32 key   = SortKey(value);

            uint32 bucket = (key >> 10) & 2047;
            uint32 index  = ++histogram1[bucket];
            d[index]      = value;
        }
    }

    // sort pass 3 对高11位排序，继续反转
    {
        const uint32* RESTRICT s = (const uint32* RESTRICT)src;
        uint32* RESTRICT       d = dst;
        for (uint32 i = 0; i < num; i++) {
            uint32 value = s[i];
            uint32 key   = SortKey(value);

            uint32 bucket = (key >> 21) & 2047;
            uint32 index  = ++histogram2[bucket];
            d[index]      = value;
        }
    }
}

FORCEINLINE static float MaxComponent(Vector3f v) {
    return (v.x > v.y) ? (v.x > v.z ? v.x : v.z) : (v.y > v.z ? v.y : v.z);
}

// 在空间上建立三角形的邻近关系
template<typename FuncType>
FORCEINLINE void GraphPartitioner::BuildLocalityLinks(
    DisjointSet&              disjoint_set,
    const Bounds3f&           bounds,
    const std::vector<int32>& group_indices,
    FuncType&                 GetCenter
) {
    std::vector<uint32> sort_keys; // 存储每个三角形质心的莫顿码
    sort_keys.reserve(num_elements);
    sorted_to.reserve(num_elements);

    const bool enable_groups = !group_indices.empty();

    ParallelFor("BuildLocalityLinks.ParallelFor", num_elements, 4096, [&](uint32 index) {
        Point3f center = GetCenter(index);
        // 将坐标系转换到以包围盒最小点为原点的本地坐标系,除以包围盒的尺寸得到归一化的坐标
        Point3f center_local = (center - bounds.GetMin()) / (bounds.GetMax() - bounds.GetMin());

        // 将3D坐标映射为一维的莫顿码，空间上接近的坐标其数值也更接近
        uint32 morton;
        // 分别获取三个维度的莫顿码，将0到1的坐标放大为0到1023的整数
        morton = MorotonCode3(static_cast<uint32>(center_local.x * 1023));
        morton |= MorotonCode3(static_cast<uint32>(center_local.y * 1023)) << 1;
        morton |= MorotonCode3(static_cast<uint32>(center_local.z * 1023)) << 2;
        sort_keys[index] = morton;
    });

    // 基数排序
    RadixSort32(sorted_to.data(), indices.data(), num_elements, [&](uint32 index) { return sort_keys[index]; });

    // 清理sort_keys数组
    sort_keys.clear();
    sort_keys.shrink_to_fit();

    // 交换数据后，indices可以根据位置索引得到三角形索引
    std::swap(indices, sorted_to);
    // 遍历所有三角形，做反向映射
    for (uint32 i = 0; i < num_elements; i++) {
        sorted_to[indices[i]] = i; // sorted_to可以根据三角形索引得到位置索引
    }

    // 每个三角形都有一个range记录所属联通区域的起始和结束
    std::vector<Range> island_ranges;
    island_ranges.reserve(num_elements);

    {
        uint32 curr_range  = 0;
        uint32 range_begin = 0;

        // 遍历所有三角形
        for (uint32 i = 0; i < num_elements; i++) {
            // 确定当前三角形所属的连通区域
            uint32 range_id = disjoint_set.Find(indices[i]);
            if (curr_range != range_id) {
                // 更新区域内所有range的end
                for (uint32 j = range_begin; j < i; j++) {
                    island_ranges[j].end = i - 1;
                }

                // 更新当前range标识和begin索引
                curr_range  = range_id;
                range_begin = i;
            }

            island_ranges[i].begin = range_begin; // 设置该三角形所属联通区域的begin
        }

        // 完成最后一个range的end更新
        for (uint32 j = range_begin; j < num_elements; j++) {
            island_ranges[j].end = num_elements - 1;
        }
    }

    // 遍历所有三角形，此时三角形索引是按照莫顿码排序后的indices
    for (uint32 i = 0; i < num_elements; i++) {
        uint32_t index = indices[i];

        // 若该三角形属于小于128个三角形的独立拓扑结构
        uint32 range_size = island_ranges[i].end - island_ranges[i].begin + 1;
        if (range_size < 128) {
            uint32 island_id = disjoint_set[index];
            int32  group_id  = enable_groups ? group_indices[index] : 0;

            Point3f center = GetCenter(index);

            const uint32 max_links = 5;

            // 初始化
            uint32 closest_index[max_links];
            float  closest_dist2[max_links];
            for (auto k = 0; k < max_links; k++) {
                closest_index[k] = ~0u;
                closest_dist2[k] = FLT_MAX;
            }

            // 向前和向后搜索邻接adj的island
            for (int direction = 0; direction < 2; direction++) {
                // 向前不能超过0，向后不能超过size-1
                uint32 limit = direction ? num_elements - 1 : 0;
                uint32 step  = direction ? 1 : -1;

                uint32 adj = i;
                // 最多搜索16步
                for (int32 it = 0; it < 16; it++) {
                    if (adj == limit) break;
                    adj += step;

                    uint32 adj_index     = indices[adj];
                    uint32 adj_island_id = disjoint_set[adj_index]; // 获取邻接三角形所属的island

                    int32 adj_group_id = enable_groups ? group_indices[adj_index] : 0;

                    // island相同 或者 group不匹配 则跳过整个区间
                    if (island_id == adj_island_id || (group_id != adj_group_id)) {
                        if (direction)
                            adj = island_ranges[adj].end;
                        else
                            adj = island_ranges[adj].begin;
                    } else {
                        // 计算二者的距离，按最短距离优先存入数组，记录索引
                        float adj_dist2 = Math::Vector3::DistanceSquared(center, GetCenter(adj_index));
                        for (int k = 0; k < max_links; k++) {
                            // 维护最多5个元素的最近邻居数组
                            if (adj_dist2 < closest_dist2[k]) {
                                std::swap(adj_dist2, closest_dist2[k]);
                                std::swap(adj_index, closest_index[k]);
                            }
                        }
                    }
                }
            }

            // 存储其局部连接性
            for (int k = 0; k < max_links; k++) {
                if (closest_index[k] != ~0u) {
                    locality_links.emplace(index, closest_index[k]);
                    locality_links.emplace(closest_index[k], index);
                }
            }
        }
    }
}

FORCEINLINE
GraphPartitioner::GraphPartitioner(uint32 num_elements, int32 min_partition_size, int32 max_partition_size) {
}

FORCEINLINE GraphPartitioner::GraphData* GraphPartitioner::NewGraph(uint32 num_adjacency) const {
    return nullptr;
}

FORCEINLINE void GraphPartitioner::AddAdjaceny(GraphData* graph, uint32 adj_index, idx_t cost) {
}

FORCEINLINE void GraphPartitioner::AddLocalityLinks(GraphData* graph, uint32 index, idx_t cost) {
}

FORCEINLINE void GraphPartitioner::Partition(GraphData* graph) {
}

FORCEINLINE void GraphPartitioner::ParititionStrict(GraphData* graph, bool enable_threaded) {
}

FORCEINLINE void GraphPartitioner::BisectGraph(GraphData* graph, GraphData* child_graphs[2]) {
}

FORCEINLINE void GraphPartitioner::RecursiveBisectGraph(GraphData* graph) {
}