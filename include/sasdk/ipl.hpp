// SPDX-License-Identifier: MIT
// include/sasdk/ipl.hpp — GTA:SA item placement file parser (sa10us offline, SASDK::data)
//
// IPL files place model instances and define gameplay zones in the world.
// They live in data/maps/ (streaming areas use binary IPLs inside IMG files;
// this parser handles the text format shipped on disk).
//
// Sections (case-insensitive):
//   inst  — object placement   id, modelname, interior_id, x, y, z, qx, qy, qz, qw
//   cull  — culling zones      cx, cy, cz, unk1..7
//   grge  — garage zones       id, x, y, z, x_size, y_size, z_angle, door_type, flags, name
//   enex  — entry/exit points  ex, ey, ez, x_ang, x_sz, y_sz, ix, iy, iz, i_ang, int_id, sky_col, flags, name
//   pick  — item pickups       model_id, x, y, z
//   cars  — parked cars        x, y, z, angle, model_id, col1, col2, force_spawn, alarm, locks, unk1, unk2
//
// Line syntax: comma-separated, whitespace trimmed, # = comment.
#pragma once
#include <sasdk/core/result.hpp>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace sasdk {

// One placed object instance (inst).
struct IplInst {
    int32_t  id;             // model ID matching an IDE entry
    char     model_name[24]; // model name (redundant with id; present for readability)
    int32_t  interior_id;    // interior room index (0 = exterior / SA main world)
    float    x, y, z;        // world position
    float    qx, qy, qz, qw; // rotation quaternion (unit quaternion, ZXY convention)
};

// Culling zone (cull).
struct IplCull {
    float cx, cy, cz;     // zone centre
    float unk[7];          // zone extents and flags (format not fully published)
};

// Garage zone (grge).
struct IplGrge {
    int32_t  id;
    float    x, y, z;
    float    x_size, y_size;
    float    z_angle;
    uint8_t  door_type;
    uint8_t  flags;
    char     name[32];
};

// Entry/exit zone (enex).
struct IplEnex {
    float    ex, ey, ez;       // exterior entrance position
    float    x_angle;          // exterior facing angle (degrees)
    float    x_size, y_size;   // entrance marker extents
    float    ix, iy, iz;       // interior spawn position
    float    i_angle;          // interior facing angle
    int32_t  interior_id;
    uint32_t sky_color;        // packed RGBA sky override
    uint32_t flags;
    char     name[32];
};

// Pickup spawn (pick).
struct IplPick {
    int32_t model_id;
    float   x, y, z;
};

// Parked vehicle (cars).
struct IplCar {
    float    x, y, z;
    float    angle;
    int32_t  model_id;
    uint8_t  primary_color;
    uint8_t  secondary_color;
    uint8_t  force_spawn;
    uint8_t  alarm;
    uint8_t  locks;
    uint8_t  unk1, unk2;
};

// Parsed IPL file.
class IplArchive {
public:
    // Parse a complete text IPL file.
    static Result<IplArchive> parse(std::string_view text) noexcept;

    // Span access.
    std::span<const IplInst> instances()  const noexcept { return insts_; }
    std::span<const IplCull> culls()      const noexcept { return culls_; }
    std::span<const IplGrge> garages()    const noexcept { return grges_; }
    std::span<const IplEnex> entrances()  const noexcept { return enexs_; }
    std::span<const IplPick> pickups()    const noexcept { return picks_; }
    std::span<const IplCar>  cars()       const noexcept { return cars_; }

    // Find a placed instance by model ID.  Returns first match or nullptr.
    const IplInst* find_instance(int32_t model_id)          const noexcept;
    const IplInst* find_instance(std::string_view model_name) const noexcept;

    size_t total_entries() const noexcept {
        return insts_.size() + culls_.size() + grges_.size()
             + enexs_.size() + picks_.size() + cars_.size();
    }

private:
    IplArchive() = default;
    std::vector<IplInst> insts_;
    std::vector<IplCull> culls_;
    std::vector<IplGrge> grges_;
    std::vector<IplEnex> enexs_;
    std::vector<IplPick> picks_;
    std::vector<IplCar>  cars_;
};

} // namespace sasdk
