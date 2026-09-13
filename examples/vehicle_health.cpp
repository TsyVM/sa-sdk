// examples/vehicle_health.cpp
//
// End-to-end demonstration of the SASDK runtime stack. This is the "prove the
// whole thing works" example the audit asked for: bind the module, wrap a live
// CVehicle, read/write its cold-verified health field, repair it via a real
// __thiscall into the engine, and hook a vehicle virtual to observe events.
//
// On the real target this is compiled into an .asi and injected into
// gta_sa.exe. On a host/CI build it compiles (proving the API is well-formed)
// but does not run against a live game -- Game::init() returns an error there.
//
// NOTE: obtaining a CVehicle* (e.g. the player's car) requires a pool/global
// accessor not yet curated (FindPlayerVehicle). To stay honest, this example
// receives the vehicle pointer from the caller -- in a real plugin it would
// come from the vehicle pool or from a hooked engine function's argument.
#include <sasdk/sasdk.hpp>
#include <cstdio>

using namespace sasdk;
using namespace sasdk::sa10us;

// Do something useful with a vehicle we already have a pointer to.
void demo_on_vehicle(void* live_vehicle_ptr) {
    CVehicleRef veh(live_vehicle_ptr);
    if (!veh) return;

    std::printf("health before: %.1f\n", veh.health());   // 1000.0 = full

    // Read/write the cold-verified m_fHealth field (offset 0x4C0).
    if (veh.is_wrecked())
        veh.set_health(250.0f);          // revive a wreck a little

    // Or repair fully by calling the engine's own CVehicle::Fix() (__thiscall).
    veh.fix();

    std::printf("health after fix: %.1f\n", veh.health());
}

// A detour for an inline hook on a vehicle-related engine function. The calling
// convention must match the target; CStreaming::RemoveAllUnusedModels is
// __cdecl/void, used here purely to show the hook wiring.
static rt::InlineHook g_hook;
static void SASDK_CDECL detour_remove_unused() {
    std::printf("[hook] RemoveAllUnusedModels called\n");
    // (intercept-and-replace: we do not chain to original in this minimal demo)
}

int plugin_main() {
    if (auto st = Game::init(); !st) {
        std::printf("SASDK: %s\n", st.error().message.c_str());
        return 1;  // host build: expected -- no live game
    }

    // Typed global: read the active camera index through TheCamera (0xB6F028).
    auto cam = Game::camera();
    std::printf("active cam index: %u\n", cam->m_nActiveCam);

    // Callable surface: fetch the player's vehicle and act on it.
    if (CVehicle* pv = Game::player_vehicle()) {
        CVehicleRef veh(pv);
        std::printf("player vehicle: model=%u health=%.0f\n",
                    veh.model_index(), veh.health());  // model_index @0x22, health @0x4C0
        veh.set_health(1000.0f);   // full health
        // or veh.fix();  // call the engine's own repair
    }

    // Install an inline hook on a VERIFIED-address engine function.
    g_hook.install(addr::CStreaming_RemoveAllUnusedModels,
                   reinterpret_cast<void*>(&detour_remove_unused));

    // ... game runs; detour fires when the engine calls the function ...

    g_hook.remove();
    return 0;
}

// So the example is a self-contained TU in host/CI builds.
int main() { return plugin_main(); }
