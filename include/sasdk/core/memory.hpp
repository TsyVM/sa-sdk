// include/sasdk/core/memory.hpp
//
// Raw memory read/write/patch over the game's address space. In-process:
// reads/writes are just typed pointer access at the resolved address. Patching
// code (for hooks) needs the page made writable first (VirtualProtect on the
// real target); on the host build the protect calls are no-ops so the byte
// logic can be unit-tested against ordinary buffers.
#pragma once
#include <sasdk/core/config.hpp>
#include <sasdk/core/address.hpp>
#include <cstdint>
#include <cstring>

#if SASDK_TARGET_ABI && defined(_WIN32)
  #include <windows.h>
#endif

namespace sasdk::rt::mem {

// Typed read/write at a logical VA (in-process; the pointer is live).
template <class T>
T read(Address a) {
    T v;
    std::memcpy(&v, a.ptr(), sizeof(T));
    return v;
}

template <class T>
void write(Address a, const T& v) {
    std::memcpy(a.ptr(), &v, sizeof(T));
}

// Make [addr, addr+size) writable+executable, returning the previous
// protection so it can be restored. On host builds this is a no-op that
// reports success, letting hook byte-encoding be tested without an OS.
struct ProtectGuard {
    void*        addr = nullptr;
    std::size_t  size = 0;
#if SASDK_TARGET_ABI && defined(_WIN32)
    DWORD        old  = 0;
    bool         active = false;

    ProtectGuard(void* a, std::size_t n) : addr(a), size(n) {
        active = VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &old) != 0;
    }
    ~ProtectGuard() {
        if (active) {
            DWORD tmp;
            VirtualProtect(addr, size, old, &tmp);
            FlushInstructionCache(GetCurrentProcess(), addr, size);
        }
    }
    bool ok() const { return active; }
#else
    ProtectGuard(void* a, std::size_t n) : addr(a), size(n) {}
    bool ok() const { return true; }   // host: pretend success for logic tests
#endif
    ProtectGuard(const ProtectGuard&) = delete;
    ProtectGuard& operator=(const ProtectGuard&) = delete;
};

// Copy raw bytes into game memory, flipping protection as needed.
inline bool patch(void* dst, const void* src, std::size_t n) {
    ProtectGuard g(dst, n);
    if (!g.ok()) return false;
    std::memcpy(dst, src, n);
    return true;
}

inline bool patch(Address a, const void* src, std::size_t n) {
    return patch(a.ptr(), src, n);
}

} // namespace sasdk::rt::mem
