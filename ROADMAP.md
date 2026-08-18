# AMLP - World-Class LPC Driver Roadmap

Goal: transform AMLP from a single-mudlib FluffOS-targeting driver into
the best LPC runtime available - meeting or exceeding FluffOS, LDMud, and DGD
on their own terms and surpassing all three on the dimensions none of them
addressed.

Each source directory carries an `instruct.md` that owns the detailed task
list for that subsystem. This file is the master sequencing reference.

---

## Phase 0 - Stabilize the current base

**Prerequisite for everything else. Do not start Phase 1 until Phase 0 is
complete and the full test suite is passing with no regressions.**

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 0.1 | `throw()` efun - carry a Value to the nearest `catch()` | `src/efun` | [x] |
| 0.2 | `sscanf` full format set: `%f`, `%x`, `%(regexp)`, adjacent `%s` | `src/efun` | [x] (partial: `%(regexp)` still pending; 0.11's PCRE2 wrapper it depended on is now done) |
| 0.3 | `sprintf` `%*` dynamic field width | `src/efun` | [x] |
| 0.4 | `set_eval_limit` as a real accumulated-cost model (not no-op) | `src/vm` | [x] |
| 0.5 | Full `O_DESTRUCTED` apply guards on every cross-object call | `src/object` | [x] |
| 0.6 | Shadow support: `shadow(ob, flag)` efun + shadow chain traversal | `src/object` | [x] |
| 0.7 | `save_object`/`restore_object` in FluffOS `.o` text format | `src/efun` | [x] (partial: restore-side only; `save_object` still writes this driver's own custom format) |
| 0.8 | Full telnet IAC negotiation, echo suppression, NAWS | `src/net` | [x] |
| 0.9 | `map`/`filter`/`sort_array` as real closure consumers | `src/efun` | [x] (partial: all mudlib shapes covered) |
| 0.10 | `socket_*` family basics (create/connect/write/read/close) | `src/net` + `src/efun` | [x] (partial: STREAM/DATAGRAM only, no MUD mode or binary modes; no `socket_read` efun -- real FluffOS has none, reads are callback-only) |
| 0.11 | `regexp`/`regexplode`/`reg_assoc` PCRE efuns | `src/efun` | [x] |
| 0.12 | Every efun has at least one regression test | `tests` | [x] (audited 2026-08-20: all 167 then-registered efuns confirmed covered; a moving target as 0.13 grows the table, each new efun needs its own test at the time it's added) |
| 0.13 | Grow efun table to FluffOS parity (~300 efuns) | `src/efun` | [x] (non-`parse_*` scope closed 2026-08-24: 240 registered against this exact vendored reference build's own 270 real `efun_defs.c` names. Final accounting of the real gap (`comm -23` reconciliation, not the raw registered-count difference -- see STATUS.md's own reconciliation): 40 non-`parse_*` names, all confirmed genuine documented exclusions (architecture mismatch or zero-call-site deferral, individually re-verified 2026-08-24), with no further batch work remaining against that portion, plus the separately-tracked 8-name `parse_*` package. `origin()` and `reload_object()` both implemented 2026-08-23; `functions`/`variables`/`fetch_variable`/`store_variable` implemented 2026-08-24, correcting an earlier "no implementation exists" miscall for all four (real bodies in `packages/contrib.c`); `socket_release`/`socket_acquire` implemented 2026-08-24 (continued), correcting an earlier "Tier 3, out of basics scope" miscall. The `parse_*` package is carved out as its own row, 0.13a) |
| 0.13a | `parse_*` natural-language sentence/grammar-rule parser package (`parse_init`, `parse_refresh`, `parse_sentence`, `parse_add_rule`, `parse_add_synonym`, `parse_my_rules`, `parse_dump`, `parse_remove`) | `src/efun` | [ ] (not started, deliberately deferred. Real implementation source: `packages/parser.c`, 3419 lines -- FluffOS's real "parser" package, confirmed substantial and genuinely implemented in the reference build, not unverifiable or architecture-mismatched. 66 combined call-site hits across the 8 names in the vendored mudlib corpora. Sized well beyond a normal batch item -- comparable to or larger than everything else row 0.13 has implemented combined -- so it warrants its own explicit go-ahead before being taken on, not an assumed default. See STATUS.md's 2026-08-23/2026-08-24 entries for the scoping history.) |
| 0.14 | `global include file` config support (auto-`#include` prepended to every compiled object) | `src/config` + `src/object` | [x] |
| 0.15 | `ObjectManager::compile()`'s `programCache_` has no invalidation path -- a recompiled file's own new source is silently ignored | `src/object` | [ ] |

**0.15 scope note:** found live (2026-08-21) while confirming `mudlib`'s
own `eval` command works end to end. `ObjectManager::compile(filename)`
checks `programCache_` first and returns the cached `CompiledProgram` for
any filename it has ever successfully compiled before, unconditionally,
with no path anywhere that invalidates or replaces that entry -- confirmed
directly by reading the function, not inferred from the symptom alone.
Real, reproducible failure mode: `mudlib/command/eval.c` writes a new
`/tmp_eval_file.c`, destructs the previous `/tmp_eval_file` instance, then
calls `"/tmp_eval_file"->eval()` again -- the destruct correctly clears
the *instance*, but the *compiled program* for that filename is still
sitting in `programCache_` from the first call, so every subsequent `eval`
silently re-runs the first call's own stale bytecode against the new
instance instead of the newly-written source. Confirmed live: three
different real `eval` expressions in one session (`5+5`, an array
literal, `this_player()`) all returned the first call's own cached result.
Not fixed here, filed as its own row instead: a fix would likely mean a
narrow, explicit cache-invalidation or force-recompile path (e.g. an
`ObjectManager::recompile(filename)` that erases the matching
`programCache_` entry before calling `compile()` again, triggered by
`destruct()`-then-recompile on the same filename, or a dedicated
`update_object()`-style efun/apply real FluffOS itself uses for this same
purpose) -- scoped narrowly enough that the caching behavior every other
compiled object correctly relies on (each file compiled once, reused by
every later `inherit`/`clone_object()`/`call_other()` reaching that same
filename) is not disturbed for the overwhelming majority of call sites
that never rewrite their own source out from under a running driver.

---

## Phase 1 - Dialect universality

**Goal: one binary, three dialects - FluffOS/MudOS, LDMud, DGD.**

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 1.1 | `LpcDialect` enum + config key `dialect` | `src/config` + `src/dialect` | [x] (the enum -- `LpcDialect`, `dialectName()`/`dialectFromString()` -- implemented 2026-08-24 in `src/dialect/LpcDialect.hpp`/`.cpp`; the `dialect` config key implemented 2026-08-24 (continued) in `Config::dialect()` (`src/config/Config.hpp`/`.cpp`, plain `"fluffos"`/`"ldmud"`/`"dgd"` string, kept out of the `LpcDialect` enum itself to avoid a `config`<->`dialect` library cycle since `src/dialect` already depends on `src/config`; parsed via `dialectFromString()` at the one real call site that needs the enum, `src/dialect/DialectSelect.cpp`). Defaults to `"fluffos"`, matching this driver's prior behavior exactly for every config file that never sets the key. Actually consumed by exactly one call site so far -- `main.cpp`'s `masterUidApply()` boot query, see row 1.4 -- every other applies still hardcodes FluffOS or is unimplemented) |
| 1.2 | Dialect-aware Lexer: `#'`, `lambda`, `atomic`, `rlimits`, `nil` tokens | `src/compiler` + `src/dialect` | [ ] |
| 1.3 | Dialect-aware Parser: LDMud symbol/lambda syntax, DGD `atomic`/`rlimits` | `src/compiler` + `src/dialect` | [ ] |
| 1.4 | Pluggable boot API: FluffOS master/simul_efun, LDMud master, DGD driver+auto | `src/apply` + `src/dialect` | [ ] (partial: `BootApi` abstract base plus `FluffOsBootApi`/`LdmudBootApi` implemented 2026-08-24 in `src/dialect`, deliberately trimmed to omit `connectApply()`/`netDeadApply()` and `DgdBootApi` per the still-open design note on this row -- see `src/dialect/instruct.md`. `masterUidApply()` alone is now wired for real: `main.cpp` calls the new `queryMasterUid(vm, bootApi)` helper (`src/dialect/MasterUidBoot.cpp`) right after master loads, routed through `VM::applyMaster()` against the real master object, 2026-08-24 (continued). `main.cpp`'s `BootApi` is now genuinely config-driven, 2026-08-24 (continued further) -- `makeBootApiForConfig()` (`src/dialect/DialectSelect.cpp`) selects `FluffOsBootApi`/`LdmudBootApi` from `Config::dialect()` (row 1.1), defaulting to FluffOS for any config that never sets the key; DGD throws `NotImplementedError` rather than silently picking a wrong dialect. Deliberately a small free function, not a `DialectFactory` class -- see `DialectSelect.hpp`'s own comment on why a factory is premature with only one real consumer. No `DialectFactory`, and every other `ApplyTable`/`VM::applyMaster()` call site (`Server.cpp`'s `"connect"`, `ObjectManager.cpp`'s `"compile_object"`/`"privs_file"`) still hardcodes its apply name string directly rather than routing through `BootApi` -- only the one UID call site was in scope) |
| 1.5 | LDMud shadows: `shadow(ob)`, `unshadow()`, `query_allow_shadow` | `src/efun` | [x] (rescoped 2026-08-17: the row's own original title carried the same signature error `src/dialect/instruct.md`/`src/apply/instruct.md` had already flagged elsewhere -- `shadow(ob,1)` is FluffOS's shape, not LDMud's. Re-read the real sources directly: `temp/ldmud/src/func_spec` ("int shadow(object) no_lightweight;", one argument) + `temp/ldmud/src/simulate.c`'s own `f_shadow()`/`validate_shadowing()`/`f_unshadow()` (LDMud 3.6.8, this project's vendored clone). Confirmed divergences from the already-implemented FluffOS `shadow(ob, flag)` (row 0.6): one argument only (no query flag -- LDMud's own separate query mechanism, `query_shadowing()`, is **obsolete** in this exact 3.6.8 clone, relocated to `temp/ldmud/doc/obsolete/query_shadowing`, so the row's own "`query_shadowing`" target does not apply to a current LDMud and is correctly dropped, not implemented, for the dialect); returns int 1/0, never the shadowed object; master apply is `query_allow_shadow()`, not `valid_shadow()`; and -- a genuine driver-level asymmetry confirmed by reading both `validate_shadowing()`s side by side -- LDMud's has no "cannot shadow the master object" guard at all (FluffOS's does, already ported under 0.6), master protection under LDMud being purely an advisory mudlib convention inside `query_allow_shadow()`'s own body, not a driver mechanism. Also confirmed real LDMud has a second, genuinely FluffOS-absent efun, `void unshadow(void)` (`temp/ldmud/src/simulate.c`'s `f_unshadow()`; grepped the full vendored `fluffos-2.9-ds2.08` tree for "unshadow", zero hits) -- it only splices current_object out when current_object is itself shadowing something (reconnecting that victim to whoever was shadowing current_object), a genuine no-op when current_object is merely being shadowed by someone else with no victim of its own, confirmed from the C directly rather than the doc's looser prose. The row's third original item, "shadow-chain `call_other`", turned out to already be done and dialect-agnostic -- `VM::callFunction()`'s existing shadow-chain walk (implemented under row 0.6, see `src/object/instruct.md`) matches both dialects' documented call_other-redirection model (`doc/efun/shadow`'s own "if A shadows B... not even object B can call_other() itself" is already exercised by the existing `!= current_object` re-entry guard), so nothing further needed there. What was actually small and unambiguous: the `shadow()`/`unshadow()` efun-level signature and semantics work, implemented 2026-08-17 in `src/efun/EfunTable.cpp`, gated on `vm.config().dialect() == "ldmud"` exactly like row 1.6's `replace_program()` branch, with 4 new regression tests. Directory column corrected from `src/object` to `src/efun`, the only place any of this actually lives) |
| 1.6 | LDMud `replace_program()` no-argument, sole-inherit auto-select | `src/efun` | [x] (rescoped 2026-08-17: the row's original premise -- a `replaces` directive in `inherit` -- **does not exist anywhere in real LDMud**. Re-grepped `temp/ldmud/src/prolang.y`'s own `inheritance_qualifier`/`inheritance_modifier` productions: the complete inherit-modifier set is `static`/`private`/`public`/`protected`/`nosave`/`nomask`/`deprecated`/`virtual`/`visible`, no `replaces` token anywhere in the grammar (`temp/ldmud/src` grep for the bare word `replaces` also turns up nothing but unrelated English prose in comments -- `array.c`, `ed.c`, `lex.c`, `object.c`, `efuns.c`, `swap.c`). The real, genuine per-dialect divergence lives entirely inside `replace_program()` itself, confirmed by reading both real implementations in full: `temp/ldmud/doc/efun/replace_program` + `temp/ldmud/src/object.c`'s `v_replace_program()` (LDMud 3.2.9+) accept `void replace_program()` with **no** argument, auto-selecting the object's sole inherited program when it has exactly one, throwing "requires argument for object with more than one inherit" when it has more, and "called with no inherited program" when it has none; real FluffOS's `temp/reference/fluffos-2.9-ds2.08/replace_program.c` own `f_replace_program()` has no such form at all -- it unconditionally rejects a missing/non-string arg via `bad_arg(1, ...)` before anything else runs, mandatory argument always. `replace_program(string)` itself was already implemented FluffOS-scoped under row 0.13 (`src/efun/EfunTable.cpp`, `f_replace_program`-equivalent); this row's actual, narrower scope is the LDMud-only zero-arg form. Small and unambiguous once rescoped -- implemented 2026-08-17 in the same `replace_program` efun registration, gated on `vm.config().dialect() == "ldmud"` (row 1.1's `Config::dialect()`, already reachable from `src/efun` with no new library dependency), with 3 new regression tests: sole-inherit auto-select applies and swaps correctly, more-than-one-inherit throws, and the fluffos-dialect zero-arg call is confirmed still rejected exactly as before this fix, unchanged. Directory column corrected from `src/compiler` + `src/object` to `src/efun`, the only place any of this actually lives) |
| 1.7 | LDMud `lambda()` / `unbound_lambda()` / `bind_lambda()` closure kinds | `src/vm` + `src/compiler` | [ ] (name corrected 2026-08-17: `bind()` does not exist as an LDMud efun -- the real name is `bind_lambda(closure cl [, object ob])`, confirmed against `temp/ldmud/doc/efun/bind_lambda` and `temp/ldmud/src/closure.c`'s own `v_bind_lambda()` (3.6.8). Investigated in real depth this pass, alongside row 1.5's shadow work, per the same "shadow()/bind_lambda() divergence" note in `src/dialect/instruct.md`/`src/apply/instruct.md` -- **not implemented, scope confirmed genuinely bigger than a normal batch item, same category as the still-open `parse_*` and connect/disconnect design questions.** Real `v_bind_lambda()` switches on the closure's own kind (`sp->x.closure_type`): `CLOSURE_LAMBDA`/`CLOSURE_IDENTIFIER` are unconditionally unbindable; `CLOSURE_LFUN` rebinds in place; `CLOSURE_BOUND_LAMBDA` either rebinds in place or, if the closure is shared (`ref > 1`), copy-on-write clones it first; `CLOSURE_UNBOUND_LAMBDA` allocates a new bound-lambda wrapper; the default branch (efun/simul-efun/operator closures) rebinds directly. This driver's own `Closure` (`include/amlp/vm/Value.hpp`) has none of these kinds -- one flat struct with a single `owner` field, no `CLOSURE_LAMBDA`/`UNBOUND_LAMBDA`/`BOUND_LAMBDA` distinction and no reference-counted sharing to copy-on-write around -- because `lambda()`/`unbound_lambda()`/`#'symbol` (this same row's other two items) are themselves entirely unimplemented; there is nothing yet for a real `bind_lambda()` to bind. Real `bind_lambda()` also gates a non-`this_object()` target through `privilege_violation("bind_lambda", this_object(), ob)`, a whole master-apply subsystem (`doc/concepts/privilege`) this driver has no equivalent of at all -- a different gate family from the UID-based FluffOS applies already implemented. A narrowed "just rebind this driver's existing FluffOS-style `(: name :)` closures' own `owner` field, skip the privilege check, skip the lambda-specific branches" version was considered and rejected as a mismatch: it would answer to the name `bind_lambda()` while implementing none of the closure-kind matrix that makes the real efun what it is, and the missing privilege check is a real permissiveness gap, not a harmless simplification like the shadow work's already-documented `nomask` skip. Options for whoever picks this up: (a) treat this as its own explicitly-scoped go-ahead item the way `parse_*` (row 0.13a) was carved out, sized to real closure-kind work (`lambda()`/`unbound_lambda()` literal support in `src/compiler`+`src/vm` first, `bind_lambda()` as a natural follow-on once real bound/unbound lambda values exist to rebind); (b) implement a minimal, explicitly-labeled `bind_lambda()` now that only covers this driver's existing FluffOS-style closures (owner-reassignment, dialect-gated, no privilege check), clearly documented as a partial stand-in until (a) lands; (c) leave it deferred exactly as before, now with the real scope on record instead of a guess. **Decision (2026-08-17): (c).** No partial stand-in. `lambda()`/`unbound_lambda()` themselves are entirely unimplemented prerequisites -- there is no real lambda value yet for any `bind_lambda()`, minimal or otherwise, to rebind, so a stand-in would only be exercising this driver's pre-existing FluffOS-style closures under a name that means something structurally different in real LDMud. Stays deferred alongside `parse_*` (row 0.13a) and the connect/disconnect design question (row 1.4/1.16) until row 1.7's own prerequisite closure-kind work gets an explicit go-ahead) |
| 1.8 | LDMud `#'symbol` references baked at construction | `src/compiler` + `src/vm` | [ ] |
| 1.9 | LDMud mapping width > 1: `m_allocate`, `m_indices`, `m_values` | `src/vm` + `src/efun` | [ ] |
| 1.10 | DGD `nil` as distinct type in `Value` variant | `src/vm` | [ ] |
| 1.11 | DGD `rlimits` statement: per-task tick + stack depth limits | `src/vm` + `src/compiler` | [ ] |
| 1.12 | DGD `atomic` function modifier: VM-level checkpoint/rollback | `src/vm` | [ ] |
| 1.13 | DGD `parse_string` kfun | `src/efun` | [ ] |
| 1.14 | DGD lightweight objects (LWOs): value-semantics object kind | `src/vm` + `src/object` | [ ] |
| 1.15 | DGD driver+auto object boot path | `src/apply` + `src/dialect` | [ ] |
| 1.16 | LDMud master apply name table | `src/apply` + `src/dialect` + `src/efun` | [ ] (partial: `get_master_uid` implemented and tested 2026-08-24 via `LdmudBootApi::masterUidApply()`/`queryMasterUid()` (row 1.4). `query_allow_shadow` implemented 2026-08-17 (row 1.5, `src/efun/EfunTable.cpp`'s `shadow()` LDMud branch). `valid_snoop` implemented 2026-08-17 this same pass, alongside a full re-scope of `snoop()` itself -- `temp/ldmud/src/comm.c`'s own `set_snoop()` ("The function calls master->valid_snoop() to test if the snoop is allowed") calls it for both the start and stop forms, confirmed by reading the C directly rather than `doc/efun/snoop`'s own stale prose, which also wrongly claims an object return (`func_spec`'s own `"int snoop(object, void|object);"` and `v_snoop()`'s own `put_number(sp, i)` are authoritative: plain int, 1/0/-1, never the object). Gated on `vm.config().dialect() == "ldmud"` in the same `snoop` registration, 5 new regression tests. `valid_query_snoop` intentionally not implemented alongside it: the LPC-visible efun it used to gate, `query_snoop()`, is itself obsolete in this exact 3.6.8 clone (`temp/ldmud/doc/obsolete/query_snoop`) -- its real replacement is `interactive_info(ob, II_SNOOP_*)` (confirmed live call sites in `comm.c`'s `f_interactive_info()`), a materially different, much larger efun this driver has no equivalent of at all; implementing `valid_query_snoop` with nothing that calls it would be dead code. **2026-08-18: all three of the row's remaining items investigated against the real 3.6.8 driver source in full, none turned out small -- each has its own blocking issue, recorded below rather than forced.**
- `get_bb_uid`: **dead in this exact vendored build, not implementable against anything real.** `doc/master/get_bb_uid` claims it is called by `process_string()` when there is no current object, but `temp/ldmud/src/efuns.c`'s own `f_process_string()`/`process_value()` -- read in full -- make no euid/`get_bb_uid` call of any kind. Grepped the generated constant `STR_GET_BB_UID` (from `string_spec`'s own `"GET_BB_UID \"get_bb_uid\""` entry) across every `.c`/`.h` in `temp/ldmud/src`: zero call sites anywhere in the driver. The only two places the name appears at all are `string_spec` (the string-table declaration) and `applied_spec` (a compile-time argument/return-type spec for the compiler to type-check a master.c that happens to define the lfun -- confirmed by `applied_spec`'s own header comment, not itself a call-site generator). This is a real doc-vs-code divergence in LDMud itself, the same category already found for `unshadow()`/`snoop()` in rows 1.5/1.16 -- the C is authoritative, the doc is stale. Implementing a `get_bb_uid` accessor would be scaffolding with no real driver-side consumer to model faithfully, not a divergence to port.
- `make_path_absolute`: **blocked on a missing prerequisite.** Grepped `STR_ABS_PATH` (`string_spec`'s `"ABS_PATH \"make_path_absolute\""`) across the whole driver: exactly one real call site, `temp/ldmud/src/ed.c:1128`, inside the built-in line editor's own relative-filename resolution (`ed`'s file-open command, when the typed filename does not start with `/`). This driver has no `ed()` efun at all -- confirmed zero `"ed"` registrations in `EfunTable.cpp`, and already on record in `src/efun/instruct.md` as excluded ("`get_char`/`ed`/`origin`/`resolve`/the driver-internal-dump family", previously-documented architecture-scope reasons). `make_path_absolute()` has no other real caller to model against until `ed()` itself exists -- same shape as `bind_lambda()` needing `lambda()`/`unbound_lambda()` first.
- `valid_read`/`valid_write`: **genuinely bigger than a normal batch item, not LDMud-specific at all.** Read `temp/ldmud/src/simulate.c`'s own `check_valid_path()` in full: one shared function backing both applies (`apply_master(STR_VALID_WRITE/READ, 4)`, args `path, uid-or-0, func, ob` in that push order, confirmed matching `doc/master/valid_read`/`valid_write`'s own SYNOPSIS exactly), called from every file-touching efun -- `doc/master/valid_read`/`valid_write`'s own lists name `copy_file`, `ed_start`, `file_size`, `get_dir`, `print_file`/`cat`, `read_bytes`, `read_file`, `restore_object`, `tail` (read) and `copy_file`, `rename_from`/`rename_to`, `ed_start`, `garbage_collection`, `mkdir`, `memdump`, `objdump`, `opcdump`, `remove_file`/`rm`, `rmdir`, `save_object`, `write_bytes`, `write_file` (write). Checked whether this could be a small LDMud-only branch on an existing FluffOS mechanism the way `shadow()`/`snoop()`/`replace_program()` were: it cannot -- FluffOS has the identical real applies too (`temp/reference/fluffos-2.9-ds2.08/applies.h`'s own `APPLY_VALID_READ`(33)/`APPLY_VALID_WRITE`(38)), and this driver currently gates **zero** of its 11 already-implemented file efuns (`read_file`, `write_file`, `read_bytes`, `write_bytes`, `save_object`, `restore_object`, `rm`, `mkdir`, `rmdir`, `rename`, `get_dir`, confirmed by grep) with either dialect's version. This is a whole missing cross-cutting security feature for both dialects at once -- a new shared path-check helper plus wiring into 11+ call sites, each needing its own literal `func` string -- not a single-efun signature divergence. Same category as `parse_*`: sized well beyond a normal batch item, needs its own explicit go-ahead as a dedicated row rather than folding into 1.16's remaining-items list.

No implementation this pass: all three blocked, none forced. `get_bb_uid` stays recorded as dead/unimplementable against this exact vendored source; `make_path_absolute` stays deferred behind `ed()`; `valid_read`/`valid_write` is recommended as its own future row (a real, well-scoped, dialect-shared file-permission feature) rather than staying folded into 1.16, whenever it gets an explicit go-ahead. `valid_query_snoop` (if `interactive_info()` is ever taken on as its own row) stays alongside it as previously noted) |

---

## Phase 2 - Architecture differentiation

**Goal: surpass all three drivers on the dimensions none of them addressed.**

### 2a - Persistence

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 2.1 | World-level statedump: serialize full heap to compact binary snapshot | `src/persist` | [ ] |
| 2.2 | Object swapout: page inactive objects to disk; demand-page on access | `src/persist` + `src/object` | [ ] |
| 2.3 | Hotboot: fd-passing exec into new binary without dropping connections | `src/persist` + `src/net` | [ ] |
| 2.4 | Dual persistence: per-object `save_object` AND world snapshot coexist | `src/persist` + `src/efun` | [ ] |

### 2b - Concurrency

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 2.5 | C++20 coroutine scheduler: cooperative suspend/resume of LPC tasks | `src/scheduler` | [ ] |
| 2.6 | LPC `async`/`await` keyword pair backed by coroutine scheduler | `src/compiler` + `src/vm` + `src/scheduler` | [ ] |
| 2.7 | `call_out_future(delay)` - awaitable call_out | `src/efun` + `src/scheduler` | [ ] |
| 2.8 | Open Hydra: speculative parallel tasks on disjoint object graphs | `src/scheduler` | [ ] |

### 2c - Apply cache + JIT

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 2.9 | Apply cache: hash (object × function-name) → FunctionEntry; invalidate on recompile | `src/apply` + `src/vm` | [ ] |
| 2.10 | Closure bake-at-construction: resolve `FP_*` kind + index at bind time | `src/vm` + `src/compiler` | [ ] |
| 2.11 | LLVM JIT backend: compile hot bytecode functions to native via LLVM IR | `src/jit` | [ ] |

### 2d - Efun breadth beyond FluffOS

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 2.12 | Full PCRE regexp suite (already started in Phase 0 - extend) | `src/efun` | [ ] |
| 2.13 | TLS support (OpenSSL/BoringSSL) for game + MXP/WebSocket | `src/net` | [ ] |
| 2.14 | WebSocket framing on top of TLS | `src/net` | [ ] |
| 2.15 | SQLite built-in: `db_connect`/`db_exec`/`db_fetch`/`db_close` efuns | `src/efun` | [ ] |
| 2.16 | Hash efuns: SHA-256/SHA-512/MD5/bcrypt/BLAKE2 | `src/efun` | [ ] |
| 2.17 | `json_encode`/`json_decode` efun pair | `src/efun` | [ ] |
| 2.18 | `http_get`/`http_post` async efuns (non-blocking, via async scheduler) | `src/efun` + `src/scheduler` | [ ] |

### 2e - Developer experience

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 2.19 | LSP server for LPC (`--lsp` flag): hover, go-to-def, diagnostics | `src/lsp` | [ ] |
| 2.20 | Structured error objects: JSON-serializable source/line/column/message | `src/core` | [ ] |
| 2.21 | Hot-reload: recompile + migrate one `.c` file while server is live | `src/object` + `src/vm` | [ ] |
| 2.22 | LPC-native test runner: `assert_equal`/`assert_throws` efun suite | `src/efun` + mudlib `std/test.c` | [ ] |

---

## Phase 3 - Production hardening

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 3.1 | Full `privs_file` / uid/gid object trust hierarchy | `src/security` | [ ] |
| 3.2 | Filesystem jail per object domain; capability grants for `call_other` | `src/security` | [ ] |
| 3.3 | Generational GC replacing `shared_ptr`-everywhere | `src/gc` | [ ] |
| 3.4 | Full telnet option negotiation + GMCP, MSDP, MSSP, MTTS, MXP | `src/proto` + `src/net` | [ ] |
| 3.5 | Conformance test suite (any driver can certify FluffOS/LDMud/DGD dialect) | `tests` | [ ] |
| 3.6 | LPC language specification document | `docs` | [ ] |
| 3.7 | Driver API reference + mudlib porting guide for each dialect | `docs` | [ ] |
| 3.8 | Boot a real third-party FluffOS mudlib against this driver (dead-souls.net's TMI2, LPUniversity, or LIL) | `tests` | [ ] |

**3.8 scope note:** a distinct compatibility test from `test/test_lexer.cpp`'s own regression suite, not a replacement for it -- that suite exercises this driver's own AMLP-derived mudlib content; this row exercises an independently-written real-world mudlib this driver has never seen. Not started. Involves, roughly in order: download and unpack one of the three archives (dead-souls.net's `tmi2_fluffos_v3.zip`, `lpuni_fluffos_v1.zip`, or `lil_0.3.zip`); identify real mismatches against this driver's own current conventions (master-object apply table, simul_efun resolution, directory-layout/`#include` path assumptions, any efun that mudlib calls but this table doesn't register); then whatever driver-side compat work those actually-found gaps call for, scoped to what's found, not speculative. No work done yet -- filed as a row, per this session's own request, so it has a place to land rather than being reintroduced ad hoc later.

---

## Build and test

```bash
cmake -B build -S .
cmake --build build
ctest --test-dir build --output-on-failure
```

Current baseline: **528 tests passing** (as of 2026-08-22). Every new slice
must pass the full suite before merging.

---

## Sequencing principle

- **Never break the current test baseline** (see above). All work is incremental slices.
- **Phase 0 before Phase 1.** A buggy foundation makes dialect work meaningless.
- **Phase 1 before Phase 2.** Dialect abstraction unlocks concurrent dialect work.
- **Read the instruct.md in the target directory first.** Each one lists exact
  files to read, exact reference sources, and the precise scope of its tasks.
- **One slice = one PR.** Keep changes small, reviewable, and revertable.
