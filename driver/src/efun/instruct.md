# src/efun/ — EfunTable: All Registered Efuns

## What lives here

| File | Role |
|------|------|
| `EfunTable.cpp` + `include/.../EfunTable.hpp` | Singleton registry mapping efun name → `EfunFn`. `registerCoreEfuns()` is called from `main.cpp`. |

Currently **~144 registered names** (including aliases). Target is **~300** for
FluffOS parity and **beyond 300** for the features that exceed all three
reference drivers.

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

After the above Phase 0 items, audit `func_spec.c` against the current
`EfunTable.cpp` registration list and implement the remaining core efuns in
priority order. Tier the work:

**Tier 1 (high usage, moderate effort):**
`strsrch`, `replace_string` (full bounds), `string_to_array`, `capitalize`,
`lower_case`, `trim`, `pad`, `repeat_string`, `count`, `slice_array`,
`unique_array`, `member_array`, `flatten_array`, `allocate`, `allocate_mapping`,
`keys`, `values`, `m_delete`, `m_add`, `sizeof` on all types, `typeof`,
`objectp`, `stringp`, `intp`, `floatp`, `arrayp`, `mappingp`, `functionp`,
`classp`, `pointerp`, `undefinedp`, `nullp`, `random`, `time`, `ctime`,
`localtime`, `abs`, `min`, `max`, `pow`, `sqrt`, `floor`, `ceil`, `round`,
`log`, `exp`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`,
`read_file`, `write_file`, `append_file`, `file_size`, `file_exists`,
`get_dir`, `mkdir`, `rmdir`, `rm`, `rename`, `stat`, `read_bytes`,
`write_bytes`, `copy_file`, `link`, `users`, `this_player`, `this_object`,
`previous_object`, `all_previous_objects`, `caller_stack_depth`,
`caller_stack`, `trace`, `traceing`, `debug_info`.

**Tier 2 (medium effort):**
`add_action`, `remove_action`, `query_actions` (already done — verify),
`socket_create`, `socket_bind`, `socket_connect`, `socket_write`,
`socket_read`, `socket_close`, `socket_error`, `socket_status`,
`sockets_status`, `net_connect`, `resolve`, `interactive`, `query_ip_number`,
`query_ip_name`, `query_idle`, `set_living_name`, `find_living`, `find_player`,
`living`, `query_once_interactive`, `enable_commands`, `disable_commands`,
`add_verb`, `remove_verb`.

**Tier 3 (lower urgency, high effort):**
Socket package, database package, crypto package — see Phase 2.

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
