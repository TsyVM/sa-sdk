# SASDK v0.1.0 — Initial Release

**Target:** sa10us · HOODLUM 1.0 US · `170b3a9108687b26da2d8901c6948a18`  
**Standard:** C++20 · MSVC / GCC / Clang · CMake 3.16+

---

## What's included

### Struct database (`SASDK::game`)

Generated from 302 reverse-engineered JSON schemas covering the sa10us binary. Offsets verified by Capstone disassembly sweep and plugin-sdk `VALIDATE_OFFSET` compile-time asserts.

**100%-coverage structs:** `CPhysical` · `CPed` · `CPlayerPed` · `CVehicle` · `CAutomobile` · `CAutoPilot` · `CWanted` · `CGangInfo` · `CStats` · `CGangWars`

**Novel findings not documented elsewhere:**
- `CHandlingData` size = **0xE0 (224 B)** — community had 0xD4; proven by IMUL stride at `0x6F0151`
- `ms_vehicleHandling` array at `0xC2B9DC`; sub-handling at `0xC3BB00` (stride 0x94)
- `CVehicleModelInfo[+0x4A]` = handling index byte
- `CExplosion` type count = **21** (0–20); proven by `cmp eax, 0x14` at `0x73702C`
- Blast radius float table extracted from rdata `0x8592D4`
- `m_nVehicleFlags @0x4EC` — 52 named bitflags previously anonymous in plugin-sdk
- `m_nHandlingFlagsIntValue @0x38C` — 29 handling bitflags

### Offline format parsers (`SASDK::data`)

Derived from RE-Data binary format specs, each tested against synthetic vectors:

| Parser | Format | Highlights |
|--------|--------|-----------|
| `GxtArchive` | GXT text | TABL/TKEY/TDAT; binary search by CRC-32 (no final complement); `lookup(table, key)` |
| `ImgArchive` | IMG VER2 | 32B TOC; 2048-byte sector addressing; `find` + `read` by name |
| `ColArchive` | COL2 | Multi-model; verified geometry element sizes; raw span access |
| `RwStream` / `RwChunk` | RenderWare | Full chunk type enum; nested child traversal; `first_child`, `struct_data` |
| `FxpArchive` | FXP particle | Count-driven text grammar; full hierarchy (systems → prims → infos → curves → keyframes) |
| `SaveArchive` | Save blocks | `BLOCK ` framing; per-block data access; additive checksum |

### Runtime layer (`SASDK::game`, header-only)

- `InlineHook` — 5-byte JMP patch with save/restore
- `VmtHook` — vtable slot swap
- `Global<T>` / `ArrayView<T>` — typed game memory access
- `call_cdecl` / `call_thiscall` — type-safe function invocation with rebased addresses
- `fn::*` — 9 typed callable handles for verified game functions

### Host test suite

93 tests covering struct offsets, hook byte encoding, address math, and all six format parsers. Runs on 64-bit hosts without the game.

---

## CMake integration

```cmake
add_subdirectory(SASDK)
target_link_libraries(my_asi PRIVATE SASDK::sasdk)
```

For tools that only need the offline parsers (no game structs):

```cmake
target_link_libraries(my_tool PRIVATE SASDK::data)
```

---

## License

MIT
