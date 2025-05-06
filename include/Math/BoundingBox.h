﻿#pragma once

#include "VectorMath.h"

//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard
//

#pragma once

#include "VectorMath.h"

namespace Math {
class AxisAlignedBox {
public:
    AxisAlignedBox():
        m_min(FLT_MAX, FLT_MAX, FLT_MAX),
        m_max(-FLT_MAX, -FLT_MAX, -FLT_MAX) {
    }

    AxisAlignedBox(Vector3 min, Vector3 max):
        m_min(min),
        m_max(max) {
    }

    void AddPoint(Vector3 point) {
        m_min = Vector3::Min(point, m_min);
        m_max = Vector3::Max(point, m_max);
    }

    void AddBoundingBox(const AxisAlignedBox& box) {
        AddPoint(box.m_min);
        AddPoint(box.m_max);
    }

    AxisAlignedBox Union(const AxisAlignedBox& box) {
        return AxisAlignedBox(Vector3::Min(m_min, box.m_min), Vector3::Max(m_max, box.m_max));
    }

    Vector3 GetMin() const { return m_min; }
    Vector3 GetMax() const { return m_max; }
    Vector3 GetCenter() const { return (m_min + m_max) * 0.5f; }
    Vector3 GetDimensions() const { return Vector3::Max(m_max - m_min, Vector3()); }

private:
    Vector3 m_min;
    Vector3 m_max;
};
} // namespace Math


using Bounds3f = Math::AxisAlignedBox;