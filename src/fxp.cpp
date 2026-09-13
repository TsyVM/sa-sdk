// SPDX-License-Identifier: MIT
// src/fxp.cpp — FXP particle project file parser implementation
#include <sasdk/fxp.hpp>
#include <charconv>
#include <cstring>
#include <algorithm>

namespace sasdk {

// ---------------------------------------------------------------------------
// Line scanner
// ---------------------------------------------------------------------------
namespace {

struct Scanner {
    std::vector<std::string_view> lines;
    size_t pos{};

    // Returns next non-empty, trimmed line, or empty sv when exhausted.
    std::string_view peek() const noexcept {
        size_t i = pos;
        while (i < lines.size() && trim(lines[i]).empty()) ++i;
        return i < lines.size() ? trim(lines[i]) : std::string_view{};
    }

    std::string_view next() noexcept {
        while (pos < lines.size()) {
            auto l = trim(lines[pos++]);
            if (!l.empty()) return l;
        }
        return {};
    }

    bool done() const noexcept { return pos >= lines.size(); }

    static std::string_view trim(std::string_view s) noexcept {
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r'))
            s.remove_prefix(1);
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r'))
            s.remove_suffix(1);
        return s;
    }
};

// Split text into lines, stripping CR.
std::vector<std::string_view> split_lines(std::string_view text) noexcept {
    std::vector<std::string_view> out;
    while (!text.empty()) {
        size_t nl = text.find('\n');
        if (nl == std::string_view::npos) {
            out.push_back(text);
            break;
        }
        out.push_back(text.substr(0, nl));
        text.remove_prefix(nl + 1);
    }
    return out;
}

// Parse " VALUE" after a tag on the same line or from the next line.
std::string_view value_of(std::string_view line, std::string_view tag) noexcept {
    if (line.size() > tag.size() && line.substr(0, tag.size()) == tag)
        return Scanner::trim(line.substr(tag.size()));
    return {};
}

// Read a float from a string_view.
float parse_float(std::string_view s) noexcept {
    float v = 0.f;
    std::from_chars(s.data(), s.data() + s.size(), v);
    return v;
}

int parse_int(std::string_view s) noexcept {
    int v = 0;
    std::from_chars(s.data(), s.data() + s.size(), v);
    return v;
}

// Read "TAG: VALUE" — returns trimmed value portion.
// Line must already be trimmed.
std::string_view tagged_value(std::string_view line) noexcept {
    size_t colon = line.find(':');
    if (colon == std::string_view::npos) return {};
    return Scanner::trim(line.substr(colon + 1));
}

// Read a "KEY VALUE" scalar line — value is everything after first space.
std::string_view scalar_value(std::string_view line) noexcept {
    size_t sp = line.find(' ');
    if (sp == std::string_view::npos) return {};
    return Scanner::trim(line.substr(sp + 1));
}

// ---------------------------------------------------------------------------
// FxpKeyframe parser: "FX_KEYFLOAT_DATA:", TIME value, VAL value
// ---------------------------------------------------------------------------
FxpKeyframe parse_keyframe(Scanner& sc) {
    sc.next(); // "FX_KEYFLOAT_DATA:"
    auto tline = sc.next(); // "TIME <val>"
    auto vline = sc.next(); // "VAL <val>"
    FxpKeyframe kf{};
    kf.time = parse_float(scalar_value(tline));
    kf.val  = parse_float(scalar_value(vline));
    return kf;
}

// ---------------------------------------------------------------------------
// FxpCurve parser: curveName ":", "FX_INTERP_DATA:", LOOPED, NUM_KEYS, keyframes
// ---------------------------------------------------------------------------
FxpCurve parse_curve(Scanner& sc, std::string_view name) {
    FxpCurve c;
    c.name = std::string(name);
    sc.next(); // "FX_INTERP_DATA:"
    auto looped_line = sc.next(); // "LOOPED <n>"
    c.looped = (parse_int(scalar_value(looped_line)) != 0);
    auto nk_line = sc.next(); // "NUM_KEYS: n"
    int nkeys = parse_int(tagged_value(nk_line));
    c.keys.reserve(nkeys);
    for (int i = 0; i < nkeys; ++i)
        c.keys.push_back(parse_keyframe(sc));
    return c;
}

// ---------------------------------------------------------------------------
// FxpInfo parser
// FX_INFO_<TYPE>_DATA: already consumed; pass type_name.
// Scalars: lines whose key is not a curve trigger (no FX_INTERP_DATA next).
// Curves: "CURVENAME:" line immediately followed by "FX_INTERP_DATA:".
// ---------------------------------------------------------------------------
FxpInfo parse_info(Scanner& sc, std::string_view type_name) {
    FxpInfo info;
    info.type_name = std::string(type_name);

    while (!sc.done()) {
        std::string_view p = sc.peek();
        // Stop at the next FX_ block or known section markers
        if (p.starts_with("FX_INFO_") || p.starts_with("FX_PRIM_") ||
            p.starts_with("FX_SYSTEM_") || p.starts_with("FX_PROJECT_") ||
            p.starts_with("LODSTART") || p.starts_with("NUM_INFOS") ||
            p.starts_with("NUM_PRIMS") || p.starts_with("OMITTEXTURES"))
            break;

        // A curve line ends with ':' and is followed by FX_INTERP_DATA.
        // A scalar is everything else.
        if (!p.empty() && p.back() == ':') {
            // Consume the curve-name line
            auto curve_tag_line = sc.next();
            // Strip trailing ':'
            std::string_view curve_name = curve_tag_line.substr(0, curve_tag_line.size() - 1);
            // Check next is FX_INTERP_DATA
            if (sc.peek().starts_with("FX_INTERP_DATA")) {
                info.curves.push_back(parse_curve(sc, curve_name));
            } else {
                // Treat as scalar (shouldn't happen per spec, but be robust)
                info.scalars.emplace_back(curve_tag_line);
            }
        } else {
            info.scalars.emplace_back(sc.next());
        }
    }
    return info;
}

// ---------------------------------------------------------------------------
// FxpPrimitive parser
// ---------------------------------------------------------------------------
FxpPrimitive parse_prim(Scanner& sc) {
    FxpPrimitive p;
    sc.next(); // "FX_PRIM_EMITTER_DATA:"
    sc.next(); // "FX_PRIM_BASE_DATA:"

    // NAME, MATRIX (4 lines: one header + values?), TEXTURE, TEXTURE2..4, ALPHAON, SRC, DST
    // The grammar shows these as simple "KEY VALUE" lines.

    p.name     = std::string(scalar_value(sc.next())); // NAME <val>
    // MATRIX: 4 lines (header "MATRIX" then 4 float-vector lines)
    sc.next(); // "MATRIX"
    sc.next(); sc.next(); sc.next(); sc.next(); // 4 row lines
    p.texture  = std::string(scalar_value(sc.next())); // TEXTURE
    p.texture2 = std::string(scalar_value(sc.next())); // TEXTURE2
    p.texture3 = std::string(scalar_value(sc.next())); // TEXTURE3
    p.texture4 = std::string(scalar_value(sc.next())); // TEXTURE4
    p.alpha_on      = parse_int(scalar_value(sc.next())); // ALPHAON
    p.src_blend_id  = parse_int(scalar_value(sc.next())); // SRCBLENDID
    p.dst_blend_id  = parse_int(scalar_value(sc.next())); // DSTBLENDID

    auto ni_line = sc.next(); // "NUM_INFOS: n"
    int n_infos = parse_int(tagged_value(ni_line));

    for (int i = 0; i < n_infos; ++i) {
        auto type_line = sc.next(); // "FX_INFO_<TYPE>_DATA:"
        p.infos.push_back(parse_info(sc, type_line));
    }

    p.lod_start = parse_float(scalar_value(sc.next())); // LODSTART
    p.lod_end   = parse_float(scalar_value(sc.next())); // LODEND
    return p;
}

// ---------------------------------------------------------------------------
// FxpSystem parser
// ---------------------------------------------------------------------------
FxpSystem parse_system(Scanner& sc) {
    FxpSystem sys;
    sc.next(); // "FX_SYSTEM_DATA:"
    sys.version           = parse_int(sc.next());
    sys.filename          = std::string(scalar_value(sc.next())); // FILENAME
    sys.name              = std::string(scalar_value(sc.next())); // NAME
    sys.loop_interval_min = parse_float(scalar_value(sc.next())); // LOOPINTERVALMIN
    sys.loop_interval_max = parse_float(scalar_value(sc.next())); // LOOPINTERVALMAX (LENGTH)
    sys.play_mode         = parse_int(scalar_value(sc.next()));   // PLAYMODE
    sys.cull_dist         = parse_float(scalar_value(sc.next())); // CULLDIST
    sys.bounding_sphere   = std::string(scalar_value(sc.next())); // BOUNDINGSPHERE

    auto np_line = sc.next(); // "NUM_PRIMS: n"
    int n_prims = parse_int(tagged_value(np_line));
    sys.prims.reserve(n_prims);
    for (int i = 0; i < n_prims; ++i)
        sys.prims.push_back(parse_prim(sc));

    sys.omit_textures = parse_int(scalar_value(sc.next())); // OMITTEXTURES
    sys.txd_name      = std::string(scalar_value(sc.next())); // TXDNAME
    return sys;
}

} // namespace

// ---------------------------------------------------------------------------
// FxpArchive::parse
// ---------------------------------------------------------------------------
Result<FxpArchive> FxpArchive::parse(std::string_view text) noexcept {
    if (text.empty())
        return err(ErrorKind::BadFormat, "fxp: empty input");

    Scanner sc;
    sc.lines = split_lines(text);

    // Find and consume "FX_PROJECT_DATA:"
    bool found_header = false;
    while (!sc.done()) {
        auto l = sc.next();
        if (l == "FX_PROJECT_DATA:") { found_header = true; break; }
    }
    if (!found_header)
        return err(ErrorKind::BadFormat, "fxp: missing FX_PROJECT_DATA:");

    FxpArchive fa;
    while (!sc.done()) {
        auto p = sc.peek();
        if (p == "FX_PROJECT_DATA_END:") { sc.next(); break; }
        if (p.starts_with("FX_SYSTEM_DATA:"))
            fa.proj_.systems.push_back(parse_system(sc));
        else
            sc.next(); // skip unexpected lines
    }

    if (fa.proj_.systems.empty())
        return err(ErrorKind::BadFormat, "fxp: no systems parsed");

    return fa;
}

const FxpSystem* FxpArchive::find_system(std::string_view name) const noexcept {
    for (const FxpSystem& s : proj_.systems)
        if (s.name == name) return &s;
    return nullptr;
}

} // namespace sasdk
