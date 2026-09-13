// SPDX-License-Identifier: MIT
//
// src/img.cpp
// ===========
// Implementation of sasdk::ImgArchive (see include/sasdk/img.hpp).
#include <sasdk/img.hpp>
#include <cstdio>
#include <cstring>

namespace sasdk {

namespace {

struct RawHeader {
    char     magic[4];
    uint32_t entry_count;
};
static_assert(sizeof(RawHeader) == 8);

// RAII wrapper around a C FILE* for this translation unit only.
struct File {
    FILE* fp = nullptr;
    explicit File(const char* path, const char* mode) {
#if defined(_MSC_VER)
        fopen_s(&fp, path, mode);
#else
        fp = std::fopen(path, mode);
#endif
    }
    ~File() { if (fp) std::fclose(fp); }
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    explicit operator bool() const { return fp != nullptr; }

    bool read(void* buf, size_t bytes) const {
        return fp && std::fread(buf, 1, bytes, fp) == bytes;
    }
    bool seek(long long offset, int origin) const {
#if defined(_WIN32) || defined(_MSC_VER)
        return fp && _fseeki64(fp, offset, origin) == 0;
#else
        return fp && fseeko(fp, static_cast<off_t>(offset), origin) == 0;
#endif
    }
};

} // namespace

Result<ImgArchive> ImgArchive::open(std::string_view sv_path) {
    std::string path(sv_path);
    File f(path.c_str(), "rb");
    if (!f) return err(ErrorKind::IoError, "IMG: cannot open '" + path + "'");

    RawHeader hdr{};
    if (!f.read(&hdr, sizeof(hdr)))
        return err(ErrorKind::BadFormat, "IMG: cannot read header of '" + path + "'");

    if (std::memcmp(hdr.magic, "VER2", 4) != 0)
        return err(ErrorKind::BadFormat,
                   "IMG: '" + path + "' does not have a VER2 magic (got " +
                   std::string(hdr.magic, 4) + ")");

    std::vector<ImgEntry> entries(hdr.entry_count);
    if (hdr.entry_count > 0) {
        if (!f.read(entries.data(), sizeof(ImgEntry) * hdr.entry_count))
            return err(ErrorKind::BadFormat,
                       "IMG: truncated TOC in '" + path + "' (" +
                       std::to_string(hdr.entry_count) + " entries declared)");
    }

    ImgArchive arc;
    arc.path_    = std::move(path);
    arc.entries_ = std::move(entries);
    return arc;
}

const ImgEntry* ImgArchive::find(std::string_view name) const {
    for (const auto& e : entries_)
        if (e.name_eq(name)) return &e;
    return nullptr;
}

Result<std::vector<uint8_t>> ImgArchive::read(const ImgEntry& entry) const {
    File f(path_.c_str(), "rb");
    if (!f) return err(ErrorKind::IoError, "IMG: cannot reopen '" + path_ + "'");

    if (!f.seek(static_cast<long long>(entry.byte_offset()), SEEK_SET))
        return err(ErrorKind::IoError,
                   "IMG: seek failed for entry '" + std::string(entry.name) + "'");

    std::vector<uint8_t> buf(entry.byte_size());
    if (!f.read(buf.data(), buf.size()))
        return err(ErrorKind::IoError,
                   "IMG: read failed for entry '" + std::string(entry.name) + "'");

    return buf;
}

Result<std::vector<uint8_t>> ImgArchive::read(std::string_view name) const {
    const ImgEntry* e = find(name);
    if (!e) return err(ErrorKind::NotFound,
                       "IMG: '" + std::string(name) + "' not found in '" + path_ + "'");
    return read(*e);
}

} // namespace sasdk
