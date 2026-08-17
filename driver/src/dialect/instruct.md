# src/dialect/ - Dialect Enum, Config, and Dispatch (Phase 1a)

## Purpose

This directory owns the single source of truth for **which LPC dialect the
driver is running** and provides the concrete `BootApi` subclasses that
`src/apply` uses to route master/simul_efun/driver-object applies.

This is a **Phase 1 directory** - it does not exist yet as compiled code.
Create it only after Phase 0 is complete and the test suite is green.

## Files to create

### `include/lpcdriver/dialect/LpcDialect.hpp`

```cpp
#pragma once
#include <string>
namespace lpcdriver {

enum class LpcDialect {
    FluffOS,   // MudOS/FluffOS (: :) LPC - current default
    LdMud,     // Amylaar/LDMud #' lambda() LPC
    DGD,       // Dworkin's Generic Driver nil/atomic/rlimits LPC
};

const char* dialectName(LpcDialect d);  // "fluffos" / "ldmud" / "dgd"
LpcDialect dialectFromString(const std::string& s); // throws on unknown

} // namespace lpcdriver
```

### `include/lpcdriver/dialect/BootApi.hpp`

Abstract interface - see `src/apply/instruct.md` Phase 1.4 for the full
interface definition. Place the abstract base here; concrete implementations
in the `.cpp` files below.

### `src/dialect/FluffOsBootApi.cpp` + header

FluffOS/MudOS apply names:
- `connectApply()` → `"connect"`
- `logonApply()` → `"logon"`
- `compileObjectApply()` → `"compile_object"`
- `privsFileApply()` → `"privs_file"`
- `netDeadApply()` → `"net_dead"`
- `heartBeatErrorApply()` → `"heart_beat_error"`
- `hasAutoObject()` → false
- `simulEfunFile()` → `Config::simulEfunFile()` (may be empty)

### `src/dialect/LdmudBootApi.cpp` + header

LDMud apply names - all of the above, plus:
- `"get_root_uid"`, `"get_bb_uid"`, `"valid_read"`, `"valid_write"`,
  `"make_path_absolute"`, `"query_allow_shadow"`
- `hasAutoObject()` → false

### `src/dialect/DgdBootApi.cpp` + header

DGD driver+auto model:
- `masterFile()` → driver object path (e.g. `/kernel/driver`)
- `simulEfunFile()` → `std::nullopt`
- `connectApply()` → `"connect"`, `compileObjectApply()` → `"compile_object"`
- `"initialize"` - driver object boot callback
- `"path_read"` / `"path_write"` - path permission callbacks
- `"disconnect"` - link-death callback (DGD name for net_dead)
- `hasAutoObject()` → true
- `autoObjectFile()` → `Config::autoObjectFile()` (e.g. `/kernel/auto`)

### `src/dialect/DialectFactory.cpp` + header

```cpp
std::unique_ptr<BootApi> makeBootApi(LpcDialect d, const Config& config);
```

Called from `main.cpp` after `Config::loadFromFile()`.

## How dialect flows through the driver

```
Config::loadFromFile()
  → Config::dialect() returns LpcDialect enum
    → DialectFactory::makeBootApi(dialect, config)
      → unique_ptr<BootApi> passed to:
        - ApplyTable  (apply name routing)
        - ObjectManager (auto-inherit injection when DGD)
        - Lexer / Parser (dialect-specific token set)
        - VM (nil/atomic/rlimits behavior flags)
```

## CMakeLists.txt for this directory

```cmake
add_library(dialect STATIC
    FluffOsBootApi.cpp
    LdmudBootApi.cpp
    DgdBootApi.cpp
    DialectFactory.cpp
)
target_include_directories(dialect PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(dialect PUBLIC config)
```

Add `add_subdirectory(src/dialect)` and `dialect` to the link line in the
top-level `driver/CMakeLists.txt`.

## Testing

`tests/test_dialect_fluffos.cpp`, `test_dialect_ldmud.cpp`,
`test_dialect_dgd.cpp` - see `tests/instruct.md` Phase 1 task list.

Each test sets `Config::dialect_` and verifies correct apply names,
token recognition, and runtime semantic differences.
