// include/sasdk/core/invoke.hpp
//
// Typed invocation of engine functions at a logical VA, with the correct 32-bit
// calling convention. GTA:SA (MSVC-built) uses:
//   * __thiscall for non-static member functions (this in ECX, args on stack)
//   * __cdecl    for free functions / statics (caller cleans the stack)
//   * __stdcall  for some Win32-facing callbacks (callee cleans)
//
// Getting the convention wrong corrupts the stack, so the convention is part of
// the call's type, chosen explicitly at the call site. On the real target the
// SASDK_* keywords are the MSVC conventions; on the host build they vanish and
// the same templates can be exercised against ordinary host functions (default
// convention) so the plumbing is unit-testable without the game.
//
// Addresses are kept as logical VAs resolved through the Module slide rather
// than baked absolutes, so a relocated module still resolves correctly.
#pragma once
#include <sasdk/core/config.hpp>
#include <sasdk/core/address.hpp>
#include <cstdint>

namespace sasdk::rt {

// ---- free / static functions ----------------------------------------------

// __cdecl call at a VA.
template <class Ret = void, class... Args>
Ret call_cdecl(Address fn, Args... args) {
    using Fn = Ret(SASDK_CDECL*)(Args...);
    return reinterpret_cast<Fn>(fn.ptr())(args...);
}

// __stdcall call at a VA.
template <class Ret = void, class... Args>
Ret call_stdcall(Address fn, Args... args) {
    using Fn = Ret(SASDK_STDCALL*)(Args...);
    return reinterpret_cast<Fn>(fn.ptr())(args...);
}

// ---- non-static member functions (__thiscall) ------------------------------
//
// MSVC accepts __thiscall on a free function pointer whose first parameter is
// the explicit `this`. On the host build __thiscall vanishes and this becomes an
// ordinary first-argument call, which still type-checks and runs for tests.

template <class Ret = void, class Self = void, class... Args>
Ret call_method(Address fn, Self* self, Args... args) {
    using Fn = Ret(SASDK_THISCALL*)(Self*, Args...);
    return reinterpret_cast<Fn>(fn.ptr())(self, args...);
}

// ---- convenience: a bound, typed function handle ---------------------------
//
// Wrap a VA + signature once, call it many times. This is the "callable
// surface" the audit (04_FUNCTION_COVERAGE) found missing: a name+address
// becomes an actually-invokable, type-checked handle.

enum class CallConv { Cdecl, Stdcall, Thiscall };

template <CallConv CC, class Ret, class... Args>
class Function {
public:
    constexpr explicit Function(Address at) : at_(at) {}
    constexpr explicit Function(std::uint32_t va) : at_(va) {}

    Ret operator()(Args... args) const {
        if constexpr (CC == CallConv::Cdecl)
            return call_cdecl<Ret, Args...>(at_, args...);
        else if constexpr (CC == CallConv::Stdcall)
            return call_stdcall<Ret, Args...>(at_, args...);
        else
            static_assert(CC != CallConv::Thiscall,
                "use MemberFunction<> for __thiscall (needs an explicit this)");
    }

    Address address() const { return at_; }
private:
    Address at_;
};

// __thiscall handle: first call argument is the object pointer.
template <class Ret, class Self, class... Args>
class MemberFunction {
public:
    constexpr explicit MemberFunction(Address at) : at_(at) {}
    constexpr explicit MemberFunction(std::uint32_t va) : at_(va) {}

    Ret operator()(Self* self, Args... args) const {
        return call_method<Ret, Self, Args...>(at_, self, args...);
    }
    Address address() const { return at_; }
private:
    Address at_;
};

} // namespace sasdk::rt
