// SPDX-License-Identifier: MIT
// include/sasdk/rw.hpp — RenderWare chunk stream reader (sa10us offline, SASDK::data)
//
// RenderWare binary stream format used by DFF (RpClump), TXD (RwTexDictionary),
// and IFP (RwHAnimAnimation).
//
// Chunk header (12 bytes, always):
//   type    uint32  — chunk type ID
//   size    uint32  — byte count of THIS chunk's payload (not counting this 12B header)
//   version uint32  — high 16 bits: library version, low 16 bits: build number
//
// Chunks are nested: a parent chunk's payload is itself a sequence of child chunks.
// Leaf data lives in a Struct chunk (type 0x0001) as the first child of most objects.
//
// Root chunk IDs for GTA:SA asset files:
//   0x0010 (RwGeometry)         — sub-chunk in DFF
//   0x001C (RpClump)            — DFF root
//   0x0021 (RwTexDictionary)    — TXD root
//   0x001B (RwHAnimAnimation)   — IFP animation root
// See RwChunkId enum below for the full set.
#pragma once
#include <sasdk/core/result.hpp>
#include <cstdint>
#include <string_view>
#include <vector>
#include <functional>
#include <optional>
#include <span>

namespace sasdk {

enum class RwChunkId : uint32_t {
    Struct          = 0x0001,
    String          = 0x0002,
    Extension       = 0x0003,
    Camera          = 0x0005,
    Texture         = 0x0006,
    Material        = 0x0007,
    MaterialList    = 0x0008,
    AtomicSection   = 0x0009,
    PlaneSection    = 0x000A,
    World           = 0x000B,
    Spline          = 0x000C,
    Matrix          = 0x000D,
    FrameList       = 0x000E,
    Geometry        = 0x0010,
    Clump           = 0x001C, // DFF root
    Light           = 0x0012,
    UnicodeString   = 0x0013,
    Atomic          = 0x0014,
    TextureNative   = 0x0015,
    TexDictionary   = 0x0021, // TXD root
    GeometryList    = 0x001A,
    HAnimAnimation  = 0x001B, // IFP root
    Team            = 0x001E,
    Crowd           = 0x001F,
    RightToRender   = 0x001F,
    MorphPLG        = 0x0105,
    HAnim           = 0x011E, // skeleton plugin chunk (inside Extension)
    UserDataPLG     = 0x011F,
    MatFXPLG        = 0x0120, // material effect plugin
    SkinPLG         = 0x0116, // skin2 plugin
    BinMeshPLG      = 0x050E, // index buffer organization
    NativeDataPLG   = 0x0510,
    Unknown         = 0xFFFFFFFF,
};

// ---------------------------------------------------------------------------
// RwChunk — a parsed but not-yet-decoded chunk.  The payload span points
// into the original arena; the arena must outlive all RwChunk instances.
// ---------------------------------------------------------------------------
struct RwChunk {
    RwChunkId              type;
    uint32_t               rw_version;  // full 32-bit version field
    std::span<const uint8_t> payload;   // raw bytes (excludes the 12B header)

    bool is(RwChunkId id) const noexcept { return type == id; }

    // Parse payload as a sequence of child chunks.
    Result<std::vector<RwChunk>> children() const noexcept;

    // Return the first child with the given type, or nullopt.
    std::optional<RwChunk> first_child(RwChunkId id) const noexcept;

    // Return the Struct child (always the first child for typed objects).
    std::optional<RwChunk> struct_data() const noexcept {
        return first_child(RwChunkId::Struct);
    }
};

// ---------------------------------------------------------------------------
// RwStream — reads a sequence of top-level chunks from a byte buffer.
// -----------------------------------------------------------------------
class RwStream {
public:
    // Parse the first top-level chunk (DFF/TXD/IFP files contain exactly one).
    static Result<RwChunk> read_root(const uint8_t* data, size_t size) noexcept;

    // Iterate all top-level chunks (useful for embedded multi-chunk buffers).
    // Callback returns false to stop early.
    static Status for_each(const uint8_t* data, size_t size,
                            std::function<bool(const RwChunk&)> fn) noexcept;

    // Convenience: read all top-level chunks.
    static Result<std::vector<RwChunk>> read_all(const uint8_t* data, size_t size) noexcept;
};

// ---------------------------------------------------------------------------
// Chunk type name (for debugging / logging).
// ---------------------------------------------------------------------------
std::string_view rw_chunk_name(RwChunkId id) noexcept;

} // namespace sasdk
