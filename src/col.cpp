// SPDX-License-Identifier: MIT
// src/col.cpp — COL2 collision archive reader implementation
#include <sasdk/col.hpp>
#include <cstring>
#include <algorithm>

namespace sasdk {

namespace {

uint16_t read_u16(const uint8_t* p) noexcept { uint16_t v; std::memcpy(&v, p, 2); return v; }
uint32_t read_u32(const uint8_t* p) noexcept { uint32_t v; std::memcpy(&v, p, 4); return v; }

bool in_bounds(const uint8_t* base, size_t base_sz, const uint8_t* ptr, size_t need) noexcept {
    if (ptr < base) return false;
    size_t off = static_cast<size_t>(ptr - base);
    return off + need <= base_sz;
}

} // namespace

// ---------------------------------------------------------------------------
// ColArchive::parse
//   Walks the file as a sequence of model entries.
//   COL2 entry layout (32-byte envelope + body):
//     [0..3]   magic "COL2" or "COLL"
//     [4..7]   body_size uint32  (bytes after this envelope)
//     [8..29]  name[22]
//     [30..31] model_id uint16
//     [32..]   body_size bytes of binary data
//
//   Body structure (COL2):
//     [0..23]  bounding box  — CColBox (min+max float[3] each = 24B, no surface)
//     [24..39] bounding sphere — center float[3] + radius float = 16B
//     [40]     num_spheres   uint16
//     [42]     num_boxes     uint16
//     [44]     num_triangles uint16
//     [46]     num_lines     uint8
//     [47]     flags         uint8
//     [48..]   sphere data    [num_spheres   * kColSphereSize]
//               box data      [num_boxes     * kColBoxSize]
//               line data     [num_lines     * kColLineSize]
//               triangle data [num_triangles * kColTriangleSize]
// ---------------------------------------------------------------------------
Result<ColArchive> ColArchive::parse(const uint8_t* data, size_t size) noexcept {
    if (!data || size < 8)
        return err(ErrorKind::BadFormat, "col: file too small");

    ColArchive ca;
    size_t pos = 0;

    while (pos + 8 <= size) {
        const uint8_t* ep = data + pos;

        bool is_col2 = (std::memcmp(ep, "COL2", 4) == 0);
        bool is_col1 = (std::memcmp(ep, "COLL", 4) == 0);
        if (!is_col2 && !is_col1) {
            // Not a recognised entry — skip byte-by-byte to resync
            ++pos;
            continue;
        }

        uint32_t body_size = read_u32(ep + 4);
        size_t entry_end = pos + 8 + body_size;
        if (entry_end > size)
            return err(ErrorKind::BadFormat, "col: entry body_size overruns file");

        if (body_size < 32)
            return err(ErrorKind::BadFormat, "col: entry body too small for name/id");

        const uint8_t* body = ep + 8 + 22 + 2; // skip name[22] + model_id[2]

        ColModel m{};
        std::memcpy(m.name, ep + 8, 22);
        m.model_id = read_u16(ep + 8 + 22);
        m.is_col2  = is_col2;

        if (is_col2) {
            // 24B bounding box + 16B bounding sphere + 8B counts = 48B before geometry
            static constexpr size_t kCountsOff = 24 + 16; // 40
            if (body_size < kCountsOff + 8)
                return err(ErrorKind::BadFormat, "col: COL2 body too small for counts");

            m.num_spheres   = read_u16(body + kCountsOff + 0);
            m.num_boxes     = read_u16(body + kCountsOff + 2);
            m.num_triangles = read_u16(body + kCountsOff + 4);
            m.num_lines     = body[kCountsOff + 6];
            m.flags         = body[kCountsOff + 7];

            const uint8_t* geom = body + kCountsOff + 8;
            size_t avail = static_cast<size_t>((ep + 8 + body_size) - geom);
            size_t need =
                m.num_spheres   * kColSphereSize   +
                m.num_boxes     * kColBoxSize       +
                m.num_lines     * kColLineSize      +
                m.num_triangles * kColTriangleSize;

            if (avail < need)
                return err(ErrorKind::BadFormat, "col: geometry arrays overrun body");

            size_t off = 0;
            m.sphere_data   = {geom + off, m.num_spheres   * kColSphereSize};   off += m.sphere_data.size();
            m.box_data      = {geom + off, m.num_boxes     * kColBoxSize};       off += m.box_data.size();
            m.line_data     = {geom + off, m.num_lines     * kColLineSize};      off += m.line_data.size();
            m.triangle_data = {geom + off, m.num_triangles * kColTriangleSize};
        }
        // COLL (COL1) geometry is handled per-model by callers; we populate name/id only.

        ca.models_.push_back(m);
        pos = entry_end;
    }

    if (ca.models_.empty())
        return err(ErrorKind::BadFormat, "col: no models found");

    return ca;
}

const ColModel* ColArchive::find(std::string_view name) const noexcept {
    for (const ColModel& m : models_)
        if (m.model_name() == name) return &m;
    return nullptr;
}

const ColModel* ColArchive::find(uint16_t model_id) const noexcept {
    for (const ColModel& m : models_)
        if (m.model_id == model_id) return &m;
    return nullptr;
}

} // namespace sasdk
