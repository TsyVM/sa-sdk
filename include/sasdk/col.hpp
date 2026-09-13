// SPDX-License-Identifier: MIT
// include/sasdk/col.hpp — COL2 collision archive reader (sa10us offline, SASDK::data)
//
// COL2 file format (multi-model .col files used by GTA:SA):
//   Sequence of model entries, each:
//     magic[4]     "COL2" (or "COLL" for legacy COL1)
//     body_size[4] bytes following this 8-byte envelope
//     name[22]     NUL-padded ASCII model name
//     model_id[2]  uint16 matching a model pool slot
//     [body_size bytes of binary geometry data]
//
// CCollisionData in-memory layout (LoadColVer2 @0x537300, all fields verified_by_disassembly):
//   +0x00  m_nNumSpheres    uint16
//   +0x02  m_nNumBoxes      uint16
//   +0x04  m_nNumTriangles  uint16
//   +0x06  m_nNumLines      uint8
//   +0x07  m_nFlags         uint8
//   +0x08  m_pSpheres       ptr32
//   +0x0C  m_pBoxes         ptr32
//   +0x10  m_pLines         ptr32
//   +0x14  m_pTriangles     ptr32
//   +0x18  m_pTrianglePlanes ptr32
//   +0x1C  m_pSuspectLines  ptr32
//   +0x20  m_nNumSuspectLines uint32
//   +0x24  m_pShadowMesh    ptr32
//   +0x28  m_nAllocatedCount uint32
//   +0x2C  m_pDataBuffer    ptr32
//   +0x30  [geometry arrays packed inline — spheres, boxes, triangles]
#pragma once
#include <sasdk/core/result.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <span>

namespace sasdk {

// Geometry element sizes (COL2 on-disk, verified against LoadColVer2 read strides):
//   CColSphere:   center(12B) + radius(4B) + surface_type(1B) + lighting(1B) = 18B
//   CColBox:      min(12B) + max(12B) + surface_type(1B) + lighting(1B) = 26B
//   CColTriangle: vi[3](3*2B) + surface_type(1B) + lighting(1B) = 8B
//   CColLine:     start(12B) + end(12B) = 24B
static constexpr size_t kColSphereSize   = 18;
static constexpr size_t kColBoxSize      = 26;
static constexpr size_t kColTriangleSize = 8;
static constexpr size_t kColLineSize     = 24;

// One model entry parsed from a .col file.
struct ColModel {
    char     name[22];    // NUL-padded ASCII
    uint16_t model_id;
    bool     is_col2;     // true=COL2, false=legacy COLL

    uint16_t num_spheres;
    uint16_t num_boxes;
    uint16_t num_triangles;
    uint8_t  num_lines;
    uint8_t  flags;

    // Raw byte spans into the arena (caller must keep arena alive).
    std::span<const uint8_t> sphere_data;
    std::span<const uint8_t> box_data;
    std::span<const uint8_t> line_data;
    std::span<const uint8_t> triangle_data;

    std::string_view model_name() const noexcept {
        return {name, strnlen(name, sizeof(name))};
    }
};

// -----------------------------------------------------------------------
// ColArchive — enumerates all models in one .col file loaded into memory.
// The backing data must outlive the ColArchive.
// -----------------------------------------------------------------------
class ColArchive {
public:
    static Result<ColArchive> parse(const uint8_t* data, size_t size) noexcept;

    const std::vector<ColModel>& models() const noexcept { return models_; }
    size_t size() const noexcept { return models_.size(); }

    // Find by model name (case-sensitive, matches NUL-padded name field).
    const ColModel* find(std::string_view name) const noexcept;

    // Find by model_id.
    const ColModel* find(uint16_t model_id) const noexcept;

private:
    ColArchive() = default;
    std::vector<ColModel> models_;
};

} // namespace sasdk
