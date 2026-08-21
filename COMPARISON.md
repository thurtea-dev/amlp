# AMLP vs. real FluffOS, LDMud, and DGD

An evidence-based comparison, not a marketing page. Every number here is
either read directly out of `ROADMAP.md`'s own accounting (which is
itself sourced from `git log`-recorded, per-row citations against the
vendored reference sources) or freshly re-checked against those same
vendored sources while writing this file (`temp/reference/fluffos-2.9-ds2.08/`,
`temp/ldmud/`, `temp/dgd/`: see `CLAUDE.md` for how these are tracked
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

Last updated: 2026-08-20 (refreshed twice the same day: first, Phase 1's
real-blocker count moved from 5/11 to 10/11, not from new feature work
but from five stale checkboxes, rows 1.2, 1.3, 1.4, 1.9, and 1.16 --
corrected to match real, already-landed work their own cells had
recorded across many earlier sessions but never checked off; second, a
further session actually built row 0.13a's own last remaining Phase 0
sub-gap, `parse_sentence()`'s `nicks` argument, real `add_nicknames()`/
`expand_node()` now ported and tested. See `ROADMAP.md`'s own dated
corrections on each row and this file's own rewritten sections below for
the full accounting.)
`ROADMAP.md` and `STATUS.md` are the living
documents; if this file and either of those disagree on a specific row's
status, trust `ROADMAP.md`'s own checkbox and re-derive this file's own
summary from it rather than the reverse.

---

## How far along is AMLP, in plain language

AMLP is a working, from-scratch LPC driver (lexer/parser/compiler/VM/
object system/network layer, no code shared with any real driver) that
already runs a real bundled mudlib end to end, login, movement,
command dispatch, object creation, persistence, sockets, and has grown
a substantial fraction of real FluffOS's own efun surface plus the start
of genuine LDMud and DGD dialect support behind a config switch. It is
not yet a drop-in replacement for either real driver: Phase 2 and Phase 3
(the features meant to eventually *exceed* what either real driver
offers) each have a planning document but zero implemented code, though
Phase 1's own real, corpus-driven dialect-compatibility work is now
substantially exhausted: see immediately below.

**Phase 0 (stabilize the current base): complete.**
16 of 16 rows checked off, including `parse_*` (0.13a), the large
natural-language parser package that was the one still-open row as of
this file's prior revision: now checked (partial: one real sub-gap
remains, see below, but all 8 named efuns are implemented, including
real two-object matching, and the row's own multi-session scope has
landed).

**Phase 1 (dialect universality): the real, corpus-driven work is now
substantially exhausted, not "a bit under half done."** An earlier
revision of this file undercounted this badly: five rows (1.2, 1.3,
1.4, 1.9, 1.16) had real, already-landed work sitting in their own
`ROADMAP.md` cells across several earlier sessions that was never
reflected in their checkboxes, corrected 2026-08-20. Counting only the
rows that actually gate Phase 1 completion (DGD-only rows are comparison
context, not blockers, per the scope clarification above): **10 of 11
real blocking rows are done.** The one still open, row 1.8
(`#'lfun::`/`#'sefun::`/`#'var::` closure-literal prefixes), was
investigated for the first time this same session and found to have
**zero real corpus usage** for its own remaining scope across every
vendored mudlib corpus in `temp/`: deferred on the same
zero-evidence-discipline basis this project has applied consistently
elsewhere (`bind_lambda()`'s cross-object form, `lambda()` itself, DGD's
own five still-open rows), not forgotten or blocked. Counting only rows
with real, non-DGD, non-zero-evidence scope still open, as opposed to
counting every unchecked box regardless of what is actually left in it:
**zero real Phase 1 blockers remain.** See `ROADMAP.md`'s own row-by-row
citations for the underlying evidence on every count below, not repeated
here.

**Phase 2 and Phase 3: not started.** Every directory Phase 2/3 work
would live in (`src/jit`, `src/gc`, `src/lsp`, `src/persist`,
`src/security`) contains nothing but its own planning `instruct.md`.
This is stated plainly rather than implied by an unchecked box: nothing
in this repository currently does coroutine scheduling, JIT compilation,
hotboot, world-level statedump, TLS, or any of the other Phase 2/3
items. They are real, considered plans, not real code.

| Phase | Rows | Done | Open | % done |
|---|---|---|---|---|
| 0, Stabilize | 16 | 16 | 0 | 100% |
| 1, Dialect universality (real blockers only, DGD-only rows excluded) | 11 | 10 | 1 | 91% |
| 1, Dialect universality (including 5 DGD-only comparison rows) | 16 | 10 | 6 | 63% |
| 2, Beyond both (novel features) | 22 | 0 | 22 | 0% |
| 3: Production hardening + docs | 8 | 0 | 8 | 0% |

**What is left open in Phase 1, and why each item stays open** (each
with its own detailed, source-cited scoping note in `ROADMAP.md`, not
guessed at here):

- **Row 1.8 (LDMud `#'lfun::`/`#'sefun::`/`#'var::` closure-literal
  prefixes), the one row with real remaining scope, deferred on
  evidence**: the bare `#'name` form and the `#'efun::name` forced-tier
  prefix (rows 1.2/1.3) are real and tested; `#'lfun::`/`#'sefun::`
  (more forced-tier prefixes) and `#'var::` (`CLOSURE_IDENTIFIER`, a
  reference-to-a-global-variable closure kind this driver has no model
  of at all, structurally distinct from a callable closure) have zero
  confirmed real mudlib call sites anywhere in `temp/`: the only hit
  for any of the three is the LDMud driver's own changelog prose noting
  when it added them, not a real mudlib using them.
- **Row 1.7's own remaining sub-items (the row itself is closed, these
  are real, named exceptions inside it, not a separate open row)**:
  `H_LOAD_UIDS`/`H_CLONE_UIDS`/`H_INCLUDE_DIRS` driver-hook trigger
  points have real but minimal evidence, 3 real call sites, all in one
  file (`secure/master/hooks.c`), versus 324 files defining `reset()`
  and 43 defining `clean_up()`, which is why `H_RESET`/`H_CLEAN_UP` (now
  real, dialect-gated where the two real drivers genuinely disagree) was
  picked first. Plain dialect-agnostic `lambda()`, `bind_lambda()`'s
  cross-object form, and per-hook type-map validation all have zero real
  corpus pressure, `privilege_violation()` (which the cross-object
  `bind_lambda()` form is gated behind) is a security-relevant
  authorization gate with no corpus call-site signal of its own by
  nature, a defensive-completeness question rather than a
  compatibility gap, flagged as a candidate for its own future
  evidence-independent evaluation rather than silently grouped with the
  zero-usage items above.
- **Row 1.9's own remaining sub-items (the row itself is closed, same
  pattern as row 1.7 above)**: `m_allocate`/`m_entry`/`m_reallocate`/
  `m_add`/`m_contains` (the real N-columns-wide efun family) and the
  `([:width])` empty-mapping literal all have zero real call sites
  across every corpus in `temp/`: `m_indices()`/`m_values()` (the two
  real names with real usage), the width-2 `([ k: v1; v2 ])` literal, and
  `map[key, n]` indexing/assignment (including a real IncDec-on-column
  bug this project's own live-bug-first discipline caught and fixed) are
  the parts of this row real corpus evidence actually called for, and
  are done.
- **DGD's own five still-open rows (1.11-1.15)**: real, considered
  scope with real citations against `temp/dgd/`'s own source, but
  explicitly comparison context rather than a Phase 1 blocker per this
  project's own stated goal: a FluffOS/LDMud-level driver done better
  than either, not three-way parity with DGD.

## Row 0.13a (`parse_*`), now checked (partial), in detail

FluffOS's real natural-language sentence/grammar-rule parser package
(`packages/parser.c`, 3,419 lines): confirmed, not assumed, to matter:
Dead Souls' own core command dispatch calls `parse_sentence()` directly.
All 8 real efun names are now implemented (`parse_init`, `parse_add_rule`,
`parse_add_synonym`, `parse_remove`, `parse_dump`, `parse_refresh`,
`parse_sentence`, `parse_my_rules`), including full `OBJ`/`LIV`/`OBS`/`LVS`
noun-phrase-to-object matching, both single-object rules (candidate
resolution, adjective/ordinal narrowing, `LIV_MODIFIER`, "all of"/plural
`OBS` matching, ambiguity/error reporting) and real two-object rules
("give OBJ to LIV", both singular and plural shapes, e.g. "give OBS to
LIV") via `dependent_check_functions()`/`check_one_relation()`/
`check_object_relations()`, all live-verified against a real running
driver. Real corpus evidence found 77 real two-object rules in Dead
Souls' own `lib/verbs/` alone, so this was a real, sized piece of work,
not a theoretical corner case. `parse_sentence()`'s own 4th `nicks`
argument (a caller-supplied nickname mapping, real `add_nicknames()`/
`expand_node()`) is now real too, 2026-08-20, this row's own last
remaining sub-gap is closed. See `ROADMAP.md` row 0.13a for the full
component breakdown, citations, and live-verification history.

---

## Codebase scale

Raw line counts, `.c`/`.cpp`/`.h`/`.hpp` only, each driver's own real
source tree as vendored in `temp/`. A scale comparison, not a quality
one: AMLP is deliberately smaller because it targets specific,
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
| **AMLP** | **248 of 270 real FluffOS names** (240 non-`parse_*` + all 8 `parse_*`) | `ROADMAP.md` row 0.13/0.13a's own accounting. The 22-name real gap: 40 non-`parse_*` names are documented, individually-verified exclusions (architecture mismatch, e.g. no `TYPE_CLASS`/buffer-type/ed()-editor equivalent, or zero real call sites across all six vendored mudlib corpora) minus the ones no longer counted against the gap. `parse_*` itself is no longer part of the gap at all (all 8 names implemented, every argument of every one of them real as of 2026-08-20's `nicks` slice, see row 0.13a's own entry). AMLP's own efun table primarily targets FluffOS's surface, with LDMud/DGD-specific additions layered on where a dialect diverges (`m_indices`/`m_values`, `#'name`, `nil`, `atomic`): it does not separately track coverage against LDMud's or DGD's own full efun/kfun lists the way it does for FluffOS. |

## Master/boot apply coverage

All three real drivers gate a running game through a "master object"
(FluffOS/LDMud) or "driver object" (DGD) that the driver core calls back
into for privilege checks, boot sequencing, and connection lifecycle
events, dozens of real named applies each. AMLP's own `BootApi`
abstraction (`include/amlp/dialect/BootApi.hpp`) currently recognizes
exactly **one** real per-dialect apply, `masterUidApply()`
(`get_root_uid` for FluffOS, `get_master_uid` for LDMud), deliberately
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
| Dialect selectable via config, one driver | Yes (`fluffos`/`ldmud`/`dgd`) |, (is FluffOS) |, (is LDMud) |, (is DGD) |
| Closures: `(: name :)` / `#'name` (FluffOS-style) | Yes | Yes | Yes (also has its own richer kinds) |, |
| Driver hooks (`set_driver_hook()`, `inaugurate_master()` boot wiring) | Partial (full 32-slot storage/dispatch real; 2 of several real trigger points wired, `H_MOVE_OBJECT0/1`, `H_MODIFY_COMMAND`) |, | Yes |, |
| Closures: real `lambda()`/`unbound_lambda()`/`bind_lambda()` kind distinction | Partial (`unbound_lambda()`/`bind_lambda()` real for the one confirmed corpus quoted-code shape; plain `lambda()` and the full closure-kind matrix not started, row 1.7/1.8 open) |, | Yes |, |
| Mapping width > 1 (`m_allocate`, N-column values) | Partial (`m_indices`/`m_values` real names ported, single-column only; row 1.9 open) |, | Yes |, |
| Shadows (`shadow()`, LDMud `unshadow()`/`query_allow_shadow`) | Yes | Yes (FluffOS shape) | Yes (LDMud shape, done) |, |
| `replace_program()`, LDMud no-arg sole-inherit form | Yes | Partial (has `replace_program`, not the LDMud no-arg form) | Yes |, |
| `nil` as a distinct value | Yes (dialect-gated) |, |, | Yes |
| `atomic` function modifier (checkpoint/rollback) | Lexed only, no VM semantics (row 1.12, not started) |, |, | Yes |
| `rlimits` (per-task tick/stack limits) | No (row 1.11, not started) |, |, | Yes |
| `parse_string` (grammar-driven string parsing kfun) | No (row 1.13, not started, confirmed comparable in size to `parse_*` itself, a dedicated DFA+LALR subsystem) |, |, | Yes |
| Lightweight objects (value-semantics objects) | No (row 1.14, not started) |, |, | Yes |
| `parse_*` natural-language sentence parser | All 8 efuns real, including single- and two-object `OBJ`/`LIV`/`OBS`/`LVS` matching and the `nicks` nickname-mapping argument | Yes (real source this work is ported from) |, |, |
| `save_object`/`restore_object`, real `.o` text format | Partial (restore-side only; save still uses this driver's own format) | Yes | Yes (own format) | Statedump-based, different model entirely |
| PCRE `regexp`/`regexplode`/`reg_assoc` | Yes | Yes | Yes (own regexp efuns) |, |
| Full telnet IAC negotiation, echo suppression, NAWS | Yes | Yes | Yes | Yes |
| `socket_*` efun family | Partial (STREAM/DATAGRAM only, no MUD mode, no binary modes) | Yes (full) | Yes (full) |, |
| Coroutine scheduler / `async`/`await` | No (Phase 2, not started) |, |, |, |
| LLVM JIT backend | No (Phase 2, not started) |, |, |, |
| Hotboot (fd-passing exec, connections survive) | No (Phase 2, not started) | Yes | Yes | Yes (via statedump/restart, different mechanism) |
| World-level statedump / object swapout | No (Phase 2, not started) |, |, | Yes (DGD's own signature architecture) |
| TLS / WebSocket | No (Phase 2, not started) | Not in this vendored ds2.08 snapshot | Not checked | Not checked |
| Built-in SQLite / hash / JSON efuns | No (Phase 2, not started) | Some (own DB package options) | Some | Some |
| LSP server (`--lsp`) | No (Phase 2, not started) |, |, |: |
| Generational GC (replacing `shared_ptr`) | No (Phase 3, not started) | Real GC | Real GC | Real GC |
| Full privilege/uid trust hierarchy | Partial (`privs()`, no full uid/euid/domain hierarchy) | Yes | Yes | Yes (own model) |

---

## What AMLP does not have, stated plainly

- **No `ed()` line editor, no database package, no crypto package
  beyond `crypt`/`oldcrypt`**: real FluffOS/LDMud both ship these;
  AMLP's own row 0.13 accounting lists them as confirmed architecture-
  mismatch exclusions, not silent gaps, but they are real absent
  features regardless of the reason.
- **No real garbage collector.** Object lifetime is plain
  `std::shared_ptr` reference counting throughout (`LpcObject`, closures,
  arrays, mappings). This means no cycle collection at all: a real,
  documented category of memory a genuine GC would reclaim that this
  driver currently never does. Phase 3's own `src/gc` item is exactly
  this, and is entirely unstarted.
- **Every Phase 2/3 differentiator is a plan, not code.** Coroutines,
  JIT, hotboot, statedump, TLS, LSP, hot-reload, a conformance suite --
  all have a real `instruct.md` and zero implementation. None of them
  should be described as "in progress."
- **Master/boot apply coverage is currently one name deep**
  (`masterUidApply()` only) against each real driver's own much larger
  master-object callback surface: see the section above. LDMud's
  separate driver-hook mechanism (`set_driver_hook()`,
  `inaugurate_master()`) is real and automatically wired at boot, but
  only 2 of its many real trigger points actually dispatch anything yet.
- **Dialect coverage is asymmetric.** FluffOS is the primary, most
  complete target (this is where the bundled mudlib and most of the
  regression corpus point); LDMud has real, working, dialect-gated
  pieces (closures, `m_indices`/`m_values`, shadows, `replace_program`,
  driver hooks) but real gaps (full closure kinds, mapping width, most
  hook trigger points); DGD support is the
  thinnest of the three by design (comparison-only, not a completion
  target): `nil` and `atomic`-the-keyword are the only DGD-dialect
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
of the three real drivers: its own lexer, parser, code generator,
bytecode VM, object system, and network layer are original
implementations, verified continuously against real vendored source and
a real bundled mudlib rather than against assumption. 694 regression
tests pass as of this writing (see `STATUS.md` for the current count,
which changes every session), and the discipline behind every checked
row above is the same: read the real source, port the real behavior
(including confirmed real quirks and off-by-ones where they exist, not
just the "sensible" version), and verify live against a real running
instance before calling anything done.
