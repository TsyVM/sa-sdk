// SPDX-License-Identifier: MIT
//
// sasdk/game/sa10us/sa10us_easy.hpp
// =================================
// The Level-1 API: zero-setup one-liners for the most common live-mod tasks.
//
// The explicit API (Game::init, CVehicleRef, Global<T>, call_method<...>, the
// generated structs) is always there for full control. This header is the
// "I just want to do the thing" layer on top of it: ergonomic *views* that
// return assignable references straight into a live game object, each a 1:1
// alias over a VERIFIED offset from the generated struct database.
//
//   #include <sasdk/mod.hpp>          // pulls this in
//   using namespace sasdk;
//
//   sa10us::vehicle(car).health()   = 1000.0f;     // repair
//   sa10us::vehicle(car).mass()    *= 0.5f;         // half weight
//   sa10us::ped(guy).armour()       = 100.0f;       // full armour
//   if (auto* v = sa10us::player_vehicle())         // process-defaulted
//       sa10us::vehicle(v).health() = 2000.0f;
//
// The views are pure pointer arithmetic (available on any build). The
// process-defaulted helpers (player_vehicle/ped, patch) touch the live game.
#ifndef SASDK_GAME_SA10US_EASY_HPP
#define SASDK_GAME_SA10US_EASY_HPP

#include <cstdint>
#include <initializer_list>
#include <sasdk/core/address.hpp>
#include <sasdk/core/memory.hpp>
#include <sasdk/game/sa10us/sa10us_db.inl>
#include <sasdk/game/sa10us/addresses.hpp>
#include <sasdk/game/sa10us/functions.inl>

namespace sasdk::sa10us {

// ---- ergonomic views -------------------------------------------------------
// Each wraps a live object pointer and hands back references into it. Construct
// one per use; they own nothing. Fields are reached through the base-class
// structs so inherited members (position, model index, mass) work on any
// derived object -- a CVehicle* is a CEntity*/CPhysical* at offset 0.

class EntityView {
public:
    explicit EntityView(void* p) : p_(p) {}
    std::uint16_t& model()  { return e()->m_nModelIndex; }   // +0x22 VERIFIED
    std::uint32_t& flags()  { return e()->m_nFlags; }        // +0x1C VERIFIED
    void*          rw()     { return reinterpret_cast<void*>(static_cast<std::uintptr_t>(e()->m_pRwObject)); }
protected:
    CEntity* e() const { return reinterpret_cast<CEntity*>(p_); }
    void* p_;
};

class PhysicalView : public EntityView {
public:
    explicit PhysicalView(void* p) : EntityView(p) {}
    float& mass()        { return ph()->m_fMass; }           // +0x8C VERIFIED
    float& turn_mass()   { return ph()->m_fTurnMass; }       // +0x90 VERIFIED
    float* move_speed()  { return ph()->m_vecMoveSpeed.data(); }   // +0x44 VERIFIED (x,y,z)
    float* turn_speed()  { return ph()->m_vecTurnSpeed.data(); }   // +0x50 VERIFIED
protected:
    CPhysical* ph() const { return reinterpret_cast<CPhysical*>(p_); }
};

class VehicleView : public PhysicalView {
public:
    explicit VehicleView(void* p) : PhysicalView(p) {}
    float& health()          { return v()->m_fHealth; }      // +0x4C0 VERIFIED (cold)
    bool   wrecked() const   { return reinterpret_cast<CVehicle*>(p_)->m_fHealth <= 0.0f; }
    void   fix()             { fn::CVehicle_Fix(p_); }        // engine repair, __thiscall
private:
    CVehicle* v() const { return reinterpret_cast<CVehicle*>(p_); }
};

class PedView : public PhysicalView {
public:
    explicit PedView(void* p) : PhysicalView(p) {}
    float& health()     { return p()->m_fHealth; }           // +0x540 VERIFIED
    float& max_health() { return p()->m_fMaxHealth; }        // +0x544 VERIFIED
    float& armour()     { return p()->m_fArmour; }           // +0x548 VERIFIED
private:
    CPed* p() const { return reinterpret_cast<CPed*>(p_); }
};

// ---- one-liner constructors ------------------------------------------------
[[nodiscard]] inline VehicleView  vehicle(void* obj)  noexcept { return VehicleView{obj}; }
[[nodiscard]] inline PedView      ped(void* obj)       noexcept { return PedView{obj}; }
[[nodiscard]] inline EntityView   entity(void* obj)    noexcept { return EntityView{obj}; }
[[nodiscard]] inline PhysicalView physical(void* obj)  noexcept { return PhysicalView{obj}; }

// ---- process-defaulted helpers (touch the live game) -----------------------
[[nodiscard]] inline CVehicle* player_vehicle(int player = -1) { return fn::FindPlayerVehicle(player, false); }
[[nodiscard]] inline CPed*     player_ped(int player = -1)      { return fn::FindPlayerPed(player); }

// One-shot byte poke at a logical VA (permanent). Returns false on failure.
inline bool poke(rt::Address at, std::initializer_list<std::uint8_t> bytes) {
    return rt::mem::patch(at, bytes.begin(), bytes.size());
}

} // namespace sasdk::sa10us

#endif // SASDK_GAME_SA10US_EASY_HPP
