// examples/call_find_player_ped.cpp
//
// Demonstrates sasdk::TypedFunction: calling a catalogued game function
// DIRECTLY, as opposed to read_vehicle_health.cpp (which only reads/writes
// fields cross-process). This file is meant to be compiled INTO gta_sa.exe
// as a DLL (loaded by an ASI loader) -- it will not do anything meaningful
// linked and run standalone, because the call only makes sense at the
// address CWorld::FindPlayerPed actually occupies inside a running
// gta_sa.exe process.
//
// This intentionally duplicates what sa10us::find_player_ped() (a pure
// memory read, see sa10us_locators.hpp) already gets you more cheaply and
// more safely -- it exists as the minimal, honest proof that TypedFunction
// can invoke a real catalogued address with the right ABI, not because
// calling FindPlayerPed is the recommended way to get the player's CPed
// in practice. Prefer the read-based reimplementation when one exists;
// reach for TypedFunction for functions that actually DO something
// (mutate state, run engine logic) rather than ones that only fetch a
// value you could read yourself.
//
// Build (compiles, does not require a running game -- this only proves the
// calling-convention plumbing links and the types check; see build log):
//   i686-w64-mingw32-g++ -std=c++20 -I../include -c call_find_player_ped.cpp
//
#include <sasdk/game/local/typed_function.hpp>
#include <sasdk/game/sa10us/sa10us_locators.hpp>
#include <cstdio>
#include <windows.h>

namespace {

// CWorld::FindPlayerPed(int index = -1) -- entry 0x0056E210, cdecl
// [SASDK REASONED] (see sa10us_locators.hpp for why). Declared once here
// as a real, callable object -- not a struct field, an actual function.
constexpr sasdk::TypedFunction<sasdk::CallConv::Cdecl, sasdk::gva_t, std::int32_t>
    FindPlayerPed{sasdk::sa10us::kFindPlayerPedEntryVA};

} // namespace

// Exported so an ASI loader can call it as this mod's entry point.
extern "C" __declspec(dllexport) void OnInit() {
    // -1 => "use the current player index", matching the game's own
    // default argument (see kCurrentPlayerIndexVA in sa10us_locators.hpp).
    sasdk::gva_t local_player_ped = FindPlayerPed(-1);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "local player CPed* = 0x%08X\n", local_player_ped);
    std::fputs(buf, stdout);
}

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        OnInit();
    }
    return TRUE;
}
