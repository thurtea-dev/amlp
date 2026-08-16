# src/efun/ — EfunTable: All Registered Efuns

## What lives here

| File | Role |
|------|------|
| `EfunTable.cpp` + `include/.../EfunTable.hpp` | Singleton registry mapping efun name → `EfunFn`. `registerCoreEfuns()` is called from `main.cpp`. |

Currently **179 registered names** (including aliases, confirmed via
`grep -oE 'registerEfun\("[a-zA-Z_0-9]+"'`, not a stale estimate). Target is
**~300** for FluffOS parity and **beyond 300** for the features that exceed
all three reference drivers.

## Files to read before touching this directory

- `include/lpcdriver/efun/EfunTable.hpp`
- `include/lpcdriver/vm/Value.hpp` — every efun signature is `Value(VM&, vector<Value>&)`
- `docs/ROADMAP.md` — efun tasks are spread across Phase 0, 1, 2, and 3
- Reference: `fluffos-2.9-ds2.08/func_spec.c` — the canonical efun list (~180
  core names)
- Reference: `fluffos-2.9-ds2.08/packages/` — ~120 more package efuns
- Reference: `fluffos-2.9-ds2.08/efuns_main.c`, `simulate.c`, `array.c`,
  `mapping.c`, `string.c` — implementation of the core efuns

## Phase 0 tasks (highest priority)

### 0.1 — `throw(mixed)`

`throw()` is the other half of `catch()`. Without it, mudlib code that uses
the `catch`/`throw` idiom to signal errors silently fails.

**Spec (from `func_spec.c`):** `void throw(mixed);`

**What to build:**
1. `LpcThrownValue` already exists in `Value.hpp` — this efun just needs to
   construct and throw it.
2. Register as `"throw"` in `registerCoreEfuns()`.
3. Implementation:
   ```cpp
   registerEfun("throw", [](VM& vm, std::vector<Value>& args) -> Value {
       if (args.empty()) throw LpcRuntimeError("throw() requires an argument");
       throw LpcThrownValue(args[0]);
       return Value{};
   });
   ```
4. In `VM::run()`'s `PopCatchFrame` handler: catch `LpcThrownValue` separately
   from `LpcRuntimeError`; push `thrown.value` onto the stack as the catch
   expression's result (not `thrown.what()`).
5. Add 3 regression tests: throw a string, throw an int, throw inside nested
   catch frames.

### 0.2 — `sscanf` full format set

Current gaps: `%f` (float), `%x` (hex int), `%(regexp)` capture, adjacent
`%s%...` without literal text between them.

**Reference:** `fluffos-2.9-ds2.08/efuns_main.c` `f_sscanf()` and
`sscanf_regexp()`.

Add each format specifier one at a time with a regression test per specifier.
For `%(regexp)`: depends on the PCRE efun work (0.11) — do that first.

### 0.3 — `sprintf` `%*` dynamic field width

`%*` means "read the next argument as the field width integer". Currently
throws "unsupported format specifier".

**Reference:** `fluffos-2.9-ds2.08/sprintf.c` `INFO_STAR_*` handling.

One small change to the `sprintf` lambda: when `*` is seen after `%`, pop the
next arg as `int64_t` and use it as `fieldWidth` (or `precision` if `%.*`).

### 0.7 — `save_object`/`restore_object` in FluffOS `.o` text format

Currently the driver uses a custom binary format that is incompatible with all
existing FluffOS mudlib save files.

**FluffOS `.o` format spec** (from `save_object.c`):
- One line per variable: `varname value\n`
- Integers: bare decimal
- Strings: double-quoted with `\` escapes
- Arrays: `({item,item,...})`
- Mappings: `([key:value,key:value,...])`
- Nested structures: recursive

**What to build:**
1. Add a `SaveFormat` enum: `Custom` (current), `FluffOS`.
2. New static methods `serializeFluffOS(const LpcObject&)` and
   `restoreFluffOS(LpcObject&, const string& data)`.
3. The `save_object` efun checks `Config::saveFormat()` (add this config key)
   and calls the appropriate serializer.
4. Add 5+ regression tests covering all value kinds including nested arrays.

### 0.9 — `map`/`filter`/`sort_array` as real closure consumers

Currently these efuns may accept only simple callbacks or not be registered.

**Reference:** `fluffos-2.9-ds2.08/array.c` `f_map_array()`,
`f_filter_array()`, `f_sort_array()`.

Each takes an array and a function-pointer (closure or string name):
- `map(arr, fn)` → new array of `fn(elem)` results
- `filter(arr, fn)` → new array of elements where `fn(elem)` is truthy
- `sort_array(arr, fn)` → sorted array using `fn(a,b)` < 0 / 0 / > 0

Use `VM::callClosure()` for closure args and `VM::callFunction()` for string
args (look up the function name on the current object).

Also register `map_array`/`filter_array` as aliases (FluffOS has both spellings).

### 0.11 — PCRE regexp efuns

Wrap PCRE2 (link `-lpcre2-8`):
- `regexp(string, pattern)` → 1 if matches, 0 if not
- `regexplode(string, pattern)` → array of alternating non-match/match substrings
- `reg_assoc(string, patterns_array, tokens_array, default)` → tokenize string
- `regexp_assoc` — alias

Add `find_package(PkgConfig)` + `pkg_check_modules(PCRE2 REQUIRED libpcre2-8)`
to `driver/CMakeLists.txt` and link `efun` against it.

### 0.13 — Grow to ~300 efuns (FluffOS parity)

After the above Phase 0 items, audit `func_spec.c` AND `efun_defs.c` (the
auto-generated dispatch table, ground truth over the hand-edited text file
when the two disagree, confirmed real for `debug_info` and several others
below) against the current `EfunTable.cpp` registration list, then rank the
remaining gap by real call-site frequency across `mudlib/`, excluding
`/doc/` (grep hits there are documentation, not code). Tier the work.

**Note on the previous version of this list:** it named 17 efuns that turned
out not to be real names in this reference build at all (`string_to_array`,
`trim`, `pad`, `count`, `slice_array`, `flatten_array`, `m_add`, `mappingp`,
`round`, `atan2`, `append_file`, `file_exists`, `copy_file`,
`caller_stack_depth`, `caller_stack`, `trace`, `traceing`), zero hits in
`func_spec.c`, `efun_defs.c`, `efunctions.h`, or `opc.h`. Confirmed and
removed. `debug_info` is real (`efun_defs.c`) but has zero call sites in
this mudlib, no implementation anywhere in the vendored source tree (only
its prototype), and its documented behavior dumps FluffOS-internal C struct
fields (`O_HEART_BEAT`, `swap_num`, `next_all`) with no clean equivalent in
this driver's object model. Excluded from the implementation queue, same
category as `get_char` below, not a usage-weight tradeoff.

**Tier 1, first pass (top 15 by real call-site count, current as of the
2026-08-16 efun-growth sessions):** superseded by the corrected pass
below, kept here for the git-blame trail rather than deleted. All ten
"Real gap" rows from that pass (`base_name`, `debug_message`, `rusage`,
`command`, `shutdown`, `uptime`, `in_edit`, `in_input`, `match_path`,
`call_out_info`) are done -- confirmed registered in `EfunTable.cpp` and
covered by `tests/test_lexer.cpp` (see STATUS.md's own dated entries for
each). `tell_object`/`tell_room`/`say`/`shout` are excluded from every
pass of this ranking entirely, not just deprioritized -- all four are
already real, confirmed simul_efuns in this mudlib's own
`secure/SimulEfun/communications.c`, so this driver's existing tier-3
(simul_efun) call resolution already handles every real call site; a core
efun registration would be unreachable, shadowed code. Same for
`translate` (`secure/SimulEfun/translate.c`) and `event`
(`secure/SimulEfun/events.c`).

**Methodology correction, found this pass, worth flagging for whoever
runs the next one:** the call-site frequency count for this second pass
was first run against the whole `mudlib/` tree, same as the first pass
apparently was -- that directory also contains the *vendored C reference
driver itself* (`mudlib/nightmare3_fluffos_v2/fluffos-2.9-ds2.08/`,
including its own `testsuite/`), which is C source, not LPC, and is not
this driver's own mudlib content at all. Counting hits there inflated
several efuns' real-usage numbers substantially (`get_config` looked
like 7 calls/4 files; the real number, scoped correctly to
`mudlib/nightmare3_fluffos_v2/lib/`, is zero). Re-run scoped to that one
directory (still excluding its own `/doc/`) for this pass; every count
below is post-correction. A second, smaller trap found the same way:
`\bname\s*\(` (allowing whitespace between the identifier and the open
paren) also matches ordinary English prose like "Admin commands (setrole,
..." -- tightened to `name\(` (no whitespace) for this pass, which is
what actually distinguishes a real LPC call from a parenthetical remark
in a comment or player-facing string.

**Tier 1, corrected pass (2026-08-20), scoped to `mudlib/nightmare3_fluffos_v2/lib/`
only, `/doc/` and the driver's own C source excluded, whitespace-tight
match:**

| Rank | Efun | Calls | Files | Status |
|---|---|---|---|---|
| 1 | `ed` | 4 | 3 | Real gap, genuinely used (`secure/cmds/ambassador/_ed.c`, `std/user/editor.c`, `std/user/nmsh.c`'s own "e" command) -- needs new Connection-level infrastructure: a stateful multi-line interactive text-editing mode with its own command language, not a self-contained efun body. Not implemented this batch, same category as `get_char`. |
| 2 | `deep_inherit_list` | 3 | 3 | Real gap. Implemented. |
| 2 | `call_stack` | 3 | 1 | Real gap. Implemented partially: mode 1 (object stack) and mode 0 (program filenames, derived from the same stack) are faithful; modes 2 (function names) and 3 (per-frame origin) throw a clear "not implemented" error -- this driver's call stack tracks objects only, no per-frame function-name or origin tagging exists to report. |
| 3 | `objects` | 2 | 2 | Real gap, needed a live-object registry (every loaded object, not just currently-connected players -- `InteractiveRegistry` only ever covered the latter). Implemented: new `LiveObjectRegistry` (`src/object/`), mirroring `LivingNameRegistry`'s own weak_ptr-registry shape. |
| 3 | `socket_address` | 2 | 2 | Real gap. Implemented (reuses `SocketRegistry`'s existing local/remote address fields, and `Connection`'s own peer address for the interactive-object argument form). |
| 3 | `origin` | 2 | 1 | Real gap, but needs new infrastructure this driver's VM has none of: per-call origin tagging (`LOCAL`/`CALL_OTHER`/`DRIVER`/`EFUN`/`SIMUL_EFUN`/`FUNCTION_POINTER`) threaded through every call path (`Call`/`CallEfun`/`CallParent` opcodes, `VM::callFunction()`/`callClosure()`). Not implemented -- flagged rather than guessed, since the one real call site (`secure/daemon/chat.c`) uses it as a security gate (`origin() != ORIGIN_LOCAL`), where a wrong answer is a real security-correctness bug, not just an incomplete feature. |
| 4 | `livings` | (2, simul_efun form) | 2 | Corrected from the first pass's framing: `livings()` as a *bare call* is fully shadowed by a real simul_efun (`secure/SimulEfun/SimulEfun.c`: `object *livings() { return efun::livings() - (efun::livings() - objects()); }`), the same "unreachable core registration" category as `tell_object` etc -- but that simul_efun's own body calls `efun::livings()` explicitly (this driver's `efun::name()` escape hatch is already real and working, confirmed in `Lexer.cpp`/`Parser.cpp`), so the *core* efun is genuinely load-bearing, just reached only through that one file. Implemented alongside `objects()` (same new `LiveObjectRegistry`). |
| 5 | `virtualp` | 1 | 1 | Real gap. Implemented: new `LpcObject::isVirtual_` flag, set only by `ObjectManager::loadVirtualObject()`'s own construction path. |
| 5 | `clonep` | 1 | 1 | Real gap. Implemented with no new infrastructure: `VM::lookupObject(ob->filename())` identity-compared against `ob` itself already distinguishes a blueprint (registered in `ObjectManager::loaded_`) from a clone (same filename, but never itself the `loaded_` entry). |
| 5 | `resolve` | 1 | 1 | Real gap, but needs new infrastructure this driver has no equivalent for at all: real `resolve()` is not a plain blocking DNS call, it is a full async IPC protocol against a separate address-server daemon process (`comm.c`'s own `query_addr_number()`/`got_addr_number()`, a UDP-style query/response table keyed by handle, firing `callback(string name, string number, int handle)` only once that daemon replies). A synchronous `getaddrinfo()`-and-fire-immediately stand-in was considered and rejected -- close enough to look done but not faithful enough to trust, for one real call site. Not implemented. |
| 5 | `commands` | 1 | 1 | Real gap. Corrected: the first pass's Tier 2 list claimed a related `query_actions` was "already done -- verify"; it is not a real efun name in this reference build at all (zero hits anywhere in `efun_defs.c`), and was never registered. Implemented via `LpcObject::actions()` (the same table `add_action`/`remove_action` already use), matching real `array.c`'s own `commands()` helper's exact 4-element-per-entry shape (`({verb, flags, owner, function_name})`). |
| 5 | `query_host_name` | 1 | 1 | Real gap, though the one hit is buried inside `secure/include/network.h`'s own (UDP-intermud) `START_MSG` macro rather than a direct call site -- genuinely real, just marginal. Implemented (returns the configured hostname). |
| 5 | `flush_messages` | 1 | 1 | Real gap, but this driver's `Connection::send()` already writes synchronously with no output-buffering layer to flush -- the real observable behavior ("force any buffered output out now") is already this driver's default for every write. Implemented as an interactive-check no-op rather than a fabricated buffering mechanism. |
| 5 | `mud_status` | 1 | 1 | Architecture mismatch, not implemented: real behavior dumps driver-internal state (hash tables, function cache, shared-string interning, heart-beat internal counters) this driver's own object/VM model has no equivalent for at all -- a "real" implementation would be entirely fabricated data, same category as the already-excluded `debug_info`. |
| 5 | `cache_stats` | 1 | 1 | Same architecture-mismatch category as `mud_status` (apply-cache internals this driver doesn't track the same way real FluffOS does). Not implemented. |

Also real, confirmed, and implemented alongside `deep_inherit_list` above
as a direct byproduct of the same inherit-chain-walking code (not
separately ranked -- both showed 0 real call sites in this mudlib):
`shallow_inherit_list` and its real alias `inherit_list`
(`F_SHALLOW_INHERIT_LIST | F_ALIAS_FLAG` in `efun_defs.c` -- the exact
same efun code as `shallow_inherit_list`, confirmed directly, not two
separate implementations).

**Also found this pass, excluded from the ranking table (not close
enough to real gameplay code to rank, but noted so they are not
rediscovered from scratch next time):** `allocate_buffer`, `read_buffer`,
`write_buffer`, `bufferp` all need a `TYPE_BUFFER`/bytes value kind this
driver's `Value` variant has never had (the same pre-existing gap already
noted on `to_int()`'s own buffer case and on row 0.10's excluded socket
BINARY modes) -- architecture mismatch, not a quick efun body, for all
four at once. `author_stats`, `domain_stats`, `dump_file_descriptors`,
`dumpallobj`, `malloc_status` are real (each has exactly one genuine call
site in `mudlib/nightmare3_fluffos_v2/lib/`) but sit in the identical
architecture-mismatch category as `mud_status`/`cache_stats` above --
driver-internal C-struct dumps this driver's model has nothing
corresponding to. `children` looked real in the first raw grep pass (2
hits) but both turned out to be descriptive comments explaining the
efun's own behavior (`secure/SimulEfun/get_object.c`,
`secure/cmds/creator/_eval.c`), not real calls -- zero actual call sites,
excluded. The six `_`-prefixed `efun_defs.c` entries (`_call_other`,
`_evaluate`, `_new`, `_this_object`, `_to_float`, `_to_int`) are not
separate efuns at all -- internal aliases of the plain names this driver
already registers (`call_other`, `evaluate`, `new`, `this_object`,
`to_float`, `to_int`), confirmed by identical `F_CODE` values in
`efun_defs.c`.

**Tier 2 (medium effort), corrected:** `query_actions` removed (not a
real efun name, see `commands`'s own row above). Everything else in the
first pass's Tier 2 list is now done: `add_action`, `remove_action`,
`socket_create`, `socket_bind`, `socket_connect`, `socket_write`,
`socket_close`, `socket_error`, `socket_status`, `interactive`,
`query_ip_number`, `query_ip_name`, `query_idle`, `set_living_name`,
`find_living`, `find_player`, `living`, `query_once_interactive`,
`enable_commands`, `disable_commands`. Not real (grepped `efun_defs.c`
directly, zero hits): `socket_read` (row 0.10's own STATUS.md entry
already covers this -- real sockets are callback-driven, no synchronous
read efun exists), `sockets_status`, `net_connect`, `add_verb`,
`remove_verb`.

**Tier 3 (lower urgency, high effort):**
Socket package (advanced: `socket_acquire`/`socket_release`, object-to-
object socket transfer, out of "basics" scope), database package, crypto
package — see Phase 2.

## Phase 2 tasks

### 2.15 — SQLite built-in database efuns

Link against SQLite3 (`find_package(SQLite3 REQUIRED)`).

New efuns:
- `db_connect(string path)` → int handle
- `db_exec(int handle, string sql)` → int rows_affected
- `db_fetch(int handle, string sql)` → array of mapping rows
- `db_close(int handle)`
- `db_error(int handle)` → string last error

Maintain an internal `vector<sqlite3*>` indexed by handle. Handles are
per-connection, not global (thread-safety not needed in single-threaded model).

### 2.16 — Hash efuns

Link against OpenSSL (`find_package(OpenSSL REQUIRED)`):
- `hash(string algorithm, string data)` → hex string
  Algorithms: `"sha256"`, `"sha512"`, `"md5"`, `"blake2b"`
- `bcrypt_hash(string password, int cost)` → string
- `bcrypt_verify(string password, string hash)` → int

### 2.17 — `json_encode`/`json_decode`

Embed `nlohmann/json.hpp` (single-header, no extra deps):
- `json_encode(mixed value)` → string (JSON text)
- `json_decode(string json)` → mixed (LPC Value)

Mappings become JSON objects; arrays become JSON arrays; strings/ints/floats
map directly; `nil` (DGD) / `0` (FluffOS) maps to JSON null.

### 2.18 — `http_get`/`http_post` async efuns

Backed by the async scheduler (Phase 2.5/2.6). Use libcurl async callbacks:
- `http_get(string url)` → awaitable: suspends until response
- `http_post(string url, string body, string content_type)` → awaitable

### 2.22 — LPC-native test runner

New efuns for use inside `.c` test files:
- `assert_equal(mixed a, mixed b)` — throws if not equal
- `assert_not_equal(mixed a, mixed b)`
- `assert_throws(function fn)` — catches and returns the error string
- `test_pass(string name)` / `test_fail(string name, string reason)`
- `run_tests(object test_ob)` — run all `test_*` functions in the object

These efuns write structured results to a JSON file for CI integration.

## Testing

Every efun must have at least one regression test in `tests/` before Phase 1
begins. Prefer `tests/test_efun_<name>.cpp` for new efun tests.

```bash
ctest --test-dir driver/build -R "efun" --output-on-failure
```

## Key invariants

- Efun signatures are `Value(VM&, vector<Value>&)`. Never change this.
- Check arg count and types at the start of every efun and throw
  `LpcRuntimeError` with a clear message on mismatch.
- Register aliases (`new`/`clone_object`, `evaluate`/`funcall`) using the same
  lambda so the implementation is never duplicated.
- An efun that calls back into LPC (e.g. `map`, `filter`, `sort_array`) must
  catch `LpcRuntimeError` from the callback and re-throw with added context.
