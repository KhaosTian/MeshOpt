#include "Common.hpp"
#include "Cluster.hpp"
#include "Parallel.hpp"
#include "StridedView.hpp"
#include "EdgeHash.hpp"
#include "VectorMath.hpp"
#include "Adjacency.hpp"
#include "DisjointSet.hpp"
#include "GraphPartitioner.hpp"

struct MeshBuildVertexView {
    std::vector<Point3f> Positions;
};

static void ClusterTriangles(
    MeshBuildVertexView&       verts,
    const std::vector<uint32>& indices,
    const std::vector<int32>&  material_indexes,
    std::vector<Cluster>&      clusters,
    const Bounds3f&            mesh_bounds
) {
    uint32 num_triangles = static_cast<uint32>(indices.size() / 3);

    Adjacency adjacency { indices.size() };
    EdgeHash  edge_hash { indices.size() };

    auto GetPosition = [&verts, &indices](uint32 edge_index) { return verts.Positions[indices[edge_index]]; };

    // 将每个索引视作一条边，构建边的哈希表
    ParallelFor("ClusterTriangles.ParalleFor", indices.size(), 4096, [&](int edge_index) {
        edge_hash.AddConcurrent(edge_index, GetPosition);
    });

    // 将每个索引视作一条边，确定边的邻接关系
    ParallelFor("ClusterTriangles.ParalleFor", indices.size(), 1024, [&](int32 edge_index) {
        int32 adj_index = -1; // -1表示没有邻接边
        int32 adj_count = 0;

        // 遍历边的邻接边
        edge_hash.ForAllMatching(edge_index, false, GetPosition, [&](int32 edge_index, int32 other_edge_index) {
            adj_index = other_edge_index; // 记录邻接边的索引
            adj_count++;
        });

        // 通常共边三角形的那条共边是一对方向相反的边互相邻接
        if (adj_count > 1) adj_index = -2; // 如果超过了1条邻接边，说明是个复杂连接

        adjacency.direct[edge_index] = adj_index; // 记录直接邻边
    });

    DisjointSet disjoint_set(num_triangles);

    // 遍历所有边，最终得到若干个互不连通的拓扑结构
    for (uint32 edge_index = 0, num = static_cast<uint32>(indices.size()); edge_index < num; edge_index++) {
        // 处理复杂边
        if (adjacency.direct[edge_index] == -2) {
            std::vector<std::pair<int32, int32>> edges;
            // 收集所有匹配当前边的边
            edge_hash.ForAllMatching(edge_index, false, GetPosition, [&](int32 edge_index0, int32 edge_index1) {
                edges.emplace_back(edge_index0, edge_index1);
            });

            // 标准库排序保证确定性
            std::sort(edges.begin(), edges.end());

            // 建立邻接关系
            for (const auto& edge: edges) {
                adjacency.Link(edge.first, edge.second);
            }
        }

        // 遍历当前边的邻接边
        adjacency.ForAll(edge_index, [&](int32 edge_index0, int32 edge_index1) {
            // 合并邻边三角形
            if (edge_index0 > edge_index1) {
                // 随着连续合并操作，三角形间形成一条路径链，最大索引的三角形自然成为整个连通结构的终点
                disjoint_set.UnionSequential(edge_index0 / 3, edge_index1 / 3);
            }
        });
    }

    // 初始化图划分器
    GraphPartitioner partitioner(num_triangles, Cluster::ClusterSize - 4, Cluster::ClusterSize);
    {
        // 获取三角形的中心坐标
        auto GetCenter = [&verts, &indices](uint32 tri_index) {
            Point3f center;
            center = verts.Positions[indices[tri_index * 3 + 0]];
            center += verts.Positions[indices[tri_index * 3 + 1]];
            center += verts.Positions[indices[tri_index * 3 + 2]];
            return center * (1.0f / 3.0f);
        };

        // 建立邻接关系
        partitioner.BuildLocalityLinks(disjoint_set, mesh_bounds, material_indexes, GetCenter);

        // restrict 保证只有这个指针指向这块内存，方便编译器优化，若违反则可能导致未定义行为
        auto* RESTRICT graph = partitioner.NewGraph(num_triangles * 3);

        // 遍历每个三角形
        for (uint32 i = 0; i < num_triangles; i++) {
            graph->adjacency_offset[i] = graph->adjacency.size(); // 设置邻接表偏移量
            uint32 tri_index           = partitioner.indices[i]; // 获取三角形索引
            // 遍历三角形的三个边
            for (int k = 0; k < 3; k++) {
                // 遍历边的所有邻接边
                adjacency.ForAll(tri_index * 3 + k, [&partitioner, graph](int32 edge_index, int32 adj_index) {
                    partitioner.AddAdjaceny(graph, adj_index / 3, 4 * 65); // 将邻接边所在的三角形索引添加到邻接三角形
                });
            }

            // 将该三角形索引添加
            partitioner.AddLocalityLinks(graph, tri_index, 1);
        }

        // 设置最后一个三角形的邻接偏移量
        if (num_triangles <= 0) return;
        graph->adjacency_offset[num_triangles] = graph->adjacency.size();

        // 三角形超过5000个则启用多线程划分
        bool enable_multi_threaded = num_triangles >= 5000;
        partitioner.ParititionStrict(graph, enable_multi_threaded);

        CHECK(partitioner.ranges.size());
    }
}

int main() {
    MeshBuildVertexView  verts {};
    std::vector<uint32>  indexes {};
    std::vector<int32>   material_indexes {};
    std::vector<Cluster> clusters = {};
    Bounds3f             mesh_bounds;

    ClusterTriangles(verts, indexes, material_indexes, clusters, mesh_bounds);

    std::cout << "Hello!\n";
    return 0;
}