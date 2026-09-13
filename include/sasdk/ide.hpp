// SPDX-License-Identifier: MIT
// include/sasdk/ide.hpp — GTA:SA item definition file parser (sa10us offline, SASDK::data)
//
// IDE files are plain-text asset definition tables shipped in data/maps/ and data/.
// They define every model/texture pairing, draw distances, and flags known to the game.
// The game loads them at startup through CFileLoader::LoadObjectTypes.
//
// Sections (case-insensitive):
//   objs   — static world objects     id, model, txd, meshcount, dist[0..n-1], flags
//   tobj   — time-gated objects       ... drawdist, time_on, time_off, flags
//   weap   — weapon pickups           id, model, txd, meshcount, dist, anim, flags
//   hier   — hierarchy/clump objects  id, model, txd
//   anim   — animated objects         id, model, txd, anim, dist, flags
//   2dfx   — 2D billboard/light fx    id, x, y, z, r, g, b, a, type, <type data>
//
// Line syntax: comma-separated fields, whitespace trimmed, # = line comment.
// meshcount (objs / tobj / weap) is 1–3; that many drawdist values follow it.
// We capture the first drawdist only — it is always present.
#pragma once
#include <sasdk/core/result.hpp>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace sasdk {

// Static world object (objs).
struct IdeObject {
    int32_t  id;
    char     model_name[24];
    char     txd_name[24];
    float    draw_dist;   // primary (first) draw distance
    uint32_t flags;
};

// Time-gated object (tobj) — extends IdeObject with active hours.
struct IdeTimeObject : IdeObject {
    uint8_t time_on;    // hour game clock when model becomes visible (0-23)
    uint8_t time_off;   // hour game clock when model disappears (0-23)
};

// Weapon pickup model (weap).
struct IdeWeapon {
    int32_t  id;
    char     model_name[24];
    char     txd_name[24];
    char     anim_name[24];
    float    draw_dist;
    uint32_t flags;
};

// Hierarchy / clump (hier).
struct IdeHier {
    int32_t id;
    char    model_name[24];
    char    txd_name[24];
};

// Animated object (anim).
struct IdeAnim {
    int32_t  id;
    char     model_name[24];
    char     txd_name[24];
    char     anim_name[24];
    float    draw_dist;
    uint32_t flags;
};

// Parsed IDE file.
class IdeArchive {
public:
    // Parse a complete IDE file given as a string_view over its text.
    // Blank lines and lines beginning with '#' are ignored.
    static Result<IdeArchive> parse(std::string_view text) noexcept;

    // Span access over each section.
    std::span<const IdeObject>     objects()      const noexcept { return objs_; }
    std::span<const IdeTimeObject> time_objects() const noexcept { return tobjs_; }
    std::span<const IdeWeapon>     weapons()      const noexcept { return weaps_; }
    std::span<const IdeHier>       hiers()        const noexcept { return hiers_; }
    std::span<const IdeAnim>       anims()        const noexcept { return anims_; }

    // Find by numeric model ID.  Returns nullptr if not found.
    const IdeObject*     find_object(int32_t id)      const noexcept;
    const IdeTimeObject* find_time_object(int32_t id) const noexcept;
    const IdeWeapon*     find_weapon(int32_t id)      const noexcept;
    const IdeHier*       find_hier(int32_t id)        const noexcept;
    const IdeAnim*       find_anim(int32_t id)        const noexcept;

    // Find a model by name (case-insensitive) across all sections.
    // Returns {ptr, section_tag} where section_tag is one of "objs","tobj","weap","hier","anim".
    // Returns {nullptr, ""} if not found.
    const void*    find_by_name(std::string_view model_name,
                                std::string_view& out_section) const noexcept;

    size_t total_entries() const noexcept {
        return objs_.size() + tobjs_.size() + weaps_.size() + hiers_.size() + anims_.size();
    }

private:
    IdeArchive() = default;
    std::vector<IdeObject>     objs_;
    std::vector<IdeTimeObject> tobjs_;
    std::vector<IdeWeapon>     weaps_;
    std::vector<IdeHier>       hiers_;
    std::vector<IdeAnim>       anims_;
};

} // namespace sasdk
