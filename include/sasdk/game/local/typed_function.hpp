// include/sasdk/game/local/typed_function.hpp
//
// Phase 6: local (in-process) typed function invocation.
//
// This is the SDK's answer to "492 catalogued addresses and no way to call
// any of them" (audit AUDIT-2026-07/13_MISSING_CAPABILITIES.md, item #2,
// CRITICAL). It is deliberately the FIRST runtime-call layer built, ahead of
// hooking, because hooking a function still needs to be able to call the
// *original* through a trampoline with the right ABI -- this header is that
// building block.
//
// Scope, explicitly:
//   - This header is for code COMPILED INTO gta_sa.exe's process (an ASI /
//     DLL loaded by an ASI loader). In that world, a "function at game VA X"
//     really is a function at address X in this process's own address
//     space, and calling it is an ordinary function-pointer call.
//   - This is NOT the same problem as sasdk::Process (game/process/
//     process.hpp), which reads/writes another process's memory from the
//     outside. Cross-process *invocation* (making the other process's CPU
//     execute a function at a chosen address) is a materially harder
//     problem -- it needs a remote code-cave/stub, a way to set the remote
//     instruction pointer and arguments, and a way to resume and wait for
//     it to return (CreateRemoteThread + a tiny shellcode trampoline on
//     Windows; a ptrace PTRACE_SETREGS/PTRACE_CONT dance on Linux). That is
//     real, separate future work and is NOT implemented here -- see the
//     "Remote invocation" note at the bottom of this file rather than
//     assuming TypedFunction quietly does it.
//
// Usage (from inside the game process):
//   using namespace sasdk;
//   // CWorld::FindPlayerPed(int index = -1) -- entry 0x0056E210, confirmed
//   // NOT HOODLUM-hooked (decodes directly, no jmp trampoline) --
//   // see sa10us_locators.hpp for the disassembly this address rests on.
//   // Calling convention below is REASONED, not disassembly-confirmed:
//   // the decoded body reads its argument via [esp+4] and the tail is a
//   // bare `ret` (no stack-cleanup immediate), which is consistent with
//   // __cdecl and inconsistent with __stdcall/__thiscall -- but no
//   // structural_fact in the encyclopedia currently asserts the calling
//   // convention as a checked literal, so treat this instantiation as
//   // [SASDK REASONED] until someone confirms the `ret` operand byte-for-
//   // byte the way structural_facts.verification.md does for offsets.
//   constexpr TypedFunction<CallConv::Cdecl, gva_t, int32_t>
//       FindPlayerPed{sa10us::kFindPlayerPedVA};
//
//   gva_t local_player_ped = FindPlayerPed(-1);
#pragma once

#include <sasdk/core/callconv.hpp>
#include <sasdk/game/process/process.hpp>  // for gva_t
#include <cstdint>
#include <type_traits>

namespace sasdk {

// A callable wrapper around a fixed game virtual address, typed with the
// calling convention, return type and argument types the caller asserts
// for it. Trivially copyable, zero runtime overhead beyond the underlying
// call -- this is a reinterpret_cast wearing a type-safe C++ front end, not
// a dispatcher.
//
// SASDK_BACKEND_LINUX note: this template compiles cleanly on any i686
// target (including a Linux i686 host, for unit testing the plumbing), but
// *calling* operator() only makes sense when `address` genuinely points at
// mapped, executable code in the calling process -- i.e. when this code is
// itself running inside gta_sa.exe. Calling it against an arbitrary address
// from a standalone Linux test binary will crash, same as any bad function
// pointer would; TypedFunction does not and cannot validate that for you.
template <CallConv Conv, typename Ret, typename... Args>
class TypedFunction {
public:
    using FnPtr = typename FnPtrTraits<Conv, Ret, Args...>::type;

    // `address` is expected to already be a *live, rebased* address in this
    // process (see sasdk::Process::rebase() if you obtained it as a static
    // VA and need to account for a relocated image -- not needed for the
    // pinned sa10us 1.0 US build, which never relocates).
    constexpr explicit TypedFunction(gva_t address) noexcept : addr_(address) {}

    // Direct call. No Result<T> wrapper: once you are executing inside the
    // target process, a function-pointer call either works or it doesn't in
    // exactly the way any other function-pointer call in C++ either works
    // or doesn't -- wrapping it in a Result would only paper over a crash
    // one frame later. Get the address and ABI right (i.e. prefer 🟢/✅
    // catalogue entries) instead of relying on this to fail gracefully.
    Ret operator()(Args... args) const {
        auto fn = reinterpret_cast<FnPtr>(static_cast<std::uintptr_t>(addr_));
        return fn(args...);
    }

    gva_t address() const noexcept { return addr_; }

private:
    static_assert(std::is_trivially_copyable_v<Ret> || std::is_void_v<Ret>,
        "TypedFunction<...>: Ret must be trivially copyable (or void) -- "
        "this is a raw ABI call, not a marshalling layer.");

    gva_t addr_;
};

// ---------------------------------------------------------------------
// Remote invocation -- NOT IMPLEMENTED.
//
// A cross-process sasdk::Process::call<Conv,Ret,Args...>(addr, args...)
// is a legitimate, separate future feature (needed for out-of-process trainers
// that want to invoke engine logic without injecting a DLL), but it
// requires:
//   1. Allocating a small scratch page of executable memory in the target
//      process (VirtualAllocEx + a hand-built call stub, or a ptrace'd
//      mmap on Linux).
//   2. Marshalling `args` into that process's stack/registers per `Conv`.
//   3. Redirecting a thread's instruction pointer into the stub and
//      resuming it (CreateRemoteThread, or PTRACE_SETREGS + PTRACE_CONT
//      + waitpid on a stopped thread).
//   4. Reading the return value back out of EAX (and cleaning up the
///     scratch page).
// None of this exists yet anywhere in SASDK. Do not assume TypedFunction
// or sasdk::Process silently does it -- there is currently no remote-call
// path at all, only remote read/write (process.hpp) and local call
// (this file).
// ---------------------------------------------------------------------

} // namespace sasdk
