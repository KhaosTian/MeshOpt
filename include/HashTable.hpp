#pragma once

#include "Common.hpp"

class HashTable {
public:
    HashTable(uint32 hash_size = 1024, uint32 index_size = 0);
    HashTable(const HashTable& other);
    HashTable(HashTable&& other) noexcept;
    HashTable& operator=(const HashTable& other);
    HashTable& operator=(HashTable&& other) noexcept;

    void Clear() const;
    void Free();

    void Resize(uint32 new_index_size);

    uint32 IndexSize() const { return m_index_size; }
    uint32 HashSize() const { return m_hash_size; }

    uint32 First(uint32 key) const;
    uint32 Next(uint32 index) const;
    bool   IsValid(uint32 index) const;

    void Add(uint32 key, uint32 index);
    void AddConcurrent(uint32 key, uint32 index) const;
    void Remove(uint32 key, uint32 index) const;

protected:
    uint32 m_hash_size;
    uint32 m_hash_mask;
    uint32 m_index_size;

    uint32* m_hashes; // 哈希数组中存储每个桶的链表头节点索引
    uint32* m_next_indices; // 存储从index可以达到的下一个索引

    static uint32 EmptyHash[1];
};

inline uint32 HashTable::EmptyHash[1] = { ~0u };

inline HashTable::HashTable(uint32 hash_size, uint32 index_size):
    m_hash_size(hash_size),
    m_hash_mask(0),
    m_index_size(index_size),
    m_hashes(EmptyHash),
    m_next_indices(nullptr) {
    // 确保哈希表的大小大于0且是2的幂次方
    CHECK(m_hash_size > 0);
    CHECK(std::has_single_bit(m_hash_size));

    // 对于大小为2的幂次方的哈希表，x % m_HashSize 等价于 x & (m_HashSize - 1)
    if (m_index_size) {
        m_hash_mask = m_hash_size - 1;
        // 分配哈希桶的头索引和链表
        m_hashes       = new uint32[m_index_size];
        m_next_indices = new uint32[m_index_size];
        // 初始化数组元素为0xff
        std::memset(m_hashes, 0xff, m_hash_size * sizeof(uint32));
    }
}

inline HashTable::HashTable(const HashTable& other):
    m_hash_size(other.m_hash_size),
    m_hash_mask(other.m_hash_mask),
    m_index_size(other.m_index_size),
    m_hashes(EmptyHash), // 让未初始化或已释放的哈希表也能安全响应
    m_next_indices(nullptr) {
    if (m_index_size) {
        m_hashes       = new uint32[m_hash_size];
        m_next_indices = new uint32[m_index_size];

        // 拷贝内存
        std::memcpy(m_hashes, other.m_hashes, m_hash_size * sizeof(uint32));
        std::memcpy(m_next_indices, other.m_next_indices, m_index_size * sizeof(uint32));
    }
}

inline HashTable::HashTable(HashTable&& other) noexcept:
    m_hash_size(other.m_hash_size),
    m_hash_mask(other.m_hash_mask),
    m_index_size(other.m_index_size),
    m_hashes(other.m_hashes),
    m_next_indices(other.m_next_indices) {
    other.m_hash_size    = 0;
    other.m_hash_mask    = 0;
    other.m_index_size   = 0;
    other.m_hashes       = EmptyHash;
    other.m_next_indices = m_next_indices;
}

inline HashTable& HashTable::operator=(const HashTable& other) {
    if (this == &other) { // 显式处理自赋值
        return *this;
    }

    Free();

    m_hash_size    = other.m_hash_size;
    m_hash_mask    = other.m_hash_mask;
    m_index_size   = other.m_index_size;
    m_hashes       = EmptyHash;
    m_next_indices = nullptr;

    if (m_index_size) {
        m_hashes       = new uint32[m_hash_size];
        m_next_indices = new uint32[m_index_size];

        // 拷贝内存
        std::memcpy(m_hashes, other.m_hashes, m_hash_size * sizeof(uint32));
        std::memcpy(m_next_indices, other.m_next_indices, m_index_size * sizeof(uint32));
    }

    return *this;
}

inline HashTable& HashTable::operator=(HashTable&& other) noexcept {
    m_hash_size    = other.m_hash_size;
    m_hash_mask    = other.m_hash_mask;
    m_index_size   = other.m_index_size;
    m_hashes       = other.m_hashes;
    m_next_indices = other.m_next_indices;

    other.m_hash_size    = 0;
    other.m_hash_mask    = 0;
    other.m_index_size   = 0;
    other.m_hashes       = EmptyHash;
    other.m_next_indices = m_next_indices;
    return *this;
}

inline void HashTable::Clear() const {
    // 切断从桶到链表的访问入口
    if (m_index_size) {
        std::memset(m_hashes, 0xff, m_hash_size * sizeof(uint32));
    }
}

inline void HashTable::Free() {
    if (m_index_size) {
        m_hash_mask  = 0;
        m_index_size = 0;

        delete[] m_hashes;
        m_hashes = EmptyHash;

        delete[] m_next_indices;
        m_next_indices = nullptr;
    }
}

inline void HashTable::Resize(uint32 new_index_size) {
    if (new_index_size == m_index_size) return;
    if (new_index_size == 0 || m_index_size == 0) return;

    uint32* new_nex_index = new uint32[new_index_size];
    if (m_next_indices) {
        std::memcpy(new_nex_index, m_next_indices, m_index_size * sizeof(uint32));
        delete[] m_next_indices;
    }

    m_index_size   = new_index_size;
    m_next_indices = new_nex_index;
}

// 返回key对应的链表的第一个索引
inline uint32 HashTable::First(uint32 key) const {
    key &= m_hash_mask;
    return m_hashes[key];
}

// 返回链表当前索引后的下一个索引
inline uint32 HashTable::Next(uint32 index) const {
    CHECK(index < m_index_size);
    // 避免链表节点自引用导致的无限循环
    CHECK(m_next_indices[index] != index);
    return m_next_indices[index];
}

inline bool HashTable::IsValid(uint32 index) const {
    return index != ~0u;
}

// key决定元素放入哪个桶，index决定数据在外部数组中的索引位置，HashTable本身不存储数据，只存储索引
inline void HashTable::Add(uint32 key, uint32 index) {
    // 如果提供的索引超出当前哈希表容量，动态扩容
    if (index >= m_index_size) {
        // 新容量取32和(index+1)向上取整到2的幂中的较大值
        Resize(std::max<uint32>(32u, ((index + 1) <= 1) ? 1u : std::bit_ceil(static_cast<uint32>(index + 1))));
    }

    // 等价于 key % m_HashSize
    key &= m_hash_mask;

    // 使用头插法处理哈希冲突，构建链表
    // m_Hash[key]存储的是头节点的索引
    m_next_indices[index] = m_hashes[key];
    // 更新链表头为新添加的元素，从m_Hash[key]可以访问到整个链表上的所有元素:
    m_hashes[key] = index;
}

inline void HashTable::AddConcurrent(uint32 key, uint32 index) const {
    CHECK(index < m_index_size);

    key &= m_hash_mask;
    // 使用原子交换操作实现线程安全的头插法
    m_next_indices[index] = std::atomic_exchange( // 将一个原子对象的值替换为新值并返回替换前的旧值
        reinterpret_cast<std::atomic<uint32>*>(&m_hashes[key]), // 将m_Hash[key]视为原子变量
        index // 原子地读取m_Hash[key]的当前值，并将其替换为新的index
    ); // 将原始值设置为新元素的next指针
}

inline void HashTable::Remove(uint32 key, uint32 index) const {
    if (index >= m_index_size) {
        return;
    }

    key &= m_hash_mask;

    // 如果index正好是该key桶的头节点
    if (m_hashes[key] == index) {
        // 将后一个节点设为头节点即可
        m_hashes[key] = m_next_indices[index];
        return;
    }

    // 从头节点开始遍历
    for (uint32 i = m_hashes[key]; IsValid(i); i = m_next_indices[i]) {
        if (m_next_indices[i] == index) // 找到该节点
        {
            m_next_indices[i] = m_next_indices[index]; // 指向下下一个节点即可
            break;
        }
    }
}