// include/sasdk/game/sa10us/sa10us_locators.hpp
//
// Entry-point locators: fixed static addresses that let a caller find live
// game objects (the player's CPed, etc.) without needing an already-known
// address to start from.
//
// Unlike sa10us_db.inl (struct layouts), this file documents *global*
// addresses -- fixed slots in the exe's data segment that always hold a
// pointer/value at runtime, independent of any single object instance.
//
// Confirmed by disassembly against the user-supplied GTA_SA.EXE:
//   - CWorld::FindPlayerPed(int index = -1) @ entry 0x0056E210 -- NOT
//     HOODLUM-hooked (this address decodes directly, no jmp trampoline).
//       mov eax, [esp+4]                  ; index arg
//       test eax, eax
//       jge use_arg
//       movzx eax, byte ptr [0xB7CD74]    ; default: current-player index
//     use_arg:
//       imul eax, eax, 0x190              ; slot stride
//       mov eax, [eax + 0xB7CD98]         ; CPed* array base
//       ret
//   - Confirmed as returning a genuine, directly-usable CPed* (not a
//     wrapper needing a further dereference) by two independent call
//     sites that index off the return value with ordinary small CPed
//     offsets:
//       * 0x404719: `movzx ecx, byte ptr [eax + 0x2f]` (a flag/type byte)
//       * CGameLogic::DoWeaponStuffAtStartOf2PlayerGame body @ 0x156AA30:
//         `add eax, 0x5a0` used as the base of the per-ped weapon-slot
//         array immediately after the same call pattern.
//
// name_alternates: this function is labelled `FindPlayerPed` here for
// readability, but only the address, byte pattern, and behavior above were
// independently verified against the supplied exe -- the name itself is not a
// checked fact.

#pragma once

#include <cstdint>
#include <sasdk/core/result.hpp>
#include <sasdk/game/process/process.hpp>

namespace sasdk::sa10us {

// Static VAs (pass through Process::rebase() before use -- see process.hpp).
inline constexpr gva_t kPlayerPedArrayVA      = 0xB7CD98u; // CPed* slots, stride kPlayerSlotStride
inline constexpr gva_t kCurrentPlayerIndexVA  = 0xB7CD74u; // uint8_t, default player-index
inline constexpr uint32_t kPlayerSlotStride   = 0x190u;

// Entry address for CWorld::FindPlayerPed itself (see the disassembly at
// the top of this file). NOT HOODLUM-hooked -- this decodes directly, so
// it is safe to call locally (TypedFunction) as well as reimplement as a
// pure read (find_player_ped() below). Calling convention is [SASDK
// REASONED, not disassembly-confirmed]: the body reads its argument from
// [esp+4] and the exit is a bare `ret` with no stack-cleanup immediate,
// which is consistent with __cdecl and inconsistent with __stdcall; no
// structural_fact currently asserts this as a checked literal.
inline constexpr gva_t kFindPlayerPedEntryVA  = 0x0056E210u;

// CGangZone fixed array (see CGangZone struct in sa10us_db.inl for field
// evidence). Confirmed directly in CanPlayerStartAGangWarHere @ 0x443F80.
inline constexpr gva_t kGangZoneArrayVA       = 0xBA1DF0u;
inline constexpr uint32_t kGangZoneStride     = 0x11u; // sizeof(CGangZone)
inline constexpr gva_t kNumGangZonesVA        = 0xBA3794u; // uint16_t, runtime count -- NOT a compile-time array bound

// CStuckCarCheckEntry fixed array (see struct in sa10us_db.inl for field
// evidence). Confirmed slot count in IsCarInStuckCarArray @ 0x463C70 and
// AddCarToCheck @ 0x465970. Singleton's own absolute base address NOT
// found in this pass -- no kStuckCarCheckArrayVA constant here (would be
// a guess); this is left as a documented open item.
inline constexpr uint32_t kStuckCarCheckSlotCount = 16u;

// CStuntJumpEntry dynamic array (see struct in sa10us_db.inl for field
// evidence). Unlike the fixed arrays above, this one is a pointer-to-array:
// dereference kStuntJumpArrayPtrVA once to get the actual array base.
// Confirmed in CStuntJumpManager::AddOne @ 0x0049CB40.
inline constexpr gva_t kStuntJumpArrayPtrVA   = 0xA9A888u; // gva_t*, one more deref needed
inline constexpr gva_t kNumStuntJumpsVA       = 0xA9A89Cu; // uint32_t, live count

// CTagEntry fixed array (see struct in sa10us_db.inl for field evidence).
// Confirmed in CTagManager::AddTag @ 0x0049CC90 and
// CTagManager::UpdateNumTagged @ 0x0049CDE0.
inline constexpr gva_t kTagArrayVA            = 0xA9A8C0u;
inline constexpr uint32_t kTagEntryStride     = 0x8u; // sizeof(CTagEntry)
inline constexpr gva_t kNumTagsVA             = 0xA9AD70u; // uint32_t, live count

// CRoadBlockEntry fixed array (see struct in sa10us_db.inl for field
// evidence). Confirmed in CRoadBlocks::RegisterScriptRoadBlock @ 0x00460DF0.
inline constexpr gva_t kRoadBlockArrayVA      = 0xA43AD0u;
inline constexpr uint32_t kRoadBlockStride    = 0x1Cu; // sizeof(CRoadBlockEntry)
inline constexpr uint32_t kRoadBlockSlotCount = 16u;

// CIplDefEntry fixed array (see struct in sa10us_db.inl for field
// evidence). Base confirmed via CIplDefPool::New @ 0x004059B0.
inline constexpr gva_t kIplDefArrayVA         = 0x8E3FB0u; // pointer, one more deref needed (same convention as kStuntJumpArrayPtrVA)
inline constexpr uint32_t kIplDefEntryStride  = 0x34u; // sizeof(CIplDefEntry)

// CColStore manager pointer chain (confirmed in CColStore::RemoveColSlot
// @ 0x00411330 -- also independently re-confirms CColStoreSlot::status@0x28,
// one of the original 30 structs). Dereference kColStoreManagerPtrVA to
// get the manager struct, whose fields are: +0x0 slot-array pointer,
// +0x4 a parallel per-slot flags-byte array pointer (separate allocation,
// same index as the slot array), +0xC a live "high water mark" index.
inline constexpr gva_t kColStoreManagerPtrVA  = 0x965560u;
inline constexpr uint32_t kColStoreSlotStride = 0x2Cu; // sizeof(CColStoreSlot), cross-confirmed here too

// COnscreenCounterEntry array (see struct in sa10us_db.inl for field
// evidence). Confirmed in COnscreenTimer::AddCounter @ 0x0044CDA0. The
// manager singleton's own absolute address was not found in this pass.
inline constexpr uint32_t kOnscreenCounterStride = 0x44u; // sizeof(COnscreenCounterEntry)
inline constexpr uint32_t kOnscreenCounterArrayOffsetInManager = 0x40u; // array starts at manager_this+0x40

// Reproduces CWorld::FindPlayerPed(index) @ 0x0056E210 as a pure memory
// read (no call-into-process / code injection required). index < 0 means
// "use the current player index" exactly as the game's own default arg does.
inline Result<gva_t> find_player_ped(const Process& proc, int32_t index = -1) {
    if (index < 0) {
        auto idx = proc.read<uint8_t>(proc.rebase(kCurrentPlayerIndexVA));
        if (!idx) return idx.error();
        index = static_cast<int32_t>(*idx);
    }
    const gva_t slot_addr =
        proc.rebase(kPlayerPedArrayVA) + static_cast<gva_t>(index) * kPlayerSlotStride;
    return proc.read<gva_t>(slot_addr);
}

} // namespace sasdk::sa10us
