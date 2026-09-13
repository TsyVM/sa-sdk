// include/sasdk/core/config.hpp
//
// Platform / ABI detection for the SASDK runtime layer.
//
// SASDK targets an IN-PROCESS 32-bit ASI plugin loaded into gta_sa.exe (1.0 US,
// ImageBase 0x00400000). The runtime therefore does not "attach" to a remote
// process -- it runs inside the game and reads/writes/calls at the game's own
// virtual addresses. Two build modes exist:
//
//   * TARGET build  (MSVC, /arch:IA32, 32-bit): the real thing. Calling
//     conventions are exact (__thiscall etc.), memory ops hit the live game,
//     hooks patch real code. This is what ships in the .asi.
//
//   * HOST/dev build (any compiler, incl. 64-bit g++/clang on Linux): the
//     ABI-exact machinery is compiled but the calling-convention keywords
//     degrade to nothing and the OS memory calls are stubbed. This lets the
//     portable *logic* (address math, hook byte-encoding, table integrity,
//     struct offsets) be unit-tested in CI without the game or Windows.
//
// A consumer never sets these directly; they are derived from the compiler.
#pragma once
#include <cstdint>

// ---- target detection ------------------------------------------------------
#if defined(_WIN32) && (defined(_M_IX86) || defined(__i386__))
  // 32-bit Windows: the real ABI target.
  #define SASDK_TARGET_ABI 1
#else
  // Anything else (64-bit host, Linux, macOS): dev/CI build, ABI-exact calls
  // and live memory ops are not available. Logic still compiles.
  #define SASDK_TARGET_ABI 0
#endif

// ---- calling-convention keywords ------------------------------------------
// On the real target these expand to the MSVC keywords; on the host build they
// vanish so the templates still type-check and can be exercised against host
// functions with the default convention.
#if SASDK_TARGET_ABI && defined(_MSC_VER)
  #define SASDK_CDECL    __cdecl
  #define SASDK_STDCALL  __stdcall
  #define SASDK_THISCALL __thiscall
  #define SASDK_FASTCALL __fastcall
#else
  #define SASDK_CDECL
  #define SASDK_STDCALL
  #define SASDK_THISCALL
  #define SASDK_FASTCALL
#endif

namespace sasdk::rt {

// The image base the RE-Data virtual addresses are expressed against. Every
// address stored in the encyclopedia/SDK is a VA of the form 0x004xxxxx; at
// runtime the module may load elsewhere (ASLR / rebasing), so the runtime
// applies a slide = actual_base - kPreferredImageBase. See address.hpp.
inline constexpr std::uint32_t kPreferredImageBase = 0x00400000u;

// The exact image these addresses were verified against (C0 / binary-identity).
// A runtime validation step should confirm the loaded module matches before
// trusting any address (see game.hpp).
inline constexpr char kTargetBuild[]      = "sa10us";             // 1.0 US HOODLUM
inline constexpr char kTargetExeMd5[]     = "170b3a9108687b26da2d8901c6948a18";
inline constexpr std::uint32_t kTargetExeSize = 14383616u;         // bytes, on-disk reference copy

// True on the shipping target, false in host/CI builds. Consumers can branch on
// this to give clear errors instead of silently no-oping on the wrong platform.
inline constexpr bool kIsTargetAbi = (SASDK_TARGET_ABI == 1);

} // namespace sasdk::rt
