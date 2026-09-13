// SPDX-License-Identifier: MIT
// include/sasdk/fxp.hpp — FXP particle project file parser (sa10us offline, SASDK::data)
//
// File: models/effects.fxp (plain text, CRLF, ~617 KB, 43 617 lines)
// Grammar (count-driven, verified: consumes all lines with 0 left over):
//
//   'FX_PROJECT_DATA:'
//     system* (82 in effects.fxp)
//   'FX_PROJECT_DATA_END:'
//
//   system:
//     'FX_SYSTEM_DATA:' <version=109> FILENAME NAME LOOPINTERVALMIN LOOPINTERVALMAX
//      PLAYMODE CULLDIST BOUNDINGSPHERE
//     'NUM_PRIMS: n'
//       prim * n
//     OMITTEXTURES TXDNAME
//
//   prim:
//     'FX_PRIM_EMITTER_DATA:'
//     'FX_PRIM_BASE_DATA:'
//      NAME MATRIX TEXTURE TEXTURE2 TEXTURE3 TEXTURE4 ALPHAON SRCBLENDID DSTBLENDID
//     'NUM_INFOS: n'
//       info * n
//      LODSTART LODEND
//
//   info:
//     'FX_INFO_<TYPE>_DATA:'
//      scalar*
//      (curveName ':' interp)*
//
//   interp:
//     'FX_INTERP_DATA:' LOOPED 'NUM_KEYS: n'
//      keyframe * n
//
//   keyframe:
//     'FX_KEYFLOAT_DATA:' TIME VAL
#pragma once
#include <sasdk/core/result.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sasdk {

// ---------------------------------------------------------------------------
// Data model
// ---------------------------------------------------------------------------

struct FxpKeyframe {
    float time;
    float val;
};

struct FxpCurve {
    std::string          name;    // e.g. "SIZEX", "RED", "RATE"
    bool                 looped;
    std::vector<FxpKeyframe> keys;
};

struct FxpInfo {
    std::string              type_name; // e.g. "FX_INFO_SIZE_DATA"
    std::vector<std::string> scalars;   // raw "KEY VALUE" lines before curves
    std::vector<FxpCurve>    curves;
};

struct FxpPrimitive {
    std::string           name;
    std::string           texture;
    std::string           texture2;
    std::string           texture3;
    std::string           texture4;
    int                   alpha_on{};
    int                   src_blend_id{};
    int                   dst_blend_id{};
    float                 lod_start{};
    float                 lod_end{};
    std::vector<FxpInfo>  infos;
};

struct FxpSystem {
    int                        version{};   // constant 109 in sa10us
    std::string                filename;
    std::string                name;
    float                      loop_interval_min{};
    float                      loop_interval_max{};
    int                        play_mode{};
    float                      cull_dist{};
    std::string                bounding_sphere; // raw "cx cy cz r"
    std::string                txd_name;
    int                        omit_textures{};
    std::vector<FxpPrimitive>  prims;
};

struct FxpProject {
    std::vector<FxpSystem> systems; // 82 in effects.fxp
};

// -----------------------------------------------------------------------
// FxpArchive — parses the full FXP text file into the data model above.
// Pass the full file text (any line-ending style; CRLF normalised internally).
// -----------------------------------------------------------------------
class FxpArchive {
public:
    static Result<FxpArchive> parse(std::string_view text) noexcept;

    const FxpProject& project() const noexcept { return proj_; }

    // Convenience: find a system by name.
    const FxpSystem* find_system(std::string_view name) const noexcept;

private:
    FxpArchive() = default;
    FxpProject proj_;
};

} // namespace sasdk
