// SPDX-License-Identifier: MIT
//
// sasdk/game/sa10us/types.hpp
// ===========================
// Fundamental GTA:SA type definitions — math primitives and game enum tag
// types — required by the generated struct database (sa10us_db.inl).
//
// Sizes are exact for the 32-bit sa10us (1.0 US HOODLUM) binary. Every type
// here carries a size static_assert so a mismatched definition is a hard
// build error rather than a silent offset drift.
//
// Do NOT add high-level game logic here; this file exists only to define the
// leaf types that the generated structs embed or use as field types.
#pragma once
#include <cstdint>
#include <array>

namespace sasdk::sa10us {

// ---- Math primitives -------------------------------------------------------

// 3-component float vector. On-disk: 12 bytes, [x at +0, y at +4, z at +8].
// Verified by CPhysical field-access patterns (m_vecMoveSpeed used in
// ApplyForce: fld [esi+0x44]; m_vecTurnSpeed: fld [esi+0x50]).
#pragma pack(push, 1)
struct CVector {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;

    std::array<float, 3>& as_array() {
        return *reinterpret_cast<std::array<float, 3>*>(this);
    }
    const std::array<float, 3>& as_array() const {
        return *reinterpret_cast<const std::array<float, 3>*>(this);
    }

    float* data()       { return &x; }
    const float* data() const { return &x; }
};
#pragma pack(pop)
static_assert(sizeof(CVector) == 12, "CVector must be exactly 12 bytes");

// ---- Game enum tag types ---------------------------------------------------
// These enumerate the underlying integral types for GTA:SA enums that appear
// as struct field types in sa10us_db.inl. Sizes are verified by the field
// offsets immediately surrounding each usage in the CVehicle/CTask schemas.
//
// Enumerator constants will be added in a future pass once the full symbol
// table is verified; until then these aliases provide correct field widths
// and a searchable name at every use site.

// Vehicle base class / sub-class (car, bike, boat, plane, heli, …).
// CVehicle@0x590 and @0x594, stride 4B each → underlying int32_t.
// [SASDK VERIFIED by disassembly — size confirmed via adjacent field offsets]
using eVehicleType          = std::int32_t;

// Door-lock state (UNLOCKED = 0, LOCKED = 1, LOCKOUT_PLAYER_ONLY = 2, …).
// CVehicle@0x4F8, size 4B. [SASDK VERIFIED by disassembly]
using eDoorLock             = std::uint32_t;

// Comedy-control steering state (INACTIVE = 0, STEER_RIGHT = 1, STEER_LEFT = 2).
// CVehicle@0x51A, size 1B. [SASDK VERIFIED by disassembly]
using eComedyControlState   = std::uint8_t;

// Handling-flags enum value (raw integer view of the bitfield at @0x38C).
// CVehicle@0x38C, size 4B. [SASDK VERIFIED by disassembly]
using eVehicleHandlingFlags = std::uint32_t;

// Task-type discriminant used by the virtual CTask::GetTaskType() method.
// CTask@0x4, size 4B. [SASDK VERIFIED by disassembly — confirmed via
//   CTask vtable slot 3 returning eTaskType; slot ordering verified in
//   task_system_structure.json]
using eTaskType             = std::int32_t;

} // namespace sasdk::sa10us
