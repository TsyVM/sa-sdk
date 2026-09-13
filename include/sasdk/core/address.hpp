// include/sasdk/core/address.hpp
//
// The address model: turn an encyclopedia virtual address (VA, expressed
// against kPreferredImageBase = 0x00400000) into the actual runtime pointer,
// accounting for where the module really loaded (the "slide").
//
// This is the seam the audit (11_VERSION_SUPPORT) asked for: addresses are
// logical VAs, resolved at runtime, not baked absolutes.
#pragma once
#include <sasdk/core/config.hpp>
#include <cstdint>

namespace sasdk::rt {

// Process-wide module base + slide. Set once at plugin init via
// Module::bind(); defaults to the preferred base so that on a non-relocated
// module (the common case for gta_sa.exe, which loads at 0x400000) everything
// Just Works and host tests are deterministic.
class Module {
public:
    // The base the module actually loaded at. On the real target this is set
    // from GetModuleHandle(nullptr); in host builds it stays at the preferred
    // base so VA math is identity and tests are reproducible.
    static std::uintptr_t& base() {
        static std::uintptr_t b = kPreferredImageBase;
        return b;
    }

    // slide = actual_base - preferred_base. Added to every VA at resolve time.
    static std::intptr_t slide() {
        return static_cast<std::intptr_t>(base()) -
               static_cast<std::intptr_t>(kPreferredImageBase);
    }

    // Bind to a concrete runtime base (call once from plugin entry with the
    // real module handle cast to uintptr_t).
    static void bind(std::uintptr_t runtime_base) { base() = runtime_base; }

    // Resolve a logical VA to a live pointer.
    static void* resolve(std::uint32_t va) {
        return reinterpret_cast<void*>(static_cast<std::uintptr_t>(va) + slide());
    }

    // Is a VA inside the plausible image span? Cheap sanity gate; the real
    // image is ~14 MB, so anything wildly outside [base, base+0x1000000) is
    // suspect. Used by validators/tests, not a security boundary.
    static bool in_image(std::uint32_t va) {
        return va >= kPreferredImageBase &&
               va <  kPreferredImageBase + 0x01000000u;
    }
};

// A strongly-typed logical address. Cheap value type; carries the VA and knows
// how to become a live pointer. Prefer this over raw uint32_t so intent and
// provenance are visible at call sites.
struct Address {
    std::uint32_t va = 0;

    constexpr Address() = default;
    constexpr explicit Address(std::uint32_t v) : va(v) {}

    // Live pointer (base + slide applied).
    void* ptr() const { return Module::resolve(va); }
    template <class T> T* as() const { return reinterpret_cast<T*>(ptr()); }

    constexpr bool valid() const { return va != 0; }
    constexpr explicit operator bool() const { return va != 0; }

    friend constexpr bool operator==(Address a, Address b) { return a.va == b.va; }
    friend constexpr bool operator<(Address a, Address b) { return a.va < b.va; }
};

} // namespace sasdk::rt
