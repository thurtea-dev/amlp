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
| 0.13 | Grow efun table to FluffOS parity (~300 efuns) | `src/efun` | [ ] (in progress: 219 registered) |
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
| 1.1 | `LpcDialect` enum + config key `dialect` | `src/config` + `src/dialect` | [ ] |
| 1.2 | Dialect-aware Lexer: `#'`, `lambda`, `atomic`, `rlimits`, `nil` tokens | `src/compiler` + `src/dialect` | [ ] |
| 1.3 | Dialect-aware Parser: LDMud symbol/lambda syntax, DGD `atomic`/`rlimits` | `src/compiler` + `src/dialect` | [ ] |
| 1.4 | Pluggable boot API: FluffOS master/simul_efun, LDMud master, DGD driver+auto | `src/apply` + `src/dialect` | [ ] |
| 1.5 | LDMud shadows: `shadow(ob,1)`, shadow-chain `call_other`, `query_shadowing` | `src/object` | [ ] |
| 1.6 | LDMud `replaces` directive in `inherit` | `src/compiler` + `src/object` | [ ] |
| 1.7 | LDMud `lambda()` / `unbound_lambda()` / `bind()` closure kinds | `src/vm` + `src/compiler` | [ ] |
| 1.8 | LDMud `#'symbol` references baked at construction | `src/compiler` + `src/vm` | [ ] |
| 1.9 | LDMud mapping width > 1: `m_allocate`, `m_indices`, `m_values` | `src/vm` + `src/efun` | [ ] |
| 1.10 | DGD `nil` as distinct type in `Value` variant | `src/vm` | [ ] |
| 1.11 | DGD `rlimits` statement: per-task tick + stack depth limits | `src/vm` + `src/compiler` | [ ] |
| 1.12 | DGD `atomic` function modifier: VM-level checkpoint/rollback | `src/vm` | [ ] |
| 1.13 | DGD `parse_string` kfun | `src/efun` | [ ] |
| 1.14 | DGD lightweight objects (LWOs): value-semantics object kind | `src/vm` + `src/object` | [ ] |
| 1.15 | DGD driver+auto object boot path | `src/apply` + `src/dialect` | [ ] |
| 1.16 | LDMud master apply name table | `src/apply` + `src/dialect` | [ ] |

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
