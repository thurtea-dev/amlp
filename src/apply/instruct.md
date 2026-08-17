# src/apply/ - ApplyTable: Master/SimulEfun Apply Dispatch

## What lives here

| File | Role |
|------|------|
| `ApplyTable.cpp` + `include/.../ApplyTable.hpp` | Named apply dispatch: calls a function by string name on the master or simul_efun object. Backs `VM::applyMaster()`. |

An "apply" is a driver-to-mudlib callback: the driver calls a specific function
on a specific object by name. Examples: `connect()`, `logon()`,
`compile_object()`, `valid_read()`, `heart_beat()`.

## Files to read before touching this directory

- `include/amlp/apply/ApplyTable.hpp`
- Reference: `fluffos-2.9-ds2.08/applies.h` - the real apply name constants
- Reference: `fluffos-2.9-ds2.08/simulate.c` `safe_apply()` and `apply()`

## Phase 0 tasks

No urgent stub gaps here. Verify coverage:
- All applies listed in `applies.h` that are called by the FluffOS runtime
  should be documented (even if not all are wired up) in a comment at the
  top of `ApplyTable.cpp`.

## Phase 1 tasks

### 1.4 - Pluggable boot API

Currently the apply system is hard-wired to the FluffOS master/simul_efun
model. Phase 1 requires that the same machinery support three different apply
name sets.

**What to build:**

1. **`BootApi` abstract base class** (new file
   `include/amlp/apply/BootApi.hpp`):
   ```cpp
   class BootApi {
   public:
       virtual ~BootApi() = default;
       virtual std::string masterFile() const = 0;
       virtual std::optional<std::string> simulEfunFile() const = 0;
       virtual std::string connectApply() const = 0;
       virtual std::string logonApply() const = 0;
       virtual std::string compileObjectApply() const = 0;
       virtual std::string privsFileApply() const = 0;
       virtual std::string netDeadApply() const = 0;
       virtual std::string heartBeatErrorApply() const = 0;
       virtual bool hasAutoObject() const = 0;
       virtual std::optional<std::string> autoObjectFile() const = 0;
   };
   ```

2. **`FluffOsBootApi`**: returns FluffOS/MudOS apply names
   (`"connect"`, `"logon"`, `"compile_object"`, `"privs_file"`, `"net_dead"`,
   `"heart_beat_error"`). `hasAutoObject()` → false.

3. **`LdmudBootApi`**: LDMud apply names (`"connect"`, `"logon"`,
   `"compile_object"`, `"valid_read"`, `"valid_write"`, `"get_root_uid"`,
   `"get_bb_uid"`). Shadow policy applies (see `src/object`).
   `hasAutoObject()` → false (LDMud uses simul_efun differently).

4. **`DgdBootApi`**: DGD has no master/simul_efun in the FluffOS sense.
   Instead: **driver object** handles boot callbacks
   (`"initialize"`, `"path_read"`, `"path_write"`, `"compile_object"`,
   `"connect"`, `"disconnect"`) and **auto object** is implicitly inherited
   by every other object. `hasAutoObject()` → true;
   `autoObjectFile()` → `/kernel/auto` (configurable).

5. **`ApplyTable`** takes a `const BootApi&` and routes all applies through
   it. `VM::applyMaster()` is unchanged - it just calls `ApplyTable`.

6. **`src/dialect/instruct.md`** owns the code that constructs the right
   `BootApi` subclass based on the configured `LpcDialect`.

### 1.15 - DGD driver+auto object boot path

When `DgdBootApi` is active:
- On boot, load the driver object (`/kernel/driver` or `Config::masterFile()`).
- Call `driver_object->initialize()`.
- For every subsequently loaded object, **inject an `inherit "/kernel/auto";`**
  at the top of its compiled program - this is the DGD auto-inherit mechanism.
  Implement this as an `ObjectManager::compile()` preprocessing step that
  prepends the inherit declaration when `DgdBootApi::hasAutoObject()` is true
  and the file is not the auto object itself.

### 1.16 - LDMud master apply name table

LDMud's master has a richer apply surface than FluffOS. The key additional
applies beyond the FluffOS set:
- `"get_root_uid"` - returns the root UID string
- `"get_bb_uid"` - returns the backbone (fallback) UID
- `"valid_read"` / `"valid_write"` - path-level filesystem permission checks
- `"make_path_absolute"` - resolve a relative path
- `"query_allow_shadow"` - shadow permission check (see `src/object`)

Add these to `LdmudBootApi` and wire them into `ApplyTable`'s apply-dispatch
methods.

## Phase 2 tasks

### 2.9 - Apply cache

The apply cache sits logically in `src/apply` because it caches the result of
"what function does this object have for this apply name?".

**What to build:**
1. `ApplyCache` singleton (or a member of `ApplyTable`):
   `unordered_map<pair<LpcObject*, string>, FunctionEntry*>`.
2. On a miss: run the existing `findFunctionInChain()` walk; store the result.
3. On hit: skip the walk, call directly.
4. Invalidation: when `ObjectManager::loadObject()` recompiles an object,
   erase all cache entries where the first element matches the recompiled
   object OR any object that inherits from it (the inheritance chain must be
   invalidated too - track the reverse inherit map).
5. On `destructObject()`: erase all entries for the destroyed object.

**Expected impact:** eliminates the dominant hot path in combat/heartbeat storms
where `call_heart_beat()` calls `apply("heart_beat", ob)` on every living
object every 2 seconds.

## Key invariants

- All apply dispatch (master, simul_efun, per-object) must check
  `obj->isDestructed()` before calling - see `src/object/instruct.md` Phase 0.5.
- Applies that return `void` in the spec should silently swallow the result
  rather than throwing on a non-void return.
- The `BootApi` abstraction must not be bypassed: never hardcode `"connect"` or
  any other apply name string outside of a `BootApi` subclass.
