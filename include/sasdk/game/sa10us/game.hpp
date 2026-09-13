// include/sasdk/game/sa10us/game.hpp
//
// Runtime entry point for the sa10us target. Call Game::init() once from your
// ASI plugin's entry to bind the module base (so all logical VAs resolve) and
// validate that the loaded image is the one these addresses were verified
// against. After that, the typed globals/functions/entities are live.
#pragma once
#include <sasdk/core/config.hpp>
#include <sasdk/core/address.hpp>
#include <sasdk/core/global.hpp>
#include <sasdk/core/invoke.hpp>
#include <sasdk/core/result.hpp>
#include <sasdk/game/sa10us/addresses.hpp>
#include <sasdk/game/sa10us/functions.inl>
#include <sasdk/game/sa10us/sa10us_db.inl>
#include <cstdint>

#if SASDK_TARGET_ABI && defined(_WIN32)
  #include <windows.h>
#endif

namespace sasdk::sa10us {

class Game {
public:
    // Bind the runtime module base and check the platform. Returns an error in
    // host/CI builds (where there is no live game) so callers fail loudly
    // instead of reading garbage.
    static sasdk::Status init() {
#if SASDK_TARGET_ABI && defined(_WIN32)
        auto h = reinterpret_cast<std::uintptr_t>(GetModuleHandleA(nullptr));
        if (!h) return sasdk::err(sasdk::ErrorKind::IoError,
                                  "GetModuleHandle(nullptr) returned null");
        sasdk::rt::Module::bind(h);
        return {};
#else
        return sasdk::err(sasdk::ErrorKind::Unverified,
            "Game::init() called on a non-target (host/CI) build -- no live "
            "gta_sa.exe. Runtime read/write/call/hook is unavailable here; "
            "this build exists to compile and test the portable logic.");
#endif
    }

    static bool is_target_build() { return sasdk::rt::kIsTargetAbi; }
    static const char* target() { return sasdk::rt::kTargetBuild; }

    // ---- typed globals ----
    static sasdk::rt::Global<CCamera> camera() {
        return sasdk::rt::Global<CCamera>(addr::TheCamera);
    }
    static sasdk::rt::ArrayView<CStreamingInfo> streaming_info() {
        return sasdk::rt::ArrayView<CStreamingInfo>(
            addr::StreamingInfoArray.va,
            addr::kStreamingInfoCount,
            addr::kStreamingInfoStride);
    }

    // ---- typed functions (callable surface, generated in functions.inl) ----

    // The player's ped / current vehicle (null if on foot).
    static CPed*     player_ped(int player = -1) { return fn::FindPlayerPed(player); }
    static CVehicle* player_vehicle(int player = -1) {
        return fn::FindPlayerVehicle(player, false);
    }

    // CStreaming::RemoveAllUnusedModels() -- __cdecl (static). VERIFIED addr.
    static void remove_all_unused_models() { fn::CStreaming_RemoveAllUnusedModels(); }
};

} // namespace sasdk::sa10us
