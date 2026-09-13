// SPDX-License-Identifier: MIT
// include/sasdk/gxt.hpp — GXT text archive reader (sa10us offline, SASDK::data)
//
// Binary format (american.gxt, etc.)
//   [header: 4B]  version_word=4 (uint16), bits_per_char=8 (uint16)
//   [TABL block]  "TABL" tag[4] + size[4] + N*{name[8] + offset[4]}
//   [per table at offset]
//     [TKEY block]  "TKEY" + size + M*{tdat_offset[4] + key_hash[4]}  sorted asc by hash
//     [TDAT block]  "TDAT" + size + NUL-terminated UTF-8 strings
//
// Key hash: CRC-32 reflected (poly 0xEDB88320, init 0xFFFFFFFF) WITHOUT final
//           complement, computed on the uppercased key name.
//           Confirmed: 99.5% of SCM text opcodes resolve correctly.
#pragma once
#include <sasdk/core/result.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <optional>

namespace sasdk {

// CRC-32 variant used by the GXT key hash (no final XOR complement).
uint32_t gxt_key_hash(std::string_view key) noexcept;

struct GxtTableEntry {
    char     name[8]; // NUL-padded 8-char table ID (e.g. "MAIN", "MISSION")
    uint32_t offset;  // byte offset from file start to this table's TKEY block
};

struct GxtKeyEntry {
    uint32_t tdat_offset; // byte offset into TDAT data section (after "TDAT"+size header)
    uint32_t key_hash;    // gxt_key_hash(uppercase_key_name)
};

// -----------------------------------------------------------------------
// GxtArchive — read-only view of one .gxt file loaded into memory.
// Load the whole file into a vector<uint8_t> and pass the span here.
// The data must outlive the GxtArchive.
// -----------------------------------------------------------------------
class GxtArchive {
public:
    // Validate the header and index the TABL block.  O(N_tables).
    static Result<GxtArchive> parse(const uint8_t* data, size_t size) noexcept;

    // List the table names present in this .gxt.
    const std::vector<GxtTableEntry>& tables() const noexcept { return tables_; }

    // Find a string by table name + key name.
    // Returns std::nullopt if the table or key does not exist.
    std::optional<std::string_view> lookup(std::string_view table_name,
                                           std::string_view key_name) const noexcept;

    // Find by pre-computed hash (avoids re-hashing inside tight loops).
    std::optional<std::string_view> lookup_hash(std::string_view table_name,
                                                uint32_t hash) const noexcept;

    // Enumerate every string in one table.  Returns false if table not found.
    // Callback: (key_hash, string_view) -> void
    template <typename Fn>
    bool for_each(std::string_view table_name, Fn&& fn) const noexcept;

    uint32_t version_word()   const noexcept { return version_word_; }
    uint32_t bits_per_char()  const noexcept { return bits_per_char_; }

private:
    GxtArchive(const uint8_t* data, size_t size)
        : data_(data), size_(size) {}

    const uint8_t*            data_;
    size_t                    size_;
    uint32_t                  version_word_{};
    uint32_t                  bits_per_char_{};
    std::vector<GxtTableEntry> tables_;

    struct TableIndex {
        std::string_view    name;
        const GxtKeyEntry*  keys;
        uint32_t            key_count;
        const char*         tdat_base; // start of TDAT string data (after "TDAT"+size)
    };

    std::vector<TableIndex>   index_;

    const TableIndex* find_table(std::string_view name) const noexcept;
    Result<TableIndex> index_table(const GxtTableEntry& te) const noexcept;
};

// -----------------------------------------------------------------------
// template impl
// -----------------------------------------------------------------------
template <typename Fn>
bool GxtArchive::for_each(std::string_view table_name, Fn&& fn) const noexcept {
    const TableIndex* t = find_table(table_name);
    if (!t) return false;
    for (uint32_t i = 0; i < t->key_count; ++i) {
        const GxtKeyEntry& e = t->keys[i];
        std::string_view s{t->tdat_base + e.tdat_offset};
        fn(e.key_hash, s);
    }
    return true;
}

} // namespace sasdk
