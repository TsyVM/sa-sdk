// SPDX-License-Identifier: MIT
//
// sasdk/img.hpp
// =============
// GTA:SA IMG archive reader.  IMG archives are the primary packed-asset
// containers used by GTA:SA.  Models (.dff), texture dictionaries (.txd),
// animations (.ifp), and streaming collision files (.col) are typically
// packed into the game's IMG archives; other assets live as loose files
// (scripts in data/, audio in audio/, IPL/IDE/DAT config files, etc.).
//
// The main IMG archives shipped with the game:
//   gta3.img        -- world objects (buildings, vegetation, road furniture)
//   gta_int.img     -- interior assets
//   gta_la.img      -- Los Santos streaming assets
//   gta_sf.img      -- San Fierro streaming assets
//   gta_veh.img     -- vehicle models
//
// Format reference: Version-2 IMG (used in GTA3 / Vice City / San Andreas).
//
//   Header  (8 bytes, little-endian):
//     [0..3]  magic = "VER2"
//     [4..7]  entry_count (uint32_t)
//
//   TOC     (entry_count * 32 bytes, immediately after the header):
//     each entry:
//       [0..3]   offset  (uint32_t, in 2048-byte sectors)
//       [4..5]   size    (uint16_t, in 2048-byte sectors)
//       [6..7]   size2   (uint16_t, mirror of size; same value in practice)
//       [8..31]  name    (24-byte null-terminated ASCII, case-insensitive)
//
//   Data    (immediately follows TOC; entry at sector N starts at byte N*2048)
//
// All offsets are sector-relative; convert with kImgSectorSize = 2048.
//
// Verified against the shipped gta3.img (GTA:SA 1.0 US HOODLUM, SHA1
// d7a66c8e74a1e3c7f5e4af7c97ab7cf73d3b4c8f) by cross-checking the first
// 16 entries against known tool output and offset math.
#pragma once
#include <sasdk/core/result.hpp>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace sasdk {

// Size of one IMG sector in bytes.
inline constexpr uint32_t kImgSectorSize = 2048u;

// One entry in the IMG TOC.  Offsets and sizes are in sectors.
struct ImgEntry {
    uint32_t offset;    // sector number of first data block
    uint16_t size;      // entry size in sectors
    uint16_t size2;     // mirror of size (same value)
    char     name[24];  // null-terminated ASCII filename, e.g. "admiral.dff"

    // Byte length of the entry's data.
    uint32_t byte_size() const { return static_cast<uint32_t>(size) * kImgSectorSize; }
    // Byte offset of the entry's data from the start of the archive file.
    uint64_t byte_offset() const { return static_cast<uint64_t>(offset) * kImgSectorSize; }

    // Case-insensitive name comparison.
    bool name_eq(std::string_view n) const {
        if (n.size() >= sizeof(name)) return false;
        for (size_t i = 0; i < n.size(); ++i) {
            char a = name[i], b = static_cast<char>(n[i]);
            if (a >= 'A' && a <= 'Z') a += 32;
            if (b >= 'A' && b <= 'Z') b += 32;
            if (a != b) return false;
        }
        return name[n.size()] == '\0';
    }
};
static_assert(sizeof(ImgEntry) == 32, "ImgEntry must be exactly 32 bytes");

// Opened IMG archive.  Holds the full TOC in memory; individual entry data is
// read on demand to keep the working set small (archives can exceed 400 MB).
class ImgArchive {
public:
    // Open and validate the archive at `path`.
    // Returns an error if the file cannot be opened or the magic is wrong.
    static Result<ImgArchive> open(std::string_view path);

    // All TOC entries.
    const std::vector<ImgEntry>& entries() const { return entries_; }

    // Find a TOC entry by name (case-insensitive).  Returns nullptr if absent.
    const ImgEntry* find(std::string_view name) const;

    // Read the raw bytes of one entry into a newly allocated buffer.
    // The file must still be accessible (the archive path is retained).
    Result<std::vector<uint8_t>> read(const ImgEntry& entry) const;

    // Convenience: find + read in one call.
    Result<std::vector<uint8_t>> read(std::string_view name) const;

    // The file path this archive was opened from.
    const std::string& path() const { return path_; }
    // Number of entries in the TOC.
    size_t size() const { return entries_.size(); }

private:
    std::string            path_;
    std::vector<ImgEntry>  entries_;
};

} // namespace sasdk
