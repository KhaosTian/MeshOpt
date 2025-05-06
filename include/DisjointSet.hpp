#pragma once

#include "Common.hpp"

class DisjointSet {
public:
    DisjointSet() {}
    DisjointSet(uint32 size);

    void Init(uint32 size);
    void Reset();
    void AddDefaulted(uint32 num = 1);

    void   Union(uint32 x, uint32 y);
    void   UnionSequential(uint32 x, uint32 y);
    uint32 Find(uint32 i);

    uint32 operator[](uint32 i) const { return m_parents[i]; }

private:
    std::vector<uint32> m_parents; // 数组存储的是每个节点的父节点索引
};

FORCEINLINE DisjointSet::DisjointSet(uint32 size) {
    Init(size);
}

FORCEINLINE void DisjointSet::Init(uint32 size) {
    m_parents.reserve(size);
    for (uint32 i = 0; i < size; i++) {
        m_parents[i] = i; // 初始化每个节点的父节点是自己
    }
}

FORCEINLINE void DisjointSet::Reset() {
    m_parents.clear();
}

FORCEINLINE void DisjointSet::AddDefaulted(uint32 num) {
    uint32 start = m_parents.size();
    uint32 end   = start + num;

    m_parents.reserve(end);

    for (uint32 i = start; i < end; i++) {
        m_parents[i] = i;
    }
}

// 合并共享边或顶点的三角形，合并时遵循"索引大的合并索引小的"原则，使小索引三角形的父节点指向大索引三角形。
FORCEINLINE void DisjointSet::Union(uint32 x, uint32 y) {
    // 获取x和y的根节点
    uint32 px = m_parents[x];
    uint32 py = m_parents[y];

    // 两节点的parent不同时合并
    while (px != py) {
        if (px < py) // 索引较小的根合并到较大的根
        {
            m_parents[x] = py; // x的父节点设为y的父节点
            if (x == px) // 若x就是根节点，合并到此结束
            {
                return;
            }
            // 向上追溯，更新x和px为上层节点
            x  = px;
            px = m_parents[x];
        } else {
            m_parents[y] = px;
            if (y == py) {
                return;
            }
            y  = py;
            py = m_parents[y];
        }
    }
}

// 高性能版本Union，
// 为按索引升序遍历0到N切仅合并当前元素x与更小索引的场景设计
FORCEINLINE void DisjointSet::UnionSequential(uint32 x, uint32 y) {
    // 限定y小于x索引，且x不是根节点
    CHECK(x >= y);
    CHECK(x == m_parents[x]);

    uint32 px = x;
    uint32 py = m_parents[y];

    while (px != py) {
        // 索引较小的合并到较大的
        m_parents[y] = px;
        if (y == py) {
            return;
        }
        // 向上追溯，更新x和px为上层节点
        y  = py;
        py = m_parents[y];
    }
}

// 确定每个三角形所属的cluster根节点，根节点就是这个独立的拓扑结构最大的索引（最后一个三角形）
FORCEINLINE uint32 DisjointSet::Find(uint32 i) {
    // 根据i查找其根节点，同时压缩路径上的所有节点
    uint32 start = i; // 记录i的初始值
    uint32 root  = m_parents[i]; // 根节点，从i父节点开始

    // 向上遍历到根节点
    while (root != i) {
        i    = root;
        root = m_parents[i];
    }

    i = start; // i设回初始值

    uint32 parent = m_parents[i]; // 记录i的父节点

    // 向上遍历到根节点
    while (parent != root) {
        m_parents[i] = root; // i的父节点设为根节点
        // 向上追溯，更新i和parent为更上层节点
        i      = parent;
        parent = m_parents[i];
    }
    // 从i到根节点上的所有节点的父节点都指向了根节点，对路径进行了压缩
    return root;
}