# src/efun/ - EfunTable: All Registered Efuns

## What lives here

| File | Role |
|------|------|
| `EfunTable.cpp` + `include/.../EfunTable.hpp` | Singleton registry mapping efun name → `EfunFn`. `registerCoreEfuns()` is called from `main.cpp`. |

Currently **219 registered names** (including aliases, confirmed via
`grep -oE 'registerEfun\("[a-zA-Z_0-9]+"'`, not a stale estimate). Target is
**~300** for FluffOS parity and **beyond 300** for the features that exceed
all three reference drivers.

**Ranking source changed 2026-08-22:** the old `nightmare3_fluffos_v2`
Rifts mudlib every ranking pass below this point ranked call-site
frequency against stayed behind in the AetherMUD monorepo when this
driver was extracted into its own standalone repository -- `mudlib/` is
now just the bundled Lil starter mudlib. New batches rank against Lil's
own real efun conformance suite instead
(`mudlib/single/tests/efuns/*.c`, one file per real efun) -- see
STATUS.md's own 2026-08-22 entry for the first batch done this way.

## Files to read before touching this directory

- `include/amlp/efun/EfunTable.hpp`
- `include/amlp/vm/Value.hpp` - every efun signature is `Value(VM&, vector<Value>&)`
- `ROADMAP.md` - efun tasks are spread across Phase 0, 1, 2, and 3
- Reference: `fluffos-2.9-ds2.08/func_spec.c` - the canonical efun list (~180
  core names)
- Reference: `fluffos-2.9-ds2.08/packages/` - ~120 more package efuns
- Reference: `fluffos-2.9-ds2.08/efuns_main.c`, `simulate.c`, `array.c`,
  `mapping.c`, `string.c` - implementation of the core efuns

## Phase 0 tasks (highest priority)

### 0.1 - `throw(mixed)`

`throw()` is the other half of `catch()`. Without it, mudlib code that uses
the `catch`/`throw` idiom to signal errors silently fails.

**Spec (from `func_spec.c`):** `void throw(mixed);`

**What to build:**
1. `LpcThrownValue` already exists in `Value.hpp` - this efun just needs to
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

### 0.2 - `sscanf` full format set

Current gaps: `%f` (float), `%x` (hex int), `%(regexp)` capture, adjacent
`%s%...` without literal text between them.

**Reference:** `fluffos-2.9-ds2.08/efuns_main.c` `f_sscanf()` and
`sscanf_regexp()`.

Add each format specifier one at a time with a regression test per specifier.
For `%(regexp)`: depends on the PCRE efun work (0.11) - do that first.

### 0.3 - `sprintf` `%*` dynamic field width

`%*` means "read the next argument as the field width integer". Currently
throws "unsupported format specifier".

**Reference:** `fluffos-2.9-ds2.08/sprintf.c` `INFO_STAR_*` handling.

One small change to the `sprintf` lambda: when `*` is seen after `%`, pop the
next arg as `int64_t` and use it as `fieldWidth` (or `precision` if `%.*`).

### 0.7 - `save_object`/`restore_object` in FluffOS `.o` text format

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

### 0.9 - `map`/`filter`/`sort_array` as real closure consumers

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

### 0.11 - PCRE regexp efuns

Wrap PCRE2 (link `-lpcre2-8`):
- `regexp(string, pattern)` → 1 if matches, 0 if not
- `regexplode(string, pattern)` → array of alternating non-match/match substrings
- `reg_assoc(string, patterns_array, tokens_array, default)` → tokenize string
- `regexp_assoc` - alias

Add `find_package(PkgConfig)` + `pkg_check_modules(PCRE2 REQUIRED libpcre2-8)`
to `driver/CMakeLists.txt` and link `efun` against it.

### 0.13 - Grow to ~300 efuns (FluffOS parity)

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
covered by `test/test_lexer.cpp` (see STATUS.md's own dated entries for
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
driver itself* (`reference/fluffos-2.9-ds2.08/`,
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

**Corpus note (2026-08-22, this pass):** `mudlib/nightmare3_fluffos_v2/lib/`,
the directory every pass above cites, does not exist in this repo or on
this machine -- the LDMud-style restructure (`7a4121c`) never vendored a
gameplay mudlib at all, `mudlib/` here has only ever been the bundled Lil
driver-tooling mudlib. Re-cloned the real corpus fresh from
`github.com/fluffos/nightmare3` into `temp/nightmare3` (gitignored,
matching the `temp/dgd`/`temp/fluffos`/`temp/ldmud` research-clone
precedent) and confirmed it is the exact same version prior passes used
(every count in the table above reproduces exactly against it, and the
specific files those rows cite by path all exist in it). Whoever runs
the next pass needs this clone present again first -- it is not tracked.

**Tier 1, third pass (2026-08-22), re-scoped to the reconstructed
`temp/nightmare3/lib/` corpus:** 51 gap efuns remained against
`efun_defs.c`'s 270 real names (222 registered as of this pass). Top of
the re-ranked list, `pluralize` (13 calls, 6 files), is a real efun with
a large but fully self-contained implementation (`packages/contrib.c`,
~440 lines, no missing infrastructure) -- but this exact mudlib's own
`secure/SimulEfun/english.c` defines a complete, independently-written
`pluralize()` simul_efun with no `efun::pluralize()` delegation
anywhere in it, so every one of those 13 call sites (all bare
`pluralize(...)` calls) resolves to the simul_efun, not the core efun.
Same permanently-unreachable-core-registration category as
`translate`/`event`/`tell_object`/bare `livings()` above, not a "someday,
once there's time" deferral the way a previous pass's "similar scope to
`query_num`" framing implied -- corrected here. **This verdict itself
was reversed one pass later, see the fourth-pass section below**: it was
only ever true against nightmare3 specifically, and a wider corpus
turned up a real `efun::pluralize()` delegation elsewhere, the same
"load-bearing shadow" exception bare `livings()` already gets a few
lines up. `get_char` (8/1) and
`ed` (4/3) remain excluded for their previously-documented reason (new
raw single-character-delivery / stateful interactive-editor
infrastructure `Server::dispatchLine()`'s line-buffered design has no
path for). `origin` (2/1), `resolve` (1/1), and the driver-internal-dump
family (`mud_status`, `cache_stats`, `malloc_status`,
`dump_file_descriptors`, `dumpallobj`, `domain_stats`, `author_stats`,
all 1/1) remain excluded for their own previously-documented reasons,
re-verified accurate.

With no call-site-bearing gap efun implementable this pass, picked from
the 0-call-site remainder instead, on "real, self-contained, completes
an already-real subsystem" rather than frequency -- the same bar
`shallow_inherit_list`/`inherit_list` were included under above.
Implemented: `named_livings` (real, walks the same set-living-name table
`find_living()`/`find_player()` already use -- `LivingNameRegistry`,
which needed exactly the enumeration capability this doc's own earlier
note said it lacked; added `LivingNameRegistry::allWithCommandsEnabled()`),
`query_notify_fail` (real, a non-consuming peek at `notify_fail()`'s
pending message, distinct from `notify_no_command()`'s own one-shot
take; added `Connection::peekPendingNotifyFail()`), and
`request_term_size` (real, a bare IAC DO NAWS -- this driver's NAWS
*receiving* side, `handleSubnegotiation()`/`query_screen_width()`/
`query_screen_height()`, was already complete from row 0.8/an earlier
0.13 pass; this was the missing proactive-request half; added
`Connection::requestWindowSize()`). `request_term_type`/
`start_request_term_type`/`act_mxp` considered and rejected alongside
these: each would fire a real raw negotiation request this driver has
zero downstream parsing for at all (no TTYPE/MXP subnegotiation handler,
no stored field, no apply call) -- sending a request whose answer is
silently dropped is a half-feature, not a faithful port, same reasoning
`resolve()`'s own synchronous-stand-in rejection above already
establishes. `unique_mapping`, `variables`/`functions`/`fetch_variable`/
`store_variable`, and the `rotate_x`/`rotate_y`/`rotate_z`/`scale`/
`lookat_rotate`/`id_matrix` VRML-pose family were all surveyed and left
for a future pass: the first needs a nontrivial new hash-bucket grouping
implementation with zero real call sites to validate against; the
second is a real reflection API family of comparable scope to a fresh
mini-subsystem; the third has zero real call sites anywhere in this
corpus and no plausible use in a text-only MUD driver.

**Corpus note (2026-08-22, fourth pass):** five more real, independently-
maintained mudlibs are now available alongside `temp/nightmare3`:
`temp/mudlib` (Genesis/CD, targets the CD gamedriver, not FluffOS),
`temp/core-lib` (RealmsMUD, targets LDMud, not FluffOS), `temp/es2_mudlib`
(ES2, targets Neolith -- documented as backward-compatible with
MudOS-level LPC, the same lineage FluffOS itself descends from),
`temp/lima` and `temp/dead-souls` (both explicitly FluffOS-targeted).
The two non-MudOS-lineage ones (Genesis/CD, RealmsMUD) are weaker
signal -- a shared efun name there is not guaranteed to mean the same
thing semantically -- so treat hits concentrated only in those two with
extra caution and lean harder on this row's own standing "verify
against reference source" step before trusting them. All gitignored the
same way, none tracked; re-clone before running the next pass.

**Tier 1, fourth pass (2026-08-22), ranked across all six corpora
combined:** `pluralize` (65: nightmare3 13, lima 19, dead-souls 33)
topped the combined ranking, same as before -- but re-checking the
shadow question against the wider corpus reversed the third pass's own
verdict just above: lima's own `std/modules/m_grammar.c` wraps a small
set of hardcoded exceptions around a direct `return
efun::pluralize(str);` fallthrough for everything else, the exact
load-bearing-shadow pattern bare `livings()` already established,
simply never visible from nightmare3 alone. `translate` (56) stays
excluded, re-confirmed the same way across all three shadowing corpora
(nightmare3/lima/dead-souls all define their own, and none delegates to
`efun::translate()` anywhere). Implemented: `pluralize`, ported
mechanically from the real ~440-line `packages/contrib.c` body (see
STATUS.md's own dated entry for the full derivation and one confirmed
real quirk worth knowing about -- `pluralize("lotus")` produces
`"lotuss"` in the real reference build itself, ported faithfully, not a
typo).

`replace_program` (33, es2 31) and `origin` (27, up from nightmare3's
own 2) were investigated in real depth, not just re-flagged. Both are
strong, concretely-scoped candidates for a dedicated future session --
see STATUS.md's own entry for the full architecture notes (this
driver's `CompiledProgram::inheritedPrograms`/`ancestorBaseOffsets`
turned out to be a much closer match for `replace_program()` than
expected, needing "only" a filename-keyed inherit search plus a
deferred per-tick replace queue, not a ground-up new subsystem; `origin`
still needs per-call origin tagging through the whole VM call-stack,
and is used as a real security gate in `secure/daemon/chat.c`, so a
rushed partial implementation was rejected as worse than none). Neither
implemented this pass. `debug_info` (6: lima 4, dead-souls 2) re-checked
given real cross-corpus demand -- both wrap `efun::debug_info()`, the
same load-bearing-shadow shape `pluralize` turned out to have, but its
real C implementation genuinely does not exist anywhere in this
project's only reference source (prototype only, no body, in
`packages/contrib.c` or anywhere else) -- stays excluded, now for an
unverifiable-spec reason, not the previous architecture-mismatch
framing.

Also implemented: `unique_mapping` (real, `mapping.c`'s
`f_unique_mapping()`, real call sites confirmed in
`dead-souls/lib/verbs/items/{get,wield,unwield}.c`) and `reclaim_objects`
(real, `reclaim.c`'s `reclaim_objects()` -- see STATUS.md's own entry
for why this driver's existing lazy `coerceIfDestructed()` mechanism
does not make this redundant: it never coerces a mapping's own *key*
half, only its value half, a real gap only this efun's eager sweep
closes). `get_char`/`ed`/`origin`/`resolve`/the driver-internal-dump
family all re-verified excluded for their previously-documented reasons
with the wider corpus behind them too.

**Tier 1, fifth pass (2026-08-22):** `replace_program` (33, es2 31) and
`origin` (27) -- the two items the fourth pass flagged as strong,
architecturally-scoped-but-not-yet-implemented candidates -- were both
investigated in real depth this pass. `replace_program` was ready and
implemented: this driver's `CompiledProgram::inheritedPrograms`/
`ancestorBaseOffsets` (`Bytecode.hpp`) turned out to need no new field
at all, just a name-matched depth-first walk of the existing parallel
`inherits[i]`/`inheritedPrograms[i]` vectors (new `searchInheritedProgram()`,
`EfunTable.cpp`) plus one `ancestorBaseOffsets` lookup for the real
`var_offset`. New `VM::enqueueReplaceProgram()`/
`processPendingReplacePrograms()` (deferred application, wired into
`Scheduler::run()`'s own loop at the same relative position real
`backend.c`'s own `while(1)` loop calls `remove_destructed_objects()`
from) and new `VM::simulEfunObject()`/`LpcObject::setProgram()`/
`programPtr()` accessors. See STATUS.md's own dated entry for the full
derivation, including the one real guard intentionally not ported (real
`prog->func_ref`, which has no equivalent given this driver's lazy-
resolved `Closure` model) and the one real asymmetry ported exactly as
read (the shadow-splice-on-replace only checks `ob->shadowing`, never
`ob->shadowed`). `query_replaced_program` (real, `packages/contrib.c`)
implemented alongside it as a natural, small follow-on -- new
`LpcObject::replacedProgramName_`, set only once a swap actually
applies, cleared on destruct.

`origin` was investigated just as deeply and still not implemented,
now for a sharper, more specific reason than "needs per-call origin
tagging" alone: real `f_origin()` (`efuns_main.c`) is genuinely simple,
just one scalar (`caller_type`) saved/restored across nested calls the
same way this driver's own `objectChangeStack_` already saves/restores
object-crossing state -- but real `call_other()`/`->` does not compile
to its own opcode in this driver at all, it is compiler-forced through
`OpCode::CallEfun` targeting the literal name `"call_other"`
(`Bytecode.hpp`'s own comment), the exact same opcode every genuine efun
call also uses. Distinguishing real `ORIGIN_CALL_OTHER` from real
`ORIGIN_EFUN` therefore needs a name-based special case at that one
opcode on top of otherwise-straightforward per-opcode tagging elsewhere
(`Call`'s own tiered resolution, `CallParent`, `callClosure()`, and
every external `callFunction()` entry point for `ORIGIN_DRIVER`) -- a
real, easy-to-get-subtly-wrong seam invisible from a first architecture
read, all 8 real `ORIGIN_*` values and their real C set sites now
enumerated (see STATUS.md) for whoever takes this on next as its own
fully-focused pass, not shared with other batch work.

Two more items resolved into sharper categories while investigating the
above. The reflection-efun family (`variables`, `functions`,
`fetch_variable`, `store_variable`, `fetch_class_member`,
`store_class_member`) -- previously "a real API family of comparable
scope to a fresh mini-subsystem" -- has **no implementation anywhere in
this project's only reference source** for any of the six names
(confirmed by grepping every real `.c` file, `packages/` included, not
just the top-level ones an earlier pass's narrower grep covered):
prototype-only, the same `debug_info` category. Recategorized from
"large effort" to "unverifiable, nothing to check a port against." The
`parse_*` family (66 combined across `parse_refresh`/`parse_init`/
`parse_sentence`/`parse_add_rule`/`parse_add_synonym`/`parse_my_rules`/
`parse_dump`/`parse_remove`) does have a real implementation after all,
missed the same way by an earlier top-level-only grep:
`packages/parser.c`, a genuine 3419-line natural-language sentence/
grammar-rule parser package. Real and substantial, not unverifiable or
architecture-mismatched -- but sized closer to `replace_program`'s own
"own dedicated session" category (likely larger) than anything to fold
into a batch. Flagged with this citation so it is not rediscovered from
scratch next time.

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
package - see Phase 2.

## Phase 2 tasks

### 2.15 - SQLite built-in database efuns

Link against SQLite3 (`find_package(SQLite3 REQUIRED)`).

New efuns:
- `db_connect(string path)` → int handle
- `db_exec(int handle, string sql)` → int rows_affected
- `db_fetch(int handle, string sql)` → array of mapping rows
- `db_close(int handle)`
- `db_error(int handle)` → string last error

Maintain an internal `vector<sqlite3*>` indexed by handle. Handles are
per-connection, not global (thread-safety not needed in single-threaded model).

### 2.16 - Hash efuns

Link against OpenSSL (`find_package(OpenSSL REQUIRED)`):
- `hash(string algorithm, string data)` → hex string
  Algorithms: `"sha256"`, `"sha512"`, `"md5"`, `"blake2b"`
- `bcrypt_hash(string password, int cost)` → string
- `bcrypt_verify(string password, string hash)` → int

### 2.17 - `json_encode`/`json_decode`

Embed `nlohmann/json.hpp` (single-header, no extra deps):
- `json_encode(mixed value)` → string (JSON text)
- `json_decode(string json)` → mixed (LPC Value)

Mappings become JSON objects; arrays become JSON arrays; strings/ints/floats
map directly; `nil` (DGD) / `0` (FluffOS) maps to JSON null.

### 2.18 - `http_get`/`http_post` async efuns

Backed by the async scheduler (Phase 2.5/2.6). Use libcurl async callbacks:
- `http_get(string url)` → awaitable: suspends until response
- `http_post(string url, string body, string content_type)` → awaitable

### 2.22 - LPC-native test runner

New efuns for use inside `.c` test files:
- `assert_equal(mixed a, mixed b)` - throws if not equal
- `assert_not_equal(mixed a, mixed b)`
- `assert_throws(function fn)` - catches and returns the error string
- `test_pass(string name)` / `test_fail(string name, string reason)`
- `run_tests(object test_ob)` - run all `test_*` functions in the object

These efuns write structured results to a JSON file for CI integration.

## Testing

Every efun must have at least one regression test in `test/` before Phase 1
begins. Prefer `test/test_efun_<name>.cpp` for new efun tests.

```bash
ctest --test-dir build -R "efun" --output-on-failure
```

## Key invariants

- Efun signatures are `Value(VM&, vector<Value>&)`. Never change this.
- Check arg count and types at the start of every efun and throw
  `LpcRuntimeError` with a clear message on mismatch.
- Register aliases (`new`/`clone_object`, `evaluate`/`funcall`) using the same
  lambda so the implementation is never duplicated.
- An efun that calls back into LPC (e.g. `map`, `filter`, `sort_array`) must
  catch `LpcRuntimeError` from the callback and re-throw with added context.
