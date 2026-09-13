// examples/read_vehicle_health.cpp
//
// Demonstrates using sasdk::Process to read CVehicle::m_fHealth from a
// running gta_sa.exe (GTASA 1.0 US, SecuROM+HOODLUM).
//
// You need the vehicle's VA (game virtual address). The easiest way to get
// one for testing is from a memory scanner (Cheat Engine) while in a vehicle.
//
// Build:
//   g++ -std=c++20 -I../include -o read_vehicle_health read_vehicle_health.cpp
//
// Usage:
//   ./read_vehicle_health <gta_sa_pid> <vehicle_va_hex>
//   e.g.  ./read_vehicle_health 1234 0x1234ABCD
//
#include <sasdk/sasdk.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::fprintf(stderr,
            "usage: %s <pid> <vehicle_va_hex>\n"
            "  pid            -- PID of gta_sa.exe\n"
            "  vehicle_va_hex -- CVehicle* game VA (e.g. 0x1A2B3C4D)\n",
            argv[0]);
        return 1;
    }

    // pid_t is POSIX-only; process.hpp's Windows backend takes a DWORD.
    // This was previously hardcoded to pid_t and silently never compiled
    // against SASDK_BACKEND_WINDOWS -- fixed here to match whichever
    // backend process.hpp actually selected for this build.
#if defined(SASDK_BACKEND_WINDOWS)
    DWORD         pid        = static_cast<DWORD>(std::atoi(argv[1]));
#else
    pid_t         pid        = static_cast<pid_t>(std::atoi(argv[1]));
#endif
    sasdk::gva_t  vehicle_va = static_cast<sasdk::gva_t>(std::strtoul(argv[2], nullptr, 16));

    // ---- attach --------------------------------------------------------
    auto proc = sasdk::Process::attach(pid);
    if (!proc) {
        std::fprintf(stderr, "attach failed: %s\n", proc.error().message.c_str());
        return 1;
    }
    std::printf("Attached to PID %d  (image base: 0x%08X)\n",
        static_cast<int>(pid), proc->process_base());

    // ---- read CVehicle::m_pDriver (verified: +0x460) -------------------
    auto driver_va = proc->read<sasdk::gva_t>(vehicle_va + offsetof(sasdk::sa10us::CVehicle, m_pDriver));
    if (!driver_va) {
        std::fprintf(stderr, "read m_pDriver failed: %s\n", driver_va.error().message.c_str());
        return 1;
    }
    std::printf("CVehicle @ 0x%08X\n", vehicle_va);
    std::printf("  m_pDriver (+0x460) = 0x%08X  %s\n",
        *driver_va,
        *driver_va == 0 ? "(no driver)" : "(has driver)");

    // ---- read CVehicle::m_fHealth (verified: +0x4C0) -------------------
    auto health = proc->read<float>(vehicle_va + offsetof(sasdk::sa10us::CVehicle, m_fHealth));
    if (!health) {
        std::fprintf(stderr, "read m_fHealth failed: %s\n", health.error().message.c_str());
        return 1;
    }
    std::printf("  m_fHealth  (+0x4C0) = %.2f / 1000.0\n", *health);

    // ---- optionally set health to max ----------------------------------
    // Uncomment to repair the vehicle:
    //
    // float max_health = 1000.f;
    // auto st = proc->write<float>(vehicle_va + offsetof(sasdk::sa10us::CVehicle, m_fHealth), max_health);
    // if (!st) {
    //     std::fprintf(stderr, "write m_fHealth failed: %s\n", st.error().message.c_str());
    //     return 1;
    // }
    // std::printf("  m_fHealth set to 1000.0\n");

    return 0;
}
