// SPDX-License-Identifier: MIT
//
// sasdk/sasdk.hpp
// ===============
// Umbrella header. #include <sasdk/sasdk.hpp> in your ASI plugin to get
// the full SASDK surface: struct database, runtime hooks/invocation, typed
// game globals, ergonomic views, and the mod-entry macro.
//
// For host/CI builds this still compiles cleanly; calling-convention
// keywords vanish on non-i686 (see sasdk/core/config.hpp), and the runtime
// path that touches live game memory is not exercised.
#pragma once

// ---- core layer (address math, memory, hooks, invocation) ------------------
#include <sasdk/core/config.hpp>
#include <sasdk/core/result.hpp>
#include <sasdk/core/address.hpp>
#include <sasdk/core/memory.hpp>
#include <sasdk/core/global.hpp>
#include <sasdk/core/hook.hpp>
#include <sasdk/core/invoke.hpp>

// ---- generated struct database (302 game structs, byte-packed, asserted) ---
#include <sasdk/game/sa10us/sa10us_db.inl>

// ---- generated function catalogue (typed callable handles) -----------------
#include <sasdk/game/sa10us/functions.inl>

// ---- curated runtime surfaces (verified VAs, typed globals, helpers) -------
#include <sasdk/game/sa10us/addresses.hpp>
#include <sasdk/game/sa10us/game.hpp>
#include <sasdk/game/sa10us/vehicle.hpp>
#include <sasdk/game/sa10us/sa10us_easy.hpp>

// ---- entry-point locators (find live objects by known VA) ------------------
#include <sasdk/game/sa10us/sa10us_locators.hpp>

// ---- cross-process read/write (out-of-process tools and CI builds) ---------
#include <sasdk/game/process/process.hpp>

// ---- mod-entry macro (DllMain + background thread wiring) -----------------
#include <sasdk/mod.hpp>
