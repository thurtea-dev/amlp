# STATUS

Older session entries (everything before the 5 most recent) live in
`STATUS-ARCHIVE.md`.

**2026-08-21 (a further session, continued the same day): a fresh
full-project status sweep (Phase 0 confirmed still 16/16, Phase 1
confirmed still 10/11 with the full remaining item list re-verified,
Phase 2/3 confirmed still 0/22 and 0/8), `COMPARISON.md` refreshed to
match, and this session's own next-priority recommendation written
directly into this entry, not left in chat only, closing a real gap
this project has now hit twice (see below). No test count change this
session (715, unchanged, no driver code touched).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules: `git add` only, no commits/pushes;
no em dashes or emojis).

**Why this entry is written the way it is.** The user asked directly
for the recommendation below to be written into this file in enough
detail that a future session or a different reviewer could read it cold
and understand the choice without the original conversation, because
the immediately prior session's own equivalent recommendation was given
only in the chat reply and turned out to be unrecoverable afterward.
That is not a one-off: this project has hit the exact same failure mode
before, on record in this very file, `STATUS.md`'s own 2026-08-19 entry
for the `parse_*`-versus-Phase-2 decision ends with "Full reasoning
given directly to the user this turn, not duplicated here: see this
same session's own reply for the three-way comparison." That reasoning
is also gone now, for the same reason. This entry is written to not
repeat that mistake a third time.

**Phase 0: confirmed still 16 of 16 rows checked, 100%, no open
sub-gap.** Re-read every row's own checkbox directly rather than
trusted from memory: all 16 rows (`0.1` through `0.15`, including
`0.13a`) read `[x]`, and `0.13a`'s own cell text confirms no remaining
sub-gap ("Nicks implemented, 2026-08-20", the last item this row had on
record). Unchanged from the last status-read pass.

**Phase 1: confirmed still 10 of 11 real (non-DGD) rows closed, 91%,
the fraction itself unchanged by this session's own driver work.** Row
1.7 was already checkbox-closed before this session (it was one of the
already-counted 10, "partial" status, real evidence-backed items still
open inside its own cell); this session's `call_out_info()`/`input_to()`
work closed the last of those items, but did not move the row-level
fraction, since the row was already counted as closed at the row
granularity `ROADMAP.md`'s own table tracks. Row 1.8 remains the only
row genuinely still open. Exact remaining scope, read directly from
each row's own current cell text rather than summarized from memory:

- **Row 1.8, `#'lfun::name`/`#'sefun::name`** (more forced-tier closure
  prefixes, reusable on the same mechanism `#'efun::` already
  established) **and `#'var::variable_name`** (a structurally distinct
  closure kind, real `CLOSURE_IDENTIFIER`, a reference to a global
  variable, not a callable at all, `doc/LPC/closures:43`,
  `closure.c:450`/`524`/`977`/`1198`/`4178`). Zero real mudlib call
  sites for any of the three across every corpus vendored in `temp/`,
  re-confirmed fresh by this row's own investigating session
  (2026-08-20); the only hit anywhere is the LDMud driver's own
  changelog prose noting when it added them.
- **Row 1.7's own remaining sub-items** (the row itself is closed,
  these are named exceptions inside it): `H_LOAD_UIDS`/`H_CLONE_UIDS`/
  `H_INCLUDE_DIRS` driver-hook trigger points (3 real call sites, all
  in one file, `secure/master/hooks.c`); real per-hook type-map
  validation (`hook_type_map[]`); plain dialect-agnostic `lambda()`;
  `inaugurate_master()`'s own arg=1/2/3 master-reload/reactivation
  cases (only arg=0, first boot, is wired); the remaining plain-string
  `hooks.c` hooks (`H_CREATE_SUPER`/`H_CREATE_OB`/`H_CREATE_CLONE`/
  `H_MODIFY_COMMAND_FNAME`/`H_NOTIFY_FAIL`/`H_TELNET_NEG`/
  `H_AUTO_INCLUDE`); and roughly 20 of the 26 real doc-cataloged
  `privilege_violation()` operations still ungated (several correspond
  to packages this driver does not implement at all, mysql/pgsql/
  sqlite, the erq demon, wizlist, so gating them would be meaningless
  until those packages exist; the rest have no real corpus evidence
  beyond `core-lib`'s own generic default-case fallback). All zero real
  corpus pressure, deferred on the same evidence discipline as
  everywhere else in this row.
- **Row 1.9's own remaining sub-items** (row itself closed): the
  `m_allocate`/`m_entry`/`m_reallocate`/`m_add`/`m_contains` N-columns-
  wide efun family and the `([:width])` empty-mapping literal, zero
  real call sites across every corpus in `temp/`.
- **DGD's own five still-open rows (1.11-1.15)**: real, scoped,
  cited against `temp/dgd/`'s own source, but comparison context, not a
  Phase 1 blocker, per this project's own explicit goal (a FluffOS/
  LDMud-level driver done better than either, not three-way DGD
  parity).

**Phase 2/3: confirmed still 0/22 and 0/8, planning documents only.**
Every row in both phases reads `[ ]`. Checked `src/jit`, `src/gc`,
`src/lsp`, `src/persist`, `src/security`, `src/scheduler`, `src/proto`:
each contains its own `instruct.md` and nothing else, no implementation
of coroutines, JIT, hotboot, statedump, a generational GC, TLS, or any
other Phase 2/3 item exists anywhere in this repository.

**`COMPARISON.md` refreshed to match**, in place, the same "refreshed
in place, not narrated" convention this file already used rather than
appending new prose sections: the Phase 0 summary no longer claims a
remaining `parse_*` sub-gap (closed last session); the row 1.7 bullet
describing `privilege_violation()` now states its real, current
four-trigger-point scope instead of framing it as an unbuilt future
candidate; the "Driver hooks" feature-table row now reads 5 real
trigger points wired (`H_MOVE_OBJECT0/1`, `H_MODIFY_COMMAND`, `H_RESET`,
`H_CLEAN_UP`), up from 2; a new feature-table row was added for
`privilege_violation()` itself (4 of 26 real operations gated); the
"what AMLP does not have" bullet's hook-trigger-point count updated to
match; and the closing test-count line bumped from 694 to 715. The
Phase/rows/done/open/percent table itself needed no change, both real
fractions (Phase 0 100%, Phase 1 real-blockers-only 91%) were already
correct. `ROADMAP.md`'s own Phase 1 status-read header gained a dated
"Update, 2026-08-21" paragraph closing the loop on its own prior
`privilege_violation()` framing, rather than being rewritten in place,
matching this file's own established per-row update convention.

**The recommendation, written here in full rather than left in chat.**
Three real candidates were weighed, the same three named in this
session's own prompt: (A) continue closing Phase 1's remaining
low-corpus-usage items anyway, for completeness; (B) pick up
`notes/ACCOUNT_LOGIN_PLAN.md`, now that Phase 1's well-evidenced work
has thinned out; (C) begin real Phase 2 planning-to-code work (JIT,
hotboot, a real GC, statedump, the LSP server).

**(A), continuing to close Phase 1's zero-evidence items anyway, is
rejected outright, not merely deprioritized.** Every one of the items
listed above under Phase 1's own remaining scope was already deferred
on an explicit, repeatedly-applied project discipline: real corpus
evidence decides what gets built, not checkbox completeness for its
own sake (the same discipline that correctly rejected a `bind_lambda()`
stand-in, deferred plain `lambda()`, and deferred DGD's own five rows).
Building `#'var::`, a structurally distinct closure kind needing its
own new value-representation work, for zero confirmed real callers
anywhere in `temp/`, or gating 20 more `privilege_violation()`
operations, several for packages this driver does not even implement,
would directly reverse that discipline for the sake of a cosmetic 11/11
rather than real compatibility value. Real evidence does not support
this candidate; it is not a live option unless new corpus evidence
surfaces.

**(C), beginning real Phase 2 planning-to-code work, is real and not
blocked, but is the weaker pick this session, for reasons specific to
how this project has made every other build decision, not a
size-based preference.** `ROADMAP.md`'s own stated sequencing
principle is "Phase 1 before Phase 2. Dialect abstraction unlocks
concurrent dialect work" (Phase 2's own header). Phase 1 being 10/11
with the 11th deliberately, permanently deferred pending evidence that
does not currently exist arguably satisfies that principle's actual
intent (a stable dialect abstraction to build on, which already
exists: `BootApi`, dialect-gated efuns throughout `EfunTable.cpp`), so
Phase 1's own incompleteness is not by itself a hard block. The real
reason to not pick this now: every other "big" item this project has
built (`parse_*`, `valid_read`/`valid_write`, `privilege_violation()`
itself) got its own dedicated cold-start scoping session, real source
read in full, real corpus usage checked, before any code was written,
specifically because guessing at scope for something this size wastes
a session or worse, produces an unfaithful shim. Phase 2 has 22 rows
across 5 genuinely different sub-areas (persistence, concurrency,
apply-cache/JIT, efun breadth, developer experience) with no single
obvious next row, and, unlike every Phase 0/1 item, none of it is
mudlib-compatibility work this project's own citation-against-real-
source methodology directly applies to: Phase 2 is novel
differentiation, not a compatibility gap, so the evidence bar that
decided every prior priority call (corpus call-site counts) does not
transfer cleanly. Picking a specific Phase 2 row well enough to build
it faithfully this session, rather than speculatively, would need its
own scoping investigation first, the same discipline `privilege_violation()`
got two sessions ago, not a same-session jump straight to code.

**(B), picking up `notes/ACCOUNT_LOGIN_PLAN.md`, is the recommendation,
on real evidence, not just because A is rejected and C is deferred.**
Four concrete reasons: first, it is the only candidate that is already
fully scoped and immediately buildable right now, not something
needing its own investigation session first, its own document already
did that work (2026-08-19): every efun it needs (`crypt`, `save_object`/
`restore_object`, `input_to`, `exec`, `mkdir`/`get_dir`/`file_size`,
`valid_read`/`valid_write`) is confirmed real and working today, a real
reference implementation shape was already read directly from
`temp/core-lib/secure/login.c`'s own real `input_to()`-driven state
machine, and a concrete first build slice is already named (`/single/
account_d.c`: account file format, `create_account`, `check_password`,
`account_exists`, testable in isolation via `eval`, no login
integration yet). Second, the document's own explicit self-deferral,
"does not compete with, block, or get worked ahead of the current
Phase 0/1 driver priority", was written when Phase 0 was still open and
Phase 1 still had real, well-evidenced driver-side work in flight
(`parse_*`, then `privilege_violation()`'s first two trigger points);
that condition has now genuinely lapsed, Phase 0 is 100% and Phase 1's
real corpus-driven work is exhausted down to (A)'s explicitly-rejected
items and (C)'s DGD-only rows, so picking this up now honors the
document's own stated condition rather than jumping the queue. Third,
it directly grows the bundled mudlib past its current "one room, a
wand, no real login" state into something a real player could actually
use, a different, concrete kind of value than either A (a cosmetic
percentage) or C (architecture nobody outside this project can observe
yet). Fourth, choosing it does not foreclose C: Phase 2 stays exactly
as real and as open as it is today, ready for its own dedicated
scoping session whenever it is picked up next, nothing about building
mudlib content this session makes that scoping work any smaller or
larger later.

**Built this same session, after the recommendation above: build
ordering item 1 from `notes/ACCOUNT_LOGIN_PLAN.md`, real account
storage.** New `/single/account_record.c` (a small per-account
data-holder object, `name`/`hash`/`created`/`characters` variables,
cloned fresh per operation and destructed right after: real
`save_object()`/`restore_object()` always act on `current_object()`'s
own variables, no target-object argument, so a per-account on-disk
file needs a per-account object to be `current_object()` while the
efun runs, not the daemon calling it from outside). New
`/single/account_d.c`: `account_exists()`, `create_account()`,
`check_password()`, files bucketed by the account name's own first
letter under a new `ACCOUNTS_DIR` (`/accounts`), the exact same
`name[0..0]` idiom this mudlib's own pre-existing `simul_efun.c:55`
already uses for `user_path()`, confirmed by reading that file rather
than invented fresh. `crypt(password, 0)` hashes a new password (real
salt-generation idiom, already cited in `crypt()`'s own EfunTable.cpp
registration comment); `crypt(password, existingHash) == existingHash`
verifies one (passing an already-computed hash back in as the "salt"
argument re-derives it with the same embedded salt, confirmed directly
from this driver's own `crypt()` implementation, a string salt of
length >= 2 is used as-is, not regenerated). Two new `globals.h`
constants, `ACCOUNT_D`/`ACCOUNT_RECORD`/`ACCOUNTS_DIR`.

**One tangential finding surfaced while researching real reference
material for this, flagged rather than silently passed over.**
`src/efun/EfunTable.cpp`'s own `save_object`/`restore_object`
registration comment, and two comments in `test/test_lexer.cpp`
(`testBareParentCallInvokesInheritedFunctionNotLocalOverride`'s own
header and one other), cite a file at `secure/daemon/account_d.c` as
something "found live compiling" with "confirmed live" behavior
against it. Searched for it directly before trusting the citation, the
same discipline used everywhere else in this project: it does not
exist anywhere in any vendored corpus, extracted or zipped
(`temp/core-lib`, every other extracted tree, and every zip/tar
archive under `temp/`, searched by name). The closest real match,
`temp/lima/lib/daemons/account_d.c`, is a same-named but unrelated
in-game banking/currency daemon (`query_account`/`deposit`/`withdraw`,
gold and credit balances), not a login/account-auth file at all, read
in full before ruling it out rather than assumed from the filename
alone, the same false-positive-by-name trap `notes/ACCOUNT_LOGIN_PLAN.md`
had already separately flagged for `skylib_fluffos_v3`'s own
`bank_accounts/`. This session's own new `/single/account_d.c` is
therefore original design work against this plan's own real citations
(`temp/core-lib/secure/login.c`'s state-machine shape, the real
`crypt()`/`save_object()` semantics already confirmed elsewhere), not
a port of the phantom-cited file, and does not reuse its path or
naming. The stale citation itself was not corrected this session,
tangential to this session's own actual task and not investigated
further than confirming it does not point at anything real: flagged
here so a future session does not build on it as if it were a
confirmed real source the way every other citation in this codebase is
meant to be.

**Live-verified against the real running driver, real bundled
`mudlib/`** (a scratch config on spare port 4144, default dialect, a
real Python TCP client, real `eval` calls only, no scratch objects or
master edits needed this time): `account_exists("bob")` correctly `0`
before creation; `create_account("bob", "hunter2")` returns `1`, the
real on-disk file (`mudlib/accounts/b/bob.o`) inspected directly,
correct bucketed path and correct saved fields; `account_exists("bob")`
now `1`; `check_password("bob", "hunter2")` returns `1`,
`check_password("bob", "wrongpass")` returns `0`; a duplicate
`create_account("bob", ...)` correctly returns `0`, does not overwrite;
case-insensitivity confirmed (`account_exists("BOB")`/
`check_password("BOB", "hunter2")` both resolve to the same account); a
second account (`"Alice"`, a different bucket letter) created
independently and correctly; empty name and empty password both
correctly rejected (`0`, no file written). Scratch accounts directory
removed afterward, confirmed via `git status` that `mudlib/` shows only
the two genuinely new files plus the `globals.h` addition.

**No new C++ regression tests for this slice, a deliberate choice, not
an oversight, stated so it reads as one on a future review.** Checked
first: every one of this suite's roughly 715 tests that touches mudlib
content writes its own scratch temp mudlib (`ObjectVarHarness`'s own
`mkdtemp()`-based tempdir), none exercises the real bundled `mudlib/`
tree directly, confirmed by grep, zero hits for a config pointing at
the real `mudlib_root: mudlib` anywhere in `test_lexer.cpp`. This
project's own established split, confirmed by precedent rather than
decided fresh here: driver-level C++ mechanisms (efuns, VM behavior,
dialect gates) get unit tests in this suite; real bundled mudlib
content (`master.c`/`simul_efun.c`/`wand_of_creation.c`/`login.c`
before it) gets live-running-driver verification instead, documented
in `STATUS.md`'s own dated entries, the same pattern this entry follows
above. `notes/ACCOUNT_LOGIN_PLAN.md` updated in place to record this
slice done and the reasoning above, matching its own established
per-item update convention.

715 tests passing (unchanged: no driver-side `src/` code was touched
this session at all, only documentation and new mudlib content).

**2026-08-21 (a further session): docs cleanup (personal scoping/plan
notes relocated to an untracked `notes/` folder, the `" -- "` em-dash
stand-in rewritten to proper punctuation across every active doc), then
row 1.7's own remaining `call_out_info()`/`input_to()` privilege_violation()
follow-on closed for real (715 tests, up from 709).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules: `git add` only, no commits/pushes;
no em dashes or emojis).

**The prior session's own next-priority recommendation could not be
recovered, said so directly rather than guessing.** Checked `git log`
first: the privilege_violation() cold-start investigation and its first
real slice (`bind_lambda()`/`set_driver_hook()`, commit `81f849b`) were
already committed, so nothing from that prompt's Steps 1 to 3 needed
redoing. But the specific three-way comparison the user asked to recover
(`call_out_info()`/`input_to()` vs. `notes/ACCOUNT_LOGIN_PLAN.md` vs.
starting Phase 2 planning) is not in `STATUS.md`, not in `ROADMAP.md`,
and this session has no access to that prior session's own raw chat
transcript, only what got persisted to repo files. This matches a known
pattern already on record elsewhere in this project (row 0.13a's own
"Full reasoning given directly to the user this turn, not duplicated
here" note, `STATUS.md`'s own 2026-08-19 entries): some sessions give
their full reasoning only in the chat reply, not in any file. Reported
this plainly rather than fabricating a recollection, then independently
re-derived a fresh recommendation from the actual current evidence
(below), rather than assume the missing one would have agreed.

**`notes/ACCOUNT_LOGIN_PLAN.md`'s relocation was pure repo hygiene, not
a priority signal, stated plainly since the user asked directly.** The
prior session's move of all three `*_PLAN.md`/`*_SCOPING.md` files out
of the tracked repo (per the user's own explicit "they're for me not the
public repo" instruction) was applied uniformly by filename pattern, not
by reading or judging any file's content or priority. The file's own
text already says this itself: "does not compete with, block, or get
worked ahead of the current Phase 0/1 driver priority." Nothing about
moving it to `notes/` changes that standing; it is still the live scope
document for whenever mudlib account/login work is actually picked up,
exactly as before the move.

**Fresh three-way comparison, evidence-based.** `call_out_info()`/
`input_to()`: a real, already-cited, already-scoped follow-on sitting in
row 1.7's own cell (`call_out.c:805-829`, `comm.c:7315-7317`), the last
concretely-open item keeping Phase 1 at 10/11 rather than 11/11, sized
for one session, no new architecture needed (reuses the exact
`VM::privilegeViolation()` shared core `bind_lambda()`/`set_driver_hook()`
already built). `notes/ACCOUNT_LOGIN_PLAN.md`: real, well-scoped mudlib
work, explicitly self-subordinated in its own text to "the current Phase
0/1 driver priority", not blocked, just not what its own author flagged
as next. Phase 2: `ROADMAP.md`'s own stated sequencing principle is
"Phase 1 before Phase 2. Dialect abstraction unlocks concurrent dialect
work" (see Phase 2's own header), and Phase 1 still has this one real,
concretely-scoped item open, so starting speculative Phase 2 work now
would violate the project's own already-stated ordering, not just be a
matter of taste. `call_out_info()`/`input_to()` picked on this basis:
smallest, most concretely scoped, continues the project's own stated
priority order, and does not preempt the account/login plan's own
explicit self-deferral.

**Docs cleanup, this session, before the driver work.** Moved
`mudlib/{WAND_OF_CREATION_SCOPING,LIBRARY_MUDLIB_PLAN,ACCOUNT_LOGIN_PLAN}.md`
to a new `notes/` folder, `git rm --cached`'d them, added `notes/` to
`.gitignore` (untracked, still on disk). Wrote a small heuristic script
to rewrite every `" -- "` em-dash stand-in (CLAUDE.md's own rule says
"a period, comma, or start a new sentence instead", not a literal
double-hyphen, which reads exactly like the character it was meant to
avoid) into a paired comma, a conjunction-led comma, or a colon
depending on context: paired dashes bracketing an aside became paired
commas, a clause led by a conjunction (so/since/because/...) got a
comma, everything else (the dominant "citation/justification" pattern
throughout this corpus) became a colon. Ran on `STATUS.md` first,
hand-verified all 62 changes via diff, found and fixed one double-colon
collision, then applied the corrected script to `ROADMAP.md` (426),
`COMPARISON.md` (70), `INSTALL.md` (14), `CREDITS.md` (5), and every
`src/*/instruct.md` file with a hit (129 total), spot-checking dozens
more across `ROADMAP.md`. Zero `" -- "` remain in any of them.
`STATUS-ARCHIVE.md` (838 instances, a dated historical log) and source/
test code comments (~2,150 instances, real risk of colliding with an
actual C++ `--` decrement operator) stayed out of scope this pass, per
the user's own explicit choice. Rebuilt and reran the full suite after:
709 tests passing, zero regressions (docs-only).

**`call_out_info()` gated dialect-aware.** Real LDMud's own
`f_call_out_info()` (`call_out.c:805-829`) wraps its own
`get_all_call_outs()` in `privilege_violation(STR_CALL_OUT_INFO,
&const0, sp)`, degrading to the real empty array on denial. Real
FluffOS's own `f_call_out_info()` (`efuns_main.c:292`ff) has no such
gate at all, confirmed directly (FluffOS never had a
`privilege_violation()` mechanism at all). This driver's own
`call_out_info()` already ported the real FluffOS shape unconditionally,
so the fix is dialect-branched (`vm.config().dialect() == "ldmud"`),
not unconditional the way `bind_lambda()`/`set_driver_hook()` could be:
only under `dialect: ldmud` is the new gate consulted at all; under
FluffOS it stays exactly as before.

**`input_to()` gated on the real `INPUT_IGNORE_BANG` flag bit
specifically, not dialect.** Real `comm.c:7315-7317`'s own `"(flags &
IGNORE_BANG) && !privilege_violation4(STR_INPUT_TO,
svalue_object(command_giver), 0, flags, sp)"`, resolved via real
`privilege_violation4()`'s own "whom && !how_str" branch
(`interpret.c:8578-8621`) to `master->privilege_violation("input_to",
current_object, command_giver, flags)`. Real `INPUT_IGNORE_BANG` is bit
128 (`mudlib/sys/input_to.h`). Confirmed real FluffOS's own `input_to()`
flag bits (`I_NOECHO`=0x1, `I_NOESC`=0x2, `I_SINGLE_CHAR`=0x4, get_char
only, `comm.h`) never define that bit at all, so gating on it
unconditionally, regardless of dialect, carries the same "no FluffOS
equivalent, no conflict risk" safety already established for
`bind_lambda()`/`set_driver_hook()`, confirmed by grep before relying on
it rather than assumed.

**6 new regression tests** (`test_lexer.cpp`): `call_out_info()` denied
and granted under `dialect: ldmud`, plus a FluffOS-dialect regression
proving it stays fully ungated even against an actively denying master;
`input_to()` denied and granted with the bang flag set, plus a control
test proving a flags value that omits the bit never consults
`privilege_violation()` at all even against a denying master.

**Verified live against the real running driver, real bundled
`mudlib/`** (two scratch configs on spare ports 4141/4142, one
`dialect: ldmud` one default FluffOS, a real Python TCP client, a
temporary toggleable `privilege_violation()` lfun appended to the real
bundled `/single/master.c`, reverted via `git checkout` immediately
after, confirmed clean via `git diff --stat`): under `dialect: ldmud`, a
real pending `call_out()` produced the real empty array on denial and
the real one-entry array on grant; `input_to()` returned real `0` on
denial and real `1` on grant with the bang flag set, and the granted
registration incidentally proved itself live end to end beyond the
simple return-code check: the very next line sent over the same
connection was genuinely intercepted by the newly-registered handler
instead of being dispatched as an ordinary command, confirmed by the
connection closing when that handler's own undefined target function
was invoked, the real per-connection error-isolation behavior
(`Server.cpp`) rather than a driver crash. A flags value omitting the
bang bit still returned real `1` even with the master denying
everything, confirming the gate is genuinely conditional on the flag,
live, not just in the unit tests. Under the default FluffOS-dialect
driver, `call_out_info()` still returned the real one-entry array even
with the exact same master actively set to deny, confirmed ungated.
Both scratch processes stopped, `mudlib/single/master.c` reverted,
confirmed clean via `git diff --stat`.

`ROADMAP.md` row 1.7 updated in place with this session's own citations
and live-verification account, appended after the prior update rather
than rewritten. 715 tests passing (up from 709), zero regressions.

Staged with `git add` only, per this project's own standing rule; not
committed.

**2026-08-20 (a further session): `privilege_violation()`'s real cold-start
scoping investigation, deferred multiple sessions ago per row 1.7/1.8's own
long-standing note: real doc catalog cross-checked against the vendored
LDMud source, real corpus usage checked across every vendored corpus, and
the first two real, evidence-justified trigger points wired: `bind_lambda()`'s
cross-object form and `set_driver_hook()` (709 tests, up from 704).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules: `git add` only, no commits/pushes;
no em dashes or emojis).

**Real doc read first, then cross-checked against the real source rather
than trusted alone.** `temp/ldmud/doc/master/privilege_violation` catalogs
26 named operations. Grepped every real `privilege_violation`/`_2`/`_4`/
`_n` call site across `temp/ldmud/src` (`interpret.c:8492-8722` for the
four wrapper functions themselves, `string_spec` for the real `STR_*` op
name catalog) and found the doc stale in two places for this exact
vendored 3.6.8 clone: `enable_telnet` and `set_limits` are both still-
defined string constants with zero real call sites anywhere (superseded
by the newer unified `configure_interactive`/`configure_driver`
mechanism: confirmed via `doc/efun/configure_interactive`'s own
`IC_MAX_COMMANDS` case, which covers what `set_max_commands` used to
gate); `set_driver_hook`'s own doc text implies two extra args (hook
number and the value being set) but the real call site
(`simulate.c:5091`, its own function header comment at `:5070`, "Raises
a privilege violation (\"set_driver_hook\", this_object, what)") only
ever passes one, the hook number.

**Also checked, and corrected, this row's own prior framing of which
operations are gated at all.** `shadow()`'s real LDMud gate is the
separate `query_allow_shadow()` master apply (row 1.5's own already-
implemented mechanism), not `privilege_violation()`; `clone_object()` has
zero `privilege_violation` hits anywhere in the real source; plain file
access beyond `valid_read`/`valid_write` uses those same separate master
applies, no overlap; `snoop()`'s real gate is `valid_snoop()`
(`comm.c:4465`), a different master apply entirely. None of these four
are real `privilege_violation()` trigger points, despite being named as
candidates in this session's own starting framing: confirmed directly
from the C rather than assumed from the framing being correct.

**Real corpus check across every vendored corpus in `temp/`** (extracted
trees and the zipped/tarred archives alike, searched via stdout
extraction without writing new files to disk): `privilege_violation`
itself has zero real hits in every FluffOS-family corpus checked
(`dead-souls`, `es2_mudlib`, `lima`, `nightmare3`, `mudlib`, `lil`,
`wiz_tools`, and all 8 zipped FluffOS-family archives), expected, since
real FluffOS has no `privilege_violation()` mechanism at all (zero hits
in the vendored `fluffos-2.9-ds2.08` reference tree). Exactly one real
corpus has a real `privilege_violation()` at all: `core-lib` (the one
confirmed LDMud-targeting corpus vendored here),
`secure/master/security.c:216-241`, a real handler explicitly granting
`call_out_info`/`mysql` unconditionally, checking `nomask simul_efun`
against a hardcoded path, restricting `input_to` to non-login callers,
and falling back to a generic privileged-object check for everything
else, real, if narrow, evidence a real LDMud-targeting mudlib does rely
on this mechanism, correcting this row's own prior "no corpus signal of
its own by nature" framing (reasonable before this investigation, not
wrong at the time it was written). `__PACKAGE_UIDS__` (named in this
session's own framing as background context, not the actual target) was
corpus-checked too: every hit across all 8 zipped FluffOS-family
archives traces back to the same bundled `fluffos-2.23-ds03/testsuite/`
files already vendored separately as `temp/lil`, one real occurrence
duplicated by the archives bundling a full driver tree, not seven-times-
eight independent real ones. The one hit outside that duplication,
`temp/lima/lib/secure/check_config.c:65-67`, actively requires
`__PACKAGE_UIDS__` to be undefined, real evidence at least one real
corpus deliberately rejects the mechanism, not evidence anything needs it
built.

**Real semantics confirmed directly from `interpret.c:8492-8722`.** Trust
bypass (`:8552-8553`, identical across all four wrapper functions):
`current_object == master_ob` or `== simul_efun_object` grants
immediately, no apply at all. Result contract (`:8570-8578`): a positive
return grants; a missing lfun, a non-number return, or a negative number
all raise a hard error; exactly 0 is a real, valid "gently denied"
answer, not an error. This driver's own `Value{}` (returned by
`callFunction()` for both "no such function" and "function returned
nothing") cannot distinguish those two real cases on its own, so the new
`VM::privilegeViolation()` checks `functionExists()` explicitly first to
preserve the real distinction rather than losing it to that collapse.

**Bounded first slice, scoped to what real evidence and this row's own
already-on-record deferrals actually justify.** New shared
`VM::privilegeViolation(what, args)` (`VM.hpp`/`VM.cpp`) ports the real
four-wrapper-function core in one place. Wired into the two real trigger
points this row's own history had already named and explicitly deferred
pending exactly this investigation: `bind_lambda()`'s cross-object form
(`EfunTable.cpp`, real arg shape `(what, current_object, ob)`, a denial
silently returns the closure unharmed, still unbound) and
`set_driver_hook()` (`EfunTable.cpp`, real arg shape `(what,
current_object, hook_number)` only, checked after the pre-existing range
check per real code's own ordering, a denial silently leaves the hook
unchanged). Both have no FluffOS equivalent at all (confirmed by grep,
zero hits in the vendored FluffOS tree for either efun), so gating them
unconditionally carries no dialect-conflict risk.

**Deliberately not wired this session, despite real corpus evidence,**
and recorded precisely rather than left vague: `call_out_info()` and
`input_to()`, the two ops core-lib's own handler actually branches on.
This driver's own `call_out_info()` efun already ports real *FluffOS's*
`get_all_call_outs()` shape (dialect-neutral, registered
unconditionally), not LDMud's separate `f_call_out_info()`
(`call_out.c:805-829`, which additionally gates on `privilege_violation()`
and degrades to an empty array on denial), gating the existing
implementation unconditionally would incorrectly reject the default
FluffOS-dialect caller for an operation real FluffOS never gates at all,
a real LDMud-vs-FluffOS disambiguation this efun does not yet have
machinery for. `input_to()`'s own real gate is conditional on the
`IGNORE_BANG` flag specifically (`comm.c:7315-7317`), which this driver's
own `input_to()` already accepts positionally but discards with no
behavior attached, wiring the gate to a flag that currently does
nothing would be cosmetic, not a real port. Both are precisely scoped as
follow-ons in ROADMAP.md row 1.7's own cell rather than rushed in. The
remaining ~20 doc-cataloged ops are correctly left ungated: several
correspond to efuns/packages this driver does not implement at all
(mysql/pgsql/sqlite, the erq demon, wizlist), and none has any real
corpus evidence beyond core-lib's own generic default-case fallback.

**5 new regression tests** (`test_lexer.cpp`): a denied cross-object
`bind_lambda()` returns the closure unharmed, still unbound; a granted
one genuinely rebinds (`owner`/`unboundUntilBound` inspected directly,
not through `funcall()`: a nested `#'name` inside an `unbound_lambda()`'s
own quoted body is a separately pre-built `CLOSURE_LFUN` pinned to
whichever object's source literally wrote it, confirmed while writing
this test, not something the outer wrapper's own rebind would ever move);
a master with no `privilege_violation()` lfun at all hard-errors on a
cross-object bind attempt; the master object itself bypasses the check
entirely with no lfun needed (trust bypass); a denied `set_driver_hook()`
silently leaves the hook unchanged. Fixed 2 pre-existing tests whose own
harness never boots a real master object (`master_file: /unused`, never
loaded) that now correctly need one, matching the established
`loadMasterObject()` + permissive-`/unused.c` pattern the shadow tests
already use.

**Verified live against the real running driver, real bundled `mudlib/`**
(a scratch config on spare port 4134, `dialect: ldmud`, a real Python TCP
client, a temporary permissive `privilege_violation()` lfun and one
trust-bypass probe function appended to the real bundled
`/single/master.c`: reverted via `git checkout` immediately after,
confirmed clean via `git status`; three temporary scratch mudlib objects,
removed afterward): a real cross-object `bind_lambda()` grant genuinely
rebound the closure to the target object, confirmed by destructing that
object afterward and getting the real "owner of function pointer is
destructed" error rather than "Uncallable closure" (the tell for "never
actually bound"); a real `set_driver_hook()` denial from a non-master
caller silently left the hook unset, confirmed via a subsequent
`move_object()` still using the plain hardcoded fallback; and the real
trust bypass let the master install `H_MOVE_OBJECT0` on itself despite
its own `privilege_violation()` denying "set_driver_hook" for everyone
else, with the hook then genuinely firing for a different, non-master
object's own `move_object()` call afterward.

709 tests passing (up from 704), zero regressions.

Staged with `git add` only, per this project's own standing rule; not
committed.

**2026-08-20 (a further session): built row 0.13a's own last remaining
Phase 0 sub-gap, `parse_sentence()`'s 4th `nicks` argument: real
`add_nicknames()`/`expand_node()`, already concretely scoped from a
prior session's own citations, re-confirmed against the real source
before building anything (704 tests, up from 702).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules: `git add` only, no commits/pushes;
no em dashes or emojis).

**Re-confirmed the three cited real source sections before trusting
them.** `add_nicknames()` (`packages/parser.c:1095-1108`) confirmed
exactly as the prior session's own note had it, plus one detail the
note left implicit: `mn->values[0]` is the mapping node's own KEY slot,
not a value column, confirmed against real `mapping.h`'s own
`mapping_node_t::values[2]` and `mapping.c:39`'s own
`MAP_SVAL_HASH(mn->values[0])` (a node is hashed by its own key, the
only way lookup works at all), settling "for string keys" as
unambiguous. `expand_node()` re-read in full: the real function is
`:1302-1323`, four lines longer than the prior session's own `:1302-1319`
citation (the extra lines are the function's own closing brace, not
missed logic). Two real quirks confirmed directly and ported faithfully
rather than smoothed over: the `he->flags &= ~HV_NICKNAME;` clear is
unconditional, the function's own first statement, before any success/
failure check, so a *failed* resolution permanently disables
re-attempting that same hash entry for the rest of the call, exactly
like a successful one, not just an optimization for the happy path; and
real `find_string_in_mapping()` (`mapping.c:830-848`) never returns
null, it returns a static `&const0u` on a missing key, so real code's
own single `sv->type != T_OBJECT` check already covers "key missing"
and "key present but not an object" together, ported here as the two
real cases it actually is since this driver's own `Mapping` has no
equivalent sentinel to reuse.

**What was built.** `LoadedObjectSet::loadObjects()`/
`ParserPackage::parseSentence()` both gained a `const Value* nicks =
nullptr` parameter, matching `envArray`'s own already-established shape
exactly. New `addNicknames()` (`ParserPackage.cpp`) ports
`add_nicknames()` at the real call site inside `loadObjects()`, right
after the fixed `"my"` adjective entry, before the `"num_people"` loop,
matching real `parser.c:1162-1163` exactly. New `expandNode()` ports
`expand_node()`, called from `parseObj()`'s own word loop at the real
position, right before the `isNoun` check gets a chance to read
whatever it may have just set. `SentenceSession` gained a `nicks`
field, same per-call-scoped shape as `envArray`, confirmed this is
not a coincidental simplification but the real observable contract:
real `free_parse_globals()` (`parser.c:621-639`) explicitly resets
`parse_nicks = 0;` after every single `parse_sentence()` call, so real
code's own end-to-end behavior already is "fresh per call, nothing
leaks across calls." Also confirmed `parse_my_rules()` never resolves a
nickname in real code either (no `nicks` argument of its own, and
`parse_nicks` is always 0 by the time it would run): this driver's own
`parseMyRules()` needed no change at all. `EfunTable.cpp`'s
`parse_sentence` registration now extracts and passes the real 4th
argument. One incidental stale comment fixed along the way:
`addHashEntry()`'s own header comment cited `mark_hash_entry()` as its
only other real caller; confirmed by grepping the whole vendored driver
tree that `mark_hash_entry()` (`packages/parser.c:1015-1037`) is itself
real dead code in FluffOS (declared, defined, zero call sites anywhere)
-- the same category as `get_bb_uid()`/`multiple_adj()`/`err_obs()`
already found dead elsewhere in this row's own investigation, corrected
rather than left implying otherwise.

2 new regression tests (`test/test_lexer.cpp`):
`testParseSentenceNicknameResolvesToAnAlreadyLoadedObject` (a nickname
word that is not any real noun id of the target object at all resolves
correctly when the object is reachable) and
`testParseSentenceNicknamePresentButObjectNotYetLoadedDoesNotResolve`
(the identical nickname mapping to the identical real object, sitting
in an unrelated, unreachable room: correctly does not resolve,
silently). 704 tests
passing (up from 702), zero regressions.

**Verified live against the real running driver, real bundled
`mudlib/`** (a scratch config on spare port 4132, a real Python TCP
client, four temporary scratch objects, removed afterward, no config
or master changes): a real `eval`-driven `parse_sentence("get sam", 0,
0, (["sam": widget]))`, where `"sam"` is not `widget`'s own real noun id
at all, correctly resolved to the real `widget` object when it was in
the same room as the player (`r == 1`, `probe() == widget`); the
identical nickname mapping to the identical real object, moved to a
second, unreachable room, correctly did not resolve (`r == 0`, `probe()`
stayed its own default `0`); and, confirming backward compatibility, the
same player with no nicks argument still correctly failed to resolve
`"sam"` while still correctly resolving `"get widget"` via its own real
noun id. One real, self-inflicted test-authoring mistake caught and
correctly diagnosed, not a driver bug: an early attempt called bare
`parse_sentence()` directly from inside the `eval` body itself, whose
own scratch object was never `parse_init()`'d, threw the real,
correct "is not known by the parser" error, an uncaught dispatch error
that correctly dropped only that one triggering connection, confirmed
via the driver's own log and a follow-up `eval return 500+1;` on a fresh
connection returning `501` immediately after. Scratch object files
removed before stopping the scratch process, confirmed via `git status`
that `mudlib/` is exactly as found.

**Result: Phase 0 is now genuinely, fully complete**: all 16 rows,
including 0.13a's own last remaining sub-gap, real and tested. `ROADMAP.md`
row 0.13a and `COMPARISON.md`'s own Phase 0/`parse_*` sections updated to
match.

**2026-08-20 (a further session): audited the prior session's own two
real-reset/clean_up fixes for independent regression coverage (found and
fixed a real gap, 2 tests were not one apiece), then produced a full
Phase 1 status read: five stale checkboxes corrected (rows 1.2, 1.3,
1.4, 1.9, 1.16), a plain "real, corpus-driven work is substantially
exhausted" statement added to ROADMAP.md, COMPARISON.md's Phase 1
numbers refreshed from 5/11 to 10/11, and an evidence-based next-session
recommendation produced without building anything this session
(702 tests, up from 701).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules: `git add` only, no commits/pushes;
no em dashes or emojis).

**Test-coverage audit.** The prior session's own two real fixes (the
same-cycle reset/clean_up dialect gate, and the readyForCleanUp-latched-
before-reset ordering fix it exposed) landed with only 2 new tests, one
per dialect (LDMud/FluffOS), not one per fix. Rather than assume that was
fine or assume it was a gap, tested it directly: reverted each fix in
isolation (keeping the other applied), rebuilt, and ran the suite each
time. Result: reverting the ordering fix alone, or the dialect gate
alone, each independently flips the FluffOS-dialect test's own
`cleanUpCalls` assertion from 1 to 0, so that one test *does*
independently catch either regression, confirmed by direct experiment,
not assumed. The LDMud-dialect test does not catch either regression in
isolation (it asserts `cleanUpCalls == 0`, which is also what both
reverted states produce under `dialect: ldmud`, since LDMud's own real
behavior wants suppression regardless of which mechanism, correct or
broken, currently produces it), also confirmed by the same
experiment. This is not accidental undercoverage: a real reset() call
can only ever move `timeOfRef()` forward, never backward, so the
ordering fix has no observable effect except in exactly the same
same-cycle-collision-under-a-non-suppressing-dialect window the dialect
gate also governs; there is no black-box scenario where one fix's
reversion is visible and the other's is not. Documented this finding
directly in both existing tests' own comments (previously written under
an unverified assumption, now corrected to state what was actually
proven). Added one further test, `dialect: dgd`, closing a real, distinct
adjacent gap this same audit surfaced: neither existing test would catch
a plausible future "simplification" of the dialect check from an
allowlist (`dialect == LpcDialect::LdMud`) to a denylist (`dialect !=
LpcDialect::FluffOS`): DGD is the one dialect where the two phrasings
disagree. 702 tests passing (up from 701), zero regressions, restored
code confirmed byte-identical to the pre-experiment version via `diff`
before rebuilding for real.

**Phase 1 status read.** Surveyed every Phase 1 row's own current cell
text against its checkbox, not just the checkbox alone, continuing the
same discipline as the immediately prior session's own row 1.9
correction. Found four more stale checkboxes, each with real, already-
landed work sitting in its own cell across earlier sessions:

- Row 1.2/1.3 (dialect-aware Lexer/Parser): `atomic`/`nil` (DGD-gated),
  bare `#'name`/`'name`, and `#'efun::name` are all real and tested,
  recorded directly in these rows' own cells across several earlier
  sessions; mapping-width syntax was explicitly absorbed into row 1.9's
  own scope, not split across rows. Remaining real scope is `rlimits`
  (both rows) and DGD's own `&ident(args)` closure syntax (row 1.3) --
  both DGD-only, out of this project's own explicit Phase 1 completion
  scope. Checkboxes corrected to `[x]`.
- Row 1.16 (LDMud master apply name table): re-read its own below-table
  prose in full (the row's own cell ends mid-sentence, "recorded below
  rather than forced," referring to that prose, not a truncated cell).
  `get_bb_uid` is confirmed dead code in this exact vendored LDMud build
  itself (zero real driver call sites, a doc-vs-code divergence);
  `make_path_absolute` is blocked on `ed()`, an already-and-separately-
  excluded efun; `valid_read`/`valid_write` are real and tested;
  `disconnect()` is zero real corpus usage. No real open item remains.
  Checkbox corrected to `[x]`.
- Row 1.4 (pluggable boot API): re-checked whether `Server.cpp`'s
  hardcoded `"connect"` master apply is genuinely FluffOS-specific
  (which would make the "not routed through `BootApi`" note a real
  functional gap, not just hygiene) rather than assuming the prior
  framing was complete, confirmed both real dialects use the identical
  apply name and role (`temp/reference/fluffos-2.9-ds2.08/applies_table.c`'s
  own `"connect"`, real LDMud's own `doc/master/connect`, `"object
  connect(void)"`). An initial check of this returned zero results for
  both dialects and nearly got reported as a real, newly-discovered
  functional gap, caught before trusting it: the grep had been run
  from `build/`, not the repo root, so `temp/ldmud/...` silently did not
  exist under that working directory. Re-run from the correct directory,
  confirmed the apply names genuinely match. So the un-routed applies
  are exactly what the row's own prior note already said, an
  abstraction-hygiene gap, not a functional one. Checkbox corrected to
  `[x]`.

**Refreshed numbers.** `ROADMAP.md`'s own Phase 1 header gained a plain
"real, corpus-driven work is substantially exhausted" status statement:
10 of 11 real-blocker rows closed (DGD-only 1.11-1.15 excluded per this
project's own stated goal), the one remaining open row (1.8) confirmed
zero-evidence for its own remaining scope this same session (see its own
2026-08-20 entry above). `COMPARISON.md`'s Phase 1 section rewritten from
its own stale "5 of 11, a bit under half done" (predating several
sessions' worth of already-landed work) to the current 10/11 (91%)/
16-row 10/16 (63%) numbers, with a rewritten "what is left open, and why"
bullet list replacing the old, now-inaccurate one.

**Next-session recommendation, not built this session (see the reply to
the user this same turn for the full reasoning): return to Phase 0's own
one remaining open item, row 0.13a's `parse_sentence()` `nicks` argument,
over continuing to close Phase 1's zero-evidence items defensively or
starting Phase 2 planning-to-code work.** Full reasoning given directly
to the user this turn, not duplicated here: see this same session's
own reply for the three-way comparison and why `nicks` won.

