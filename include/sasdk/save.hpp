// SPDX-License-Identifier: MIT
// include/sasdk/save.hpp — GTA:SA save file block reader (sa10us offline, SASDK::data)
//
// Save file format (save_format_structure.json, all fields verified_by_disassembly):
//   Flat binary, ~202 KB, no global file header.
//   Sequential blocks:
//     "BLOCK "[7B null-padded to fixed label size?]  — actually: tag "BLOCK " (6B)
//     block_size uint32                               — bytes of block body following
//     [block_size bytes of block body]
//   No padding between blocks.
//   Additive checksum covers all block bodies.
//
// Block index (from save_format_structure.json block_index):
//   0   Game header / player name
//   1   Simple variables
//   2   Garages
//   3   Vehicle pool
//   4   Stats (CStats 343 int32 entries)
//   5   Streaming state
//   6   Player peds
//   7   Vehicles
//   8   Objects
//   9   Paths
//   10  Cranes
//   11  Pickups
//   ... (to block 29+)
#pragma once
#include <sasdk/core/result.hpp>
#include <cstdint>
#include <string_view>
#include <vector>
#include <span>
#include <optional>

namespace sasdk {

struct SaveBlock {
    uint32_t               index;    // zero-based sequential index
    std::span<const uint8_t> data;   // raw block body (points into arena)
};

// -----------------------------------------------------------------------
// SaveArchive — iterates blocks in one GTA:SA save file loaded into memory.
// -----------------------------------------------------------------------
class SaveArchive {
public:
    // Parse the save file and index all blocks.
    static Result<SaveArchive> parse(const uint8_t* data, size_t size) noexcept;

    const std::vector<SaveBlock>& blocks() const noexcept { return blocks_; }
    size_t block_count() const noexcept { return blocks_.size(); }

    // Return block by index, or nullopt.
    std::optional<SaveBlock> block(uint32_t index) const noexcept;

    // Additive checksum of all block bodies (verify integrity).
    uint32_t checksum() const noexcept { return checksum_; }

private:
    SaveArchive() = default;
    std::vector<SaveBlock> blocks_;
    uint32_t               checksum_{};
};

} // namespace sasdk
