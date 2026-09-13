<div align="center">

<img src="sasdk-logo.png" width="500" alt="SASDK"/>

<p><em>Complete API reference for every public function, class, and type in SASDK</em></p>

<a href="#">
<img src="https://readme-typing-svg.demolab.com/?lines=GxtArchive+%C2%B7+ImgArchive+%C2%B7+ColArchive.;RwStream+%C2%B7+FxpArchive+%C2%B7+SaveArchive.;InlineHook+%C2%B7+VmtHook+%C2%B7+Global%3CT%3E.;Result%3CT%3E+%E2%80%94+no+exceptions%2C+no+surprises.;fn%3A%3A+typed+handles+for+game+functions.;302+structs.+6%2C545+generated+lines.&font=Fira%20Code&center=true&width=700&height=45&color=E07B00&vCenter=true&size=20&pause=1800"/>
</a>

<br/>

[![License: MIT](https://img.shields.io/badge/License-MIT-E07B00?style=for-the-badge&labelColor=000000)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-E07B00?style=for-the-badge&labelColor=000000&logo=cplusplus&logoColor=E07B00)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x86-E07B00?style=for-the-badge&labelColor=000000&logo=windows&logoColor=E07B00)](#)
[![TeamVanilla](https://img.shields.io/badge/Team-TeamVanilla-E07B00?style=for-the-badge&labelColor=000000)](https://www.teamvanilla.org/)

<br/>

[![GXT](https://img.shields.io/badge/GXT-text%20archive-E07B00?style=flat-square&labelColor=000000)](#gxtarchive--gxthpp)
[![IMG](https://img.shields.io/badge/IMG-VER2%20archive-E07B00?style=flat-square&labelColor=000000)](#imgarchive--imghpp)
[![COL](https://img.shields.io/badge/COL-collision-E07B00?style=flat-square&labelColor=000000)](#colarchive--colhpp)
[![RW](https://img.shields.io/badge/RW-RenderWare%20chunks-E07B00?style=flat-square&labelColor=000000)](#rwstream--rwhpp)
[![FXP](https://img.shields.io/badge/FXP-particle%20project-E07B00?style=flat-square&labelColor=000000)](#fxparchive--fxphpp)
[![Save](https://img.shields.io/badge/Save-block%20reader-E07B00?style=flat-square&labelColor=000000)](#savearchive--savehpp)
[![Hooks](https://img.shields.io/badge/Hooks-inline%20%C2%B7%20vmt-E07B00?style=flat-square&labelColor=000000)](#-runtime-layer)
[![sa10us](https://img.shields.io/badge/sa10us-struct%20DB-E07B00?style=flat-square&labelColor=000000)](#-struct-database)

</div>

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

<div align="center">

### 📑 Contents

[Error Handling](#-error-handling--resulthpp) · [GxtArchive](#gxtarchive--gxthpp) · [ImgArchive](#imgarchive--imghpp) · [ColArchive](#colarchive--colhpp)

[RwStream / RwChunk](#rwstream--rwhpp) · [FxpArchive](#fxparchive--fxphpp) · [SaveArchive](#savearchive--savehpp)

[Address · Module](#address--module--addresshpp) · [Hooks](#hooks--hookhpp) · [Invoke](#invoke--invokehpp) · [Global / ArrayView](#global--arrayview--globalhpp)

[Struct Database](#-struct-database) · [fn:: Handles](#fn-function-handles--functionsinl) · [addr:: Globals](#addr-global-addresses--addresseshpp)

[sa10us_easy](#sa10us_easy--sa10us_easyhpp) · [Locators](#locators--sa10us_locatorshpp) · [mod.hpp umbrella](#modhpp-umbrella)

</div>

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## ❌ Error Handling — `result.hpp`

Every fallible function in SASDK returns `Result<T>`. There are no exceptions.

```cpp
namespace sasdk {

enum class ErrorKind {
    NotFound,      // schema key, field, entry, or file path does not exist
    OutOfRange,    // read outside a known valid extent
    Unverified,    // data exists but tier is open; caller required verified
    IoError,       // file or process read failure
    BadFormat,     // file did not parse as the expected format
};

template<typename T>
class Result {
public:
    // Construct a success result
    static Result<T> ok(T value);
    // Construct a failure result
    static Result<T> err(ErrorKind kind, std::string_view message);

    // Test and access
    explicit operator bool() const noexcept;    // true on success
    bool has_value()  const noexcept;
    const T& value()  const;                    // throws if error (internal only)
    T&       value();
    const T* operator->() const noexcept;
    T*       operator->() noexcept;
    const T& operator*()  const noexcept;
    T&       operator*()  noexcept;

    // Error access
    ErrorKind        error_kind() const noexcept;
    std::string_view error_message() const noexcept;
};

// Status = Result<void>; use for operations that succeed or fail with no return value
using Status = Result<void>;

std::string_view error_kind_name(ErrorKind kind) noexcept;  // "NotFound", "BadFormat", ...

} // namespace sasdk
```

### Pattern

```cpp
auto archive = sasdk::GxtArchive::parse(data, size);
if (!archive) {
    // archive.error_kind()    — sasdk::ErrorKind
    // archive.error_message() — human-readable string_view
    return;
}
// use *archive or archive->lookup(...)
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## GxtArchive — `gxt.hpp`

GTA:SA text archive (`american.gxt`). TABL → TKEY → TDAT block format. Keys are looked up by CRC-32 hash (reflected, init=0xFFFFFFFF, **no final XOR complement**).

### Free function

```cpp
// Hash a GXT key name exactly as the game does.
// Upper-cases ASCII a–z before hashing; does NOT apply the final ^ 0xFFFFFFFF.
uint32_t gxt_key_hash(std::string_view key) noexcept;
```

### Types

```cpp
struct GxtTableEntry {
    char     name[8];   // NUL-padded table name (e.g. "MAIN")
    uint32_t tkey_off;  // byte offset of this table's TKEY block within the file
};

struct GxtKeyEntry {
    uint32_t value_off; // byte offset into TDAT of the NUL-terminated UTF-8 string
    uint32_t hash;      // CRC-32 hash of the upper-cased key name
};
```

### `GxtArchive`

```cpp
class GxtArchive {
public:
    // Parse in-memory GXT data. O(n tables) setup; O(log n keys) per lookup.
    static Result<GxtArchive> parse(const uint8_t* data, size_t size) noexcept;

    // Look up a text entry by table and key name.
    // Returns a string_view into the original buffer — valid as long as the
    // buffer outlives the archive. Returns nullopt when the table or key is absent.
    std::optional<std::string_view>
        lookup(std::string_view table_name, std::string_view key_name) const noexcept;

    // Look up by pre-computed CRC-32 hash. Avoids re-hashing in hot paths.
    std::optional<std::string_view>
        lookup_hash(std::string_view table_name, uint32_t hash) const noexcept;

    // Iterate every (hash, text) pair in a table.
    // fn(hash, text) — return false to stop early.
    template<typename Fn>
    void for_each(std::string_view table_name, Fn&& fn) const noexcept;

    size_t table_count() const noexcept;
};
```

### Example

```cpp
std::vector<uint8_t> bytes = load_file("text/american.gxt");
auto gxt = sasdk::GxtArchive::parse(bytes.data(), bytes.size());
if (!gxt) return;

// Name lookup (hashes internally)
if (auto s = gxt->lookup("MAIN", "WELCOME"))
    puts(std::string(*s).c_str());

// Pre-hashed lookup in a hot loop
uint32_t h = sasdk::gxt_key_hash("SCORE");
if (auto s = gxt->lookup_hash("STAT", h))
    printf("Score: %.*s\n", (int)s->size(), s->data());

// Dump all keys in a table
gxt->for_each("MAIN", [](uint32_t hash, std::string_view text) {
    printf("0x%08X = %.*s\n", hash, (int)text.size(), text.data());
});
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## ImgArchive — `img.hpp`

GTA:SA IMG VER2 archive (`gta3.img`, `player.img`). 32-byte TOC entries. 2,048-byte sector addressing.

### Types

```cpp
struct ImgEntry {
    uint32_t sector_offset;    // start sector (multiply by 2048 for byte offset)
    uint32_t size_sectors;     // size in sectors
    char     name[24];         // NUL-padded asset name, e.g. "player.dff"
};
```

### `ImgArchive`

```cpp
class ImgArchive {
public:
    // Open an IMG file from disk. Reads the TOC into memory; asset data is
    // read on demand via read().
    static Result<ImgArchive> open(std::string_view path) noexcept;

    // Parse an already-loaded IMG blob (TOC must be the first 8+32n bytes).
    static Result<ImgArchive> parse(const uint8_t* data, size_t size) noexcept;

    // Find a TOC entry by name (case-insensitive).
    const ImgEntry* find(std::string_view name) const noexcept;

    // Read an asset into a vector, given a TOC entry.
    Result<std::vector<uint8_t>> read(const ImgEntry& entry) const noexcept;

    // Convenience: find + read in one call.
    Result<std::vector<uint8_t>> read(std::string_view name) const noexcept;

    size_t size() const noexcept;   // total entry count
};
```

### Example

```cpp
auto img = sasdk::ImgArchive::open("models/gta3.img");
if (!img) return;

// Find and read a DFF
auto dff = img->read("player.dff");
if (!dff) return;

// Iterate all entries (no public iterator — use size() + an index array if needed)
// For asset enumeration, open() + a loop over find() results is the pattern.
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## ColArchive — `col.hpp`

GTA:SA COL2 collision archive. Multi-model per file. Element sizes verified by Capstone disassembly of sa10us.

### Constants

```cpp
static constexpr size_t kColSphereSize   = 18;   // per collision sphere entry
static constexpr size_t kColBoxSize      = 26;   // per collision box entry
static constexpr size_t kColTriangleSize =  8;   // per collision triangle entry
static constexpr size_t kColLineSize     = 24;   // per collision line entry
```

### Types

```cpp
struct ColModel {
    char     name[22];          // NUL-padded model name matching IDE entry
    uint16_t model_id;          // numeric model ID
    bool     is_col2;           // true = COL2 magic; false = COL1 (older format)

    // Counts
    uint16_t num_spheres;
    uint16_t num_boxes;
    uint16_t num_triangles;
    uint8_t  num_lines;
    uint8_t  flags;

    // Raw byte spans into the original buffer — valid as long as the buffer lives.
    // Each span is num_* × kColXxxSize bytes; cast to your own structs if needed.
    std::span<const uint8_t> sphere_data;
    std::span<const uint8_t> box_data;
    std::span<const uint8_t> line_data;
    std::span<const uint8_t> triangle_data;
};
```

### `ColArchive`

```cpp
class ColArchive {
public:
    // Parse in-memory COL data. Accepts a file containing one or more COL2 models.
    static Result<ColArchive> parse(const uint8_t* data, size_t size) noexcept;

    // Find a model by NUL-padded name (case-sensitive, 22 chars max).
    const ColModel* find(std::string_view name)  const noexcept;

    // Find a model by numeric model ID.
    const ColModel* find(uint16_t model_id)       const noexcept;

    size_t model_count() const noexcept;
};
```

### Example

```cpp
auto col = sasdk::ColArchive::parse(bytes.data(), bytes.size());
if (!col) return;

const sasdk::ColModel* m = col->find("PLAYER");
if (!m) return;

printf("Spheres: %u, Boxes: %u, Tris: %u\n",
    m->num_spheres, m->num_boxes, m->num_triangles);

// Raw sphere data — 18 bytes each
const uint8_t* sp = m->sphere_data.data();
for (uint16_t i = 0; i < m->num_spheres; ++i, sp += sasdk::kColSphereSize) {
    // bytes [0..11]  = center XYZ (3 floats)
    // byte  [12..15] = radius (float)
    // bytes [16..17] = surface type bytes
}
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## RwStream — `rw.hpp`

RenderWare binary chunk stream reader for DFF (`RpClump`), TXD (`RwTexDictionary`), and IFP (`RwHAnimAnimation`) files.

### `RwChunkId` — full type enum

```cpp
enum class RwChunkId : uint32_t {
    Struct          = 0x0001,
    String          = 0x0002,
    Extension       = 0x0003,
    Texture         = 0x0006,
    Material        = 0x0007,
    MaterialList    = 0x0008,
    FrameList       = 0x000E,
    Geometry        = 0x0010,
    Atomic          = 0x0014,
    GeometryList    = 0x001A,
    HAnimAnimation  = 0x001B,
    Clump           = 0x001C,   // DFF root
    TexDictionary   = 0x0021,   // TXD root
    HAnim           = 0x011E,
    SkinPLG         = 0x0116,
    MatFXPLG        = 0x0120,
    BinMeshPLG      = 0x050E,
    NativeDataPLG   = 0x0510,
    Unknown         = 0xFFFFFFFF,
    // ... additional plugin chunk IDs
};

std::string_view rw_chunk_name(RwChunkId id) noexcept;  // e.g. "Clump", "Geometry"
```

### `RwChunk`

```cpp
struct RwChunk {
    RwChunkId              type;        // chunk type ID
    uint32_t               rw_version;  // RenderWare version word from header
    std::span<const uint8_t> payload;   // raw bytes after the 12-byte header

    // Identity
    bool is(RwChunkId id) const noexcept { return type == id; }

    // Child traversal — reads the payload as a stream of child chunks
    Result<std::vector<RwChunk>> children() const noexcept;

    // Find the first direct child with a given type
    std::optional<RwChunk> first_child(RwChunkId id) const noexcept;

    // Shortcut: first_child(RwChunkId::Struct)
    std::optional<RwChunk> struct_data() const noexcept;
};
```

### `RwStream`

```cpp
class RwStream {
public:
    // Read exactly one top-level chunk from a buffer (DFF / TXD / IFP files
    // each start with one root chunk).
    static Result<RwChunk> read_root(const uint8_t* data, size_t size) noexcept;

    // Read all top-level chunks from a buffer.
    static Result<std::vector<RwChunk>> read_all(const uint8_t* data, size_t size) noexcept;

    // Iterate top-level chunks. fn(chunk) — return false to stop early.
    static Status for_each(
        const uint8_t* data, size_t size,
        std::function<bool(const RwChunk&)> fn) noexcept;
};
```

### Example

```cpp
// DFF clump traversal
auto root = sasdk::RwStream::read_root(dff.data(), dff.size());
if (!root || !root->is(sasdk::RwChunkId::Clump)) return;

// Geometry list
auto geom_list = root->first_child(sasdk::RwChunkId::GeometryList);
if (geom_list) {
    auto children = geom_list->children();
    if (children) {
        for (const auto& geom : *children) {
            if (geom.is(sasdk::RwChunkId::Geometry)) {
                auto raw = geom.struct_data();  // first Struct child = raw geometry data
            }
        }
    }
}

// TXD — iterate all textures
sasdk::RwStream::for_each(txd.data(), txd.size(), [](const sasdk::RwChunk& c) {
    if (c.is(sasdk::RwChunkId::TexDictionary)) {
        auto textures = c.first_child(sasdk::RwChunkId::Texture);
    }
    return true;
});
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## FxpArchive — `fxp.hpp`

GTA:SA particle project reader (`models/effects.fxp`). Count-driven plain-text grammar. Full hierarchy: project → system → primitive → info → curve → keyframe.

### Types

```cpp
struct FxpKeyframe {
    float time;
    float val;
};

struct FxpCurve {
    std::string             name;
    bool                    looped;
    std::vector<FxpKeyframe> keys;
};

struct FxpInfo {
    std::string              type_name; // e.g. "FX_INFO_EMRATE_DATA"
    std::vector<std::string> scalars;   // scalar fields in declaration order
    std::vector<FxpCurve>    curves;    // animation curves
};

struct FxpPrimitive {
    std::string name;
    std::string texture;    // primary texture
    std::string texture2;   // secondary texture (may be empty)
    std::string texture3;
    std::string texture4;
    int   alpha_on;
    int   src_blend_id;
    int   dst_blend_id;
    float lod_start;
    float lod_end;
    std::vector<FxpInfo> infos;
};

struct FxpSystem {
    int         version;              // constant = 109 in sa10us
    std::string filename;
    std::string name;
    std::string bounding_sphere;
    std::string txd_name;
    float       loop_interval_min;
    float       loop_interval_max;
    float       cull_dist;
    int         play_mode;
    int         omit_textures;
    std::vector<FxpPrimitive> prims;
};

struct FxpProject {
    std::vector<FxpSystem> systems;
};
```

### `FxpArchive`

```cpp
class FxpArchive {
public:
    // Parse a complete FXP file given as a string_view over its text content.
    // Zero-copy — the parser uses from_chars throughout; all strings are copied
    // into the archive on parse.
    static Result<FxpArchive> parse(std::string_view text) noexcept;

    // Find a system by name. Returns nullptr if not found.
    const FxpSystem* find_system(std::string_view name) const noexcept;

    const FxpProject& project() const noexcept;

    size_t system_count() const noexcept;
};
```

### Example

```cpp
std::string text = load_text("models/effects.fxp");
auto fxp = sasdk::FxpArchive::parse(text);
if (!fxp) return;

printf("Systems: %zu\n", fxp->system_count());

const sasdk::FxpSystem* smoke = fxp->find_system("SMOKE_EXHAUST");
if (!smoke) return;

printf("System version: %d\n", smoke->version);   // always 109
printf("Primitives: %zu\n", smoke->prims.size());

// Walk curves in the first primitive's first info
if (!smoke->prims.empty() && !smoke->prims[0].infos.empty()) {
    for (const auto& curve : smoke->prims[0].infos[0].curves) {
        printf("Curve: %s (%zu keys)\n", curve.name.c_str(), curve.keys.size());
        for (const auto& kf : curve.keys)
            printf("  t=%.3f val=%.3f\n", kf.time, kf.val);
    }
}
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## SaveArchive — `save.hpp`

GTA:SA save file block reader. `BLOCK ` (6-byte tag) framing; each block has a uint32 size followed by its body. Additive checksum covers all block bodies.

### Types

```cpp
struct SaveBlock {
    uint32_t                 index;  // sequential block number (0-based)
    std::span<const uint8_t> data;   // raw block body (zero-copy view)
};
```

### `SaveArchive`

```cpp
class SaveArchive {
public:
    // Parse in-memory save file data.
    static Result<SaveArchive> parse(const uint8_t* data, size_t size) noexcept;

    // Access a block by index. Returns nullopt for out-of-range index.
    std::optional<SaveBlock> block(uint32_t index) const noexcept;

    // Additive sum of all bytes across all block bodies.
    uint32_t checksum() const noexcept;

    size_t block_count() const noexcept;
};
```

### Known block indices

| Index | Content |
|---|---|
| 0 | `SimpleVars` — player name, stats, timestamps |
| 1 | `Scripts` — running script state |
| 4 | `CStats` — 343 × int32 stat counters |
| 5 | `Streaming` — loaded model list |
| 17 | `GangData` — gang territories and membership |
| 23 | `AudioScript` — audio script state |

### Example

```cpp
auto save = sasdk::SaveArchive::parse(bytes.data(), bytes.size());
if (!save) return;

printf("Blocks: %zu, Checksum: 0x%08X\n",
    save->block_count(), save->checksum());

// Read CStats block
auto stats = save->block(4);
if (stats) {
    // 343 int32 entries, little-endian
    const int32_t* counters = reinterpret_cast<const int32_t*>(stats->data.data());
    printf("Missions passed: %d\n", counters[0]);
}
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## 🔧 Runtime Layer

The runtime layer (`SASDK::game`, header-only) is designed for 32-bit `.asi` plugins targeting the sa10us process. All virtual addresses are rebased against the actual load offset via `Module::bind`.

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## Address · Module — `address.hpp`

```cpp
namespace sasdk {

// A virtual address in the sa10us binary (as-linked at 0x00400000).
// All addr:: entries are VirtualAddress values; call .va to get the
// rebased runtime address after Module::bind().
struct VirtualAddress {
    uint32_t rva;        // relative virtual address (from ImageBase 0x00400000)
    uintptr_t va() const noexcept;  // rva + Module::base_address()
};

class Module {
public:
    // Set the module base once at startup. All subsequent .va calls are correct.
    static void   bind(HMODULE hmod) noexcept;
    static void   bind(uintptr_t base) noexcept;

    // Current module base (0 if not yet bound).
    static uintptr_t base_address() noexcept;

    // True if va is within [base, base+image_size).
    static bool in_image(uintptr_t va) noexcept;
};

} // namespace sasdk
```

### Startup

```cpp
// In DllMain or ASI entry — call exactly once
sasdk::Module::bind(GetModuleHandleA(nullptr));
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## Hooks — `hook.hpp`

```cpp
namespace sasdk::rt {

// 5-byte JMP inline hook. Writes a relative JMP at `target_va`, saves the
// original 5 bytes. Restores on destruction or explicit remove().
class InlineHook {
public:
    // Install a hook at a runtime virtual address.
    // Returns err if the address is not in the module image.
    static Result<InlineHook> install(uintptr_t target_va, void* detour) noexcept;

    // Remove the hook and restore original bytes.
    Status remove() noexcept;

    // Auto-remove on destruction.
    ~InlineHook();

    // Move-only
    InlineHook(InlineHook&&) noexcept;
    InlineHook& operator=(InlineHook&&) noexcept;
};

// vtable slot swap hook. Stores the original function pointer for chaining.
class VmtHook {
public:
    // Install a hook on slot `slot_index` in the vtable of `object`.
    // `object` must be a live C++ object with a vtable pointer at offset 0.
    static Result<VmtHook> install(void* object, uint32_t slot_index, void* detour) noexcept;

    // Original function pointer — call it to chain to the original behaviour.
    void* original() const noexcept;

    // Restore the original slot and release.
    Status remove() noexcept;

    ~VmtHook();
    VmtHook(VmtHook&&) noexcept;
    VmtHook& operator=(VmtHook&&) noexcept;
};

} // namespace sasdk::rt
```

### Example

```cpp
// Inline hook on a game function
static void __cdecl my_vehicle_fix(void* vehicle) {
    // custom logic here
}

auto hook = sasdk::rt::InlineHook::install(
    sasdk::addr::CVehicle_Fix.va(), &my_vehicle_fix);

// VMT hook — intercept a virtual call
static int __thiscall my_get_type(void* self) { return 2; }

auto vmt = sasdk::rt::VmtHook::install(entity_ptr, /*slot=*/4, &my_get_type);
// chain to original:
// using Fn = int(__thiscall*)(void*);
// return reinterpret_cast<Fn>(vmt->original())(self);
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## Invoke — `invoke.hpp`

Type-safe wrappers for calling game functions by virtual address with the correct calling convention.

```cpp
namespace sasdk::rt {

// __cdecl call — most global game functions
template<typename Ret, typename... Args>
Ret call_cdecl(uintptr_t va, Args&&... args);

// __thiscall — member functions; pass the object pointer as first argument
template<typename Ret, typename... Args>
Ret call_thiscall(uintptr_t va, void* self, Args&&... args);

// __stdcall — Windows API wrappers in the game
template<typename Ret, typename... Args>
Ret call_stdcall(uintptr_t va, Args&&... args);

} // namespace sasdk::rt
```

### Example

```cpp
// Call a __cdecl game function at a known virtual address
sasdk::rt::call_cdecl<void>(
    sasdk::addr::CStreaming_RequestModel.va(),
    model_id, /*flags=*/2);

// Call a __thiscall member function
sasdk::rt::call_thiscall<void>(
    sasdk::addr::CVehicle_Fix.va(), vehicle_ptr);
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## Global / ArrayView — `global.hpp`

Typed access to game global variables and arrays.

```cpp
namespace sasdk {

// Typed pointer to a game global at a fixed virtual address.
// The address is rebased after Module::bind().
template<typename T>
class Global {
public:
    explicit Global(uint32_t rva) noexcept;
    explicit Global(VirtualAddress va) noexcept;

    T&       get()  noexcept;   // reference to the live game value
    const T& get()  const noexcept;
    T        read() const noexcept;
    void     write(const T& value) noexcept;

    T* ptr()  noexcept;
    const T* ptr() const noexcept;
};

// Typed view over a fixed-length game array.
template<typename T, size_t N = 0>
class ArrayView {
public:
    explicit ArrayView(uint32_t rva) noexcept;
    explicit ArrayView(uint32_t rva, size_t count) noexcept;  // runtime count

    T&       operator[](size_t i) noexcept;
    const T& operator[](size_t i) const noexcept;
    T*       data()  noexcept;
    size_t   size()  const noexcept;
};

} // namespace sasdk
```

### Example

```cpp
// CJ's health: CPed::m_fHealth is at offset 0x540 within the CPed struct.
// CWorld::Players[0] holds a CPlayerPed*.
auto* player_ped = sasdk::Global<uint8_t*>{ sasdk::addr::CWorld_Players }.read();
float health = *reinterpret_cast<float*>(player_ped + 0x540);

// Time scale global
float ts = sasdk::Global<float>{ 0x00B7CB84u }.read();

// Ped pool — access via addr::
auto& ped_pool = sasdk::Global<void*>{ sasdk::addr::PedPool }.get();
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## 🗄️ Struct Database

`include/sasdk/game/sa10us/sa10us_db.inl` — 302 structs, 6,545 lines, generated. Include via `<sasdk/game/sa10us/sa10us.hpp>` or the umbrella `<sasdk/sasdk.hpp>`.

Every struct has `static_assert(sizeof(Struct) == expected)`. Fields are annotated with one of four confidence tiers:

```cpp
// [SASDK VERIFIED]                — VALIDATE_OFFSET confirmed in plugin-sdk
// [SASDK VERIFIED_BY_DISASSEMBLY] — 2+ Capstone hits across sa10us functions
// [SASDK REASONED]                — derived from surrounding verified fields
// [SASDK OPEN]                    — not yet verified; use with caution
```

### Selected struct layouts

```cpp
// CPed — full coverage, 1948 bytes, 93 fields
struct CPed {
    // inherited from CPhysical (256B), CVehicle slot, ...
    /* 0x000 */ void*    m_vtable;
    /* 0x08C */ uint32_t m_nPedFlags;      // [VERIFIED] 31 named bitflags
    /* 0x540 */ float    m_fHealth;        // [VERIFIED] 0.0–200.0 default range
    /* 0x544 */ float    m_fArmour;        // [VERIFIED]
    /* 0x5F4 */ int32_t  m_nWeaponSlot;    // [VERIFIED] current weapon slot
    // ... 88 more fields
    static_assert(sizeof(CPed) == 0x79C);
};

// CVehicle — full coverage, 1128 bytes, 86 fields
struct CVehicle {
    /* 0x000 */ void*    m_vtable;
    /* 0x4EC */ uint32_t m_nVehicleFlags;  // [VERIFIED_BY_DISASSEMBLY] 52 named bitflags
    /* 0x4F0 */ uint32_t m_nVehicleFlags2; // [VERIFIED_BY_DISASSEMBLY]
    // ... 84 more fields
    static_assert(sizeof(CVehicle) == 0x468);
};

// CHandlingData — novel size 224B (community had 212B)
struct CHandlingData {
    // All fields [VERIFIED_BY_DISASSEMBLY] via IMUL stride probe at 0x6F0151
    /* 0x000 */ int32_t  m_nHandlingId;
    /* 0x004 */ float    m_fMass;
    // ...
    static_assert(sizeof(CHandlingData) == 0xE0);  // 224B
};
```

### Novel findings

| Struct | Field | Finding | Evidence |
|---|---|---|---|
| `CHandlingData` | `sizeof` | **0xE0 (224B)** — community had 0xD4 | `imul eax, eax, 0E0h` @`0x6F0151` |
| `CExplosion` | type count | **21 types** (0–20) | `cmp eax, 0x14` @`0x73702C` |
| `CVehicle` | `m_nVehicleFlags @0x4EC` | **52 named bitflags** | disassembly of `CAutomobile::ProcessControl` |
| Globals | `ms_vehicleHandling` | `0xC2B9DC` | stride probe across `0x6F0151` |
| Globals | `ms_vehicleHandling` sub | `0xC3BB00` (stride 0x94) | secondary handling table |

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## fn:: Function Handles — `functions.inl`

Generated typed callable handles for verified game functions. All addresses are sa10us VAs (rebased on first call after `Module::bind`).

```cpp
namespace sasdk::sa10us::fn {

// Each handle has:
//   .va()             — uintptr_t runtime virtual address
//   .call(args...)    — type-safe call with correct calling convention

// Ped
inline constexpr auto CPed_GiveWeapon = FnHandle<
    void(__thiscall*)(void* ped, int weapon_type, int ammo_count)
>{ 0x004C4790u };

inline constexpr auto CPed_RemoveWeaponModel = FnHandle<
    void(__thiscall*)(void* ped, int weapon_model)
>{ 0x004C4950u };

inline constexpr auto CPed_SetCurrentWeapon = FnHandle<
    void(__thiscall*)(void* ped, int weapon_type)
>{ 0x004C4B40u };

// Vehicle
inline constexpr auto CVehicle_Fix = FnHandle<
    void(__thiscall*)(void* vehicle)
>{ 0x006D0E90u };

inline constexpr auto CVehicle_SetEngineOn = FnHandle<
    void(__thiscall*)(void* vehicle, bool on, bool skip_sfx)
>{ 0x006D6400u };

// Streaming
inline constexpr auto CStreaming_RequestModel = FnHandle<
    void(__cdecl*)(int model_id, int flags)
>{ 0x004087E0u };

inline constexpr auto CStreaming_LoadAllRequestedModels = FnHandle<
    void(__cdecl*)(bool prioritise)
>{ 0x0040EA10u };

// World
inline constexpr auto CWorld_Add = FnHandle<
    void(__cdecl*)(void* entity)
>{ 0x00563B10u };

inline constexpr auto CWorld_Remove = FnHandle<
    void(__cdecl*)(void* entity)
>{ 0x00563C10u };

} // namespace sasdk::sa10us::fn
```

### Usage

```cpp
sasdk::sa10us::fn::CPed_GiveWeapon.call(cj_ped, /*weapon_type=*/31, /*ammo=*/500);
sasdk::sa10us::fn::CVehicle_Fix.call(my_car_ptr);
sasdk::sa10us::fn::CStreaming_RequestModel.call(model_id, 2);
sasdk::sa10us::fn::CStreaming_LoadAllRequestedModels.call(true);
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## addr:: Global Addresses — `addresses.hpp`

Verified global variable addresses in sa10us. All are `VirtualAddress` — call `.va()` for the rebased runtime address.

```cpp
namespace sasdk::sa10us::addr {

// World / Ped
inline constexpr VirtualAddress CWorld_Players       = { 0x00B7CD98u };  // CPlayerPed*[4]
inline constexpr VirtualAddress PedPool              = { 0x00B74490u };  // CPool<CPed>*

// Vehicle
inline constexpr VirtualAddress VehiclePool          = { 0x00B74494u };  // CPool<CVehicle>*
inline constexpr VirtualAddress ms_vehicleHandling   = { 0x00C2B9DCu };  // CHandlingData[htype]
inline constexpr VirtualAddress ms_vehicleSubHandling= { 0x00C3BB00u };  // sub-handling array

// Globals
inline constexpr VirtualAddress TimeScale            = { 0x00B7CB84u };  // float
inline constexpr VirtualAddress PedDensity           = { 0x00C1E9D0u };  // float
inline constexpr VirtualAddress CarDensity           = { 0x00C1E9D4u };  // float
inline constexpr VirtualAddress WeatherIndex         = { 0x00C81320u };  // int32_t
inline constexpr VirtualAddress GameState            = { 0x00C8D4C0u };  // int32_t

// Streaming
inline constexpr VirtualAddress CStreaming_RequestModel     = { 0x004087E0u };
inline constexpr VirtualAddress CStreaming_RenderEntity     = { 0x00407700u };

} // namespace sasdk::sa10us::addr
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## sa10us_easy — `sa10us_easy.hpp`

High-level helper functions for common mod tasks. All are inline wrappers over the lower-level API.

```cpp
namespace sasdk::sa10us::easy {

// Get a pointer to CJ (player 0 ped). Returns nullptr if not yet spawned.
void* get_player_ped() noexcept;

// Read CJ's health (0.0–200.0 default range).
float get_player_health() noexcept;

// Set CJ's health. Clamped to [0, 200].
void set_player_health(float health) noexcept;

// Give CJ a weapon with ammo.
void give_player_weapon(int weapon_type, int ammo) noexcept;

// Fix CJ's current vehicle (if in one).
void fix_player_vehicle() noexcept;

// Request and immediately stream a model. Blocks until loaded.
bool load_model(int model_id) noexcept;

// Get the current weather index.
int get_weather() noexcept;

// Set the weather index.
void set_weather(int weather_index) noexcept;

} // namespace sasdk::sa10us::easy
```

### Example

```cpp
#include <sasdk/game/sa10us/sa10us_easy.hpp>
using namespace sasdk::sa10us::easy;

// One-liner mod: god mode + minigun
set_player_health(200.0f);
give_player_weapon(38 /* minigun */, 9999);
fix_player_vehicle();
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## Locators — `sa10us_locators.hpp`

Typed memory locators derived from the struct database. Find game objects by position or ID without iterating pools manually.

```cpp
namespace sasdk::sa10us {

// Find the nearest ped to a world position within radius.
// Returns nullptr if none found within radius.
void* find_nearest_ped(float x, float y, float z, float radius) noexcept;

// Find the nearest vehicle to a world position within radius.
void* find_nearest_vehicle(float x, float y, float z, float radius) noexcept;

// Get CJ's current vehicle (nullptr if on foot).
void* get_player_vehicle() noexcept;

// Get CJ's current world position into out_x, out_y, out_z.
void get_player_pos(float& out_x, float& out_y, float& out_z) noexcept;

} // namespace sasdk::sa10us
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## mod.hpp umbrella

`<sasdk/mod.hpp>` pulls in the complete runtime layer in one include — use this for `.asi` plugins.

```cpp
// mod.hpp includes:
#include <sasdk/core/address.hpp>     // VirtualAddress, Module
#include <sasdk/core/hook.hpp>        // InlineHook, VmtHook
#include <sasdk/core/invoke.hpp>      // call_cdecl, call_thiscall
#include <sasdk/core/global.hpp>      // Global<T>, ArrayView<T>
#include <sasdk/core/result.hpp>      // Result<T>, Status, ErrorKind
#include <sasdk/core/memory.hpp>      // patch_bytes, read_memory
#include <sasdk/core/callconv.hpp>    // __cdecl/__thiscall helpers
#include <sasdk/game/sa10us/sa10us.hpp>   // struct DB + fn:: + addr::
#include <sasdk/game/sa10us/sa10us_easy.hpp>
#include <sasdk/game/sa10us/sa10us_locators.hpp>
```

### sasdk.hpp — full umbrella

`<sasdk/sasdk.hpp>` includes everything including the offline parsers:

```cpp
#include <sasdk/mod.hpp>     // full runtime
#include <sasdk/gxt.hpp>     // GXT text archive
#include <sasdk/img.hpp>     // IMG archive
#include <sasdk/col.hpp>     // COL2 collision
#include <sasdk/rw.hpp>      // RenderWare stream
#include <sasdk/fxp.hpp>     // FXP particle project
#include <sasdk/save.hpp>    // save file blocks
```

<img width="100%" src="https://capsule-render.vercel.app/api?type=rect&color=0:000000,50:E07B00,100:000000&height=3"/>

## 📐 Quick-reference — Which header for what

| Task | Header | Namespace |
|---|---|---|
| Parse GXT text archive | `gxt.hpp` | `sasdk` |
| Open IMG archive | `img.hpp` | `sasdk` |
| Parse COL2 collision | `col.hpp` | `sasdk` |
| Traverse DFF/TXD/IFP chunks | `rw.hpp` | `sasdk` |
| Parse FXP particle project | `fxp.hpp` | `sasdk` |
| Read save file blocks | `save.hpp` | `sasdk` |
| Error type | `core/result.hpp` | `sasdk` |
| Module rebase | `core/address.hpp` | `sasdk` |
| Inline/VMT hooks | `core/hook.hpp` | `sasdk::rt` |
| Typed function calls | `core/invoke.hpp` | `sasdk::rt` |
| Typed game globals | `core/global.hpp` | `sasdk` |
| Struct database | `game/sa10us/sa10us_db.inl` | `sasdk::sa10us` |
| Function handles | `game/sa10us/functions.inl` | `sasdk::sa10us::fn` |
| Global addresses | `game/sa10us/addresses.hpp` | `sasdk::sa10us::addr` |
| Easy helper API | `game/sa10us/sa10us_easy.hpp` | `sasdk::sa10us::easy` |
| All runtime | `mod.hpp` | all of the above |
| Everything | `sasdk/sasdk.hpp` | all of the above |

<div align="center">

<sub>SASDK Functions Guide · Built and maintained by <a href="https://github.com/TsyVM">TsyVM</a> · <a href="https://www.teamvanilla.org/">TeamVanilla</a></sub>

<img width="100%" src="https://capsule-render.vercel.app/api?type=waving&color=0:8B4500,100:000000&height=80&section=footer"/>

</div>
