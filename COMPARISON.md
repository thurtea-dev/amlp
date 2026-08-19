# AMLP vs. real FluffOS, LDMud, and DGD

An evidence-based comparison, not a marketing page. Every number here is
either read directly out of `ROADMAP.md`'s own accounting (which is
itself sourced from `git log`-recorded, per-row citations against the
vendored reference sources) or freshly re-checked against those same
vendored sources while writing this file (`temp/reference/fluffos-2.9-ds2.08/`,
`temp/ldmud/`, `temp/dgd/` -- see `CLAUDE.md` for how these are tracked
and why they are gitignored). Nothing here is from memory or general
reputation. Where a number is an estimate rather than an exact count,
it is labeled as one.

**DGD is comparison-only context, not a target.** `ROADMAP.md`'s own
Phase 1 header, and the scope clarification dated 2026-08-18, are
explicit about this: the actual goal is a FluffOS/LDMud-level driver
done better than either, not three-way parity with DGD. DGD numbers
below exist so a reader can see how a third, architecturally different
driver solved the same problems, not because AMLP is trying to match it
feature-for-feature.

Last updated: 2026-08-18. `ROADMAP.md` and `STATUS.md` are the living
documents; if this file and either of those disagree on a specific row's
status, trust `ROADMAP.md`'s own checkbox and re-derive this file's own
summary from it rather than the reverse.

---

## How far along is AMLP, in plain language

AMLP is a working, from-scratch LPC driver (lexer/parser/compiler/VM/
object system/network layer, no code shared with any real driver) that
already runs a real bundled mudlib end to end -- login, movement,
command dispatch, object creation, persistence, sockets -- and has grown
a substantial fraction of real FluffOS's own efun surface plus the start
of genuine LDMud and DGD dialect support behind a config switch. It is
not yet a drop-in replacement for either real driver: large, well-scoped
pieces of Phase 1 (dialect completeness) are still open, and every
Phase 2 and Phase 3 item (the features meant to eventually *exceed* what
either real driver offers) has a planning document but zero implemented
code.

**Phase 0 (stabilize the current base): effectively complete.**
15 of 16 rows checked off; the one open row, `parse_*` (0.13a), is a
large natural-language parser package that is itself most of the way
done (see below) and is explicitly carved out as its own multi-session
project rather than a Phase-0-blocking gap.

**Phase 1 (dialect universality): the real work, roughly one third
done.** Counting only the rows that actually gate Phase 1 completion
(DGD-only rows are comparison context, not blockers, per the scope
clarification above): 4 of 11 real blocking rows are done. The rest are
individually scoped, with real, specific reasons on record for why each
is still open (see the phase table below) -- none of them are vague
"todo" placeholders.

**Phase 2 and Phase 3: not started.** Every directory Phase 2/3 work
would live in (`src/jit`, `src/gc`, `src/lsp`, `src/persist`,
`src/security`) contains nothing but its own planning `instruct.md`.
This is stated plainly rather than implied by an unchecked box: nothing
in this repository currently does coroutine scheduling, JIT compilation,
hotboot, world-level statedump, TLS, or any of the other Phase 2/3
items. They are real, considered plans, not real code.

| Phase | Rows | Done | Open | % done |
|---|---|---|---|---|
| 0 -- Stabilize | 16 | 15 | 1 (large, in progress) | 94% |
| 1 -- Dialect universality (real blockers only, DGD-only rows excluded) | 11 | 4 | 7 | 36% |
| 1 -- Dialect universality (including 5 DGD-only comparison rows) | 16 | 4 | 12 | 25% |
| 2 -- Beyond both (novel features) | 22 | 0 | 22 | 0% |
| 3 -- Production hardening + docs | 8 | 0 | 8 | 0% |

**What is genuinely blocking Phase 1 completion right now** (each with
its own detailed, source-cited scoping note in `ROADMAP.md`, not
guessed at here):

- **Row 1.2/1.3 (dialect-aware lexer/parser), partially landed**: DGD's
  `atomic` keyword and `nil` literal are dialect-gated and working;
  LDMud's `#'name` bare-closure-literal form is dialect-gated and
  working. Still open: `'name` symbol literals, `rlimits`, and every
  `#'` form beyond the bare name (`#'+`, `#'[`, `#'efun::`, ...).
- **Row 1.4/1.16 (pluggable boot API), the one open architectural
  question**: real FluffOS/LDMud/DGD each have a *differently shaped*
  connect/disconnect callback contract (FluffOS: `connect()`/`net_dead()`
  on a fresh login object; LDMud: broadly similar but different apply
  names; DGD: a three-way `telnet_connect`/`binary_connect`/
  `datagram_connect` port-type fork plus `close()` on the persistent
  user object itself, not a fresh login object at all). `BootApi`
  currently abstracts exactly one apply (`masterUidApply()`) and
  deliberately omits connect/disconnect until this three-way shape
  mismatch gets a real design decision -- not an oversight, an
  explicitly recorded open question (`include/amlp/dialect/BootApi.hpp`'s
  own comment).
- **Row 1.7/1.8 (LDMud closure kinds)**: `lambda()`/`unbound_lambda()`/
  `bind_lambda()`/baked-`#'symbol` need a real `CLOSURE_LAMBDA`/
  `CLOSURE_BOUND_LAMBDA`/`CLOSURE_UNBOUND_LAMBDA` kind distinction this
  driver's own flat `Closure` struct does not have yet. Confirmed
  genuinely larger than a normal batch item, not attempted as a partial
  stand-in (a stand-in was considered and explicitly rejected -- see
  row 1.7's own note).
- **Row 1.9 (LDMud mapping width > 1)**: `m_indices()`/`m_values()`
  (the two highest-real-call-site names) are done; the real N-columns-
  wide value semantics (`m_allocate`/`m_entry`/`m_reallocate`/`m_add`/
  `m_contains`, the `([ k: v1; v2 ])` literal syntax) need a `Mapping`
  structure rework this driver's own single-column-per-key
  `std::vector<std::pair<Value, Value>>` does not support, cascading
  through indexing, `save_object`, and the existing mapping-union `+`
  operator.

## Row 0.13a (`parse_*`), the one open Phase 0 row, in detail

FluffOS's real natural-language sentence/grammar-rule parser package
(`packages/parser.c`, 3,419 lines) -- confirmed, not assumed, to matter:
Dead Souls' own core command dispatch calls `parse_sentence()` directly.
7 of 8 real efun names are implemented (`parse_init`, `parse_add_rule`,
`parse_add_synonym`, `parse_remove`, `parse_dump`, `parse_refresh`,
`parse_sentence`); only `parse_my_rules()` (a thin variant of
`parse_sentence()` itself) and full `OBJ`/`LIV`/`OBS`/`LVS`
noun-phrase-to-object matching (`parse_sentence()` currently handles
`STR`/`WRD`/literal-only rules, a real, confirmed subset of what the
package supports, not a simplification of it) remain. See `ROADMAP.md`
row 0.13a for the full component breakdown and exact remaining scope.

---

## Codebase scale

Raw line counts, `.c`/`.cpp`/`.h`/`.hpp` only, each driver's own real
source tree as vendored in `temp/`. A scale comparison, not a quality
one -- AMLP is deliberately smaller because it targets specific,
confirmed-real-usage compatibility rather than reimplementing every
package (own database/crypto/ed-editor/full-MXP suites) either real
driver ships.

| Driver | Lines (`.c`/`.cpp`/`.h`) | Note |
|---|---|---|
| LDMud | ~211,600 | `temp/ldmud/src` |
| FluffOS 2.9 (ds2.08) | ~92,100 | `temp/reference/fluffos-2.9-ds2.08` |
| DGD (this vendored C++ port) | ~70,500 | `temp/dgd/src` |
| **AMLP** | **~22,500** | `src/` + `include/`, this repo |

## Efun / kfun surface

| Driver | Real count | Method |
|---|---|---|
| FluffOS 2.9 (ds2.08) | 270 | `ROADMAP.md` row 0.13's own `efun_defs.c` accounting (excludes ifdef'd-out/non-runtime entries; a raw `grep -c '^{"'` over the same file gives 276, the 6-name difference being exactly those exclusions) |
| LDMud | ~305 | Rough estimate: `temp/ldmud/doc/efun/` file count (one doc page per real efun is LDMud's own documentation convention; not independently cross-checked against a table the way the FluffOS/DGD/AMLP numbers were, so treat as approximate) |
| DGD (this vendored C++ port) | 243 | Real count: `grep -c '^FUNCDEF('` across `temp/dgd/src/kfun/{builtin,std,file,math,extra}.cpp` |
| **AMLP** | **247 of 270 real FluffOS names** (240 non-`parse_*` + 7 of 8 `parse_*`) | `ROADMAP.md` row 0.13/0.13a's own accounting. The 23-name real gap: 40 non-`parse_*` names are documented, individually-verified exclusions (architecture mismatch, e.g. no `TYPE_CLASS`/buffer-type/ed()-editor equivalent, or zero real call sites across all six vendored mudlib corpora) minus the ones no longer counted against the gap, plus the one still-open `parse_my_rules()`. AMLP's own efun table primarily targets FluffOS's surface, with LDMud/DGD-specific additions layered on where a dialect diverges (`m_indices`/`m_values`, `#'name`, `nil`, `atomic`) -- it does not separately track coverage against LDMud's or DGD's own full efun/kfun lists the way it does for FluffOS. |

## Master/boot apply coverage

All three real drivers gate a running game through a "master object"
(FluffOS/LDMud) or "driver object" (DGD) that the driver core calls back
into for privilege checks, boot sequencing, and connection lifecycle
events -- dozens of real named applies each. AMLP's own `BootApi`
abstraction (`include/amlp/dialect/BootApi.hpp`) currently recognizes
exactly **one** real per-dialect apply, `masterUidApply()`
(`get_root_uid` for FluffOS, `get_master_uid` for LDMud) -- deliberately
narrow, not an oversight: `connectApply()`/`netDeadApply()` are
explicitly omitted pending the three-way connect/disconnect design
question above (row 1.4/1.16). Real per-file `valid_read`/`valid_write`
privilege checks are implemented directly in `EfunTable.cpp` (dialect-
gated per real FluffOS 3-arg vs. LDMud 4-arg call convention) without
going through the `BootApi` abstraction at all. This is a real, current
gap against both real drivers' own much larger master-apply surface,
not yet closed.

---

## Feature-by-feature

Checkmarks mean "implemented and verified live against the real running
driver," not "attempted." A dash means the real driver in that column
does not have the feature at all (not a gap for it, just not
applicable).

| Feature | AMLP | FluffOS 2.9 | LDMud | DGD |
|---|---|---|---|---|
| Dialect selectable via config, one driver | Yes (`fluffos`/`ldmud`/`dgd`) | -- (is FluffOS) | -- (is LDMud) | -- (is DGD) |
| Closures: `(: name :)` / `#'name` (FluffOS-style) | Yes | Yes | Yes (also has its own richer kinds) | -- |
| Closures: real `lambda()`/`unbound_lambda()`/`bind_lambda()` kind distinction | No (row 1.7, open) | -- | Yes | -- |
| Mapping width > 1 (`m_allocate`, N-column values) | Partial (`m_indices`/`m_values` real names ported, single-column only; row 1.9 open) | -- | Yes | -- |
| Shadows (`shadow()`, LDMud `unshadow()`/`query_allow_shadow`) | Yes | Yes (FluffOS shape) | Yes (LDMud shape, done) | -- |
| `replace_program()`, LDMud no-arg sole-inherit form | Yes | Partial (has `replace_program`, not the LDMud no-arg form) | Yes | -- |
| `nil` as a distinct value | Yes (dialect-gated) | -- | -- | Yes |
| `atomic` function modifier (checkpoint/rollback) | Lexed only, no VM semantics (row 1.12, not started) | -- | -- | Yes |
| `rlimits` (per-task tick/stack limits) | No (row 1.11, not started) | -- | -- | Yes |
| `parse_string` (grammar-driven string parsing kfun) | No (row 1.13, not started -- confirmed comparable in size to `parse_*` itself, a dedicated DFA+LALR subsystem) | -- | -- | Yes |
| Lightweight objects (value-semantics objects) | No (row 1.14, not started) | -- | -- | Yes |
| `parse_*` natural-language sentence parser | 7 of 8 efuns real, `STR`/`WRD`/literal rules only | Yes (real source this work is ported from) | -- | -- |
| `save_object`/`restore_object`, real `.o` text format | Partial (restore-side only; save still uses this driver's own format) | Yes | Yes (own format) | Statedump-based, different model entirely |
| PCRE `regexp`/`regexplode`/`reg_assoc` | Yes | Yes | Yes (own regexp efuns) | -- |
| Full telnet IAC negotiation, echo suppression, NAWS | Yes | Yes | Yes | Yes |
| `socket_*` efun family | Partial (STREAM/DATAGRAM only, no MUD mode, no binary modes) | Yes (full) | Yes (full) | -- |
| Coroutine scheduler / `async`/`await` | No (Phase 2, not started) | -- | -- | -- |
| LLVM JIT backend | No (Phase 2, not started) | -- | -- | -- |
| Hotboot (fd-passing exec, connections survive) | No (Phase 2, not started) | Yes | Yes | Yes (via statedump/restart, different mechanism) |
| World-level statedump / object swapout | No (Phase 2, not started) | -- | -- | Yes (DGD's own signature architecture) |
| TLS / WebSocket | No (Phase 2, not started) | Not in this vendored ds2.08 snapshot | Not checked | Not checked |
| Built-in SQLite / hash / JSON efuns | No (Phase 2, not started) | Some (own DB package options) | Some | Some |
| LSP server (`--lsp`) | No (Phase 2, not started) | -- | -- | -- |
| Generational GC (replacing `shared_ptr`) | No (Phase 3, not started) | Real GC | Real GC | Real GC |
| Full privilege/uid trust hierarchy | Partial (`privs()`, no full uid/euid/domain hierarchy) | Yes | Yes | Yes (own model) |

---

## What AMLP does not have, stated plainly

- **No `ed()` line editor, no database package, no crypto package
  beyond `crypt`/`oldcrypt`** -- real FluffOS/LDMud both ship these;
  AMLP's own row 0.13 accounting lists them as confirmed architecture-
  mismatch exclusions, not silent gaps, but they are real absent
  features regardless of the reason.
- **No real garbage collector.** Object lifetime is plain
  `std::shared_ptr` reference counting throughout (`LpcObject`, closures,
  arrays, mappings). This means no cycle collection at all -- a real,
  documented category of memory a genuine GC would reclaim that this
  driver currently never does. Phase 3's own `src/gc` item is exactly
  this, and is entirely unstarted.
- **Every Phase 2/3 differentiator is a plan, not code.** Coroutines,
  JIT, hotboot, statedump, TLS, LSP, hot-reload, a conformance suite --
  all have a real `instruct.md` and zero implementation. None of them
  should be described as "in progress."
- **Master/boot apply coverage is currently one name deep**
  (`masterUidApply()` only) against each real driver's own much larger
  master-object callback surface -- see the section above.
- **Dialect coverage is asymmetric.** FluffOS is the primary, most
  complete target (this is where the bundled mudlib and most of the
  regression corpus point); LDMud has real, working, dialect-gated
  pieces (closures, `m_indices`/`m_values`, shadows, `replace_program`)
  but real gaps (full closure kinds, mapping width); DGD support is the
  thinnest of the three by design (comparison-only, not a completion
  target) -- `nil` and `atomic`-the-keyword are the only DGD-dialect
  pieces implemented, with the rest (`rlimits`, `atomic`-the-semantics,
  `parse_string`, LWOs, the driver+auto boot path) confirmed real and
  scoped but not started.
- **This driver's own memory model can reach states real FluffOS
  cannot.** Documented directly in `STATUS.md`/`ROADMAP.md` where
  found (e.g. `parse_dump()`'s own `"(destructed)"` fallback for a
  weak_ptr that expired without ever going through `destruct()`) --
  a consequence of `shared_ptr`-based lifetime instead of real FluffOS's
  synchronous refcounted free, not a bug, but a real behavioral
  difference worth knowing about if porting mudlib code that depends on
  exact destruction timing.

## Where AMLP is already a real, working, from-scratch driver

Worth stating alongside the gaps above, since a gap list alone
undersells what already works: AMLP is not a wrapper or a fork of any
of the three real drivers -- its own lexer, parser, code generator,
bytecode VM, object system, and network layer are original
implementations, verified continuously against real vendored source and
a real bundled mudlib rather than against assumption. 657 regression
tests pass as of this writing (see `STATUS.md` for the current count,
which changes every session), and the discipline behind every checked
row above is the same: read the real source, port the real behavior
(including confirmed real quirks and off-by-ones where they exist, not
just the "sensible" version), and verify live against a real running
instance before calling anything done.
