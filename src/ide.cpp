// SPDX-License-Identifier: MIT
// src/ide.cpp — GTA:SA IDE parser implementation
#include <sasdk/ide.hpp>
#include <algorithm>
#include <charconv>
#include <cstring>
#include <string>
#include <vector>

namespace sasdk {
namespace {

// Split text into trimmed, non-empty lines.
static std::vector<std::string_view> split_lines(std::string_view text) {
    std::vector<std::string_view> out;
    while (!text.empty()) {
        auto nl = text.find('\n');
        auto line = text.substr(0, nl == std::string_view::npos ? text.size() : nl);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        // ltrim
        while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.remove_prefix(1);
        // rtrim
        while (!line.empty() && (line.back()  == ' ' || line.back()  == '\t')) line.remove_suffix(1);
        // strip comment
        auto hash = line.find('#');
        if (hash != std::string_view::npos) line = line.substr(0, hash);
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) line.remove_suffix(1);
        if (!line.empty()) out.push_back(line);
        if (nl == std::string_view::npos) break;
        text.remove_prefix(nl + 1);
    }
    return out;
}

// Split a line on commas, trimming each field.
static std::vector<std::string_view> split_csv(std::string_view line) {
    std::vector<std::string_view> out;
    while (true) {
        auto comma = line.find(',');
        auto field = line.substr(0, comma);
        while (!field.empty() && (field.front() == ' ' || field.front() == '\t')) field.remove_prefix(1);
        while (!field.empty() && (field.back()  == ' ' || field.back()  == '\t')) field.remove_suffix(1);
        out.push_back(field);
        if (comma == std::string_view::npos) break;
        line.remove_prefix(comma + 1);
    }
    return out;
}

static void copy_name(char* dst, size_t cap, std::string_view src) noexcept {
    size_t n = std::min(src.size(), cap - 1);
    std::memcpy(dst, src.data(), n);
    dst[n] = '\0';
}

static float parse_float(std::string_view sv) noexcept {
    float v = 0.f;
    std::from_chars(sv.data(), sv.data() + sv.size(), v);
    return v;
}

static int32_t parse_int(std::string_view sv) noexcept {
    int32_t v = 0;
    std::from_chars(sv.data(), sv.data() + sv.size(), v);
    return v;
}

static uint32_t parse_uint(std::string_view sv) noexcept {
    uint32_t v = 0;
    std::from_chars(sv.data(), sv.data() + sv.size(), v);
    return v;
}

static bool icase_eq(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        char ca = a[i], cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return false;
    }
    return true;
}

enum class Section { None, Objs, Tobj, Weap, Hier, Anim, Skip };

static Section section_from(std::string_view tag) noexcept {
    if (icase_eq(tag, "objs")) return Section::Objs;
    if (icase_eq(tag, "tobj")) return Section::Tobj;
    if (icase_eq(tag, "weap")) return Section::Weap;
    if (icase_eq(tag, "hier")) return Section::Hier;
    if (icase_eq(tag, "anim")) return Section::Anim;
    if (icase_eq(tag, "2dfx")) return Section::Skip;
    return Section::None;
}

} // namespace

Result<IdeArchive> IdeArchive::parse(std::string_view text) noexcept {
    IdeArchive arch;
    auto lines = split_lines(text);
    Section cur = Section::None;

    for (auto& line : lines) {
        if (icase_eq(line, "end")) { cur = Section::None; continue; }

        Section s = section_from(line);
        if (s != Section::None) { cur = s; continue; }

        if (cur == Section::None || cur == Section::Skip) continue;

        auto f = split_csv(line);

        if (cur == Section::Hier) {
            // id, modelname, txdname
            if (f.size() < 3) continue;
            IdeHier h{};
            h.id = parse_int(f[0]);
            copy_name(h.model_name, sizeof(h.model_name), f[1]);
            copy_name(h.txd_name,   sizeof(h.txd_name),   f[2]);
            arch.hiers_.push_back(h);
            continue;
        }

        // objs / tobj / weap / anim all have: id, model, txd, meshcount_or_animname, ...
        if (f.size() < 5) continue;

        if (cur == Section::Anim) {
            // id, model, txd, anim, dist, flags
            IdeAnim a{};
            a.id = parse_int(f[0]);
            copy_name(a.model_name, sizeof(a.model_name), f[1]);
            copy_name(a.txd_name,   sizeof(a.txd_name),   f[2]);
            copy_name(a.anim_name,  sizeof(a.anim_name),  f[3]);
            a.draw_dist = f.size() > 4 ? parse_float(f[4]) : 0.f;
            a.flags     = f.size() > 5 ? parse_uint(f[5])  : 0u;
            arch.anims_.push_back(a);
            continue;
        }

        // objs / tobj / weap: id, model, txd, meshcount, dist[0..meshcount-1], ...
        int32_t meshcount = parse_int(f[3]);
        if (meshcount < 1) meshcount = 1;
        if (meshcount > 3) meshcount = 3;
        size_t dist_start = 4;
        float  draw_dist  = f.size() > dist_start ? parse_float(f[dist_start]) : 0.f;
        size_t after_dist = dist_start + static_cast<size_t>(meshcount);

        if (cur == Section::Objs) {
            IdeObject o{};
            o.id        = parse_int(f[0]);
            copy_name(o.model_name, sizeof(o.model_name), f[1]);
            copy_name(o.txd_name,   sizeof(o.txd_name),   f[2]);
            o.draw_dist = draw_dist;
            o.flags     = f.size() > after_dist ? parse_uint(f[after_dist]) : 0u;
            arch.objs_.push_back(o);
        } else if (cur == Section::Tobj) {
            // ... dist, time_on, time_off, flags
            IdeTimeObject t{};
            t.id        = parse_int(f[0]);
            copy_name(t.model_name, sizeof(t.model_name), f[1]);
            copy_name(t.txd_name,   sizeof(t.txd_name),   f[2]);
            t.draw_dist = draw_dist;
            t.time_on   = f.size() > after_dist     ? static_cast<uint8_t>(parse_int(f[after_dist]))     : 0;
            t.time_off  = f.size() > after_dist + 1 ? static_cast<uint8_t>(parse_int(f[after_dist + 1])) : 0;
            t.flags     = f.size() > after_dist + 2 ? parse_uint(f[after_dist + 2]) : 0u;
            arch.tobjs_.push_back(t);
        } else if (cur == Section::Weap) {
            // id, model, txd, meshcount, dist, anim_name, flags
            IdeWeapon w{};
            w.id        = parse_int(f[0]);
            copy_name(w.model_name, sizeof(w.model_name), f[1]);
            copy_name(w.txd_name,   sizeof(w.txd_name),   f[2]);
            w.draw_dist = draw_dist;
            if (f.size() > after_dist)
                copy_name(w.anim_name, sizeof(w.anim_name), f[after_dist]);
            w.flags = f.size() > after_dist + 1 ? parse_uint(f[after_dist + 1]) : 0u;
            arch.weaps_.push_back(w);
        }
    }
    return arch;
}

const IdeObject* IdeArchive::find_object(int32_t id) const noexcept {
    for (const auto& o : objs_) if (o.id == id) return &o;
    return nullptr;
}
const IdeTimeObject* IdeArchive::find_time_object(int32_t id) const noexcept {
    for (const auto& o : tobjs_) if (o.id == id) return &o;
    return nullptr;
}
const IdeWeapon* IdeArchive::find_weapon(int32_t id) const noexcept {
    for (const auto& o : weaps_) if (o.id == id) return &o;
    return nullptr;
}
const IdeHier* IdeArchive::find_hier(int32_t id) const noexcept {
    for (const auto& o : hiers_) if (o.id == id) return &o;
    return nullptr;
}
const IdeAnim* IdeArchive::find_anim(int32_t id) const noexcept {
    for (const auto& o : anims_) if (o.id == id) return &o;
    return nullptr;
}

static bool name_icase_eq(const char* stored, std::string_view query) noexcept {
    for (size_t i = 0; i < query.size(); ++i) {
        if (!stored[i]) return false;
        char a = stored[i], b = query[i];
        if (a >= 'A' && a <= 'Z') a += 32;
        if (b >= 'A' && b <= 'Z') b += 32;
        if (a != b) return false;
    }
    return stored[query.size()] == '\0';
}

const void* IdeArchive::find_by_name(std::string_view n, std::string_view& out_sec) const noexcept {
    for (const auto& o : objs_)  if (name_icase_eq(o.model_name, n)) { out_sec = "objs"; return &o; }
    for (const auto& o : tobjs_) if (name_icase_eq(o.model_name, n)) { out_sec = "tobj"; return &o; }
    for (const auto& o : weaps_) if (name_icase_eq(o.model_name, n)) { out_sec = "weap"; return &o; }
    for (const auto& o : hiers_) if (name_icase_eq(o.model_name, n)) { out_sec = "hier"; return &o; }
    for (const auto& o : anims_) if (name_icase_eq(o.model_name, n)) { out_sec = "anim"; return &o; }
    out_sec = "";
    return nullptr;
}

} // namespace sasdk
