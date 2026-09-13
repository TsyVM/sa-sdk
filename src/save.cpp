// SPDX-License-Identifier: MIT
// src/save.cpp — GTA:SA save file block reader implementation
#include <sasdk/save.hpp>
#include <cstring>

namespace sasdk {

namespace {
uint32_t read_u32(const uint8_t* p) noexcept { uint32_t v; std::memcpy(&v, p, 4); return v; }
} // namespace

// ---------------------------------------------------------------------------
// SaveArchive::parse
//   Scans the file for "BLOCK " tags and extracts block bodies.
//   Format: "BLOCK " (6B) + block_size uint32 (4B) + body[block_size].
//   No global file header; blocks are contiguous with no inter-block padding.
// ---------------------------------------------------------------------------
Result<SaveArchive> SaveArchive::parse(const uint8_t* data, size_t size) noexcept {
    if (!data || size < 10)
        return err(ErrorKind::BadFormat, "save: file too small");

    SaveArchive sa;
    size_t pos = 0;
    uint32_t running_sum = 0;
    uint32_t index = 0;

    while (pos + 10 <= size) {
        const uint8_t* ep = data + pos;
        if (std::memcmp(ep, "BLOCK ", 6) != 0) {
            // Not a block tag — scan forward to resync (handles file header noise)
            ++pos;
            continue;
        }

        uint32_t block_size = read_u32(ep + 6);
        size_t body_start = pos + 10;
        if (body_start + block_size > size)
            return err(ErrorKind::BadFormat, "save: block body overruns file");

        const uint8_t* body = data + body_start;

        // Additive checksum over body bytes (as uint32 sum of all bytes)
        for (size_t i = 0; i < block_size; ++i)
            running_sum += body[i];

        SaveBlock blk{};
        blk.index = index++;
        blk.data  = {body, block_size};
        sa.blocks_.push_back(blk);

        pos = body_start + block_size;
    }

    if (sa.blocks_.empty())
        return err(ErrorKind::BadFormat, "save: no blocks found");

    sa.checksum_ = running_sum;
    return sa;
}

std::optional<SaveBlock> SaveArchive::block(uint32_t index) const noexcept {
    if (index < blocks_.size()) return blocks_[index];
    return std::nullopt;
}

} // namespace sasdk
