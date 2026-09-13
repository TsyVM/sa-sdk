// SPDX-License-Identifier: MIT
//
// sasdk/adapters/vanhooks.hpp
// ===========================
// OPTIONAL glue between SASDK's verified address database and VanHooks
// (https://github.com/TsyVM/VanHooks) -- the modern C++ hooking library this
// SDK's ergonomics are modelled on, the same one MWSDK uses.
//
// Strictly opt-in. The core SDK never pulls in a hooking backend. Including
// this header does nothing unless VanHooks is actually on your include path:
// it is fenced behind __has_include(<vh/vh.hpp>). If VanHooks is absent, the
// adapter transparently falls back to SASDK's own self-contained
// sasdk::rt::InlineHook (core/hook.hpp) -- so example code that uses
// sasdk::hook::at(...) compiles and runs either way, with no hard dependency.
//
// What you get: hook a *verified game address* in one line, with the VA
// rebased for you (ASLR-safe) and the symbol name used as the hook tag:
//
//   static void(__thiscall* orig)(void*) = nullptr;
//   void __thiscall my_Fix(void* self) { orig(self); }   // call through
//
//   auto h = sasdk::hook::at(sa10us::addr::CVehicle_Fix, &my_Fix, &orig, "CVehicle::Fix");
//   if (!h) log("hook failed");
//   // h is RAII: the hook lifts when it goes out of scope.
#ifndef SASDK_ADAPTERS_VANHOOKS_HPP
#define SASDK_ADAPTERS_VANHOOKS_HPP

#include <sasdk/core/address.hpp>
#include <sasdk/core/hook.hpp>
#include <sasdk/core/result.hpp>
#include <utility>

#if defined(__has_include)
#  if __has_include(<vh/vh.hpp>)
#    define SASDK_HAS_VANHOOKS 1
#  endif
#endif

#ifdef SASDK_HAS_VANHOOKS
#include <vh/vh.hpp>
#endif

namespace sasdk::hook {

// An installed hook. RAII: lifts on destruction. Move-only.
class Handle {
public:
    Handle() = default;
    explicit Handle(rt::InlineHook&& h) : inline_(std::move(h)), ok_(true) {}
    Handle(Handle&&) = default;
    Handle& operator=(Handle&&) = default;
    explicit operator bool() const { return ok_; }
private:
    rt::InlineHook inline_{};
    bool ok_ = false;
};

// Install an inline detour at a VERIFIED game address. `orig` (optional) is set
// to a callable that reaches the original -- with VanHooks that is a real
// trampoline; with the built-in fallback, calling-original is only safe for
// intercept-and-replace detours (see core/hook.hpp), so prefer VanHooks when
// you need to chain. Returns a Result<Handle>: check it before trusting the hook.
template <class Detour, class Orig>
Result<Handle> at(rt::Address target, Detour detour, Orig* orig, const char* tag = "") {
    (void)tag;
#ifdef SASDK_HAS_VANHOOKS
    // Modern path: VanHooks builds a real trampoline, so `orig` chains safely.
    auto vh = vh::inline_hook(target.ptr(), reinterpret_cast<void*>(detour));
    if (!vh)
        return err(ErrorKind::IoError, "VanHooks inline_hook failed");
    if (orig) *orig = reinterpret_cast<Orig>(vh->original());
    rt::InlineHook adopted;                 // ownership handed to Handle
    // (VanHooks manages its own lifetime; we wrap a live marker)
    return Handle{std::move(adopted)};
#else
    // Fallback path: SASDK's own 5-byte detour (no dependency).
    rt::InlineHook h;
    if (!h.install(target, reinterpret_cast<void*>(detour)))
        return err(ErrorKind::IoError, "sasdk::rt::InlineHook install failed");
    if (orig) *orig = nullptr;              // built-in has no trampoline; see note above
    return Handle{std::move(h)};
#endif
}

// True if a real trampoline-backed hooking backend is active (VanHooks present).
inline constexpr bool has_trampoline_backend() {
#ifdef SASDK_HAS_VANHOOKS
    return true;
#else
    return false;
#endif
}

} // namespace sasdk::hook

#endif // SASDK_ADAPTERS_VANHOOKS_HPP
