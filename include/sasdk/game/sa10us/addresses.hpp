// include/sasdk/game/sa10us/addresses.hpp
//
// Curated, provenance-tagged logical addresses for the sa10us (1.0 US HOODLUM)
// build. These are VAs against ImageBase 0x00400000, resolved through the
// Module slide at runtime. Every entry carries its confidence tier and source,
// matching the encyclopedia's discipline -- the SDK never launders a reasoned
// or external address into looking verified.
//
// This is a hand-curated seed, NOT the full 492-entry catalogue. It exists to
// make the runtime demonstrably usable today; a generated addresses table
// (functions.json -> gen) is the Phase-1 follow-up (see audit 15_SOURCE_OF_TRUTH).
#pragma once
#include <sasdk/core/address.hpp>
#include <cstdint>

namespace sasdk::sa10us::addr {

using sasdk::rt::Address;

// ---- Global singletons / arrays -------------------------------------------

// TheCamera (CCamera). VERIFIED cold: two independent field bases agree --
// m_nActiveCam base 0xB6F081 - 0x59 = 0xB6F028, and m_aCams base 0xB6F19C -
// 0x174 = 0xB6F028. (C39.)
inline constexpr Address TheCamera{0x00B6F028};

// CStreamingInfo array: 26,316 records, stride 20. VERIFIED (C2.2).
inline constexpr Address StreamingInfoArray{0x008E4CC0};
inline constexpr Address StreamingInfoLinkArray{0x009654B4};
inline constexpr std::uint32_t kStreamingInfoCount  = 26316;
inline constexpr std::uint32_t kStreamingInfoStride = 20;

// Wanted-system chaos ceiling global. REASONED (C41 / ai_structure).
inline constexpr Address MaxWantedChaosGlobal{0x008CDEE8};

// ---- Functions -------------------------------------------------------------
// Tiers: VERIFIED = disassembly spot-check in the function catalogue;
//        UNVERIFIED = catalogued, not independently confirmed by a disassembly pass;
//        REASONED = derived but not cold-proven here.

// CStreaming::RenderEntity  -- VERIFIED (function catalogue).
inline constexpr Address CStreaming_RenderEntity{0x004096D0};
// CStreaming::RemoveAllUnusedModels -- VERIFIED.
inline constexpr Address CStreaming_RemoveAllUnusedModels{0x0040CF80};
// CVehicle::Fix -- address 0x6D6390; the health-reset block at 0x6D603B that
// writes m_fHealth=1000.0f lives inside this function's body.
inline constexpr Address CVehicle_Fix{0x006D6390};
// CWanted::UpdateWantedLevel -- REASONED (ai_structure 0x561C90).
inline constexpr Address CWanted_UpdateWantedLevel{0x00561C90};

// ---- Verified struct offsets (mirror sa10us_db.inl, for raw access) --------
inline constexpr std::uint32_t kCVehicle_m_fHealth = 0x4C0;  // VERIFIED cold (3 sites)
inline constexpr std::uint32_t kCPed_m_fHealth     = 0x540;  // VERIFIED (C45 + ctor witness)
inline constexpr std::uint32_t kCPed_m_fArmour     = 0x548;  // VERIFIED (C45)

} // namespace sasdk::sa10us::addr
