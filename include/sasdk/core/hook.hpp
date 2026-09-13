// include/sasdk/core/hook.hpp
//
// Two hooking primitives:
//   * InlineHook -- overwrite a function's first 5 bytes with a near JMP to a
//     detour (the classic x86 detour). Install/remove are complete; calling the
//     ORIGINAL requires a relocating trampoline (instruction-length aware),
//     which a header-only runtime with no disassembler cannot do safely, so
//     that is intentionally left to the tooling layer -- see note below.
//   * VmtHook -- swap one entry in an object's virtual method table. Complete,
//     safe, and can call the original trivially. This covers the common
//     polymorphic-intercept case (every CEntity/CVehicle/CPed virtual).
//
// The JMP encoding (encode_jmp_rel32) is a pure function and is unit-tested on
// the host build; the actual patching is guarded and real only on the target.
#pragma once
#include <sasdk/core/config.hpp>
#include <sasdk/core/address.hpp>
#include <sasdk/core/memory.hpp>
#include <array>
#include <cstdint>
#include <cstring>

namespace sasdk::rt {

// Encode a 5-byte `E9 rel32` near jump from `from` to `to`.
// rel32 = to - (from + 5). Pure; no memory touched. Unit-tested.
inline std::array<std::uint8_t, 5>
encode_jmp_rel32(std::uintptr_t from, std::uintptr_t to) {
    std::array<std::uint8_t, 5> out{};
    out[0] = 0xE9;
    const std::int32_t rel =
        static_cast<std::int32_t>(static_cast<std::intptr_t>(to) -
                                  static_cast<std::intptr_t>(from) - 5);
    std::memcpy(&out[1], &rel, 4);
    return out;
}

// Inline (detour) hook. Overwrites 5 bytes at the target VA with a jump to the
// detour. remove() restores the saved bytes. Idempotent guards prevent
// double-install / double-remove.
class InlineHook {
public:
    InlineHook() = default;
    InlineHook(const InlineHook&) = delete;
    InlineHook& operator=(const InlineHook&) = delete;
    // Movable (RAII ownership transfer): the moved-from hook must not also try
    // to remove on destruction, so zero its installed flag.
    InlineHook(InlineHook&& o) noexcept
        : target_(o.target_), saved_(o.saved_), installed_(o.installed_) {
        o.installed_ = false;
    }
    InlineHook& operator=(InlineHook&& o) noexcept {
        if (this != &o) {
            if (installed_) remove();
            target_ = o.target_; saved_ = o.saved_; installed_ = o.installed_;
            o.installed_ = false;
        }
        return *this;
    }

    // detour must be a real function pointer (correct calling convention is the
    // caller's responsibility). Returns false if already installed.
    bool install(Address target, void* detour) {
        if (installed_) return false;
        target_ = target.ptr();
        std::memcpy(saved_.data(), target_, saved_.size());
        auto patch = encode_jmp_rel32(reinterpret_cast<std::uintptr_t>(target_),
                                      reinterpret_cast<std::uintptr_t>(detour));
        if (!mem::patch(target_, patch.data(), patch.size())) return false;
        installed_ = true;
        return true;
    }

    bool remove() {
        if (!installed_) return false;
        if (!mem::patch(target_, saved_.data(), saved_.size())) return false;
        installed_ = false;
        return true;
    }

    ~InlineHook() { if (installed_) remove(); }

    bool installed() const { return installed_; }
    const std::array<std::uint8_t, 5>& original_bytes() const { return saved_; }

    // NOTE on calling the original: with the first 5 bytes replaced you cannot
    // simply call target_ (you'd re-enter the detour). A correct
    // call-original needs a trampoline holding the relocated stolen
    // instructions + a jump back to target+len. That requires an
    // instruction-length decoder, which lives in the tooling layer
    // (tools/scan_offset.py's capstone), not in this runtime header. For
    // intercept-and-replace hooks (no original call) InlineHook is complete;
    // for intercept-and-continue, prefer VmtHook where the original is just the
    // saved pointer.
private:
    void*                          target_ = nullptr;
    std::array<std::uint8_t, 5>    saved_{};
    bool                           installed_ = false;
};

// Virtual-method-table hook. Given an object (whose first member is its vtable
// pointer) and a slot index, swap the function pointer at that slot and keep
// the original for chaining. Complete and safe.
class VmtHook {
public:
    // `object` points at an instance; **object is its vtable. `index` is the
    // 0-based virtual slot. `detour` replaces it; original() returns the prior.
    bool install(void* object, std::size_t index, void* detour) {
        if (installed_) return false;
        vtable_ = *reinterpret_cast<void***>(object);
        index_  = index;
        original_ = vtable_[index_];
        mem::ProtectGuard g(&vtable_[index_], sizeof(void*));
        if (!g.ok()) return false;
        vtable_[index_] = detour;
        installed_ = true;
        return true;
    }

    bool remove() {
        if (!installed_) return false;
        mem::ProtectGuard g(&vtable_[index_], sizeof(void*));
        if (!g.ok()) return false;
        vtable_[index_] = original_;
        installed_ = false;
        return true;
    }

    ~VmtHook() { if (installed_) remove(); }

    void* original() const { return original_; }   // call this to chain
    bool  installed() const { return installed_; }
private:
    void**      vtable_   = nullptr;
    std::size_t index_    = 0;
    void*       original_ = nullptr;
    bool        installed_ = false;
};

} // namespace sasdk::rt
