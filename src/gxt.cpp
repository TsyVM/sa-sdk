// SPDX-License-Identifier: MIT
// src/gxt.cpp — GXT archive reader implementation
#include <sasdk/gxt.hpp>
#include <algorithm>
#include <cstring>

namespace sasdk {

// ---------------------------------------------------------------------------
// CRC-32 reflected, poly 0xEDB88320, init 0xFFFFFFFF, NO final complement.
// GXT key hash = crc32_no_final(uppercase(key)).
// Reference: gxt_structure.json "key_hash" section; 99.5% SCM-match confirmed.
// ---------------------------------------------------------------------------
uint32_t gxt_key_hash(std::string_view key) noexcept {
    uint32_t crc = 0xFFFFFFFFu;
    for (unsigned char c : key) {
        crc ^= static_cast<uint8_t>(c >= 'a' && c <= 'z' ? c - 32 : c);
        for (int i = 0; i < 8; ++i)
            crc = (crc >> 1) ^ (0xEDB88320u * (crc & 1u));
    }
    return crc; // no ^ 0xFFFFFFFF — GXT uses pre-complement CRC
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

bool bounds_check(const uint8_t* base, size_t base_size,
                  const uint8_t* ptr, size_t need) noexcept {
    if (ptr < base) return false;
    size_t off = static_cast<size_t>(ptr - base);
    return off + need <= base_size;
}

uint16_t read_u16(const uint8_t* p) noexcept {
    uint16_t v; std::memcpy(&v, p, 2); return v;
}
uint32_t read_u32(const uint8_t* p) noexcept {
    uint32_t v; std::memcpy(&v, p, 4); return v;
}

} // namespace

// ---------------------------------------------------------------------------
// GxtArchive::parse
// ---------------------------------------------------------------------------
Result<GxtArchive> GxtArchive::parse(const uint8_t* data, size_t size) noexcept {
    if (!data || size < 4)
        return err(ErrorKind::BadFormat, "gxt: file too small");

    GxtArchive g(data, size);
    g.version_word_  = read_u16(data + 0);
    g.bits_per_char_ = read_u16(data + 2);

    if (g.version_word_ != 4)
        return err(ErrorKind::BadFormat, "gxt: unexpected version_word (want 4)");

    // TABL block immediately follows header
    const uint8_t* tabl = data + 4;
    if (!bounds_check(data, size, tabl, 8))
        return err(ErrorKind::BadFormat, "gxt: truncated TABL header");
    if (std::memcmp(tabl, "TABL", 4) != 0)
        return err(ErrorKind::BadFormat, "gxt: missing TABL tag");

    uint32_t tabl_size = read_u32(tabl + 4);
    if (tabl_size % 12 != 0)
        return err(ErrorKind::BadFormat, "gxt: TABL size not multiple of 12");
    if (!bounds_check(data, size, tabl + 8, tabl_size))
        return err(ErrorKind::BadFormat, "gxt: TABL entries out of bounds");

    uint32_t n_tables = tabl_size / 12;
    g.tables_.reserve(n_tables);
    g.index_.reserve(n_tables);

    for (uint32_t i = 0; i < n_tables; ++i) {
        const uint8_t* ep = tabl + 8 + i * 12;
        GxtTableEntry te{};
        std::memcpy(te.name, ep, 8);
        te.offset = read_u32(ep + 8);
        g.tables_.push_back(te);

        auto ti = g.index_table(te);
        if (!ti) return err(ti.error().kind, ti.error().message);
        g.index_.push_back(*ti);
    }

    return g;
}

// ---------------------------------------------------------------------------
// Index a single table's TKEY+TDAT blocks
// ---------------------------------------------------------------------------
Result<GxtArchive::TableIndex> GxtArchive::index_table(const GxtTableEntry& te) const noexcept {
    if (!bounds_check(data_, size_, data_ + te.offset, 8))
        return err(ErrorKind::BadFormat, "gxt: TKEY offset out of bounds");

    const uint8_t* tkey = data_ + te.offset;
    if (std::memcmp(tkey, "TKEY", 4) != 0)
        return err(ErrorKind::BadFormat, "gxt: missing TKEY tag");

    uint32_t tkey_size = read_u32(tkey + 4);
    if (tkey_size % 8 != 0)
        return err(ErrorKind::BadFormat, "gxt: TKEY size not multiple of 8");
    if (!bounds_check(data_, size_, tkey + 8, tkey_size))
        return err(ErrorKind::BadFormat, "gxt: TKEY entries out of bounds");

    uint32_t key_count = tkey_size / 8;
    const GxtKeyEntry* keys = reinterpret_cast<const GxtKeyEntry*>(tkey + 8);

    const uint8_t* tdat = tkey + 8 + tkey_size;
    if (!bounds_check(data_, size_, tdat, 8))
        return err(ErrorKind::BadFormat, "gxt: TDAT header out of bounds");
    if (std::memcmp(tdat, "TDAT", 4) != 0)
        return err(ErrorKind::BadFormat, "gxt: missing TDAT tag");

    uint32_t tdat_size = read_u32(tdat + 4);
    if (!bounds_check(data_, size_, tdat + 8, tdat_size))
        return err(ErrorKind::BadFormat, "gxt: TDAT data out of bounds");

    TableIndex ti{};
    ti.name       = std::string_view(te.name, std::min<size_t>(8, strnlen(te.name, 8)));
    ti.keys       = keys;
    ti.key_count  = key_count;
    ti.tdat_base  = reinterpret_cast<const char*>(tdat + 8);
    return ti;
}

// ---------------------------------------------------------------------------
// find_table
// ---------------------------------------------------------------------------
const GxtArchive::TableIndex* GxtArchive::find_table(std::string_view name) const noexcept {
    for (const TableIndex& ti : index_) {
        if (ti.name == name) return &ti;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// lookup / lookup_hash
// ---------------------------------------------------------------------------
std::optional<std::string_view> GxtArchive::lookup(std::string_view table_name,
                                                    std::string_view key_name) const noexcept {
    return lookup_hash(table_name, gxt_key_hash(key_name));
}

std::optional<std::string_view> GxtArchive::lookup_hash(std::string_view table_name,
                                                         uint32_t hash) const noexcept {
    const TableIndex* t = find_table(table_name);
    if (!t) return std::nullopt;

    // Binary search: TKEY entries are sorted ascending by key_hash
    uint32_t lo = 0, hi = t->key_count;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        if (t->keys[mid].key_hash == hash)
            return std::string_view{t->tdat_base + t->keys[mid].tdat_offset};
        else if (t->keys[mid].key_hash < hash) lo = mid + 1;
        else                                   hi = mid;
    }
    return std::nullopt;
}

} // namespace sasdk
