// include/sasdk/core/callconv.hpp
//
// Phase 6: the x86 calling-convention vocabulary every typed function call
// (local and, eventually, remote) is built on.
//
// GTA:SA (gta_sa.exe) is a 32-bit x86 PE binary. Its functions were compiled
// by MSVC and use the four classic x86 conventions:
//   - __cdecl    : caller cleans the stack. Used by most free/static funcs.
//   - __stdcall  : callee cleans the stack (single `ret N`). Used by some
//                  Win32-style callbacks and a handful of engine funcs.
//   - __thiscall : `this` in ECX, rest on the stack, callee cleans. This is
//                  the convention for the overwhelming majority of the
//                  engine's C++ methods (MSVC's default for non-variadic
//                  member functions) -- so most of SASDK's 492 catalogued
//                  functions that turn out to be methods will need this.
//   - __fastcall : first two integer/pointer args in ECX/EDX, rest on the
//                  stack, callee cleans. Rare in this binary but present
//                  (MSVC sometimes chooses it for small leaf functions).
//
// This header ONLY targets 32-bit x86 (i686). GTA:SA never shipped a 64-bit
// build, and calling-convention attributes below are meaningless (and, on
// GCC/Clang, a hard error) on any other architecture -- so we fail the build
// loudly instead of silently compiling something that can never link
// against the real address space.
//
// Cross-compile target for this project: i686-w64-mingw32-g++ (verified
// against gcc-mingw-w64-i686 13.2.0 -win32 while writing this header).
#pragma once

#if !defined(__i386__) && !defined(_M_IX86)
#  error "sasdk/core/callconv.hpp: GTA:SA is a 32-bit x86 binary. Build with " \
         "an i686 target (e.g. i686-w64-mingw32-g++), not x86_64."
#endif

// ---- per-compiler attribute/keyword spelling ---------------------------
#if defined(_MSC_VER)
#  define SASDK_CDECL    __cdecl
#  define SASDK_STDCALL  __stdcall
#  define SASDK_THISCALL __thiscall
#  define SASDK_FASTCALL __fastcall
#else
// GCC / Clang on i686 (this is the toolchain SASDK actually builds with --
// MSVC branch above is provided for forward-compatibility, not yet tested).
#  define SASDK_CDECL    __attribute__((cdecl))
#  define SASDK_STDCALL  __attribute__((stdcall))
#  define SASDK_THISCALL __attribute__((thiscall))
#  define SASDK_FASTCALL __attribute__((fastcall))
#endif

namespace sasdk {

// Tag type selecting which convention a TypedFunction<> instantiation uses.
// Kept as a plain enum (not enum class-only-in-template-args) so call sites
// read naturally: TypedFunction<CallConv::Thiscall, void, int>.
enum class CallConv {
    Cdecl,
    Stdcall,
    Thiscall,
    Fastcall,
};

// ---- FnPtrTraits: CallConv -> the actual C function-pointer type --------
//
// Partial-specialized per convention so TypedFunction<Conv,Ret,Args...> can
// ask FnPtrTraits<Conv,Ret,Args...>::type for the correctly-attributed
// function-pointer type without a giant if/else in the caller.
//
// IMPORTANT (thiscall in particular): when treating a class method as a
// free function pointer this way, the `this` pointer must be passed as an
// explicit first parameter of type matching the object (commonly a raw
// pointer). This is the standard convention for attribute-cast-based ASI
// plugins -- it is how GCC/Clang's __attribute__((thiscall)) is defined to behave.
template <CallConv Conv, typename Ret, typename... Args>
struct FnPtrTraits;

template <typename Ret, typename... Args>
struct FnPtrTraits<CallConv::Cdecl, Ret, Args...> {
    using type = Ret (SASDK_CDECL*)(Args...);
};

template <typename Ret, typename... Args>
struct FnPtrTraits<CallConv::Stdcall, Ret, Args...> {
    using type = Ret (SASDK_STDCALL*)(Args...);
};

template <typename Ret, typename... Args>
struct FnPtrTraits<CallConv::Thiscall, Ret, Args...> {
    using type = Ret (SASDK_THISCALL*)(Args...);
};

template <typename Ret, typename... Args>
struct FnPtrTraits<CallConv::Fastcall, Ret, Args...> {
    using type = Ret (SASDK_FASTCALL*)(Args...);
};

} // namespace sasdk
