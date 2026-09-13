// SPDX-License-Identifier: MIT
//
// sasdk/mod.hpp -- the one include for writing a live GTA:SA plugin (.asi).
//
// Pulls in the full runtime (address resolution, typed globals/functions,
// hooking, the generated struct DB + game bindings) and gives you a single
// entry-point macro, SASDK_MOD(fn), that becomes your DLL's whole DllMain:
//
//     #include <sasdk/mod.hpp>
//     using namespace sasdk;
//     static void my_mod() { ... }
//     SASDK_MOD(my_mod)
//
// Build it as a 32-bit DLL, rename to .asi, drop it next to gta_sa.exe with an
// ASI loader. On DLL_PROCESS_ATTACH the macro binds the module base (so every
// verified VA resolves, ASLR-safe) and runs your function on its own thread.
#ifndef SASDK_MOD_HPP
#define SASDK_MOD_HPP

#include <sasdk/sasdk.hpp>          // struct DB + error vocabulary
// In-process runtime + sa10us game bindings (the plugin API). Included here
// explicitly so a plugin gets the full callable/hookable surface regardless of
// how the umbrella is configured.
#include <sasdk/core/address.hpp>
#include <sasdk/core/memory.hpp>
#include <sasdk/core/invoke.hpp>
#include <sasdk/core/global.hpp>
#include <sasdk/core/hook.hpp>
#include <sasdk/game/sa10us/addresses.hpp>
#include <sasdk/game/sa10us/functions.inl>
#include <sasdk/game/sa10us/game.hpp>
#include <sasdk/game/sa10us/vehicle.hpp>
#include <sasdk/game/sa10us/sa10us_easy.hpp>    // the one-liner "easy" API
#include <cstdio>
#include <cstdarg>

namespace sasdk::mod {

// Minimal file logger for plugins (no iostream bloat). RAII-closed.
class Log {
public:
    explicit Log(const char* path) {
#if defined(_MSC_VER)
        fopen_s(&fp_, path, "w");   // MSVC: fopen is deprecated; fopen_s is safe
#else
        fp_ = std::fopen(path, "w");
#endif
    }
    ~Log() { if (fp_) std::fclose(fp_); }
    void operator()(const char* fmt, ...) {
        if (!fp_) return;
        va_list ap; va_start(ap, fmt);
        std::vfprintf(fp_, fmt, ap); std::fputc('\n', fp_); std::fflush(fp_);
        va_end(ap);
    }
private:
    std::FILE* fp_ = nullptr;
};

} // namespace sasdk::mod

// ---- entry point --------------------------------------------------------
#if defined(_WIN32)
#  include <windows.h>

// Defines DllMain for you. On attach: bind the module base, then spin the mod
// up on a worker thread so we never block the loader.
#  define SASDK_MOD(FN)                                                        \
    static DWORD WINAPI sasdk_mod_thread_(LPVOID) {                            \
        ::sasdk::rt::Module::bind(                                             \
            reinterpret_cast<std::uintptr_t>(::GetModuleHandleA(nullptr)));    \
        FN();                                                                  \
        return 0;                                                             \
    }                                                                         \
    BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID) {                \
        if (reason == DLL_PROCESS_ATTACH) {                                   \
            ::DisableThreadLibraryCalls(inst);                                 \
            ::CreateThread(nullptr, 0, &sasdk_mod_thread_, nullptr, 0, nullptr);\
        }                                                                     \
        return TRUE;                                                          \
    }
#else
// Host/CI build: no DLL entry. `SASDK_MOD(fn)` provides a main() so the example
// is a self-contained translation unit that still compiles and links.
#  define SASDK_MOD(FN) int main() { FN(); return 0; }
#endif

#endif // SASDK_MOD_HPP
