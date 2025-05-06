#pragma once

#include "Common.h"

template<typename InElementType, typename InSizeType = int32>
class StridedView;

template<typename T, typename SizeType = int32>
using ConstStridedView = StridedView<const T, SizeType>;

template<typename ElementType, typename SizeType>
class StridedView {
public:
    static_assert(std::is_signed<SizeType>::value, "StridedView only supports signed index types");

    StridedView() = default;

    template<typename OtherElementType>
        requires(std::is_convertible_v<OtherElementType*, ElementType*>)
    StridedView(SizeType stride, OtherElementType* address, SizeType num):
        m_address(address),
        m_stride(stride),
        m_num(num) {
        assert(m_num >= 0 && "Number of elements must be non-negative");
        assert(m_stride >= 0 && "Stride must be non-negative");
        assert(m_stride % alignof(ElementType) == 0 && "Stride must respect element alignment");
    }

    template<typename OtherElementType>
        requires(std::is_convertible_v<OtherElementType*, ElementType*>)
    StridedView(const StridedView<OtherElementType, SizeType>& other):
        m_address(nullptr),
        m_stride(other.Stride()),
        m_num(other.num()) {
        if (other.num() > 0) {
            m_address = &other[0];
        }
    }

    bool IsValidIndex(SizeType index) const { return (index >= 0) && (index < m_num); }
    bool IsEmpty() const { return m_num == 0; }

    SizeType Num() const { return m_num; }
    SizeType Stride() const { return m_stride; }

    ElementType& GetUnsafe(SizeType index) const { return *GetElementPtrUnsafe(index); }

    ElementType& operator[](SizeType index) const {
        RangeCheck(index);
        return *GetElementPtrUnsafe(index);
    }

    class Iterator {
    public:
        Iterator(const StridedView* owner, SizeType index): m_owner(owner), m_index(index) {}

        Iterator& operator++() {
            ++m_index;
            return *this;
        }

        ElementType& operator*() const { return m_owner->GetUnsafe(m_index); }

        bool operator==(const Iterator& other) const { return m_owner == other.m_owner && m_index == other.m_index; }
        bool operator!=(const Iterator& other) const { return !(*this == other); }

    private:
        const StridedView* m_owner;
        SizeType           m_index;
    };

    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, m_num); }

private:
    void RangeCheck(SizeType index) const { assert((index >= 0) && (index < m_num) && "Array index out of bounds"); }

    ElementType* GetElementPtrUnsafe(SizeType index) const {
        using ByteType = typename std::conditional<std::is_const<ElementType>::value, const uint8_t, uint8_t>::type;
        ByteType*    asBytes   = reinterpret_cast<ByteType*>(m_address);
        ElementType* asElement = reinterpret_cast<ElementType*>(
            asBytes + static_cast<std::size_t>(index) * static_cast<std::size_t>(m_stride)
        );
        return asElement;
    }

private:
    ElementType* m_address = nullptr;
    SizeType     m_stride  = 0;
    SizeType     m_num     = 0;
};

// helper functions for make strided view
template<typename ElementType>
StridedView<ElementType> MakeStridedView(int stride, ElementType* first_element, int count) {
    return StridedView<ElementType>(stride, first_element, count);
}

template<typename ContainerType, typename ElementType, typename StructType>
StridedView<ElementType> MakeStridedView(ContainerType& container, ElementType StructType::*member) {
    if (container.size() == 0) {
        return StridedView<ElementType>();
    }
    auto* data = &container[0];
    return StridedView<ElementType>(sizeof(StructType), &(data->*member), static_cast<int32>(container.size()));
}

template<typename ContainerType>
auto MakeStridedView(ContainerType& container) -> StridedView<std::remove_reference_t<decltype(container[0])>> {
    using ElementType = std::remove_reference_t<decltype(container[0])>;
    if (container.size() == 0) {
        return StridedView<ElementType>();
    }
    return StridedView<ElementType>(sizeof(ElementType), &container[0], static_cast<int32>(container.size()));
}

// for const strided view
template<typename ElementType>
StridedView<const ElementType> MakeConstStridedView(int stride, const ElementType* firstElement, int count) {
    return StridedView<const ElementType>(stride, firstElement, count);
}

template<typename ContainerType, typename ElementType, typename StructType>
ConstStridedView<ElementType> MakeConstStridedView(ContainerType& container, const ElementType StructType::*member) {
    if (container.size() == 0) {
        return ConstStridedView<ElementType>();
    }
    auto* data = &container[0];
    return ConstStridedView<ElementType>(sizeof(StructType), &(data->*member), static_cast<int32>(container.size()));
}

template<typename ContainerType>
auto MakeConstStridedView(const ContainerType& container
) -> ConstStridedView<std::remove_reference_t<decltype(container[0])>> {
    using ElementType = typename std::remove_reference<decltype(container[0])>::type;
    if (container.size() == 0) {
        return ConstStridedView<ElementType>();
    }
    return ConstStridedView<ElementType>(sizeof(ElementType), &container[0], static_cast<int32>(container.size()));
}