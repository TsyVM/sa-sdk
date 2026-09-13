// SPDX-License-Identifier: MIT
// include/sasdk/core/pool.hpp — CPool<T> read-only view for live game pools
//
// CPool<T> internal layout (verified cold from CPools::GetRef at 0x54F420):
//   offset 0: T*       m_pObjects  — contiguous object array
//   offset 4: uint8_t* m_byteMap   — parallel flag array (one byte per slot)
//
// byteMap byte encoding (verified at 0x54F441):
//   bit 7 = 1  →  slot is FREE (skip it)
//   bit 7 = 0  →  slot is LIVE (object valid)
//   bits 6..0  →  allocation counter (ABA protection for handles)
//
// Handle encoding (verified at 0x54F441–0x54F448):
//   handle = (slot_index << 8) | byteMap[slot_index]
//
// Known pool globals (sa10us VA, verified by disassembly):
//   CPools::ms_pPedPool     → 0xB74490   capacity 140   (CPed)
//   CPools::ms_pVehiclePool → 0xB74494   capacity 110   (CVehicle)
//   CPools::ms_pObjectPool  → 0xB7449C   capacity 350   (CObject)
//   CBuilding pool          → 0xB74490   capacity 13000 (CBuilding)
//
// PoolView<T> is deliberately read-only — it borrows the pool's own memory
// and never allocates.  It is a runtime-only type; compile with SASDK_RUNTIME.
#pragma once
#include <cstddef>
#include <cstdint>

#ifdef SASDK_RUNTIME
namespace sasdk {

// Read-only typed view over a live CPool<T>.
// Construct once per frame (or per use); the pool pointer is stable across frames.
template<typename T>
class PoolView {
public:
    // pool_ptr — the CPool<T>* value read from the game's global variable
    // capacity — maximum slot count for this pool (hard limit from CPools::Initialise)
    explicit PoolView(void* pool_ptr, uint32_t capacity) noexcept
        : objects_(nullptr), byte_map_(nullptr), capacity_(capacity)
    {
        if (!pool_ptr) return;
        // CPool<T>: { T* m_pObjects @0, uint8_t* m_byteMap @4 }
        auto* raw = static_cast<void**>(pool_ptr);
        objects_  = static_cast<T*>(raw[0]);
        byte_map_ = static_cast<uint8_t*>(raw[1]);
    }

    bool valid() const noexcept { return objects_ && byte_map_; }

    // Number of live (in-use) slots.
    uint32_t count_live() const noexcept {
        if (!valid()) return 0;
        uint32_t n = 0;
        for (uint32_t i = 0; i < capacity_; ++i)
            n += ((byte_map_[i] & 0x80u) == 0u) ? 1u : 0u;
        return n;
    }

    // Iterate every live slot. fn(T& obj) — return false to stop early.
    template<typename Fn>
    void for_each(Fn&& fn) const noexcept {
        if (!valid()) return;
        for (uint32_t i = 0; i < capacity_; ++i)
            if ((byte_map_[i] & 0x80u) == 0u)
                if (!fn(objects_[i])) return;
    }

    // Get a pointer to the object at slot index, or nullptr if the slot is free.
    T* at(uint32_t slot) const noexcept {
        if (!valid() || slot >= capacity_) return nullptr;
        if (byte_map_[slot] & 0x80u) return nullptr;
        return &objects_[slot];
    }

    // Validate a pool handle.  Returns the object or nullptr.
    // handle = (slot << 8) | byteMap[slot]
    T* from_handle(uint32_t handle) const noexcept {
        uint32_t slot = handle >> 8;
        if (slot >= capacity_) return nullptr;
        if (byte_map_[slot] != (handle & 0xFFu)) return nullptr;
        if (byte_map_[slot] & 0x80u) return nullptr;
        return &objects_[slot];
    }

    uint32_t capacity() const noexcept { return capacity_; }

private:
    T*       objects_;
    uint8_t* byte_map_;
    uint32_t capacity_;
};

// ---------------------------------------------------------------------------
// Typed pool accessors for the three main sa10us entity pools.
// Call after Module::bind().
// ---------------------------------------------------------------------------
namespace sa10us {

// Forward declarations — defined in sa10us_db.inl
struct CPed;
struct CVehicle;
struct CObject;

// CPeds — 140 slots @ 0xB74490
inline PoolView<CPed> ped_pool() noexcept {
    void** global = reinterpret_cast<void**>(0xB74490u);
    return PoolView<CPed>(*global, 140u);
}

// CVehicles — 110 slots @ 0xB74494
inline PoolView<CVehicle> vehicle_pool() noexcept {
    void** global = reinterpret_cast<void**>(0xB74494u);
    return PoolView<CVehicle>(*global, 110u);
}

// CObjects — 350 slots @ 0xB7449C
inline PoolView<CObject> object_pool() noexcept {
    void** global = reinterpret_cast<void**>(0xB7449Cu);
    return PoolView<CObject>(*global, 350u);
}

} // namespace sa10us
} // namespace sasdk
#endif // SASDK_RUNTIME
