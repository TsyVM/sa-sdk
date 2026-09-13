// include/sasdk/game/process/process.hpp
//
// Phase 4: runtime process-attach / typed-read-write layer.
//
// Design constraints:
//   - Header-only (no .cpp required; all platform code is thin and inline).
//   - C++20, no exceptions, no global error state -- errors surface via
//     sasdk::Result<T> / sasdk::Status (see core/result.hpp).
//   - Pointer-width independence: the game is a 32-bit process; on a 64-bit
//     host we open it cross-process and all addresses are uint32_t (game VA).
//     We never cast game addresses to native pointers.
//   - Linux-only for now (ptrace / /proc/<pid>/mem). A Windows backend
//     (ReadProcessMemory) is stubbed but not compiled in; add
//     SASDK_BACKEND_WINDOWS when the time comes.
//
// Usage:
//   auto proc = sasdk::Process::attach(pid);
//   if (!proc) { /* proc.error() */ }
//
//   // Read a verified struct field:
//   auto health = proc->read<float>(vehicle_va + offsetof(CVehicle, m_fHealth));
//   if (!health) { /* health.error() */ }
//
//   // Write a field:
//   auto st = proc->write<float>(vehicle_va + offsetof(CVehicle, m_fHealth), 1000.f);
//
//   // Read a whole struct (zero-copy into caller's buffer):
//   CVehicle veh;
//   auto st = proc->read_struct(vehicle_va, veh);
//
// Rebase:
//   gta_sa.exe (HOODLUM) loads at its preferred base 0x400000 and never
//   relocates, so no rebase is needed for the 1.0 US build. The layer
//   exposes process_base() anyway for forward-compatibility.
#pragma once

#include <sasdk/core/result.hpp>
#include <cstdint>
#include <cstring>
#include <string>

// ---- platform selection ------------------------------------------------
#if defined(_WIN32)
#  define SASDK_BACKEND_WINDOWS 1
#elif defined(__linux__)
#  define SASDK_BACKEND_LINUX 1
#else
#  error "sasdk/game/process/process.hpp: unsupported platform (add a backend)"
#endif

// Linux headers (included only when compiling on Linux)
#if defined(SASDK_BACKEND_LINUX)
#  include <sys/types.h>
#  include <sys/uio.h>       // process_vm_readv / process_vm_writev
#  include <sys/ptrace.h>
#  include <sys/wait.h>
#  include <cerrno>
#  include <cstdio>          // snprintf
#  include <fcntl.h>
#  include <unistd.h>
#endif

#if defined(SASDK_BACKEND_WINDOWS)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

namespace sasdk {

// game VA type -- always 32-bit regardless of host pointer width
using gva_t = uint32_t;

// GTASA 1.0 US preferred image base (never relocated in HOODLUM build)
inline constexpr gva_t SA10US_IMAGE_BASE = 0x00400000u;

// -----------------------------------------------------------------------
// ProcessMemory -- cross-process read/write primitives
// -----------------------------------------------------------------------
class Process {
public:
    // Not copyable; move-only (owns the OS handle).
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    Process(Process&& o) noexcept;
    Process& operator=(Process&& o) noexcept;
    ~Process();

    // Attach to a running gta_sa.exe by PID.
    // Returns an owning Process or an Error.
    static Result<Process> attach(
#if defined(SASDK_BACKEND_LINUX)
        pid_t pid,
#elif defined(SASDK_BACKEND_WINDOWS)
        DWORD pid,
#endif
        gva_t expected_base = SA10US_IMAGE_BASE
    );

    // Read sizeof(T) bytes from game VA `addr` into a T.
    // T must be trivially copyable (enforced by static_assert in body).
    template <typename T>
    Result<T> read(gva_t addr) const {
        static_assert(std::is_trivially_copyable_v<T>,
            "sasdk::Process::read<T>: T must be trivially copyable");
        T value{};
        auto st = read_raw(addr, &value, sizeof(T));
        if (!st) return st.error();
        return value;
    }

    // Write sizeof(T) bytes to game VA `addr` from value.
    template <typename T>
    Status write(gva_t addr, const T& value) const {
        static_assert(std::is_trivially_copyable_v<T>,
            "sasdk::Process::write<T>: T must be trivially copyable");
        return write_raw(addr, &value, sizeof(T));
    }

    // Read an entire struct S from game VA `addr` into `out`.
    // Convenience wrapper around read_raw so callers don't repeat sizeof.
    template <typename S>
    Status read_struct(gva_t addr, S& out) const {
        static_assert(std::is_trivially_copyable_v<S>,
            "sasdk::Process::read_struct<S>: S must be trivially copyable");
        return read_raw(addr, &out, sizeof(S));
    }

    // The actual image base as observed in the running process.
    // For GTASA 1.0 US (HOODLUM) this is always 0x400000.
    gva_t process_base() const { return base_; }

    // Translate a static game VA to a rebased one (identity for this build,
    // but present for forward-compatibility if a future build relocates).
    gva_t rebase(gva_t static_va) const {
        return static_va - SA10US_IMAGE_BASE + base_;
    }

    // PID of the attached process.
#if defined(SASDK_BACKEND_LINUX)
    pid_t pid() const { return pid_; }
#elif defined(SASDK_BACKEND_WINDOWS)
    DWORD pid() const { return pid_; }
#endif

private:
#if defined(SASDK_BACKEND_LINUX)
    explicit Process(pid_t pid, gva_t base);
    pid_t pid_   = -1;
#elif defined(SASDK_BACKEND_WINDOWS)
    explicit Process(DWORD pid, HANDLE handle, gva_t base);
    DWORD  pid_    = 0;
    HANDLE handle_ = INVALID_HANDLE_VALUE;
#endif
    gva_t base_ = SA10US_IMAGE_BASE;

    Status read_raw(gva_t addr, void* buf, size_t len) const;
    Status write_raw(gva_t addr, const void* buf, size_t len) const;
};


// -----------------------------------------------------------------------
// Implementation -- Linux backend
// -----------------------------------------------------------------------
#if defined(SASDK_BACKEND_LINUX)

inline Process::Process(pid_t pid, gva_t base) : pid_(pid), base_(base) {}

inline Process::Process(Process&& o) noexcept : pid_(o.pid_), base_(o.base_) {
    o.pid_ = -1;
}
inline Process& Process::operator=(Process&& o) noexcept {
    if (this != &o) { pid_ = o.pid_; base_ = o.base_; o.pid_ = -1; }
    return *this;
}
inline Process::~Process() {
    // We do NOT detach (ptrace DETACH) automatically: the caller may have
    // only opened /proc/<pid>/mem for reads and never called PTRACE_ATTACH.
    // Detaching blindly would kill a game the user didn't ask us to stop.
}

inline Result<Process> Process::attach(pid_t pid, gva_t expected_base) {
    // Try process_vm_readv first (no ptrace stop needed, Linux 3.2+).
    // Do a probe read of 4 bytes at the expected base to confirm the process
    // is alive and reachable before handing back a Process object.
    uint32_t probe = 0;
    struct iovec local  { &probe, sizeof(probe) };
    struct iovec remote { reinterpret_cast<void*>(static_cast<uintptr_t>(expected_base)), sizeof(probe) };
    ssize_t n = process_vm_readv(pid, &local, 1, &remote, 1, 0);
    if (n != static_cast<ssize_t>(sizeof(probe))) {
        // probe failed -- process not accessible or wrong base
        char msg[128];
        std::snprintf(msg, sizeof(msg),
            "Process::attach(%d): probe read at 0x%08X failed (errno=%d)",
            static_cast<int>(pid), expected_base, errno);
        return err(ErrorKind::IoError, msg);
    }
    // MZ header check: first two bytes of a PE are 'MZ' (0x4D 0x5A)
    if ((probe & 0xFFFF) != 0x5A4D) {
        char msg[128];
        std::snprintf(msg, sizeof(msg),
            "Process::attach(%d): no MZ header at 0x%08X (got 0x%08X) -- "
            "wrong base or not a PE",
            static_cast<int>(pid), expected_base, probe);
        return err(ErrorKind::BadFormat, msg);
    }
    return Process(pid, expected_base);
}

inline Status Process::read_raw(gva_t addr, void* buf, size_t len) const {
    if (pid_ < 0) return err(ErrorKind::IoError, "Process not attached");
    struct iovec local  { buf,  len };
    struct iovec remote { reinterpret_cast<void*>(static_cast<uintptr_t>(addr)), len };
    ssize_t n = process_vm_readv(pid_, &local, 1, &remote, 1, 0);
    if (n != static_cast<ssize_t>(len)) {
        char msg[128];
        std::snprintf(msg, sizeof(msg),
            "process_vm_readv(%d, 0x%08X, %zu): got %zd (errno=%d)",
            static_cast<int>(pid_), addr, len, n, errno);
        return err(ErrorKind::IoError, msg);
    }
    return Status{};
}

inline Status Process::write_raw(gva_t addr, const void* buf, size_t len) const {
    if (pid_ < 0) return err(ErrorKind::IoError, "Process not attached");
    struct iovec local  { const_cast<void*>(buf), len };
    struct iovec remote { reinterpret_cast<void*>(static_cast<uintptr_t>(addr)), len };
    ssize_t n = process_vm_writev(pid_, &local, 1, &remote, 1, 0);
    if (n != static_cast<ssize_t>(len)) {
        char msg[128];
        std::snprintf(msg, sizeof(msg),
            "process_vm_writev(%d, 0x%08X, %zu): got %zd (errno=%d)",
            static_cast<int>(pid_), addr, len, n, errno);
        return err(ErrorKind::IoError, msg);
    }
    return Status{};
}

#endif // SASDK_BACKEND_LINUX


// -----------------------------------------------------------------------
// Implementation -- Windows backend (stub -- not yet exercised)
// -----------------------------------------------------------------------
#if defined(SASDK_BACKEND_WINDOWS)

inline Process::Process(DWORD pid, HANDLE handle, gva_t base)
    : pid_(pid), handle_(handle), base_(base) {}

inline Process::Process(Process&& o) noexcept
    : pid_(o.pid_), handle_(o.handle_), base_(o.base_) {
    o.handle_ = INVALID_HANDLE_VALUE;
}
inline Process& Process::operator=(Process&& o) noexcept {
    if (this != &o) {
        if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_);
        pid_ = o.pid_; handle_ = o.handle_; base_ = o.base_;
        o.handle_ = INVALID_HANDLE_VALUE;
    }
    return *this;
}
inline Process::~Process() {
    if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_);
}

inline Result<Process> Process::attach(DWORD pid, gva_t expected_base) {
    HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
                           FALSE, pid);
    if (h == NULL || h == INVALID_HANDLE_VALUE) {
        return err(ErrorKind::IoError,
            std::string("OpenProcess(") + std::to_string(pid) +
            ") failed: " + std::to_string(GetLastError()));
    }
    // Probe MZ header at expected_base
    uint32_t probe = 0;
    SIZE_T read_bytes = 0;
    if (!ReadProcessMemory(h, reinterpret_cast<LPCVOID>(static_cast<uintptr_t>(expected_base)),
                           &probe, sizeof(probe), &read_bytes)
        || read_bytes != sizeof(probe)) {
        CloseHandle(h);
        return err(ErrorKind::IoError,
            std::string("probe read at ") + std::to_string(expected_base) + " failed");
    }
    if ((probe & 0xFFFF) != 0x5A4D) {
        CloseHandle(h);
        return err(ErrorKind::BadFormat, "no MZ header at expected_base -- wrong base or not a PE");
    }
    return Process(pid, h, expected_base);
}

inline Status Process::read_raw(gva_t addr, void* buf, size_t len) const {
    if (handle_ == INVALID_HANDLE_VALUE)
        return err(ErrorKind::IoError, "Process not attached");
    SIZE_T read_bytes = 0;
    if (!ReadProcessMemory(handle_,
            reinterpret_cast<LPCVOID>(static_cast<uintptr_t>(addr)),
            buf, len, &read_bytes) || read_bytes != len) {
        return err(ErrorKind::IoError,
            std::string("ReadProcessMemory(0x") + std::to_string(addr) +
            ", " + std::to_string(len) + ") failed: " + std::to_string(GetLastError()));
    }
    return Status{};
}

inline Status Process::write_raw(gva_t addr, const void* buf, size_t len) const {
    if (handle_ == INVALID_HANDLE_VALUE)
        return err(ErrorKind::IoError, "Process not attached");
    SIZE_T written = 0;
    if (!WriteProcessMemory(handle_,
            reinterpret_cast<LPVOID>(static_cast<uintptr_t>(addr)),
            buf, len, &written) || written != len) {
        return err(ErrorKind::IoError,
            std::string("WriteProcessMemory(0x") + std::to_string(addr) +
            ", " + std::to_string(len) + ") failed: " + std::to_string(GetLastError()));
    }
    return Status{};
}

#endif // SASDK_BACKEND_WINDOWS

} // namespace sasdk
