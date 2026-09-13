// include/sasdk/game/sa10us/vehicle.hpp
//
// A thin, typed wrapper over a live CVehicle* -- the first real "do something
// to a game object" surface. It reads/writes the cold-verified m_fHealth field
// (offset 0x4C0, proven by the 1000.0f constant stored at three sites) and can
// call CVehicle::Fix() through the runtime __thiscall path.
//
// This is deliberately small: only fields/methods with real provenance are
// exposed. As CVehicle/CEntity/CPhysical gain cold-verified fields, this
// wrapper grows -- never ahead of the evidence.
#pragma once
#include <sasdk/core/address.hpp>
#include <sasdk/core/invoke.hpp>
#include <sasdk/game/sa10us/addresses.hpp>
#include <sasdk/game/sa10us/sa10us_db.inl>
#include <cstdint>

namespace sasdk::sa10us {

// Wraps a pointer to a live CVehicle in the game's memory. Non-owning.
class CVehicleRef {
public:
    explicit CVehicleRef(CVehicle* p) : p_(p) {}
    explicit CVehicleRef(void* p) : p_(reinterpret_cast<CVehicle*>(p)) {}

    CVehicle* raw() const { return p_; }
    explicit operator bool() const { return p_ != nullptr; }

    // Base-class (CEntity) view: a CVehicle* is a CEntity* at offset 0.
    CEntity* entity() const { return reinterpret_cast<CEntity*>(p_); }
    // m_nModelIndex @ 0x22 -- VERIFIED cold (CEntity::SetModelIndexNoCreate).
    std::uint16_t model_index() const { return entity()->m_nModelIndex; }
    // m_nFlags @ 0x1C -- VERIFIED cold. e.g. bit for visibility etc.
    std::uint32_t entity_flags() const { return entity()->m_nFlags; }

    // Health -- VERIFIED cold at +0x4C0. Full = 1000.0f; <= 0 triggers the
    // explode path. Accessed through the generated struct member so the
    // static_assert-checked layout is the single source of the offset.
    float health() const { return p_->m_fHealth; }
    void  set_health(float hp) { p_->m_fHealth = hp; }
    bool  is_wrecked() const { return p_->m_fHealth <= 0.0f; }

    // CVehicle::Fix() -- repair to full (address 0x6D6390); it is the function
    // whose body resets m_fHealth to 1000.0f.
    // Virtual in the engine, but the concrete base address is known, so we call
    // it directly via __thiscall.
    void fix() {
        sasdk::rt::MemberFunction<void, CVehicle> fn(addr::CVehicle_Fix);
        fn(p_);
    }

private:
    CVehicle* p_;
};

} // namespace sasdk::sa10us
