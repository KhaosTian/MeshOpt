#pragma once

#include "Common.h"
#include "VectorMath.h"
#include "Math/BoundingBox.h"

class Cluster {
public:
    Cluster() {}

public:
    Vector3f& GetPosition(uint32 vertIndex);
    Vector3f& GetNormal(uint32 vertIndex);
    Vector3f& GetUVs(uint32 vertIndex);
    Vector3f& GetColor(uint32 vertIndex);

    const Vector3f& GetPosition(uint32 vertIndex) const;
    const Vector3f& GetNormal(uint32 vertIndex) const;
    const Vector3f& GetUVs(uint32 vertIndex) const;
    const Vector3f& GetColor(uint32 vertIndex) const;

    static const uint16_t ClusterSize = 128;

    uint32 NumVerts = 0;
    uint32 NumTris  = 0;

    std::vector<float>  Verts;
    std::vector<uint32> Indexes;
    std::vector<int32>  MaterialIndexes;

    Bounds3f Bounds;
    uint64_t GUID     = 0;
    int32    MipLevel = 0;
};
