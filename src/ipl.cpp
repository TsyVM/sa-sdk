// SPDX-License-Identifier: MIT
// src/ipl.cpp — GTA:SA IPL parser implementation
#include <sasdk/ipl.hpp>
#include <algorithm>
#include <charconv>
#include <cstring>
#include <string>
#include <vector>

namespace sasdk {
namespace {

static std::vector<std::string_view> split_lines(std::string_view text) {
    std::vector<std::string_view> out;
    while (!text.empty()) {
        auto nl   = text.find('\n');
        auto line = text.substr(0, nl == std::string_view::npos ? text.size() : nl);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.remove_prefix(1);
        while (!line.empty() && (line.back()  == ' ' || line.back()  == '\t')) line.remove_suffix(1);
        auto hash = line.find('#');
        if (hash != std::string_view::npos) line = line.substr(0, hash);
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) line.remove_suffix(1);
        if (!line.empty()) out.push_back(line);
        if (nl == std::string_view::npos) break;
        text.remove_prefix(nl + 1);
    }
    return out;
}

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

static float pf(std::string_view sv) noexcept {
    float v = 0.f;
    std::from_chars(sv.data(), sv.data() + sv.size(), v);
    return v;
}
static int32_t pi(std::string_view sv) noexcept {
    int32_t v = 0;
    std::from_chars(sv.data(), sv.data() + sv.size(), v);
    return v;
}
static uint32_t pu(std::string_view sv) noexcept {
    uint32_t v = 0;
    std::from_chars(sv.data(), sv.data() + sv.size(), v);
    return v;
}
static void cn(char* dst, size_t cap, std::string_view src) noexcept {
    size_t n = std::min(src.size(), cap - 1);
    std::memcpy(dst, src.data(), n);
    dst[n] = '\0';
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

enum class Section { None, Inst, Cull, Grge, Enex, Pick, Cars, Skip };

static Section section_from(std::string_view tag) noexcept {
    if (icase_eq(tag, "inst")) return Section::Inst;
    if (icase_eq(tag, "cull")) return Section::Cull;
    if (icase_eq(tag, "grge")) return Section::Grge;
    if (icase_eq(tag, "enex")) return Section::Enex;
    if (icase_eq(tag, "pick")) return Section::Pick;
    if (icase_eq(tag, "cars")) return Section::Cars;
    // path, zone, mult, tcyc, auzo, occl are present in some IPLs; skip
    if (icase_eq(tag, "path") || icase_eq(tag, "zone") || icase_eq(tag, "mult") ||
        icase_eq(tag, "tcyc") || icase_eq(tag, "auzo") || icase_eq(tag, "occl"))
        return Section::Skip;
    return Section::None;
}

} // namespace

Result<IplArchive> IplArchive::parse(std::string_view text) noexcept {
    IplArchive arch;
    auto lines = split_lines(text);
    Section cur = Section::None;

    for (auto& line : lines) {
        if (icase_eq(line, "end")) { cur = Section::None; continue; }

        Section s = section_from(line);
        if (s != Section::None) { cur = s; continue; }

        if (cur == Section::None || cur == Section::Skip) continue;

        auto f = split_csv(line);

        if (cur == Section::Inst) {
            // id, modelname, interior_id, x, y, z, qx, qy, qz, qw
            if (f.size() < 10) continue;
            IplInst inst{};
            inst.id          = pi(f[0]);
            cn(inst.model_name, sizeof(inst.model_name), f[1]);
            inst.interior_id = pi(f[2]);
            inst.x  = pf(f[3]); inst.y  = pf(f[4]); inst.z  = pf(f[5]);
            inst.qx = pf(f[6]); inst.qy = pf(f[7]); inst.qz = pf(f[8]); inst.qw = pf(f[9]);
            arch.insts_.push_back(inst);

        } else if (cur == Section::Cull) {
            // cx, cy, cz, u0..u6
            if (f.size() < 3) continue;
            IplCull c{};
            c.cx = pf(f[0]); c.cy = pf(f[1]); c.cz = pf(f[2]);
            for (size_t i = 0; i < 7 && i + 3 < f.size(); ++i) c.unk[i] = pf(f[3 + i]);
            arch.culls_.push_back(c);

        } else if (cur == Section::Grge) {
            // id, x, y, z, x_size, y_size, z_angle, door_type, flags, name
            if (f.size() < 10) continue;
            IplGrge g{};
            g.id      = pi(f[0]);
            g.x       = pf(f[1]); g.y = pf(f[2]); g.z = pf(f[3]);
            g.x_size  = pf(f[4]); g.y_size = pf(f[5]);
            g.z_angle = pf(f[6]);
            g.door_type = static_cast<uint8_t>(pi(f[7]));
            g.flags     = static_cast<uint8_t>(pu(f[8]));
            cn(g.name, sizeof(g.name), f[9]);
            arch.grges_.push_back(g);

        } else if (cur == Section::Enex) {
            // ex, ey, ez, x_ang, x_sz, y_sz, ix, iy, iz, i_ang, int_id, sky_col, flags, name
            if (f.size() < 14) continue;
            IplEnex e{};
            e.ex = pf(f[0]); e.ey = pf(f[1]); e.ez = pf(f[2]);
            e.x_angle = pf(f[3]); e.x_size = pf(f[4]); e.y_size = pf(f[5]);
            e.ix = pf(f[6]); e.iy = pf(f[7]); e.iz = pf(f[8]);
            e.i_angle    = pf(f[9]);
            e.interior_id = pi(f[10]);
            e.sky_color   = pu(f[11]);
            e.flags       = pu(f[12]);
            cn(e.name, sizeof(e.name), f[13]);
            arch.enexs_.push_back(e);

        } else if (cur == Section::Pick) {
            // model_id, x, y, z
            if (f.size() < 4) continue;
            IplPick p{};
            p.model_id = pi(f[0]);
            p.x = pf(f[1]); p.y = pf(f[2]); p.z = pf(f[3]);
            arch.picks_.push_back(p);

        } else if (cur == Section::Cars) {
            // x, y, z, angle, model_id, col1, col2, force_spawn, alarm, locks, unk1, unk2
            if (f.size() < 12) continue;
            IplCar c{};
            c.x = pf(f[0]); c.y = pf(f[1]); c.z = pf(f[2]);
            c.angle    = pf(f[3]);
            c.model_id = pi(f[4]);
            c.primary_color   = static_cast<uint8_t>(pi(f[5]));
            c.secondary_color = static_cast<uint8_t>(pi(f[6]));
            c.force_spawn     = static_cast<uint8_t>(pi(f[7]));
            c.alarm           = static_cast<uint8_t>(pi(f[8]));
            c.locks           = static_cast<uint8_t>(pi(f[9]));
            c.unk1            = static_cast<uint8_t>(pi(f[10]));
            c.unk2            = static_cast<uint8_t>(pi(f[11]));
            arch.cars_.push_back(c);
        }
    }
    return arch;
}

const IplInst* IplArchive::find_instance(int32_t model_id) const noexcept {
    for (const auto& i : insts_) if (i.id == model_id) return &i;
    return nullptr;
}

const IplInst* IplArchive::find_instance(std::string_view model_name) const noexcept {
    for (const auto& i : insts_) {
        size_t len = model_name.size();
        bool match = true;
        for (size_t j = 0; j < len && match; ++j) {
            char a = i.model_name[j], b = model_name[j];
            if (!a) { match = false; break; }
            if (a >= 'A' && a <= 'Z') a += 32;
            if (b >= 'A' && b <= 'Z') b += 32;
            if (a != b) match = false;
        }
        if (match && i.model_name[len] == '\0') return &i;
    }
    return nullptr;
}

} // namespace sasdk
