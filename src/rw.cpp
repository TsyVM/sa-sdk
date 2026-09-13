// SPDX-License-Identifier: MIT
// src/rw.cpp — RenderWare chunk stream reader implementation
#include <sasdk/rw.hpp>
#include <cstring>

namespace sasdk {

namespace {

uint32_t read_u32(const uint8_t* p) noexcept { uint32_t v; std::memcpy(&v, p, 4); return v; }

// Parse one chunk header at `pos` inside [data, data+size).
// On success, advances pos past the header+payload.
Result<RwChunk> parse_one(const uint8_t* data, size_t size, size_t& pos) noexcept {
    static constexpr size_t kHdrSize = 12;
    if (pos + kHdrSize > size)
        return err(ErrorKind::BadFormat, "rw: truncated chunk header");

    const uint8_t* hdr = data + pos;
    RwChunk c{};
    c.type       = static_cast<RwChunkId>(read_u32(hdr + 0));
    uint32_t sz  = read_u32(hdr + 4);
    c.rw_version = read_u32(hdr + 8);

    if (pos + kHdrSize + sz > size)
        return err(ErrorKind::BadFormat, "rw: chunk payload overruns buffer");

    c.payload = {hdr + kHdrSize, sz};
    pos += kHdrSize + sz;
    return c;
}

} // namespace

// ---------------------------------------------------------------------------
// RwStream
// ---------------------------------------------------------------------------
Result<RwChunk> RwStream::read_root(const uint8_t* data, size_t size) noexcept {
    if (!data || size < 12)
        return err(ErrorKind::BadFormat, "rw: buffer too small");
    size_t pos = 0;
    return parse_one(data, size, pos);
}

Status RwStream::for_each(const uint8_t* data, size_t size,
                           std::function<bool(const RwChunk&)> fn) noexcept {
    if (!data)
        return err(ErrorKind::BadFormat, "rw: null buffer");
    size_t pos = 0;
    while (pos < size) {
        size_t before = pos;
        auto r = parse_one(data, size, pos);
        if (!r) return r.error();
        if (!fn(*r)) break;
        if (pos == before) break; // safety: no progress
    }
    return {};
}

Result<std::vector<RwChunk>> RwStream::read_all(const uint8_t* data, size_t size) noexcept {
    std::vector<RwChunk> out;
    auto s = for_each(data, size, [&](const RwChunk& c) {
        out.push_back(c);
        return true;
    });
    if (!s) return s.error();
    return out;
}

// ---------------------------------------------------------------------------
// RwChunk::children / first_child
// ---------------------------------------------------------------------------
Result<std::vector<RwChunk>> RwChunk::children() const noexcept {
    return RwStream::read_all(payload.data(), payload.size());
}

std::optional<RwChunk> RwChunk::first_child(RwChunkId id) const noexcept {
    std::optional<RwChunk> found;
    RwStream::for_each(payload.data(), payload.size(), [&](const RwChunk& c) {
        if (c.type == id) { found = c; return false; }
        return true;
    });
    return found;
}

// ---------------------------------------------------------------------------
// rw_chunk_name
// ---------------------------------------------------------------------------
std::string_view rw_chunk_name(RwChunkId id) noexcept {
    switch (id) {
    case RwChunkId::Struct:         return "Struct";
    case RwChunkId::String:         return "String";
    case RwChunkId::Extension:      return "Extension";
    case RwChunkId::Camera:         return "Camera";
    case RwChunkId::Texture:        return "Texture";
    case RwChunkId::Material:       return "Material";
    case RwChunkId::MaterialList:   return "MaterialList";
    case RwChunkId::AtomicSection:  return "AtomicSection";
    case RwChunkId::PlaneSection:   return "PlaneSection";
    case RwChunkId::World:          return "World";
    case RwChunkId::FrameList:      return "FrameList";
    case RwChunkId::Geometry:       return "Geometry";
    case RwChunkId::Clump:          return "Clump";
    case RwChunkId::Light:          return "Light";
    case RwChunkId::Atomic:         return "Atomic";
    case RwChunkId::TextureNative:  return "TextureNative";
    case RwChunkId::TexDictionary:  return "TexDictionary";
    case RwChunkId::GeometryList:   return "GeometryList";
    case RwChunkId::HAnimAnimation: return "HAnimAnimation";
    case RwChunkId::HAnim:          return "HAnim";
    case RwChunkId::SkinPLG:        return "SkinPLG";
    case RwChunkId::MatFXPLG:       return "MatFXPLG";
    case RwChunkId::BinMeshPLG:     return "BinMeshPLG";
    case RwChunkId::NativeDataPLG:  return "NativeDataPLG";
    default:                        return "Unknown";
    }
}

} // namespace sasdk
