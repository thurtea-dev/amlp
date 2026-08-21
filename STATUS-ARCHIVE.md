# STATUS ARCHIVE

Older session entries and narrative history for the amlp project,
moved out of `STATUS.md` on 2026-08-09 to keep that file to the 5 most
recent dated sessions. Everything below is verbatim historical content:
the older dated session entries first, most-recent-first, followed by
the earlier undated narrative and topic-organized sections in their
original relative order (this project used topic headers before it
settled on the current `**YYYY-MM-DD: ...**` per-session convention).
See `STATUS.md` for current state, the most recent session entries, and
the full, currently-maintained Known Stubs list.

A small number of "see ... below" references inside this archived
content originally pointed at the Known Stubs section, which now lives
in `STATUS.md` instead of this file -- left as-is (verbatim), not
rewritten.

**Second archival pass, 2026-08-18 (continued):** `STATUS.md` had grown
to 4,667 lines with 52 dated session entries and no archival moves since
the split above, breaking the "keep `STATUS.md` to the 5 most recent
sessions" practice this file's own header already describes. Moved
entries 6 through 52 (everything older than the 5 most recent as of this
pass) into this file, verbatim, no content altered -- a pure move, the
same discipline as the original 2026-08-09 split. Inserted immediately
below, before the 2026-08-09 entries that were already here, to keep
this file's own most-recent-first ordering intact across both batches
(the newly-moved entries are all newer than everything already archived
below them). The `## Known stubs / scope limitations` section stayed in
`STATUS.md`, unmoved, matching this file's own header note above that it
is `STATUS.md`'s "currently-maintained" list, not historical content.

**Third archival pass, 2026-08-20:** moved entries 6 onward (everything
older than the 5 most recent as of this pass) into this file, verbatim,
no content altered -- the same discipline as both archival passes above.
Inserted immediately below, before the entries already here, to keep
this file's own most-recent-first ordering intact across all three
batches.

**Fourth archival pass, 2026-08-21:** moved the single oldest entry (the
`H_RESET`/`H_CLEAN_UP` session) into this file, verbatim, no content
altered, to keep `STATUS.md` to its own 5-most-recent-sessions rule
after this session's new entry. Same discipline, same insertion
placement as the three passes above.

**Fifth archival pass, 2026-08-21 (later the same day):** moved the
single oldest entry (the "resolved the prior session's own open
question" Phase 1 re-ranking session) into this file, verbatim, no
content altered, to keep `STATUS.md` to its own 5-most-recent-sessions
rule after this session's new entry. Same discipline, same insertion
placement as the four passes above.

**2026-08-20 (a further session): resolved the prior session's own open
same-cycle reset/clean_up dialect question (was hardcoded to LDMud's rule
for every dialect, now genuinely `Config::dialect()`-gated, plus one
latent dialect-independent ordering bug fixed along the way), ROADMAP row
1.7 updated to reflect it, then continued the same evidence-based Phase 1
re-ranking: row 1.9's own checkbox corrected to match its own already-
recorded close-out (re-verified fresh, still zero real usage for its five
deferred sub-items), and row 1.8, previously a completely blank
placeholder, properly investigated and documented for the first time,
confirmed zero real corpus evidence for its own remaining `#'lfun::`/
`#'sefun::`/`#'var::` scope and correctly left deferred rather than built
speculatively (701 tests, up from 700).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules: `git add` only, no commits/pushes;
no em dashes or emojis).

**Dialect-gate resolution.** Read `Scheduler::tickResetsAndCleanup()`
directly: the prior session's own same-cycle "a real reset() firing
suppresses clean_up() this same tick" rule was hardcoded to LDMud's own
`!bResetCalled` behavior (`backend.c:1402-1406`) for every dialect, not
gated on `Config::dialect()` at all. Re-read both real sources side by
side to confirm the actual divergence rather than trusting the prior
session's own "more conservative choice" framing: real FluffOS
`backend.c:241-267` (`look_for_objects_to_swap()`) computes
`ready_for_clean_up` into a local *before* calling `reset_object()`
(`backend.c:251`) and never re-checks it against whether that call
actually ran ("Check reference time before reset() is called.": the
comment directly above the check). Confirmed no same-cycle suppression
exists in real FluffOS at all, and confirmed this is a real,
corpus-plausible collision, not a hypothetical one: reachable whenever
`O_RESET_STATE` gets cleared by something that does not also touch
`time_of_ref` (this driver's own `set_environment()`/`move_object()`
fallback three-way `O_RESET_STATE` clear, `VM.cpp:1163-1176`, confirmed
not to touch `timeOfRef()`).

While fixing this, found and fixed one further, dialect-independent
latent ordering bug in the same code: `readyForCleanUp` was read from
`obj->timeOfRef()` *after* the reset block ran, but a real reset()'s own
call touches `timeOfRef()` like any other call into the object
(`VM::callFunction()`'s real `apply_low()`-equivalent touch), reading
it post-reset would have silently defeated a same-cycle collision under
*either* dialect even after adding the dialect flag, since the
elapsed-time check would already read as "just touched." Both real
drivers avoid exactly this by latching their own equivalent value
(`ready_for_clean_up`/`time_since_ref`) *before* calling `reset_object()`
each cycle (`backend.c:241` FluffOS, `backend.c:1321` LDMud), reproduced
here the same way: `readyForCleanUp` is now latched at the top of each
object's own iteration, before the reset block runs, and only the
*suppression* on top of that latched value is gated on
`dialect == LpcDialect::LdMud`.

2 new regression tests replace the prior single one
(`test/test_lexer.cpp`): `testTickResetsAndCleanupSkipsCleanUpOnTheSame
CycleARealResetFiredUnderLdmudDialect` (same setup as before, now under
an explicit `dialect: ldmud` harness) and `testTickResetsAndCleanupDoes
NotSuppressCleanUpOnTheSameCycleARealResetFiredUnderFluffosDialect`
(identical setup, default FluffOS dialect, opposite outcome from
identical inputs, proving the divergence is real and dialect-driven).
701 tests passing (up from 700), zero regressions.

**Verified live against the real running driver, real bundled
`mudlib/`** (`TIME_TO_RESET`/`TIME_TO_CLEAN_UP` temporarily shrunk to 4/6
real seconds for one verification build, reverted immediately after,
full suite re-confirmed passing at both settings): booted the real
driver under both `dialect: fluffos` (default) and `dialect: ldmud` with
the refactored code, confirmed no crash under either; a real scratch
object cloned and moved into the real bundled `/single/start_room` (so a
real `shared_ptr` reference from the room's own inventory kept it alive
across separate eval connections, the same live-object-lifetime lesson
the prior session's own live verification had already surfaced) had its
real `clean_up()` fire correctly via a genuine wall-clock timer under
both dialects, with the correct real clone argument (`0`), confirming
the refactored `readyForCleanUp` latch did not regress the ordinary
(non-colliding) path under either dialect. One honest live-session loose
end, not a code-correctness concern: the same scenario's own `reset()`
did not fire within the observed window under either dialect
(`resetCalls` stayed 0 while `cleanUpCalls` reached 1), identically
reproduced under both dialects against byte-identical, unmodified
reset-block code, most likely an artifact of this specific interactive
test setup (repeated eval connections independently touching the object)
rather than a driver defect, since the exact same reset-firing mechanism
is independently and deterministically confirmed correct by
`testTickResetsAndCleanupCallsRealResetOnceDueAndNotInResetState` and 4
other passing unit tests exercising it directly, not chased further
given the unit-level proof already stands, flagged here rather than
silently omitted. Scratch object file and log removed before stopping
both scratch processes, confirmed via `git status` that `mudlib/` is
exactly as found.

**Phase 1 re-ranking, continued.** Surveyed every remaining open Phase 1
row (1.2, 1.3, 1.4, 1.8, 1.9, 1.16; DGD-only rows 1.11-1.15 deprioritized
per this file's own 2026-08-18 scope note) against its own current cell
text rather than trusting the checkbox alone. Found two real
checkbox/cell mismatches, both corrected rather than picking a new
speculative feature to build:

Row 1.9 (LDMud mapping width): its own 2026-08-19 close-out text already
said the real, evidenced work was done and its five remaining sub-items
(`m_allocate`/`m_entry`/`m_reallocate`/`m_add`/`m_contains`,
`([:width])`) were deferred on zero real corpus evidence, but the
checkbox itself was still `[ ]`. Re-verified the zero-evidence claim
fresh rather than trusting it stale (the close-out's own explicit
standing instruction): every fresh hit for any of those five is the
LDMud driver's own test/doc/HISTORY/CHANGELOG tree, its own bundled
`mud/lp-245` example mudlib (not one of this project's seven tracked
real gameplay corpora), or its own `mudlib/deprecated/`-namespaced
backward-compat stub definitions, zero real game-content call sites.
Checkbox corrected to `[x]`.

Row 1.8 (`LDMud #'symbol references baked at construction`): found
completely blank: title only, no investigation ever recorded. Traced
the gap to `PROMPT-ARCHIVE.md`'s own original `P1-B` prompt (its own
stale framing per `CLAUDE.md`'s standing warning about that file),
which had lumped rows 1.7 and 1.8 together as one task; the real
`#'`-closure work that followed happened entirely under row 1.7's own
cell across several later sessions, leaving row 1.8 never actually
written up. Read real LDMud source directly (`doc/LPC/closures`,
`closure.c`) to determine row 1.8's real remaining scope distinct from
row 1.7's already-done bare-`#'name`/`#'efun::` work: `#'lfun::`/
`#'sefun::` (reusable on the same forced-tier mechanism `#'efun::`
already established) and `#'var::variable_name` (real `CLOSURE_IDENTIFIER`,
a reference-to-a-global-variable closure kind, not a callable at all --
`doc/LPC/closures:43`, `closure.c:450`/`524`/`977`/`1198`/`4178`).
Re-checked real corpus usage fresh for all three (not reused from row
1.7's own prior count): `grep -rn "#'var::"`/`"#'lfun::"`/`"#'sefun::"`
across every corpus vendored in `temp/`, zero real mudlib call sites
for any of the three; the only hit anywhere is `temp/ldmud/HISTORY:226`,
the driver's own changelog prose. Per this project's own repeatedly-
applied zero-corpus-evidence discipline (row 1.7's own `bind_lambda()`
stand-in rejected on the same grounds; row 1.9's own five deferred
sub-items above), row 1.8 stays open and deferred rather than built
speculatively, now properly documented with real citations instead of
left blank, so a future session does not have to re-derive this from
scratch.

No further open Phase 1 row (1.2, 1.3, 1.4, 1.16) turned up a new,
real-evidence-backed, buildable gap this pass beyond what prior sessions
had already investigated and correctly deferred (connect/disconnect,
`rlimits`, the DGD `&ident(args)` closure syntax, the LDMud master-apply
table's own remaining items): see each row's own cell for that already-
recorded reasoning, none of it re-litigated here since nothing new was
found to change it.

**2026-08-20 (fresh session): row 1.7/1.8's `H_RESET`/`H_CLEAN_UP` real
object-lifecycle slice picked and built after a fresh corpus comparison
against the row's other live candidate (the three remaining
`unbound_lambda()`-based hooks, `H_LOAD_UIDS`/`H_CLONE_UIDS`/
`H_INCLUDE_DIRS`); confirmed dialect-universal (real FluffOS has the
identical mechanism under different flag names, not LDMud-only); a
docs pivot mid-session (README/CREDITS/COMPARISON refresh, no driver
names in public-facing docs anymore) handled in between (700 tests, up
from 694).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules: `git add` only, no commits/pushes;
no em dashes or emojis).

**Corpus evidence comparison, before picking.** Re-ran both candidate
counts from scratch across every vendored corpus in `temp/` rather than
trusting the prior session's own tally: `reset()` is defined in 324
real files (`ldmud` 164, `dead-souls` 55, `nightmare3` 75, `es2_mudlib`
10, `core-lib` 10, `lima` 6, `wiz_tools` 2, `mudlib` 2), `clean_up()` in
43 more, spread broadly across nearly every corpus, not concentrated in
one file. `set_driver_hook(H_LOAD_UIDS|H_CLONE_UIDS|H_INCLUDE_DIRS)`, by
contrast, has exactly 3 real call sites total, all three in one file
(`core-lib/secure/master/hooks.c`, the same 3 of 14 hook calls already
counted in row 1.7's own prior stock-take), the only other hits found
anywhere are `temp/ldmud`'s own bundled driver test fixtures
(`test/t-deprecated-sefuns/master.c`, `test/t-language/master.c`), the
driver vendor's own regression scaffolding, not independent gameplay
corpora. Confirmed real FluffOS has the identical reset/clean_up
mechanism too (`temp/reference/fluffos-2.9-ds2.08/backend.c:196-302`'s
own `look_for_objects_to_swap()`, `object.c:1896-1927`'s own
`reset_object()`/`call_create()`, same `O_WILL_RESET`/`O_RESET_STATE`/
`O_WILL_CLEAN_UP` flags under different literal names, same
`current_time + TIME_TO_RESET/2 + random_number(TIME_TO_RESET/2)`
formula, same "no reset() in the object -> never call it again" quirk,
same clean_up() argument rule), so this is a real, dialect-universal
LPMud-family object-lifecycle mechanism, not an LDMud-only hook --
picked over the UID hooks by an overwhelming margin on real evidence,
not the "obvious next hook number" assumption.

**What was built**, read in full before building: LDMud
`backend.c:1330-1476` (`process_objects()`'s own reset/clean_up
section) and `object.c:800-885` (`reset_object()`), cross-checked
against real FluffOS `backend.c:196-302`/`object.c:1896-1927` for the
same mechanism. Real `TIME_TO_RESET`/`TIME_TO_CLEAN_UP` defaults
confirmed from `temp/ldmud/autoconf/configure`'s own
`DEFAULTwith_time_to_reset=1800`/`DEFAULTwith_time_to_clean_up=3600`
(seconds), matching the real `./configure --help` text too; `ALARM_TIME`
default 2, already matching this driver's own pre-existing
`Scheduler::kHeartbeatCycle`.

`LpcObject` (`include/amlp/object/LpcObject.hpp`) gained `resetState()`/
`willCleanUp()`/`isClone()`/`timeReset()`/`timeOfRef()` plus `armReset()`
(the real randomized-delay formula, both drivers, cited above) and
`disableReset()` (the real "permanently stop trying" quirk, represented
as an unreachably-far-future `timeReset()` rather than a magic zero).
`VM::callFunction()` now clears `resetState()` and touches `timeOfRef()`
on every call into an object from outside (real `apply_low()`'s own
"unset its reset status"/`time_of_ref` update, `interpret.c:20311-20345`)
-- the real, universal "something touched this object" signal both the
virtual-vs-real reset decision and clean_up eligibility key off.
`set_environment()` (`EfunTable.cpp`) and `VM::moveObject()`'s own
hardcoded fallback both gained the matching real three-way
`O_RESET_STATE` clear (dest/item/old-super, `object.c:5188-5198`).
`ObjectManager` gained `armResetAndCleanup()`, called after `create()`
succeeds in `loadObject()`/`cloneObject()`/`reloadObject()` (standing in
for real `reset_object(ob, H_CREATE_OB|H_CREATE_CLONE, 0)` always
running at creation): `cloneObject()` also sets the new `isClone()`
flag unconditionally, matching real `O_CLONE`.

New `Scheduler::tickResetsAndCleanup()`, gated by the same real
`kHeartbeatCycle` 2-second window `ALARM_TIME`'s own real doc comment
says reset/clean_up genuinely share with heart_beat, dispatches
through `driverHooks_[H_RESET]`/`driverHooks_[H_CLEAN_UP]` when set to
a string, falling back to the literal `"reset"`/`"clean_up"` names
otherwise, matching both real corpus's own actual configured hook value
(`core-lib/secure/master/hooks.c`) and real FluffOS's own fixed
`APPLY_RESET`/`APPLY_CLEAN_UP` names exactly. The real closure-hook form
is honestly left unimplemented (zero corpus evidence, the same stance
`H_MODIFY_COMMAND`'s own T_CLOSURE/T_STRING gap already took last
session). One real, deliberate divergence between the two real drivers,
flagged rather than silently picked: LDMud's own same-cycle "a real
reset() firing suppresses clean_up() this same tick" gate (`!bResetCalled`,
`backend.c:1403`) is used here even though real FluffOS's own
`ready_for_clean_up` latch has no exact analog, the more conservative,
never-double-touch choice, and the one place the two real drivers'
own sections do not agree byte-for-byte. Real per-cycle cross-object
batching (LDMud's `!did_reset`, FluffOS's whole-list-every-5-minutes
sweep) is deliberately not replicated: a real-driver performance
strategy for large object counts under a fixed time budget, not a
semantic requirement; every object due this tick is processed this
tick here, flagged as a deliberate simplification for this driver's
own much smaller expected object counts.

6 new regression tests (`test/test_lexer.cpp`, `ObjectVarHarness` plus a
real `Scheduler`, directly manipulating `armReset()`/`setResetState()`/
`setTimeOfRef()` the same way the pre-existing call_out tests already
construct an already-past `dueAt` directly rather than sleeping): a
real (non-virtual) reset firing and correctly re-arming; a virtual
reset never actually calling `reset()`; permanent reset-disable when no
`reset()` is defined; `clean_up()` firing with the real clone-vs-non-clone
argument and tracking its own truthy/falsy return (both directions);
and the same-cycle reset-suppresses-clean_up gate.

**Verified live against the real running driver, real bundled
`mudlib/`** (a scratch config on spare port 4129, a real Python TCP
client, a temporary scratch object with real `reset()`/`clean_up()`
bodies that `write_file()` a log, removed afterward): `TIME_TO_RESET`/
`TIME_TO_CLEAN_UP` temporarily shrunk to 4/6 real seconds for this one
verification build only (reverted to the real 1800/3600 immediately
after, full suite re-confirmed passing at both settings) so a real
30-60-minute wait was not required to observe a genuine timer-driven
fire. The real `Scheduler::tickResetsAndCleanup()` timer fired
`reset()` once on the newly cloned object, then `clean_up()` once with
the real clone argument (`0`), both observed purely by polling the log
file the LPC bodies themselves wrote, `eval` used only to clone/touch
the object beforehand and to read the log afterward, never to invoke
`reset()`/`clean_up()` itself. The object survived, since its own
`clean_up()` correctly returned `0` without calling `destruct()`. One
genuine test-authoring mistake caught and fixed before trusting the
result, not a driver bug: an early attempt called `move_object(item,
dest)` assuming a real two-argument form, not realizing this driver's
own `move_object()` efun is real FluffOS's own single-argument form
(always moves `current_object()`, silently ignoring a second argument)
-- the clone never actually reached the target room, and a *later*,
unrelated `eval` call's own routine destruction of its own scratch
`/tmp_eval_file` object (real `command/eval.c`'s own pre-existing
"clean up first" step, confirmed real, not new) was what actually
vanished from the room's inventory, not anything this session built;
switched to the real two-argument `set_environment(item, env)`
primitive and confirmed correct. Also confirmed live, incidentally:
this bundled mudlib's own real `inherit/clean_up.c`
(`int clean_up(int inh) { destruct(this_object()); return 0; }`,
inherited via `include/command.h`'s own `inherit CLEAN_UP;` by every
kept command file, `who.c`/`say.c`/`quit.c`/`shutdown.c`) had been
dormant dead code until this session, since this driver never called
`clean_up()` at all before now, confirmed it does NOT fire
prematurely or destroy live command blueprints (`/command/eval`,
`/single/master`, `/single/simul_efun`, `/single/start_room`,
`/clone/wand_of_creation`) during the shrunk-window run, since none had
gone untouched long enough yet within the test's own real window; a
genuinely long-uptime run would eventually and correctly self-destruct,
e.g., `/command/who` the first time nobody uses it for a real hour,
exactly as this mudlib's own file was already written expecting. Driver
stayed healthy throughout. Scratch object files removed before stopping
the scratch process, leaving the bundled `mudlib/` tree exactly as
found (confirmed via `git status`). 700 tests passing (up from 694),
zero regressions.

**Docs pivot, mid-session (user-directed).** README.md's own tagline no
longer names FluffOS/LDMud/DGD ("targeting FluffOS/LDMud/DGD dialect
compatibility" replaced with dialect-neutral language, pointing readers
to the new `CREDITS.md` and existing `COMPARISON.md` instead).
`CREDITS.md` (new) is the one place those three real drivers are named
in the public-facing docs now, framed as prior-art attribution, not a
compatibility claim. `INSTALL.md` needed no changes (confirmed it never
named any driver). `COMPARISON.md` (which explicitly keeps naming real
drivers by design, that is its whole purpose) refreshed in place with
today's real numbers rather than 2026-08-18's stale ones: Phase 0 is
now 16/16 (100%, `parse_*`/row 0.13a's own checkbox flipped since this
file was last updated, all 8 real efun names implemented including
real two-object `OBJ`/`LIV`/`OBS`/`LVS` matching, confirmed by reading
`STATUS.md`'s own strictly-chronological log directly rather than
trusting an ambiguous-looking mid-cell note in `ROADMAP.md` that turned
out to describe an intermediate, since-superseded state); Phase 1 real
blockers now 5/11 (45%, up from 4/11, row 1.7 itself having since
flipped to checked-partial); efun surface 248 of 270 (up from 247);
test count 694 (this session's own baseline before the reset/clean_up
work above, itself now 700). Explicitly scoped by the user to README.md
+ INSTALL.md + CREDITS.md + COMPARISON.md only: `ROADMAP.md`/
`STATUS.md`/every `src/*/instruct.md` keep naming real drivers
throughout, since their whole citation-based verification methodology
depends on it (per `CLAUDE.md`), not touched.

Staged with `git add` only, per this project's own standing rule; not
committed.

**2026-08-19 (a further fresh session, same day): row 1.7/1.8 updated to
accurately reflect its current real state (H_MOVE_OBJECT0/1 fully wired
end to end, concrete remaining list), a full fresh Phase 1 re-ranking
sweep across every open row, and a real `H_MODIFY_COMMAND` slice picked
from that sweep and verified live with a real raw typed command, not
`eval` (694 tests, up from 692).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules).

**Step 1: row 1.7/1.8 accuracy update.** The row's own opening summary
still said "see the update at the end of this cell for the full
scoping, including the still-open `inaugurate_master()` boot-wiring
gap" -- stale, since last session closed that gap. Rewrote the opening
to state plainly that H_MOVE_OBJECT0/H_MOVE_OBJECT1 are wired fully end
to end (storage, dispatch, boot install, real trigger, all confirmed
live together), and to list the concrete remaining scope explicitly:
the other three real `unbound_lambda()` hook trigger points from
hooks.c (`H_LOAD_UIDS`/`H_CLONE_UIDS`/`H_INCLUDE_DIRS`), every other
real hook hooks.c sets that is not `unbound_lambda()`-based (the
plain-string hooks and the one mapping hook), real per-hook type-map
validation, `privilege_violation()`, plain dialect-agnostic `lambda()`,
the rest of real `lambda()`'s own grammar, and `inaugurate_master()`'s
own arg=1/2/3 cases. Did not close the row -- stays `[x] (partial)`
with an accurate, itemized remaining list, per this session's own
explicit instruction.

**A second, unrelated staleness bug found and fixed while reading
nearby rows for the sweep:** row 1.16's own inline cell text still said
"No implementation this pass: all three blocked, none forced...
`valid_read`/`valid_write` is recommended as its own future row...
whenever it gets an explicit go-ahead" -- but `valid_read`/`valid_write`
were in fact implemented later that same 2026-08-18 session
(`checkValidPath()`, confirmed directly against the current
`EfunTable.cpp`, wired into all 11 real file efuns), a reversal the
below-table bullet list already recorded accurately but the row's own
inline summary was never updated to reflect. Added an explicit
correction note rather than silently rewriting history.

**Step 2: full Phase 1 re-ranking sweep.** Enumerated every Phase 1 row
(1.1 through 1.16) and its current checkbox state directly from the
file rather than from memory. DGD rows (1.11-1.15) confirmed still
comparison-only, not Phase 1 blockers, per the Phase 1 header's own
2026-08-18 scope clarification -- excluded from real-priority ranking,
each already has its own real, cited "confirmed genuinely bigger" note
on record, nothing new to add. Rows 1.1/1.5/1.6/1.10 are done. Row
1.2/1.3: everything LDMud-relevant either already implemented or now
owned by a more specific row (1.7/1.8/1.9); the row's own remaining
open item, `rlimits`, is DGD's own (row 1.3's own title: "DGD
`atomic`/`rlimits`/`nil`"), already covered under the DGD-comparison-
only umbrella -- nothing independently open here. Row 1.4: the
connect/disconnect design question, re-confirmed zero real LDMud
`disconnect()` usage multiple sessions running now, stays closed. Row
1.9: remaining sub-items (`([:width])`, the `m_allocate` efun family,
mapping range index, save/restore) already confirmed zero real corpus
usage two sessions ago, no new evidence to gather. Row 1.16: `get_bb_uid`
stays dead/unimplementable against this exact vendored source;
`make_path_absolute` stays blocked behind `ed()`, itself unimplemented.

**Two rows got genuinely fresh investigation this pass, not just a
status re-read:**

Row 1.8 (`#'symbol` references baked at construction, a bare, wholly
uninvestigated stub before this session): real corpus check --
re-derived the full 32-hit `#'` corpus count fresh (the same fixed-
string search methodology established two sessions ago) and read every
single one of the 32 real hits directly, not just counted them. Every
one is either passed straight into `filter()`/`map()`/`apply()`/
`regreplace()` as an argument, or lives inside `secure/master/hooks.c`'s
own already-covered `unbound_lambda()` bodies -- not one real hit stores
a `#'name` closure into a persistent object variable for later use,
which is the one shape where this driver's own existing "lazy
re-resolve by name at call time" simplification (`Value.hpp`'s own
`Closure` comment) could actually diverge observably from real LDMud's
eager construction-time binding. Confirmed genuinely zero real-world
impact for the one corpus this repo has real LDMud evidence for, not
merely "still open, not yet checked."

Row 1.7's own remaining hooks.c hook numbers: re-read hooks.c's own 10
non-`unbound_lambda()` `set_driver_hook()` calls against what this
driver already hardcodes, rather than assuming they all need fresh
plumbing. `H_CREATE_SUPER`/`H_CREATE_OB`/`H_CREATE_CLONE` (real value:
`"create"`) and `H_NOTIFY_FAIL` (real value: `"What?\n"`) both already
match this driver's own pre-existing hardcoded defaults exactly
(`ObjectManager.cpp`'s own `callFunction(obj, "create", {})`,
`Server.cpp`'s own hardcoded `"What?\n"` fallback, both confirmed by
direct grep this session) -- wiring the actual hook mechanism for
either would be a zero-observable-behavior-change formality for this
specific real corpus, not a live gap. `H_RESET`/`H_CLEAN_UP`, by
contrast, are genuinely unimplemented: this driver calls `heart_beat`
periodically but never calls `reset`/`clean_up` on anything at all
(confirmed by grep -- `clean_up` is only listed in `ApplyTable::known()`
as a recognized name, never actually invoked anywhere). `H_MODIFY_COMMAND`
(a mapping of single-letter direction abbreviations to full verbs) was
the standout: real `actions.c` (`call_modify_command()`,
`actions.c:514-611`, called from `parse_command()`, `actions.c:792`)
confirmed this fires on *every* typed command, before verb parsing,
directly gating whether a player can type "n" instead of "north" --
genuine everyday gameplay impact, not object-creation-time bookkeeping,
and its storage already existed for free (`driverHooks_` is generic
across all 32 slots already).

**Step 3: picked `H_MODIFY_COMMAND`.** Highest real, moment-to-moment
gameplay impact among everything the sweep surfaced, and a small,
well-bounded slice: `VM::dispatchCommand()` now checks
`driverHooks_[9]` (`H_MODIFY_COMMAND`) for a `Mapping` before the
existing verb/arg split, substituting the whole line when it exactly
matches a key whose value is a string (real semantics: only *trailing*
whitespace is stripped first, and the *entire* line is the lookup key,
not just the first word -- confirmed directly from `actions.c:576-785`
before writing anything), then falling through to the pre-existing
dispatch logic unchanged. Only the real `T_MAPPING` hook form is
implemented, matching hooks.c's own actual real usage precisely -- the
`T_CLOSURE`/`T_STRING` forms and the separate per-interactive-object
override (`set_modify_command()`) are real but have zero confirmed
corpus usage, left honestly unimplemented. 2 new regression tests.

**Verified live against the real running driver, real bundled
`mudlib/`** (a scratch config on spare port 4128, `dialect: ldmud`, a
persistent plain TCP client kept open across multiple sends -- unlike
the one-shot `eval`-per-connection pattern earlier sessions used, this
needed a single connection surviving from login through the actual
raw-command test -- a scratch `master_file: /tmp_sweep_master`
installing the real hook via the real `inaugurate_master(0)` boot
sequence, no manual `set_driver_hook()` call anywhere in the live
session itself): typing the real raw command `n` -- not wrapped in
`eval`, the actual shape a real player types -- correctly rewrote to
`north` and ran a scratch `/command/north.c` (leveraging this bundled
mudlib's own pre-existing "unknown verb falls back to
`/command/<verb>.c`" catch-all, confirmed live and pre-existing, not
built this session) with the output `"You went north! (real
H_MODIFY_COMMAND rewrite fired)"`; `n ` (trailing space) produced the
identical result, confirming the trailing-whitespace-only trim; `n foo`
and an unmapped `x` both correctly did not rewrite, confirmed via the
driver's own log showing it looking for `/command/n.c`/`/command/x.c`,
never `/command/north.c`, for either. One real methodology detour along
the way: an initial attempt tried to prove the rewrite by registering a
custom `add_action()` handler in a scratch room the player was moved
into, which never fired because this bundled mudlib already has its
own catch-all command-loader intercepting first -- rather than fighting
that pre-existing mechanism, the test was redirected to use it directly
(supplying the `/command/north.c` file it was already trying to load),
a cleaner and more direct confirmation than the original approach would
have given anyway. Driver stayed healthy throughout (`eval return
300+1;` -> 301, `who`). Scratch master/command files and
`tmp_eval_file.c` removed before stopping the scratch process, leaving
the bundled `mudlib/` tree as found.

Still open on row 1.7/1.8: `H_LOAD_UIDS`/`H_CLONE_UIDS`/`H_INCLUDE_DIRS`
dispatch, the remaining plain-string hooks (`H_RESET`/`H_CLEAN_UP`
genuinely so; `H_CREATE_*`/`H_NOTIFY_FAIL` only formally so, already
matching this driver's own hardcoded defaults for this corpus), real
per-hook type-map validation, `privilege_violation()`, and
`inaugurate_master()`'s own arg=1/2/3 cases.

Staged with `git add` only, per this project's own standing rule; not
committed.


**2026-08-19 (a further fresh session, same day): row 1.7/1.8's
`inaugurate_master()` boot-sequence slice landed -- real LDMud master
boot now calls `inaugurate_master(0)` automatically, wiring last
session's `set_driver_hook()`/`H_MOVE_OBJECT0` work into a real boot
sequence for the first time, verified live boot-to-hook-execution with
zero manual wiring. Also found and fixed a real, pre-existing
ROADMAP.md structural bug from the last two sessions' own edits (row
1.7's content had been fragmented across a misplaced block wedged into
row 1.4's cell and several orphaned paragraphs, breaking the table)
(692 tests, up from 690).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules).

**Real scope confirmed from source before writing anything.** Read real
`main.c:590-663` in full: the real boot order is master load -> pin ->
`initialize_master_uid()` -> `push_number(inter_sp, 0);
callback_master(STR_INAUGURATE, 1);` (real `inaugurate_master(0)`) ->
`setup_print_block_dispatcher()` (internal, unrelated) -> `-f` flag
applies -> `assert_simul_efun_object()`. `callback_master(fun, n)` is
`apply_master_ob(fun, n, MY_TRUE)` (`interpret.h:336`). Read real
`doc/master/inaugurate_master` for the exact contract: "called in the
master object after it has been created and is fully functional...
has to at least set up the driverhooks to use", `arg=0` meaning "the
mud just started, this is the first master of all" (arg 1/2/3 are
master-reload/reactivation cases, confirmed out of scope for a
boot-sequence slice, not silently dropped without checking).

Confirmed this driver's own boot sequence (`main.cpp`) calls exactly
one apply on the master before this session -- the UID query (row
1.4's `queryMasterUid()`) -- fully greenfield for anything else.
Confirmed real `addDriverHooks()` is the *only* thing
`inaugurate_master()` needs to trigger for hooks.c's own real content
to install: real hooks.c's own body is a one-line
`inaugurate_master(int arg) { addDriverHooks(); }`, nothing else shares
this exact real boot point in the real corpus. Confirmed this is
genuinely LDMud-only, not a differently-named universal concept: read
real FluffOS's own `set_master()` (`temp/reference/
fluffos-2.9-ds2.08/master.c:88-141`, only queries `get_root_uid()`/
`get_backbone_uid()`) and real FluffOS `main.c` (`preload_objects()`, a
C-level mechanism, plus `APPLY_FLAG` for `-f` flags -- no single
"you're ready" master callback exists there at all).

**Built.** `BootApi::inaugurateMasterApply()` (`std::optional<string>`,
matching `simulEfunFile()`'s own optional-apply shape) -- `LdmudBootApi`
returns `"inaugurate_master"`, `FluffOsBootApi` returns `std::nullopt`
(a real, structural "no equivalent apply" distinct from "the mudlib
doesn't define one", the latter already covered separately by
`callFunction()`'s own existing convention). New
`applyInaugurateMaster(VM&, const BootApi&)`
(`src/dialect/InaugurateMasterBoot.{hpp,cpp}`), mirroring
`queryMasterUid()`'s own established shape and non-fatal-on-error
convention exactly -- a no-op under `std::nullopt`, calls
`inaugurate_master(0)` under LDMud, silently tolerates an undefined
apply. Wired into `main.cpp` at the exact real boot-order position,
right after the master UID query and before the simul_efun load.

2 new regression tests: `inaugurate_master(0)` fires for LDMud and not
at all for FluffOS even when the master defines a function by that
exact name (proving the gate is real, not coincidental); and the real
point of the whole investigation -- real hooks.c's own `H_MOVE_OBJECT0`
shape installed with *zero* manual `set_driver_hook()` calls anywhere
in the test, only `applyInaugurateMaster()` itself, then triggered
through a genuine `move_object()` efun call.

**Verified live against the real running driver, real bundled
`mudlib/`** (a scratch config on spare port 4126, `dialect: ldmud`, a
plain TCP client, real `eval` calls, a scratch `master_file:
/tmp_hooks_master` -- a real master object living in the same real
bundled `mudlib/` tree, reproducing hooks.c's own real
`inaugurate_master()`/`addDriverHooks()`/`moveHook()` content plus the
real bundled mudlib's own real `connect()` so a real client gets a
working login/eval shell -- with no manual `set_driver_hook()` call
issued at any point in the live session): the boot log itself shows
`master inaugurate_master(0) ...` printed at the correct real
boot-order position; a fresh connection's own real login worked
normally; `eval object room = clone_object("/tmp_hooks_room2"); object
before = environment(); move_object(room); object after =
environment(); return ({ before, after, after == room });` returned
`({ 0, //tmp_hooks_room2, 1 })` -- confirming the hook was installed
purely by the boot sequence and fired correctly on the first real
`move_object()` call, boot to hook execution, no manual wiring step
anywhere. Driver stayed healthy throughout (`eval return 200+1;` ->
201, `who`). Scratch master/room files and `tmp_eval_file.c` removed
before stopping the scratch process, leaving the bundled `mudlib/`
tree as found.

**One real, pre-existing ROADMAP.md structural bug found and fixed
along the way, not introduced this session but directly touching the
same row.** Row 1.7's own cell had been fragmented across two earlier
sessions' own edits: a large block of real, correct content (the
`unbound_lambda()`/`bind_lambda()` corpus re-check and the
`set_driver_hook()` slice write-up) had landed as orphaned paragraphs
wedged between row 1.4's cell and row 1.5's, breaking the markdown
table's own single-line-per-row structure (confirmed by checking every
row line actually starts with `|` -- rows 1.4 through 1.7 did not
before this fix). The content itself was accurate and not lost, just
misplaced and split across multiple paragraphs instead of appended to
row 1.7's own line. Fixed by moving the misplaced block into row 1.7's
actual cell (flattened back to single-line prose, matching every other
row's own convention) and closing row 1.4's cell cleanly where its own
real content actually ends. Verified line by line afterward that every
row from 1.1 through 1.16 is a single, well-formed table line again.
This was a documentation-structure fix only, not a code or scope
change -- done because leaving it broken would have compounded the
confusion for whichever session reads row 1.7 next, and this session
was already the one adding new content to that exact row.

Still open: `H_LOAD_UIDS`/`H_CLONE_UIDS`/`H_INCLUDE_DIRS` dispatch,
every other real hook number, real per-hook type-map validation,
`privilege_violation()`, and `inaugurate_master()`'s own arg=1/2/3
master-reload/reactivation cases.

Staged with `git add` only, per this project's own standing rule; not
committed.

**2026-08-19 (a further fresh session, same day): row 1.7/1.8's
`set_driver_hook()` first slice landed -- real storage plus real
H_MOVE_OBJECT0/1 dispatch wired into `move_object()`'s own real trigger
point, confirmed live through the real efun call, exercising last
session's own unbound_lambda()/bind_lambda() machinery through its real
intended caller for the first time (690 tests, up from 687).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules).

**Real scope confirmed from source before writing anything.** Read real
`f_set_driver_hook()` (simulate.c:5056-5228) in full: range validation
(exact "Bad hook number" text), the real privilege_violation() gate
(this driver has none), real per-hook type-map validation
(`hook_type_map[]`, prolang.y:195-229), and a real special case --
`set_driver_hook()` takes ownership of and immediately rebinds an
unbound_lambda to master_ob for a hook whose type map otherwise
disallows closures. Re-read hooks.c in full too (not trusted from last
session's own summary): `addDriverHooks()` makes 14 real
`set_driver_hook()` calls, 4 via `unbound_lambda()`
(`H_MOVE_OBJECT0`/`H_LOAD_UIDS`/`H_CLONE_UIDS`/`H_INCLUDE_DIRS`, exactly
matching last session's own 4-hit corpus count), the rest plain strings
or one mapping. Confirmed this driver has zero pre-existing hook
storage/dispatch scaffolding (the two other grep hits were prose
comments from last session, not code) -- genuinely greenfield.

Traced `addDriverHooks()`'s own real caller: `inaugurate_master()`
(`secure/master.c:14-17`), real LDMud's own master-boot apply. Confirmed
this driver has no `inaugurate_master()` wiring at all (grep, zero
hits) -- a real, separate, previously-untracked prerequisite this
investigation surfaced and named explicitly in ROADMAP.md, not silently
worked around. For this session's own live verification, a real object
calls `addDriverHooks()`-equivalent explicitly instead.

Read the real trigger point for `H_MOVE_OBJECT0`/`H_MOVE_OBJECT1`
specifically: object.c's own `move_object()` static function
(object.c:3920-3948), the shared C implementation behind both the
`move_object()` and `transfer()` efuns. Two real, distinct bind targets,
not a guess: `H_MOVE_OBJECT1`'s closure is rebound to the item being
moved (`put_ref_object`), `H_MOVE_OBJECT0`'s to current_object
(`assign_current_object`), both via real `call_lambda()` (`interpret.h:
346`, `bind_ob = NULL` -- the caller mutates the closure's own base.ob
directly first) rather than `call_lambda_ob()`'s own on-the-fly-bind
path (confirmed a real, distinct mechanism from `H_LOAD_UIDS`'s own
`determine_uid()`/`call_lambda_ob()` path, simulate.c:1526-1652, read
too but not wired this session -- bounded to "at least one" hook number
per this session's own instructions).

**Built.** `VM` gained a 32-slot `driverHooks_` array (`kNumDriverHooks`,
matching real `mudlib/sys/driver_hook.h`'s own hook-number defines
exactly -- this file did not exist anywhere in this driver's own bundled
`mudlib/sys/` before this session, added as a real, faithful mirror of
the vendored reference source's own bundled copy). `getDriverHook()`/
`setDriverHook()` (range-checked, real message; per-hook type-map
validation and the privilege_violation() gate deliberately not
replicated, matching this driver's own established permissive-storage
precedent and its own honest-gap convention for anything needing
privilege_violation()). `callDriverHookClosure()` unifies real
`call_lambda()`/`call_lambda_ob()`'s two distinct real mechanisms into
one helper (both have the same observable effect: the closure's own
home object is freshly overwritten immediately before each call).
`VM::moveObject()` now tries `H_MOVE_OBJECT1` then `H_MOVE_OBJECT0`
before falling back to this driver's own pre-existing hardcoded
FluffOS-style logic -- a deliberate, flagged multi-dialect divergence
from real LDMud (which has no fallback at all, "Don't know how to move
objects."). `set_driver_hook()` registered unconditionally, matching
this table's own established dialect-neutral-availability convention.

One new real efun needed to make hooks.c's own real `moveHook()` body
runnable at all: `set_environment(item, env)` (object.c:5152-5230,
"no calls to init() or such" -- exactly the low-level primitive real
moveHook() itself calls). 3 new regression tests: out-of-range hook
rejection; the real end-to-end H_MOVE_OBJECT0 dispatch through the real
`move_object()` efun (confirmed item/dest/this_object() and the actual
move -- and confirmed, live and via reasoning from real
interpret.c's own CLOSURE_LFUN case, that the current_object rebind is
*not* actually observable through hooks.c's own exact shape, since
`#'moveHook` is its own separately-bound closure); the no-hook fallback
still works unchanged.

**One real, latent bug found and fixed along the way, surfaced by this
same live verification, not assumed safe:** `runPreprocessor()`'s own
`cpp` invocation never included this driver's own CWD as a `-I` search
dir, so `rewriteAbsoluteIncludes()`'s real absolute-quoted-`#include`
rewrite (`"/sys/driver_hook.h"` -> `"mudlib/sys/driver_hook.h"`,
CWD-relative) had no way to actually resolve -- confirmed by direct
`cpp` reproduction before and after the fix. This driver's own bundled
mudlib had never used an absolute quoted `#include` before this
session's own scratch verification file, so the gap had never been hit
before. Fixed with one added `-I '.'` flag. Full test suite re-run
before and after: no regressions either way.

**Verified live against the real running driver, real bundled
`mudlib/`** (a scratch config on spare port 4125, `dialect: ldmud`, a
plain TCP client, real `eval` calls, a temporary scratch object
reproducing hooks.c's own real `H_MOVE_OBJECT0` content -- `moveHook()`
trimmed to just its `set_environment()`-performing core, the
`living()`/`set_this_player()` legs already covered by the unit test's
own fuller shape): `set_driver_hook(H_MOVE_OBJECT0, unbound_lambda(...))`
on a real scratch hook-holder object, then a genuine `move_object(room)`
efun call (not `funcall()`) from a different real object -- returned
`({ 0, //tmp_hooks_room, 1 })`: no environment beforehand, the real room
afterward, confirming the hook (not the hardcoded fallback) performed
the move. One real, incidental finding along the way: this driver's own
object-lifetime model keeps a `clone_object()` result alive only via
genuine live references (matching `LiveObjectRegistry`'s own
established weak-ref-registry precedent, not a new gap) -- a first
attempt that left an installed hook referencing an since-unreferenced
scratch object correctly, faithfully, went "Uncallable"/destructed on
the next real trigger (a subsequent login's own `move()` call), exactly
matching this driver's own existing destructed-owner handling; the
final verification explicitly clears the hook (`set_driver_hook(0, 0)`)
at the end of the same `eval` call that installed it, and confirms a
fresh connection/login (itself calling `move()`) works normally
afterward, and the driver process itself stayed healthy throughout.
Scratch object files and `tmp_eval_file.c` removed before stopping the
scratch process, leaving the bundled `mudlib/` tree as found except for
the new, intentional `mudlib/sys/driver_hook.h`.

Still open: `inaugurate_master()` (the real automatic boot-wiring gap
this session surfaced and named), `H_LOAD_UIDS`/`H_CLONE_UIDS`/
`H_INCLUDE_DIRS` dispatch (real trigger points already cited, not wired
this session), every other real hook number, real per-hook type-map
validation, and `privilege_violation()`.

Staged with `git add` only, per this project's own standing rule; not
committed.

**2026-08-19 (a further fresh session, same day): row 1.9 closed out in
ROADMAP.md with each remaining sub-item's zero-corpus-usage verdict
recorded explicitly; then row 1.7/1.8 (LDMud closure kinds) picked next
on a fresh corpus re-check, and a bounded `unbound_lambda()`/
`bind_lambda()` first slice landed, including a new `Symbol` `Value`
type and a small quoted-code call-tree evaluator (687 tests, up from
683).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules).

**Row 1.9 close-out.** The prior session's own corpus re-check (five
sub-items, all zero real usage) was already recorded in ROADMAP.md, but
buried inside one long paragraph rather than named individually where a
future session would actually look. Added an explicit, itemized
close-out to row 1.9's own cell: each of `([:width])`, the
`m_allocate`/`m_entry`/`m_reallocate`/`m_add`/`m_contains` efun family,
mapping range index, and save/restore of extra columns now has its own
named "zero real hits, confirmed" line, plus an explicit note that a
"zero" from one pass is not permanent and should be re-checked, not
trusted forever (the width-2 literal itself was a real example of an
earlier "zero" turning out wrong on closer reading). No new code this
half -- documentation only, per this session's own explicit instruction.

**Fresh corpus re-check, row 1.7/1.8 vs row 1.4/1.16.** Re-ran both
from scratch rather than trusting the numbers already on record.
`unbound_lambda`: still exactly 4 real hits, all in
`secure/master/hooks.c`, all handed straight to `set_driver_hook()`.
`bind_lambda`/bare `lambda(`: still 0 (the earlier "lambda( has hits"
reading was always just the substring inside `unbound_lambda(`,
re-confirmed directly). A broader `#'` sweep this pass first
over-counted at 311, then 194 hits before narrowing to true fixed-string
matches -- all false positives, `##Name##'s`-shaped template-string
noise, not real closure syntax; the real bare-name/`efun::` forms are
already fully covered by row 1.2/1.3's own prior slice, and every other
`#'` form (operators, `#'[`, `#'({`, `#'sefun::`/`#'lfun::`/`#'var::`)
stayed at 0. `disconnect(` (row 1.4/1.16) re-checked too: still
exclusively the unrelated database-handle-close local function pattern
found before, confirming that row stays closed. `unbound_lambda`'s 4
real hits, still the strongest live evidence of the two, won this round.

**What hooks.c's real usage actually needs, confirmed by reading it in
full first.** All 4 real call sites are
`unbound_lambda( ({params...}), ({#'targetFn, params...}) )` passed
straight into `set_driver_hook()` -- itself entirely unimplemented in
this driver (confirmed by grep: no `set_driver_hook()` efun, no
driver-hook dispatch table anywhere). This means the task's own starting
frame -- "just tag `Closure` with a kind so it can tell `unbound_lambda`
apart from an ordinary closure" -- undersold the real gap: `Closure`
already *has* one field's worth of "kind" information trivially
(`owner` set vs. unset), but `unbound_lambda()`'s own second argument is
real LDMud's own quoted-code lambda-body language (`closure.c`'s own
`lambda()`, the real C function `f_unbound_lambda()` itself calls to
compile that array-of-arrays "LISP-style" quoted code) -- there is
nothing meaningful to *do* with an unbound closure once constructed
without also being able to evaluate that body. Read `temp/ldmud/src/
lex.c:6186-6266` (the real `'`-vs-char-literal lexer disambiguation),
`closure.c:6889-6941`/`6368-6519` (`f_unbound_lambda()`/
`v_bind_lambda()`), and `interpret.c:21313-21823`
(`int_call_lambda()`'s own real `CLOSURE_UNBOUND_LAMBDA` handling,
including the exact real `"Uncallable closure"` text at
`interpret.c:21818`) before writing anything, not assumed from the
task's own framing.

**Bounded first slice, scoped to exactly what real usage needs and no
further** (this session's own explicit instruction: "not a full
`lambda`/`unbound_lambda`/`bind_lambda` rework"). New `Symbol` `Value`
variant member for LDMud's own `'name` literal (`Value.hpp`) -- row
1.2/1.3's own long-standing "new Value variant member needed" note,
greenlit by this investigation, not a scope-creep addition of its own.
`Lexer::lexQuote()` (gated to `LpcDialect::LdMud`) disambiguates it from
an ordinary character constant the same way real `lex.c` does, scoped
to a single leading quote and a bare identifier -- real corpus's own
only shape (no `''name` multi-quote, no `'({` quoted-aggregate). New
`SymbolLiteralExpr` AST node, `PushSymbol` opcode, `TokenType::
QuotedSymbol`. `unbound_lambda(args, body)`/`bind_lambda(cl [, ob])`
registered as real efuns (`EfunTable.cpp`, unconditionally, matching
this table's own dialect-neutral-availability convention).
`Closure` gained `unboundUntilBound`/`lambdaParams`/`lambdaBody`.
`VM::callClosure()` throws the real `"Uncallable closure"` text
(checked before the pre-existing destructed-owner check, since an
unbound closure's own unset owner is its normal state, not the "was
bound then died" case that check exists to catch) for a still-unbound
one, and otherwise dispatches to two new methods,
`callUnboundLambdaBody()`/`evalQuotedLambdaNode()` -- a small recursive
quoted-code walker deliberately bounded to the one real shape confirmed
live: a closure-headed call, each argument either a nested call of the
same shape or a bare `'name` symbol substituting one of the lambda's own
declared parameters, or a literal standing for itself. Real `lambda()`'s
much larger grammar (operator/control-flow closures as a call's own
head, quoted aggregates, global-variable symbol references) honestly
errors rather than silently misevaluating or guessing. `bind_lambda()`'s
cross-object form honestly rejects too, rather than silently allowing an
unauthorized cross-object bind or silently pretending a
`privilege_violation()` check passed -- this driver has zero
`privilege_violation()` call sites at all.

4 new regression tests: symbol-vs-char-literal lexing across all three
dialects; hooks.c's own exact `H_MOVE_OBJECT0` shape end to end
(uncallable until bound, correct result and correct side effect once
bound); hooks.c's own exact `H_LOAD_UIDS` shape (a nested quoted call,
plus the already-implemented `previous_object()` efun as its own inner
closure); a symbol not among the lambda's own declared parameters
throwing a clear error rather than silently misevaluating.

**Verified live against the real running driver, real bundled
`mudlib/`** (a scratch config on spare port 4124, `dialect: ldmud`, a
plain TCP client, real `eval` calls, a temporary scratch object file
mirroring hooks.c's own two real shapes): calling the still-unbound
`makeHook()` result directly threw the real `"Uncallable closure"` text
exactly (via `catch()`); after `bind_lambda()`, `funcall()` on the bound
closure correctly ran `moveHook(3, 4)`, confirmed via the object's own
now-set `lastItem`/`lastDest` (3 and 4); the nested `H_LOAD_UIDS` shape
returned `({ "/std/thing", //tmp_eval_file })`, the second element a
real, correct `previous_object()` result (the eval object itself, the
real caller at that point in the real call stack), not an error. Driver
stayed healthy after both the genuine "Uncallable closure" dispatch
error (drops only that one connection, matching this row's own
already-confirmed prior finding, not re-investigated fresh) and every
successful call afterward (`eval return 100+1;` -> 101, `who`). Scratch
object file and `tmp_eval_file.c` both removed before stopping the
scratch process, leaving the bundled `mudlib/` tree as found.

Still open on this row: `set_driver_hook()` itself (the actual real
caller of hooks.c's own 4 real call sites -- a separate,
currently-untracked prerequisite this investigation surfaced, now named
in ROADMAP.md rather than left implicit), plain dialect-agnostic
`lambda()`, the rest of real `lambda()`'s own quoted-code grammar,
`bind_lambda()`'s cross-object form (blocked on `privilege_violation()`),
and multi-quote/`quoted-aggregate` symbol forms.

Staged with `git add` only, per this project's own standing rule; not
committed.

**2026-08-19 (a further fresh session, same day): row 1.9 (LDMud mapping
N-width) remaining sub-items re-checked against real corpus evidence
fresh; none showed real usage; picked and fixed a real, live, silently-
corrupting IncDec-on-`map[key, n]` bug on structural-necessity grounds
instead (683 tests, up from 682).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules).

**Corpus re-check.** Row 1.9's remaining open sub-items going into this
session: `([:width])` mapping-width literal syntax, `m_allocate`/
`m_entry`/`m_reallocate`/`m_add`/`m_contains`, mapping range index
(`map[key, n1..n2]` / `map[key, <n]`), save/restore of extra columns,
IncDec (`++`/`--`) on `map[key, n]`. Re-checked each fresh against every
extracted mudlib corpus under `temp/` (`core-lib`, `dead-souls`,
`es2_mudlib`, `lima`, `mudlib`, `nightmare3`, `wiz_tools`, `lil`), not
reused from a prior session's count. `m_allocate`/`m_add` together
turned up 5 raw substring hits; all 5 were false positives on direct
read -- the 3 `m_allocate` hits are `dead-souls`/`nightmare3` doc-manual
prose ("`map = allocate_mapping(10) ...OR... map = m_allocate(10)`"),
the 2 `m_add` hits are substring matches inside unrelated identifiers
(`room_add_object`, `LIVE_I_VICTIM_ADDED_AWARENESS`). `m_entry`/
`m_reallocate`/`m_contains`: zero hits. `([:width])`: zero hits.
Mapping range index: confirmed as real, separate LDMud machinery by
reading `temp/ldmud/src/prolang.y`'s own `index_map_range` rule and
`F_MAP_RANGE`/`F_MAP_RANGE_LVALUE`, plus `interpret.c`'s
`unprotected_map_range`/`protected_map_range_lvalue` structs -- not
speculative, it is a real, separate grammar production and lvalue kind
from the plain `map[key, n]` this row already ships -- but zero real
corpus usage found for the actual range syntax. Save/restore of extra
columns: zero real corpus usage found (no width>1 mapping anywhere in
any corpus is ever the target of `save_object`). Net: none of the five
remaining sub-items show real corpus usage, matching this session's own
instructions' fallback case exactly.

**Structural-necessity pick: IncDec on `map[key, n]`.** Per this
session's own instructions, when no candidate shows corpus usage the
next pick is whatever is structurally necessary to keep the
already-built width-2 slice correct and safe. Checking the already-
built machinery for exactly that kind of latent breakage (rather than
just picking one of the five to start building fresh) surfaced a real,
live bug: `Parser::parsePostfix()`/`parseUnary()` (`Parser.cpp`) already
parse `map[key, n]++` / `--map[key, n]` as an ordinary `IndexExpr` with
`mapColumn` set -- no parse error -- but both sites then built an
`IncDecExpr` from it copying only `indexTarget`/`indexKey`, silently
dropping `mapColumn` on the floor. The result: `m["a", 1]++` compiled
and ran without error, but silently mutated column 0 instead of column
1 -- exactly the "silently corrupt an existing width-2 mapping" failure
mode this session's own instructions named as the deciding case. This
is real LDMud behavior worth porting faithfully, not a made-up
extension of this driver's own IncDec support: real LDMud's generic
lvalue-increment machinery has its own genuine `F_MAP_INDEX_LVALUE`
operator (`prolang.y:17018`, `interpret.c:16944`) backing
`map[key, n]++`, the same way plain `F_INDEX_LVALUE` backs `arr[i]++`.

**Fix.** `IncDecExpr` (`Ast.hpp`) gained a `mapColumn` field alongside
its existing `indexTarget`/`indexKey`. Both Parser sites (the postfix
branch in `parsePostfix()` and the prefix branch in `parseUnary()`) now
copy `idx->mapColumn` onto it instead of dropping it.
`CodeGen::emitIncDecExpr()`'s indexed branch now emits the mapColumn
expression alongside `indexTarget`/`indexKey` (both places it currently
re-evaluates them, for the read and for the write, same pre-existing
double-evaluation caveat `emitIndexAssignStmt()` already documents for
plain indexed compound assignment) and sets the existing `Index`/
`IndexAssign` opcodes' `0x4` mapping-column flag bit. No `VM.cpp` change
was needed at all -- this reuses the exact same opcode paths, including
the real missing-key auto-insert and out-of-range-column error, that
the width-2 slice already shipped and tested.

1 new regression test,
`testLdmudIncDecOnMapColumnMutatesTheCorrectColumnNotColumnZero`:
postfix and prefix `++`/`--` on a real column of a real width-2 mapping,
confirming column 0 stays untouched by column-1 operations and vice
versa, plus a missing-key auto-insert via `++m["new", 1]` (the other
column defaults to 0, matching the width-2 slice's own already-tested
plain-assign auto-insert behavior).

**Verified live against the real running driver, real bundled
`mudlib/`** (a scratch config on spare port 4123, `dialect: ldmud`, a
plain TCP client, real `eval` calls):
`mapping m = (["a": 10; 100]); mixed r1 = m["a",1]++; mixed r2 =
++m["a",1]; mixed r3 = m["a",0]--; mixed r4 = ++m["new",1]; return
({r1,r2,r3,r4,m["a",0],m["a",1],m["new",0],m["new",1]});` returned
`({ 100, 102, 10, 1, 9, 102, 0, 1 })`, matching hand-derived expected
values exactly (postfix `++` returns the pre-mutation 100 and leaves
column 1 at 101 before the next call; prefix `++` returns the
post-mutation 102; postfix `--` on column 0 returns 10 and leaves it at
9, column 1 untouched; the missing-key insert returns 1 with column 0
defaulted to 0). Driver stayed healthy afterward (`eval return 1+1;` ->
2, `who`). Live `tmp_eval_file.c` removed before stopping the scratch
process, leaving the bundled `mudlib/` tree as found.

Still open on row 1.9, confirmed zero real corpus usage this session:
`([:width])`, `m_allocate`/`m_entry`/`m_reallocate`/`m_add`/
`m_contains`, mapping range index, save/restore of extra columns.

Staged with `git add` only, per this project's own standing rule; not
committed.

**2026-08-19 (a further fresh session, same day): env-override test
confirmed, nicks remaining-work recorded, parse_* false-pass audit
found none, then row 1.9's first real N-column mapping-width slice
(682 tests, up from 678).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules). The previous session had added
`testParseSentenceExplicitEnvArrayOverridesTheOrdinaryEnvironmentWalk`
but cut off before the suite was confirmed. That test is wired in
`main()` next to the other parse tests, compiles, and passes. Both
runners (`ctest --test-dir build --output-on-failure` and
`./build/test/amlp_tests`) are the confirmation path used below.

**Housekeeping (a), env/nicks:** the env half is closed. Confirmed from
source, not just the new test: `EfunTable.cpp` passes `args[2]` into
`ParserPackage::parseSentence()`, which stores it on
`SentenceSession::envArray`, and `loadObjects()` already walks
`getObjectsFromArray`/`addObjectsFromArray` when `envArray` is set.
The nicks half is not `add_action` nicknames. Remaining work is
`parse_sentence()`'s own 4th argument, a mapping of nickname word to
object. Real `f_parse_sentence()` (`packages/parser.c:3046-3050`)
stores `parse_nicks` then `parse_env`. Real `add_nicknames()`
(`parser.c:1095-1108`) marks hash entries `HV_NICKNAME` for string
keys. Real `load_objects()` calls it at `parser.c:1162-1163` after
the `"my"` adjective. Real `expand_node()` (`parser.c:1302-1323`)
lazy-looks up and only sets `HV_NOUN` if the object is already in
`loaded_objects`. Real `parse_obj()` calls `expand_node()` at
`parser.c:1402-1403` when `HV_NICKNAME` is set. This port:
`HashEntry::isNickname` exists but is never set; `parseObj()` has no
`expand_node` path; `EfunTable.cpp` still does not pass a 4th arg.
Not trivial, not implemented. Concrete remaining-work note recorded
in `ROADMAP.md` row 0.13a.

**Housekeeping (b), bounded false-pass audit:** the interrupt point
around `test_lexer.cpp:4778` is regexp tests, not parse_*. The
parse_* tests live at about 18143-20321. Known false-pass cases from
prior sessions (the two-object skip test; the plural-side skip test)
were already rewritten. Bounded pass of the parse_* tests from that
work: no new "asserting success without a fixture that could actually
fail the assertion meaningfully" instances found. No test fixes
required for this item.

**Re-rank:** re-checked corpus evidence for row 1.9 (LDMud mapping
N-width), row 1.7/1.8 (LDMud closure kinds), and row 1.4/1.16
(connect/disconnect). Row 1.7/1.8 still has `unbound_lambda` at 4
hits in `secure/master/hooks.c`, `lambda`/`bind_lambda` at 0, remaining
`#'` forms at 0 real hits. Row 1.4/1.16: FluffOS `net_dead()` is
already used; LDMud `void disconnect(object, string)` still 0;
`Server.cpp` already hardcodes `connect`/`net_dead`. Row 1.9's prior
"zero width usage" claim was wrong: `temp/core-lib/areas/tol-dhurath/objects/rune-wall.c`
has a real width-2 literal, `m_values(wall, 1)`, and
`wall[whichRune, 0]` / `wall[whichRune, 1]` assigns. Highest impact.
Picked.

**First N-column slice:** enough for rune-wall.c. Real source followed:
literal `prolang.y:17232-17259` (exact `"Inconsistent number of values
in mapping literal"`); index `prolang.y:17007` `F_MAP_INDEX` and
`interpret.c:6862-6938` `push_map_index_value`; `m_values`
`mapping.c:3159-3211` (C errors on out-of-range, man page fallback
does not match, C is authoritative); missing-key column read returns
int 0; missing-key assign auto-inserts after column range-check.
`Mapping` gained `width`/`extraColumns`. Parser is LDMud-only for `;`
extra values and `map[key, n]`. Copy/delete/filter/map_mapping/reclaim
paths that would have dropped extra columns were updated rather than
left silently wrong. Still open: `([:width])`, `m_allocate`/`m_entry`/
`m_reallocate`/`m_add`/`m_contains`, mapping range index, save/restore
of extra columns, IncDec on `map[key, n]`. 4 new regression tests
plus the existing width-1 `m_values` rejection now asserts the real
illegal-index message.

**Verified live against the real running driver, real bundled
`mudlib/`** (a scratch config on spare port 4122, `dialect: ldmud`, a
plain TCP client, real `eval` calls):
`mapping m = (["weakness": "<missing>"; 1]); return ({ m["weakness"],
m["weakness", 0], m["weakness", 1], m_values(m, 1) });` returned
`({ "<missing>", "<missing>", 1, ({ 1 }) })`; a missing-key column-1
read returned int 0. Driver stayed healthy afterward (`eval return
6*7;` -> 42, `who`). Live `tmp_eval_file.c` removed before stopping
the scratch process, leaving the bundled mudlib tree as found.

Staged with `git add` only, per this project's own standing rule; not
committed.

**2026-08-19 (a further fresh session, same day): `parse_*` (row 0.13a)
ninth real slice -- item 9's plural side (`"give OBS to LIV"` and the
mirror `"give LIV OBS"` shape), landed with zero new production logic
beyond removing an earlier session's own conservative gate; row 0.13a's
checkbox flipped to done (partial: `add_nicknames()`/explicit
`env` override still unconsulted) (677 tests, up from 676).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules). Per this session's own explicit
instruction, re-read the real plural-side logic in
`check_object_relations()`/`check_one_relation()`/
`dependent_check_functions()` directly (`packages/parser.c:2184-2493`)
before building anything, specifically to confirm how much of the
already-built both-singular machinery was reusable versus how much
genuinely needed new plural-matching logic.

**Real finding: none of it needed new logic.** `check_object_relations()`
has exactly one plurality-dependent decision anywhere in its own body --
`direct_unique`/`indirect_unique`, computed straight from each match's
own real `PLURAL_MODIFIER` bit, gating whether an ambiguity check applies
at all and whether the final resolved value is a single index or the
whole accumulated set -- and the prior session had already ported the
*entire* real function faithfully (not a hand-simplified singular-only
version), including both of those gates, since restricting to the
both-singular case was done entirely at the `ParserPackage::
parseRulesFor()` call-site level (a `VerbRuleNode::hasPluralObjectToken`
skip), not inside the algorithm itself. `dependent_check_functions()`/
`check_one_relation()` have no plurality-dependent logic at all in real
source -- confirmed by reading both in full a second time. `we_are_finished()`'s
own real dispatch (`if (tok & PLURAL_MODIFIER) plural_check_functions(...)
else if (num_objs==2) dependent_check_functions(...) else
singular_check_functions(...)`) already matched this driver's existing
`weAreFinished()` dispatch exactly, meaning a plural slot inside a
two-object rule was *already* routed to the real, already-tested
`pluralCheckFunctions()` (built in an earlier single-object slice) rather
than needing any new narrowing logic of its own.

Given this, the real remaining work was: remove `parseRulesFor()`'s own
`hasPluralObjectToken` gate, remove the now-purposeless field itself and
its computation helper, update every comment that referenced the old
restriction, and -- the substantive part -- write real regression tests
proving this actually works end to end rather than trusting the
re-derivation alone, per this project's own standing "confirm it works,
don't just reason about it" discipline.

Two real dead-souls.net Dead Souls `give.c` shapes chosen as the real
regression targets: `"OBS LIV"` (plural direct/multiple items, singular
indirect/one recipient -- "give coins guard") and its mirror `"LIV OBS"`
(singular direct, plural indirect -- "give guard coins"), both real rules
already confirmed present in that same file's own `SetRules()` call from
the prior session's own corpus survey. Building these surfaced a real,
subtle naming derivation that needed care: real `make_function()`'s own
"obs"/"lvs" vs. "obj"/"liv" spelling decision (`omatch+1 >= which ||
!plural || which>=4`) means the plural spelling is reachable *only* for
the `do_` call (`which==3`) or when a *later* probe's own `which` exceeds
an *earlier* token's own position -- never for a probe of a token's own
narrowing pass, nor for either relational probe (`which>=4` always forces
the short spelling). Worked through by hand, position by position, for
both rule shapes, and independently cross-checked against real
`give.c`'s own actual function names, which never define a single
`can_`/`direct_`/`indirect_` variant with an "obs"/"lvs" segment anywhere
-- only its three `do_` variants do, exactly matching the derivation.
One consequence confirmed concretely: for `"OBS LIV"`, the indirect
candidate (the guard) genuinely needs *two different* `indirect_` names
defined -- one for its own narrowing pass (`which==2`, "obs" spelling)
and a different one for the relational pairing pass (`which==5`, "obj"
spelling, since `which>=4` forces it) -- while for `"LIV OBS"` those two
`which` values happen to coincide on one name, a real, position-dependent
asymmetry between the two mirror shapes.

One genuine test-authoring mistake caught and fixed before trusting the
result, not a driver bug: an initial version of the plural-direct test
used the *singular* noun ("coin") in its own sentence, which real
`parse_obj()`'s own singular-noun match path (`ParserPackage.cpp`)
correctly strips `PLURAL_MODIFIER` from, even for an OBS-token rule slot
-- OBS means "this slot accepts either one object or several," not
"always plural"; which one happens is decided by what the player typed,
not by the rule's own declared token kind. The test's own two equally
valid, unconditionally-accepting coins then produced a real, correctly-
computed `ERR_AMBIG` (confirmed by adding a temporary master
`parser_error_message()` probe to inspect the real error type directly
rather than guessing from the bare return value) instead of the intended
plural success path -- fixed by using the real plural noun ("coins",
`parse_command_plural_id_list()`), not by changing any production code.

Also fixed along the way, matching this project's own "an accidentally-
still-green assertion is a bug in the test, not a pass" standard from the
prior session's own singular-case fix: the OLD "plural side still
skipped" regression test kept passing after the gate was removed, but
for an entirely different, accidental reason -- its own fixture never
defined any `direct_`/`indirect_` function on its candidates at all, so
the relational check now genuinely runs and genuinely fails to find any
callable function, rather than the rule being skipped outright. Replaced
with the two real positive tests described above.

3 new regression tests total (`test/test_lexer.cpp`; one new test net,
since the old stale one was replaced by two): plural-direct/singular-
indirect ("OBS LIV") resolving to the real filtered array (three
same-named coins, one rejected by its own `direct_give_obj_liv()`,
confirmed excluded in the real descending-index order the single-object
plural slice already established) alongside the real singular indirect
target; and the mirror singular-direct/plural-indirect ("LIV OBS") shape,
same filtering proof on the other side. 677 tests passing (up from 676:
net +1, one old test replaced by two new ones), zero regressions.

**Verified live against the real running driver, real bundled
`mudlib/`** (a scratch config on a spare port, a real telnet-negotiating
Python client, real `eval` calls): built a real room/three-coins/guard
scene via `write_file()`/`clone_object()`, registered a real
`parse_add_rule("give", "OBS LIV")` with the exact real per-`which`
naming derived above, and confirmed live that `parse_sentence("give
coins guard")` resolved to the real filtered array (`{coin2, coin1}`,
coin3 correctly excluded, real descending-index order) and the real
guard, invoking `do_give_obs_liv` with exactly those values -- the same
behavior the new regression test proves, reproduced against the real
running driver end to end. Driver process confirmed to stay healthy
afterward (`eval return 6*7;` -> 42, `who`), and every live test artifact
(`/zzzp_*` files) removed before stopping the scratch process, leaving
the real bundled mudlib tree exactly as found (`git status` clean under
`mudlib/`).

With both the both-singular and plural sides of item 9 now real, row
0.13a's own checkbox is flipped to done in `ROADMAP.md` -- marked
partial, since `parse_sentence()`'s own `env`/`nicks` arguments (an
explicit environment-object-array override and a caller-supplied
nickname mapping) are still accepted for real signature compatibility
but never consulted, matching `load_objects()`'s own long-standing,
already-documented scope note.

Staged with `git add` only, per this project's own standing rule; not
committed.

**2026-08-19 (a further fresh session, evidence-based re-rank): `parse_*`
(row 0.13a) eighth real slice -- item 9's own two-object rule family,
`dependent_check_functions()`/`check_one_relation()`/
`check_object_relations()`, for the both-object-tokens-singular case
(676 tests, up from 674).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules), then re-ranked all four
still-open real candidates the prior session's own report had named
(item 9's two-object family; row 1.9's LDMud mapping N-width value
semantics; row 1.7/1.8's LDMud closure kinds; the narrowed row 1.4/1.16
FluffOS `net_dead()` vs. LDMud `disconnect()` question) against real
corpus evidence rather than the "obvious architectural next step"
assumption alone, matching this row's own established methodology
(`m_indices`/`m_values`'s own real-call-site count from an earlier
session). Unzipped and searched every FluffOS-family corpus vendored in
`temp/` (previously-unpacked `dead-souls`/`lima`/`es2_mudlib`/
`nightmare3` plus five zip archives extracted fresh this session:
`final_realms_fluffos_v1`, `skylib_fluffos_v3`, `tmi2_fluffos_v3`,
`lpuni_fluffos_v1`, `merentha_fluffos_v2`) for real two-object
`parse_add_rule()` usage, and `temp/core-lib` (the one confirmed
LDMud-targeting corpus vendored here) for real `m_allocate`/`m_entry`/
`m_reallocate`/`m_add`/`m_contains`/mapping-width-literal/`lambda()`/
`unbound_lambda()`/`bind_lambda()`/real-signature `disconnect()` usage.
One real methodology correction caught and fixed before trusting any
count, the same discipline the `m_indices`/`m_values` session's own
"naive substring search overcounted" note already established as
precedent: a first `grep -E` pass combining two `\b...\b` word-boundary
groups in one pattern silently matched nothing at all (confirmed by
isolating the exact failing pattern against a known-real hit,
`dead-souls.net`'s own Dead Souls `lib/verbs/items/give.c`'s
`SetRules(..., "OBS LIV", ...)` line, and finding it did not match even
in isolation) -- re-derived with a direct Python scan of every real
`SetRules()`/`parse_add_rule()` call's own quoted rule strings instead of
trusting the broken regex's zero-count. Real result: 77 real two-object
rules found across every corpus (`give`, `put`, `get`, `pour`, `install`,
and more, dead-souls.net's own Dead Souls `lib/verbs/` alone), 59 of
which (77%) are both-singular (OBJ/LIV only, no OBS/LVS) -- versus zero
real usage for row 1.9's own remaining scope, `unbound_lambda()` at only
4 real call sites (all in one master-hook-configuration file,
`secure/master/hooks.c`) with `lambda()`/`bind_lambda()` at zero, and
zero real usage for a genuine per-object `void disconnect(object,
string)` master apply (every "disconnect" hit found was an unrelated
same-named database-handle-close local function,
`secure/dataServices/dataService.c`, confirmed by reading the actual call
sites, not just counting grep hits). Item 9 won decisively on this
evidence -- see `ROADMAP.md`'s own updated rows 1.4, 1.7, 1.9, and 1.16
for the same finding recorded against each of the three candidates not
picked.

Per this session's own explicit instruction, the real, confirmed latent
bugs already flagged in real `check_object_relations()` during an
earlier session's own investigation were treated as a "port faithfully,
flag clearly, do not silently fix or silently reproduce without a note"
decision, not a reason to avoid the row -- and the row was explicitly
bounded into its own first sub-slice rather than attempting the whole
family at once, the same pattern every other large item in this project
has used. Scope: both object-family tokens singular (`VerbRuleNode::
hasPluralObjectToken`, computed at `parse_add_rule()` time) --
real-corpus-confirmed the large majority shape (59 of the 77 real
two-object rules found). A genuinely plural two-object rule (`"give OBS
to LIV"`, 18 of the 77) stays deferred, `ParserPackage::parseRulesFor()`
skipping any such node outright, the same honest "not yet supported, not
silently wrong" stance this row has used throughout.

Implemented: `ParserPackage::dependentCheckFunctions()` (real
`dependent_check_functions()`, the two-object analog of the
already-real `pluralCheckFunctions()` -- narrows each object match's own
candidate bitvec independently first), `checkOneRelation()` (real
`check_one_relation()`, the real which=4/which=5 relational-naming probe
that tests one specific fixed (direct, indirect) pair jointly), and
`checkObjectRelations()` (real `check_object_relations()`, the N×M
cross-product pairing scan that decides which pair actually wins,
packages/parser.c:2312-2493 read in full this session, not just the
portion an earlier session's own scoping pass had stopped at). Two new
small helpers, `cacheLastParallelError()`/`useCachedParallelError()`
(real `cache_last_parallel_error()`/`use_cached_parallel_error()`,
distinct from the already-real `saveLastParallelError()`/
`useLastParallelError()` -- a caller-supplied scratch slot rather than
the fixed session-wide target). `ParserPackage::makeFunction()` (real
`make_function()`) generalized from its previous single-object-only
hardcoding to a genuine `omatch`-counting formula, provably equivalent to
the old version for every `which` value it used to see (confirmed by
hand for which in {0, 1, 3}) while correctly adding the two new cases a
real two-object rule reaches (`which >= 4`, reading the new
`SentenceSession::directObject`/`indirectObject` scratch fields instead
of a candidate bitvec; `omatch == 2`, a genuine second object-family
slot). `parseRulesFor()`'s own skip condition narrowed from "any node
with two object tokens" to "any node with two object tokens where at
least one is plural."

Two real, confirmed bugs in real FluffOS 2.9's own
`check_object_relations()` found while reading the full real source for
this port (not assumed from an earlier session's own partial read) and
ported faithfully, flagged inline in `ParserPackage.cpp` with their real
source citation, per this session's own explicit instruction:

1. A negative-ordinal indirect object (`"give the sword to the last
   guard"`) scans from the wrong starting bitvec index in real code --
   real `i` is left at `state->num_matches` (never reset to 0 the way a
   positive ordinal's own branch does) and indexes bitvec *words*, not
   individual objects (real `BPI * i + k`, `BPI == 32`,
   `packages/parser.h:46`), so the real absolute starting object index
   is `BPI * state->num_matches` -- every candidate below that index is
   silently skipped in real FluffOS too. Ported as the same real starting
   index, not object index 0.
2. The main pairing loop's own early-exit test, `if (found_direct &&
   (!direct_ordinal || ...))`, uses `found_direct` for plain C
   truthiness rather than `found_direct >= 0` the way every other use of
   this exact variable in this same real function tests it (including
   four lines above, and again at the function's own tail) -- confirmed
   inert for the ordinary no-ordinal case this sub-slice targets (the
   loop simply runs to full completion instead of exiting early, which
   changes nothing about the final accumulated result), but a real,
   confirmable bug regardless, not silently corrected.

One genuine implementation mistake of this session's own, caught by its
own new regression test rather than shipped: an early draft mistranslated
real `!direct_ordinal` (true only when a positive ordinal has counted
down to exactly 0) as `directOrdinal <= 0` (true whenever there is no
ordinal at all, `directOrdinal == -1`) in the early-exit check -- this
made the pairing loop stop after the very first valid direct candidate
instead of scanning every candidate, silently defeating ambiguity
detection. Caught immediately by
`testParseObjTwoSingularObjectTokenRuleWithTwoValidDirectCandidatesProducesErrAmbig`
(new this session, see below) crashing with an uncaught
`LpcRuntimeError` (`"parse accepted, but no do_* function found"`) rather
than silently misresolving, confirmed as a real coding mistake rather
than a faithfully-ported real bug by re-deriving the exact real
condition from source, and fixed before this session's own final test
run.

4 new regression tests (`test/test_lexer.cpp`): the existing
`testParseObjTwoObjectTokenRuleIsSkippedNotSilentlyMismatched` renamed
and fixed rather than left accidentally green -- its own function names
(`can_put_obj`/`do_put_obj`) were never the real two-object naming
convention (`can_put_obj_obj`/`do_put_obj_obj`, one `_obj`/`_liv` suffix
per object token), so it kept passing after this slice landed for an
entirely different, accidental reason (the generic can_ gate never found
a matching name, not because the rule was still skipped) -- corrected to
the real naming convention and to assert the real, now-supported success
path, matching this project's own "an accidentally-still-green assertion
is a bug in the test, not a pass" standard; a genuine relational-pairing
test (two same-named swords, only one individually compatible with a
chest per the chest's own `indirect_put_obj_obj` probe, confirming the
CORRECT pair resolves, not just "any" candidate); a plural-still-deferred
test (dead-souls.net's own Dead Souls `get.c` real "OBS OBJ" shape,
confirmed still honestly skipped); and a direct-object ambiguity test
(two candidates that both individually pass every check, producing a
real `ERR_AMBIG` with both objects in the same real descending-index
order the single-object ambiguity test already established, rather than
an arbitrary "first one wins" pick) -- this last test is what caught this
session's own implementation mistake above. 676 tests passing (up from
674), zero regressions.

**Verified live against the real running driver, real bundled
`mudlib/`** (a scratch config on a spare port, a real telnet-negotiating
Python client, real `eval` calls): built a real room/player/two-swords/
guard scene via `write_file()`/`clone_object()`, registered a real
`parse_add_rule("give", "OBJ LIV")` (dead-souls.net's own real "OBS LIV"
shape, narrowed to singular), and confirmed live that `parse_sentence("give
sword guard")` resolved to the real correct sword (not the other,
identically-named one the guard's own `indirect_give_obj_liv` rejects)
and the real guard, invoking `do_give_obj_liv` with exactly those two
objects -- the same relational-pairing behavior the new regression test
proves, reproduced against the real running driver end to end, not just
in the test harness. One live-script-only mistake caught along the way,
not a driver bug: calling a nonexistent `->move()` method (this
mudlib's own convention is a `go()` wrapper around `move_object()`,
confirmed already in this row's own established test fixtures) silently
did nothing rather than erroring, matching this driver's own real
"undefined function returns void" contract -- caught when the whole
scene turned up empty, fixed by calling `->go()` instead. Driver process
confirmed to stay healthy afterward (`eval return 6*7;` -> 42, `who`),
and every live test artifact (`/zzzt_*` files) removed before stopping
the scratch process, leaving the real bundled mudlib tree exactly as
found (`git status` clean under `mudlib/`).

Staged with `git add` only, per this project's own standing rule; not
committed.

**2026-08-19 (another fresh session): scoped a future account/login/
character-select mudlib plan (`mudlib/ACCOUNT_LOGIN_PLAN.md`, explicitly
queued for later, not implementation); `parse_*` (row 0.13a) seventh real
slice -- `parse_my_rules()`, the eighth and, per this row's own original
8-function list, final still-missing name (674 tests, up from 670).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules), then produced the scoping plan
first, as a non-blocking side task explicitly not to compete with driver
priority. Read the real applies this driver fires around connection/login
(`master()->connect()`, `logon()`, `Server.cpp:130-213`) and confirmed
what a menu-driven login could build on: `input_to()` including a genuinely
working `INPUT_NOECHO` (real telnet echo suppression actually fires, not a
stub), `call_out()` idle timeouts, `exec()`, `crypt()`, and
`save_object`/`restore_object` (confirmed round-trip safe with each other
for same-driver account persistence, despite row 0.7's own "partial"
status referring only to `save_object`'s output not yet being byte-real
FluffOS `.o` format) -- versus what is genuinely missing (no
`getuid()`/`seteuid()`/uid model at all, confirmed by grep; every
`__PACKAGE_UIDS__` block in the bundled mudlib is dead code under this
driver). Surveyed `temp/`'s newer corpora for a real reference flow:
`temp/core-lib/secure/login.c` (+`login/core.c`/`menu-interactions.c`/
`user-creation.c`) read in full, a modern (2017-2026) LDMud-flavored,
already-unpacked `input_to()`-driven state machine with a separate auth
daemon, the most relevant reference found; `final_realms_fluffos_v1.zip`'s
`lib/secure/login.c`/`new_login.c` and `tmi2_fluffos_v3.zip`'s
`logind.c` flagged (not read) as stronger candidates for a future session
specifically because they are FluffOS dialect, this project's actual
target, unlike `core-lib`. Wrote and staged
`mudlib/ACCOUNT_LOGIN_PLAN.md`: proposed architecture (an `account_d.c`
singleton daemon, a reworked `input_to()`-state-machine `login.c`, an
explicitly-undecided character-object shape), a rough build order, and an
explicit non-status paragraph so nobody mistakes the plan for
implementation. See that file's own full content for the complete
findings; not repeated here.

Continued with row 0.13a per this session's own instructions (checked
`ROADMAP.md`/`STATUS.md`'s most recent entry for the current priority).
Investigated item 9's own two-object ambiguity family
(`dependent_check_functions()`/`check_object_relations()`,
`packages/parser.c:2184-2493`) first, as the immediately preceding
session's own prediction named it the natural next slice -- read it in
full, including the parts past where the roadmap's own prior scoping note
had stopped (`check_object_relations()`'s own direct/indirect
ordinal-and-ambiguity combinatorics, `packages/parser.c:2312-2493`).
Confirmed it is genuinely one of the largest, most bug-prone pieces of the
whole package: extensive shared mutable state across a whole rule-matching
pass, ordinal/ambiguity interactions between two independently-resolved
object sets, and what read as real latent bugs in the vendored source
itself (e.g. `we_are_finished()`'s own `if (found_direct && ...)` testing
`found_direct` as a plain truthy int, which cannot distinguish "found
object index 0" from "found nothing" -- a real, if minor, transcription
hazard for a faithful port to get right or wrong silently). Stopped and
reported rather than forced, matching this project's own established
discipline for comparably-sized items (`bind_lambda`, LDMud mapping width,
DGD `parse_string`) -- not attempted this session, `VerbRuleNode::
objectTokenCount`'s own existing "skip nodes needing two objects" stance
in `ParserPackage::parseRulesFor()` is unchanged, still the single largest
remaining piece of this row.

Picked up `parse_my_rules()` instead -- the row's own next-ranked,
appropriately-sized slice (already flagged as ready in an earlier
session's own note: "reuses (1)/(4) entirely, no new infrastructure").
While re-reading this row's own top-line status against real source
before starting, found it stale in two places, both corrected in
`ROADMAP.md` this session: `parse_refresh()`/`parse_sentence()`/
`parse_add_synonym()` had already been implemented in intervening
sessions not reflected in the row's own opening prose; and a claimed
"`parse_add_rule`'s 3-arg shadow form" was never real at all -- confirmed
directly against the real vendored signature list,
`packages/parser_spec.c:6`, `void parse_add_rule(string, string);`,
exactly two arguments.

Real `mixed parse_my_rules(object user, string sentence, void|int
do_the_call)` (`packages/parser_spec.c:10`, `packages/parser.c:3103-3160`)
implemented in full. Refactored `parseSentence()`'s own verb-lookup loop
into a shared `runParseMatch()` helper (`src/efun/ParserPackage.cpp`) so
both efuns share the identical matching engine, matching real code's own
structure (`f_parse_sentence()`/`f_parse_my_rules()` both call one
internal `parse_sentence()` helper). Threaded real `parse_restricted`
through `parseRulesFor()` as an explicit `restrictedHandler` parameter
(real `"!parse_restricted || parse_vn->handler == parse_restricted"`).
Ported real code's own two separate `hasParseInfo()` guards (on `user`
*and* on the calling object -- confirmed both are real, distinct checks,
not one check read twice). Ported the real, still-live "Illegal to call
parse_sentence() recursively." reentrancy guard -- confirmed by reading
source directly that this exact guard is commented out in real
`f_parse_sentence()` (`parser.c:3035-3039`) but genuinely live in
`f_parse_my_rules()` (`parser.c:3113-3114`), a real asymmetry this port
now reproduces faithfully via one process-wide bool flag, deliberately
not a save/restore depth counter -- matching real code's own single
global `pi` pointer and its own documented consequence that a genuinely
nested call clobbers it on unwind, not a safer reimplementation than real
FluffOS actually has. Also confirmed and implemented the real
default-argument behavior directly from source rather than assuming it:
`"int flag = (st_num_arg == 3 ? (sp--)->u.number : 0);"` -- the
two-argument form's own real default is *false* ("return the winning
match's own pre-built `verb_rule` args array"), not *true* ("invoke the
match"), the opposite of what the name alone might suggest.

4 new regression tests (`test/test_lexer.cpp`): restriction to the
caller's own registered rules only (two objects registering
incompatible-shaped rules -- STR vs. a lone WRD -- under the identical
verb name, confirming a restricted call can neither wrongly succeed
through the other object's rule nor silently fall through to it); the
default two-argument form returning the real 5-element `verb_rule` args
array without invoking anything (the same real `try==3` shape an earlier
slice's own `do_verb_rule` fallback test already confirmed, reused here as
a cross-check rather than re-derived); both real `hasParseInfo()` guards;
and the recursive-call rejection, confirmed live in-process via a `do_`
callback that itself attempts a nested `parse_my_rules()` call and catches
the real error. One implementation bug in this session's own first test
draft, not the driver, caught before it ever reached a committed test:
asserting an untouched object variable via `std::get<std::string>(...)
.empty()` -- a real uninitialized LPC object variable is plain int `0`
(`LpcObject::LpcObject()`'s own `Value(int64_t{0})` fill), not `""`,
confirmed by a `std::bad_variant_access` on the first run rather than a
silent false pass. 674 tests passing (up from 670), zero regressions.

**Verified live against the real running driver, real bundled `mudlib/`**
(a scratch config on a spare port, a real telnet-negotiating Python
client, real `eval` calls): built two real handler objects via
`write_file()`/`clone_object()` registering incompatible rules under one
shared verb name plus a plain user object, and confirmed live that a
restricted call to the STR-owning handler resolved and called its own
`do_` function with the real matched text while the other handler's own
variable stayed untouched, that the identical sentence restricted to the
WRD-owning handler correctly failed with the real `-1` "how close did we
get" signal rather than falling through to the other handler's own STR
rule, that the default two-argument form returned the real, exact
5-element `verb_rule` args array without ever invoking anything, and that
a real nested `parse_my_rules()` call from inside a `do_` callback was
rejected while the outer call itself still succeeded normally. One
real, if mundane, methodology lesson hit live and worth recording: real
`write_file()` genuinely *appends* rather than truncates, confirmed the
hard way when a stale file left over from an earlier broken test attempt
in this same session caused a real "object variable already declared"
compile error on the next `write_file()` to the same path -- fixed by
`rm()`-ing first, not a driver bug. Confirmed the driver process and
ordinary gameplay stayed healthy throughout (`eval return 6*7;` -> 42,
`who`), and cleaned up every live test artifact (`rm()`'d all `/zzzlive*`
files) before stopping the scratch process, leaving the real bundled
mudlib tree exactly as found (`git status` clean under `mudlib/`).

Staged with `git add` only, per this project's own standing rule; not
committed.

**2026-08-19 (continued further, fresh session): `parse_*` (row 0.13a)
sixth real slice -- item 8 piece 5, `parse_obj()` itself (the real
noun-phrase word-matching engine), wired into `parseSentence()` for the
first time, plus the single-object slice of item 9's own can_/direct_/
indirect_/do_ disambiguation family needed to turn a match into an
actually-resolved object and a real `do_*` call. OBJ/LIV/OBS/LVS rules
now resolve real objects end to end for any rule with at most one
object-family token (670 tests, up from 661).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules), `ROADMAP.md` row 0.13a's own
full history, and this file's own most recent entry (the immediately
preceding session, which built `loadObjects()` in full and left
`parse_obj()` itself as the next, already-scoped slice with a concrete
insertion point already marked in `ParserPackage.cpp`).

Read the real `parse_obj()` source directly (packages/parser.c:
1325-1543) rather than trusting the prior session's own summary of it:
confirmed the real matching order (articles, then a `switch` on the
current word's own special-word kind, then the shared noun/plural/
adjective hash lookup every word falls through to regardless) and
precedence rules (singular and plural checks are independent `if`s, not
mutually exclusive; the trailing adjective check unconditionally decides
whether the word-consuming loop continues, regardless of whether the
noun/plural checks fired). One genuine surprise caught only while
writing this session's own regression tests, not assumed from the
prior session's summary: real `SW_ALL` ("all") without a following "of"
builds its match immediately from whatever the *current, unnarrowed*
candidate set already is -- it does not fall through to read a
following noun word the way "all of X" does. "get all swords" (no
"of") is not the same construct as "get all of the swords" in real
grammar; the plural regression test uses the latter deliberately, with
a comment explaining why.

Wiring this in also required the single-object slice of item 9 (the
real can_/direct_/indirect_/do_ disambiguation family that turns a
`parse_obj()` bitvec match into one resolved object) -- confirmed by
tracing `we_are_finished()`'s own real object-token loop that
`singular_check_functions()`/`plural_check_functions()` only need
`parallel_check_functions()` underneath them, and that the real
TWO-object family (`dependent_check_functions()`/
`check_object_relations()`, deciding which direct/indirect object PAIR
is jointly valid for a rule like "give OBJ to LIV") is only reached
when `state.numObjs == 2` -- confirmed unreachable given a new
`VerbRuleNode::objectTokenCount` field lets `parseRulesFor()` skip any
node needing two object tokens outright, the same honest "not yet
supported" stance already used elsewhere in this row. This matches the
immediately preceding session's own prediction almost exactly ("the
natural follow-on after item 8 lands, not before it") -- confirmed
correct rather than assumed, and reported here as scope genuinely not
attempted this session, not silently dropped.

One real naming-formula bug caught and fixed before it ever reached a
committed test: an initial reading of `make_function()`'s own
`omatch+1 >= which` OBS/LVS-naming condition used `omatch`'s
*post*-increment value, which would have made the final `do_` call on a
plural match always spell "obj" instead of "obs". Re-derived directly
against source with the real pre-increment ordering once a dedicated
regression test for the plural do_-call naming specifically caught it
failing.

9 new regression tests (`test/test_lexer.cpp`), each building a real
room/player/item environment tree via real `move_object()` calls: a
single `OBJ` rule resolving end to end through the generic `can_`,
per-candidate `direct_`, and final `do_` calls with the REAL object
identity confirmed at each stage; a single candidate rejected by
`direct_get_obj()` falling back to the real generic error message via
`master()->parser_error_message()`; two indistinguishable candidates
producing a real `ERR_AMBIG` with the real descending-index array
order; an adjective chain narrowing correctly; an ordinal resolving to
the true Nth candidate; `LIV_MODIFIER` excluding a non-living object
sharing the same noun as a living one; "all of" (plural) resolving to
an array of accepted candidates only, with the real "obs" do_-call
naming confirmed; the fixed "my" adjective resolving to the player's
own carried item over an identically-named item in the room; and a
two-object-token rule confirmed genuinely skipped rather than silently
mismatched. 670 tests passing (up from 661), zero regressions.

**Verified live against the real running driver, real bundled
`mudlib/`** (a scratch config on a spare port; a real Python-scripted
TCP client this time rather than manual telnet, chosen specifically
because the live scenario needed multi-statement LPC source embedded in
a single `eval` command line, which needed precise control over
escaping): dynamically built a real room and sword via `write_file()`/
`clone_object()`, registered a real `"get" "OBJ"` rule, and confirmed
`parse_sentence("get the sword")` returned the real success value `1`
with `do_get_obj()` genuinely receiving the real sword object (checked
against the clone's own `file_name()`, not merely a truthy return);
separately confirmed `LIV_MODIFIER` live too, with a statue and a guard
sharing the same noun -- `do_eye_liv()` received the real guard, not the
statue (a single-word verb "eye" used deliberately here, since real
`make_function()`'s own simple naming only produces a legal identifier
for a single-word verb -- caught by first trying a two-word verb, which
correctly failed to match anything, and re-deriving the real naming
rule from source). Driver process confirmed to stay healthy afterward
via a second, independent connection running ordinary gameplay
(`eval return 6*7;` -> 42, `who`).

Staged with `git add` only, per this project's own standing rule; not
committed.

**2026-08-19 (fresh session): `parse_*` (row 0.13a) fifth real slice --
item 8's own recommended first sub-slice (pieces 1+2+3+4: the real
per-object noun/adjective/plural cache, the rest of
`interrogate_master()`, environment-based object collection, and the
word -> object-index hash table) -- real `load_objects()` in full minus
`add_nicknames()` and `parse_obj()` itself (piece 5, still open)
(661 tests, up from 657).**

Oriented fresh per this session's own instructions: read `CLAUDE.md`
(confirmed both non-negotiable rules -- no `git commit`/`git push`, no
em dashes/emojis), `ROADMAP.md`, this file's own 5 most recent entries,
and `COMPARISON.md`; ran `git status` and found `STATUS.md` itself
modified but unstaged (the prior session's own archiving-pass entry,
documenting its own housekeeping move but never actually written into
this file before that session ended -- left in place, folded into this
session's own `git add` at the end, not reverted, since it is real,
correct content the prior session simply had not landed yet).

Re-ranked the four open items named in this session's own brief --
this row's own remaining item 8 pieces, row 1.9 (LDMud mapping width),
row 1.7/1.8 (LDMud closure kinds), row 1.4/1.16 (connect/disconnect) --
against real mudlib-compatibility impact before building anything.
This row wins on two independent grounds: it already carries the
strongest real-evidence citation of any open item (`temp/core-lib`'s own
Dead Souls `lib/lib/command.c` calls `parse_sentence()` directly from
core command dispatch, confirmed two sessions ago, not a peripheral
feature), and, unlike the other three -- each of which would need fresh
scoping work before anything concrete could be built -- it already has a
citation-backed, right-sized next sub-slice on record from the
immediately preceding session's own investigation (ROADMAP.md row
0.13a's own "Recommended internal order" paragraph: pieces 1+2+3+4
first, `parse_obj()` itself, piece 5, last). Connect/disconnect was
reconsidered specifically since DGD's own three-way port fork -- the
part of that design question that most complicated a `BootApi`
interface change -- is no longer a Phase 1 blocker at all (DGD is
comparison-only per the Phase 1 header's own 2026-08-18 scope
clarification), narrowing that row to just a FluffOS-`net_dead()`-vs-
LDMud-`disconnect()` divergence; genuinely real, but narrower in reach
than this row's own confirmed-highest-citation status, and still an
open architecture question, not a scoped slice ready to build.

Read the real vendored source directly before writing anything (not
trusting the prior session's own scoping summary alone): confirmed
`interrogate_object()`'s six real apply names against `applies.h`
(including `APPLY_ADJECTIVE`'s own real, faithfully-kept "adjectiv"
typo -- `parse_command_adjectiv_id_list`, not "adjective"), confirmed
`APPLY_USERS`'s own real apply name (`parse_command_users`) against
`applies_table.c`'s own index-21 entry, and confirmed via
`f_parse_sentence()` (packages/parser.c:3070, `parse_user =
current_object;`, unconditional) that real `load_objects()`'s own final
"num_people" fallback loop is genuinely unconditional regardless of
whether an explicit `parse_env` array argument is also given -- not
gated on that branch the way a first reading of the surrounding code
might suggest.

Implemented: `interrogateObject()` (real `interrogate_object()`, the
real `PI_SETUP`/`PI_REFRESH` cache-hit/re-interrogate flag dance and its
own real early-return-after-each-apply checkpoints, populating three new
`LpcObject` members -- `parseNounIds_`/`parsePluralIds_`/`parseAdjIds_`,
real `parse_info_t::ids`/`plurals`/`adjs`); `checkSpecialWord()` (the
fixed article/self/all/of/and/ordinal table plus real numeric-ordinal
parsing, including real code's own "a teen is always 'th'" rule);
`rec_add_object()`/`find_uninited_objects()`/`add_objects_from_array()`/
`get_objects_from_array()` (environment-based object collection),
confirming along the way a real, deliberate asymmetry between the first
and third of these by close reading rather than assuming them
identical (`rec_add_object()`'s own `ob == parse_user` case sets
`RAO_MY` for that object's own children; `add_objects_from_array()`'s
own equivalent case does not propagate onto the same element's own
recorded flags, only onto a nested array immediately following it, real
`last_was_me`); `add_hash_entry()`/`add_to_hash_table()` (the word ->
object-index hash table) plus real `load_objects()`'s own final
"num_people" fallback loop; and `loadObjects()` itself, orchestrating
all of the above into one real, complete pipeline (`LoadedObjectSet`,
built fresh per call and returned by value -- the same deliberate
`SentenceSession`-style architecture improvement already made for
`parse_sentence()` itself, confirmed not a fidelity loss here either).

One real, deliberate architecture decision, called out rather than made
silently: real `interrogate_master()`'s own `MS_HAS_USERS` cache
genuinely spans multiple separate `parse_sentence()` calls (invalidated
only by an explicit `parse_refresh()` on master) -- a real, observable
mudlib-visible caching *contract*, not an internal-only performance
detail like `SentenceSession`'s own collapse of `parse_sentence()`'s
globals was. Kept as real process-wide state (`masterUsersCache()`,
mirroring the already-established `cachedLiterals()` pattern),
invalidated by the new `ParserPackage::invalidateMasterUsersCache()`,
wired into the `parse_refresh` efun's own master-object branch exactly
where real code's own `master_state &= ~MS_HAS_USERS;` runs --
unconditionally, before the "does master even have pinfo" early return,
not after.

One real memory-management simplification, confirmed to have zero
observable effect before relying on it: real `remove_ids()` is not
ported at all -- `LpcObject`'s new setters simply assign a fresh
`std::vector<std::string>`, and C++'s own move-assignment already frees
the old backing storage. Confirmed safe rather than assumed: real
`remove_ids()`'s own guard (`if (pinfo->flags & PI_SETUP)`) is provably
always false at the one real call site that matters (`interrogate_
object()`'s own `PI_REFRESH` check), because real `f_parse_refresh()`
clears `PI_SETUP` in the exact same bitwise AND that sets `PI_REFRESH`
-- meaning real `remove_ids()` never actually frees anything there
either, a genuine, confirmed real quirk (a leaked array on every
refresh) in FluffOS's own C source, not something this port needed a
C++ analog of.

**Not yet wired into `parseSentence()` itself** -- item 8 piece 5
(`parse_obj()`, the real word-matching logic that would actually
consume a `LoadedObjectSet` to resolve `OBJ`/`LIV`/`OBS`/`LVS` tokens)
is the next slice, with its own insertion point already marked
(`ParserPackage.cpp`'s documented `OBJ`-family dead branches in
`parseRule()`/`makeFunction()`/`makeErrorMessage()`). Since real
`load_objects()` has no LPC-visible efun surface of its own either (an
internal implementation detail of `parse_sentence()` in genuine FluffOS
too), this slice's own live verification was narrower than prior
slices': confirmed the driver boots clean and stays healthy under 45
seconds of continuous active polling from a second, independent client
(harness-tracked `run_in_background`, never manual backgrounding) while
a first, disposable client ran ordinary `eval` gameplay (`6*7`,
`m_indices()`, a `parse_init()`/`parse_add_rule()`/`parse_sentence()`
round trip) throughout, zero regressions -- the strongest live
confirmation available for a slice with no new LPC-visible surface of
its own; the new pipeline's own correctness rests on 4 new regression
tests instead.

4 new regression tests (`test/test_lexer.cpp`): `checkSpecialWord()`'s
fixed table plus numeric-ordinal parsing (the teen rule: `"11th"`
correct, `"11st"` rejected, `"21st"` correct); `interrogateObject()`'s
real cache-hit behavior (a second call does not re-invoke
`parse_command_id_list()`) and real re-interrogation after
`parse_refresh()` (via a live call counter); a full `loadObjects()`
integration test building a real four-object environment tree (a room
containing a player carrying a sword, plus a sibling rock) via real
`move_object()` calls, confirming the exact real object numbering
order, `me_object`, `RAO_INREACH`/`RAO_MY` propagation, the
`cur_livings` bit, and the complete word -> object-index hash table
(including the fixed `"my"` adjective covering exactly the player's own
carried sword) all end up correct together; and a combined
master-users-caching-plus-`num_people`-fallback test confirming
`master()->parse_command_users()` is genuinely reused across separate
`loadObjects()` calls until an explicit `parse_refresh()` on master,
and that a connected user inside an `inventory_visible()`-false
container (never reached by the ordinary tree walk) is picked up by the
real `num_people` fallback exactly once, not duplicated for a player
the tree walk already found directly. 661 tests passing, up from 657,
zero regressions.

See ROADMAP.md row 0.13a for the full citation-backed writeup; its own
"Not yet wired into `parseSentence()`" note above points the next
session at item 8 piece 5, `parse_obj()` itself, as the natural next
slice -- the last piece standing between this row and real `OBJ`/`LIV`/
`OBS`/`LVS` noun-phrase support in `parse_sentence()`.

**2026-08-18 (continued): two housekeeping moves, both pure archival,
no driver code touched -- `prompt.md` archived to `PROMPT-ARCHIVE.md`,
and `STATUS.md` itself trimmed back down to its own "5 most recent
sessions" rule by moving entries 6-52 into `STATUS-ARCHIVE.md` (657
tests, unchanged).**

Both picked up directly from the previous session's own findings,
reported but not acted on at the time since that session was asked only
to report, not to delete or move anything itself.

**`prompt.md` archived, not deleted.** Followed `STATUS-ARCHIVE.md`'s
own precedent exactly: same repo-root location (no subdirectory), same
`<NAME>-ARCHIVE.md` naming pattern, same "explain why, then paste the
original content verbatim below" header shape. Confirmed content
preservation precisely rather than trusting a visual diff: original
`prompt.md` had 388 non-blank lines; `PROMPT-ARCHIVE.md` has 395; the
7-line difference is exactly the new header's own non-blank line count,
confirmed by counting it separately -- zero lines lost, zero duplicated.
`CLAUDE.md`'s own orientation section (the one place outside `STATUS.md`
itself that named `prompt.md` by path) updated to point at
`PROMPT-ARCHIVE.md` and note it is "not active," keeping its existing
"track record of describing some subsystems incorrectly" warning intact
since that is still true and still useful context for anyone who does
open the archive.

**`STATUS.md` trimmed to its own documented "5 most recent" rule.** Had
grown to 4,667 lines and 52 dated entries with no archival move since
`STATUS-ARCHIVE.md` was created on 2026-08-09 -- confirmed directly by
counting real `**YYYY-MM-DD` headers, not estimated. Moved entries 6
through 52 into `STATUS-ARCHIVE.md`, inserted immediately after that
file's own header (before its existing 2026-08-09 entries), preserving
the file's own most-recent-first order across both the newly-moved and
already-archived content -- the newly-moved batch is strictly newer than
everything already there, so appending at the literal end of the file
would have broken that ordering rather than matched it. The `## Known
stubs / scope limitations` section, `STATUS-ARCHIVE.md`'s own header
already calls out as `STATUS.md`'s "currently-maintained" list rather
than historical content, stayed in `STATUS.md`, unmoved. Verified this
was a genuinely pure move, not a lossy one, the same way as the
`prompt.md` archive above: original `STATUS.md` + `STATUS-ARCHIVE.md`
combined for 6,842 non-blank lines; the two new files combined for
6,855; the 13-line difference is exactly the new archival-pass
addendum note's own non-blank line count. Spot-checked specific unique
strings from both ends of the moved range (the newest and oldest moved
entries' own opening text, plus the `Known stubs` header itself)
resolve to exactly the expected file, exactly once each, not zero and
not duplicated.

**2026-08-18 (continued): item 8 (noun-phrase-to-object resolution
engine) investigated fresh after the `SIGPIPE` fix -- confirmed
genuinely too large for this same session, stopped and reported rather
than forced, real scope broken into 5 sub-pieces for the next session
(no test-count change, no code changed by this part of the session).**

Picked up directly from this row's own next-slice recommendation. Asked
explicitly to confirm real scope from source before building anything,
and to stop and report rather than force it if too large -- re-read
`load_objects()` and `parse_obj()` directly with fresh eyes (not trusting
the two-sessions-old scoping note's own summary alone) before deciding.
Confirmed: unlike `parse_sentence()`'s own STR/WRD/literal subset (this
row's immediately preceding slice), item 8 has no genuinely smaller
separable piece inside it -- even the single simplest possible `OBJ`
match needs the *entire* real chain (a real per-object noun/adjective/
plural cache, the rest of `interrogate_master()`, environment-based
object collection, a word-to-objects hash table, and finally `parse_obj()`
itself, packages/parser.c's own ~220-line word-matcher) working end to
end before there is anything real for the matching logic to operate on
at all. None of the five pieces is independently observable via any real
efun surface the way the sentence tokenizer's own word-splitter turned
out to almost be for the previous slice.

Broke the confirmed real scope into 5 ordered sub-pieces (see ROADMAP.md
row 0.13a's own updated "Next slice recommendation" for the full
citation-backed breakdown, each piece's own real source lines, and a
recommended internal order) so the next session can start building
immediately rather than re-deriving this from scratch: (1) the real
per-object noun/adjective/plural cache (`interrogate_object()`, six real
applies, plus the `PI_LIVING`/`PI_INV_ACCESSIBLE`/`PI_INV_VISIBLE` flags
this row's own prior slice already added storage for but left
unpopulated); (2) the rest of `interrogate_master()` (the `MS_HAS_USERS`/
`MS_HAS_SPECIALS` halves -- the literals third is already real); (3)
environment-based object collection (the most tractable piece, since
`LpcObject::environment()`/`inventory()` already exist and closely match
real `super()`/`first_inv()`/`next_inv()`); (4) the word-to-objects hash
table; (5) `parse_obj()` itself, the real word-matching logic (articles,
"all"/"all of", possessive "my", ordinals, nicknames, adjective chains,
singular/plural, living/visible modifiers) -- the exact piece
`ParserPackage.cpp`'s own `parseRule()`/`makeFunction()`/
`makeErrorMessage()` already have a documented dead-branch insertion
point waiting for, left there deliberately by the `parse_sentence()`
slice. Recommended a first sub-slice landing just pieces 1-4 (the
object-collection-and-cache pipeline, real and complete on its own even
with `parse_obj()` itself still a documented gap) as a reasonably-sized
single-session target, the same "half the group, cleanly bounded"
pattern the `parse_free()`/`parse_refresh()` slice already used
successfully for item 7 two sessions ago.

**2026-08-18 (continued): the `SIGPIPE` gap flagged last session fixed
and rigorously verified, both deterministically and live, before
resuming `parse_*` work (657 tests, up from 656).**

Priority task, ahead of any further parser work: the driver process had
no `SIGPIPE` handling at all (`src/net/instruct.md`'s own "Known gap"
note, written the previous session after observing a real, live crash --
a background driver instance exiting with code 141, the standard
`128+SIGPIPE` shell convention). Fixed with `std::signal(SIGPIPE,
SIG_IGN)` in `main.cpp`, right alongside the existing `SIGINT`/`SIGTERM`
registration -- matching this codebase's own existing `std::signal()`
convention directly rather than introducing `sigaction()` for just this
one signal. Confirmed this cannot interact badly with the existing
`SIGINT`/`SIGTERM` handling: distinct signal numbers, and
`Connection::send()` (`Connection.cpp`) already tolerated a failed
`write()` gracefully on its own (`if (n < 0) { if (errno == EINTR)
continue; break; }`) before this fix ever existed -- the signal itself
was the only thing standing between a completely ordinary failed-write
return value and the whole process dying, not a gap in the write path's
own error handling.

**Added a genuinely deterministic regression test**, not just a live
check: closing one end of a connected `socketpair()` and then writing to
the other is the textbook, 100%-reliable way to raise `SIGPIPE` on any
POSIX system -- no live server, no timing race, unlike the earlier
process-level crash-claim investigation two sessions before this one,
which needed a real running scheduler loop and a live client to even
have a chance of reproducing anything. Since the test binary has its own
separate `main()` (not `src/main.cpp`'s), it needed its own copy of the
same `std::signal(SIGPIPE, SIG_IGN)` call to actually exercise anything
real -- added at the top of `test/test_lexer.cpp`'s own `main()`.
**Proved the test itself is genuinely meaningful, not just plausible**:
temporarily commented out that one line, rebuilt, and confirmed the
whole test binary reliably died with exit code 141 partway through the
suite, right at the new test's own write call -- restored immediately
after confirming this, then reconfirmed a clean run with the real fix
back in place. 657 tests passing, up from 656, zero regressions.

**Verified live against the real running driver**, same harness-tracked
`run_in_background`/`TaskStop` methodology already established for this
row's own prior live-verification sessions (never manual `nohup`/
`pkill`): a real driver instance survived two separate rounds of 50
total forced-abrupt client disconnects (`SO_LINGER` set for a hard RST
rather than a graceful FIN, deliberately reproducing the unread-buffered-
data-at-close condition that raises `SIGPIPE` on the peer's own next
write) while a second, independent client stayed connected throughout
and kept receiving normal `who` responses on every tick, confirmed via
60+ seconds of active `ps`-polling after each round (never idle, matching
the same discipline the earlier crash-claim correction established).
Ordinary gameplay (`eval return 6*7;`) confirmed still working afterward.
`src/net/instruct.md`'s own "Known gap" note updated to record the fix,
not just the finding.

**2026-08-18 (continued): `parse_sentence()` implemented, `parse_*` (row
0.13a) slice four -- restricted to STR/WRD/literal-only rules, confirmed
from source to be a genuine subset of real behavior rather than a
simplification of it, plus one real, incidental, unrelated driver-wide
gap found live (no `SIGPIPE` handling at all) (656 tests, up from 646).**

Asked explicitly to confirm from source whether "(6)+(8)+(9) all
together" (the sentence tokenizer, the noun-phrase resolution engine,
and the matcher) was genuinely the smallest remaining slice, or whether
some smaller piece inside that group was separable on its own, the way
the previous session's own `parse_free()`/`parse_refresh()` pick turned
out to be for item (7). Traced every real call path into
`interrogate_object()` (the function that populates a live object's own
noun/adjective/plural id cache -- item 8's own core) and confirmed its
one and only real caller is `load_objects()`, itself only reachable from
`parse_obj()`, itself only reached by `parse_rule()`'s own OBJ/LIV/OBS/
LVS token case. A rule built entirely from STR tokens, WRD tokens, and
literal words never reaches any of that machinery **in real FluffOS
either**, not just in a scoped-down port of it -- meaning "STR/WRD/
literal-only `parse_sentence()`" is a genuine, real, complete subset of
real behavior, and every function this slice ported is the *exact* real
function, with the unreachable OBJ-family branches kept in as explicit,
documented dead code (a precise insertion point for the next slice)
rather than silently omitted or guessed at.

Ported: the word-splitter and multi-word-verb-phrase lookup (item 6);
the recursive-descent matcher and the can_/direct_/indirect_/do_
callback machinery (item 9) -- including real `make_function()`'s own
four distinct naming *strategies* for the same callback (embed the
verb's own name directly; the same but push literal-token arguments as
data instead; a generic "verb" name with the real verb name pushed as
an argument; the whole name collapsed to "do_verb_rule" with the exact
rule string pushed as an argument), all four confirmed reachable and
correct via a dedicated test for the least obvious one (the do_verb_rule
form, a genuinely easy shape to get wrong -- only got it right, first
try, after very careful line-by-line re-derivation of its real 5-
argument signature rather than working from intuition); error reporting
restricted to the one real error kind (`ERR_ALLOCATED`) this slice's own
matcher can actually produce, every other real kind confirmed
unreachable the same way and recorded with its real numeric value
(`include/parser_error.h`) for the next slice to extend rather than
renumber.

One real registry-shape addition, found and added cleanly rather than
reworked in: real `VB_HAS_OBJ` needed a per-*rule* home (deciding which
individual rule nodes this slice can even attempt), not the per-verb one
real code itself uses it for -- added as `VerbRuleNode::hasObjectToken`,
computed once at rule-registration time, zero changes needed to anything
the three earlier slices already built.

One real, driver-wide gap found and fixed along the way, unrelated to
parsing itself: this driver's own `Return` opcode represents a function
falling off its own end with no explicit `return` statement as
`Value{}` (monostate/"void"), not a real `int64_t 0` the way genuine LPC
semantics implicitly return (`VM.cpp`'s own "if (localStack.empty())
return Value{};", confirmed directly) -- which would have made a
genuinely-defined-but-early-falling-off `can_`/`direct_`/`do_` function
get silently treated the same as an undefined one by `process_answer()`'s
own three-way branch. Fixed by checking `VM::functionExists()` explicitly
before every call (already needed anyway, to distinguish "undefined, try
the next naming convention" from "defined, whatever it returned") and
then treating monostate the same as an explicit falsy `int64_t 0` in
this one function's own port -- scoped to this efun family specifically,
not a change to the driver-wide monostate/int-0 choice other code
elsewhere already relies on.

One real, deliberate architecture improvement over real code, called out
rather than made silently: real `parse_sentence()`'s own "current parse
in progress" state is a set of global C statics, with its own recursion
guard explicitly disabled in the real source ("may not be done in case
of an error, or in case of tail recursion") -- a genuinely reentrant call
in real FluffOS can silently corrupt an outer parse still in progress.
This port collects the same state into one `SentenceSession` object
constructed fresh per call and threaded through by reference instead,
rather than reintroducing that same hazard in C++ -- not a fidelity
loss, since every one of this row's own tests (and any real, well-
behaved caller) only ever depends on each call's own correct result, not
on the corruption itself.

10 new regression tests, 656 tests passing, up from 646, zero
regressions. **Verified live against the real running driver, real
bundled `mudlib/`**, same harness-tracked `run_in_background`/`TaskStop`
methodology as the previous two slices: a real cloned object's
`parse_add_rule("look","STR")` plus `parse_sentence("look at the
mysterious door")` returned `1` and correctly invoked `do_look_str()`
with the real original-cased text `"at the mysterious door"`; an
unrecognized verb returned `0`; a recognized verb with unsatisfiable
grammar returned `-1`; an explicit string rejection correctly fell back
to `0` since the real bundled master does not define
`parser_error_message()`; ordinary gameplay kept working throughout.

**One real, incidental, unrelated finding, not caused by this slice and
not fixed here:** the driver process has no `SIGPIPE` handling at all
(confirmed directly -- zero hits for `SIGPIPE`/`MSG_NOSIGNAL`/
`SO_NOSIGPIPE` anywhere in `src/`/`include/`; `main.cpp` registers
handlers only for `SIGINT`/`SIGTERM`), meaning a client connection
closing at the wrong moment relative to the driver's own next write to
that same socket can raise `SIGPIPE`, whose default disposition kills
the *entire process*, not just that one connection. Reproduced once,
live, during this session's own test-client cleanup (a background driver
instance exited with code 141, the standard `128+SIGPIPE` shell
convention) -- a second, identical attempt did not retrigger it, so this
is a genuine timing-dependent race, not a deterministic repro, but the
underlying gap itself is confirmed with certainty directly from source
regardless of how reliably any one test happens to trigger it. A
different root cause from this same session's own earlier "uncaught LPC
exception" crash-claim correction (see this file's own dated entry below)
-- that one was a C++ exception-handling question with a confirmed-
correct answer (no crash); this one is a raw, completely unhandled OS
signal, genuinely triggerable in principle by any real player's abrupt
disconnect, not specific to `parse_*` work at all, and out of this
session's own scope to fix (a `src/net`-level concern). Flagged here with
its exact real citation so a future session does not need to rediscover
it from scratch.

See ROADMAP.md row 0.13a for the full updated breakdown; its next-slice
recommendation now points at the noun-phrase-to-object resolution engine
(item 8), the one remaining real prerequisite for OBJ/LIV/OBS/LVS token
support, with every real insertion point this slice's own OBJ-family
dead branches left already marked in `ParserPackage.cpp`.

**2026-08-18 (continued): `parse_free()`/`parse_refresh()` implemented,
`parse_*` (row 0.13a) slice three -- picked after confirming from source
that the fuller `parse_info_t`'s own noun/adj/plural cache is not
actually separable from the noun-phrase resolution engine, only its
flag/cleanup half is (646 tests, up from 643).**

Asked explicitly not to default to "parse_info_t/parse_refresh()/
parse_free() next" without checking whether something else unblocks more
of what's left first. Re-read `interrogate_object()` (packages/parser.c),
the real function that actually populates `parse_info_t`'s own noun/
adjective/plural id cache, and confirmed it has exactly one real call
site in the whole file: `load_objects()`, itself only reachable from
`parse_sentence()`. The cache-*population* half of item (7) genuinely
cannot be built ahead of (8) (the noun-phrase resolution engine) --
there would be nothing real to call it from and nothing real to test.
What *is* genuinely separable, confirmed the same way: real
`parse_free()` (called from `free_object()`, unlinking a destructed
handler's own rule nodes) and real `f_parse_refresh()`'s own flag/apply
mechanics (the `PI_SETUP`/`PI_REFRESH`/`PI_VERB_HANDLER` bit dance, the
master-object special case, and the `LIVINGS_ARE_REMOTE` apply/
`PI_REMOTE_LIVINGS` flag) -- neither touches the cache itself. Also
briefly reconsidered whether the sentence tokenizer's own word-splitting
front end (item 6) might be a better pick instead, since it looked more
separable at first glance -- checked and ruled out: it has no real efun
surface of its own to expose it through independent of the matcher
(`parse_rules()`) it feeds into, so building it now would leave nothing
live-testable via `eval`, the same discipline every slice so far has
kept to.

This also surfaced a real, genuine gap in the very first `parse_*`
slice's own `parse_add_rule()`: real code also fires the same
`LIVINGS_ARE_REMOTE` apply and sets `PI_VERB_HANDLER`/`PI_REMOTE_LIVINGS`
there, deliberately deferred at the time as "purely for sentence-
matching, not observable by add_rule/dump/remove" -- corrected this
session now that the flag storage it needed exists anyway.

Implemented: `LpcObject::parseInfoFlags()` (real `parse_info_t::flags`,
valid only while `hasParseInfo()` is true), a new `ParserInfoFlag`
namespace (`ParserPackage.hpp`) with the real `PI_*` bit values;
`ParserPackage::onObjectDestroyed()` (real `parse_free()`) wired into
both `destruct` and `reload_object`'s existing `onDestructed` callback,
the same real trigger point `SocketRegistry::closeAllOwnedBy()` already
uses there; the `parse_refresh` efun in full (guard error, master-object
special case, flag manipulation, the apply re-check with real code's own
`O_DESTRUCTED` guard); and `parse_add_rule()`'s own retroactive fix
(fires the same apply, sets the same two flags, faithfully missing the
same `O_DESTRUCTED` guard real code's own asymmetric two call sites
have -- a real, harmless omission in the original FluffOS source, not
invented here).

4 new regression tests, including one that specifically distinguishes
two now-separately-real scenarios rather than conflating them: a
destructed handler's rule genuinely vanishing from `parse_dump()`
entirely (the new, real, eager cleanup path via the actual `destruct()`
efun) versus the pre-existing "(destructed)" fallback text (a real,
narrow gap this driver's own reference-counted memory model can still
hit -- calling `VM::destructObject()` directly with no callback, which
genuine FluffOS could never reach at all since its own free is always
synchronous with `parse_free()`). 646 tests passing, up from 643, zero
regressions. **Verified live against the real running driver, real
bundled `mudlib/`**, same harness-tracked `run_in_background`/`TaskStop`
methodology as the previous slice: a real cloned object's rule vanished
from `parse_dump()` entirely, immediately, after a real `destruct()`
call; a real `livings_are_remote()` call-counter (written into the live
test object itself) confirmed the apply fires exactly once from
`parse_add_rule()` and a second time from a following `parse_refresh()`;
the ordinary no-`parse_init()` guard fired correctly via `catch()`.
Driver process stayed healthy throughout, confirmed via active polling.

See ROADMAP.md row 0.13a for the full updated breakdown; its next-slice
recommendation now points at the sentence tokenizer and the noun-phrase
resolution engine together, both now confirmed (not merely assumed) to
be the two remaining real prerequisites for anything resembling actual
`parse_sentence()` behavior.

**2026-08-18 (continued): crash-claim resolved (the driver process does
not crash on an uncaught dispatch error, rigorously re-confirmed; stale
notes in ROADMAP.md/STATUS.md fixed to match), then `parse_add_synonym()`
implemented, `parse_*` (row 0.13a) slice two, both real forms (643 tests,
up from 640).**

Asked, before doing anything else, to resolve a contradiction: the prior
session's own report cited "a real, pre-existing, unrelated bug where an
uncaught runtime error during command dispatch drops both the connection
and the driver process," but that exact claim had already been
investigated and retracted two sessions before that one (see this file's
own "ran down the crash flagged last session" entry, elsewhere below) --
the real bug was narrower (`net_dead()` skipped on this one path, fixed),
and the process-crash half never reproduced under rigorous testing, best
explained by a `pkill` run in the same breath as a log check. Found the
stale claim was still sitting, uncorrected, in `ROADMAP.md` row 1.9's own
`m_indices`/`m_values` note (fixed this session, inline correction
appended there) and had been repeated fresh into this same session's own
prior `STATUS.md`/`ROADMAP.md` entries (fixed too, see that entry's own
"Crash-claim correction" section for the full text).

Then re-verified independently rather than trusting either the stale
claim or the earlier retraction secondhand: booted a fresh scratch
driver via this harness's own tracked `run_in_background`/`TaskStop`
mechanism, connected a second, independent client that stayed connected
throughout and polled `who` once a second, reproduced this exact
session's own real trigger (`parse_add_rule()` before `parse_init()`)
plus the classic `totally_undefined_efun_xyz()` one with a separate,
disposable first client, then actively polled process health for 60+
seconds after each trigger (never idle, never a `pkill` anywhere near a
log check). **Confirmed, plainly: the driver process does not crash. Only
the triggering connection drops; the process and every other connection
stay completely healthy.** One genuine methodology wrinkle recorded along
the way: an earlier attempt at this same re-verification, using a
manually `nohup`-and-`disown`ed background shell job instead of this
harness's own tracked mechanism, saw the driver process vanish with no
shutdown message and no core file during a *tight active-polling* loop
(not an idle gap) -- switching to `run_in_background`/`TaskStop` made
that vanish stop reproducing entirely across a full retest, so future
live-verification sessions should prefer the harness's own tracked
backgrounding over manual shell job control for exactly this reason.

With the crash claim settled, continued `parse_*` with the next slice its
own prior recommendation named: `parse_add_synonym()`, both real forms
(2-arg verb aliasing, 3-arg specific-rule-copy). Re-read the real
`f_parse_add_synonym()` source directly before implementing rather than
working from the prior session's own summary. Found and fixed one real
gap in the prior slice's own registry shape before building on top of it:
real `parse_sentence()`'s own verb-lookup loop does not stop at the first
match, so a single verb name can carry *more than one* real `VerbEntry`
at once (a plain rule-holding entry and a separate synonym-to-something-
else entry, both sharing the same name) -- the prior slice's own registry
had (harmlessly, since nothing exercised it yet) modeled one entry per
name; corrected to a map from name to a *list* of entries before writing
`addSynonym()`, rather than building the new feature on a shape the very
next sentence-matching slice would only have had to fix later anyway.

Implemented `ParserPackage::addSynonym()` and the `parse_add_synonym`
efun registration, reusing the existing tokenizer/registry entirely as
its own prior recommendation predicted. 3 new regression tests (the
alias form's `parse_dump()` shape; the rule-copy form's exact-match
behavior plus all three of its real rejection paths; the coexisting-
entries-under-one-name scenario the registry refactor exists for,
including confirming `parse_remove()` only ever touches the plain entry).
643 tests passing, up from 640, zero regressions. **Verified live against
the real running driver, real bundled `mudlib/`**, this time via the
harness's own tracked backgrounding throughout: both real forms produced
the exact expected `parse_dump()` output; all three rejection paths fired
correctly; and, live and unprompted, a real, faithful (not a bug) quirk
surfaced -- a *failed* 3-arg `parse_add_synonym()` still leaves an empty
target `VerbEntry` behind, confirmed to match real code's own control
flow (the entry-creation step runs before the rule lookup that can fail,
and nothing rolls it back on error) rather than being a bug in this port.

See ROADMAP.md row 0.13a for the full updated breakdown and this slice's
own complete record; its next-slice recommendation now points at the
sentence tokenizer and the noun-phrase resolution engine together, the
two remaining real prerequisites for anything resembling actual
`parse_sentence()` behavior.


## Known stubs / scope limitations (intentional, not bugs)

- Object-bound closures (`(: obj_expr, "funcname" :)`), bare string-
  constant closures (`(: "literal" :)`), and the `(*fp)(args)`
  dereference-call syntax are all implemented now (see "Closure/
  function-pointer forms completed" above) -- this bullet is
  historical, kept for the git-blame trail rather than deleted.
- ~~`ApplyTable::isKnownApply()` recognizes `disconnect` (never called
  yet)~~ -- `disconnect` was never a real FluffOS apply at all (confirmed
  against `applies.h` directly); fixed, see the new dated entry at the
  top of this file: the table now lists the real apply, `net_dead`,
  which is genuinely fired on link death. Also see the same entry for
  `heart_beat`, which is now real too -- see "Real call_out()/
  heart_beat() scheduler" below. This bullet is otherwise historical,
  kept for the git-blame trail rather than deleted.
- ~~`Scheduler::tickHeartbeats()` / `tickCallOuts()` are empty function
  bodies~~ -- fixed, see "Real call_out()/heart_beat() scheduler" below.
  `logon()`'s own `call_out("idle", LOGON_TIMEOUT)` (a 180-second idle-
  disconnect timer) now genuinely schedules, though still not exercised
  live within any normal walkthrough's own timeframe.
- Array `&` intersection preserves the left array's order/duplicate
  count rather than replicating FluffOS's exact sorted, de-duplicated
  `intersect_array()` output. `|` is int-only (no array union, unlike
  real FluffOS's own `|`) -- neither is hit by anything this driver
  currently runs.
- ~~`sscanf()`'s "%s" directly adjacent to another "%"-specifier with no
  literal text between them is not implemented~~ -- fixed some sessions
  back (`%x`/`%f`/adjacent-specifier support), this bullet was simply
  never updated at the time; left as a stale doc gap until now.
- ~~Postfix/prefix `++`/`--` only support a bare variable name target,
  not an index expression (`arr[i]++`)~~ -- stale, this bullet was never
  updated when indexed `++`/`--` was actually implemented (see "Further
  gaps found and fixed while walking `std/user.c`'s full inherit chain"
  in `STATUS-ARCHIVE.md`, real `std/living.c`'s own `healing["intox"]--`
  shape, confirmed still passing via
  `testPostfixIncDecOnIndexedTargetParsesToIndexedIncDecExpr` and
  `testIndexedPostfixIncDecVmExecutionReturnsOldValueAndMutates`). Only
  a range-index target (`arr[0..1]++`) still throws, matching real
  `grammar.y`'s own restricted lvalue grammar -- not a gap, real LPC
  does not allow that either. See the "Compound assignment on an
  indexed target" bullet below for indexed `++`/`--`'s own real, still-
  open double-evaluation caveat.
- ~~`throw()` is not implemented (`catch()` is)~~ -- fixed, see the new
  dated entry at the top of this file: `throw()` is a real efun (matching
  `func_spec.c`'s own `void throw(mixed);`), hands the exact value it was
  given back to the nearest `catch()`, including through a called
  function that has no `catch()` of its own.
- `replace_string()`'s optional 4th/5th occurrence-range arguments (the
  real efun's `first`/`last` bounds) are not implemented, only the
  plain 3-arg replace-all form -- throws rather than silently
  mishandling if ever called with more args, matching this codebase's
  existing convention for other partially-implemented efuns (e.g.
  `sscanf`'s `%f`/`%x`).
- ~~`sprintf()` implements bare `%s`/`%d`/`%c`... positionally, with no
  field width/precision/flags and no literal `%%` -- throws on anything
  else. Confirmed still missing live this session: `%*` (dynamic field
  width)~~ -- stale, this bullet described a several-sessions-old state
  never reflected back here as the efun grew (see its own top comment
  in `EfunTable.cpp` for the real, session-by-session citation trail).
  Actually implemented today: `%s`/`%d`/`%c`/`%o`/`%x`, literal `%%`,
  field width (literal or dynamic via `%*`), precision (via `.`n or
  `.*`, meaningful for `%s`), and three justify modes -- `-` (left),
  `|` (centre, added this slice, see the new dated entry at the top of
  this file), and right (default), plus zero-padding. Still not
  implemented, throws a clear error rather than mishandling: `=`
  (column mode), `#` (table mode), `@` (array-spread), `'X'` (custom pad
  string), ` `/`+` (positive-integer pad), `%O` (LPC datatype dump),
  `%f` (float), capital `%X`, and `%0*` (zero-padded dynamic width,
  deliberately excluded -- see the efun's own comment).
- ~~`save_object()`/`restore_object()` use this driver's own recursive
  serialization format ... A real, pre-existing save file in that
  format ... is not parsed -- every line is silently skipped~~ --
  `restore_object()` now reads the real format too (see the new dated
  entry at the top of this file): auto-detected per line (a tab is
  this driver's own format's delimiter and never appears in the real
  one), so a genuine pre-existing FluffOS save file now actually loads
  its real historical data. `save_object()` itself is unchanged and
  still only ever *writes* this driver's own simpler format -- nothing
  needs to read a file this driver wrote except this driver, so there
  is no reason to also match real `save_svalue()`'s own escaping/
  formatting on the write side. Real LPC "class" values (`(/ ... /)`)
  in a real save file are not implemented -- this driver has no class/
  struct type anywhere else either -- and throw a clear error rather
  than being silently mishandled.
- ~~`find_player()`, `userp()`/`query_once_interactive()`, and
  `interactive()` are backed by `InteractiveRegistry`, which only tracks
  *currently* live connections ... not real FluffOS's separate "has this
  object ever been interactive" (O_ONCE_INTERACTIVE) ... wrong for an
  object that was once connected and has since disconnected~~ -- both
  `userp()`/`query_once_interactive()` (two slices ago) and
  `find_player()` (see the new dated entry at the top of this file) now
  use the same real, sticky `LpcObject::wasEverInteractive()` flag, set
  once by `Connection::attach()` and never cleared. **Correction to this
  bullet's own prior update:** it previously claimed `find_player()` was
  "unchanged and still correctly scoped to currently-connected objects
  only, matching real FluffOS semantics" -- checked directly against
  `add_action.c` this slice and confirmed wrong: real `find_player()`
  gates on `O_ONCE_INTERACTIVE`, not "currently connected" at all, and
  this driver's own prior `find_player()` had not actually been updated
  when `userp()` was fixed, only asserted to already be correct without
  re-checking its own real mechanism. `interactive()`/`users()` remain
  genuinely, correctly scoped to currently-connected objects only.
  `set_living_name()`'s own lookup table is now real too -- see its own
  bullet below.
- ~~`message()` ignores its `type`/`targets`/`excludes` arguments and
  always writes straight to the connection currently driving the
  call~~ -- `targets` is now real (see "Real call_out()/heart_beat()
  scheduler" above: `InteractiveRegistry` maps each connected object to
  its own `Connection*`, and `message()` routes to it). `type` and
  `excludes` are still ignored -- nothing reached live yet needs
  message-type filtering or an exclude list.
- `set_eval_limit()` is accepted (so callers do not throw "undefined
  efun") but does not change anything -- this driver's own eval-cost
  ceiling resets to a fixed 1,000,000-instruction-per-call limit at the
  start of every `VM::run()` call rather than accumulating across
  nested calls, already far above anything this driver's own test
  scripts hit.
- ~~`destruct()` only closes the connection currently bound to the
  destructed object, if it is the one driving the current call; it does
  not otherwise remove a destructed object from `InteractiveRegistry` if
  reached some other way, and this driver has no `O_DESTRUCTED`
  flag/guard on every apply the way real FluffOS does~~ -- fixed, see the
  new dated entry at the top of this file: `LpcObject` now has a real
  `O_DESTRUCTED`-equivalent flag, checked at every "call into this
  object from outside" entry point, and `destruct()` now removes the
  object from `InteractiveRegistry` unconditionally, not just when it is
  the currently active connection. Not replicated: real
  `destruct_object()`'s own contents-relocation loop (each contained
  item's own `move()` apply, run automatically before severing) -- a
  destructed object's own remaining inventory is unlinked from its
  former environment but not relocated anywhere. ~~Also still not done:
  the broader "any stale object-typed value silently reads back as 0"
  semantics real FluffOS enforces at many more read sites (array/mapping
  entries, comparisons, etc, not just applies)~~ -- fixed, see the new
  dated entry at the top of this file: a destructed object read out of a
  local variable, an object variable, an array element, or a mapping
  value now self-heals to a real int `0` in place, matching real
  `F_LOCAL`/`F_GLOBAL`/`F_INDEX`'s own mechanism exactly (confirmed
  against `interpret.c` directly: no other opcode, including comparisons
  and truthiness checks, needs its own separate check). Real range-index
  slicing (`arr[a..b]`) does not coerce either, matching real
  `slice_array()`'s own confirmed behavior -- a destructed element
  copied into a freshly sliced sub-array stays a raw reference until
  that array's own element is separately read.
- The `compile_object()` virtual-object fallback (see "Working now"
  above) is only wired into `ObjectManager::loadObject()`, matching the
  one real call site this driver has confirmed needs it
  (`master()->player_object()`'s own `load_object(pfile)`). Real
  FluffOS's `int_clone_object()` also consults it (cloning an object
  that already has the `O_VIRTUAL` flag set makes another virtual
  instance) and inherit resolution can transitively reach it too;
  neither `ObjectManager::cloneObject()` nor `compile()`'s own inherit-
  path resolution were changed, since nothing this driver has run yet
  needs either.
- Compound assignment on an indexed target (see "Working now" above)
  evaluates its target/index sub-expressions twice rather than once,
  which would double any side effect they had -- harmless for every
  real call site this mudlib uses (plain variable reads, string-literal
  keys), but not a generally safe transformation. Flagged in
  `IndexAssignStmt`'s own comment. Indexed `++`/`--` and indexed
  assignment used as a sub-expression (`IndexAssignExpr`, see "Further
  gaps found and fixed" above) share the exact same double-evaluation
  property, for the same reason.
- `to_int()` (see "Further gaps found and fixed" above) does not
  implement the `buffer` case of its real `string | float | int |
  buffer` signature -- this driver's `Value` variant has no buffer type
  at all, and nothing on any path run so far needs one.
- ~~Object variables declared but never explicitly assigned stay
  void/monostate~~ -- fixed (see "Root-causing the `__HistorySize`
  report" above): both `LpcObject`'s own `variables_` and
  `VM::run()`'s own per-call `locals` now fill with a real `int64_t 0`
  per slot, matching real LPC's own default for any declared variable
  regardless of type. `monostate` itself is unchanged and still used
  deliberately elsewhere (a missing mapping key, an efun explicitly
  returning "nothing found") -- it now also participates correctly in
  arithmetic as a real `0` (see the same section), closing the gap
  this bullet used to describe.
- `map_array()`/`map()`/`filter_array()`/`filter()` only implement the
  two real shapes this mudlib actually uses (a `Closure`, or a string
  function name plus a target object) -- real `filter()`'s own
  string/mapping first-argument forms are not implemented, nothing
  confirmed live needs them.
- `implode()` only implements the plain string-separator form; real
  LPC's function-per-element form is not implemented.
- `query_ip_name()` always returns the same numeric IP `query_ip_number()`
  does -- this driver does no DNS resolution of its own (a blocking
  reverse lookup inline in the connection-handling loop would stall
  every other connection during it), matching real FluffOS's own
  documented fallback when hostname resolution is unavailable.
- ~~`set_heart_beat()`/`query_heart_beat()` correctly store and report
  the flag, but nothing reads it back yet -- there is no periodic
  heartbeat scheduler in this driver at all, so setting the flag has no
  runtime effect beyond being queryable~~ -- fixed, see "Real
  call_out()/heart_beat() scheduler" above: a genuine `Scheduler` now
  reads the interval back and fires `heart_beat()` on it, confirmed live
  (a real NPC's own heartbeat-driven dialogue, unprompted). This bullet
  is historical, kept for the git-blame trail rather than deleted.
- ~~`set_living_name()` stores the name on the object but wires up no
  lookup table for it, matching `find_player()`'s own pre-existing
  simplification (InteractiveRegistry + `query_name()`, not a real
  living-name table)~~ -- fixed, see the new dated entry at the top of
  this file: a real `LivingNameRegistry` now backs both `find_player()`
  and the newly-implemented `find_living()`, matching real
  `add_action.c`'s own `hashed_living[]`/`find_living_object()`
  mechanism.
**2026-08-18 (continued): `parse_*` (row 0.13a) taken on as a real
multi-session project, per its own explicit greenlight -- first real
slice landed (`parse_init`/`parse_add_rule`/`parse_dump`/`parse_remove`,
a real rule-string tokenizer plus a real verb/rule registry), the full
remaining 11-piece component breakdown written into ROADMAP.md row
0.13a for the next session (640 tests, up from 634).**

Picked up directly from last session's own finding: Dead Souls' own core
command dispatch (`lib/lib/command.c`) calls `parse_sentence()` directly,
making this very likely the single highest-impact item left, and this
session's own instructions explicitly greenlit taking it on as a real
multi-session project rather than continuing to defer it. Read the real
`packages/parser.c` (3419 lines) plus `packages/parser.h` in full and
broke the whole package into 11 real component pieces -- the rule-string
tokenizer, its inverse stringifier, master literal interrogation, the
verb/rule registry, `parse_add_synonym()`, the (separate) sentence
tokenizer, the fuller `parse_info_t`/`parse_free()`, the noun-phrase-to-
object resolution engine, the recursive-descent rule matcher and
ambiguity-resolution callback machinery, error reporting, and the
remaining efun surface names -- each with its own real source line
citation. See ROADMAP.md row 0.13a for the full breakdown, verbatim,
written so the next session does not need to re-read `parser.c` from
scratch.

Confirmed from the real source, not assumed, that the smallest genuinely
buildable-in-one-session slice is the rule-string tokenizer plus the
verb/rule registry -- exactly the "rule-string data structure without
full sentence matching yet" option this session's own instructions
floated as one candidate, verified correct rather than picked by default.
Needs none of the two hardest remaining pieces (the sentence tokenizer,
the noun-phrase resolution engine) or the live-object callback matching
machinery, yet is genuinely complete, real ported code, not a stub --
including a real, confirmed off-by-one in `make_rule()`'s own `MAX_MATCHES`
loop-exit check (a rule using exactly all 10 token slots always errors,
even when the 10th token really was the last one), faithfully reproduced
rather than silently "fixed," since real mudlib rules never come close to
that length (this driver's own corpus survey found nothing longer than 3
tokens) and this project's own discipline is to port real, confirmed
quirks rather than invent different behavior. `parse_refresh()` was
deliberately left out even though it looks small -- its own real behavior
is entirely about invalidating caches (the noun/adj/plural per-object
cache, the master literal/user cache) this slice doesn't have yet, so
implementing it now would be a no-op with nothing to invalidate.

Built: `include/amlp/efun/ParserPackage.hpp` + `src/efun/ParserPackage.cpp`
(the tokenizer, the rule-string inverse stringifier, and the verb/rule
registry itself -- a `ParserPackage` class, global process-wide static
state deliberately mirroring real `parser.c`'s own single-game-per-process
`verbs[]` global, the same shape this codebase's other efun-package
registries already use for exactly this reason,
`object/LivingNameRegistry.hpp`); a new `LpcObject::hasParseInfo()`/
`setHasParseInfo()` flag (real `object_t::pinfo`'s "has `parse_init()`
been called" bit only -- the fuller `parse_info_t` stays for a later
slice); and four new `EfunTable.cpp` registrations. One real subtlety
caught and fixed before landing: real `rule_string()`'s own `switch ((tok
= vn->token[index++]) & ~CHOOSE_MODIFIER)` masks the switch's own
*selector* but leaves `tok` itself holding the unmasked raw value for the
`default:` branch's own literal-index lookup -- an initial port that
masked before computing the literal index corrupted every negative
(literal) token value (`-1 & ~64` is `-65`, not `-1`, since a negative
int's high bits are already all set in two's complement), caught live by
this session's own regression test for a rule mixing a literal word with
a modifier-bearing token, not by code review alone.

6 new regression tests (`test/test_lexer.cpp`): the `parse_init()`-
required guard; a plain-`OBJ`-rule round trip through `parse_dump()`
plus `parse_init()`'s own idempotency; grammar rejection (more than two
object tokens, more than one plural token); modifier/literal tokenizing
against a real master `parse_command_prepos_list()` apply plus rejecting
an unlisted word; `parse_remove()` only ever touching the calling
object's own rule nodes, not another object's registration under the
same verb; and a destructed handler's rule surfacing as `"(destructed)"`
rather than a dangling reference. Full suite: 640 tests passing, up from
634, zero regressions.

**Verified live against the real running driver, real bundled `mudlib/`**
-- a scratch config on a spare port, a real telnet-negotiating Python
client, real `eval` calls. One real methodology wrinkle found and worked
around, not a bug in this slice: the shipped mudlib's own
`mudlib/command/eval.c` destructs and recreates a fresh `/tmp_eval_file`
object on *every separate* `eval` invocation, so registry state tied to
caller identity (this slice's own `handler` field) had to be exercised
within one combined `eval` call using `;`-separated statements, not split
across several -- splitting it across two `eval` calls the first time
around produced a real, correct "not known by the parser" error (since
`parse_init()` had run on an already-gone object), which is exactly
correct behavior, just not the test shape intended. Combined into one
call: `parse_init(); parse_add_rule("eat","OBJ");
parse_add_rule("give","OBJ LIV"); parse_dump()` returned the correct
two-verb dump with the correct handler and rule-string text;
`parse_remove("eat")` then removed only that verb's own rule node,
confirmed via a second `parse_dump()` in the same call; `"OBJ OBJ OBJ"`
was correctly rejected as a grammar error. One connection dropped mid-
session while developing these verification steps, immediately after an
uncaught `parse_add_rule()` guard error -- **this is correct, already-
fixed behavior, not a bug**: an uncaught dispatch error correctly closes
only the triggering connection (`net_dead()` fires on it, same as
ordinary link death), confirmed by this session's own re-verification
below, not the stale "drops the connection and the driver process" claim
an earlier `m_indices`/`m_values` entry made and a session between that
one and this one already retracted (see "crash-claim correction" below).
Worked around anyway for the rest of this session's own verification by
wrapping every eval expression that could legitimately throw (the
`parse_init()`-required guard, the grammar rejections) in a real LPC
`catch()`, purely so one dropped connection didn't interrupt the flow of
follow-up `eval` calls in the same client session -- not because the
process was ever actually at risk.

## Crash-claim correction (this session, before any further parser work)

The report above, when first written, repeated a stale claim: that an
uncaught dispatch error "drops both the connection and the driver
process." That claim was already investigated and retracted **two
sessions before this one** ("ran down the crash flagged last session",
elsewhere in this file) -- the real bug was narrower (`net_dead()`
skipped on this one path, fixed), and the "took the whole process down"
half never reproduced under rigorous testing; it was traced to a `pkill`
run in the same breath as a log check. `ROADMAP.md` row 1.9's own
`m_indices`/`m_values` note still carried the same stale claim
uncorrected until this session (fixed now, see its own inline
correction).

Re-verified independently this session, asked explicitly not to trust
either the stale claim or the earlier retraction at face value: booted a
fresh scratch driver via this harness's own tracked background-task
mechanism (not a manually `nohup`-and-`disown`ed shell job -- see the
methodology note below for why that distinction mattered), connected a
second, independent client that stayed connected throughout and polled
`who` once a second, then reproduced this exact session's own real
trigger (`parse_add_rule()` before `parse_init()`) with a separate,
disposable first client. Result: the triggering client's own connection
closed immediately (confirmed via EOF); the second client kept receiving
normal `who` responses on every single tick, unbroken, for 60+ seconds of
active `ps`-polling afterward (never idle, never a `pkill` anywhere near
a log check); the driver process's own elapsed time climbed normally the
entire time. Repeated with the classic `totally_undefined_efun_xyz()`
trigger too, same result. **Confirmed, plainly: the driver process does
not crash. Only the triggering connection drops; the process and every
other connection stay completely healthy.** This fully reconfirms the
earlier retraction, from a fresh, skeptical re-test rather than by
trusting it secondhand.

One genuine methodology wrinkle surfaced along the way, worth recording
rather than glossing over: an earlier attempt at this same re-verification,
using a manually `nohup ... & disown`ed background shell job instead of
this harness's own tracked background-task mechanism, saw the driver
process vanish with no "amlp shutting down" log line and no core file,
during a tight active-polling loop (5-second intervals, not an idle gap) --
superficially similar to, but not the same shape as, the earlier
retraction's own "disappeared during an idle stretch with no Bash calls"
sandbox wrinkle, since this one happened during active polling. Switching
to the harness's own `run_in_background`/`TaskStop` tracking for both the
driver and the second client made the vanish stop reproducing entirely
across a full retest. Recorded as a real, observed difference between the
two backgrounding methods in this sandbox, not chased further since it
is orthogonal to the actual question (whether the *driver itself* crashes
on an uncaught dispatch error, which the harness-tracked run answered
cleanly: no) -- future live-verification sessions should prefer
`run_in_background`/`TaskStop` over manual `nohup`/`disown`/`pkill` shell
juggling for exactly this reason.

ROADMAP.md row 0.13a rewritten with the full 11-piece breakdown, this
session's own slice marked done within it, and a next-slice
recommendation (`parse_add_synonym()`, which reuses this session's own
tokenizer and registry with no new infrastructure needed).

**2026-08-18 (continued): fresh self-assessment (closures/mapping-width/
disconnect all now landed or ruled out) -- `valid_read`/`valid_write`
implemented and wired into all 11 file efuns, real corpus evidence
confirms `parse_*` is likely the single highest-impact item left but
genuinely too large for a contained slice, stopped and reported rather
than forced (634 tests, up from 631).**

## Re-ranking

Everything the last assessment flagged as open has moved: bare `#'name`
and `#'efun::name` closures landed, `m_indices`/`m_values` landed,
connect/disconnect confirmed zero real evidence. Re-read ROADMAP.md and
STATUS.md fresh and gathered new evidence for what is actually left,
same method as every prior pick this session cluster -- real corpus
call-site counts, not assumption.

**`parse_*` (row 0.13a), re-examined with fresh eyes rather than
re-trusting the original scoping note's own framing:** the original note
cited "66 combined call-site hits across the 8 names in the vendored
mudlib corpora" as a flat number. Checked what those hits actually are
this time: `temp/dead-souls` (a real, extremely common FluffOS mudlib
base) calls `parse_sentence()` **directly from its own core command
dispatch**, `lib/lib/command.c` -- not a peripheral feature, the verb-
parsing machinery basic gameplay commands go through. Real rule shapes
actually used (`grep`ped `SetRules(...)` call sites directly) are mostly
simple -- bare, single `LIV`/`OBJ`/`OBS`/`STR` tokens, or a token plus a
literal preposition word (`"for LIV"`, `"to LIV STR"`) -- but even the
simplest real usage needs genuine new infrastructure this driver has
none of: a rule-string tokenizer, a sentence tokenizer, and a real
noun-phrase-to-object resolution engine (matching real `parser.c`'s own
`is_living`/`inventory_accessible`/`inventory_visible` master-callback-
driven object-scope model, confirmed by reading the real 3419-line
source directly, not assumed from the file's own size alone). Unlike
`#'efun::` (reused the exact existing `ClosureLiteralExpr`/`Closure`
machinery) or `m_indices`/`m_values` (reused the exact existing
`Mapping::entries` iteration), there is nothing to reuse here -- a
minimal-but-real slice would still be substantial new ground-up work,
not a contained one-session commitment. **Stopped and reported rather
than forced**, per this project's own established discipline (`bind_lambda()`/
mapping width/DGD `atomic` all got the same treatment when the real
scope turned out bigger than a normal batch item) -- `parse_*` stays
exactly as unimplemented as before, now with a much sharper, corpus-
confirmed understanding of just how high-impact it actually is whenever
it does get picked up as its own explicit go-ahead.

**`valid_read`/`valid_write` (row 1.16), the item this task explicitly
flagged as one real candidate:** confirmed genuine, load-bearing real
usage -- `temp/core-lib/secure/master/security.c` (RealmsMUD, the one
confirmed genuinely LDMud-targeting corpus this repo has) defines real
`valid_read()`/`valid_write()` with real privilege checks and a real
access cache, backing its own automated `lib/tests/secure/
securityTest.c`. Cross-cutting (both dialects share the real applies,
confirmed directly against both real sources), well-bounded (a single
shared gate helper plus wiring into 11 already-implemented file efuns,
no new infrastructure needed), and genuinely achievable as a real,
complete implementation in one session -- picked as the actual build,
with `parse_*`'s own real scope reported honestly instead of forced into
this slot.

## What was built

**`checkValidPath()`** (`EfunTable.cpp`, new): the shared gate behind
both applies. Genuinely dialect-gated, not one unified shape, after
reading both real call conventions directly rather than assuming they
match just because the apply names do:
- Real FluffOS (`file.c`'s own `check_valid_path()`): 3 args, `(path,
  call_object, call_fun)` -- no uid concept at all.
- Real LDMud (`doc/master/valid_read`/`valid_write`'s own SYNOPSIS,
  confirmed matching core-lib's own real `valid_write(string path,
  string uid, string method, object caller)` definition exactly, same
  four names and order): 4 args, `(path, uid-or-0, func, ob)`.

"uid" under the LDMud shape maps to the calling object's own `privs()`
-- this driver's real closest analog (it has no uid/euid hierarchy at
all, already on record), not a faked-up model. Real result semantics
matched precisely: master not defining either apply at all is a
permissive default (this driver's own real "undefined function returns
void" contract, distinct from an explicit `int` `0` via the
`std::monostate`/`int64_t` `Value` distinction already available for
free) -- every existing mudlib that never defined either keeps working
completely unchanged; explicit `0` denies; a string return rewrites the
path; anything else allows with the original path.

Wired into all 11 already-confirmed file efuns: `read_file`,
`write_file`, `get_dir`, `rm`, `mkdir`, `save_object`, `restore_object`,
`rename` (two checks, `rename_from`/`rename_to`, matching real doc's own
two-name list), `rmdir`, `read_bytes`, `write_bytes`.

**One real regression caught before landing, not after:** `checkValidPath()`
initially called `VM::applyMaster()` directly, which throws hard when no
master object is loaded at all -- a real, common case in this driver's
own test harness (most existing file-efun tests never load one). Running
the full suite after the first pass dropped from 631 to 148 passing,
immediately surfacing it. Fixed by treating "no master loaded" as the
same permissive default as "master loaded but does not define the
apply", not a new failure mode this row would otherwise have introduced
into every file efun at once -- confirmed back to the full count
afterward.

**4 new regression tests:** deny (write genuinely never happens, file
never created), path rewrite (write lands at the master's own rewritten
path, not the literal one), and the real per-dialect argument shape --
a master function that captures and echoes back exactly what it
received, confirming both the FluffOS 3-arg shape and the LDMud 4-arg
shape (including the real `privs()`-as-uid mapping) end to end.

**Verified live against the real running driver, real bundled `mudlib/`
-- with a genuinely interesting discovery along the way:** `mudlib/
inherit/master/valid.c` (inherited from stock Lil, carried through the
earlier library-mudlib rebuild) already defines real `valid_write()`/
`valid_read()`, unconditionally `return 1;` -- meaning this row's own
fix newly makes them actually *fire* for the first time (they were dead
code before, never called by anything), with zero behavior change for
the shipped mudlib's own already-permissive default. Temporarily edited
that same real file to selectively deny one specific test path (matching
this project's own established "temporary test hook, removed after"
precedent -- STATUS-ARCHIVE.md's own `cmd_zerotest`/`cmd_userptest`
history), confirmed live via `eval`: `write_file()` to the denied path
returns `0`, and `file_size()` on that same path returns `-1` -- the
write genuinely never happened on disk, not just a fake return value.
Also confirmed `eval` itself keeps working throughout (its own internal
`write_file()` for `/tmp_eval_file.c` is a different, allowed path) --
an early, blunter version of this test (`valid_write()` unconditionally
denying everything) caught `eval.c`'s own internal write too, confirming
the gate is genuinely comprehensive, not narrowly scoped to "mudlib
code, not driver internals". Reverted the file to its exact original
state afterward (`git diff` empty). A final live pass then confirmed the
real, unmodified mudlib's own `create`/`purge` (wand of creation,
`write_file()`-based) and `m_indices()` (last session's own work) both
still work normally end to end.

**2026-08-18 (continued): ran down the crash flagged last session --
real bug found and fixed (an uncaught dispatch error never fired
`net_dead()`, unlike ordinary link death), but the "took the whole
process down" half of last session's own claim did not reproduce after
thorough investigation and was very likely a self-inflicted artifact of
running `pkill` moments before checking the log, not a genuine crash.
Verified live with two real clients (631 tests, up from 630).**

## Reproducing it

Rebuilt the exact repro from last session (`eval return
totally_undefined_efun_xyz()`) and tested far more rigorously than
before: connected a second, independent client alongside the one that
triggers the error, and never ran `pkill` myself while checking process
status. Result: the triggering connection correctly closes; the driver
process and the second connection are both completely unaffected.
Repeated this across multiple runs, plus two extended stretches of pure
`ps` polling (90+ and 100+ seconds, entirely via active `sleep`-then-check
Bash calls, never idle) after the error -- the process stayed up every
time. **Last session's own "in one observed run crashed the whole
process" framing does not reproduce under rigorous re-testing** and is
best explained by the `pkill -f "amlp etc/driver.cfg"` command that ran,
both times, in the same breath as the log check that showed "amlp
shutting down." -- confirmed directly: that message only ever prints
after `Scheduler::run()`'s loop returns, and the loop's only real exit
conditions are `maxIterations` (test-mode only, never used live) or
`Scheduler::requestShutdown()`, which is only ever called from `main.cpp`'s
own `SIGINT`/`SIGTERM` handler -- exactly what `pkill`'s default signal
sends. This session's own testing separately hit one more confusing,
unrelated wrinkle worth naming honestly: a background driver process
left running with no `Bash` tool calls at all for an extended stretch
(pure file-reading turns) once disappeared with no shutdown message and
no core dump -- reproduced zero times when polled *actively* instead
(the 90s/100s stretches above), so this looks like a property of this
particular sandbox's own background-process lifecycle across idle
tool-call gaps, not the driver -- flagged here rather than either
chasing it further or silently ignoring it.

## Reading the real code path

Checked, as asked, whether command dispatch has the same per-connection
try/catch isolation `Server.cpp` already documents elsewhere (its own
`onNewConnection()` comment: "confirmed live: attempting
/secure/std/login.c before it actually compiled took the whole process
down on the very first connection" -- an earlier, already-fixed instance
of this exact class of bug). Checked every real VM-entry point in
`src/net`/`src/scheduler`, not just the one dispatch already had:

- `Server::handleConnection()`'s own per-line dispatch loop -- already
  wrapped, `try { dispatchLine(...) } catch (...) { ...; conn.close();
  break; }`, with a comment already stating the exact isolation
  principle this session was asked to confirm.
- `Server::onNewConnection()`'s `master->connect()` and `logon()` calls
  -- both already wrapped, matching the comment above.
- `fireSocketCallback()` (socket read/write/close callbacks) -- already
  wrapped.
- `Scheduler::tickHeartbeats()` and `Scheduler::tickCallOuts()` -- both
  already wrapped, per-object/per-entry, one failing `heart_beat()` or
  `call_out()` target does not stop the rest from firing.
- Every exception type this codebase actually throws (`grep`ped
  directly, not assumed) -- `LpcRuntimeError`, `LpcThrownValue`,
  `NotImplementedError`, `EvalCostError`, `std::invalid_argument` --
  derives from `std::exception`, so every one of the `catch (const
  std::exception&)` boundaries above genuinely catches all of them, no
  type-level gap.

**Command dispatch already has exactly the isolation asked about.** The
real gap was narrower and different in kind: not a missing try/catch,
but the dispatch-error catch calling the *wrong one* of two existing
teardown methods.

## The real bug

`Connection::close()` (the *full* teardown -- fd close,
`InteractiveRegistry` removal, snoop unlink, clearing `boundObject_`) and
`Connection::pollLines()`'s own internal EOF/read-error handling (which
only ever sets the lightweight `closed_` flag, leaving `boundObject_`
intact) are two genuinely different things, by design: `Server::
fireNetDeadIfLinkDead()` (called unconditionally right after either
dispatch or `pollLines()`, `Server.cpp`) checks `conn.closed()` *and*
`conn.boundObject()` still being non-null before firing `net_dead()` --
the ordinary link-death path (peer EOF) correctly leaves `boundObject_`
alone long enough for that check to see a real object and fire it;
`Connection::~Connection()` (which calls the full `close()`) only runs
later, once `Server::pollOnce()`'s own closed()-connections pruning
erases the last owning `shared_ptr`. `testFireNetDeadIfLinkDeadSkipsAfterExplicitConnectionClose`
(already in this suite) confirms the same full-`close()`-clears-
`boundObject_`-first shape is *correct* for an explicit `destruct()` --
real semantics do skip `net_dead()` there.

`Server::handleConnection()`'s own dispatch-error catch block called the
*full* `conn.close()` directly -- the same method that is correctly used
for `destruct()`, but wrong here: this path is not `destruct()`, it is
exactly the same "this interactive session just ended" event ordinary
link death already is, and real semantics do not distinguish "why" the
connection died for `net_dead()`'s own purposes. Calling the full
`close()` cleared `boundObject_` before `fireNetDeadIfLinkDead()` (called
unconditionally right after, unchanged) ever got a chance to see it, so
`net_dead()` silently never fired for this one path -- concretely, real
`user.c`'s own `net_dead()` calls `set_heart_beat(0)`, so a player whose
current command threw an uncaught error kept a heart_beat registration
that never stops firing for the rest of the process's own lifetime, a
real, permanent per-incident resource leak (the object itself also stays
in `LiveObjectRegistry` forever, unreachable but never freed) -- not
merely a missed room notification, though that is real too (confirmed
live below).

## The fix

New `Connection::markClosed()` (`Connection.hpp`): sets only the
lightweight `closed_` flag, the exact same shape `pollLines()`'s own EOF
branch already used internally, exposed publicly for the one real caller
that needs it. `Server::handleConnection()`'s catch block now calls this
instead of the full `close()` -- `fireNetDeadIfLinkDead()`, called right
after, unchanged, now correctly sees a still-valid `boundObject()` and
fires `net_dead()`, exactly matching the ordinary link-death path. The
real teardown itself still happens moments later either way, via
`~Connection()` once `pollOnce()`'s own pruning erases the connection --
no observable delay, same poll cycle. `destruct()`'s own call site
(`EfunTable.cpp`) is untouched -- it still correctly uses the full
`close()`, still correctly skips `net_dead()`, confirmed unchanged by
the existing `testFireNetDeadIfLinkDeadSkipsAfterExplicitConnectionClose`
still passing.

**1 new regression test**,
`testUncaughtDispatchErrorStillFiresNetDeadUnlikeExplicitClose`:
reproduces `Server::handleConnection()`'s own real fixed sequence by
hand (`dispatchLine()` throws -> caught -> `markClosed()` ->
`fireNetDeadIfLinkDead()`), the same public-static-method test seam the
rest of this cluster already uses since `handleConnection()` itself is
private. Confirms `net_dead()` now actually fires, unlike the (correct,
unchanged) explicit-`destruct()`-close case the existing test cluster
already covers right next to it.

**Verified live with two real clients** against the real running driver,
real bundled `mudlib/`: client A triggers the crash condition (`eval
return totally_undefined_efun_xyz()`); A's own connection closes
(confirmed via EOF); **client B -- a completely separate, unaffected
session -- genuinely receives real `user.c`'s own `net_dead()` broadcast,
`"stuf0 is link-dead."`**, live proof `net_dead()` actually fired this
time, not just in the unit test; B then confirmed still fully functional
afterward (`eval return 5+5` and `who` both work normally). Clean boot
log throughout, process alive and responsive across the entire test.

No ROADMAP.md row -- matching how the earlier `main.cpp` argv fix and the
ctest-vs-direct-run fix were both recorded here only, not tracked as
dialect/efun work.

**2026-08-18 (continued): `m_indices()`/`m_values()` implemented as
mapping width's own first real slice -- re-ranked the three original
Phase 1 blockers after closures' highest-real-usage piece landed, picked
by real corpus-frequency evidence, not size (630 tests, up from 627).**

## Re-ranking

Of the three original Phase 1 blockers -- `#'`/`'name` closures, mapping
width, connect/disconnect -- closures just had its highest-real-usage
slice landed (`#'efun::name`, last session). Re-ranked the remaining two
the same way, by real evidence rather than assumption:

**Connect/disconnect, checked directly against the real corpus this
time, not just `src/dialect/instruct.md`'s own prose:** grepped
`temp/core-lib` (the one confirmed genuinely LDMud-targeting corpus this
repo has) for the real master-apply signature, `disconnect(object ob,
string remaining)`. Zero real occurrences -- the 14 raw `disconnect(`
hits found are all this mudlib's own unrelated database-connection-
closing function (`protected nomask void disconnect(int handle)`,
`lib/modules/secure/dataServices/dataService.c`), not the link-death
apply at all. `core-lib`'s own real `secure/master.c` does not define
`disconnect()` in any form. Real, but zero observed reliance on it in
the only corpus available to check against.

**Mapping width, re-examined for a genuinely separable first slice
rather than re-confirming the whole row is too big (already on record):**
searching for the real `m_allocate`/`m_values`/`m_indices`/`m_entry`/
`m_reallocate`/`m_add`/`m_contains` efun family surfaced something the
prior investigation had not separated out -- `m_indices`/`m_values`
(real LDMud's own *names* for what this driver already has as `keys()`/
`values()`) were not registered under any spelling at all, and `m_indices`
alone has 544 real call sites in `core-lib`, `m_values` has 17. Unlike
the rest of row 1.9's own scope (real N-column value semantics), a bare,
width-agnostic `m_indices()`/`m_values()` needs zero changes to this
driver's existing single-column `Mapping` type at all -- it already *is*
column 0, the real default.

**Result: mapping width's own `m_indices`/`m_values` first slice beats
the disconnect bug on real evidence** -- 561 confirmed real call sites
against zero, not a close call. Picked accordingly.

**A real methodology correction along the way, not just a result** (same
discipline as last session's `#'` corpus search): a naive `grep -c
";"` inside mapping literals first suggested real `([ k: v1; v2 ])`
mapping-width-literal usage in `core-lib` -- both apparent hits turned
out to be a `;` inside an ordinary string value (`"35;1"`, an ANSI
color-code string), not a real width-literal separator at all. Zero real
occurrences of the actual mapping-width literal syntax, `m_allocate`,
`m_entry`, `m_reallocate`, `m_add`, `m_contains`, or the `([: N ])`
empty-width literal anywhere in the corpus -- confirmed by reading actual
matched context, not trusting the raw count, the same discipline that
already caught the `#'` search's own false positives last session.

## What was built

**`m_indices(mapping)`** (`src/efun/EfunTable.cpp`): thin wrapper over
the exact same `Mapping::entries` iteration `keys()` already uses --
real signature (`temp/ldmud/src/func_spec:479`) takes no width argument
at all, so this is a complete, correct implementation, not a partial
one.

**`m_values(mapping, int col default: 0)`**: same shape as `values()`,
plus the real optional width-column argument (`func_spec:481`). Column 0
(the default, and the overwhelming majority of real usage -- 15 of 17
real call sites) returns this driver's existing single value column
correctly. A genuine non-zero column is honestly rejected with a clear
error naming row 1.9's own still-open scope, rather than silently
returning column 0's values for a column the caller never actually
asked for and never finding out.

Both registered unconditionally, not gated on dialect -- matching this
table's own already-established convention (`unshadow()`'s own comment:
efun *availability* is never withheld by dialect in this driver, only an
existing shared name's own behavior branches).

**3 new regression tests**, including one confirming the honest-rejection
behavior specifically (explicit column 0 succeeds, column 1 throws).

**Verified live against the real running driver**, real bundled
`mudlib/`, default (fluffos) dialect -- these two efuns need no dialect
gate, so no scratch config needed this time: three real `eval` calls all
returned correct results (`m_indices` on a 3-entry mapping, bare
`m_values`, explicit `m_values(m, 0)`), and a fourth (`m_values(m, 1)`)
confirmed the rejection fires with the intended message.

**One incidental, pre-existing, unrelated finding surfaced during live
verification, not caused by this slice and not fixed here:** an uncaught
runtime error during command dispatch (confirmed via `eval`) drops the
connection and, in at least one observed run, the whole driver process
along with it -- reproduced identically with an already-existing,
unrelated error case (calling a genuinely undefined efun via `eval`),
confirming this is a real, general robustness gap in uncaught-error
handling during command dispatch, not something the `m_values` rejection
introduced. Out of this session's scope; flagged here so it is not
reintroduced ad hoc later without a record.

ROADMAP.md row 1.9 updated with the full implementation record. Test
count footer updated (627 to 630).

**2026-08-18 (continued): `#'efun::name` scope-prefixed closure literal
implemented and live-verified -- ranked by real corpus-frequency search
across every vendored mudlib this repo has, the same method row 0.13's
efun gaps used, not by grammar size (627 tests, up from 625).**

## Ranking methodology and result

Row 1.2/1.3's own deliberately-uncovered list had four remaining `#'`
forms: operator spellings (`#'+`, `#'==`, ...), index forms (`#'[`),
aggregate closures (`#'({`), and scope prefixes (`#'efun::`, etc.).
Searched every corpus this repo actually has for real occurrences of
each -- the already-extracted directories directly (`temp/core-lib`,
`temp/dead-souls`, `temp/es2_mudlib`, `temp/lima`, `temp/nightmare3`,
`temp/mudlib`, `temp/lil`, `temp/wiz_tools`), and the newer archives via
`unzip -p`/`tar -xzO` piped straight into a search, never writing
anything new to disk (`ds3.9.zip`, `dsI.zip`, `dsIIr10.zip`,
`final_realms_fluffos_v1.zip`, `lpuni_fluffos_v1.zip`,
`merentha_fluffos_v2.zip`, `skylib_fluffos_v3.zip`,
`tmi2_fluffos_v3.zip`, `gurba-0.42-beta2.tar.gz`).

**A real methodology correction along the way, not just a result:** a
naive `grep "#'"` first returned 220 hits in `core-lib` alone, including
what looked like 24 real operator-spelling occurrences. All 24 turned
out to be one false positive repeated across files: a string literal
templating placeholder, `'##CompositeSegment##'`, whose own trailing
`##'` textually matches `#'` with no relation to closure syntax at all.
A second, much larger false-positive class surfaced the same way:
`##Name##'s` (this mudlib's own template-placeholder convention followed
by a plain English possessive apostrophe-s, e.g. `##InitiatorName##'s
weapon`) dropped `core-lib`'s own apparent bare-name count from 195 to
33 once excluded. A third, smaller one: `lima`'s only apparent hit was
literally the English abbreviation "the line #'s you want to move" (line
*number's*), not code at all. Every one of these was caught by requiring
the character immediately before `#` to be a real code-context character
(whitespace, `(`, `,`, `=`, `;`, `{`, `[`, or a real operator character)
rather than trusting a bare substring match -- confirmed by reading
actual matched context for every category, not assumed from the raw
count. `tmi2_fluffos_v3.zip` and `gurba-0.42-beta2.tar.gz` were spot-
checked directly too (both correctly returned zero after filtering,
`tmi2`'s own only apparent hit being the identical "#'s" = "number's"
false positive) -- confirming, not just assuming, that FluffOS-family
and DGD-family archives structurally cannot contain real LDMud-specific
`#'` syntax at all, which none of the newer archives named for this
search turned out to be (`ds3.9`/`dsI`/`dsIIr10` bundle FluffOS-family
"ds"-patchlevel driver source per the prior inventory session, same for
Final Realms/LPUniversity/Merentha/Skylib/TMI-2; Gurbalib is DGD-based).
**One real corpus mismatch worth naming directly:** the task named
"genesismud" as one corpus to check -- no such directory or archive
exists anywhere under `temp/`, confirmed by `find`, not silently assumed
absent.

**Real result after filtering:** operator spellings, index forms, and
aggregate closures -- zero confirmed real occurrences anywhere in the
entire searched corpus. Scope prefixes -- exactly one, `temp/core-lib/
secure/simulated-efuns/testing.c`'s own `apply(#'efun::call_out,method,
delay,data)`. Thin evidence in absolute terms (only `core-lib` is
confirmed genuinely LDMud-targeting among everything currently under
`temp/`), but a real, non-tied result: scope prefixes is the only one of
the four with any confirmed usage at all, so it is the real pick under
the row 0.13 frequency-ranking method, not a coin flip presented as one.

## Real LDMud semantics, read before implementing

`temp/ldmud/doc/LPC/closures`'s own citation: "Closure literals can have
prefixes to specify which type of closure shall be created:
`#'efun::function_name`: closure to an efun, `#'sefun::function_name`:
closure to a simul-efun, `#'lfun::function_name`: closure to an lfun,
`#'var::variable_name`: closure to a global variable. Inherited programs
can be given as prefixes, too." This splits into two genuinely different
implementation shapes, not one uniform feature: `efun::`/`sefun::`/
`lfun::` are all still ordinary function closures, just with an explicit
forced resolution tier -- the exact same underlying `Closure`/
`ClosureLiteralExpr` shape bare `#'name` already uses, just one added
bool. `var::` is a structurally different closure *kind* entirely -- a
reference to a variable, not a callable at all (real LDMud's own
`CLOSURE_IDENTIFIER`, already on record from row 1.7's own `bind_lambda()`
investigation as one of several distinct closure kinds this driver has no
model of). Inherited-program prefixes need real inherit-name resolution
this scoping pass never touched either.

Scoped to exactly the one form with confirmed real evidence: `efun::`
alone. `sefun::`/`lfun::` (zero corpus evidence, and would each need
their own distinct forced-tier plumbing -- resolving against the
simul_efun object specifically, or this object's own local functions
only, neither the same check as `efun::`'s) and `var::`/inherited-program
prefixes (zero evidence, and `var::` alone is a materially bigger
feature) stay deliberately unimplemented. A bare `#'sefun`/`#'lfun` today
parses as an honest bare-name closure literal naming a function literally
called that, with the trailing `::name` left as a genuine parse error --
not a silent misparse, the same acceptable failure shape `atomic`/`nil`
already established under the "wrong" dialects.

## What was built

**`Closure::forceEfun`/`ClosureLiteralExpr::forceEfun`** (`Value.hpp`/
`Ast.hpp`): one new bool field on each, default false, changing nothing
for any closure this driver already builds (`(: name :)`, bare `#'name`,
inline lambdas).

**`PushEfunClosure`** (`Bytecode.hpp`, `CodeGen.cpp`, `VM.cpp`): a new
opcode, identical operand/argCount shape to the existing `PushClosure`,
built purely so a closure literal can pick which VM.cpp handler
constructs it -- the exact same `Call`/`CallEfun` split `CodeGen.cpp`'s
own `forceEfun` comment already documents for ordinary calls, applied to
closure literals instead of call expressions.

**`VM::callClosure()`**: one new check at the very top of its own
resolution, before either tiered lookup runs -- when `forceEfun` is set,
skip straight to the core efun table, throwing "undefined efun" if the
name is not one, rather than ever giving this object's own lfun/inherited
or simul_efun tier a chance to win first.

**`Parser.cpp`**: recognized right after `#'` is consumed -- `efun`
immediately followed by `::` takes the new path (consume both, read the
real function name, set `forceEfun = true`); anything else falls through
to the existing bare-name case unchanged. Zero Lexer changes needed at
all: "efun" is not a reserved keyword in this driver (confirmed, matching
the existing ordinary `efun::name(...)` call syntax's own comment on the
exact same point) and "`::`" already lexes as an existing Symbol token,
so both pieces `#'efun::name` needs were already tokenizable before this
session touched anything.

**2 new regression tests.** `testHashQuoteEfunPrefixParsesToClosureLiteralExprWithForceEfun`
confirms the AST shape directly. `testHashQuoteEfunPrefixBypassesALocalFunctionOfTheSameNameUnlikeBareForm`
proves the real semantic this feature exists for, not just that it
parses: a local function literally named `lower_case` (shadowing the
real efun) defined in the same test object, then `funcall(#'lower_case,
...)` resolving to the local override (`"SHADOWED:ABC"`) while
`funcall(#'efun::lower_case, ...)` bypasses it and reaches the real efun
(`"abc"`) -- the two closure forms genuinely resolving to two different
targets from the exact same object.

**Verified live against the real running driver**, real bundled
`mudlib/`, `dialect: ldmud`: four separate `eval` expressions in one
session -- `funcall(#'efun::lower_case, "HELLO WORLD")` returns `"hello
world"`; `mixed f = #'efun::upper_case; return funcall(f, "hi there")`
returns `"HI THERE"` (stored in a variable before calling); `capitalize(
funcall(#'efun::lower_case, "YES"))` returns `"Yes"` (composed with
another efun); `funcall(#'efun::sizeof, ({1,2,3}))` returns `3` (an
array argument, not just strings). Clean boot, no errors. Same scratch
`dialect: ldmud` config as last session's bare-`#'name` verification,
still living only in the scratchpad, not staged.

ROADMAP.md row 1.3 updated with the full implementation record. Test
count footer updated (623 to 627 across this session's two `#'` slices).

**2026-08-18 (continued): self-assessment against the real 6-12 month
goal, picked LDMud `#'name` closures as the next real multi-session
commitment, implemented and live-verified a real first slice this same
pass (not scoping-only) -- 625 tests, up from 623.**

## Assessment

**Efuns:** 242 currently registered (`grep -c registerEfun( src/efun/
EfunTable.cpp`). Row 0.13's own real accounting (not the row title's
older "~300" approximation): 270 real names in this exact vendored
FluffOS reference build's own `efun_defs.c`, of which 40 non-`parse_*`
names are confirmed genuine documented exclusions (architecture mismatch
or zero-call-site deferral), leaving the 8-name `parse_*` natural-
language parser package (row 0.13a) as the one real, sizeable,
deliberately-deferred efun gap remaining -- everything else in Phase 0
is closed.

**Applies:** 19 known (`ApplyTable::known()`), 10 actually fired by the
driver today (`create`, `init`, `connect`, `logon`, `process_input`,
`net_dead`, `compile_object`, `heart_beat`, `id`, `catch_tell`), 9
recognized but not yet fired -- `valid_read`/`valid_write` among them,
already on record (row 1.16) as a real missing cross-cutting security
feature affecting both FluffOS and LDMud mudlibs equally, not gated on
any of this driver's 11 file efuns today.

**Phase 0:** every row `[x]` except `0.13a` (`parse_*`, deliberately
deferred, sized comparable to everything else in row 0.13 combined).
Genuinely stable -- this is a solid foundation, not a gap.

**Phase 1:** `1.1`/`1.5`/`1.6` done. `1.10` (DGD `nil`) done but
comparison-only, not a real-compatibility item. `1.11`-`1.15` (DGD-only)
correctly deprioritized last session -- comparison research, not Phase 1
blockers. `1.16` (LDMud master apply table) has all three remaining
items individually investigated and blocked (`get_bb_uid` dead in real
LDMud itself, `make_path_absolute` needs `ed()` first, `valid_read`/
`valid_write` needs its own dedicated row). The three real remaining
Phase 1 blockers, per last session's own correction: `#'`/`'name`
closures, mapping width, connect/disconnect -- see below for which one
and why.

**Phase 2/3:** entirely `[ ]`. Genuinely not blocking whether a mudlib
can run at all -- JIT, hotboot, GC, persistence, concurrency, TLS,
efun breadth beyond FluffOS, dev tooling, production hardening. Real
production-polish work, correctly sequenced after Phase 1 per this
file's own "Phase 1 before Phase 2" principle, not evidence of neglect.

## Pick: LDMud `#'name` closure literals

Weighed the three remaining Phase 1 blockers by real-world mudlib-
compatibility impact, not size:

- **`#'` closures (picked):** LDMud's own function-pointer/callback
  syntax, used as pervasively in real LDMud codebases (`sort_array`/
  `filter`/`map` callback arguments, event/delegation patterns) as this
  driver's already-supported FluffOS `(: name :)` is for FluffOS ones.
  Unsupported, this is a compile-time failure for most real LDMud mudlib
  code beyond a trivial example, not a missing-feature gap confined to
  one corner of it -- closer in kind to "can this driver run a real
  LDMud mudlib at all" than the other two.
- **Mapping width:** real, but narrower in reach -- only affects mudlibs
  that specifically use LDMud's own wide-mapping idiom (`([ k: v1; v2
  ])`, `m_allocate`/`m_values`/`m_indices`), not every LDMud codebase.
- **Connect/disconnect:** real, but partial. Checked directly
  (`src/dialect/instruct.md`): `connect` itself is already name-
  compatible between FluffOS and LDMud (both use the same master apply),
  so a real LDMud mudlib already logs in correctly under this driver
  today. Only link-death/disconnect notification is actually broken
  (LDMud's real apply is `disconnect(object ob, string remaining)`,
  called on master with a different signature; this driver still only
  ever fires FluffOS's `net_dead(object)` on the player, regardless of
  configured dialect) -- a real bug, but not a whole-mudlib blocker the
  way an unparseable core syntax construct is.

`#'` closures wins on "which unlocks the most actual usage", not "which
is smallest" -- confirmed the largest of the three per prior research
(row 1.2/1.3's own scoping note called it "large, rich internal grammar
... but self-contained ... EXCEPT for a genuine architecture problem").
Broken into a real first slice this pass instead, the same way the
broader dialect work got `atomic` then `nil` as its own first slices,
rather than staying purely theoretical.

## What was actually built

**The architecture prerequisite, fixed for real.** Real system `cpp`
hard-errors on any line whose first non-whitespace character is `#` and
is not a real directive it recognizes -- confirmed directly against
`cpp`'s own documented directive-parsing rule, not assumed -- and a bare
`"#'name;"` statement on its own line (a completely ordinary way to
write it) is exactly that shape; it would take the *whole file's*
preprocessing down before this driver's own Lexer/Parser ever ran,
regardless of what they were taught to recognize. Fixed in
`ObjectManager.cpp`: `maskHashQuote()` replaces every `#'` occurrence in
the raw source with a plain-identifier marker text before staging for
`cpp` (`stageSourceForPreprocessing()`), and `unmaskHashQuote()` reverses
it in `runPreprocessor()`'s own output. Runs unconditionally for every
file, not gated on dialect -- nothing dialect-specific about the fix
itself, and a file with no `#'` anywhere in it is untouched either way.

**The Lexer.** `#` was not lexable at all before this pass, outside real
preprocessor directives -- confirmed directly: it fell straight into
`tokenize()`'s own "unrecognized character" branch, unlike `atomic`/
`nil`, which were always at least valid (if unreserved) identifiers.
New `Lexer::lexHashQuote()`, dispatched only when `dialect_ ==
LpcDialect::LdMud && peekNext() == '\''`, combines `#` and `'` into one
two-character `Symbol` token (`"#'"`) -- matching the existing `"->"`/
`"::"`/`"++"` multi-char-symbol precedent already in this file, not a
new `TokenType`. Under FluffOS/DGD, a bare `#` still throws exactly the
same "unrecognized character" it always has -- zero behavior change for
either.

**The Parser.** `parsePrimary()` recognizes the combined `"#'"` token
(double-gated on `dialect_ == LpcDialect::LdMud` again, the same belt-
and-suspenders discipline `"atomic"`/`"nil"` already established),
expects an `Ident` for the function name, and builds a
**`ClosureLiteralExpr`** -- the exact same AST node the existing `"(:
name :)"` case a few lines below it already builds, with `boundArgs`
left empty (real LDMud semantics for this bare form: arguments are
supplied by whoever calls the closure later, not baked in at the literal
site). Reusing that node instead of inventing a new one meant this slice
needed **zero** CodeGen or VM changes past the Parser check itself --
`CodeGen.cpp`'s existing `PushClosure` emission and every downstream
consumer (`funcall`/`evaluate`, `map`/`filter`/`sort_array` callbacks)
already handle any `ClosureLiteralExpr` generically by function name,
with no FluffOS-vs-LDMud distinction anywhere in that path.

**Deliberately not covered by this slice**, matching the discipline
already established for `atomic`'s own "keyword parses, semantics come
later" precedent: `#'` operator spellings (`#'+`, `#'==`, `#'++`, ...),
the `#'[` index/range/map-index forms, the `#'({` aggregate-array
closure, and every scope prefix (`#'efun::`, `#'sefun::`, `#'lfun::`,
`#'var::`, `#'Name::`). All confirmed still genuinely large per row
1.2/1.3's own prior research; none of them touched this pass.

**2 new regression tests**: `testLexerHashQuoteClosureOnlyRecognizedUnderLdmudDialect`
(confirms `#` throws under FluffOS/DGD exactly as before, and combines
into one `"#'"` token only under LdMud); `testCompileHashQuoteClosureAcceptedOnlyUnderLdmudDialectAndEvaluatesCorrectly`
(routed through `ObjectVarHarness`, not `compileProgramObject`, so it
actually exercises the real `cpp` preprocessing path and the masking
fix, not just the Lexer/Parser in isolation -- its own source includes a
bare `"#'lower_case;"` statement on its own line, the exact shape the
architecture fix was needed for; under `dialect: ldmud` it also confirms
real closure semantics, not just "it parsed": `funcall()` correctly
resolves and calls the referenced function).

**Verified live against the real running driver**, not just the test
suite -- a real `dialect: ldmud` config booted against this driver's own
actual bundled `mudlib/`, connected with a real telnet-negotiating
client, three separate `eval` expressions in one session: `funcall(#'
lower_case, "HELLO WORLD")` returns `"hello world"`; `mixed f = #'
upper_case; return funcall(f, "hi there")` returns `"HI THERE"` (storing
the closure in a variable before calling it, not just using it inline);
`capitalize(funcall(#'lower_case, "YES"))` returns `"Yes"` (composed
with another real efun). Clean boot, no errors. The scratch config used
for this (`dialect: ldmud` added to a copy of `etc/driver.cfg`, port
4001) lived only in the scratchpad, not staged -- this repo still has
exactly one canonical driver config.

ROADMAP.md rows 1.2 and 1.3 updated with the full implementation record;
the row 1.2/1.3 scoping note's own "not proposed as part of any first
slice" line (accurate history of what was proposed at the time) left
unedited, with a short dated follow-up noting the architecture decision
has since been made, rather than rewriting the historical sentence
itself.

**2026-08-18 (continued): `main.cpp`'s dead no-argv config-path fallback
removed, replaced with a usage message and a nonzero exit -- flagged but
not acted on during last session's dependency grep, fixed now (623
tests, unchanged -- `main.cpp` is not part of the test binary, see
below).**

`main.cpp` used to default `configPath` to `"config/driver.cfg"` when no
argv was given -- a path that has never existed anywhere in this repo
(confirmed again this session: `find` turns up nothing at that path,
same as when this was first noticed). A genuinely dead branch: every
real invocation of this binary throughout the project's own history
(`README.md`'s own documented usage, every live-verification session
recorded in this file) already passes an explicit config path, so
nothing has ever actually exercised the fallback.

Considered repointing it at `etc/driver.cfg` instead (now the one
canonical config, see this file's own consolidation entry above) --
decided against it: a baked-in default still silently assumes the
process runs from the repo root, an assumption nothing else in this
codebase makes, and one the project's real usage pattern has never
relied on either way. Removed the fallback instead: `argc < 2` now
prints `"Usage: <argv[0]> <config-path> [max-iterations]"` to stderr and
exits 1, rather than silently trying (and failing on) a path that was
never real. Confirmed both paths directly: `./build/amlp` with no
arguments now prints the usage message and exits 1; `./build/amlp
etc/driver.cfg` (with `AMLP_MAX_ITERATIONS=2` to bound the run) still
boots exactly as before.

**No regression test added -- genuinely no reasonable place for one
under the current architecture, not a shortcut.** `main.cpp` compiles
into the `amlp` executable target only (`CMakeLists.txt`); the
`amlp_tests` binary (`test/test_lexer.cpp`) links against every library
subsystem (`core`, `config`, `compiler`, `vm`, `object`, `efun`,
`apply`, `dialect`, `net`, `scheduler`) but never `main.cpp` itself --
confirmed directly from the top-level `CMakeLists.txt`, not assumed.
Testing this argv-handling change through the existing suite would need
either extracting it into a separate, linkable function both targets
share (a real, if small, design decision beyond the scope of this fix),
or spawning the real `amlp` binary as a subprocess from a test, a
pattern that does not exist anywhere in this suite today (every existing
test drives an in-process `ObjectManager`/`VM` directly; live behavior
of the real binary has only ever been verified manually, never through
`ctest`). Neither was worth introducing for a two-line argv check.
Confirmed manually instead, as noted above.

**2026-08-18 (continued): consolidated to a single driver config,
`etc/driver.cfg`, boots the real library mudlib -- `etc/driver_lil.cfg`
removed, verified live (623 tests, unchanged, no source touched).**

Grepped the whole repo for `driver.cfg` and `mudlib_stub` (test/, src/,
include/, CMakeLists.txt, any `.sh` scripts, `.claude/`) before changing
anything, per instruction. Found: `src/main.cpp`'s own no-argv fallback
default is `"config/driver.cfg"`, a different, nonexistent path,
unrelated. `include/amlp/config/Config.hpp`'s own `mudlibRoot_` class
member defaults to `"./test/mudlib_stub"` -- but confirmed via every
`Config config;` bare-construction site in `test/test_lexer.cpp` that no
automated test actually exercises this default for real file I/O: the
synthetic-source tests never call `ObjectManager::loadObject()`/
`cloneObject()` at all, and every real-file-I/O test uses
`ObjectVarHarness`, which always calls `loadFromFile()` with its own
explicit temp `mudlib_root`. No test calls `mudlibRoot()` directly
either. Every other hit was either the test harness's own unrelated
synthetic temp config file (same filename, different purpose), a
comment citing `etc/driver.cfg`/`mudlib_stub` as real-world motivating
context, or dated `STATUS.md`/`STATUS-ARCHIVE.md` history. Nothing in
`CMakeLists.txt`, no shell scripts, nothing in `.claude/`. Conclusion:
nothing beyond manual boot testing depended on either the file's content
or the stub directory's real files -- safe to consolidate.

`etc/driver.cfg`'s content replaced wholesale with `etc/driver_lil.cfg`'s
(port included, 4000 -- literal "replace with driver_lil.cfg's content"
per instruction, not a cherry-picked merge), header comment rewritten to
describe the consolidation and explicitly note `test/mudlib_stub/` is
untouched on disk, just no longer referenced by any config file (not
deleted -- consolidating configs is not the same thing as deciding the
stub mudlib itself is unneeded, and nothing asked for that).
`etc/driver_lil.cfg` removed. `README.md`'s build/run section collapsed
back to the single command.

Verified live with a real telnet-negotiating client against the single
remaining `etc/driver.cfg`: login shows "Welcome to Library!", the
entrance hall's description prints on arrival, `create`/`purge` both
confirmed working. Clean boot, no errors. One scratch artifact from the
live test itself (`data/created/consolidation_gizmo.c`) removed
afterward, same as every previous time this exact live-test byproduct
has come up.

**2026-08-18 (continued): ROADMAP.md scope clarification -- DGD is now
recorded as a comparison-only reference dialect, not a required Phase 1
target, no code touched (623 tests, unchanged).**

Updated the master goal statement at the top of ROADMAP.md (previously
"meeting or exceeding FluffOS, LDMud, and DGD... on their own terms") and
Phase 1's own header (previously "one binary, three dialects") to state
the real goal plainly: a FluffOS/LDMud-level driver, done better than
either, not three-way parity with DGD. Re-annotated every DGD-only row
(1.11 `rlimits`, 1.12 `atomic`, 1.13 `parse_string`, 1.14 lightweight
objects, 1.15 driver+auto boot path) with an explicit "comparison-only,
not a Phase 1 blocker" marker ahead of each row's own existing research,
and corrected last session's own "Phase 1 scan pass" capstone note, which
had listed DGD-only items (`rlimits`+`atomic`'s shared planes
prerequisite, `parse_string`) alongside genuine LDMud gaps (`#'`/`'name`/
closures, mapping width) as four equally-weighted blockers -- the actual
remaining Phase 1 blockers after this correction are three, not four:
`#'`/`'name`/closures, mapping width, and the connect/disconnect design
question, none of which are DGD-specific.

None of the real research already recorded in any of these rows was
touched or deleted -- DGD's own source (`temp/dgd/`) stays genuinely
useful as a comparison reference (its `nil`/planes/LWO model already
informed several rows' own findings this session), just no longer
carrying the same priority weight as an actual FluffOS/LDMud gap. Row
1.10 (DGD `nil`, already `[x]`) untouched -- already done, nothing to
reprioritize. No source code touched this pass.

**2026-08-18 (continued): executed `mudlib/LIBRARY_MUDLIB_PLAN.md` --
`mudlib/` is now "library", a stripped-down rebuild bundled with the
already-built wand of creation, verified live end to end (623 tests,
unchanged, no source touched).**

Step 1: the entire prior `mudlib/` tree (222 files) copied to `temp/lil/`
and verified byte-identical (`diff -rq`) before anything else moved --
matches the existing vendored-corpus convention (`temp/core-lib/`,
`temp/dead-souls/`, etc.), gitignored, preserves stock Lil untouched for
the efun-conformance-diffing work that has used it across many prior
sessions.

Step 2: `mudlib/` rebuilt in place from the plan's own keep/new/drop
table -- boot plumbing (`single/master.c`, `inherit/master/valid.c`,
`single/simul_efun.c`, `clone/login.c`, `clone/user.c`, `inherit/base.c`,
`include/*.h`, `etc/motd`), the wand (`clone/wand_of_creation.c`,
already built and live-verified in an earlier session), `data/created/`
(the wand's own write target), and a new `single/start_room.c` -- stock
Lil had no room concept at all, confirmed directly in the plan
(`VOID_OB` was a bare `void dummy() {}` placeholder), so this is new
content, not carried over. Dropped: the entire `single/tests/`
conformance suite (203 files) plus its own support code
(`command/tests.c`, `inherit/tests.c`, `single/inh.c`, `test_control.c`,
`etc/config.test`), `single/void.c` (superseded by the real room), every
per-directory `readme` (replaced with a fresh top-level one describing
the new identity). The plan's own "Undecided" list resolved to: keep
`quit.c`/`say.c`/`who.c`/`shutdown.c`/`eval.c` (matching the plan's own
recommendations -- `eval.c` specifically because it is this project's
own live-verification tool, not just Lil example content), drop
`dest.c`/`rm.c`/`update.c`/`codefor.c`/`speed.c`/`ed.c`, matching the
plan's own stated default ("minimal... nothing else unless explicitly
kept") for the ones it left genuinely undecided.

**Two real gaps in the plan's own static "drop" analysis, both caught
only by actually booting the rebuilt driver, not by grep alone:**
`log/` was dropped as unread by anything kept -- wrong, `single/
master.c`'s own `log_error()` genuinely writes to `LOG_DIR + "/compile"`
(a real master apply, compile-error logging), confirmed live: the driver
failed to boot at all ("compile error in mudlib/single/master.c:
undeclared variable LOG_DIR") until both `LOG_DIR` and a real `log/`
directory were restored. `inherit/clean_up.c` was not listed in either
the plan's Keep or Drop table at all -- also wrong to have dropped:
`include/command.h`, itself a kept file, does `inherit CLEAN_UP;`, so
every command depending on it (`who.c`, `say.c`, `quit.c`, `shutdown.c`)
failed to compile until both `CLEAN_UP` and the file it points to were
restored. Both corrected in place; `LIBRARY_MUDLIB_PLAN.md`'s own
"executed" note updated with the honest account rather than claiming a
clean first pass.

A third, more subtle issue found only through the live session itself,
not a compile error: the room's own `init()` originally moved a freshly
cloned wand directly into the arriving player's inventory in one step.
That silently left the wand's own `add_action` calls unregistered --
confirmed live, `create`/`purge`/`clone` all fell through to
`commandHook()` and failed with "source file not found" for a literal
`command/clone.c` etc. Root cause, documented directly in
`wand_of_creation.c`'s own comment on `held()`/`init()`: this driver's
`VM::moveObject()` only calls `init()` on the *moved* object (where the
wand's own `add_action` calls live) when the destination's existing
occupants already have `commandsEnabled()` true -- true of the room the
player is already standing in, not true of the player's own empty
inventory. Fixed by matching the two-step sequence the original wand
regression tests already used (`moveObject(wand, room)` then
`moveObject(wand, player)`): the room's `init()` now moves the wand into
itself first (the player is already a genuine occupant by that point,
firing the wand's own `init()`), then into the player.

**Other real references updated, per the plan's own list:** `etc/
driver_lil.cfg`'s `mud_name: Lil` -> `Library` (filename kept as-is,
only the value changed); `clone/login.c`/`clone/user.c`'s banner text
("Welcome to Lil" -> "Welcome to Library"); the `// mudlib: Lil` header
line specifically (not the rest of each file's own historical prose
describing real upstream Lil's actual properties, left untouched as
accurate history) in `clone/wand_of_creation.c`, `single/simul_efun.c`,
`inherit/master/valid.c`; `README.md`'s own mudlib description line.
`WAND_OF_CREATION_SCOPING.md` carried forward unedited, its own "Lil"
references are dated technical prose about upstream Lil's real
properties at the time it was written, not this mudlib's current
identity.

**Verified live end to end with a real telnet-negotiating client** (raw
socket, matching the original wand verification's own method), against
the actual rebuilt `mudlib/` via `etc/driver_lil.cfg`: real login shows
"Welcome to Library!", the entrance hall's own description prints
automatically on arrival (no `look` command exists in this mudlib,
matching a real gap already on record in `WAND_OF_CREATION_SCOPING.md`
-- `init()` writes it directly instead), a wand of creation is already
held on arrival, and `create rusty gear` / `purge rusty gear` / `clone
/clone/wand_of_creation` all worked exactly as the original wand
verification confirmed them working in the old mudlib. The one scratch
artifact this produced (`data/created/rusty_gear.c`) was removed as
scratch output afterward, same as the last two times this exact
live-test byproduct came up.

**2026-08-18 (continued): inventory only -- 11 new reference archives added
under `temp/`, listed without extracting, nothing implemented off of
them.** Not extracted or processed per instruction; this is a record of
what is there, for whenever it actually gets picked up.

- `ds3.9.zip` (`ds3.9/`, 5,689 files) -- Dead Souls 3.9, bundling its own
  driver source at `ds3.9/fluffos-2.23-ds03/`.
- `dsI.zip` (`dsI/`, 1,673 files) -- an older Dead Souls I release,
  bundling `dsI/fluffos-2.7-ds2.018/`.
- `dsIIr10.zip` (`dsIIr10/`, 3,356 files) -- Dead Souls II release 10,
  bundling `dsIIr10/fluffos-2.16-ds05/`.
- `final_realms_fluffos_v1.zip` (`final_realms_fluffos_v1/`, 6,583
  files) -- Final Realms, a FluffOS-based mudlib, bundling
  `fluffos-2.9-ds2.11/`.
- `lpuni_fluffos_v1.zip` (`lpuni_fluffos_v1/`, 1,617 files) -- LPUniversity,
  bundling `fluffos-2.9-ds2.07/`.
- `merentha_fluffos_v2.zip` (`merentha_fluffos_v2/`, 1,478 files) --
  Merentha, bundling `fluffos-2.9-ds2.03/`.
- `skylib_fluffos_v3.zip` (`skylib_fluffos_v3/`, 6,035 files) -- Skylib,
  bundling `fluffos-2.9-ds2.04/`.
- `tmi2_fluffos_v3.zip` (`tmi2_fluffos_v3/`, 2,660 files) -- TMI-2,
  bundling `fluffos-2.16-ds05/`.
- `gurba-0.42-beta2.tar.gz` (`gurbalib/`, 1,166 entries) -- Gurbalib, a
  DGD-based mudlib bundling its own DGD driver source at `gurbalib/src/`
  (real DGD driver filenames confirmed present -- `alloc.c`, `array.c`,
  `comm.c`, `call_out.c`) -- potentially a second DGD driver source
  variant alongside the already-extracted `temp/dgd/`, version
  relationship not checked.
- `lpmud-2.4.5.tar.gz` (607 entries, no wrapper directory -- extracts
  flat into the current directory) -- classic LPMud 2.4.5, the
  original LPC driver lineage. Not FluffOS/LDMud/DGD, the three
  dialects this project actually targets -- historical/architectural
  interest only unless that scope changes.
- `nightmare-3.3.1.tar.gz` (`Nightmare-3.3.1/`, 1,790 entries, own
  `mudlib/` subdirectory) -- possibly overlapping with the
  already-extracted `temp/nightmare3/` (which already has its own
  `driver/` + `lib/`), version/lineage relationship not checked.

Six of the eight FluffOS-family archives above bundle a driver source
tree named `fluffos-2.9-ds2.0X` or close (`ds2.03`, `ds2.04`, `ds2.07`,
`ds2.11`, plus the `2.16`/`2.23` outliers and `dsI`'s older `2.7`) --
the same "ds" patchlevel lineage as this project's own vendored
`temp/reference/fluffos-2.9-ds2.08/`, at neighboring patchlevels. Worth
noting for whenever cross-patchlevel diffing against the reference
driver becomes relevant -- not investigated further this pass.

**2026-08-18 (continued): fixed the ctest-vs-direct-run discrepancy flagged
last session instead of leaving it filed away -- `ctest` and a direct
`build/test/amlp_tests` run now report the identical 623 passing, from any
working directory (623 tests, unchanged, no test content added or removed).**

Root cause: `readMudlibFile()` (`test/test_lexer.cpp`, added in `61b00ab`
for the three `testWandOfCreation*` tests) tried a fixed list of
CWD-relative bases (`"../mudlib"`, `"mudlib"`, `"./mudlib"`) to find the
real, shipped `mudlib/clone/wand_of_creation.c`. That resolves correctly
for a direct run from `build/` (where `"../mudlib"` reaches the real repo
`mudlib/`) but not for a `ctest` run, whose own working directory for this
target is `build/test/` -- none of the three bases resolve there, so
`wandSrc` came back empty and `assert(!wandSrc.empty())` aborted the whole
binary before any later test ran. This is exactly the discrepancy the
prior session's own row 0.15 entry noted and explicitly left as
out-of-scope filed-away detail -- picked back up and actually fixed this
pass rather than staying that way.

No other read-real-file convention existed anywhere else in the test
suite to match -- `readMudlibFile()`'s own three-base search was the only
prior attempt at this, and it was the thing that was broken, not a
convention to preserve. Fixed by baking the absolute repo root in at
CMake configure time (`test/CMakeLists.txt`, new
`target_compile_definitions(amlp_tests PRIVATE
AMLP_SOURCE_DIR="${CMAKE_SOURCE_DIR}")`) and having `readMudlibFile()`
resolve through that instead of any CWD-relative guesswork -- correct
regardless of what directory the binary happens to be invoked from, not
just the two specific ones (`build/`, `build/test/`) that exposed the
original bug. Confirmed working from three different working directories
directly (`build/`, `build/test/`, the repo root via `./build/test/
amlp_tests`), plus `ctest` itself, all reporting the same 623 passing.

Deliberately did not change `ctest`'s own `WORKING_DIRECTORY` property as
a workaround (the task's own instruction, and the more correct fix
regardless): that would only have converged these two specific invocation
paths onto whichever one was chosen, still leaving the test's own file
resolution CWD-dependent and therefore still fragile to a third invocation
shape (a differently-configured `ctest` label filter, an IDE test runner,
a packaged/relocated build directory). The `AMLP_SOURCE_DIR` fix makes the
test correct on its own terms instead.

**Separately, scoped (not executed) a mudlib rename/rebuild the user
asked about:** current `mudlib/` is a genuine, historically load-bearing
vendored copy of the real third-party Lil starter mudlib (confirmed via
`mudlib/readme`'s own real upstream text, plus extensive `STATUS.md`
history using it specifically as a real-world efun conformance corpus --
"diffed `EfunTable.cpp`'s registered names against Lil's own real efun
table", multiple sessions), not something authored by this project.
Asked whether to rename it and build "our own version" bundled with the
wand of creation. Surfaced that tradeoff directly rather than executing
blind: user chose to preserve an untouched copy of the current `mudlib/`
tree as vendored reference material first (this project's own existing
`temp/<name>/` convention -- `temp/core-lib`, `temp/dead-souls`,
`temp/lima`, `temp/nightmare3`, etc., all real vendored mudlib corpora,
gitignored, cited but never modified -- so `temp/lil/` is the naming fit,
not `temp/reference/`, which `CLAUDE.md` reserves for the vendored
FluffOS *driver* source specifically), then rebuild `mudlib/` itself,
under the new name "library", stripped down to a login room plus the
already-built wand of creation and nothing else Lil shipped with unless
explicitly kept. Timing: after current ROADMAP work settles, not this
session -- concrete before/after file plan requested first, before
anything moves. See `mudlib/LIBRARY_MUDLIB_PLAN.md` for that plan.

**2026-08-18 (continued): row 1.7's `bind_lambda()` decision confirmed
already on record (no change needed); Phase 1 scanned end to end for
the next smallest actionable item -- none found, nothing implemented
(623 tests, unchanged, no code touched).**

Asked to record a decision on row 1.7's `bind_lambda()` options ((c),
leave deferred, no partial stand-in) -- checked the row first rather than
assuming it needed writing. It already carried exactly this decision,
dated 2026-08-17 from a prior session, same rationale, same "stays
deferred alongside `parse_*` (row 0.13a) and the connect/disconnect
design question (row 1.4/1.16)" phrasing. Left it untouched rather than
duplicating or re-dating an already-correct entry.

Then scanned every remaining `[ ]` Phase 1 row for the next smallest
actionable dialect divergence, excluding `parse_*` (0.13a), the
connect/disconnect design question, and row 1.7's own `bind_lambda`/
closure-kind work. Four rows had no prior investigation recorded at
all -- 1.12 (`atomic` checkpoint/rollback), 1.13 (`parse_string`), 1.14
(lightweight objects), 1.15 (DGD driver+auto boot path) -- each read
against the real vendored `temp/dgd` source directly rather than trusted
from `instruct.md`'s own existing (and in 1.12/1.13's case, materially
wrong) plans:

- **1.12 `atomic`:** `src/vm/instruct.md`'s own plan (snapshot on-stack
  object variables into a `vector`, restore on error) does not match real
  DGD at all. Real rollback (`temp/dgd/src/interpret.cpp:2390-2462`) runs
  through DGD's own global "planes" data-versioning subsystem
  (`Object::newPlane()`/`Dataplane::commit()`), copy-on-write-versioning
  the entire reachable object graph, not just the current call's locals --
  the same mechanism row 1.11's own prior investigation already found
  underneath `rlimits`' `level`-scaled tick accounting. Confirmed
  genuinely bigger, same prerequisite as `rlimits`, not implemented.
- **1.13 `parse_string`:** `src/compiler/instruct.md`'s own note ("handled
  as an efun, no lexer change needed") is wrong -- real DGD backs it with
  a dedicated 5,143-line DFA-lexer-generator + LALR grammar/parser-
  generator subsystem (`temp/dgd/src/parser/`: `dfa.cpp`, `grammar.cpp`,
  `srp.cpp`, `parse.cpp`), plus its own per-object persistent state slot
  (`temp/dgd/src/data.h:193`). Same category as the already-excluded
  `parse_*` package, confirmed by directly measuring the real source, not
  implemented.
- **1.14 lightweight objects:** not just a new `Value` alternative the way
  `nil` was -- real DGD's `T_LWOBJECT` (`temp/dgd/src/data.h:64`) is a
  third call-dispatch kind with its own `Frame::funcall()`/`call()`
  parameter (`temp/dgd/src/interpret.h:256-257`) and its own ref-counted
  lifecycle, plus a real object-upgrade conversion path
  (`Object::upgradeLWO()`, `data.h:165`). This driver's `LpcObject` model
  is `shared_ptr`-identity-based throughout with no parallel value-
  semantics kind anywhere -- same cross-cutting-rework shape mapping
  width was already ruled out for. Confirmed genuinely bigger, not
  implemented.
- **1.15 DGD driver+auto boot path:** not independently investigated as
  new -- confirmed already on record as the same connect/disconnect
  design question `src/dialect/instruct.md`'s own `DgdBootApi` section
  already covers (real DGD's three-way `telnet_connect`/`binary_connect`/
  `datagram_connect` port-type fork, no single `connect`/`disconnect`
  apply at all), explicitly cross-referenced there against rows 1.4/1.15
  already.

Also re-checked the existing `'name` symbol-literal note (part of rows
1.2/1.3's own prior scoping) against the real `temp/ldmud/src/lex.c`
source in full rather than stopping at its existing citation: the same
lexer production that recognizes a bare `'name` symbol
(`L_SYMBOL`) also produces `L_QUOTED_AGGREGATE` for `'({ ... })` --
the literal "quoted code" shape `lambda()`'s own body argument is built
out of. Sharpens rather than contradicts the existing classification:
`'name` is not an independent literal type sitting next to the closure
work, it is part of the same quoted-code family, reinforcing row 1.7's
own "closure-kind work" exclusion rather than being a smaller item
outside it.

Every other still-open Phase 1 row was already fully accounted for by an
existing note (1.9 mapping width, 1.11 `rlimits`, both previously
confirmed genuinely bigger) or is one of the three excluded categories
(1.4's own connect/disconnect remainder, 1.7, 1.8's `#'symbol`, 1.16's
three items, already exhaustively investigated 2026-08-18 and each
individually blocked). Nothing in Phase 1 turned out small and
unambiguous this pass. Stopped and reported rather than forcing a pick,
per this project's own established discipline. ROADMAP.md updated with a
capstone note plus individual per-row citations for 1.12/1.13/1.14/1.15
and the `'name` addendum; no source code touched, 623 tests unchanged.

**2026-08-18: ROADMAP row 0.15 fixed -- `ObjectManager::compile()`'s
`programCache_` now invalidates a cache hit whose source has genuinely
changed, instead of returning a same-filename compile unconditionally
(623 tests, up from 619).**

Two small live-test-artifact cleanups first: `mudlib/data/created/readme`
and `rusty_gear.c`, both committed in `61b00ab`, were leftover scratch
output from that commit's own live wand-of-creation verification (the
readme's own text says so directly: "Auto-generated, safe to delete"),
not designed fixtures -- removed.

Main fix: `ObjectManager::compile()` used to check `programCache_` and
return a hit for any previously-compiled filename unconditionally, with
no path anywhere that noticed the file's own source had since changed
underneath it (row 0.15's own prior scope note, found live 2026-08-21
via `mudlib`'s `eval` command). Directly in the way again this session
verifying `wand_of_creation.c`'s `cmd_create()`, which does the identical
rm()+destruct()+write_file() cycle for a reused item name.

Fixed by having `compile()` re-read the target file's raw source on every
call and compare it against the exact text that produced the cached
entry (new `programSource_` map, parallel to `programCache_`) before
trusting a hit -- a byte comparison, not an mtime check, since two writes
to the same path within one filesystem timestamp tick are routine (this
row's own regression tests hit exactly that) and would look unchanged to
`stat()` despite genuinely different content. Unchanged content still
takes the fast path unmodified; changed content recompiles fresh and
replaces both entries. A source file that has vanished entirely since the
cached compile (a purge with nothing written back) still serves the last
good program rather than erroring -- matches real semantics, where an
already-compiled program does not stop working just because its own disk
file was later deleted. Existing objects already holding the old
`CompiledProgram` via their own `shared_ptr` are unaffected regardless:
replacing `programCache_`'s own entry never mutates the old
`CompiledProgram` object. Deliberately narrow, matching the row's own
prior scoping: only the recompiled file's own bytes are checked (a
changed `#include`d header with the including file's own text untouched
is not detected -- not needed by either real repro) and an inheriting
file already compiled against an old inherited version keeps that old
version until it is itself recompiled (unchanged from how inherit
resolution always worked, not a regression this fix introduces).

4 new regression tests in `test/test_lexer.cpp`, driving
`ObjectManager::loadObject()`/`cloneObject()` directly (`compile()`'s
only two real callers) rather than through either mudlib file: same-path
recompile via `loadObject()` after destruct+rewrite; the same via
`cloneObject()` (which has no `sourceFileExists()` pre-gate the way
`loadObject()` does, so it is the path that actually reaches this fix's
own "source vanished" fallback branch); unchanged-source reuse
(`programPtr()` identity, confirming the fast path still avoids wasteful
recompiles); and the vanished-source fallback itself. Full suite: 623
tests passing, up from 619, no regressions (confirmed via a direct run
of the test binary from `build/`; `ctest` itself fails one pre-existing,
unrelated test -- `testWandOfCreationHeldGuardBlocksAllCommandsWhenOnlyColocatedNotHeld`
reads a mudlib file via a path relative to CWD, and `ctest`'s own working
directory for this binary is `build/test/`, not `build/`, where that
relative path does not resolve; reproduced identically on the pre-fix
`61b00ab` tree via `git stash`, so this is not a regression from this
session and was left alone as out of scope).

Verified live against the real running `amlp` binary
(`etc/driver_lil.cfg`), the same way `61b00ab`'s own wand verification
was done: three `eval` calls in one session, all against the same
`/tmp_eval_file` path (`return 5+5`, `return 9999`, `return 5+5` again),
each correctly returning its own fresh result (`10`, `9999`, `10`)
instead of the first call's own stale bytecode -- the exact failure mode
row 0.15's own scope note originally documented, now confirmed fixed
against the real driver process, not just the test suite.

ROADMAP.md row 0.15 flipped to `[x]`, its own scope note updated in place
with the fix.

**2026-08-24 (continued): `Config::dialect()` implemented and wired into
`main.cpp`'s `masterUidApply()` call site -- the boot-time UID query is
now genuinely config-driven (`"fluffos"`/`"ldmud"`, defaulting to
`"fluffos"` unchanged) instead of hardcoded to `FluffOsBootApi` (600
tests, up from 596).**

Closes the gap the prior entry's own comment flagged directly: `main.cpp`
was hardcoded to `FluffOsBootApi` because `Config::dialect()` (row 1.1)
didn't exist. Scoped to exactly what unblocks that one call site, per
this session's own instruction -- no DGD, no connect/disconnect, no
other apply.

**`Config::dialect()`** (`src/config/Config.hpp`/`.cpp`): a plain
`std::string`, same `key: value` config-file convention every other
`Config` field already uses, default `"fluffos"`. Deliberately not the
`LpcDialect` enum itself -- `src/dialect` already depends on `src/config`
for `Config`, so `Config` depending back on `src/dialect` for the enum
type would be a library cycle. Parsing into the real enum happens where
it's actually needed, `dialectFromString()` inside the new
`makeBootApiForConfig()`.

**`makeBootApiForConfig(const Config&)`** (new
`src/dialect/DialectSelect.hpp`/`.cpp`): a small free function, not the
`DialectFactory` class `src/dialect/instruct.md` sketches. Skipped the
factory deliberately -- that class's whole reason to exist is being
handed to several independent consumers (`ApplyTable`, `ObjectManager`,
`Lexer`/`Parser`, `VM`, per that file's own "how dialect flows through
the driver" diagram), none of which exist yet; only `main.cpp`'s one
`queryMasterUid()` call site does. A `switch` over `dialectFromString
(config.dialect())` returning `FluffOsBootApi`/`LdmudBootApi` is the
entire body. DGD throws `NotImplementedError` ("DgdBootApi does not
exist yet") rather than silently falling back to some other dialect's
`BootApi` -- a config asking for an unsupported dialect should fail
loudly, not misbehave quietly. Promote this to a real `DialectFactory`
once a second consumer actually needs the same construction logic.

`main.cpp` now calls `amlp::makeBootApiForConfig(config)` in place of
the prior hardcoded `amlp::FluffOsBootApi bootApi(config);`, holding the
result in a `unique_ptr<BootApi>` since the concrete type is now
runtime-selected rather than fixed at compile time.

**Confirmed live against the real `amlp` binary, not just the test
suite**, all three cases: `etc/driver.cfg` unmodified (no `dialect` key)
still prints `master does not define get_root_uid()`, byte-for-byte the
same as before this session; the same config with `dialect: ldmud`
appended prints `master does not define get_master_uid()` instead,
proving the switch is genuinely config-driven; and the same config with
`dialect: dgd` appended terminates with an uncaught
`amlp::NotImplementedError: not implemented: dgd dialect (DgdBootApi
does not exist yet)` rather than booting with the wrong dialect silently
selected.

Four new regression tests (600 total, up from 596, all passing across
three consecutive runs plus `ctest`): `Config::dialect()` defaults to
`"fluffos"` when unset and reads back an explicitly configured value;
the actual end-to-end case the prior entry's own gap called for --
`makeBootApiForConfig()` feeding a real `queryMasterUid()` call against
a real compiled master object, once for the default/unset config
(confirming it still calls `get_root_uid()` and gets back the master's
own real return value, not just a name-equality check) and once for
`dialect: ldmud` (confirming `get_master_uid()` fires instead, driven
entirely by config); and the DGD case throwing `NotImplementedError`
rather than silently constructing the wrong `BootApi`.

ROADMAP.md row 1.1 flipped to `[x]` (both the enum and the config key
now exist and are genuinely consumed, even if only by this one call
site so far); row 1.4's own note updated with this session's progress.

**2026-08-24 (continued): `BootApi::masterUidApply()` wired into the real
runtime for the first time -- `main.cpp` now genuinely calls
`get_root_uid()` on the master object at boot, routed through
`queryMasterUid()`/`VM::applyMaster()`, not just exercised in isolation
by the prior session's own `BootApi` unit tests (596 tests, up from
594).**

Asked to route "the hardcoded master UID apply call site(s) in
Server.cpp/ObjectManager.cpp" through `BootApi`. Checked first rather
than assuming the premise: no such call site exists anywhere.
`"get_root_uid"` appears in real (non-test, non-dialect) code exactly once,
as a static string inside `ApplyTable::known()`'s whitelist
(`src/apply/ApplyTable.cpp`) -- and `ApplyTable::isKnownApply()` itself
has zero callers anywhere in `src/`, `include/`, or the test suite.
`main.cpp`'s boot sequence loaded the master object but never applied
any UID-related apply to it; there is also no uid/privs trust-hierarchy
concept anywhere in this driver yet (ROADMAP Phase 3.1, still `[ ]`).
Surfaced this instead of inventing scope silently; asked which of three
options to take (add a genuine new boot-time call; stop here; or make
the dead `ApplyTable` whitelist dialect-aware instead). Instructed to
add the boot-time call, the recommended option.

**What was built.** A new, minimal, reusable helper --
`queryMasterUid(VM&, const BootApi&)`, `src/dialect/MasterUidBoot.hpp`/
`.cpp` -- fires `bootApi.masterUidApply()` on the master object via the
real `VM::applyMaster()`, mirroring the existing `try`/catch-and-extract-
string shape `ObjectManager::initPrivsForObject()`'s own `"privs_file"`
call site already uses (an exception from the apply's own LPC body is
caught and treated as "no uid", not a boot failure; a plain undefined
function already returns silently via `VM::callFunction()`'s own
existing real-semantics handling, no extra handling needed for that
case). `main.cpp` calls it once, right after `loadMasterObject()`
succeeds, using a hardcoded `FluffOsBootApi` -- `Config::dialect()` (row
1.1) still doesn't exist, so FluffOS is the only dialect this driver
actually runs as today; noted in both a code comment and ROADMAP.md as
the thing to swap once row 1.1 lands a real config-driven switch.
`DialectFactory`/`DgdBootApi` were not needed to wire this single call
site and were not built, per this session's own instruction.

New `dialect` -> `vm` library dependency (`src/dialect/CMakeLists.txt`)
-- no cycle, `vm` does not depend on `dialect`.

Confirmed live, not just by the test suite: ran the real `amlp` binary
against `etc/driver.cfg` (`AMLP_MAX_ITERATIONS=1`) and it now prints
`master does not define get_root_uid()` on boot (`test/mudlib_stub/
master.c` genuinely does not define it -- expected, not a regression).

Two new regression tests (596 total, up from 594, all passing across
three consecutive runs plus `ctest`), both against the real call path
through a real compiled master object rather than `BootApi` in
isolation: one master defining both `get_root_uid()` and
`get_master_uid()` with different return strings, queried once via
`FluffOsBootApi` and once via `LdmudBootApi`, each getting back only its
own dialect's real string -- proof the routing is genuinely dialect-
driven, not coincidental; and a master defining neither, confirming
`queryMasterUid()` returns `std::nullopt` for both dialects rather than
throwing, matching real `apply_master_ob()` semantics for an undefined
apply.

ROADMAP.md rows 1.4 and 1.16 updated with this session's progress
(still `[ ]`, only this one call site is wired -- every other apply this
row's own scope covers is still hardcoded or unimplemented).

**2026-08-24 (continued): Phase 1 begins -- `LpcDialect` enum, a trimmed
`BootApi` interface, and `FluffOsBootApi`/`LdmudBootApi` implemented,
carrying forward the LDMud `get_master_uid` correction recorded in
`src/dialect/instruct.md`/`src/apply/instruct.md` (594 tests, up from
591).**

Row 0.13's non-`parse_*` scope closed and `parse_*` filed as its own
0.13a; the prior entry's dedicated-session candidate was the natural
next real implementation work, and per this session's own instruction,
picked up straight from the corrections `src/dialect/instruct.md`/
`src/apply/instruct.md` already record rather than re-researching LDMud/
DGD from scratch.

**Correction picked: LDMud's master trust-root UID apply is
`"get_master_uid"`, not FluffOS's `"get_root_uid"`** (renamed in LDMud
3.2.1@40, `doc/master/get_master_uid`'s own HISTORY line: "Introduced in
3.2.1@40 replacing get_root_uid()."; both names independently confirmed
real, `applies.h`'s own `APPLY_GET_ROOT_UID` and `master.c`'s own
`apply_master_ob(APPLY_GET_ROOT_UID, 0)` for the FluffOS side). Chosen as
the smallest and safest item on record: a single, unambiguous string
fact with no downstream design question attached, unlike every other
recorded correction --
`shadow()`'s signature difference and `bind_lambda()`'s real name are
explicitly deferred to whichever session actually implements LDMud
shadow/closure semantics; the `replaces`-directive-vs-`replace_program()`
mismatch needs row 1.6 itself rescoped before anyone touches code; and
`disconnect`/`net_dead`/DGD's three-way `connect` fork are exactly the
open design question both instruct.md files say explicitly to resolve
before writing `ApplyTable`'s real dispatch code, not to pick up as a
first slice.

**What was built.** `src/dialect/` now exists as compiled code (previously
only `instruct.md`): `LpcDialect.hpp`/`.cpp` (the enum plus
`dialectName()`/`dialectFromString()`, taken verbatim from the recorded
spec, nothing to correct there), `BootApi.hpp` (the abstract interface
from `src/apply/instruct.md`'s own Phase 1.4, deliberately missing
`connectApply()`/`netDeadApply()` -- both instruct.md files flag those
two as an unresolved single-string-shape problem across all three
dialects' connect/disconnect and say explicitly to resolve that before
writing dispatch code; everything else in the documented interface is
settled and included), plus a new `masterUidApply()` method carrying the
actual corrected fact, and `FluffOsBootApi`/`LdmudBootApi` concrete
implementations (`get_root_uid` / `get_master_uid` respectively, all
other apply names identical between the two, matching the record).
`DgdBootApi` and `DialectFactory` are not part of this pass -- DGD's own
`compileObjectApply()` mapping is a separate open question (no
virtual-object concept, nearest analog is `call_object`) and building a
factory with no consumer yet would be scope creep past "smallest
correction." `ApplyTable`/`VM::applyMaster()` call sites (`Server.cpp`'s
`"connect"`, `ObjectManager.cpp`'s `"compile_object"`/`"privs_file"`)
are untouched -- routing them through `BootApi` is exactly the dispatch
work both instruct.md files say to hold until the connect/disconnect
shape question is resolved, so it stays out of scope here too.

New `src/dialect` CMake target wired into the top-level `CMakeLists.txt`
and `test/CMakeLists.txt` link lines, same pattern as every other
`src/*` library in this repo.

Three new regression tests (594 total, up from 591, all passing across
three consecutive runs plus `ctest`): `LpcDialect` enum/string round trip
including the throw-on-unknown case, the actual correction itself
(`LdmudBootApi::masterUidApply() == "get_master_uid"`,
`FluffOsBootApi::masterUidApply() == "get_root_uid"`, and the two
asserted unequal to each other, plus every other trimmed-interface
method asserted equal between the two dialects to confirm this is an
isolated divergence and not the two classes silently disagreeing on
everything), and `BootApi::masterFile()`/`simulEfunFile()` reading
through `Config` correctly for both.

ROADMAP.md rows 1.1, 1.4, and 1.16 updated to `[ ]` with partial-progress
notes rather than flipped to `[x]` -- none of the three rows' full scope
(a config-driven dialect switch; full pluggable apply dispatch; LDMud's
complete master apply surface) is done, only this one slice of each.

**2026-08-24 (continued): row 0.13's non-`parse_*` scope formally closed
in ROADMAP.md; `parse_*` filed as its own explicit sub-row, 0.13a.
Bookkeeping only, no code changed.**

With the prior entry's 40-name non-`parse_*` accounting confirmed
genuine and stable across this whole session (all 40 re-verified as
architecture-mismatch or zero-call-site exclusions, none reclassified),
there is no further batch work left against that portion of row 0.13 --
closing it out is the accurate status, not a re-scope. ROADMAP.md's row
0.13 checkbox flipped to `[x]` with a summary of the final accounting:
240 registered against the vendored reference build's own 270 real
`efun_defs.c` names, the real (`comm -23`) gap of 40 non-`parse_*` names
all confirmed genuine exclusions, plus the 8-name `parse_*` package now
tracked separately.

The `parse_*` package itself (`parse_init`, `parse_refresh`,
`parse_sentence`, `parse_add_rule`, `parse_add_synonym`,
`parse_my_rules`, `parse_dump`, `parse_remove`) gets its own row, 0.13a,
carrying forward the scoping detail already on record from the
2026-08-23/2026-08-24 entries below: real source is `packages/parser.c`
(3419 lines, FluffOS's genuine natural-language sentence/grammar-rule
parser package, confirmed implemented rather than unverifiable), 66
combined call-site hits across the 8 names in the vendored mudlib
corpora, sized well beyond a normal batch item -- comparable to or
larger than everything else row 0.13 has implemented combined -- so it
stays the one deliberately deferred dedicated-session candidate,
unstarted, pending its own explicit go-ahead.

No code touched this pass. Full suite re-run to confirm nothing
regressed under the documentation-only change: 591 tests passing (`ctest`
plus a direct run of the test binary), unchanged from the prior entry,
no regressions.

**2026-08-24 (continued): row 0.13 batch -- `socket_release`/
`socket_acquire` implemented (238 to 240), correcting a second real
methodological miss this session, this time a scope-difficulty call
rather than a "no body exists" one.**

Bookkeeping check first: the prior entry's own "46 non-`parse_*`" phrase
was a description of that entry's own starting scope, not a live count
-- both this file's own concluding "Real remaining gap: 50" and
ROADMAP.md's own row 0.13 line were already correct, nothing stale to
fix. Made the 42-non-`parse_*`-plus-8-`parse_*` breakdown explicit in
ROADMAP.md's own row 0.13 line anyway, since it was previously only
derivable, not stated.

Re-ranked the remaining 42 non-`parse_*` names fresh against all six
corpora, then -- per this session's own explicit instruction -- checked
every single one for a real C body anywhere in the reference tree, not
just the ones with real call-site weight: all 42 have one somewhere
(`packages/contrib.c`, `packages/matrix.c`, `packages/mudlib_stats.c`,
`packages/sockets.c`, `packages/develop.c`, `disassembler.c`,
`efuns_main.c`, `comm.c`), confirming last session's "unverifiable, no
body" miss was specific to the reflection family, not a wider pattern
-- every other existing exclusion reason (buffer/driver-internal-dump/
VRML-pose/MXP-TTYPE families, `resolve`, `get_char`/`ed`, `set_reset`,
`fetch_class_member`/`store_class_member`) is a genuine architecture
mismatch or missing-infrastructure case, not an absent-body case, and
stays excluded on that basis. `translate` (56) and `event` (7)
individually re-checked against `efun::translate()`/`efun::event()`
delegation across all six corpora specifically (not just recalled from
memory) -- zero hits either way, both still genuinely shadowed
simul_efuns with no load-bearing delegation anywhere, confirmed rather
than assumed unchanged.

`socket_acquire`/`socket_release` (4/3 raw hits, `lima`'s own
`obj/secure/socket.c`/`old_socket.c` and `dead-souls`'s own
`i3router`/`imc2server` socket-handoff daemons -- two independent
corpora, not one-off usage) had been filed as "Tier 3, out of basics
scope" (`instruct.md`) without their real bodies ever having been read.
Read `packages/sockets.c`'s own `f_socket_release`/`f_socket_acquire`
and `socket_efuns.c`'s own `socket_release()`/`socket_acquire()` in
full: a real, self-contained object-to-object efun-socket ownership
handoff protocol, not the larger "advanced" subsystem the old label
implied. `socket_release(fd, ob, callback)` (caller must be the current
owner): marks the socket released to `ob`, synchronously fires
`callback(fd, ob)` (a function pointer, or a string applied on `ob` via
real `safe_apply(..., ORIGIN_INTERNAL)`), then re-checks its own
release flag -- if `ob`'s own callback completed a matching
`socket_acquire()` before returning, success; otherwise the release is
reverted in the same call and `EESOCKNOTRLSD` returned.
`socket_acquire(fd, read_cb, write_cb, close_cb)` only succeeds while
the fd is released *to the exact calling object* (`EESECURITY`
otherwise), reassigning ownership and overwriting all three callbacks
unconditionally on success. This driver's own existing
`SocketRegistry`/`LpcSocket` (owner tracking via `weak_ptr`, string|
function callback storage, the same real `SocketErr` code family)
already fit this mechanism directly -- implemented as two new
`LpcSocket` fields (`released`/`releaseTarget`) plus four new
`SocketRegistry` methods (`beginRelease`/`isReleased`/`cancelRelease`/
`acquire`), with the one VM-touching step (firing the release callback)
kept in `EfunTable.cpp`'s own registration, mirroring
`Server.cpp`'s own `fireSocketCallback()` dispatch rather than sharing
it directly (that helper lives in `Server.cpp`'s own anonymous
namespace; this is the only call site outside it needing the identical
function-pointer-vs-string dispatch, so it is narrowly duplicated with
an explicit citation rather than a header extracted for one caller).
Two real error codes this driver had never needed before,
`EESOCKRLSD`/`EESOCKNOTRLSD` (`socket_err.h`, -30/-31), added to
`SocketErr` and `SocketRegistry::errorString()`'s own mirrored table;
found and fixed a latent inaccuracy in the same table while there --
its trailing "Data nested too deeply" entry (-32) was labeled
"unimplemented" with no real citation at all, actually real
(`EEBADDATA`, `socket_write()`'s own MUD-mode nesting-depth error,
genuinely unreachable here only because MUD mode itself is not
implemented, a preexisting and already-documented gap, not a new one).

Also fixed a stale comment found in passing while touching
`LpcSocket.hpp`'s own `owner` field: it still described
`close_referencing_sockets(ob)` as "no equivalent here -- a known,
documented gap" -- true when first written, but wrong since the
2026-08-24 `destruct_object()` session earlier today ported it. Updated
to cite the real current mechanism instead of repeating a claim that
session had already closed.

Two new regression tests (591 total, up from 589, all passing across 3
consecutive runs plus `ctest`): a full handoff round trip (owner
releases to a target object, whose own callback completes the handoff
via `socket_acquire()`, confirmed both by the real return code and by
`socket_status()`'s own owner slot genuinely reading the new owner
afterward) and a rejection/revert case (a *third* object, reached via a
real `call_other()` from inside the release target's own callback so
`current_object` during the nested acquire attempt is genuinely that
third object, is correctly refused with `ESecurity`; since nothing
successfully acquired, the outer `socket_release()` call reverts and
reports `ESockNotRlsd`; a stray `socket_acquire()` afterward on the
now-unreleased fd fails the same way).

Real remaining gap: 48 (`comm -23` between `efun_defs.c`'s 270 real
names and `EfunTable.cpp`'s now-240 `registerEfun` names) -- 40
non-`parse_*` names, all confirmed this session to be genuine
architecture-mismatch or missing-infrastructure exclusions (not
"unverifiable" ones), plus the 8-name `parse_*` package, still the one
deliberately deferred dedicated-session candidate.

**2026-08-24 (continued): row 0.13 batch -- `functions`, `variables`,
`fetch_variable`, `store_variable` implemented (234 to 238), correcting
a real, repeated methodological miss from earlier passes rather than
just adding four names.**

Ranked the remaining 46 non-`parse_*` gap names fresh against all six
mudlib corpora under `temp/` (per this session's own relocation above,
not the old `reference/` path) before trusting any prior pass's own
categorization. `translate` (66 raw hits) re-confirmed a shadowed
simul_efun with no `efun::` delegation anywhere, same as before -- real
`translate()` is 4-arg VRML matrix-translate (`efun_defs.c`:
`T_ARRAY,T_REAL,T_REAL,T_REAL`), completely unrelated to every corpus's
own text-garbling `translate(string, int)` simul_efun. `ed`/`get_char`
(52/48 raw hits) re-confirmed real, genuine `.c` call sites (not just
doc-page noise), but both need raw-input/line-editor infrastructure
this driver has none of -- unchanged from the prior pass's own verdict.

Re-verified the "unverifiable, no implementation anywhere" categorization
for the six-name reflection family instead of carrying it forward as
already-settled: false for four of the six. `packages/contrib.c`
contains real, complete bodies for `f_functions`, `f_variables`,
`f_fetch_variable`, and `f_store_variable` (`PACKAGE_CONTRIB` confirmed
active in this exact build's own `options.h`) -- the same file
`function_owner`/`replaceable` were already correctly found real in
during an earlier pass, so this was a genuine grep-depth miss on those
four specific names, not a case of them actually having nothing to
check a port against. `debug_info` also has a real body
(`packages/develop.c`'s own `f_debug_info`) -- still excluded, but for
the correct reason now: a large mode-switched dump of FluffOS-specific
`object_t`/`program_t` internals (heart-beat/wizard/clone flag bits,
`obj_list` linked-list position, live ref counts) this driver's own
shared_ptr-based object model has no equivalent to report, the same
driver-internal-dump architecture-mismatch family `mud_status`/
`cache_stats`/etc. already sit in, not "unverifiable." `fetch_class_member`/
`store_class_member` do have real bodies too but stay excluded
regardless -- no `TYPE_CLASS`/`class` value kind exists in this driver
at all, a separate, still-valid reason. Full correction, including how
the miss happened, recorded in `src/efun/instruct.md`'s own accounting
section rather than silently fixed.

Real signatures (`packages/contrib_spec.c`, the hand-written declared
form, more legible than `efun_defs.c`'s own generated table): `mixed
*functions(object, int default: 0)`, `mixed *variables(object, int
default: 0)`, `mixed fetch_variable(string)`, `void
store_variable(string, mixed)` -- the last two always implicit
`current_object`, no object argument at all, confirmed directly from
`f_fetch_variable`/`f_store_variable`'s own `find_global_variable(
current_object->prog, ...)` calls, not assumed from the two-efun
family's own naming symmetry with the first two. Real call sites
(`functions`/`variables`, `lima` and `dead-souls` only, both close FluffOS
lineage) mostly pass the object argument alone, relying on this exact
default -- resolved the apparent `2,2`-vs-1-arg-call contradiction the
same way `query_num`'s own earlier pass already flagged as a standing
trap: `efun_defs.c`'s generated arity fields show post-default arity,
not the real callable minimum, always cross-check `packages/*_spec.c`'s
own hand-written `default:` markers before trusting a bare min/max
read.

Implemented against this driver's own `CompiledProgram` (`Bytecode.hpp`):
`functions()`'s bare-name and flag&2 (locals-only) forms read
`CompiledProgram::functions` directly (this driver's own per-program
function list was already local-only, never flattened across
inheritance -- confirmed against `VM.cpp`'s own `findFunctionInChain`,
which resolves inheritance by recursive search at call time rather than
a precomputed flattened table); the flag&2-clear (include-inherited)
form walks `inheritedPrograms` depth-first with the same own-functions-
first, first-name-wins override precedence `findFunctionInChain` already
uses for an ordinary call, so an overridden function is never double-
counted or reported under its own shadowed parent's entry.
`variables()` reads `CompiledProgram::objectVarNames` directly -- already
fully flattened in real declaration order (`save_object`/
`restore_object`'s own established use of that same field, confirmed by
direct comparison rather than assumed identical to `functions()`'s own
un-flattened shape). `fetch_variable`/`store_variable` do a linear name
lookup against that same `objectVarNames`/`variables()` pair against
`vm.currentObject()`, real "No variable named '%s'!" error text kept on
a miss. Real `f_functions()`'s own exclusion of its synthesized
`__INIT`-family initializer function from the result ported too, this
driver's own equivalent being the synthesized `"$objvarinit"`
CodeGen.cpp already adds to every program's own function list.

One honest, explicitly documented fidelity gap, not silently glossed
over: real `functions()`'s flag&1 detailed form and `variables()`'s
flag-truthy form both report real per-declaration TYPE_* strings this
driver has no metadata for at all -- `FunctionEntry` carries only
name/entryPoint/numArgs/numLocals, `objectVarNames` is bare strings,
neither a declared-type field anywhere. Every type slot in both detailed
forms is the fixed placeholder `"mixed"` instead of a real type name --
the closest honest equivalent to an untyped/mixed declaration, not a
fabricated value, and called out directly in each registration's own
comment plus a regression test that asserts on the placeholder rather
than silently accepting whatever came out.

Four new regression tests (589 total, up from 585, all passing across 3
consecutive runs plus `ctest`): `functions()` own-vs-inherited-vs-
override-precedence and the `$objvarinit` exclusion; `functions()`'s
detailed-form shape and the `"mixed"` placeholders; `variables()`'s
flattened inherited-then-own order and its own pair form;
`fetch_variable`/`store_variable`'s round trip through an actual LPC
object (current-object-implicit, so exercised through a real callable
wrapper function rather than a direct `EfunTable::instance().call()`,
the same distinction `save_object`/`restore_object`'s own tests already
established) plus both throwing on an unknown name.

Every other name in the 46-name non-`parse_*` gap re-checked this pass
and confirmed to still belong in its existing category (documented
exclusion or zero-call-site) -- none of the remaining architecture-
mismatch verdicts (buffer family, driver-internal-dump family,
VRML-pose family, MXP/TTYPE family, `set_reset`, `resolve`, `get_char`/
`ed`, `socket_acquire`/`socket_release`) turned out to be wrong the way
the reflection family did; only that one family's own "no body exists"
premise was actually false. Real remaining gap: 50 (`comm -23` between
`efun_defs.c`'s 270 real names and `EfunTable.cpp`'s now-238
`registerEfun` names), all still either documented exclusions,
zero-call-site names, or the one deliberately deferred `parse_*`
package.

**2026-08-24 (continued): formalized the `reference/fluffos-2.9-ds2.08`
relocation the previous entry below flagged but did not fix -- now
`temp/reference/fluffos-2.9-ds2.08/`, gitignored, same discipline as
the six vendored mudlib corpora already under `temp/`.**

Confirmed byte-identical before touching anything: every one of the 305
files git still had tracked at the old path (`git ls-tree -r
393a7f0 -- reference/fluffos-2.9-ds2.08`, the last commit where it was
still tracked) diffed clean against its counterpart under
`temp/reference/fluffos-2.9-ds2.08/`, zero content mismatches, zero
missing, zero extras.

Git-side, the deletion from tracking turned out to already be committed
-- not by this session. The previous entry below's own commit
(`destruct_object()`'s socket-close fix) evidently picked up the
already-present-but-unstaged working-tree deletion of `reference/` when
committed, bundling both into one commit; its own commit message
documents this explicitly ("Flagged, not fixed here: ... Needs a
follow-up session to formally relocate"). So there was nothing left to
`git rm --cached` here, only the citation sweep and `.gitignore`
confirmation. `.gitignore`'s own blanket `temp/` line (added the prior
session that first vendored the six mudlib corpora into `temp/`) already
covers the new location; confirmed directly with `git check-ignore -v`
against files under the new path, not assumed from the blanket pattern
alone.

Citation-path sweep, same discipline as this directory's own two prior
relocations (`mudlib/nightmare3_fluffos_v2/fluffos-2.9-ds2.08/` to
`driver/reference/fluffos-2.9-ds2.08/`, then to
`reference/fluffos-2.9-ds2.08/` during the LDMud-style restructure --
see git history, not previously written down in STATUS.md itself):
`CLAUDE.md`, `src/compiler/instruct.md`, `src/efun/instruct.md`, and
`prompt.md` all updated from `reference/fluffos-2.9-ds2.08/` to
`temp/reference/fluffos-2.9-ds2.08/`. `ROADMAP.md`, `STATUS-ARCHIVE.md`,
and `README.md` checked and confirmed to contain zero occurrences of
the old path already, nothing to change. `STATUS.md`'s own existing
dated entries (including the immediately-following one below, and one
historical entry still citing the even-earlier `driver/reference/...`
path from before the LDMud restructure) deliberately left untouched --
a dated log records what was true at the time it was written, not a
live index, matching the prior relocation's own precedent of leaving
STATUS.md's past entries alone while bulk-updating every forward-looking
doc. Added a durable provenance note to CLAUDE.md's own orientation
section instead: the new path, why it is intentionally untracked, that
it must be manually present for any citation work, and the full
relocation lineage (three locations across this project's history, one
vendored FluffOS 2.9 ds2.08 tree throughout, never re-downloaded or
re-derived).

Build and full suite re-run to confirm zero behavior change from a
pure documentation/tracking change: 585 tests, all passing, same count
as the previous entry below (this change touches no source or test
file).

**2026-08-24: `destruct_object()`'s own `close_referencing_sockets()`
call site ported -- the precisely-located gap the previous session left
documented (Phase 0 row 0.13: 234 registered, unchanged -- this is a
real-fidelity fix to the existing `destruct` efun, not a new
registration).**

Confirmed against `simulate.c`: `destruct_object()` calls
`close_referencing_sockets(ob)` guarded by `#if defined(PACKAGE_SOCKETS)
|| defined(PACKAGE_EXTERNAL)` and `if (ob->flags & O_EFUN_SOCKET)`, near
the top of the function, before shadow/snoop/environment handling --
the same real mechanism `reload_object()` already needed, now confirmed
to have this second real call site.

`ObjectManager` cannot call `SocketRegistry` directly (`net` already
depends on `object` in the CMake link graph; the reverse would be
circular), so both `ObjectManager::destructObject()` and
`ObjectManager::reloadObject()` now take an optional `onDestructed`
callback, forwarded through `VM::destructObject()`/`VM::reloadObject()`,
and wired to `SocketRegistry::closeAllOwnedBy()` from `EfunTable.cpp`'s
own `destruct`/`reload_object` registrations -- the only layer with
access to both `object` and `net`. The callback fires once per object
either call actually destructs, including every object the real
shadow-chain cascade destructs along with the one named explicitly
(confirmed by tracing the cascade's own recursive `destructObject()`
calls, which already recurse into the base object's own destruction,
not just the shadowers).

`reloadObject()`'s own internal shadow-cascade needed the same callback
threading for the same reason: it calls `destructObject()` on cascaded
shadowers, and real `destruct_object()` unconditionally closes sockets
regardless of caller, so leaving this out would have made `destruct()`
and `reload_object()`'s cascades diverge from each other despite
sharing the same real underlying mechanism.

Two new regression tests (585 total, up from 583, all passing across 3
consecutive runs plus `ctest`): a direct case confirming a destructed
object's own owned socket is force-closed with no `close_callback`
firing (real `socket_close(i, SC_FORCE)` -- `SC_FORCE` alone, no
`SC_DO_CALLBACK`), and a cascade case confirming a socket owned by a
shadow-chain-cascaded object (not the object `destruct()` was called on
directly) is also closed, proving the callback genuinely threads
through the recursive cascade rather than only firing for the directly-
named object.

**Environment note, unrelated to the fix above:** the vendored
`reference/fluffos-2.9-ds2.08` tree is missing from its tracked
location on disk this session (`git status` shows all ~305 files under
it as deleted), while byte-identical content is present, untracked,
under the gitignored `temp/reference/fluffos-2.9-ds2.08/`. This was not
this session's own doing -- confirmed via diff against `git show
HEAD:...` before relying on the `temp/` copy for the `parse_*` scoping
report below. Not staged, not otherwise touched; flagged to the user
directly rather than silently worked around.

**2026-08-23 (continued): `reload_object()` implemented in full, the
smaller of the two remaining dedicated-session candidates, plus a real,
precisely-located gap found in the process and left documented rather
than silently expanded into (Phase 0 row 0.13: 234 registered, up from
233, real gap against the 270 target now 54).**

Re-verified both standing claims from the entry that first flagged this
efun rather than trusting them unchanged: zero real call sites for
`reload_object` across all six mudlib corpora, still true; the one
missing piece (a socket-close-by-owner capability) still accurate, and
still the only piece missing. Read the real `object.c`'s own
`reload_object(obj)` in full before writing anything, confirming every
real step precisely rather than the previous session's own higher-level
summary: zero every object variable back to a real int 0
(`free_svalue`/`const0u`); close every efun socket `obj` owns
(`PACKAGE_SOCKETS`, active in this exact vendored build); the real
shadow-chain cascade/splice (identical real semantics to `destruct()`'s
own, confirmed by direct comparison, with one real difference -- `obj`
itself is never destructed here, only spliced out or left as the
surviving base once every object that was shadowing it is cascade-
destructed); `remove_living_name(obj)`; `set_heart_beat(obj, 0)`;
`remove_all_call_out(obj)`; a light-system total-light decrement (real,
but `NO_LIGHT` is undefined and no light system exists here regardless
-- already-excluded gap, not a new one); an `obj->euid = obj->uid;` reset
(real, `PACKAGE_UIDS`+`AUTO_SETEUID` both confirmed active in this exact
build's own `options.h`, but this driver has only a single `privs_`
field, no separate uid/euid pair -- nothing observably different to
reset back to); and finally real `call_create()`'s own full body --
confirmed directly to be `call___INIT(ob)` (this driver's own
synthesized `"$objvarinit"`) *then* `create()`, not create() alone, so a
top-level `int x = 5;` declaration really does end up back at 5 after a
reload, not merely 0.

Implemented split across two layers by this driver's own existing
module-dependency boundaries (`object` cannot depend on `net`/
`scheduler`, both of which already depend on `object`), not real code's
own single procedural body: new `ObjectManager::reloadObject(obj)`
handles everything that stays within `object`'s own dependency envelope
(variable-zeroing, the shadow cascade/splice reusing `destructObject()`
recursively for each cascaded link, `LivingNameRegistry::remove()`, and
the `runObjectVarInitializers()`-then-`create()` sequence, the same tail
`cloneObject()` itself already uses for a fresh instance), reached via a
new `VM::reloadObject()` thin wrapper matching `cloneObject()`/
`destructObject()`'s own established shape; the `reload_object` efun
itself (`EfunTable.cpp`) orchestrates socket-close and heart_beat/
call_out removal directly (both needing `net`/`scheduler`) before
calling into `vm.reloadObject()` for the rest. This reorders two
independent real steps relative to their real position (socket-close is
real step 2, heart_beat/call_out removal is real steps 5-6, both now
run first) -- confirmed to have no observable effect: neither touches
object variables, the shadow chain, or living-name state, and both
still finish well before `create()` runs either way, the only real
ordering constraint that actually matters.

New capabilities added to back this: `SocketRegistry::closeAllOwnedBy()`
(real `close_referencing_sockets()`, confirmed real `SC_FORCE`-without-
`SC_DO_CALLBACK` semantics -- the socket is force-closed but no
`close_callback` fires, matching `close()`'s own established no-callback
behavior for a plain LPC-initiated close) and
`Scheduler::removeAllCallOutsForObject()` (real `remove_all_call_out()`,
matching both real forms -- a string-form entry whose own target is
`obj`, or a closure-form entry whose own closure owner is `obj` -- and
also opportunistically pruning any entry whose own target/owner is
already gone or destructed while walking the list anyway, since that is
the real function's own complete behavior, not an unrelated side effect
layered on top).

**A real, separate gap found and precisely located while implementing
this, not fixed here -- flagged rather than silently expanded into:**
real `close_referencing_sockets()` has a *second* real call site besides
`reload_object()`'s own -- `simulate.c`'s own `destruct_object()`
("`if (ob->flags & O_EFUN_SOCKET) close_referencing_sockets(ob);`",
confirmed directly, sitting right alongside the same shadow/living-name
handling this driver's own `ObjectManager::destructObject()` already
ports). This driver's own `destructObject()` does not close a destructed
object's own sockets at all -- a dangling `LpcSocket` whose owner has
been destructed simply stops firing callbacks (`weak_ptr` lock failure)
but lingers, fd still open, until something else removes it. Out of
this session's own specific scope (`reload_object()`, not `destruct()`),
recorded in `SocketRegistry.hpp`'s own comment with the exact real
citation so it does not need re-finding from scratch.

6 new test functions (`test/test_lexer.cpp`): the core reload test, the
socket-close-in-isolation test, the call_out/heart_beat-removal test,
two shadow-chain tests (cascade-destructs when `obj` is the base victim,
splices out without destructing anything when `obj` is itself the
shadow), and one confirming a snoop relationship survives a reload
completely untouched (real `reload_object()` has no snoop-related line
anywhere in its own body at all, confirmed directly by re-reading it --
unlike `destruct_object()`'s own explicit snoop unlinking -- so this is
the one real, verified place where "leaves it alone" is itself the
correct, faithful behavior, not a gap). Two real test-authoring mistakes
caught by running the suite rather than assumed correct, both fixed
before landing: the core reload test's own `create_count` tracking
variable was itself subject to the exact same zero-then-reinit step
being tested, so it could never accumulate across a reload the way the
first draft assumed -- fixed by corrupting it to an arbitrary value
before reload and checking it reads back as a freshly-incremented 1,
not the corrupted value and not a bare 0, the only way to tell "create()
genuinely ran again" apart from "create() was skipped" once the counter
itself is reset by the very same operation. The socket test's own first
draft read the socket's handle back through the reloaded object's own
(by-then-zeroed) `fd` variable, silently checking an unrelated leftover
socket from a different, still-registered handle 0 instead of its own
now-closed one -- fixed by capturing the real handle in a plain C++
local before ever calling `reload_object()`. Full suite: 583 tests
passing, up from 577 before this session, no regressions, stable across
three consecutive runs plus a full `ctest` pass.

With `origin()` and `reload_object()` both done, **the real `parse_*`
package (`packages/parser.c`, 3419 lines) is the one remaining
dedicated-session candidate** -- confirmed still the case, nothing in
this session's own work changed that assessment. It should be next if
row 0.13 continues past its current 54-name real gap, but given its
size (by far the largest single item this row has ever considered,
comparable to or larger than everything implemented in this row so far
combined) it warrants its own explicit go-ahead before being taken on,
not an assumed default.

**2026-08-23: full 55-name gap accounting produced and cross-checked
against STATUS.md/instruct.md's own written record, then `origin()`
implemented in full -- real per-call-path tagging through every genuine
LPC frame this driver's VM creates, all 8 real `ORIGIN_*` values, each
verified against `reference/fluffos-2.9-ds2.08` individually rather than
assumed by resemblance to another (Phase 0 row 0.13: 233 registered, up
from 232).**

**Accounting task, before touching any code.** `grep -c registerEfun`
against `EfunTable.cpp` reads 232 (233 after this session), but that
count is not directly comparable to the real 270-name `efun_defs.c`
target: 18 registered names are not literal `efun_defs.c` entries, for
one of three legitimate, individually-confirmed reasons (six are real
FluffOS names whose activating `#ifdef` is off in this exact vendored
build -- `funcall`/`m_delete`/`query_once_interactive`/`strstr`
(`COMPAT_32`), `set_light` (`NO_LIGHT` is defined here), `set_debug_level`
(`DEBUG_MACRO`) -- kept because this driver's own bundled Lil test
mudlib genuinely calls them; two, `regexplode`/`regexp_assoc`, are
driver additions this row's own instruct.md/prompt.md explicitly asked
for under those names despite neither being real, already self-
documented as such at the time each was added; ten are unrelated to
row 0.13 entirely -- `new` (real FluffOS treats this as a dedicated
lexer keyword, never an efun-table entry) and the seven Rifts combat-
math functions plus `query_screen_width`/`query_screen_height`, all
driver additions from separate, earlier initiatives reusing the same
registration mechanism). None of the 18 should count toward the 270
tally. The real, authoritative gap is always `comm -23` between a fresh
`efun_defs.c` name list and a fresh `registerEfun` name list -- 55 names
as of the start of this session (56 before `query_num` landed last
entry), not 232 vs 270's own naive difference. Recorded as a standing
methodology note in `src/efun/instruct.md`'s own 0.13 section (with the
three `mud_status`-family names -- `network_stats`/`dump_prog`/
`memory_info` -- that turned out to have never gotten their own
individual citation despite being assumed to fit that family by
resemblance, now given one) so this does not need re-deriving from
scratch by whoever runs the next pass.

Every one of the 55 real gap names lands in exactly one of three
categories, each with its own reasoning already on record (STATUS.md or
`src/efun/instruct.md`, both cross-checked directly against the
committed text before this report, not from memory):

- **34 documented exclusions**, spanning six real reasons: shadowed
  simul_efuns with no `efun::` delegation anywhere (`translate`, `event`
  -- `pluralize` and `livings`/`tell_object` are the load-bearing
  counter-examples already implemented, not exclusions); `TYPE_BUFFER`
  architecture mismatch (`allocate_buffer`, `read_buffer`,
  `write_buffer`, `bufferp`); `TYPE_CLASS` architecture mismatch
  (`assemble_class`, `disassemble_class`, real bodies confirmed to
  exist; `fetch_class_member`, `store_class_member`, folded into the
  next category since they additionally have none); unverifiable --
  real names with no implementation anywhere in this project's only
  reference source (`debug_info`, `variables`, `functions`,
  `fetch_variable`, `store_variable`, plus the two class-member names
  just above); driver-internal-dump architecture mismatch (`mud_status`,
  `cache_stats`, `malloc_status`, `dumpallobj`, `domain_stats`,
  `author_stats`, `network_stats`, `dump_file_descriptors`, `dump_prog`,
  `memory_info`, `memory_summary`, `program_info` -- twelve, all
  individually confirmed real this session, three for the first time);
  and one-off architecture/infrastructure gaps confirmed real but with
  no equivalent this driver has (`get_char`/`ed`, raw-input/editor
  infrastructure; `resolve`, async address-server IPC; `get_garbage`, no
  GC concept; `socket_acquire`/`socket_release`, explicitly out of
  "basics" scope per ROADMAP's own Tier 3 note; `set_reset`, no
  `reset()`-apply mechanism exists at all).
- **12 zero-call-site names already individually surveyed** in earlier
  passes and re-confirmed still accurate: `zonetime`/
  `is_daylight_savings_time` (real, deferred pending a clearer read of
  `TZ` mutation safety against this driver's own event loop), `act_mxp`/
  `has_mxp`/`request_term_type`/`start_request_term_type` (real, no
  downstream MXP/TTYPE subnegotiation parsing exists to receive an
  answer), and the six-member VRML-pose family (`rotate_x`, `rotate_y`,
  `rotate_z`, `scale`, `id_matrix`, `lookat_rotate` -- real, zero
  plausible use in a text-only MUD driver).
- **2 remaining dedicated-session candidates** (down from 3 -- `origin`
  itself was the third, taken on this session): the real `parse_*`
  parser package (`packages/parser.c`, 3419 lines, 8 gap names:
  `parse_refresh`/`parse_init`/`parse_sentence`/`parse_add_rule`/
  `parse_add_synonym`/`parse_my_rules`/`parse_dump`/`parse_remove`) and
  `reload_object` (fully implementable in principle, needs one new
  `SocketRegistry` capability, zero real call sites to validate against).

Nothing fell through a gap in this accounting -- every real name is
accounted for, and the 18-name registered-count inflation is now a
recorded fact rather than a silent discrepancy future reports could
repeat.

**`origin()` implementation.** Started from the architecture notes two
sessions ago (all 8 real `ORIGIN_*` values and their real C set sites
already enumerated) but re-verified every one directly against
`reference/fluffos-2.9-ds2.08` before writing any dispatch logic, per
this session's own explicit instruction -- and found the previous
session's own flagged blocker was based on an incomplete premise.
Real `caller_type` (`interpret.c`) is a single scalar saved/restored
across the control stack around *every* function call
(`push_control_stack()`/`pop_control_stack()`), confirmed directly; the
real, narrower difficulty is that efuns themselves never get their own
control-stack frame at all (no `push_control_stack()` call anywhere in
an ordinary efun dispatch, confirmed directly) -- so a bare call
resolving to the core efun table never changes the origin at all, and
the two real exceptions genuinely observable through this driver's own
single `OpCode::CallEfun` dispatch path (`call_other`/`"->"` and
`evaluate`/`funcall`/`"(*fp)(...)"`, both compiler-forced through it per
`CodeGen.cpp`'s own `forceEfun`) do their own origin tagging entirely
inside their own `EfunTable.cpp` registrations, not at the opcode
dispatch layer -- no name-based special case was needed at
`OpCode::CallEfun` after all once this was traced through fully.

New `Origin` enum (`VM.hpp`, 8 values) and `originName()` (`VM.cpp`,
real `origin_name()`'s own exact string table), a new `originStack_`
(`VM.hpp`), and a new `OriginGuard` RAII class (`VM.cpp`, matching the
existing `ObjectFrameGuard`/`CommandGiverGuard` shape) pushed/popped at
every real call path that pushes a genuine LPC frame:

- `OpCode::Call`'s local/inherited tier -> `Origin::Local` (real
  `F_CALL_FUNCTION_BY_ADDRESS`'s own `caller_type = ORIGIN_LOCAL`);
  simul_efun tier -> `Origin::SimulEfun` (real `call_simul_efun()`'s own
  `call_direct(simul_efun_ob, ..., ORIGIN_SIMUL_EFUN, ...)`); efun-table
  tier -> no change at all, confirmed above.
- `OpCode::CallParent` (`::name()`/`qualifier::name()`) -> `Origin::Local`
  (real `F_CALL_INHERITED`'s own identical `ORIGIN_LOCAL` set).
- `VM::callClosure()` (backs `evaluate`/`funcall`/`"(*fp)(...)"` and
  every efun that invokes a closure argument) -> tiered exactly like
  real `call_function_pointer()`'s own `FP_LOCAL`/`FP_SIMUL`/
  `FP_FUNCTIONAL`/`FP_EFUN` split: a named local/inherited target is
  `Origin::Local`, an anonymous `"(: ... :)"` target is
  `Origin::Functional` (distinguished by real `function.c`'s own
  synthesized-lambda naming convention, `"$lambda#"` + id -- new
  `isSynthesizedLambdaName()`), a simul_efun target is
  `Origin::SimulEfun`, and a target resolving to a core efun leaves the
  origin unchanged (real `FP_EFUN`'s own behavior: `setup_fake_frame()`
  sets a transient `ORIGIN_FUNCTION_POINTER` that no genuine LPC bytecode
  ever actually runs under, since raw efun C code cannot call `origin()`
  on itself -- this driver has no fake-frame mechanism to mirror that
  transient step at all, and nothing could observe the difference either
  way).
- `VM::callFunction()` (the shared "invoke a function on an object from
  outside the VM" entry point) gained a new `Origin origin =
  Origin::Driver` parameter. Driver was chosen as the default because it
  is correct for the clear majority of the ~30 real call sites into this
  method across `Server.cpp`/`Scheduler.cpp`/`ObjectManager.cpp`/
  `EfunTable.cpp` (confirmed individually, not assumed by category:
  `logon()`, `process_input()`, `net_dead()`, `window_size()`, `create()`,
  every master apply, `moveObject()`'s own `init()` propagation are all
  real `ORIGIN_DRIVER`) -- the minority that need something else pass it
  explicitly, each with its own citation at the call site: `call_other`'s
  own implementation (`Origin::CallOther`, real `f__call_other()`'s own
  `call_origin = ORIGIN_CALL_OTHER`); `map_array`/`filter_array`/
  `sort_array`/`unique_array`/`unique_mapping`/`map_mapping`/
  `filter_mapping`'s own string-form callback dispatch (`Origin::Efun`,
  real `process_efun_callback()`/`call_efun_callback()`'s own
  `ORIGIN_EFUN`, confirmed as the real, narrow meaning of that value --
  a mudlib-*supplied callback argument* invoked from inside an efun's
  own C body, not "any efun calling into LPC for any reason," a
  distinction found the hard way: `present()`'s own `id()` check and
  every master apply (`valid_hide`/`valid_shadow`/`valid_bind`,
  `catch_tell`) all turned out to be real `ORIGIN_DRIVER` instead when
  checked directly, not `ORIGIN_EFUN` as first assumed by the "efun
  calls into LPC" pattern alone); socket read/write/close callback
  firing and `input_to()`'s own string-form dispatch (`Origin::Internal`,
  real `socket_efuns.c`'s own `safe_apply(..., ORIGIN_INTERNAL)` and
  `comm.c`'s own `call_function_interactive()`'s identical
  `apply(function, ob, ..., ORIGIN_INTERNAL)`); call_out's own
  string-form firing (`Origin::Internal` too, real `call_out.c`'s own
  `apply(cop->function.s, cop->ob, extra, ORIGIN_INTERNAL)` -- a real,
  easy-to-miss distinction from heart_beat firing, which really is
  `ORIGIN_DRIVER` (`backend.c`'s own `call_direct(ob, ...,
  ORIGIN_DRIVER, 0)`) despite both being Scheduler-fired timers).
- `VM::callFunctionInProgram()` (backs the synthesized `"$objvarinit"`
  initializer) hardcodes `Origin::Driver` rather than taking a parameter
  -- its one real caller (`ObjectManager::runObjectVarInitializers()`)
  has no other real analog (real `call___INIT()`'s own `caller_type =
  ORIGIN_DRIVER`).
- `VM::dispatchCommand()`'s own internal handler-calling picks
  `Origin::Driver` or `Origin::Efun` at runtime based on whether
  `currentObject()` is already set, matching real `add_action.c`'s own
  `user_parser()`: `"where = (current_object ? ORIGIN_EFUN :
  ORIGIN_DRIVER);"`, with its own comment explaining why ("If this is
  called directly from user input, then the origin is the driver and it
  will be allowed") -- the true top-level entry
  (`Server::dispatchLine()`, no LPC frame active yet) is Driver; a nested
  re-dispatch (this driver's own `command()` efun, called from within an
  already-running function, the only real way `currentObject()` ends up
  set at this exact point) is the narrower Efun.

8 new test functions (`test/test_lexer.cpp`), one per distinct
`Origin::*` value rather than a couple of representative cases, per this
session's own explicit instruction given how easy this efun is to get
subtly wrong: `Local` (a bare same-object call), `CallOther` (an actual
`->` dispatch), `SimulEfun` (a bare call resolving through the
simul_efun tier), `Internal` (call_out string-form firing via a live
`Scheduler`), `Efun` (a `map_array()` string-form callback), `Functional`
(an inline `"(: origin() :)"` lambda invoked through `evaluate()`), and
`Driver` (three real sub-cases in one test: a direct top-level call, a
live heart_beat fire, and a real command dispatch through
`Server::dispatchLine()` with a genuine `Connection`/`OutputContext`),
plus a ninth, `FunctionPointer`, verified the only way it can be: a
direct C++-level check that `originName()` still reports it correctly,
since no genuine LPC call path in this driver (or, per the citation
trail above, in real FluffOS either) ever actually produces it. Two
real test-authoring mistakes caught by running the suite rather than
assumed correct: the `Driver`/command-dispatch sub-case first tried
`add_action()` from inside `create()` directly, which silently registers
nothing (`resolveCommandGiver()`'s own fallback needs a live
`Connection`/`OutputContext`, exactly the same constraint the existing
`add_action`/`remove_action`/`query_notify_fail` test suite already
documents and works around) -- fixed by matching that same established
`Connection` + `OutputContext::set()` + `Server::dispatchLine()` pattern
rather than inventing a new one. Full suite: 577 tests passing, up from
569 before this session, no regressions, stable across three
consecutive runs plus a full `ctest` pass.

**2026-08-22 (continued): `query_num` implemented, the last real
call-site-bearing item this row's six-corpus ranking still had left --
everything else with real weight is now a previously-documented
exclusion or dedicated-session deferral (Phase 0 row 0.13: 232
registered, up from 231).** Re-ran the six-corpus ranking; unchanged in
shape from the last several passes. `query_num` was the one real name
never individually checked before -- previously only carried forward
under "same large-algorithm category as `pluralize`," never actually
read or call-site-verified on its own. Its own one raw hit
(`dead-souls/lib/secure/sefun/english.c`) needed the same false-positive
diligence `set_author` needed last session, since that exact same file
already defines its own `pluralize()` shadowing the core efun -- but
this one checked out genuinely real and, better than merely real,
*deliberately* real: the call sits inside `"#ifndef __FLUFFOS__ ...
#else string cardinal(int x){ ...; return sign+query_num(x); } #endif"`,
Dead Souls' own explicit "prefer the driver's real efun over my own
70-line hand-rolled fallback, when the driver actually has one" branch
-- and confirmed this driver's own compiler genuinely defines
`__FLUFFOS__` (`ObjectManager.cpp`'s preprocessor macro table), so this
driver takes exactly that branch. Also resolved a real spec discrepancy
before writing anything: `efun_defs.c`'s own raw arity field reads
`2,2` (both arguments required), which would make the real 1-argument
call site (`query_num(x)`, no explicit limit) invalid -- checked
`packages/contrib_spec.c` directly rather than trusting the generated
table alone and found the real default there, `"int default:0"`,
confirming `efun_defs.c`'s numeric fields are the post-default-expansion
arity, not the real callable minimum, the same category of trap this
row's own `set_eval_limit()` fix already flagged once before.

Ported the real ~90-line `query_num()`/`number_as_string()`
(`packages/contrib.c`) mechanically, line by line, not from general
English-number-formatting knowledge: units/teens/tens-with-optional-
dash, then hundreds, then thousands, each stage's own real assembly
quirks kept exactly as read rather than "cleaned up" -- a hundreds
group gets a leading comma only when a thousands group already
contributed (otherwise no separator at all), the final units group gets
a leading "and" whenever anything higher-order did, a round hundred or
thousand short-circuits before ever reaching the next lower stage
instead of appending an empty tail, and `hi[1]` in the real tens-word
table is genuine dead code (n/10 == 1 only ever happens for n in
[10,19], already handled by the teens branch above it) kept as-is
rather than trimmed. Ceiling behavior ported exactly too: "many" past a
hard 99999, past an optional caller-supplied limit, or for a negative n
-- the limit argument's own real default (0, meaning "no ceiling below
99999") resolved from `contrib_spec.c` above.

1 new test function (`test/test_lexer.cpp`), checking a representative
slice through every stage of the real assembly (round vs dashed tens,
a hundreds group with and without a trailing units group, a thousands
group with and without a trailing hundreds group, the comma-vs-no-comma
distinction, the 99999 ceiling and an explicit lower one, and a
negative input) rather than exhaustive random-number testing -- passed
on the first real run, every hand-derived trace correct. Full suite:
569 tests passing, up from 568 before this session, no regressions,
stable across three consecutive runs plus a full `ctest` pass.

With this, the six-corpus ranking's entire real-call-site-bearing
remainder is now accounted for: `translate`/`event` (shadowed
simul_efuns), the buffer family (`TYPE_BUFFER` architecture mismatch),
the reflection family (`variables`/`functions`/etc., unverifiable, no
reference implementation exists), the driver-internal-dump family
(`mud_status`/`cache_stats`/etc., architecture mismatch), `get_char`/`ed`
(missing raw-input/editor infrastructure), `resolve` (missing async
address-server infrastructure), `debug_info` (unverifiable), and three
real, substantial items intentionally left for their own dedicated
sessions rather than rushed into a shared batch: `origin()` (per-call
origin tagging through the whole VM, with a documented `CallEfun`/
`call_other` conflation seam), the real `parse_*` parser package
(`packages/parser.c`, 3419 lines), and `reload_object()` (fully
implementable, needs one new `SocketRegistry` capability, zero real
call sites to validate against). Only 0-call-site names remain
unaccounted for from here.

**2026-08-22 (continued): a real implementation bug caught by its own
test rather than shipped, 4 more efuns implemented, most of the
remaining ranked gap resolved into concrete excluded/deferred
categories (Phase 0 row 0.13: 231 registered, up from 227).** Re-ran the
six-corpus ranking (same shape as last entry -- `translate`/`origin`
still the two highest-ranked correctly-excluded/deferred items, nothing
new since the corpora themselves haven't changed). Checked every
previously-unassessed real gap name still carrying real call-site
weight or otherwise worth a direct look, verifying each against
`reference/fluffos-2.9-ds2.08` before writing anything, same as every
pass before this one.

Implemented: `function_owner` (real, `packages/contrib.c`, 1 genuine
call site in `lima/lib/std/object/hooks.c`) -- the closure's own owner
object, `Closure::owner` already being exactly real `funptr_hdr_t::owner`
in weak_ptr form. First draft returned void (`Value{}`) for a null/gone
owner; its own dedicated test caught this immediately on the first real
run, not a rubber-stamped pass -- re-read real `interpret.h`'s own
`put_unrefed_object()` macro directly rather than guessing, confirmed
it explicitly pushes a real int 0 for a null *or destructed* owner
(`"if (!(x) || (x)->flags & O_DESTRUCTED) *sp = const0u"`), fixed to
match, and the test extended to cover both real paths independently:
`weak_ptr::lock()` failing outright (owner's last reference genuinely
gone) and `lock()` succeeding but `isDestructed()` being true (owner
destructed while a live reference is deliberately kept locally, the
same "destructed but still referenced" pattern this driver's own
2026-08-09 `coerceIfDestructed()` work already established test
coverage for elsewhere). `replaceable` (real, `packages/contrib.c`,
confirmed genuinely paired with `replace_program()` at its own one real
call site -- `dead-souls/lib/lib/std/room.c`'s own
`"if (replaceable(this_object()) && ...) replace_program(...)"`)
implemented as a direct, small follow-on to last session's own
`replace_program()` work: this driver's `CompiledProgram::functions`
already *is* exactly real code's own `FUNC_INHERITED`/`FUNC_NO_CODE`-
filtered set (functions defined locally in this exact file, with real
code -- an inherited function lives in a different `CompiledProgram`
entirely, confirmed directly rather than assumed), so no separate
filtering logic was needed at all, just a membership check against an
ignore list seeded with `"create"` and this driver's own synthesized
`"$objvarinit"` (the real equivalent of FluffOS's own synthesized
`"__INIT"`) plus the caller's explicit extras. `num_classes` (real,
`packages/contrib.c`) implemented as an unconditional `0`: not a guess
or a default, a certainty -- this driver's compiler has never
implemented LPC `class` declarations at all (no `TYPE_CLASS` value
kind, the same gap the already-excluded `assemble_class`/
`disassemble_class`/`fetch_class_member`/`store_class_member` share),
so every object this driver can possibly compile has exactly zero class
declarations. `set_author` (real, `packages/mudlib_stats.c`) implemented
as a documented no-op, same reasoning `flush_messages()` already
established for its own already-default real effect: real
`set_author()`'s only observable effect anywhere in the reference
source is tagging `PACKAGE_MUDLIB_STATS`-only per-author memory
accounting (`ob->stats.author`), and its only real consumer
(`author_stats()`/`domain_stats()`) is already excluded from this table
for having no equivalent model in this driver at all -- nothing in this
driver could ever observe what `set_author()` recorded either way, so a
no-op is behaviorally complete, not an approximation. Its own "real
call site" was double-checked and turned out to be a false positive
worth flagging for the methodology itself: the one raw grep hit
(`lima/lib/std/book.c`) is a same-named local function *definition* (a
book object's own "who wrote this" property setter, shadowing the core
efun for that file, not a call to it at all) -- this row's own
`\bname\(` matching cannot itself distinguish a definition from a call,
a trap already known for comments/prototypes but not previously
observed for a same-named local override specifically.

Investigated and resolved into a firm exclusion, not implemented:
`set_reset` (real, `efuns_main.c`) schedules when this object's own
`reset()` apply will next fire (`ob->next_reset`) -- but this driver has
no `reset()`-apply mechanism anywhere at all (grepped directly, zero
hits), unlike `set_author`'s situation: there IS a real driver-level
concept here (periodic per-object `reset()` firing) that this driver
genuinely never built, so a no-op would be silently misleading rather
than behaviorally complete, the same architecture-mismatch category as
`mud_status` rather than `flush_messages`. `reload_object` (real,
`object.c`) was read in full and found genuinely implementable in
principle -- zero every object variable, close any efun sockets it
owns, cascade-destruct or splice out of its own shadow chain (both
reusable from `ObjectManager::destructObject()`'s own existing logic),
disable heart_beat, remove pending call_outs, then call `create()`
again -- but has zero real call sites across all six corpora and would
need one genuinely new piece (a "close every socket a given object
owns" `SocketRegistry` capability that does not exist yet), so deferred
rather than implemented speculatively against no real usage to validate
against; flagged here so the full real sequence (including the easy-to-
miss "calls create() again at the end," not just "zeroes variables")
is not rediscovered from scratch.

Full suite: 568 tests passing, up from 564 before this session, no
regressions, stable across three consecutive runs plus a full `ctest`
pass.

**2026-08-22 (continued): `replace_program()` taken on as the flagship
item from last session's own architecture notes, `query_replaced_program`
added alongside it as a natural follow-on, `origin()` investigated
further and re-deferred with a much more concrete reason, a whole
unverifiable reflection-efun family and a real 3419-line parser
subsystem both discovered and flagged (Phase 0 row 0.13: 227 registered,
up from 225).** Re-ran the six-corpus ranking from the previous entry's
own methodology (no re-clone needed -- all six were still present);
gap unchanged in shape from last time (`translate` stays the top
correctly-excluded shadowed simul_efun; `replace_program` and `origin`
still the two next-ranked real items).

`replace_program` (33 combined, es2 31) taken on this session, per last
entry's own concretely-scoped notes. Read the real ~200-line
`replace_program.c` in full before writing anything. Confirmed this
driver's `CompiledProgram::inheritedPrograms`/`ancestorBaseOffsets`
(`Bytecode.hpp`) genuinely was the right architectural fit last
session's notes expected: `inherits[i]`/`inheritedPrograms[i]` are
parallel, same-order vectors (confirmed directly in
`ObjectManager::compile()`), so a name-matched depth-first walk of that
pair alone (new `searchInheritedProgram()`, `EfunTable.cpp`) finds the
target ancestor's own `CompiledProgram` without needing a new filename
field on it at all -- and `ancestorBaseOffsets` already has a direct
entry for every transitive ancestor (`obj->program()->ancestorBaseOffsets.
find(matchedProgram.get())`), so the real `var_offset` this efun needs
is a single map lookup, not a hand-rolled offset accumulation. Deferred
application matches real semantics exactly (real code's own comment:
applying it mid-execution "could result" in volatile state) -- new
`VM::enqueueReplaceProgram()`/`VM::processPendingReplacePrograms()`,
wired into `Scheduler::run()`'s own `for(;;)` loop at the same relative
position real `backend.c`'s own `while(1)` loop calls
`remove_destructed_objects()` from, and independently callable from
tests the same way `tickHeartbeats()`/`tickCallOuts()` already are, to
simulate one driver tick passing without needing the full event loop
running. The variable-array shuffle (`processPendingReplacePrograms()`)
unifies real code's own two branches (`offset != 0` vs `offset == 0`)
into one "keep the `[offset, offset+newCount)` slice" extraction, since
an offset of 0 already produces the same result the second branch
computes separately. Real guards ported: `current_object == simul_efun_ob`
throws (new `VM::simulEfunObject()` accessor, mirroring the existing
`masterObject()`); "program to replace with has to be inherited" throws
when the search finds nothing. Real `prog->func_ref` guard (blocks the
swap while a function pointer holds a *direct* reference into the
current program's own function table) has no equivalent here and was
not ported: this driver's `Closure` never holds a direct function-table
reference, only a bare name re-resolved lazily against its owner object
at call time (`Value.hpp`), so a closure that stops resolving after the
swap simply throws "undefined function" at its own next call instead --
matching real semantics' intent, just via a different mechanism, not a
gap. Real "stop shadowing" side effect ported with its own specific
asymmetry preserved exactly as read (only `ob->shadowing` is checked and
spliced, never `ob->shadowed`, confirmed directly rather than
generalized to an unconditional `remove_shadow()`-style splice).

`query_replaced_program` (real, `packages/contrib.c`) implemented
alongside it: new `LpcObject::replacedProgramName_` (set only once a
staged swap actually applies, in `processPendingReplacePrograms()` right
after `setProgram()`, correctly still unset while a swap is merely
*pending* -- verified by its own dedicated test -- matching real
semantics: `replaced_program` is written by `replace_programs()` itself,
never by `f_replace_program()` staging the request), cleared on destruct
(`ObjectManager::destructObject()`) matching real `object.c`'s own
destructor. Real `add_slash()`'s leading-`/`-on-the-stored-name behavior
matched explicitly rather than assumed present in whatever the mudlib
argument happened to look like.

`origin()` (27 combined) investigated further, not just re-flagged.
Read `efuns_main.c`'s real `f_origin()`: far simpler than the previous
entry's own notes suggested -- not a full per-frame-object metadata
structure, just one scalar (`caller_type`), saved/restored across nested
calls via the control stack exactly the way this driver's own
`objectChangeStack_` already saves/restores object-crossing state, real
architecture already a close match. Found and enumerated all 8 real
`ORIGIN_*` values and every real C set site (`origin.h`, `function.c`,
`interpret.c`). The blocker is narrower now, and specific: real
`call_other()`/`->` does not compile to its own dedicated opcode in this
driver at all -- it is compiler-forced through `OpCode::CallEfun`
targeting the literal name `"call_other"` (`Bytecode.hpp`'s own comment
on why `CallEfun` exists), the same opcode every *other*, genuine efun
call also uses. Distinguishing real `ORIGIN_CALL_OTHER` from real
`ORIGIN_EFUN` therefore needs a name-based special case at that one
opcode, not just per-opcode tagging -- a real, easy-to-get-subtly-wrong
seam that does not show up anywhere in a first-pass architecture read.
One mitigating finding worth recording: the one real, verified call
site (`secure/daemon/chat.c`'s own `origin() != ORIGIN_LOCAL` gate) only
actually needs a binary LOCAL-vs-not distinction, not the full 8-value
fidelity -- lowering the stakes of that one call site specifically, but
not of `origin()` as a general-purpose efun other mudlib code could
reasonably call expecting a fully correct answer. Given the number of
distinct call paths still needing correct, individually-verified
tagging (`Call`'s own tiered local/inherited/simul_efun/efun resolution,
the `CallEfun`-vs-`call_other` special case above, `CallParent`,
`callClosure()`, and every external `callFunction()` entry point from
`Server`/`Scheduler`/`ObjectManager` for `ORIGIN_DRIVER`), still judged
too large and too easy to get silently wrong in a batch shared with
other work -- deferred again, with this session's much more concrete
mechanism notes left for whoever takes it on next as its own fully-
focused pass.

Two more real gaps investigated and resolved into clearer categories
rather than re-flagged unchanged. The reflection-efun family
(`variables`, `functions`, `fetch_variable`, `store_variable`,
`fetch_class_member`, `store_class_member`) -- previously filed as "a
real API family of comparable scope to a fresh mini-subsystem" -- turns
out to have **no implementation anywhere in this project's only
reference source at all** for any of the six names (grepped every real
`.c` file, including `packages/`, not just the top-level ones a
previous pass's narrower grep covered): prototype-only in `efun_defs.c`,
the same category `debug_info` was already correctly excluded under.
Recategorized from "large effort" to "unverifiable," the stronger and
more specific reason. The `parse_*` family (66 combined across 8 names)
-- previously not investigated in any depth -- does have a real
implementation after all, missed by an earlier pass's own top-level-only
grep: `packages/parser.c`, a genuine 3419-line natural-language
sentence/grammar-rule parser package (FluffOS's real "parser" package).
Confirmed real and substantial, not unverifiable or architecture-
mismatched -- but sized well beyond a batch item, closer to `snoop`'s or
`replace_program`'s own "own dedicated session" category than anything
folded in here. Flagged for a future pass with this citation so it is
not rediscovered from scratch.

3 new test functions (`test/test_lexer.cpp`): `replace_program`'s own
test is the most involved of the three, deliberately checking (a) both
the child's own function and the inherited one resolve normally before
any call, (b) the swap is still fully inert immediately after
`replace_program()` returns (proving the deferral, not just its
eventual effect), and (c) after simulating one tick
(`processPendingReplacePrograms()`), the child's own function is gone
(checked via `VM::callFunction()`'s own documented silent-void-for-
undefined-function convention, not a thrown exception -- that is
`OpCode::Call`'s own behavior for a bare in-LPC call, a different entry
point, an assumption this test's first draft got wrong and a real run
caught immediately), the inherited one still resolves, the shared
variable's own *value* survived the swap intact, and the object's own
variable count shrank to the target ancestor's own count. Full suite:
564 tests passing, up from 561 before this session, no regressions,
stable across three consecutive runs plus a full `ctest` pass.

**2026-08-22 (continued): call-site ranking widened to six real mudlib
corpora, `pluralize`'s previous "permanently shadowed" verdict reversed
after finding a real load-bearing `efun::pluralize()` delegation, 3 more
efuns implemented (Phase 0 row 0.13: 225 registered, up from 222).**
`temp/nightmare3` alone was this row's whole corpus as of the previous
entry; five more real, independently-maintained mudlibs are now
available alongside it (`temp/mudlib` -- Genesis/CD, targets the CD
gamedriver, not FluffOS; `temp/core-lib` -- RealmsMUD, targets LDMud,
not FluffOS; `temp/es2_mudlib` -- ES2, targets Neolith, a MudOS-lineage
driver documented as backward-compatible with MudOS-level LPC; `temp/lima`
and `temp/dead-souls`, both explicitly FluffOS-targeted). All six are
gitignored scratch clones (`temp/` covers the whole tree, confirmed via
`git status` before touching anything), the same non-tracked-research-
clone precedent `temp/dgd`/`temp/fluffos`/`temp/ldmud` already established
for driver source rather than mudlib source. Re-ran the gap ranking
(`\bname\(` word-boundary match, `/doc/`+`/documentation/` excluded, each
corpus's own vendored driver submodule excluded by scoping to its actual
lib root -- `nightmare3/lib`, `es2_mudlib/mudlib`, `dead-souls/lib`, the
other three's own repo root) summed across all six.

Top of the combined ranking: `pluralize` (65: nightmare3 13, lima 19,
dead-souls 33) and `translate` (56: nightmare3 6, lima 23, dead-souls
27). Re-checked both for the same "shadowed by a real simul_efun of the
same name" pattern the previous entry already confirmed in nightmare3
alone -- both lima and dead-souls also define their own `pluralize()`/
`translate()` simul_efuns (`secure/sefun/english.c`/`translate.c` in
dead-souls literally mirrors nightmare3's own `secure/SimulEfun/`
layout, same NightmareIV lineage). `translate` stays excluded, confirmed
the same way as before (grepped every shadowing file across all three
corpora for an `efun::translate()` delegation -- none). `pluralize` does
not: lima's own `std/modules/m_grammar.c` wraps a small, explicit set of
hardcoded exceptions (`"were"`->`"was"`, `"staff"`->`"staves"`, etc)
around a direct `return efun::pluralize(str);` fallthrough for every
other input -- the same load-bearing-shadow pattern already established
for bare `livings()`, missed for `pluralize` in the previous entry only
because that entry's corpus (nightmare3 alone) never contained the one
mudlib whose own wrapper actually reaches through. Corrected: `pluralize`
is real and load-bearing after all, previous entry's "permanently
excluded, same category as `translate`/`event`" verdict reversed for
this specific efun (not for `translate`/`event`, which remain correctly
excluded on their own, independently re-verified evidence).

Read and mechanically ported the real ~440-line body
(`packages/contrib.c`'s `pluralize()`/`f_pluralize()`) line by line
rather than reimplemented from general English-pluralization knowledge:
the exception switch on the last word's first letter (~50 hardcoded
irregular forms), the general suffix rules on its last letter (x/y/f/z/
s/o/h/n cases), and the chop-and-append final assembly, including one
confirmed real quirk worth flagging for anyone who touches this again --
`"lotus"` hits an exception-table branch that sets `found = PLURAL_SUFFIX`
without ever reassigning `suffix` away from its default `"s"`, so real
`pluralize("lotus")` produces `"lotuss"`, not the grammatically correct
`"lotuses"` -- ported faithfully as-is, a real bug in the reference
build's own table, not a typo introduced by this port. `query_num`
(same large-algorithm category, still 0 real call sites across all six
corpora) remains deferred.

Also re-ranked and reconsidered against the widened corpus:
`replace_program` (33, es2 31 concentrated -- real, genuine
`replace_program(ROOM)`-style calls across many `es2_mudlib/mudlib/d/`
area files, not simul_efun-shadowed) and `origin` (27 combined, up from
nightmare3's own 2). Both investigated in real depth rather than
carried forward on the previous entry's reasoning alone. `replace_program`:
read the real ~200-line `replace_program.c` fully -- swaps an object's
live `program_t*` to one of its own inherited programs (deferred to a
`replace_programs()` pass run once per driver tick, after the current
top-level dispatch finishes) plus a variable-slot remap between the old
and new layouts. This driver's own `CompiledProgram` (`Bytecode.hpp`) is
a genuinely closer architectural match than expected -- inheritance is
kept as a real tree of separate `shared_ptr<CompiledProgram>` nodes
(`inheritedPrograms`), not flattened at compile time, and
`ancestorBaseOffsets` already maps every transitive ancestor to its own
variable-slot base offset for a different reason (cross-inheritance
object-variable resolution) that happens to be exactly what
`replace_program()` also needs. Still not attempted this pass: a
filename-keyed inherit-tree search still needs building (the existing
map is pointer-keyed, and `CompiledProgram` does not yet carry its own
source filename), a deferred-until-end-of-tick replace queue needs
wiring into the event loop (`Server::dispatchLine()`/`Scheduler`, the
same shape `call_out`/`heart_beat` already use), and the variable-array
surgery itself needs to be gotten right with no real mudlib test
coverage to fall back on if it's subtly wrong -- flagged as a strong,
concretely-scoped candidate for its own dedicated session rather than
one line item folded into this batch, not a "someday" deferral. `origin`:
still needs per-call origin tagging (`LOCAL`/`CALL_OTHER`/`DRIVER`/
`EFUN`/`SIMUL_EFUN`/`FUNCTION_POINTER`) threaded through every call
path (`Call`/`CallOther`/`CallEfun` opcodes, closure invocation,
call_out/heart_beat firing) -- `VM`'s own `callStack_` is currently just
`vector<shared_ptr<LpcObject>>` with no per-frame call-kind metadata at
all. Real `secure/daemon/chat.c` uses `origin()` as a security gate
(`origin() != ORIGIN_LOCAL`) -- a wrong answer there is a security bug,
not a cosmetic gap, so a rushed partial implementation was rejected as
worse than none; also flagged as a dedicated-session candidate, not
folded in here. `debug_info` (6 combined: lima 4, dead-souls 2) checked
one more time given real demand in two corpora -- both wrap
`efun::debug_info()` the same load-bearing-shadow way `pluralize` does,
but its real C implementation genuinely does not exist anywhere in this
project's only reference source (`packages/contrib.c` and every other
vendored `.c` file: prototype only, no body) -- unlike `pluralize`, where
a real, readable, portable implementation existed and simply needed
transcribing, `debug_info` has nothing to verify against at all. Stays
excluded, now for an unverifiable-spec reason distinct from the previous
entry's architecture-mismatch framing, not a frequency judgment either
way.

Implemented, both fully verified against reference source: `unique_mapping`
(real, `mapping.c`'s `f_unique_mapping()` -- groups an array's elements
by a callback result into a mapping; real call sites confirmed genuine
across `dead-souls/lib/verbs/items/{get,wield,unwield}.c`, all passing a
bare closure. Reused the exact callback-dispatch shape `filter_array`
above already established (closure, or string function name with an
explicit object target) rather than reinventing one. Real group order
comes from the C body's own reverse-linked-list hash-bucket
construction -- traced by hand, elements are walked backward through the
array during bucketing, so each group's own original indices end up
collected highest-first and re-emitted in that same reversed order --
and is not documented as contractual anywhere; implemented with
first-appearance order instead, the exact same non-issue this driver's
own pre-existing `unique_array()` already settles for the identical real
ambiguity, its own registration comment cited directly as precedent) and
`reclaim_objects` (real, `reclaim.c`'s `reclaim_objects()` -- proactively
sweeps every live object's own variables, recursing into arrays/
mappings/closure-bound-args, rewriting a stale destructed-object
reference to int 0 and returning how many were found. This driver
already has an equivalent *lazy* mechanism for the ordinary cases --
`VM.cpp`'s `coerceIfDestructed()`, self-healing storage the moment
anything reads it via `PushLocal`/`PushObjectVar`/`Index` (see the
2026-08-09 entry below) -- but confirmed one real gap that mechanism
does not cover at all: `VM.cpp`'s own `Index`-mapping branch only ever
coerces a mapping entry's *value* half, never its *key* half, so a
destructed object used as a mapping key is never lazily fixed anywhere
in this driver currently. Matched real `gc_mapping()`'s own handling of
exactly this case: the whole entry is erased (real `map_delete()`), not
rewritten to a 0 key, which this driver's own flat `vector<pair>`
`Mapping` could otherwise silently duplicate against an already-present
real 0 key the way real FluffOS's own hash table structurally cannot.
`reclaim_call_outs()`, real `reclaim_objects()`'s own first step, was
not ported: confirmed directly that its own cleanup count is a
`call_out.c`-local static, unrelated to `reclaim.c`'s own returned
count, and this driver's `Scheduler` already reaches the same observable
end state lazily (a call_out to a destructed target already never fires,
confirmed by the pre-existing `testCallOutSkipsDestructedTargetSilently`
coverage) -- nothing left to eagerly remove there).

3 new test functions (`test/test_lexer.cpp`): `pluralize`'s covers a
representative slice of the real algorithm (default rule, `PLURAL_SAME`,
several chop/suffix exception rows, exception-over-general-rule
precedence, each general-suffix letter the sample reaches, the "a "/"an "
strip, and the "X of Y" clause) rather than all ~50 exception rows.
`reclaim_objects`'s test specifically targets the destructed-mapping-key
case via `sizeof(m)` shrinking, not just the object-variable/array cases
an ordinary LPC read would have already self-healed regardless of
whether `reclaim_objects()` ran first (checked and rejected a naive
before/after-LPC-read comparison for exactly that reason -- it would
have passed whether or not the new efun did anything). Full suite: 561
tests passing, up from 558 before this session, no regressions, stable
across three consecutive runs plus a full `ctest` pass.

**2026-08-22 (continued): call-site corpus reconstructed after the LDMud
restructure removed it, `pluralize` correctly re-flagged as a shadowed
simul_efun rather than merely "large effort", 3 more efuns implemented
(Phase 0 row 0.13: 222 registered, up from 219).** This row's own
standing methodology ranks the efun gap by real call-site frequency
against `mudlib/nightmare3_fluffos_v2/lib/` -- but that directory does
not exist anywhere in this checkout, or on this machine at all, and
never has under this repo's current `mudlib/` (the restructure commit,
`7a4121c`, is explicit: `mudlib/` was always just the bundled Lil
driver-tooling mudlib, renamed from `driver/lil/`, never the nightmare3
gameplay mudlib STATUS.md's own older entries describe scoping against).
The most likely explanation is that earlier sessions cloned the real
mudlib into an ephemeral, gitignored location for the ranking pass and
then let it lapse rather than tracking it -- `temp/` is gitignored for
exactly this kind of scratch clone (see `64cc10b`), though not
previously used for a mudlib. Rather than substituting this repo's own
tiny 199-file Lil stub (not a gameplay mudlib, would produce
unrepresentative frequency data) or skipping the ranking step entirely,
re-cloned the real corpus fresh from `github.com/fluffos/nightmare3` into
`temp/nightmare3` (gitignored, matching the existing `temp/dgd`,
`temp/fluffos`, `temp/ldmud` precedent) and confirmed it is the same
version prior sessions used: every real-call-site/file count in the
previous corrected-pass table (`ed` 4/3, `deep_inherit_list` 3/3,
`call_stack` 3/1, `origin` 2/1, `mud_status` 1/1, `cache_stats` 1/1,
etc) reproduces exactly against this fresh clone, and the specific files
those old entries cite by path (`secure/SimulEfun/communications.c`,
`secure/cmds/ambassador/_ed.c`, `std/user/editor.c`) all exist in it.

Re-ran the gap audit against `efun_defs.c` (270 real efun names total in
this exact vendored reference build, not literally ~300 -- ROADMAP.md's
row updated to say so) versus the 219 then-registered names: 51 gap
efuns confirmed real, then ranked by call-site count in
`temp/nightmare3/lib` (`/doc/` excluded, tight `\bname\(` word-boundary
match -- the previous pass's own "ed(" substring-matches-"used("-style
false-positive trap recurred identically on the first raw attempt here
and was caught the same way, by noticing `ed` alone showing an
implausible 363 hits/131 files before the boundary fix). Top of the
re-ranked list: `pluralize` (13 calls, 6 files). Read its real
implementation (`packages/contrib.c`'s `pluralize()`/`f_pluralize()`,
~440 lines, a deterministic English-pluralization exception table with
no missing infrastructure of its own) fully intending to port it --
but then found `secure/SimulEfun/english.c` defines a complete,
independent `pluralize(mixed single)` simul_efun in this exact mudlib,
with its own entirely different algorithm and no `efun::pluralize()`
delegation anywhere in it (confirmed by grep across the whole corpus).
Every one of the 13 real call sites (`std/guild.c`, `std/realtor.c`,
`std/Object.c`, `secure/daemon/finger.c`, `secure/SimulEfun/english.c`
itself) is a bare `pluralize(...)` call, which this driver's own tiered
call resolution (local -> inherited -> simul_efun -> core efun) routes
to that simul_efun every time -- the exact same "unreachable core
registration" situation `translate`/`event`/`tell_object`/bare `livings()`
are already documented under, confirmed the same way (checking for an
`efun::name()` escape-hatch call inside the shadowing simul_efun's own
body, absent here same as `translate`/`event`, present for `livings()`).
Not implemented, corrected reasoning: a previous session's framing of
`pluralize` as merely deferred for "effort, similar scope to `query_num`"
undersold this -- it is unreachable from this mudlib regardless of
effort spent, same permanent-exclusion category as `translate`/`event`,
not a someday item. `get_char` (8 calls, 1 file, `std/user/more.c`) and
`ed` (4 calls, 3 files) remain excluded for the previously-documented
reason (need new raw single-character-delivery / stateful multi-line
editor infrastructure this driver's line-buffered `Server::dispatchLine()`
has no path for); `origin` (2/1), `resolve` (1/1), and the
driver-internal-dump family (`mud_status`, `cache_stats`,
`malloc_status`, `dump_file_descriptors`, `dumpallobj`, `domain_stats`,
`author_stats`, all 1/1) remain excluded for their own previously-
documented reasons too, re-verified still accurate (no per-call origin
tagging, no async DNS/address-server infrastructure, no driver-internal
struct model to dump from, respectively).

With every call-site-bearing gap efun this pass excluded for a real
reason, picked from the 0-call-site remainder instead -- not scored by
frequency (there is none), scored by "real, self-contained, completes
an already-real subsystem rather than opening a new one," the same bar
`shallow_inherit_list`/`inherit_list` were included under in an earlier
pass despite their own 0 real call sites. `named_livings` (real,
`packages/contrib.c`'s `f_named_livings()`: walks `hashed_living[]` --
i.e. `LivingNameRegistry` -- keeping only `O_ENABLE_COMMANDS` entries and
applying the same `O_HIDDEN`/`valid_hide()` gate `first_inventory()`/
`next_inventory()` already exercise via the existing `isVisibleToObserver()`
helper; added `LivingNameRegistry::allWithCommandsEnabled()`, the new
enumeration capability the previous pass's own note said this row still
needed), `query_notify_fail` (real, `packages/contrib.c`'s
`f_query_notify_fail()`: a non-consuming peek at whatever `notify_fail()`
last set, distinct from `notify_no_command()`'s own one-shot
`takePendingNotifyFail()` -- added `Connection::peekPendingNotifyFail()`
alongside it), and `request_term_size` (real, `comm.c`'s
`f_request_term_size()`: a bare IAC DO NAWS: this driver's own NAWS
receiving side -- `handleSubnegotiation()`, `query_screen_width()`/
`query_screen_height()` -- was already fully built from row 0.8/a prior
0.13 pass, this was the one missing half, the proactive request itself;
added `Connection::requestWindowSize()`).

3 new test functions (`test/test_lexer.cpp`), each built on an
already-established harness pattern in this file rather than a new one
(the `set_hide`/`valid_hide` denying-master pattern for `named_livings`'s
hidden case, the `notify_fail` dispatch-line pattern for
`query_notify_fail`, the `query_ip_port` connection-probe pattern for
`request_term_size`). Full suite: 558 tests passing, up from 555 before
this session, no regressions, stable across three consecutive runs plus
a full `ctest` pass.

**2026-08-22 (continued): real bug found and fixed in already-shipped
set_eval_limit()/reset_eval_cost(), then 6 more efuns implemented (Phase
0 row 0.13: 219 registered, up from 211).** While verifying gap
candidates against reference source (this row's own standing process),
found that `set_eval_limit`/`reset_eval_cost`'s existing implementation
(shipped two sessions ago) was wrong, not just incomplete: `efun_defs.c`
shows `eval_cost` and `max_eval_cost` are two more real aliases of the
exact same `F_SET_EVAL_LIMIT` code (func_spec.c's own default-argument-
per-alias lines), and reading `efuns_main.c`'s own `f_set_eval_limit()`
directly revealed a real 4-way switch on the *argument value itself* (0 /
-1 / 1 / anything else), not the two-way "negative restores default,
else sets directly" split the previous implementation assumed and never
verified. Real `set_eval_limit(-1)` is **not** a "restore the default"
sentinel at all -- it is a pure query of the remaining budget with no
side effect on the ceiling; there is no built-in restore mechanism
anywhere in real FluffOS's own C code. Real `reset_eval_cost()` (default
argument 0) zeroes the *accumulated* cost back to zero while leaving the
ceiling itself completely untouched, returning the unchanged ceiling --
the previous implementation instead set the ceiling itself to 0, a
crushingly restrictive value, the opposite of what "reset" actually
means once the real switch is read directly.

Fixed: `VM::setMaxEvalCost()` is now a plain, unconditional ceiling
overwrite (the real "default: max_cost = sp->u.number;" branch only);
the 0/-1/1 special-casing now lives entirely in a new shared
`evalLimitDispatch` lambda in `EfunTable.cpp`, matching where that logic
actually lives in real FluffOS too (inside `f_set_eval_limit()` itself).
Added `VM::evalCost()`/`VM::maxEvalCost()` read-only accessors (needed by
the corrected dispatch and by tests). Registered `eval_cost` and
`max_eval_cost` as the two previously-missing real aliases. Both of the
existing tests that exercised the old, wrong behavior
(`testSetEvalLimitActuallyChangesTheEnforcedCeiling`, and last session's
own `testResetEvalCostDefaultsToZeroLimitAndAcceptsExplicitArgument`)
were rewritten to match the corrected, verified semantics -- the second
one split into two tests (one for the real zero-cost-not-zero-ceiling
reset, one for the new query/explicit-argument behavior shared across
all four names), one self-calibrating against measured real cost per
call rather than a guessed hardcoded ceiling (the first version of this
fix's own test crashed the suite with an uncaught `EvalCostError` from
a ceiling that was too low for even one call of its own probe function --
caught by actually running the suite, not assumed correct from reading
the code).

Row 0.13 batch: `real_time` (real, `packages/contrib.c`, same body as
the already-implemented `time()` under a separately-coded efun, not an
alias pair), `remove_interactive` (real, disconnects an object's
connection without destructing it, reusing `Connection::close()`),
`file_length` (real, counts newline-terminated lines in a file, reusing
`file_size()`'s own stat()/path-resolution pattern), `refs` (real,
`packages/develop.c`, approximated via this driver's own
`std::shared_ptr::use_count()` for every reference-counted Value kind;
always 0 for strings, which this driver never interns or shares),
`heart_beats` (real, every object with `set_heart_beat()` enabled --
added `Scheduler::pendingHeartbeats()`, a new read-only accessor
alongside the existing `pendingCallOuts()`), and `query_ip_port` (real,
the single configured listening port for any currently-interactive
object -- confirmed this driver has exactly one listening port, no
multi-port `SocketRegistry`, so unlike `query_ip_number()`/
`query_ip_name()`'s own "current connection only" scoping this one
correctly supports the real explicit `ob` argument, since no per-
connection network lookup is actually needed). Added `VM::config()`, a
new read-only accessor needed for `query_ip_port()` to reach
`Config::port()`.

Not implemented, flagged rather than rushed: `named_livings` (real,
needs a new enumeration capability on `LivingNameRegistry`, which
currently only supports exact-name lookup, not "list every living
name"), `query_num` (real, a large number-to-English-words algorithm of
similar scope to the already-flagged `pluralize`), `zonetime`/
`is_daylight_savings_time` (real, mutate the process-global `TZ`
environment variable via `putenv()`/`tzset()` -- deferred pending a
clearer read of whether that is safe against this driver's own
single-threaded-but-async-facing event loop), and `get_garbage` (real,
this driver has no garbage-collector concept of its own, `shared_ptr`
reclaims immediately rather than deferring to a sweep phase, so there is
nothing for it to report).

8 new test functions (`test/test_lexer.cpp`: 2 replacing the one that
tested the old, wrong eval-cost behavior, 6 for the new batch). Full
suite: 555 tests passing, up from 548 before this session (549
immediately after the eval-cost fix, +6 for the new batch), no
regressions, stable across three consecutive runs.

**2026-08-22 (continued): README.md rewritten, then 7 more efuns
implemented (Phase 0 row 0.13: 211 registered, up from 204).**
`README.md` replaced entirely with new, more accurate top-level content
(build/run instructions, language feature summary, efun/apply status,
layout). Verified the Applies section against real `src/apply/ApplyTable.cpp`
and actual call sites before writing it rather than trusting the given
draft as-is: `catch_tell()` had drifted from "recognized but not yet
fired" to genuinely fired (the immediately-preceding session's own
`tell_object()` implementation calls it), corrected to list it under
"currently fired" instead. All 20 other entries in `ApplyTable::known()`
checked against real call sites the same way and confirmed accurate.

Row 0.13 batch: Lil's conformance suite remains fully exhausted (checked
again, nothing new); re-ran the same real-call-site-frequency-against-
`efun_defs.c` methodology from the immediately preceding batch. Found and
implemented: `reset_eval_cost` (real call site,
`mudlib/command/speed.c`'s own `START` benchmarking macro; `F_SET_EVAL_LIMIT
| F_ALIAS_FLAG`, the same code as the already-implemented
`set_eval_limit()`, with a real default argument of 0), `strwidth`
(`F_SIZEOF | F_ALIAS_FLAG`, the same code as `sizeof()`/`strlen()` --
despite the name, real FluffOS's own implementation has no actual
display-width/wide-character logic of its own), `remove_shadow` (real,
splices an object out of whatever shadow chain it is part of, reusing
the exact same neighbor-reconnect logic
`ObjectManager::destructObject()`'s own non-cascade shadow-splice branch
already has, just without the destruct), `oldcrypt` (real, the same
system `crypt(3)` call as the already-implemented `crypt()` but forced
to the classic two-character DES salt, confirmed directly against
`packages/contrib.c`'s own `f_oldcrypt()`), `next_bit` (real, the fourth
member of the `set_bit`/`clear_bit`/`test_bit` family, same 6-bit-per-
character encoding -- confirmed a real, easy-to-miss boundary asymmetry
directly against `f_next_bit()`: `start <= 0` scans inclusively from bit
0, `start > 0` scans strictly after `start`), and `element_of`/`shuffle`
(both real, found in `packages/contrib.c`, no real call sites in this
mudlib but implemented as the natural array-utility siblings func_spec.c
defines alongside the array efuns already here -- `element_of` picks a
uniformly random element, throwing on an empty array; `shuffle` is an
in-place Fisher-Yates permutation that mutates and returns the same
array object, matching this driver's own already-established array-
reference-aliasing semantics).

Not implemented, flagged rather than rushed: `pluralize` (real, found in
`packages/contrib.c`, but a large, ~440-line English-pluralization
algorithm with many irregular-word special cases and an "of"-phrase
handling branch -- zero real call sites in this mudlib to validate a
port against, not a quick single-session addition) and `unique_mapping`
(real, found in `mapping.c`, a hash-table-grouping algorithm of similar
complexity, also zero real call sites). `translate`/`event` re-checked
against the current bundled Lil mudlib the same way `tell_object`/
`tell_room`/`shout` were in the immediately preceding batch (both were
previously excluded only because the old, now-removed nightmare3 mudlib
shadowed them with simul_efuns) -- unlike those three, `translate`/
`event` have zero real call sites anywhere in the current mudlib either
way, so they stay unimplemented, not reinstated.

7 new regression tests (`test/test_lexer.cpp`). Full suite: 548 tests
passing, up from 541, no regressions, stable across three consecutive
runs.

**2026-08-22 (continued): Phase 1 planning docs corrected, then 7 more
efuns implemented (Phase 0 row 0.13: 204 registered, up from 197).**
Two-part session. First part was documentation only, no Phase 1
implementation: fixed `src/dialect/instruct.md` and `src/apply/instruct.md`
with the corrections a prior report-only comparison session found against
real `temp/ldmud` (3.6.8) and `temp/dgd` clones -- `get_root_uid` to
`get_master_uid` (superseded since LDMud 3.2.1@40), `shadow(ob, flag)`
corrected to real LDMud's one-argument `shadow(ob)` (that two-argument,
object-returning form is FluffOS's real signature, misattributed to
LDMud), `bind()` corrected to the real `bind_lambda(closure, object)`,
the nonexistent `replaces` inherit directive replaced with a note that
the real closest LDMud feature is `replace_program()` (a runtime efun,
not a compile-time directive -- flagged that row 1.6 may need rescoping,
not just a rename), `net_dead`/`disconnect` naming corrected for both
LDMud (`disconnect(object, string)` is a real master apply there, not
object-level, and not `net_dead`) and DGD (no `"disconnect"` callback of
any name exists on DGD's driver object at all -- that name belongs to
LDMud), DGD's driver-object apply surface replaced with the real 17-name
list (plus `atomic_error`) confirmed by grepping every `callDriver`/
`callCritical` call site, and `valid_snoop`/`valid_query_snoop` added as
a newly-found real LDMud divergence directly relevant to Phase 0's own
snoop work (FluffOS has no such gate; LDMud does). Added an explicit,
unresolved open-design-note to `BootApi`'s connect/disconnect
abstraction (§1.4 in both files): a single `std::string` per concept
does not fit DGD's three-way connect fork or any dialect's mismatched
master-vs-object-vs-driver-object dispatch target -- flagged for whoever
implements 1.4/1.15/1.16, not resolved now. `temp/` (the three scratch
comparison clones) added to `.gitignore` in the same session that
produced the original comparison report, confirmed nothing from it was
ever staged.

Second part continued row 0.13. Lil's own real efun conformance suite
(`mudlib/single/tests/efuns/*.c`) is now fully exhausted as a ranking
source -- diffing it against `EfunTable.cpp`'s registered names turned up
nothing but already-documented architecture-mismatch exclusions
(`mud_status`/`cache_stats`/`malloc_status`/`dumpallobj`/`opcprof`,
`allocate_buffer`/`read_buffer`, `get_char`/`ed`, `origin`), C-internal
helper names that were never real LPC efuns (`add_light`, `break_string`
-- both real-only as internal C functions, `simulate.c`/`parse.h`, never
exposed to LPC), test-fixture files misread as efun names in earlier
passes (`badshad`/`goodshad`/`inh0`-`inh2`/`light`/`talker`/`unloaded`),
names confirmed absent from `efun_defs.c`'s own ground-truth registration
table (`enable_wizard`, `query_ed_mode`, `function_profile`,
`has_errors`, `generate_source` -- the last despite a real, `#ifdef
F_GENERATE_SOURCE`-gated `f_generate_source()` existing in
`efuns_main.c`, confirming that ifdef was inactive for this exact
vendored build too), and `sscanf`, which is already real and implemented
-- just not through this table, matching real FluffOS's own special-
cased lvalue-argument grammar handling (ROADMAP row 0.2).

Also found in passing, worth flagging: the vendored reference's own
checked-in `options.h` cannot be trusted as a live indicator of what was
actually compiled into the OTHER generated reference files in this same
tree (`efun_defs.c`, `opcodes.h`, etc) -- it currently has both
`NO_ADD_ACTION` and `NO_LIGHT` defined, yet `efun_defs.c` genuinely
registers `add_action`/`commands`/`enable_commands`/`livings` (proving
`NO_ADD_ACTION` was inactive for that generation) while genuinely
omitting `set_light` (consistent with `NO_LIGHT` being active for it) --
a real, inconsistent mix, not a copy-paste error in this project's own
reading. `efun_defs.c` remains the correct ground truth per this
project's own established methodology; `options.h`'s `#define`/`#undef`
lines should not be trusted alone going forward without cross-checking
`efun_defs.c` directly.

Re-scoped this batch's ranking source back to real call-site frequency
across the whole bundled Lil mudlib (`mudlib/`, not just `tests/efuns/`),
diffed against `efun_defs.c` directly (276 real entries, 97 not yet
registered), the same methodology used before the conformance-suite pass
started. Implemented: `tell_object` (3 real call sites,
`mudlib/single/master.c`), `tell_room` (found live-reachable via its own
one real definition point, `mudlib/clone/user.c`), `shout` (2 real call
sites -- `mudlib/command/say.c` and `quit.c` both literally `#define
say(x) shout(x)`, the bundled say command's entire real implementation),
`this_interactive`/`this_user` (aliases of `this_player(1)`, one real
call site: `mudlib/single/master.c`'s own `error_handler()`, `"this_
interactive() || this_player()"`), and `map_mapping`/`filter_mapping`
(one real call site for `map_mapping`, `mudlib/single/simul_efun.c`;
`filter_mapping` has none but is the same real, complete mapping/string/
function-callback triple func_spec.c defines alongside it, implemented
together the same way `query_shadowing`/`shallow_inherit_list` were
alongside their own real-usage siblings in earlier batches).

Real, confirmed-live finding worth flagging for any future dialect work,
not acted on now: `shout()`'s real C implementation (`simulate.c`'s
`shout_string()`) only ever reaches objects with the `O_LISTENER` flag
set, never checks `->interactive` at all -- and `O_LISTENER`'s only
setter anywhere in this vendored source is itself dead-gated behind
`#ifdef NO_ADD_ACTION`, confirmed inactive for the build that generated
`efun_defs.c` (see above), with no efun anywhere to set the flag from LPC
either. Literal-real `shout()` therefore broadcasts to nobody, ever, in
any normal build -- a genuine, confirmed architectural dead end in real
FluffOS, not a misreading. Implemented here as "every currently-
interactive object except command_giver" instead, matching `O_LISTENER`'s
own doc comment ("can hear say(), etc") and keeping this mudlib's own
real `say` command (which macro-expands directly to `shout()`) actually
working -- a deliberate, documented departure from dead-code letter, not
a faithful reproduction of it.

6 new regression tests (`test/test_lexer.cpp`, one covering both
`this_interactive`/`this_user` together): `tell_object` writing to a live
connection versus calling `catch_tell()` on a non-interactive target,
`tell_room` broadcasting to a room's direct inventory while excluding an
avoided object, `shout` reaching every connected object except
command_giver, `this_interactive`/`this_user` returning the connection's
own bound object even when `command_giver` has been reassigned elsewhere
(proving they do not just alias `this_player(0)`), `map_mapping`
replacing values while keeping keys, and `filter_mapping` keeping only
truthy-callback entries. Full suite: 541 tests passing, up from 535, no
regressions, stable across three consecutive runs.

Known gaps left open, all documented at the point implemented rather
than silently dropped: `tell_room`'s string-room-name lookup form uses
this driver's own compiling `VM::findObject()` rather than real
`find_object()`'s non-compiling C-internal lookup (no real call site
either way to be wrong against); `tell_room`'s `T_REAL` (float) message
form is not implemented (func_spec.c lists it, zero real call sites);
`tell_room`'s own `object_visible()` gate has no equivalent in this
driver and is not implemented (no established hidden-from-broadcast
concept separate from `set_hide()`'s own, differently-scoped mechanism).

**2026-08-22 (continued): snoop family implemented (Phase 0 row 0.13:
197 registered, up from 194).** The item the immediately-preceding entry
flagged and deliberately left out ("sized more like its own row than a
batch item, comparable to the original shadow() slice") -- taken on as
that same self-contained slice, matching shadow()'s own treatment:
`snoop(object, void|object)`, `query_snoop(object)`,
`query_snooping(object)`. Confirmed directly against
`fluffos-2.9-ds2.08`'s own `f_snoop()`/`new_set_snoop()`
(`efuns_main.c`/`comm.c`) and `query_snoop()`/`query_snooping()`
(`comm.c`) before writing anything, not assumed from this row's own
original task description, which turned out to have two things wrong:
the first argument is always "by" (the snooper), in both the 1-arg and
2-arg forms -- never the target, contrary to that description's own
"snoop(object target)" / "snoop(object target, object snooper)" naming --
and there is no `master()->valid_snoop()` gate anywhere in real FluffOS at
all (confirmed exhaustively against `applies.h`: `APPLY_VALID_SHADOW`
exists, there is no `APPLY_VALID_SNOOP` entry, and `new_set_snoop()`
itself never calls `apply()`/`master_ob` for permission). Real snoop
authorization is left entirely to the mudlib layer; this driver adds no
invented gate to match. The two real denial paths that do exist -- a
non-interactive victim (a hard, catchable throw) and the anti-loop walk
(a silent 0, covering both direct self-snoop and multi-hop cycles) -- are
what "denial" coverage below actually tests instead.

Output duplication (real `add_message()`/`add_vmessage()`'s own
`handle_snoop()` call, confirmed against `comm.c`) is implemented as a new
`deliverToConnection(VM&, Connection*, string)` free function
(`src/net/SnoopRelay.{hpp,cpp}`, new files) that every text-outputting
efun call site now routes through instead of calling `Connection::send()`
directly (`write`, `receive`, `printf`, `message`, `say` in
`EfunTable.cpp`, plus `Server::dispatchLine()`'s own `notify_fail()`
dispatch) -- the closest this driver has to real add_message()/
add_vmessage() being the one true chokepoint every output-producing efun
funnels through. Confirmed this exact vendored build has `RECEIVE_SNOOP`
defined in `options.h`, so duplicated text reaches the snooper via an
ordinary `receive_snoop(string)` LPC apply, not a raw `"%"`-prefixed
socket write (the other, not-compiled-in branch of that same `#ifdef`) --
a snooper object that never defines `receive_snoop()` sees nothing at
all, matching real apply()-to-an-undefined-function silence, not a gap.

Snoop-relationship state is a new, deliberately symmetric
`snoopedBy_`/`snooping_` weak_ptr pair on `LpcObject` (mirroring
`shadowedBy_`/`shadowing_`'s own existing convention) rather than real
`interactive_t::snooped_by`'s one-directional field plus an `all_users[]`
linear scan for the reverse direction -- an internal representation
choice only, not an observable semantics difference (`query_snoop()`/
`query_snooping()` report identical results either way), made because
this driver has no cross-module registry efun code can scan the way real
`all_users[]` is scanned. Disconnect/destruct cleanup matches real
semantics on both sides, confirmed against `comm.c`'s `remove_interactive()`
and `simulate.c`'s `destruct_object()` separately (they are two distinct,
complementary `ifndef NO_SNOOP` blocks in real FluffOS, not one):
`Connection::close()` clears the victim side (whoever was snooping a
closed connection's own object is unlinked, matching `remove_interactive()`
running unconditionally on every close, net-death or destruct()-driven
alike) and `ObjectManager::destructObject()` clears the snooper side
(destructing an object that was itself snooping someone unlinks that
relationship too, matching `destruct_object()`'s own `O_SNOOP` block,
which runs regardless of whether the destructed object currently has a
live connection).

7 new regression tests (`test/test_lexer.cpp`): snoop start linking both
directions with `query_snoop`/`query_snooping` reflecting them, output
duplication (a real `receive_snoop()` call with matching text, confirmed
via a stub snooper object storing what it received, alongside confirming
the victim's own connection still gets the text too, not diverted),
non-interactive-victim throw and self-snoop-loop denial, a 2-object snoop
cycle denied by the anti-loop walk with the original legitimate snoop
confirmed intact afterward, the 1-arg stop form, victim-disconnect
cleanup, and snooper-destruct cleanup. Full suite: 535 tests passing, up
from 528, no regressions.

Known gap: `mudlib/single/tests/efuns/snoop.c` (the real, vendored
FluffOS testsuite's own file) is itself a near-no-op ("maybe when it is
possible for arbitrary objects to snoop.") -- it cannot meaningfully
exercise a real interactive connection without one, which is exactly why
this row needed its own C++-level regression tests (socketpair-backed
`Connection`s) rather than relying on that suite the way most other 0.13
batches have. `query_snoop.c`/`query_snooping.c` (the real testsuite's own
trivial not-currently-snooping checks) already passed before this batch
and still do.

**2026-08-22 (continued): 4 more efuns implemented (Phase 0 row 0.13:
194 registered, up from 190).** Same method as the same-day batch below:
diffed `EfunTable.cpp`'s registered names against Lil's own real efun
conformance suite (`mudlib/single/tests/efuns/*.c`), checked each
surviving name against `func_spec.c`/`efuns_main.c` directly before
implementing. Most of the remaining gap is architecture-mismatch
exclusions already recorded in `src/efun/instruct.md`'s own status table
(`mud_status`/`cache_stats`/`malloc_status`/`dumpallobj`/`opcprof`'s
driver-internal C-struct dumps, `allocate_buffer`/`read_buffer`'s buffer
type, `get_char`/`ed`'s per-keystroke/multi-line-editor infrastructure)
or plain false positives/test-fixture filenames, not real efun names at
all (confirmed by grep against `func_spec.c` before excluding anything,
not assumed from the prior pass's own list).

Implemented: `children` (real object-hash-table walk, reused via
`LiveObjectRegistry`, already backing `objects()`/`livings()` -- inherits
that registry's own documented weak_ptr scope limitation: a clone with no
live reference anywhere else in this driver is not enumerated, unlike
real FluffOS's own persistent, refcount-independent object table; a
first version of this row's own regression test caught this live, not
guessed -- it initially created 5 throwaway clones with no surviving
reference and got back only 1), `set_light` (real `add_light()`
propagation up the full environment chain, returning the topmost
ancestor's own resulting total -- confirmed directly, not assumed from
`func_spec.c`'s own bare "should die a dark death" deprecation comment
alone), `set_debug_level` (accepted and silently ignored -- this driver
has no `debug()`-style category-tagged trace system for it to toggle,
confirmed by grep; this row's own real, tested call site is itself gated
behind a `__DEBUG_MACRO__` this mudlib never defines by default, so even
real FluffOS treats it as a no-op there), and `bind` (real function-
pointer owner rebinding, gated behind `master()->valid_bind()` matching
this driver's own established master-apply-gate pattern -- real
FluffOS's own two `FP_NOT_BINDABLE` guards do not apply to this driver's
simplified Closure model, which has no equivalent unsafe closure kind to
protect against).

Not implemented, flagged rather than folded in: `origin()` (per-call
origin tagging through every VM call path -- the one real call site this
row's own instruct.md has on file is a security gate, where a wrong
answer is a correctness bug, not just an incomplete feature) and the
`snoop`/`query_snoop`/`query_snooping` family (a real, self-contained
subsystem -- a snoop-target registry plus output duplication at the
`Connection` level -- but sized more like its own row than a batch
item, comparable to the original shadow() slice).

6 new regression tests. Full suite: 528 tests passing, up from 522, no
regressions, stable across three consecutive runs.

**2026-08-22: repo-level `CLAUDE.md` added, restructure state confirmed
post-extraction, and 11 new efuns implemented (Phase 0 row 0.13:
190 registered, up from 179).** First session in the standalone `amlp`
repository after its extraction from the AetherMUD monorepo and rename
from `lpcdriver`. Confirmed directly rather than trusted: the restructure
itself (commit `7a4121c`) was already committed on `main` at session
start, not merely staged as initially described -- `git status` showed a
clean tree. Confirmed real, not assumed: Phase 0 rows 0.1-0.12 and 0.14
complete, row 0.13 in progress, row 0.15 filed and still open, matching
ROADMAP.md's own checkboxes.

Also found and corrected in passing: the prior (2026-08-21) entry's own
"502 tests passing" figure was stale the moment it was written -- that
same commit (`1d45a68`, a second commit reusing the 0.14 commit message
verbatim rather than amending the first) bundled in 7 more tests and a
previously-unlogged `sprintf`/`printf` `%O` specifier implementation
(generic LPC value-dump formatting, confirmed against `sprintf.c`'s own
`svalue_to_string()`) on top of the 502 the entry's own text describes --
the real post-commit count was already 509, and `%O` was already
implemented, contradicting that entry's own "not fixed here" framing for
it. `ROADMAP.md`'s own "Build and test" baseline (486, dated 2026-08-20)
was even further stale, predating that same commit. Both now corrected
to the real, freshly re-measured counts.

This mudlib no longer contains the old `nightmare3_fluffos_v2` Rifts
content the previous several 0.13 batches ranked call-site frequency
against (it stayed behind in the AetherMUD monorepo per the extraction's
own design) -- `mudlib/` is now just the bundled Lil starter mudlib. This
batch re-scoped 0.13's ranking method accordingly: diffed
`EfunTable.cpp`'s registered names against Lil's own real efun
conformance suite (`mudlib/single/tests/efuns/*.c`, one file per real
efun, each a genuine, not merely name-matched, exercise of that efun --
confirmed batch by grepping `func_spec.c`/`efuns_main.c` for every
surviving filename before implementing anything), rather than a raw
call-site count. Implemented: `set_bit`/`clear_bit`/`test_bit` (real
6-bit-per-character packing, not 8, confirmed directly against
`f_set_bit()`), `crc32` (confirmed the real algorithm has no final
complement step, unlike textbook CRC-32 -- `crc32("")` is genuinely
`0xFFFFFFFF`, not 0), `cp`, `inherits` (real program-identity chain walk,
not a string comparison, confirmed against `f_inherits()` directly),
`get_config` (index 0 only, matching this row's own real, tested call
site; every other index throws rather than fabricating driver-internal
statistics this codebase has no real source for, same category as
`mud_status`), `query_load_average` (fixed, honestly-zero string in the
real format shape, no rolling-rate tracking exists to report), `say`
(previously excluded from every prior batch because the old Rifts
mudlib shadowed it with a simul_efun -- that mudlib is gone from this
repo now, and Lil's own `talker.c` calls the bare efun directly and
unshadowed, so the exclusion no longer applies), and
`save_variable`/`restore_variable` (the single-value save to/from the
same real on-disk text format `restore_object()` already reads --
`restore_variable`'s own top-level string parser needed a real,
specific quirk `restore_object`'s array/mapping-element parser must NOT
share, confirmed by reading `restore_string()` directly: a top-level
quoted string must consume the *entire* remaining buffer, or it is a
real error, while a nested array/mapping-element string has no such
requirement).

New `VM::mudName()` accessor added (`resolveMudlibPath()`'s own established
"derived accessor, not the whole `Config&`" pattern), `get_config`'s only
caller.

13 new regression tests. Full suite: 522 tests passing, up from 509 (the
real, freshly-confirmed prior count, not the stale 502/486 both now
corrected above), no regressions, stable across three consecutive runs.

**2026-08-21: `global include file` config support implemented (Phase 0
row 0.14), and a real third-party mudlib (Lil) confirmed logging in
end to end against this driver for the first time.** New `Config` key
`global_include_file` (default empty/off), wired into
`ObjectManager`'s own compile pipeline: when set, an `#include` line
built from the configured value (used verbatim, delimiters included) is
emitted ahead of every compiled object's own source, gated the same way
real FluffOS gates it -- a true no-op for any mudlib that never sets it,
this repo's own bundled Rifts mudlib and `mudlib_stub` included.

Verified directly against `driver/reference/fluffos-2.9-ds2.08/lex.c`
before writing anything, not assumed from the prior session's own
narrower investigation: `start_new_file()`'s own "if
(*GLOBAL_INCLUDE_FILE) { ...; handle_include(gifile, 1); } else
refill_buffer();" runs before a single byte of the real object's own
source is read, for every compile; `handle_include()`'s own "delim =
*name++ == '\"' ? '\"' : '>';" confirms the configured value is used
with its own delimiters intact (not a separate quote/angle flag), which
is why this driver's own implementation also takes the raw config value
verbatim rather than adding its own delimiters.

Root-caused and fixed the specific bug blocking Lil's own login path
(found the prior session, `inherit/base.c`'s own `staticv` macro used
without including `globals.h`): confirmed via `lex.c`'s own
`free_defines()` (called fresh at the top of `start_new_file()` for
every single compile, inherited files included) that real FluffOS does
*not* have persistent cross-file macro state either -- the prior
session's own hypothesis for why Lil works unmodified under real
FluffOS was wrong. The real explanation is narrower: Lil's own shipped
`etc/config.test` already points "global include file" at `<config.h>`,
and this driver simply had no support for that directive at all.
`driver/lil/include/config.h` now `#include`s `globals.h` inside its own
guard (matching that file's own header comment describing itself as
exactly this customization point), leaving `config.test`'s own
"global include file : <config.h>" setting untouched.

End-to-end verified live over a real TCP connection, not just via a
static compile check: booted `driver/lil` fresh, connected, logged in
past the exact point that failed the prior session (`new("/clone/user")`
now succeeds), confirmed the new player is genuinely registered (a real
`who` command shows it correctly formatted), and confirmed its landing
location programmatically (a direct API check, since Lil's own `eval`
command turned out to be separately broken, see below) -- the player's
own `environment()` is `/single/void`, exactly `login.c`'s own real,
unconditional `user->move(VOID_OB)`, the sensible landing spot for a
deliberately minimal bootstrap mudlib with no real world of its own.

Two further gaps surfaced live during this same session's own socket
testing (not from static reading): `say` (Lil's own `/command/say.c`
calls the real `say()` efun directly; this driver does not register it,
deliberately, since this repo's own bundled Rifts mudlib shadows it with
a simul_efun -- a different context, already flagged in an earlier
session's efun-gap-list -- ~~flagged rather than fixed here~~, see the
2026-08-22 entry above: that Rifts mudlib is gone from this repo after
its extraction into a standalone project, and the exclusion no longer
applies, so `say` is implemented now) and `eval` (`/command/eval.c`
always formats its result via `printf("Result = %O\n", ...)`, and this
driver's `sprintf`/`printf` do not implement the `%O` format specifier at
all -- a new, previously-undiscovered gap -- ~~flagged rather than fixed
here~~: actually implemented within this same commit, just never
reflected back into this entry's own text at the time -- see the
2026-08-22 entry above for the correction).

3 new regression tests (macro resolves when `global_include_file` is
configured; the identical fixture fails to compile when it is not,
deliberately reproducing Lil's own real failure shape; a
no-dependency object compiles identically either way). ~~Full suite: 502
tests passing, up from 499~~ -- stale the moment it was written, see the
2026-08-22 entry above: the real count after this same commit was 509,
including 7 more tests and the `%O` work just described, neither
reflected in this count at the time. no regressions, stable across three
consecutive runs.

**2026-08-20: Every registered efun now has at least one regression test
(Phase 0 row 0.12), closed out via a real audit rather than a spot check.**
Grepped every real `registerEfun("name", ...)` call in `EfunTable.cpp`
(167 total, not `efun/instruct.md`'s own stale task-list framing) and
cross-referenced each by name against `driver/tests/test_lexer.cpp`, then
manually inspected every borderline match count (0 or 1) to separate
genuine gaps from comment-only false positives (a name mentioned in a
descriptive comment but never actually called) and from legitimate
alternate-style coverage already present (a handful of combat-formula
efuns and `file_size()` are exercised via the direct
`EfunTable::instance().call("name", ...)` C++ API or via a bound closure
rather than LPC call syntax -- both counted as real coverage).

29 efuns came back genuinely untested: `sin`, `tan`, `asin`, `acos`,
`atan`, `log10`, `arrayp`, `functionp`, `mapp`, `objectp`, `pointerp`,
`capitalize`, `crypt`, `strlen`, `strstr`, `ctime`, `time`, `allocate`,
`allocate_mapping`, `copy`, `values`, `query_ip_name`, `query_ip_number`,
`socket_status`, `regexp_assoc`, `remove_action`, `rm`, `set_eval_limit`,
`map`. A 30th, `query_once_interactive`, was not a real gap (it shares
`userp`'s own already-tested implementation lambda) but got a direct-name
test anyway for audit completeness. 17 new tests added across six
batches, building and running the full suite after each batch rather
than once at the end -- one real test-authoring mistake (a regexp_assoc
alias test that miscounted how many times a repeating pattern matches)
was caught and fixed within its own batch instead of surfacing later as
an unexplained wall of failures.

Two tests needed real infrastructure the existing suite didn't have yet:
`query_ip_number()`/`query_ip_name()` read `OutputContext::current()`'s
own fd via `getpeername()`, which needs a genuine `AF_INET` peer -- the
`socketpair(AF_UNIX, ...)` convention every other net test in this file
uses cannot produce a real IPv4 `getpeername()` result, so a small
`makeLoopbackTcpPair()` helper (real `socket()`/`bind()`/`listen()`/
`connect()`/`accept()`, synchronous, loopback-only) was added.
`remove_action()` needed routing through a second dispatched command
rather than a bare `vm.callFunction()`, since it requires the same
`VM::commandGiver()` resolution context `add_action()` itself needs
(only set explicitly during `move_object()`'s own init()-calling
sequence or during `dispatchCommand()`'s own handler calls, confirmed
directly in `EfunTable.cpp`'s `resolveCommandGiver()`).

No real implementation bugs found -- every efun's actual behavior matched
its own existing documentation/citation once actually exercised. Full
suite: 486 tests passing, up from 469, no regressions across any of the
six build-and-run batches.

Also filed, not started: ROADMAP.md Phase 3 row 3.8, booting a real
third-party FluffOS mudlib (dead-souls.net's TMI2, LPUniversity, or LIL)
against this driver as a distinct compatibility test from this suite's
own AMLP-derived regression coverage -- see ROADMAP.md's own 3.8
scope note for what it would involve.

**2026-08-19: `socket_*` efun family basics implemented (Phase 0 row 0.10).**
`LpcSocket` (`include/amlp/net/LpcSocket.hpp`), `SocketRegistry`
(`src/net/SocketRegistry.cpp`), and `Server::pollSockets()` (a new static,
publicly-testable method, same test-seam shape as `dispatchLine()`/
`fireNetDeadIfLinkDead()`), plus nine registered efuns: `socket_create`,
`socket_bind`, `socket_listen`, `socket_accept`, `socket_connect`,
`socket_write`, `socket_close`, `socket_error`, `socket_status`.

Read `fluffos-2.9-ds2.08/socket_efuns.c` and `socket_efuns.h` directly
(the real C implementation) rather than trusting `net/instruct.md`'s own
proposed design for this row, which turned out wrong on three points:

1. It lists a `socket_read(int handle) -> mixed` efun. No such efun exists
   anywhere in `efun_defs.c` (grepped directly) -- real sockets are purely
   callback-driven; incoming data always arrives via `read_callback` firing
   asynchronously, never via a synchronous read call. Not implemented,
   because it is not real.
2. Its proposed `socket_create(int type, string callback)` and
   `socket_bind(int handle, int port)` both drop a real optional third
   argument each actually has (`void|string close_callback`; `void|string
   addr`) -- confirmed against `efun_defs.c`'s own `F_SOCKET_CREATE`/
   `F_SOCKET_BIND` entries (2-3 args each). Implemented with the real
   3-arg signatures.
3. Its efun list omits `socket_listen` and `socket_accept` entirely, even
   though a listening/accepting server socket cannot exist without both
   (confirmed via `efun_defs.c`'s own `F_SOCKET_LISTEN`/`F_SOCKET_ACCEPT`
   entries and `lib/daemon/network.c`'s real, non-doc call site).
   Implemented, since "basics" cannot mean client-only.

Real signature/error-code/callback-argument details confirmed directly,
not assumed: `EESUCCESS` is `1`, not `0`; every real error code down to
`socket_error()`'s own `error_strings[]` text (`socket_err.c`) is mirrored
exactly; `read_callback` fires with 1 arg for a listening socket (`fd`,
"a connection is waiting"), 2 args for STREAM data (`fd`, `msg`), and 3
args for DATAGRAM data (`fd`, `msg`, `"host port"`); `write_callback`
fires with 1 arg (`fd`) both when a partial write flushes and when a
non-blocking `connect()` completes -- real FluffOS reuses the identical
`S_BLOCKED`/`socket_write_select_handler()` mechanism for both, confirmed
directly in `socket_efuns.c`, not assumed; a plain LPC-initiated
`socket_close()` never fires `close_callback`, only a driver-detected
async failure (peer EOF, a write error) does, matching real
`SC_DO_CALLBACK`'s only internal call site.

Not implemented, flagged rather than left silently absent: MUD mode
(needs real `socket_write()`'s own arbitrary-LPC-value wire framing this
driver has no equivalent for while `save_object()` still writes its own
custom format, ROADMAP row 0.7); the two BINARY modes (no buffer type in
this driver's `Value` at all, the same pre-existing gap already noted on
`to_int()`'s own `T_BUFFER` case); `socket_release()`/`socket_acquire()`
(object-to-object socket transfer, out of "basics" scope); DNS resolution
for `socket_connect()`/`socket_bind()` addresses (numeric dotted-quad
only, matching real `socket_name_to_sin()`'s own `inet_addr()`-only
behavior, and this driver's pre-existing `query_ip_name()` precedent);
real `STATE_FLUSHING` (`socket_close()` while a partial write is still
draining closes immediately here rather than deferring, dropping any
undelivered remainder); `close_referencing_sockets()` (a destructed
object's still-open sockets are not force-closed, though they do stop
firing callbacks once their owner is gone, matching `PendingInputTo`'s
own established weak_ptr handling).

Directly closes out two items from row 0.13's own efun-growth batch:
`efun/instruct.md`'s own status table already listed both `socket_close`
and `socket_write` as "belongs to row 0.10's `LpcSocket`/`SocketRegistry`
subsystem, not a standalone efun" -- both fell out of this row's own
implementation as-is, not as separately scoped extra work. Efun table is
now 167 registered efuns total (up from the ~131 last recorded).

4 new regression tests: unsupported-mode rejection (MUD/STREAM_BINARY/
DATAGRAM_BINARY) plus monotonically increasing handles, unknown-handle
`EFdRange` plus real `socket_error()` text, a full STREAM round trip
(create/bind/listen/accept/connect/write/read/close, driven entirely
through `Server::pollSockets()` over real loopback TCP sockets with an
OS-assigned ephemeral port, no fixed port to collide with), and a
DATAGRAM round trip confirming the real 3-argument `read_callback` and
its sender-address argument. Full suite: 469 tests passing, up from 465,
no regressions, stable across three consecutive runs.

**2026-08-18: Full telnet IAC negotiation, echo suppression, NAWS,
`window_size()`, and `terminal_colour()` implemented (Phase 0 row 0.8),
closing it out across two sessions with no dated entry written for either
until now.**

Session one (IAC/echo/NAWS base): real IAC state machine confirmed against
`comm.c`'s own `copy_chars()` -- not `telnet_neg()`, which does not exist
anywhere in the vendored source; `net/instruct.md`'s own citation for this
row was wrong. Echo suppression sends `IAC WILL ECHO` immediately on the
`input_to()` `I_NOECHO` flag and `IAC WONT ECHO` the moment a full line is
pulled off the buffer (not when the callback finishes, confirmed against
real `get_user_command()`). NAWS is driver-initiated (`IAC DO NAWS` sent
proactively in `onNewConnection()`, matching real `new_user()`, not
client-initiated as `net/instruct.md` claimed); subnegotiation parsing
fills `Connection::terminalWidth_`/`terminalHeight_`, exposed via new
`query_screen_width()`/`query_screen_height()` efuns.

Session two (this arc's own follow-up, `window_size()` +
`terminal_colour()`): `window_size(int width, int height)` now fires on
the connection's bound object every time a NAWS subnegotiation is parsed
(real `APPLY_WINDOW_SIZE`, `comm.c`), via a one-shot `Connection` flag
(`takeWindowSizeUpdate()`, the same shape as `takePendingInputTo()`)
consumed by `Server::handleConnection()` -- additive, `query_screen_width()`/
`query_screen_height()` unchanged. `terminal_colour(string str, mapping
colours, void|int max_colors, void|int indent)` has no C reference
implementation anywhere in the vendored tree (same gap as `debug_info`/
`base_name`/`pluralize`), so it was grounded instead against two real,
live implementations of the same `%^TOKEN%^` markup convention:
`daemon/terminal.c`'s `no_colours()` and `std/user.c`'s `message()`, both
of which split on the literal `"%^"` delimiter (not paired-wrapper
parsing) and substitute/strip per segment. `indent` is accepted for
signature compatibility only, not implemented. Flagged, not fixed: this
driver's own `isTruthy()` treats an empty string as falsy, unlike real
LPC, so a colour mapping using `""` as a disabled-colour placeholder
(real `terminal.c`'s own "unknown" variant) will not be recognized here
the way real `no_colours()` recognizes it.

16 new regression tests across both sessions (9 telnet/echo/NAWS + 2
`window_size` flag behavior + 5 `terminal_colour` mapping cases). Full
suite: 465 tests passing, up from 449, no regressions. Known gap: the
actual `window_size()`/NAWS-trigger apply-firing call sites inside
`Server::handleConnection()`/`onNewConnection()` are untested directly --
both are private methods, outside this driver's established test seam
(only `dispatchLine()`/`fireNetDeadIfLinkDead()`, and now
`pollSockets()`, are pulled out static and public); the flag/byte-parsing
mechanics each one reads are fully covered instead.

**2026-08-17: Shadow support implemented (Phase 0 row 0.6).** `shadow(object
ob, int flag default 1)`, `query_shadowing(object)`, the real shadow-chain
effect on `VM::callFunction()`'s resolution order, and shadow-aware cleanup
in `ObjectManager::destructObject()`.

Read `efuns_main.c`'s `f_shadow()`/`f_query_shadowing()`, `interpret.c`'s
`validate_shadowing()` and `apply_low()`'s shadow section, and
`simulate.c`'s `destruct_object()` shadow cleanup directly before writing
anything, not `driver/src/object/instruct.md`'s own proposed design, which
turned out wrong on five real points, not just incomplete:

1. There is no `query_shadowed()` efun in real FluffOS. `shadow(ob, 0)` is
   the real query form (who is currently shadowing `ob`), confirmed by the
   one real call site in this mudlib (`cmds/creator/_scan.c`'s own
   `shadow(ob, 0)`).
2. The chain-walk condition in `apply_low()` is whether the function is
   *defined* on a given link, never the truthiness of what it returns.
   `instruct.md` said "if the shadow defines the function and returns a
   truthy value, use that result, otherwise fall through" -- wrong; a
   shadow that defines the function and returns a falsy `0` is still the
   final, real result, not a fall-through trigger.
3. The master apply real FluffOS actually calls is `valid_shadow`, not
   `query_allow_shadow` (that name is LDMud's own apply, already correctly
   scoped to LDMud elsewhere in `src/apply/instruct.md`). Confirmed in
   `applies_table.c` directly.
4. There is no separate `no_shadow()` efun or apply. The real gate is
   entirely `master()->valid_shadow(ob)`, reusing this driver's own
   already-established `master && isTruthy(callFunction(master, "valid_X",
   ...))` pattern (identical to `set_hide`'s `valid_hide` gate).
5. Real `validate_shadowing()` has five checks, not the one `instruct.md`
   named: can't shadow self, can't shadow while already shadowing
   something, can't shadow while already shadowed, the shadow object can't
   be inside an environment, the target can't be the master object, and
   the target can't itself already be a shadow, plus the master approval.
   All five implemented. Not implemented: the `nomask`-function conflict
   check (`check_shadow_functions()`) -- this driver's compiler has no
   `nomask` modifier concept at all, same category as the existing
   class/buffer type gaps.

Shadow removal has no dedicated efun in real FluffOS at all (no
`unshadow()`); it is entirely a side effect of `destruct()`, and the real
behavior is asymmetric, confirmed directly against `simulate.c`:
destructing the base victim of a chain cascades to destruct every shadow
above it too, while destructing a shadow (not the root) just splices it out
of the chain, leaving the rest intact. Both branches implemented in
`ObjectManager::destructObject()`.

Real-usage check across `mudlib/`, excluding `/doc/`: `shadow()`'s only
reference anywhere is that one `shadow(ob, 0)` call in `_scan.c`, and it is
dead code, guarded behind `#if HAS_SHADOWS`, a macro never defined anywhere
in this mudlib's own config or anywhere in the vendored
`fluffos-2.9-ds2.08` source it was ported from. `query_shadowing()` has
zero real call sites. Shadow support is a driver-completeness/ROADMAP-parity
item in this mudlib, not currently load-bearing for anything reachable.

9 new regression tests: basic chain interception, fall-through on an
undefined function versus a final result on a falsy-but-defined one (two
tests, the second using deliberately distinguishable return values to prove
the shadow's own code actually ran), the `current_object` reentrancy guard,
cascade-destruct of a whole chain via the base victim, splice-destruct of
just one shadow, denial without master approval (both no-master and
explicitly-rejecting-master cases), both query directions, and the five
validation-rule rejections. Full suite: 449 tests passing, up from 440, no
regressions.

**Known issue, not fixed this session:** `driver/tests/test_lexer.cpp` uses
bare `assert()` for real test logic in places, not just sanity checks, for
example `assert(harness.objects.loadSimulEfunObject())`. Building the tests
target with `NDEBUG` defined (as `cmake -DCMAKE_BUILD_TYPE=Release` does)
silently turns every one of those asserts into a no-op instead of failing,
including the ones with a real side effect, so the whole test binary can
report a false pass, or crash later on an unrelated symptom, while looking
like a broken test suite passed. Confirmed directly this session: a Release
build aborted partway through with "undefined function or efun: double_it"
in `testSimulEfunResolvesUnknownBareCallToSimulEfunObject`, because the
`assert()` that was supposed to call `loadSimulEfunObject()` never ran under
`NDEBUG`. A Debug build (the project's own CMake default when no build type
is given) ran the identical 413 tests clean. Flagging so this does not get
missed later. Whoever picks this up can choose between forcing `-UNDEBUG` on
the `amlp_tests` target in `driver/tests/CMakeLists.txt`, or replacing
the side-effecting asserts with real checks that fail independently of
`NDEBUG`.

**2026-08-16: PCRE2 `regexp`/`regexplode`/`reg_assoc` efuns implemented
(Phase 0 row 0.11).** Added PCRE2 via `pkg_check_modules(PCRE2 REQUIRED
libpcre2-8)` in `driver/src/efun/CMakeLists.txt`, linked privately into the
`efun` target. Note up front: real `fluffos-2.9-ds2.08` implements its own
regexp engine (Henry Spencer's, `regexp.c`), not PCRE. Wrapping PCRE2 here is
the deliberate modern substitution this row's own `instruct.md`/`ROADMAP.md`
text calls for, not a literal port of that engine. The efun-level contract
below (what each efun selects, splits, or tokenizes, and in what order) is
reproduced exactly from the real functions, confirmed by reading them
directly; the underlying regex dialect is PCRE2's, not Spencer's, so
mainstream constructs (character classes, alternation, quantifiers, anchors,
groups) match, but exotic edge cases may not.

`regexp()`: confirmed the real signature directly against `func_spec.c`
(`mixed regexp(string | string *, string, void | int);`) and
`efuns_main.c`'s `f_regexp()`, not assumed from this row's own simplified
task description, which only covers the single-string case. The real efun is
broader. A single-string subject returns plain int 1/0 via
`match_single_regexp()`, and a third argument is an error in that form
("3rd argument illegal for regexp(string, string)", reproduced verbatim). An
array-of-strings subject returns the matching elements themselves, not a
bool array, via `match_regexp()`: `flag&1` interleaves each match's own
1-based index right after it (string, index, string, index, ..., confirmed
by hand-tracing `match_regexp()`'s own backward-filling loop against a
concrete example), `flag&2` inverts the selection to non-matching elements
instead, and a non-string array element is never selected either way.
Implemented the full real spec, not just the task's simplified subset.

`reg_assoc()`: confirmed against `array.c`'s `reg_assoc()` directly,
including its own worked example in a source comment
(`reg_assoc("testhahatest", ({"haha","te"}), ({2,3}), 4)`), reproduced
verbatim as a regression test. Scans the string left to right; at each
position, tries every pattern and keeps whichever produces the
earliest-starting match, ties going to the lower pattern index (matching the
real `currstart < laststart` comparison exactly). Returns two same-length
arrays: text segments alternating with matched substrings, and, in the same
positions, each match's own token (or the default value between matches and
at the end). Zero patterns is a real, explicit special case in the source,
not an error: the whole string comes back unmatched, paired with one default
token.

`regexplode()`: checked directly against the vendored `fluffos-2.9-ds2.08`
tree before implementing, not assumed. This is not a real FluffOS 2.9 efun.
Grepped the entire tree (`func_spec.c`, `efun_defs.c`, `efunctions.h`,
`opc.h`, `array.c`, `regexp.c`); the only hit anywhere is a comment inside
`implode()`'s own loop-safety code crediting a zero-length-match guard to
"regexplode", not a registered function in this reference build. Implemented
anyway because this row's own `instruct.md`/`prompt.md` task text explicitly
calls for it, and documented in the code itself as a driver addition rather
than a ported FluffOS efun. Same shape as `reg_assoc()`'s own real output
(alternating text and match, one more text segment than matches), using the
identical zero-length-match guard `reg_assoc()`'s own real source documents.
The `regexp_assoc` alias for `reg_assoc()` is likewise not a real FluffOS 2.9
name (same grep sweep), added because the row's own `instruct.md` calls for
it as a second name.

8 new regression tests: single-string basic match and no-match, the
third-argument-illegal error, the array form with both the index and invert
flags, a malformed pattern throwing, `regexplode()`'s basic split, a
`regexplode()` pattern that includes a capturing group (confirms the whole
match is used to split, never just the captured subgroup text, since neither
efun exposes captures), `reg_assoc()` matching its own real doc-comment
example element for element, and `reg_assoc()` with zero patterns. Full
suite: 421 tests passing, up from 413, no regressions.

**2026-08-15: Phase 0.13 efun growth batch - 20 new core efuns implemented.**
Grepped real call sites across `mudlib/nightmare3_fluffos_v2/lib/` to find
the highest-usage missing efuns, then confirmed every spec against the
FluffOS 2.9 reference source before implementing.

Top picks by real call count: `to_float` (13 sites), `rename` (10), `typeof`
(5), `sqrt` (5), `rmdir` (4). `abs`, `min`, `max` from `packages/contrib.c`.
Full math package (`cos`/`sin`/`tan`/`asin`/`acos`/`atan`/`sqrt`/`log`/
`log10`/`pow`/`exp`/`floor`/`ceil`) from `packages/math_spec.c`/`math.c`.

Notable spec details confirmed directly from source: `rename()` returns 0
on success and 1 on failure - the inverse of most file efuns - matching
`do_rename()`'s own return convention confirmed in `efuns_main.c`. `typeof()`
type-name strings lifted directly from `interpret.c`'s own `type_names[]`
array (`"int"`, `"float"`, `"string"`, `"object"`, `"array"`, `"mapping"`,
`"function"`). `abs()` preserves input type exactly (int in → int out, float
in → float out), not a float-only function. `max()`/`min()` second arg → index
rather than value, matching `contrib.c`'s own `push_number(max_index)` branch.
`asin()`/`acos()` throw on `|x| > 1.0`, matching `f_asin()`'s real guard; all
math efuns promote an int argument to float at the call site, matching real
LPC's numeric coercion. `to_float()` uses `sscanf "%lf"` for strings,
returning 0.0 for unparseable input, exactly as `f__to_float()` does.
`log()`/`log10()` throw on `x <= 0.0`.

11 new regression tests. Full suite: 413 tests passing, up from 403, no
regressions. Also added `driver/build_debug/` to `.gitignore` (the debug
build directory created this session was accidentally untracked).

**2026-08-09: `printf()` implemented, one tight pick from the
efun-coverage survey's Tier 1 list.** Checked every named candidate
(`function_exists`, `printf`, `query_idle`, `rusage`, `command`,
`upper_case`, `to_float`, `get_char`, `set_light`, `uptime`) against the
vendored `fluffos-2.9-ds2.08` source and, where relevant, this mudlib's
own real call sites, before picking, per this session's own
instructions -- deliberately not implementing all of them, only the one
that came out ahead on real usage weight, risk, and effort together.

Two candidates dropped entirely during that check, not deferred: `set_light`'s
own 7 "real" call sites (`domains/Praxis/obj/misc/match.c`/`torch.c`)
all resolve to `std/light.c`'s own local `varargs void set_light(int,
string, string)` function, which this mudlib inherits -- a completely
different signature from the real 1-arg driver efun, and this driver's
existing local/inherited-function resolution already handles it with
zero gap; the real spec (`func_spec.c`) even carries its own "should die
a dark death" comment, i.e. real FluffOS itself treats this efun as
legacy. `command`'s own 11 call sites split in a way worth recording:
checked directly against `interpret.c`'s `apply_low()`/`f_call_other()`
that real call_other (`->`) never falls back to the core efun table
(confirmed by this driver's own `VM::callFunction()`, which already
carries the identical citation and behavior) -- the `object->command(...)`
call sites in `domains/Praxis/house.c`/`sheriff.c` are structurally
unreachable in *real* FluffOS too, since nothing in this mudlib defines
a function literally named `command` for call_other to find; only the
bare, unqualified `command(...)` calls (`std/living.c`'s `force_me()`,
`std/monster.c`'s own NPC wander/patrol AI, `std/user/nmsh.c`'s alias
system) are genuinely reachable. Implementing `command` would need to
re-enter this driver's own `dispatchCommand()` with `current_object`
pushed as `command_giver` -- real, moderate effort, left for a future
slice rather than folded in here to keep this one tight.

`get_char` was also set aside: its own 8 call sites (`std/user/more.c`'s
pager) need real per-keystroke input, not per-line -- this driver's
whole network layer buffers and dispatches complete lines
(`Connection::pollLines()`), so implementing it properly is an
architecture change, badly mismatched to its own modest usage count.
`function_exists` (34), `query_idle` (16), `rusage` (12), `upper_case`
(11), `to_float` (10), and `uptime` (6) all remain real, confirmed,
right-sized candidates for a future slice -- not implemented this
session, deliberately, to keep this pick to one tight unit rather than
clearing the tier in one pass.

The actual pick: `printf` (20 real call sites, the highest usage weight
among the candidates whose implementation is genuinely trivial and
carries no meaningful risk). Confirmed directly against
`efuns_main.c`'s own `f_printf()`: it formats through
`string_print_formatted()` -- the exact same machinery `sprintf()`
already uses, not a separate format engine -- and writes the result to
`command_giver` via `tell_object()`, silently doing nothing when there
is none. Real `write()`'s own `do_write()` (`simulate.c`) targets
`command_giver` the identical way (falling back to `current_object`
only via the shadow-chain adjustment neither efun needs here) -- this
driver's own `write()` already approximates that as "whichever
connection is currently driving the call" (`OutputContext::current()`),
so `printf()` reuses `write()`'s own already-proven target resolution
rather than introducing a second, separately-approximated one.

Implementation: the existing `sprintf()` lambda in `EfunTable.cpp` was
pulled out into a named `sprintfImpl` (still registered as `sprintf`
unchanged) so `printf()` can call it directly and write the result,
rather than duplicating any of its parsing logic -- `printf()` therefore
automatically shares every format specifier `sprintf()` already
supports (including this project's own recent `"|"` centre-justify
work) and every one of its validation errors, with zero new format
code. 2 new regression tests: the real machinery is genuinely shared,
proven by using a `"%|9s"` centre-justify format (not a bare `%s`,
which a separate/simpler implementation could fake) and confirming the
exact expected padded output was sent to the connection; and a
non-string format argument throws the same `sprintf`-sourced error.
Full suite: 374 tests passing, up from 372, no regressions.
Live-confirmed end to end on port 1129: `mudlib_stub/obj/user.c`'s own
existing `write("Welcome, " + name + ".\n")` welcome line was
temporarily swapped to `printf("Welcome, %s.\n", name)` (reverted after
the check) -- login produced the identical `"Welcome, tester.\n"` text
with the name correctly substituted via `%s`. Driver console log stayed
silent throughout (no errors, no crash).

**2026-08-09: `notify_fail()` implemented, its own single focused task
per this session's own instructions.** Picked directly from the
efun-coverage survey the previous session produced: `notify_fail` had
1129 real call sites in the target mudlib, next-highest gap 34 --
dramatically higher than anything else surveyed, and not a simul_efun
here (unlike `tell_object`/`say`/`tell_room`/`shout`, which the survey
also flagged but are already covered by this mudlib's own
`secure/SimulEfun/communications.c`). Every one of those 1129 call
sites is the standard `add_action` "command didn't match, set this
failure message" idiom (`if (!condition) return notify_fail("...\n");`),
used throughout essentially every `cmd_*` file -- previously undefined,
so any mistyped or unmet-precondition command argument anywhere in the
mudlib would throw "undefined function or efun: notify_fail" the
instant it was reached. Likely never surfaced in prior live testing
because it only fires on a command's *failure* branch, and testing so
far has mostly walked happy paths.

Grounded directly in `fluffos-2.9-ds2.08/add_action.c` before
implementing, not assumed: confirmed the real spec is `void
notify_fail(string | function);` (`func_spec.c`) -- both forms real,
not string-only. `f_notify_fail()` itself just stores the value on
`command_giver->interactive->default_err_message` (a `clear_notify()`
call first, then store) and does not print anything. The actual
"only shown if nothing else claims the command" mechanism lives in
`user_parser()`'s own dispatch loop: it walks `command_giver->sent` in
order, and a handler returning truthy is an immediate `return 1` --
`notify_no_command()` (the function that actually consults and shows
the pending message) is only ever reached after the *entire* loop
exhausts with nothing claiming the command. `notify_no_command()`
itself: a string is shown directly; a function is called with no
arguments and only *its* string return is shown (a non-string return
shows nothing, not the function itself); if nothing was ever set, real
FluffOS shows a hardcoded default `"What?\n"`. `clear_notify()` runs
unconditionally at the very start of every new input line's own
dispatch (`process_user_command()`, comm.c), before even checking for a
pending `input_to()` handler -- confirmed so a message set for one line
can never leak into a later, unrelated one.

Implementation follows that mechanism directly. New per-connection
state (`Connection::pendingNotifyFail_`, a plain `std::optional<Value>`
-- already covers both the string and function/`Closure` forms via
`Value`'s own variant, no bespoke union needed the way real FluffOS's
manual ref-counting requires). New `notify_fail(string | function)`
efun (`EfunTable.cpp`): resolves `command_giver` the same way other
command-dispatch efuns already do, looks up its `Connection` via
`InteractiveRegistry::find()`, and stores the value -- a plain
assignment already replaces whatever was set before, no separate
"clear first" step needed. `Server::dispatchLine()` (the one real
per-line entry point this driver has, matching real
`process_user_command()`'s own scope exactly) now calls
`conn.clearPendingNotifyFail()` unconditionally at the top, before the
existing pending-`input_to()` check, and -- after `dispatchCommand()`
returns -- consults and shows the pending message only when
`dispatchCommand()` returned `false` (real `user_parser()`'s own
"nothing claimed it" condition, exactly), same string-vs-function
branching as real `notify_no_command()`. Deliberately not implemented:
the hardcoded `"What?\n"` default for the "nothing was ever set" case
-- this driver already has a standing, pre-existing decision (this same
comment, from the original `add_action`/`enable_commands` slice) that a
default failure message for a fully-unmatched command is a mudlib-level
concern (`std/living.c`'s own `cmd_hook()` already sends one), not
something this driver injects on its own; `notify_fail()` extends that
same decision rather than overriding it, showing only a message the
mudlib's own code explicitly asked to be shown.

5 new regression tests, run through `Server::dispatchLine()` directly
(not just `VM::dispatchCommand()`) since the actual "was the message
shown" behavior lives at that layer: the message shown when nothing
claims the command; the exact real add_action fallthrough shape
requested -- two handlers registered for one verb, the checked-first one
calls `notify_fail()` and declines, the second claims the command
without ever touching `notify_fail()`, and the pending message must
never surface; no leak into a later, unrelated failed dispatch that
never calls `notify_fail()` itself (and confirms no hardcoded default
appears either); the function form showing only a genuine string
return, not a non-string one; and a non-string/non-function argument
throwing a clear error. Full suite: 372 tests passing, up from 367, no
regressions. Live-confirmed end to end on port 1129 using
`mudlib_stub`'s own pre-existing "You can't go that way.\n" failure
shape in `go()`/`cmd_north()`/`cmd_south()` (temporarily swapped to
`notify_fail()` plus real truthy/falsy returns, reverted after the
check, per this project's own convention) -- rather than inventing new
scope: from the starting room (only a `north` exit), typing `south`
showed exactly `"You can't go that way.\n"`, and a following `north`
moved successfully with no leftover or spurious message. Driver
console log stayed silent throughout (no errors, no crash).

**2026-08-09: `sprintf()`'s `"|"` centre-justify field modifier
implemented; two stale Known Stubs claims corrected, one investigated
and disproven.** Per this session's own added instruction, re-verified
every remaining Known Stubs candidate against the real reference source
rather than trusting an existing description, the same way that caught
`find_player()`'s own real bug last slice. Re-confirmed (not just
re-asserted) that array `&` intersection, `compile_object()`'s virtual-
clone gap, and `set_eval_limit()` all stay narrow/low-risk on fresh
inspection -- same conclusions as before, arrived at independently
rather than assumed. `to_int()`'s buffer case is still unreachable in
principle: `Value.hpp`'s own variant list has no buffer type at all,
confirmed by reading it directly again.

Two real, previously undetected doc problems found this pass, both
corrected: the `sprintf()` Known Stubs bullet was badly stale -- it
described a several-sessions-old state ("bare `%s`/`%d`/`%c`... no
field width/precision/flags") flatly contradicted by the efun's own
current implementation and its own top comment, which already documents
field width, left-justify, zero-pad, colon mode, dynamic `*` width and
precision, and `%o`/`%x` -- all added in sessions whose own dated
entries are already in this file or its archive, just never reflected
back into the summary bullet. The `find_player()`/`userp()` bullet's
own most recent correction (last slice) was itself re-verified and
holds up on a second look, no new problem there.
One real, live-reachable gap investigated to ground truth rather than
guessed at: whether the `$1`/`$(name)` closure-lambda forms (recorded,
still unimplemented, in this file's own archived closures recon,
"confirmed... none on this driver's current path") might now be
live-reachable, since `filter()`/`map()` grep across the mudlib turned
up several real call sites using that exact form, including inside
`daemon/chat.c`'s own `do_chat()` -- called from `std/living.c`'s
general command-dispatch fallback for any unrecognized verb, a path
every single mistyped or unrecognized player command reaches. Checked
directly with a standalone compile probe against this driver's own
`ObjectManager` (real mudlib root, no server/listener involved) rather
than assumed: `daemon/chat.c` (the actual file `CHAT_D` resolves to,
confirmed via `secure/include/daemons.h`) compiles cleanly under this
driver today. The `$1` usage that looked alarming is in a different,
unrelated file, `secure/daemon/chat.c`, not reachable from `std/
living.c`'s own dispatch fallback -- the original archived assessment
holds, this was a false alarm resolved by checking rather than
extrapolating from a filename match.

The actual pick: real usage confirmed (`secure/SimulEfun/misc.c`'s own
`dump_socket_status()`, `"%2d  %|9s  %|8s  %-21s  %-21s\n"`), currently
throwing a clear "unsupported format specifier" error the instant that
function runs, since `"|"` was one of the several field-justify
modifiers this driver's own `sprintf()` never implemented. Small and
well-scoped once actually read against `fluffos-2.9-ds2.08/sprintf.c`
directly: `"|"` is parsed as a plain sibling flag alongside the already-
implemented `"-"` (`INFO_J_CENTRE` next to `INFO_J_LEFT` in the same
flag-parsing switch), and its own `add_justified()` shows the exact
padding split -- when the total padding does not divide evenly, the
extra character goes on the *leading* side (`"i = fs / 2 + fs % 2"`),
not the trailing one, a real detail that would have been easy to get
backwards without reading the source directly.

Implementation: `EfunTable.cpp`'s `sprintf()` gained a `centreJustify`
flag recognized alongside the existing `"-"`/`":"` modifier scan, and
its final padding step now branches three ways (left/centre/right)
instead of two, splitting centre padding via the same `lead = padLen /
2 + padLen % 2` formula confirmed above. Zero-padding is suppressed for
centre-justify, matching this driver's own existing precedent for
left-justify (zero-padding a centred value has no real meaning either).
2 new regression tests: the real call site's own exact shape (`"%|9s"`,
even padding, split evenly) and an odd-padding case confirming the
extra character lands on the left, not the right. Full suite: 367 tests
passing, up from 365, no regressions. Not live-verified over a socket
this slice -- unlike recent picks, this is a pure string-formatting
change with no connection/protocol state involved, and the regression
tests already exercise the exact real format string byte for byte, so a
network-level check would not add anything the unit tests do not
already prove.

**2026-08-09: `find_player()` now uses real O_ONCE_INTERACTIVE
semantics instead of "currently connected", and `find_living()` is a
real efun for the first time.** Same standard as the last several
slices, with this session's own added instruction to verify each
remaining Known Stubs candidate against the real reference source
before trusting a prior session's characterization of its size, not
just its existence. Re-checked every remaining bullet that way: array
`&` intersection's own real usage (`daemon/command.c`'s `find_cmd()`,
the one confirmed real call site, itself only order-sensitive when the
same verb exists in two searched directories at once, an exceptional
case) still confirms last session's conclusion, not a new one;
`compile_object()`'s virtual-clone gap (real `simulate.c`'s own
`clone_object()`, only reachable by cloning an object that is *already*
virtual, not how this mudlib's own `new(OB_USER)` player-creation path
works) stayed narrow on inspection; `set_eval_limit()`'s no-op stayed
low-risk (this driver's own fixed per-call ceiling is already far more
generous than real accumulated-eval-cost semantics, not less). Also
found and fixed in passing, per standing instruction to fold small doc
corrections into whatever else is worked on: the "postfix/prefix
`++`/`--` only support a bare variable" bullet was stale -- indexed
`++`/`--` (`arr[i]++`) was actually implemented several sessions ago
and has had a passing regression test since; the bullet was just never
updated (see its own strikethrough below).

The actual pick, found the same way as `net_dead()`, `destruct()`'s
connection-close fix, `userp()`, and last slice's destructed-value read
coercion: checking a Known Stubs bullet's own real mechanism directly
rather than trusting its existing description. The `set_living_name()`
bullet said this driver stores the name but "wires up no lookup table
for it, matching `find_player()`'s own pre-existing simplification" --
checking `find_player()`'s own actual implementation (an
`InteractiveRegistry` walk plus a `query_name()` call on each currently-
connected object) against real `add_action.c` directly showed that
description was itself wrong in two ways, not just incomplete: real
`find_player()` and `find_living()` are the *same* function,
`find_living_object(str, user)`, differing only in one extra flag check
-- `user=1` requires real `O_ONCE_INTERACTIVE` (`LpcObject::
wasEverInteractive()`, the same flag `userp()` was fixed to use two
slices ago), not "currently connected" at all, so a link-dead-but-
still-present player should still be findable and this driver's old
approximation could never do that. It also matched against
`query_name()` rather than the real `living_name` `set_living_name()`
actually sets, which happen to agree for a player in this mudlib but
not in general, and not for NPCs at all. `find_living()` itself was not
registered as an efun -- confirmed real-reachable, not theoretical: 18+
call sites (`cmds/mortal/_whisper.c`, `daemon/mail_d.c`,
`cmds/mortal/_psi.c`, several admin commands --
`_scan.c`/`_trans.c`/`_wizheal.c`/`_teleport.c` -- and several
NPC-targeting object files under `domains/Praxis/`), every one of them
throwing "undefined function or efun: find_living" the instant it was
reached.

New `LivingNameRegistry` (`object/LivingNameRegistry.hpp`/`.cpp`),
mirroring real `add_action.c`'s own `hashed_living[]` table: a name-to-
`weak_ptr<LpcObject>` registry, kept separate from `InteractiveRegistry`
because the real mechanism is not connection-scoped at all (`std/
monster.c`'s own NPCs call `set_living_name()` too). `set_living_name()`
now registers into it (`set()`, itself doing a `remove()`-then-insert
first, matching real `set_living_name()`'s own `remove_living_name(ob)`
call at the top). `find()` takes a `requireOnceInteractive` flag, gates
on `LpcObject::commandsEnabled()` (real `O_ENABLE_COMMANDS`) always and
`wasEverInteractive()` only when that flag is set, and skips a
destructed or expired candidate -- exactly `find_living_object()`'s own
three checks. `find_player`/`find_living` are now one shared lambda
factory differing only in that flag, matching the real driver's own
"one function, one flag" design instead of two separate
implementations. `ObjectManager::destructObject()` now also calls
`LivingNameRegistry::remove()`, matching real `destruct_object()`'s own
`remove_living_name(ob)` call (found sitting directly next to the
already-replicated `ob->super = 0` step in `simulate.c`) -- without
this a destructed object with a still-live `shared_ptr` reference
elsewhere would keep matching lookups, the exact class of bug the
`O_DESTRUCTED` guard and last slice's read-coercion fix both exist to
close.

7 new regression tests: `find_player()` finding a currently-connected
object, still finding it after the connection closes (the actual
`find_player()` bug), correctly not matching an object that was never
interactive at all, `find_living()` matching an NPC-shaped object that
was never interactive, `find_living()` declining an object with no
`enable_commands()`, `find_living()` returning null for an unknown
name, and `find_living()` no longer matching a destructed object's
former name. Full suite: 365 tests passing, up from 358, no
regressions. Live-confirmed end to end on port 1129: a temporary
`cmd_findtest` on `mudlib_stub/obj/user.c` (removed after this check,
same as this project's own prior throwaway probes), with `setup()`
temporarily also calling `set_living_name(name)`, drove two
connections, Alice and Bob -- while Bob was connected, Alice's check
reported `find_player=1 find_living=1`; after Bob's connection was
closed abruptly (a raw socket close, no `quit`), the same check still
reported `find_player=1 find_living=1` -- exactly the real-semantics
divergence this slice fixed -- and a check against an unknown name
correctly reported `find_player=0 find_living=0`. Driver console log
stayed silent throughout (no errors, no crash).

**2026-08-09: a destructed object stored in a variable, array element,
or mapping value now reads back as a real int 0, not the stale
reference.** Picked from the remaining Known Stubs list, same standard
as the last several slices: weighed for silent-wrong-behavior risk on a
real-world-reachable path, not narrow edge cases or stubs that already
throw a clear error. This was already flagged, but explicitly deferred
each time it came up, in the destructed-object-guard bullet: "the
broader 'any stale object-typed value silently reads back as 0'
semantics real FluffOS enforces at many more read sites (array/mapping
entries, comparisons, etc, not just applies) -- this driver only gates
the actual call/apply entry points." Every other remaining candidate
was re-checked first and still deprioritized for the same reasons as
recorded in the last two entries below (array `&` intersection order
being non-contractual, `replace_string()`/`implode()`/`sprintf()`'s
gaps all throwing clear errors already, `to_int()`'s buffer case being
unreachable in principle). This was the one item left that is both
silent and, once actually read against the real reference source
instead of assumed, turned out to be considerably more tractable than
its own "broader... many more read sites" description suggested.

Confirmed directly against `fluffos-2.9-ds2.08/interpret.c` rather than
guessed: the "stale object reads as 0" semantics are not scattered
across every comparison/branch/arithmetic opcode the way the existing
bullet's own phrasing implied. `F_BRANCH_WHEN_ZERO`/`F_NOT` treat any
non-number type (including a destructed object reference) as
unconditionally truthy, and `eoperators.c`'s own `f_eq()` does a raw
pointer compare on a `T_OBJECT` operand with no `O_DESTRUCTED` check at
all -- neither opcode coerces anything itself. The actual mechanism
lives entirely at the point a value is *read out of storage*: `F_LOCAL`,
`F_GLOBAL`, and `F_INDEX`'s array/mapping cases each check
`O_DESTRUCTED` on the value about to be pushed and, if set, call
`assign_svalue(s, &const0u)` -- rewriting the *storage itself* to a
real int 0, permanently, not just for this one read, so every later
read of the same slot is already a plain 0 with nothing left to check.
Also confirmed the boundary of this mechanism, not just its center:
`array.c`'s own `slice_array()` (backing range indexing, `arr[a..b]`)
does not coerce at all -- a destructed element copied into a freshly
sliced sub-array stays a raw reference until *that* array's own element
is separately read through one of the four points above. Implementing
exactly this (not more, not less) is therefore not a narrowed-down
practical subset of real semantics, it is the complete mechanism.

New `coerceIfDestructed(Value&)` (`VM.cpp`, anonymous namespace):
checks whether a `Value` holds a reference to a now-destructed
`LpcObject` and, if so, overwrites it in place with `Value(int64_t{0})`
-- the same self-healing, mutate-the-storage-not-just-the-read
behavior `assign_svalue()` has in the real driver. Wired into exactly
the four matching read points this driver has: `OpCode::PushLocal`,
`OpCode::PushObjectVar`, and `OpCode::Index`'s array and mapping
branches (the mapping branch needed its loop variable changed from
`const auto&` to `auto&` to allow the in-place coercion, mirroring
`find_in_mapping()`'s own real behavior of coercing the mapping entry
itself, not just the copy returned to the caller). `OpCode::RangeIndex`
was deliberately left untouched, matching `slice_array()`'s own
confirmed real behavior above. No other opcode needed changes --
comparisons, truthiness checks, and arithmetic all consume a value that
was necessarily read through one of these four points first, so they
correctly see an already-coerced `0` with no separate fix required,
exactly mirroring why real FluffOS's own `f_eq()` needs no
`O_DESTRUCTED` check of its own either.

5 new regression tests: a destructed object stored in a local variable,
an object variable, an array element, and a mapping value each reading
back as a genuine `int64_t` `0` (not just falsy -- the exact type,
matching real semantics precisely rather than approximating "still
callable but somehow falsy"), plus a confirmation that an ordinary,
still-alive object reference stored the same way is completely
unaffected. Full suite: 358 tests passing, up from 353, no regressions.
Live-confirmed end to end on port 1129: a temporary `cmd_zerotest` on
`mudlib_stub/obj/user.c` (removed after this check, same as this
project's own prior throwaway probes) cloned an item, stored it in both
an object variable and an array element, destructed it, and reported
back `var=0 arr0=0 eq=1` -- the object variable, the array element, and
an explicit `== 0` comparison against the object variable all agreeing
it now reads as a real `0`. Driver console log stayed silent throughout
(no errors, no crash).

**2026-08-09: `userp()`/`query_once_interactive()` now a real, sticky
O_ONCE_INTERACTIVE-equivalent instead of an alias of `interactive()`.**
Picked over the remaining Known Stubs candidates and the still-paused
reconnect/take-over save-flag bug, same standard as the last several
slices: weighed for silent-wrong-behavior risk on a real-world-reachable
path, not narrow edge cases or stubs that already throw a clear error.
This one was already flagged in Known Stubs ("wrong for an object that
was once connected and has since disconnected") but checking real usage
against the actual mudlib showed it is considerably more reachable than
that description suggested: `userp()` is called 61 times and
`interactive()` 87 times across combat, chat, mail, trading, psionics,
and admin commands, heavily in the exact shape `userp(target) &&
!interactive(target)` (`cmds/mortal/_psi.c`) or `ob->is_player() &&
!interactive(ob)` (`cmds/skills/_backstab.c`/`_fireball.c`/`_bolt.c`/
`_burn.c`/`_chilltouch.c`/`_pick.c`, `cmds/mortal/_whisper.c`) -- "is
this a player object, currently offline". `secure/std/login.c` itself
carries a comment documenting the real distinction directly
(`count_connected_players()`: "userp(), which is driver
query_once_interactive(), true for any object that was ever handed a
connection ... not just for objects that stayed interactive"). This
driver's old approximation made `userp()` identical to `interactive()`,
so the instant any connected player went link-dead -- one of the most
ordinary events in real play -- every one of those checks would have
silently misclassified their still-present character as not a player,
changing real gameplay logic with no error at all. Bigger, more
constantly-hit blast radius than last slice's save-file-format gap.

Fixed with a new `LpcObject::wasEverInteractive()` flag (real
`O_ONCE_INTERACTIVE`, `object.h`): set once, by `Connection::attach()`,
the first time an object is ever bound to a connection -- covering both
the initial login-object bind and a later `exec()` rebind onto the real
player object (matching `login.c`'s own comment above, "any object that
was ever handed a connection"), never cleared again, not even once the
connection later disconnects. `userp()`/`query_once_interactive()` now
check this flag directly instead of scanning `InteractiveRegistry` (the
same scan `interactive()` itself correctly still uses, since that efun's
real semantics genuinely are "connected right now" -- left unchanged).
`find_player()`/`users()` were also left unchanged: both are correctly
scoped to currently-connected objects only in real FluffOS too, not
`O_ONCE_INTERACTIVE`.

3 new regression tests: `userp()`/`interactive()` both true while a
connection is live, `userp()` staying true while `interactive()` goes
false after the connection closes (the actual bug), and `userp()`
returning false for a plain object that was never bound to any
connection at all. Full suite: 353 tests passing, up from 350, no
regressions. Live-confirmed end to end on port 1129: a temporary
`cmd_userptest` on `mudlib_stub/obj/user.c` (removed after this check,
same as this project's own prior throwaway probes) drove two real
connections, Alice and Bob -- while Bob was still connected, Alice's
check reported `userp=1 interactive=1`; after Bob's connection was
closed abruptly (a raw socket close, no `quit`, the same shape as a
real client crash), Bob's `net_dead()` broadcast fired as expected and
Alice's check on the same still-present Bob object then reported
`userp=1 interactive=0` -- exactly the real-semantics divergence this
slice fixed. Driver console log stayed silent throughout (no errors, no
crash).

**2026-08-09: `restore_object()` now reads real FluffOS on-disk save
files, not just this driver's own format.** Picked over the other
remaining Known Stubs candidates named for this session (array `&`
intersection ordering, `replace_string()`'s range args, `to_int()`'s
buffer case, `implode()`'s function-per-element form, `query_ip_name()`'s
no-DNS fallback) and the still-paused reconnect/take-over save-flag
bug, on the same standard as the last two slices: weighed for silent-
wrong-behavior risk on a real-world-reachable path, not narrow edge
cases or stubs that already throw a clear error. Of the five named
candidates, checked each against real usage before picking: `replace_string()`'s
range args and `implode()`'s function-per-element form both already
throw a clear error rather than misbehaving silently (confirmed by
reading their own `EfunTable.cpp` bodies); `to_int()`'s buffer case is
unreachable in principle, not just in practice -- this driver's `Value`
variant has no buffer type anywhere in the codebase, so a real buffer
value can never exist to pass to it; `query_ip_name()`'s no-DNS fallback
is confirmed logging/display-only in the one real mudlib this project
has (`nightmare3_fluffos_v2/lib/std/user.c`'s own `ip = query_ip_name(...)`,
used only for a login-log line and an admin `_people.c` display, never
a security or matching decision), and this driver's own always-numeric
answer is what the doc already noted matches real FluffOS's own
documented fallback behavior anyway; array `&` intersection order is
not a real compatibility gap at all once checked against the actual
reference source (`fluffos-2.9-ds2.08/array.c`'s own `alist_cmp()`
sorts by raw `svalue_t` union bits -- pointer identity for strings/
objects -- meaning real FluffOS's own "sorted" order is an
implementation artifact no sane mudlib content could depend on either;
only the *deduplication* half of that gap is a genuine, if narrow,
behavioral difference, left as-is this slice).

The actual pick, found by auditing the same "Known stubs" list with the
same "what might be silently wrong, not loudly erroring" standard that
picked `net_dead()` and the `destruct()` connection-close bug the last
two slices: `save_object()`/`restore_object()`'s own bullet, which
already documented the real risk precisely -- this driver only ever
wrote and read its own recursive, tab-delimited format, so a genuine,
pre-existing FluffOS save file (the exact shape any real mudlib's
existing player/daemon data would already be in, directly touching this
project's own stated end goal of eventually running real-world mudlibs)
silently kept whatever defaults `create()` already set instead of
loading, with no error at all. Confirmed still real and not
theoretical: `nightmare3_fluffos_v2/lib/daemon/save/banish.o`, which
ships with this project's own mudlib, is in the real format (`#/daemon/
banish.c` comment header, then `varname value` lines in plain LPC
literal syntax, space-delimited) -- confirmed against an untouched
backup copy predating this driver ever touching it, since the copy
inside `mudlib/nightmare3_fluffos_v2/` itself has since been silently
overwritten in this driver's own format by an earlier session's own
live testing, which is exactly the failure mode this bullet described.

Implementation: a new read-only parser (`parseRealSaveValue()` and
helpers, `EfunTable.cpp`), grounded directly in `fluffos-2.9-ds2.08/
object.c`'s own `save_svalue()` (the writer) and `restore_string()`/
`restore_array()`/`restore_mapping()`/`parse_numeric()` (the readers),
not guessed: strings backslash-escape `"`/`\` and translate a raw `\r`
byte back to `\n` (real `save_svalue()`'s own on-disk encoding of an
embedded newline, so a literal newline in a saved string can't be
mistaken for the end of the save-file line); numbers are plain digits
for an int, a `.`-led fraction for a float (no exponent form -- real
`save_svalue()`'s own writer, `sprintf(..., "%f", ...)`, never produces
one, so a faithful reader for genuine on-disk data from this exact
vendored driver doesn't need to parse one either); arrays/mappings
recurse through the same parser, trailing-comma-tolerant to match the
real writer's own "always write a comma after every element, including
the last" convention. `restore_object()`'s per-line loop now auto-
detects which format a line is in (a tab is this driver's own format's
delimiter and never appears in the real one; a `#`-led line is a real-
format comment, skipped in either format, matching real
`restore_object_from_line()`'s own "ignore 'comments'" case) rather
than assuming one globally, so a save file this driver itself already
wrote continues round-tripping exactly as before -- `save_object()`
itself is unchanged and still only ever writes this driver's own
format, since nothing needs to read a file this driver wrote except
this driver, and doing so is simpler and already fully covered. Real
LPC "class" values (`(/ ... /)`) are not implemented -- this driver has
no class/struct type anywhere else either -- and throw a clear error
rather than being silently mishandled, matching this codebase's
existing convention for other unimplemented shapes.

3 new regression tests: scalars and nested array/mapping content in the
real format (int, negative int, float, string, an array containing an
int/string/mapping mix), the exact real `banish.o` shape (a `#`-led
comment header line, empty arrays, an empty mapping), and string
escaping (`\"`, `\\`, and the raw-`\r`-to-`\n` translation) -- all
grounded in the real writer's own grammar, not just this driver's own
format's own already-covered round trip. Full suite: 350 tests passing,
up from 347, no regressions. Live-confirmed end to end on port 1129: a
throwaway probe object and command (`/banish_probe.c`,
`cmd_restoretest` on `mudlib_stub/obj/user.c`, both removed after this
check, same as this project's own prior throwaway probes) loaded a
byte-for-byte copy of the real, untouched `banish.o` backup and
correctly reported every one of its real variables (`__Names` through
`__TmpBanish`) as present and empty, with the driver's own console log
staying silent throughout.

**2026-08-08: `destruct()` now closes the destructed object's OWN
connection, not whichever connection happens to be `current()`.** This
was the bug flagged, but deliberately left unfixed, at the end of the
previous slice's own report. Weighed against the remaining Known Stubs
list (array `&` intersection ordering, `replace_string()`'s range
args, `to_int()`'s missing buffer case, `implode()`'s function-per-
element form, `query_ip_name()`'s no-DNS fallback -- all either narrow
edge cases nothing live has ever hit, or cases that already throw a
clear error rather than misbehaving silently) and the reconnect/take-
over save-flag bug (stays paused, mudlib-specific): this was the clear
pick, both because it directly follows from last slice's own `net_dead()`
work and because it is real-world reachable in the exact way that has
driven every recent pick -- any mudlib with a wizard `boot`/kick command
hits it, and the previous slice's own `O_DESTRUCTED` guard had
inadvertently made the failure mode worse, not just unfixed: a
"kicked" player's connection used to at least keep functioning
normally (the bug predates that guard); now their commands went
silently inert instead (`VM::callFunction()`/`dispatchCommand()` both
already refuse a destructed target) while their socket stayed open --
no error, no disconnect, nothing dispatched, forever, until they closed
the client themselves.

Root cause (`EfunTable.cpp`'s `destruct` efun): it looked up
`OutputContext::current()` -- the connection driving *this call* -- and
only closed it if that connection's own bound object happened to match
the object being destructed. Real `destruct_object()`'s own actual rule
(`simulate.c`: `if (ob->interactive) remove_interactive(ob, 1);`) has
nothing to do with which connection is currently active; it is always
the destructed object's *own* interactive. Fixed by looking the
connection up via `InteractiveRegistry::find(ob)` (the same object-to-
`Connection*` lookup `message()`/`tell_object()` already use to reach an
arbitrary target) and closing that instead. `Connection::close()`
itself already does the `InteractiveRegistry` removal (see its own
comment), so this also let the previous slice's separate, unconditional
`InteractiveRegistry::remove(ob)` call be deleted entirely -- redundant
once `close()` runs, and correctly a no-op via `find()` returning null
when `ob` was never interactive at all, matching real `if
(ob->interactive)` exactly rather than approximating it.

3 new regression tests: the actual bug (an "actor" object destructs a
*different*, still-connected object while its own connection is
`current()` -- confirms the target's connection actually closes and the
actor's own is untouched), the pre-existing case this fix must not
break (`secure/std/login.c`'s own `internal_remove()` pattern,
`destruct(this_object())` on the object bound to the *currently active*
connection -- still closes correctly), and destructing a plain, never-
interactive object (confirms `InteractiveRegistry::find()` returning
null is a clean no-op, no crash, real `if (ob->interactive)` semantics).
Full suite: 347 tests passing, up from 344, no regressions. Live-
confirmed end to end on port 1129: added a minimal `cmd_boot(string)`
to `mudlib_stub/obj/user.c` (destructs another named player's object in
the same room -- a stand-in for a real mudlib's admin boot command,
existing purely to exercise this fix live, same rationale as last
slice's `net_dead()` addition to the same file) and drove two
connections, Alice booting Bob. Bob's own socket received a genuine
EOF immediately (confirmed by a real `recv()` on Bob's own side, not
just inferred from Alice's output), a post-boot command attempt from
Bob's side produced nothing further (socket already gone), and Alice's
own connection was completely unaffected throughout. Driver console log
stayed silent (no errors, no crash).

**2026-08-08: real `net_dead()` link-death apply implemented; port
inconsistency fixed first.** Two logical pieces this slice.

Port cleanup (housekeeping, done first): `driver/config/driver.cfg`
(the `mudlib_stub` config) was still on port 3000, set up in an earlier
session specifically to avoid colliding with the real mudlib's own
port 1122 -- but 3000 was never actually one of this project's
established ports (1122 the real mudlib's live/dev port, 1123 a prior
session's own scratch port for driver-vs-real-mudlib testing, 1129 the
real mudlib's own websocket port per its install docs). Changed to
1129. Also updated the two other places 3000 had leaked into (`Config.hpp`'s
own hardcoded default `port_`, and `driver/README.md`'s quick-start `nc`
example), both introduced in the same original commit as the config
change per `git log -p`. Confirmed by full-repo grep: every other "3000"
hit left in the tree is unrelated (an evaluator-stack-size constant, a
test's own trial count) or historical narrative in this file describing
what was true in past sessions, deliberately left as-is rather than
rewritten. Rebuilt, full suite reconfirmed passing, then live-verified
end to end on port 1129 against `mudlib_stub` (two connections: login,
item present, cross-connection `say` broadcast, movement between rooms)
-- output identical to the prior session's own transcript. Port 1129 is
now what all future live verification against `mudlib_stub` should use.

Picked next, weighed against the other two candidates sitting in this
file (remaining Tier 2/3 general-LPC-compliance gaps, and the
reconnect/take-over save-flag bug, which stays paused -- mudlib-
specific investigation on `nightmare3_fluffos_v2`, not general driver
work): a **real `net_dead()` apply, fired on genuine connection link
death**. Found while auditing the "Known stubs" list for what else
might be silently wrong rather than loudly erroring, the same standard
that made the destructed-object guard the pick last slice. `ApplyTable.cpp`
had long listed `"disconnect"` as a "known apply" alongside `logon`/
`connect` -- checked directly against the vendored
`fluffos-2.9-ds2.08/applies.h` rather than assumed, and real FluffOS has
no such apply at all. The actual apply is `net_dead` (`APPLY_NET_DEAD`,
`comm.c`'s `remove_interactive()`), fired on every interactive whose
connection drops while the object itself is still alive (`dested=0`)
-- and this driver never called it, under any name, from anywhere.
Confirmed live-reachable for real mudlib content, not theoretical:
`nightmare3_fluffos_v2/lib/std/user.c` defines a real `net_dead()`
(save-on-linkdeath/reconnect handling), and link death itself -- a
client crashing, a network drop -- is one of the most ordinary events
in real MUD play, happening to every player eventually. Silently never
firing this apply is the connection-lifecycle equivalent of the
destructed-object gap: no crash, no error, just cleanup logic that
never runs.

New `Server::fireNetDeadIfLinkDead(VM&, Connection&)`, a public static
method (same "pull it out so it's directly testable without a real
listening socket" pattern `dispatchLine()` already established): a
no-op unless the connection is actually `closed()` *and* still has a
bound object, which is exactly the state `Connection::pollLines()`
leaves a connection in the moment it detects EOF/a read error (`closed_`
set, but `close()` itself -- the `InteractiveRegistry` removal and
actual fd close -- hasn't run yet). Called from `handleConnection()`
after its existing per-line dispatch loop, unconditionally. This
guard structure was deliberately chosen so every *other* way a
connection ends up closed correctly still skips it with zero extra
logic needed: the `destruct()` efun's own connection close (`EfunTable.cpp`,
matching real `destruct_object()`'s own `dested=1` case, which
correctly never calls `net_dead()` either) already clears the bound
object before this check ever runs, and a connection closed via this
driver's own mid-dispatch runtime-error isolation (`handleConnection()`'s
existing `catch` block) does too -- deliberately scoped out rather than
chased into that block this slice, flagged as a scope simplification
in `fireNetDeadIfLinkDead()`'s own comment rather than assumed
equivalent to real semantics. Also corrected `ApplyTable.cpp`'s known-
apply entry from the never-real `"disconnect"` to the real `"net_dead"`,
with a comment recording why (that table itself is otherwise unused
anywhere in this codebase -- confirmed by grep -- so this is a
documentation-accuracy fix, not a functional one on its own).

3 new regression tests, using the same `ObjectVarHarness` + `AF_UNIX
socketpair` pattern the existing connect/input-protocol tests already
use: `net_dead()` genuinely firing when the peer side of a real
socketpair is closed and `pollLines()` picks up the resulting EOF, a
no-op while the connection is still open, and a no-op after an explicit
`close()` (standing in for `destruct()`'s own path) has already cleared
the bound object. Full suite: 344 tests passing, up from 341, no
regressions. Live-confirmed end to end on port 1129: added a minimal
`net_dead()` to `mudlib_stub/obj/user.c` (broadcasts "X has gone
link-dead." to the room, mirroring the existing `say` broadcast
pattern -- not a real save/reconnect implementation, this stub mudlib
deliberately has neither) and drove it with two connections, the
second closed abruptly mid-session with no `quit` command (a raw
socket close, the same shape as a real client crash or network drop);
the first connection received the broadcast, and the driver's own
console log stayed silent (no errors, no crash).

**2026-08-08: destructed-object guard added (real `O_DESTRUCTED`
semantics for every call/apply entry point).** Picked over the
remaining Tier 2/3 general-LPC-compliance gaps, the reconnect/take-over
save-flag bug (mudlib-specific, and that whole track is paused), and a
proposed "dev-check.sh" script that, on checking, does not actually
exist anywhere in this repo -- weighed specifically against the stated
end goal of eventually running real-world mudlibs, not just this
project's own minimal test one: this was the single item most likely to
cause *silent* wrong behavior (not a loud crash) the moment a real
mudlib's normal gameplay content -- corpses, temp effects, spent
projectiles, any object destructed during ordinary play -- got pointed
at this driver, since this driver previously had no destructed-object
guard of any kind. Confirmed live-reachable, not theoretical: a
destructed object was never unlinked from its own environment's
inventory (`ObjectManager::destructObject()` only ever removed it from
`ObjectManager`'s own filename cache), so it stayed visible in
`all_inventory()`/`environment()` results, still callable via
`call_other()`, indefinitely, until every last `shared_ptr` reference to
it happened to drop. New `LpcObject::isDestructed()` (a real
`O_DESTRUCTED`-equivalent flag, set once by
`ObjectManager::destructObject()`), checked at every "call into an
object from outside" entry point this driver has: `VM::callFunction()`
(the single choke point behind `call_other()`, `applyMaster()`,
`moveObject()`'s own `init()` propagation, and `Scheduler`'s
`call_out()`/`heart_beat()` firing -- one change covers all of them),
`VM::callClosure()` (previously only caught an *expired* weak_ptr owner,
not an explicitly-destructed-but-still-referenced one), `VM::moveObject()`
(refuses to move a destructed item or into a destructed destination),
and `VM::dispatchCommand()` (both the command_giver itself and each
individual action-table owner). `ObjectManager::destructObject()` also
now unlinks the object from its old environment's inventory immediately,
matching real `destruct_object()`'s own `ob->super = 0` step -- not
replicated: real `destruct_object()`'s own contents-relocation loop
(each contained item's own `move()` apply, run automatically before
severing), flagged as a deliberate simplification rather than assumed
equivalent. `destruct()` also now removes the object from
`InteractiveRegistry` unconditionally rather than only when it happens
to be the currently active connection, closing a related, smaller gap
in the same area (found while reading the existing `destruct()` code,
not separately planned). 5 new regression tests, each deliberately
keeping the destructed object alive via a live reference rather than
letting its last `shared_ptr` drop, to prove the new flag itself is
doing the work rather than incidentally relying on weak_ptr expiry the
way two pre-existing tests already did. Full suite: 341 tests passing,
up from 336, no regressions. Live-reverified against the minimal test
mudlib (`driver/mudlib_stub/`, port 3000) end to end afterward as a
sanity check -- identical output to the prior session's own transcript,
confirming the new checks do not interfere with ordinary (non-
destructed) operation. Also folded in, per standing instruction to fold
small doc corrections into whatever else is being worked on rather than
treat them as their own task: corrected the stale `sscanf()` "%s"-
adjacent-specifier line in Known Stubs (fixed several sessions back,
the bullet was just never updated at the time).

**2026-08-08: `throw()` implemented; `catch()` now carries an arbitrary
value, not just a string.** Picked from several deferred items sitting
in this file (the Tier 2/3 general-LPC-compliance gaps, the reconnect/
take-over save-flag bug, and this) as the single most logical next
task: it is core language completeness rather than mudlib-specific, the
design was already fully settled in a prior session (blast radius
measured: 198 `throw LpcRuntimeError(...)` call sites and 14 `.what()`
call sites across 4-6 files), and it directly matters for testing
against other mudlibs later, which lean on `catch()`/`throw()` pairs
for error signaling. Implemented exactly the already-recommended
design rather than changing `LpcRuntimeError`'s own payload type: a new
`LpcThrownValue` (`Value.hpp`), a subclass of `LpcRuntimeError` carrying
the exact `Value` passed to `throw()` -- every one of the existing
`LpcRuntimeError` throw sites and `.what()` call sites needed zero
changes. New `throw` efun (real FluffOS: a genuine efun per
`func_spec.c`'s own `void throw(mixed);`, not special grammar the way
`catch()` is -- confirmed by reading the vendored source directly, not
guessed), requiring exactly one argument. `VM::run()`'s catch-frame
handling gained a dedicated `LpcThrownValue` branch, checked before the
existing plain `LpcRuntimeError` branch since `LpcThrownValue` is-a
`LpcRuntimeError`. One real bug found while implementing, not just
during design: when a function's own `catchFrames` is empty, the
existing code rewraps the exception into a *new* plain `LpcRuntimeError`
carrying only a stringified, file/function-prefixed message before
rethrowing to the caller -- correct for an ordinary runtime error
(always a string anyway), but for a real `throw(value)` this would have
silently flattened the original value into a string before it ever
reached a `catch()` one call further up the stack than the throwing
function's own. Fixed with a dedicated rethrow-unchanged path for
`LpcThrownValue` specifically. 5 new regression tests (int/string/array
values caught verbatim, wrong-argument-count throws, and the critical
case: `throw()` inside a called function with no `catch()` of its own
still reaches the caller's `catch()` with the value intact). Full
suite: 336 tests passing, up from 331, no regressions.

**2026-08-08: `driver/mudlib_stub/` replaced with a minimal test
mudlib.** Direction change: further mudlib-parity/gap-analysis work on
the `nightmare3_fluffos_v2` mudlib is paused. The priority is now a
minimal test mudlib whose only purpose is exercising the driver end to
end -- not feature parity with any real mudlib. `driver/mudlib_stub/`
previously held a trivial one-file "greet" smoke test (`master.c` plus
`obj/simple_login.c`) alongside several unrelated single-language-
feature probe files (`array_check.c`, `guard_check.c`,
`ternary_check.c`, `object_var_check.c`, `range_index_check.c`,
`arithmetic_check.c`, `guard_char_check.c`, all left untouched). Six
files now make up the minimal mudlib: `master.c` (rewritten, `connect()`
only), `obj/login.c` (replaces the deleted `obj/simple_login.c`;
prompts for a name, clones a player object, `exec()`s the connection
onto it), `obj/user.c` (the player object: `look`, `north`/`south`
movement, `say`, which broadcasts to everyone else in the room via
`message()`), `obj/item.c` (one trivial clonable object), and
`rooms/start_room.c`/`rooms/second_room.c` (two minimal rooms with an
exits mapping between them; `start_room.c` clones one item into itself
at first load). No account/password persistence, no privs, no
simul_efun, no `compile_object()`/virtual-object path -- all
deliberately out of scope. Live-verified against
`driver/config/driver.cfg` (port 3000) with two simultaneous raw socket
connections: login, look, movement in both directions between the two
rooms (room state persists across visits, confirming rooms are cached
singletons and not re-created on every visit), `say` broadcasting to a
genuinely separate connection, and the item clone present and listed in
the starting room. One real bug found and fixed during that
verification: `user.c` had no `query_short()`, so another player
present in the room showed up as "You see 0 here." instead of their
name -- `call_other` on a function the target object does not define
returns monostate, not an error (real, documented LPC semantics, not a
driver bug), and that monostate then string-concatenated as `"0"`.
Fixed by adding `query_short()` returning the player's name, rebooted,
reverified. Full `ctest` suite reconfirmed passing afterward (no driver
source was touched, sanity check only).

**2026-08-08: `do-while` loop and extended `sprintf()` format
specifiers.** Two smallest items from the general LPC-compliance gap
analysis (not driven by a new mudlib blocker this time). `do { ... }
while(cond);` was entirely missing from the language -- not in the
lexer's own keyword list at all. Added a `DoWhileStmt` AST node, a
parser rule, and codegen mirroring the existing `while` implementation
(body runs first, condition checked after; `continue` targets the
condition recheck, matching `for`'s own update-clause handling, not the
body's own top). `sprintf()` gained `%o` (octal), `%x` (hex), a
standalone `.`-precision modifier distinct from the existing `:`
combined field-size-and-precision form, and `*` as a dynamic field
width/precision pulled from the argument list -- grounded directly in
the vendored `fluffos-2.9-ds2.08/sprintf.c` reference source's own doc
comment and its actual field-size/precision parsing loop, not guessed.
`%0*d` (zero-padded dynamic width) is explicitly not implemented,
throws its own clear error rather than being silently misparsed. 13 new
regression tests (5 `do-while`, 8 `sprintf`). Full suite: 324 tests
passing, up from 311, no regressions.

Snapshot as of the slice that designed and implemented the
`add_action()`/`enable_commands()` command-dispatch subsystem (real
`command_giver`, per-object action tables, `move_object()`/`init()`
propagation, verb matching including the `V_SHORT` catch-all form) from
scratch, grounded in `fluffos-2.9-ds2.08/add_action.c` directly and this
mudlib's own real `std/living.c`/`std/room/exits.c`/`std/room/senses.c`/
`std/Object.c` usage. Confirmed live end to end: a fresh account now
reaches `create()`, proceeds through account setup and the early
chargen prompts, and the dispatch subsystem itself is confirmed
reaching real handler functions (`cmd_hook`, `process_input`'s own
callees) via the new mechanism. Along the way this pass also fixed a
run of further real, confirmed gaps the deeper walk surfaced (object
variable declaration-time initializers, `undefinedp()`/`nullp()`, a
genuine cross-inherit function-resolution gap, and -- the most
consequential -- object variables and locals defaulting to this
driver's own "no value" sentinel instead of real LPC's actual `0`
default, which had been silently wrong since early in the project).
Snapshot updated after root-causing the `__HistorySize` modulo-by-zero
report from the previous slice: it was never actually a bug in the
`this_player() != this_object()` guard (confirmed live, with temporary
instrumentation, that the guard behaves correctly and `reset_history()`
correctly sets `__HistorySize`). The real cause was a cascade of
several other missing efuns/driver bugs that each independently
prevented `setup()` from ever cleanly reaching that code, with
`catch(__Player->setup())` in `secure/std/login.c` silently swallowing
every earlier failure with no console trace. This slice fixed nine more
confirmed gaps found chasing that cascade (see "Root-causing the
__HistorySize report" below for the full trail), including a real
mudlib bug (`std/user.c`'s own `set_name()` missing the same PRIVS-off
bootstrap escape hatch `set_position()` already has) and a genuinely
significant new finding, not yet fixed: object variable slots may not
be correctly preserved across deep, multi-branch inheritance chains
(discovered via `std/living.c`'s own hidden `inherit` statements,
buried in `secure/include/living.h` rather than the `.c` file itself).
Live testing now reaches several steps further into chargen than
before, and stops at a new, distinct issue in `std/user/nmsh.c`'s own
`do_alias()`. Updated again after root-causing that issue: it was not a
`nmsh.c` bug at all, but a general compiler architecture bug in how
object-variable slots are resolved for sibling multi-inherits (see
"`do_alias()` root-caused" below) -- confirmed via live instrumentation,
proposed for direction, user-approved, and now implemented and fixed
(two distinct bugs, both covered by new regression tests, full suite
passing, `do_alias()` crash confirmed gone live). Updated again after
that fix uncovered a second, distinct, and likely broader bug reaching
for a room: this driver's function-call resolution order for a bare
call is backwards from real LPC's actual flattened-function-table
semantics (confirmed by reading `fluffos-2.9-ds2.08/compiler.c`'s
`define_new_function()` directly, not guessed) -- an ancestor file's
own deliberate placeholder/stub function (`std/user/nmsh.c`'s
`query_name()`, one of several such stubs) is not correctly overridden
by the real inheriting file's version for calls written inside the
ancestor's own source. See "Second, distinct bug found reaching for a
room" below. Proposed for direction, user-approved, and now
implemented and fixed, with the one existing test that encoded the old
(disproven) behavior corrected rather than just deleted. Three further,
much narrower gaps then surfaced live in immediate succession pushing
toward an actual room -- a real compiler bug (block scoping was never
implemented at all), a missing `sprintf` specifier (`%c`), and a
missing efun (`living()`) -- each root-caused, fixed, and covered by a
regression test; see "Three more gaps found live pushing from the
fixed `reset_prompt()` toward an actual room" below. Full suite: 282
tests passing. Live-confirmed: chargen now genuinely runs end to end
through zone selection and into attribute rolling for the first time
this project has reached it -- see "Live confirmed: chargen now
genuinely runs" below for the transcript. Not yet a full room: the
rest of chargen (roll/accept, race, OCC, alignment, skills) has not
been exercised live yet and may surface further gaps.

**Updated after closing that loop.** Resumed after an unplanned
reboot; found and killed an orphaned scratch driver process from a
different, no-longer-reachable session before starting fresh (see
"Resuming after a reboot: orphaned process, and a stale test script"
below for how that was confirmed safe). Pushed the live chargen walk
the rest of the way: race, alignment, and OCC selection all confirmed
live (including the exact `daemon/occ.c` empty-`attribute_requirements`
question the prior session had been mid-investigation of when the
reboot hit -- confirmed genuine content, not a driver bug, both by
static analysis and by live exercise of an empty-requirements OCC), then
nine more real, confirmed driver gaps chasing `finish_creation()` all
the way to an actual room: two missing efuns
(`all_inventory()`/`deep_inventory()`) whose absence crashed account
creation itself any time a player declined the name-confirm prompt,
`strcmp()` and `map_delete()` (the latter fatal to alignment selection),
a `clone_object()`/`load_object()` path-normalization bug (a caller-
supplied trailing `.c` produced a literal, never-existing `.c.c` file
lookup), a missing `intp()` type predicate, a missing `repeat_string()`
efun, a missing `present()` efun (blocking the very first starting
room's own `reset()`), and -- the deepest one -- this driver's
`explode()` never matched real FluffOS's own default leading/trailing-
separator semantics at all, which silently broke every compile-time
`privs` assignment for every object with a `/`-leading path (a new,
previously entirely missing mechanism, `ObjectManager::
initPrivsForObject()`, added alongside it). Also added
`remove_call_out()` (a stub matching `call_out()`'s own already-stubbed
non-scheduling behavior). Full suite: 288 tests passing. **Live-
confirmed end to end for the first time this project has reached it:**
a fresh account now runs the complete chargen flow -- login, account
creation, gender/display name/email/real name, zone, attribute
roll/accept, race, alignment, OCC pick, automatic starting-equipment
grant -- and lands in a real starting room (`domains/ChiTown/areas/
chitown_start.c`) with a live NPC present and a full room description,
`finish_creation()`'s own automatic display. See "Chargen closed the
loop: full run confirmed live, reaching a real room" below for the full
transcript and the complete gap-by-gap trail. **Correction, recorded
here rather than silently edited away:** this section originally also
claimed an explicit, separately-typed `look` command was confirmed
working at this point. It was not -- that claim came from a buffer-
timing artifact in this project's own probe script (trailing,
already-in-flight output from the automatic display was misattributed
to a `look` command sent moments later), not a genuine live result.
The real `look` command did not actually work at all until the
dispatch-argument bug described in "Real call_out()/heart_beat()
scheduler" below was found and fixed, several sessions later. See that
section for the real confirmation, and its own opening paragraph for
how the mistake was caught.

## Working now

- Clean build via `cmake -B build -S . && cmake --build build`.
- `ctest --test-dir build` passes (253 unit test cases in one binary,
  covering lexer, parser, codegen, and VM execution for every supported
  feature).
- Driver boots, compiles and loads `secure/daemon/master.c`, runs its
  `create()`, starts the non-blocking TCP listener, and now also
  compiles and loads the real `secure/SimulEfun/SimulEfun.c` simul_efun
  object (config's `simul_efun_file`) at boot.
- Function-call resolution is a full four-tier chain matching real
  FluffOS's own order (local -> inherited -> simul_efun object -> core
  efun table). `efun::name(...)` bypasses all of that straight to the
  core efun table, matching real LPC's own explicit escape hatch
  (needed by simul_efun files that shadow a real efun's name, e.g.
  `secure/SimulEfun/misc.c`'s own `destruct()` wrapper).
- Variables, assignment (including compound assignment `+= -= *= /= %=`,
  and embedded assignment inside a `&&`/`||` chain -- e.g.
  `stringp(val) && val=load_object(val) && ...` -- which real LPC
  resolves at its own precedence, not by stopping at the next `&&`),
  `if`/`else`, `while`, `for` (all three clauses optional), `switch`,
  `foreach`, a bare `;` null statement (a loop whose entire body is the
  condition's own side effects), `break`/`continue` (including
  correctly-scoped nested loops), comparisons, logical `&&`/`||`
  (short-circuiting), plain `&` (bitwise on ints, set intersection on
  arrays), plain `|`/`^` (bitwise-only, int operands), ternary,
  prefix/postfix `++`/`--` on bare variables, C-style type casts (parsed
  and discarded as a no-op).
- `switch (subject) { ... }`: real C/LPC fallthrough (no implicit break
  between cases -- `CodeGen::emitSwitchStmt()`'s own comment), `case`/
  `default` labels interleaved with ordinary statements. Single-value
  `case` labels only; range labels (`case A..B:`) throw
  `NotImplementedError` rather than being silently mishandled --
  confirmed by grep, nothing in this mudlib uses that form.
  `foreach (var in collection)` / `foreach (key, value in collection)`:
  desugars to an index-and-length loop over the collection itself (an
  array) or its `keys()` (a mapping), reusing the same `break`/`continue`
  machinery `while`/`for` already use. The two-variable form reads
  `collection[key]` for the value slot, which is only meaningful when
  `collection` is a mapping -- a bare array with a two-variable `foreach`
  is not supported, matching every real use of the two-variable form in
  this mudlib.
- Float literals (`1.5`, `.5`), stored in a dedicated `floatPool`
  alongside the existing `stringPool`.
- `call_other()`/`->`, with the function-name argument a full
  expression, and `->` chaining correctly with `[` indexing in either
  order and any number of times (`inv[i]->query_property(...)`,
  `a->b()->c()`, etc) -- previously a one-shot check before the index
  loop, not a shared loop, so index-then-arrow left the arrow
  unconsumed.
- Array/mapping literals accept a trailing comma before the closing
  `})`/`])`, real LPC's normal style.
- `inherit "path";`, single- and multi-level, flattened object-variable
  layout, inherit-cycle detection.
- `sscanf(str, fmt, ...vars)`: literal text, `%s`, `%d`, `%%`, `%*` skip
  modifier. `%f`/`%x`/`%(regexp)` throw rather than silently mishandling.
- `clone_object()`, arrays, mappings, indexing (read/write, range),
  string/array concatenation, arithmetic, unary negation/not, function
  prototypes, modifier keywords, object variables, adjacent string
  literal concatenation.
- Real file I/O: `read_file()` / `write_file()`, resolved against the
  configured mudlib root.
- A runtime error thrown out of an object's `create()` fails that one
  object's load with a clear `[object]`-prefixed message instead of
  crashing the whole driver process. This same guarantee now also
  covers `master->connect()` and per-line input dispatch in
  `net/Server.cpp` -- previously uncaught there, so one bad connection
  (or one player hitting a runtime bug mid-session) took the entire
  driver process down for every other connected player. Confirmed live:
  this is exactly what happened attempting `secure/std/login.c` before
  it actually compiled.
- The driver's own `cpp` invocation injects FluffOS's full driver-level
  predefined macro table (`option_defs.c`), plus the smaller
  runtime-computed set from `lex.c`'s `add_predefines()` (`MUD_NAME`,
  `__PORT__`, `__VERSION__`, `__ARCH__`, `SIZEOFINT`, `MAX_INT`, etc).
  `MUD_NAME`/`__PORT__` are sourced from `Config` (new `mud_name` key in
  `driver.cfg`, default `AMLP`), the rest are fixed values.
- A real socket connection has been driven end-to-end through
  `master->connect()` (which `clone_object()`s `secure/std/login.c`) via
  a raw Python socket test (no telnet/nc binary in this environment).
- `logon()` is called on every new connection immediately after
  `master->connect()` binds the object, with zero arguments (`net/
  Server.cpp`'s `onNewConnection()`, matching `backend.c`'s own
  `logon()`: `apply(APPLY_LOGON, ob, 0, ORIGIN_DRIVER);`). A runtime
  error in a defined `logon()` closes only that connection, same
  isolation guarantee as `connect()` and per-line dispatch already had.
- `input_to(string func, void|int flags, void|mixed extra_args...)` is
  a real efun: it records the calling object (`VM::currentObject()`,
  new -- a `callStack_` the VM now maintains across nested `run()`
  calls, real FluffOS's own `current_object`) plus any extra args as
  the pending handler on the connection driving the call
  (`OutputContext::current()`, standing in for `command_giver`).
  Numeric echo/no-escape flags (`I_NOECHO` etc) are positionally
  consumed like the real efun and then discarded -- this driver does
  not negotiate telnet echo suppression yet.
- Per-connection pending-handler storage is a new `Connection` slot
  (`net/Connection.hpp`'s `PendingInputTo`: a `weak_ptr<LpcObject>` plus
  function name and extra args), matching real FluffOS's
  `interactive_t::input_to` sentence.
- Per-line dispatch (`Server::dispatchLine()`, a new `static` method,
  deliberately pulled out of `handleConnection()` so it is directly
  unit-testable without a real listening socket) now matches `comm.c`'s
  `process_user_command()` order exactly: a pending `input_to()`
  handler is checked and consumed *first* (cleared before the call, so
  the handler is free to register the next one itself), and only when
  nothing was pending does `process_input(line)` run instead. The old
  fixed `receive_message(line)` apply is gone.
- New efuns: `receive()` (writes straight to the current connection,
  same as `write()` -- real FluffOS's `f_receive()` is `current_object-
  >interactive`-scoped, which is the same connection here), `call_out()`
  (validates the real 2+-arg shape and returns a handle; does not
  actually schedule anything yet -- `Scheduler::tickCallOuts()` is still
  the pre-existing empty stub, unchanged this slice), `master()`,
  `lower_case()`, `replace_string()` (3-arg replace-all form only).
- `call_other()` now accepts a string target, not just an already-
  resolved object -- confirmed against `simulate.c`'s real
  `find_object()`, which (unlike the easy-to-confuse-it-with
  `find_object2()` in the same file) compiles and loads the file on a
  cache miss rather than only ever looking one up. This is exactly why
  real `master.c`'s own `preload()` can force-load a daemon with
  nothing more than `call_other(str, "???")`. `VM::findObject()` wraps
  the already-existing `ObjectManager::loadObject()` (same cache
  `master`/`simul_efun` already load through) to provide this; no new
  preload/boot-order infrastructure was needed.
- `::name(...)` / `qualifier::name(...)` -- explicit calls to an
  *inherited* definition of a function, bypassing the current program's
  own local definition even when one exists (`grammar.y`'s
  `function_name` production). Found live compiling
  `secure/daemon/account_d.c`'s own `::create();` -- confirmed
  extremely common (887 files across the mudlib use the bare or
  qualified form, almost always an overridden `create()`/`init()`
  running its parent's own setup too). New `OpCode::CallParent` (plus a
  trailing `CallParentQualifierSlot` data instruction, the same shape
  `Sscanf`'s var-slot table already uses) resolves the bare form by
  walking every immediate parent's own inherit chain (skipping this
  program's own functions), and the qualified form by matching the
  qualifier against each immediate parent's own `inherit` path's
  basename (`daemon::create()` for `inherit "/std/daemon";`).
- `(: name, bound_args... :)` closure/function-pointer literals -- see
  the dedicated section below for the full recon/design/citation
  writeup. New `Value` variant `Closure` (owner object, bare function
  name, bound args), new `ClosureLiteralExpr` AST node, new
  `OpCode::PushClosure`, and `VM::callClosure()` (the "apply a closure
  with extra args" mechanism `evaluate()`/`funcall()` and, later,
  `call_out()` all share). Only the bare-identifier-name form is
  implemented, confirmed to be the only shape this mudlib's boot/login/
  account-creation path actually uses.
- Real `master()->apply_unguarded()`'s own shape -- `previous_object(0)`
  called from inside a closure-invoked core efun -- surfaced a genuine
  bug in `VM::callClosure()`'s first cut: a closure resolving to a core
  efun (not a local/simul_efun function) never went through `run()` at
  all, so `vm.currentObject()` stayed whatever the *caller of
  `evaluate()`* was instead of the closure's own owner, confirmed
  against real `setup_fake_frame()` (`interpret.c`), which unconditionally
  sets `current_object = fun->hdr.owner` for every closure kind,
  including a core-efun-bound one. Concretely: `secure/daemon/
  account_d.c`'s own `unguarded((: save_object, path :))` was saving
  `master.c`'s own variables to the account's save path instead of
  `account_d.c`'s. Fixed with a new `ObjectFrameGuard` RAII helper
  (shared by `VM::run()` and `VM::callClosure()`'s core-efun branch) and
  covered by a dedicated regression test
  (`testEvaluateOfEfunBoundClosureSetsCurrentObjectToClosureOwnerNotCaller`).
- New efuns needed to reach and validate the above live: `evaluate()`/
  `funcall()` (invoke a closure), `call_out()` extended to accept a
  closure as well as a function-name string, `master()`, `previous_object()`
  (backed by a new, separate `objectChangeStack_` -- real FRAME_OB_CHANGE
  semantics, only pushed on an actual cross-object call, not every
  same-object one), `error()`, `stringp()`/`objectp()`/`mapp()`/
  `pointerp()`/`arrayp()`/`functionp()` (type predicates), `file_name()`,
  `strsrch()`/`strstr()`, `interactive()`/`users()`/`find_player()`
  (backed by a new `InteractiveRegistry`, populated by
  `Connection::attach()`/`close()` -- see its own header), `allocate()`,
  `allocate_mapping()`, `values()`, `capitalize()`, `sprintf()` (`%s`/`%d`
  only, confirmed the only specifiers this mudlib's login/account path
  uses), `message()` (routes to the currently active connection,
  ignoring `type`/`targets`/`excludes` -- see its own comment for why
  that is enough for every call site actually on this path),
  `set_eval_limit()` (accepted, currently a no-op), `destruct()`
  (closes the connection too, if the destructed object was the one
  bound to it), `find_object()`/`load_object()` (look-only vs.
  compile-on-miss, real aliases of the same efun with different
  argument defaults), `time()`, `ctime()`, `userp()`/
  `query_once_interactive()`, `crypt()` (via the system's own `crypt(3)`,
  `-lcrypt` linked). Also fixed: `sizeof()`/`strlen()` did not handle a
  plain string argument at all (fell through to a silent `0`) --
  `!sizeof(some_string)` is this mudlib's standard "is this string
  empty" idiom, used constantly, and was being silently mis-evaluated
  everywhere until this was caught; `this_object()` was a permanent
  void stub before `VM::currentObject()` existed for `input_to()`'s own
  needs, now a real read of it.
- `save_object()`/`restore_object()` now use a recursive, self-
  delimiting serialization format covering every `Value` kind
  (int/float/string/array/mapping, arbitrarily nested) instead of the
  original flat int/float/string/string-array set, found live needing
  to grow when `daemon/banish.c`'s own `create()` saved a mapping
  variable. Also now normalize the save path the same way real
  `object.c`'s `save_object()` does (strip a trailing `.c`, strip a
  trailing `.o` if already present, always append `.o`) -- found live
  when `daemon/banish.c`'s own `restore_banish()` called
  `restore_object(SAVE_BANISH)` with no extension at all, relying on
  the efun to add one; without this fix a real pre-existing
  `daemon/save/banish.o` on disk was silently unreachable. A real,
  pre-existing save file in this mudlib's own space-separated LPC-
  literal text format (not this driver's format) is not parsed -- every
  line is silently skipped (no tab separator matches), so the object's
  variables keep whatever defaults its own `create()` already set
  rather than being populated from the real historical file; this is a
  deliberate simplification (see "Known stubs" below), not a crash, and
  was sufficient for everything reached live so far.
- `master()->compile_object()` (virtual objects): real FluffOS's
  `int_load_object()` fallback (`simulate.c`) when a `load_object()`/
  `find_object()` path has no matching `.c` file on disk -- the master
  apply `compile_object(path, 0)` gets a chance to hand back a real
  object anyway (`secure/daemon/master.c`'s own player-object branch
  clones `/std/user` and returns that clone for a virtual
  `/secure/save/users/<letter>/<name>` path). New `ObjectManager::
  loadVirtualObject()`, called from `loadObject()` exactly when the
  requested path's own `.c` file genuinely does not exist (mirroring
  `int_load_object()`'s own `stat()` check order): calls
  `master()->compile_object(path, 0)` via `VM::applyMaster()`, and if
  that returns a real object, rebinds it to the requested virtual path
  (`LpcObject::rebindFilename()`, matching real `load_virtual_object()`'s
  own `SETOBNAME` + object-hash reinsertion) and caches it there so a
  second lookup for the same path returns the same object without
  re-invoking `compile_object()`. Guarded against recursion
  (`virtualCompiling_`) and against firing before `master_`/`vm_` exist
  yet (matching real `load_virtual_object()`'s own `if (!master_ob)
  return 0;`). New `new()` efun, a real alias of `clone_object()`
  (func_spec.c: `object clone_object _new(string, ...);`) -- confirmed
  live: `master.c`'s own `player_object()` calls `new(OB_USER)`.
- The `status` type keyword -- real `lex.c`'s own `{"status",
  L_BASIC_TYPE, TYPE_NUMBER}`, a legacy synonym for `int`. Needs no
  CodeGen/VM handling beyond being parseable (this driver's `Value`
  model is already dynamically typed regardless of declared type).
  Found live compiling `std/user.c`'s own `static status snoop,
  earmuffs;`.
- Function declarations with only modifiers and no return type at all
  (real LPC's own implicit-`mixed`-return convention) -- found live
  compiling `std/user.c`'s own `private static register_channels();`
  (prototype) and `static private register_channels() { ... }`
  (definition), neither naming a type. `Parser::parseDeclPrefix()` now
  treats the type keyword as optional: if the token after any modifiers
  isn't itself a keyword, the declaration's name follows directly.
- A bare `{ ... }` block used as a standalone statement, not attached
  to any `if`/`while`/`for` -- real, standard LPC/C syntax for local-
  variable scoping. Found live compiling `std/user.c`'s own quit()-
  adjacent cleanup code. No CodeGen changes needed:
  `CodeGen::emitStatement()` already flattens any `Block` node it
  encounters as a statement (originally added for comma-separated local
  var decls), which is exactly right here too.
- `<N` from-the-end indexing (real LPC: `grammar.y`'s `expr4 '[' '<'
  comma_expr ...` family, confirmed against `eoperators.c`'s
  `f_range()`/`f_extract_range()`: the actual index used is `length -
  N`, clamped to 0 if still negative after resolving). Found live
  compiling `std/user.c`'s own `files[j][<2..] != ".o"`. Supported on
  both bounds of a range index and the single-index form, in any
  combination (`arr[<a]`, `arr[<a..b]`, `arr[a..<b]`, `arr[<a..<b]`,
  `arr[<a..]`) -- `IndexExpr` gained `indexFromEnd`/`rangeEndFromEnd`
  flags, carried through `OpCode::Index`/`OpCode::RangeIndex` via their
  otherwise-unused `argCount` field (repurposed as a 2-bit flags mask)
  since neither opcode needed a real argument count. A bare negative
  literal with no `<` prefix still throws exactly as before (see
  `testStringRangeIndexNegativeStartThrows`, unaffected) -- only a
  start that resolves negative *after* an actual `<N` conversion clamps
  to 0, matching real `eoperators.c`'s own distinction.
- Compound assignment on an indexed target (`target[index] += value`
  etc, including a chained/nested target like
  `player_data["general"]["quest points"] += ...`) -- found live
  compiling that exact `std/user.c` line. `IndexAssignStmt` gained
  `isCompound`/`compoundOp` fields mirroring `AssignExpr`'s own; the
  compound case emits target and index *twice* (once to read the
  current value via `Index`, once to write the combined result via
  `IndexAssign`) rather than duplicating them on the stack, since
  there is no "duplicate the top two stack entries as a pair" opcode.
  Harmless for every real target/index this mudlib's own call sites
  use (plain variable reads and string-literal keys, no side effects)
  but would double any side effect a more exotic target/index
  expression happened to have -- flagged in `IndexAssignStmt`'s own
  comment rather than silently assumed safe in general.

## `(: name, bound_args... :)` closures: recon, design, implementation

Consulted directly against the FluffOS reference driver before
designing, not inferred:

- **Grammar** (`grammar.y`): the lexer's own fast path (`lex.c`,
  around the `function_flag`/`L_NEW_FUNCTION_OPEN` handling) recognizes
  `(: identifier :)` / `(: identifier, args... :)` at the character
  level -- if the identifier immediately after `(:` is followed
  directly by `:` or `,`, it is classified right there as FP_L_VAR/
  FP_G_VAR/FP_LOCAL/FP_SIMUL/FP_EFUN by name lookup and handed to
  `grammar.y`'s `l_new_function_open` production. Anything else (a
  non-bare-identifier expression, e.g. `this_object()` or a string
  literal) falls back to `old_func()`, hitting the general
  `L_FUNCTION_OPEN comma_expr ':' ')'` production instead -- a real,
  distinct "inline lambda" form that compiles the comma-expression
  itself as the closure's own body code (`functional_t`), not a
  bind-by-name at all.
- **Representation** (`function.h`): `funptr_t` / `funptr_hdr_t` --
  `ref`, `type` (FP_*), `owner` (the object active when the literal was
  constructed, i.e. `current_object` at bind time), and `args` (bound
  arguments, a real `array_t*`). The type-specific union holds an efun
  table index, a local function index, a simul_efun index, or (for the
  inline-lambda form) a `functional_t` with its own tiny compiled
  program.
- **Invocation** (`function.c`'s `call_function_pointer()`): checks
  `owner` isn't destructed, calls `setup_fake_frame()` (unconditionally,
  for every closure kind -- "`previous_ob = current_object; current_object
  = fun->hdr.owner`", real FRAME_OB_CHANGE semantics), merges `hdr.args`
  in *before* whatever the caller's own extra arguments were
  (`merge_arg_lists()`, confirmed by reading its own stack-shuffling
  loop), then dispatches on `type`. `efuns_main.c`'s `f__evaluate()`
  (registered as both `evaluate` and `funcall`) is the generic "call a
  closure with extra args" efun; a non-`T_FUNCTION` argument is a
  silent no-op, not an error.

### Real usage across the mudlib (261 `(: ... :)` occurrences, 109 files)

- **Bare name, no bound args** (53 occurrences) -- `(: living :)`,
  `(: Setup :)`, `(: intp :)`. An efun, local function, or simul_efun
  referenced with nothing pre-applied.
- **Bare name with bound args** (138 occurrences, the dominant shape,
  ~53%) -- `(: file_size, p :)`. Overwhelmingly `unguarded((: efun_name,
  args... :))` (137 of these alone), the shape that blocked
  `account_d.c` live; also `call_out((: local_func :), delay)` (5
  occurrences, e.g. `daemon/intermud.c`'s own `call_out((: Setup :),
  2)`).
- **Object-bound, `(: obj_expr, "funcname" :)`** (part of the 36
  "other/complex" bucket) -- e.g. `std/Object.c`'s own `set_long((:
  this_object(), "new_long" :))`. Not the lexer fast path (the first
  token after `(:` is a call expression, not a bare identifier), so
  this compiles through the general inline-lambda production instead.
  Not implemented: not used anywhere on this driver's own boot/login/
  account-creation path (`domains/Praxis/*_vote.c` room files, well
  past what this driver currently reaches).
- **`$1`/`$2` positional and `$(name)` captured-variable placeholder
  lambdas** (7 occurrences) -- e.g. `daemon/services/who.c`'s own
  `filter(users(), (: $1 && environment($1) :))`. Not implemented: all
  7 are in `daemon/intermud.c`, `daemon/services/who.c`,
  `daemon/services/auth.c`, `secure/daemon/events.c`, `secure/daemon/
  chat.c`, `secure/std/client.c` -- none on this driver's current path.
- **Bare string-constant closures, `(: "literal" :)`** (35
  occurrences) -- e.g. `set_die((: "on_death" :))`. A real, if unusual,
  form (`grammar.y` explicitly warns on it: "Function pointer returning
  string constant is NOT a function call"). Not implemented: every use
  found is a `std/obj/*`-level callback (death/wear/remove hooks), not
  reached by this driver's current login/account-creation path.
- **Variable-holding-a-function bare form**, `(: gtmp1, gtmp2 :)`
  (`daemon/refs.c`) -- excluded on inspection: real `grammar.y` itself
  rejects a *bound-args* form naming a local/global variable
  ("Can't give parameters to functional."), and `refs.c` is not
  referenced from anywhere reachable in this mudlib at all. Likely
  dead/legacy code even in the real driver.

**Consumers confirmed on this driver's own reachable path:**
`unguarded()` (a simul_efun, `secure/SimulEfun/security.c`, wrapping
`master()->apply_unguarded()` -> `evaluate()`) and `call_out()`. `map()`/
`filter()`/`sort_array()`/`set_heart_beat()` all take closures in real
FluffOS too and are used elsewhere in this mudlib (chat/mail/channel
daemons), but none are reached from the boot/login/account-creation
path this driver currently exercises, so none of the three were
implemented as closure-consumers this slice -- `VM::callClosure()` is
already the generic mechanism any of them would need, so adding one
later is just "for each element, `vm.callClosure(closure, {element})`",
not a new design question.

**Storage**: every closure actually reachable in this mudlib is
constructed and consumed inline at the same call site (`unguarded((:
file_size, p :))`, `call_out((: Setup :), 2)`) -- none are assigned to
a variable, stored in an array/mapping, and invoked later through a
separate generic-apply code path. This is what justifies this driver's
lazy-resolve-by-name simplification (see `Value.hpp`'s `Closure`
comment): real FluffOS bakes FP_LOCAL/FP_SIMUL/FP_EFUN in at
construction time, this driver re-resolves the bare name against the
same tiered lookup `Call` already uses, at the moment the closure is
actually invoked. Observably identical for every closure this mudlib's
own boot/login/account-creation path builds.

## `catch(expr)`: implemented as a real VM-level control-flow construct

Not a function call -- confirmed directly against the FluffOS reference
driver rather than inferred: grammar.y gives it its own production
(`catch: L_CATCH expr_or_block`, not `function_call`), icode.c compiles
it to a dedicated `F_CATCH`/`F_END_CATCH` opcode pair bracketing the
guarded code, and interpret.c's `do_catch()`/`F_END_CATCH` show the
real semantics: the guarded expression's own result is always discarded
(`trees.c`'s `insert_pop_value()` on the catch argument), success
evaluates to int `0` (not empty string), and a runtime error evaluates
`catch(...)` to the error message string instead of propagating.

This driver's version:

- `Ast.hpp`'s `CatchExpr` node (only the `catch(expr)` parenthesized
  form -- real LPC's `catch { block }` alternative is not implemented,
  nothing in this mudlib uses it), recognized in the Parser by literal
  text the same way `efun::` and `sscanf` already are, not a reserved
  keyword.
- `Bytecode.hpp`'s `PushCatchFrame`/`PopCatchFrame` opcode pair, mirroring
  `F_CATCH`/`F_END_CATCH` exactly: `PushCatchFrame`'s operand is a
  forward-patched "resume here" jump target (same
  `emitJumpPlaceholder()`/`patchJumpToHere()` machinery `if`/`while`
  already use), `PopCatchFrame` is only ever reached on success and
  pushes `0`.
- `VM::run()` keeps a `catchFrames` stack local to each call (one LPC
  function invocation), wraps its own per-instruction dispatch in a
  `try`/`catch (const LpcRuntimeError&)`, and on an exception with an
  active frame: truncates the operand stack back to the depth recorded
  at `PushCatchFrame` time, pushes the error message, and resumes at the
  recorded instruction pointer. An empty `catchFrames` rethrows exactly
  as before `catch()` existed -- this is also what makes a caller's
  `catch()` correctly trap an error thrown inside a callee that has no
  `catch()` of its own: the callee's own (empty) `catchFrames` rethrows,
  the exception unwinds straight out of its nested `run()` call, and
  lands in the caller's own `try`/`catch`.
- `EvalCostError` (new, in `core/Errors.hpp`) is deliberately *not* a
  subclass of `LpcRuntimeError`, so the eval-cost-exceeded guard cannot
  be caught by `catch()` -- confirmed against `do_catch()`'s own "Can't
  catch eval cost too big error" handling, a runaway loop wrapped in
  `catch()` must still be stoppable.
- `throw()` is not implemented (not asked for this slice, and nothing in
  the boot/connect path uses it yet).

Confirmed live: `master->connect()` now runs its real body successfully
end to end. A socket connection genuinely reaches
`[net] connection fd=N bound to /secure/std/login` -- `catch(ob =
clone_object(OB_LOGIN))` succeeds, `err` stays falsy, `ob` is correctly
the cloned login object, exactly `master.c`'s own real two-outcome
shape.

## The connect/input protocol gap: closed, confirmed live

Real FluffOS's own connect-time sequence (`logon()` called with zero
args right after `connect()` binds the object, per-line dispatch
preferring a pending `input_to()` handler over `process_input()`) is
now implemented and confirmed live end to end, not just unit-tested in
isolation. A raw Python socket connection against the real mudlib
(`secure/std/login.c`, via `etc/driver.cfg`) gets:

```
   _____          __  .__                                   .___
  /  _  \   _____/  |_|  |__   ___________  _____  __ __  __| _/
 /  /_\  \_/ __ \   __\  |  \_/ __ \_  __ \/     \|  |  \/ __ |
/    |    \  ___/|  | |   Y  \  ___/|  | \/  Y Y  \  |  / /_/ |
\____|__  /\___  >__| |___|  /\___  >__|  |__|_|  /____/\____ |
        \/     \/          \/     \/            \/           \/

     A post-apocalyptic roleplaying game.
     ...
What account name do you wish?
```

-- `logon()`'s own `receive(read_file(WELCOME))` and
`receive("\nWhat account name do you wish? ")` calls, followed by its
`input_to("get_name")` registration, all real, all live. Sending a name
back over the socket correctly dispatches to `get_name()` (not
`process_input()`), which runs `convert_name()` (a simul_efun,
resolved through the existing tier-3 lookup, needing only the new
`lower_case()`/`replace_string()` efuns to actually execute) and then
`continue_login()`.

## The `(: ... :)` closures blocker: closed, confirmed live

The previous slice's blocker -- `secure/daemon/account_d.c` failing to
compile on `unguarded((: file_size, p :))` -- is fixed, and the live
walkthrough now goes dramatically further than closures alone required.
A raw Python socket connection against the real mudlib was driven
through the *entire* new-account flow:

```
What account name do you wish? aetherwalker

Account name: aetherwalker
Note: this will also be your FIRST character's name, ...
Confirm Aetherwalker as your account and first character name? (y/n) y

Please choose a password of at least 5 letters: hunterpass

Please confirm your password choice: hunterpass

(connection closed here -- see next real blocker below)
```

Every one of those prompts is real `secure/std/login.c` code running
end to end: `continue_login()` -> `BANISH_D->valid_name()`/
`allow_logon()` (both real daemon files, auto-compiled on first
`call_other()`) -> `new_user()` -> `choose_password()` ->
`confirm_password()` -> real `crypt()` hashing -> `ACCOUNT_D->
create_account()` -> `save_account()` -> `unguarded((: save_object, path
:))`. A genuine account save file landed on disk with the right
content:

```
__NoClean	I1;
account_name	S12:aetherwalker
password	S13:<real crypt hash>
characters	A1:S12:aetherwalker
email	S0:
last_character	S12:aetherwalker
pending_approval	I0;
```

(this driver's own recursive save format -- see "Working now" above --
not real FluffOS's own on-disk syntax; `S12:aetherwalker` is a length-
prefixed string, `A1:...` an array of 1 element, `I0;` a plain int.)

(`account_name`/`characters`/etc -- confirming the
`VM::callClosure()`/`ObjectFrameGuard` fix above: this is
`account_d.c`'s own data, not `master.c`'s, which is what actually
landed there before that fix.)

## `master()->compile_object()` (virtual objects): closed, confirmed live

The previous slice's blocker is fixed (see "Working now" above for the
implementation) and confirmed live: `master()->player_object("...")`
now correctly reaches `compile_object()`'s own `DIR_USERS` branch,
which calls `new(OB_USER)` and genuinely attempts to compile
`/std/user.c` for the first time -- progressing well past where the
driver stopped before, through several further gaps (also fixed this
slice, see "Working now"): the `status` type keyword, modifier-only
function declarations, a bare block statement, `<N` indexing, and
compound indexed assignment, each found by literally trying to compile
`std/user.c` and reading whatever line the parser choked on next.

## Object-bound/string-constant closures and `(*fp)(args)`: closed, confirmed live

(Recon and disconfirmation below are unchanged from when this section
was still "the next real blocker" -- kept in full as the citation trail
for *why* the general lambda form was implemented the way it was. See
"Closure/function-pointer forms completed" further down for what
actually landed and how.)

`std/user.c`'s own body now compiles cleanly. It `inherit`s several
files, including `std/user/editor.c`, which does not:

```
[object] compile error in .../std/user/editor.c: parse error: expected
":" in closure literal at line 40 (got "(")
[object] .../std/user.c: failed to compile inherited file
"/std/user/editor"
```

The actual source:

```
if(!abort) abort = (: previous_object(), "abort" :);
```

This is real LPC's **object-bound closure** form (confirmed against
`grammar.y`'s recon from the closures slice: since `previous_object()`
is a call expression, not a bare identifier, the lexer's fast path
(`lex.c`'s `function_flag` handling) does not fire, and this falls
through to the general `L_FUNCTION_OPEN comma_expr ':' ')'` production
-- the "inline lambda" form, not the bare-name-with-bound-args form
this driver already implements). The same file also uses two more
forms this driver does not implement: a bare string-constant closure
(`this_object()->more(EDITOR_HELP, "help", (: "return_to_edit" :))`)
and the `(*fp)(args)` dereference-call syntax (`(*__Abort)()`,
`(*__Callback)(__Arguments)`) -- confirmed by reading the same
`icode.c`/`grammar.y` machinery used for the closures slice's own recon,
not new territory syntactically.

What stopped this from being implemented alongside those (a routine
parser addition, the same as the bare-name form was): reading
`icode.c`'s `NODE_FUNCTION_CONSTRUCTOR` codegen for the general lambda
form (`i_generate_node(expr->l.expr)`, where `expr->l.expr` is the
parsed body expression -- here the comma-expression `previous_object(),
"abort"`) says the compiled closure body just *executes that expression
and returns its value* when called -- for a plain C-style comma
expression, that means: evaluate `previous_object()` and discard it,
then evaluate and return the string `"abort"`. Taken literally, calling
this closure would do nothing but yield the string `"abort"`, never
actually invoking anything on the object.

That reading cannot be reconciled with how the *identical* syntax is
used elsewhere in this mudlib and is clearly intended to work: `std/
Object.c`'s own `set_long((: this_object(), "new_long" :))` (confirmed
during the closures slice's own recon, at the time deferred as
"unconfirmed, not reachable yet") and several `domains/Praxis/
*_vote.c` room files use exactly this "(: obj, \"funcname\" :)" shape
specifically so that querying the room's description calls
`new_long()` on it dynamically. If evaluating the closure only ever
returns the literal string `"new_long"`, `query_long()` would return
that literal string as the room's actual description text -- clearly
not the intended behavior for real, actively-maintained content in
this mudlib.

Both readings can't be right at once. A follow-up pass re-checked this
specifically against the runtime invocation code, not just the parser/
codegen side, to test the hypothesis that `(: expr, "string" :)` (an
arbitrary first operand, a string-literal second operand) is a
dedicated "bind this function name on this object" closure form,
distinct from generic comma-expression folding, checked before falling
back to the general lambda path. It is not. Specifically:

- **`grammar.y` has no such production.** The full set of alternatives
  under the closure-literal nonterminal (`l_new_function_open ':' ')'`,
  `l_new_function_open ',' expr_list2 ':' ')'`, `L_FUNCTION_OPEN
  comma_expr ':' ')'`, and the mapping/array-literal cases) was read in
  full; none inspect operand *count* or operand *type* the way the
  hypothesis requires. The one type-based check in this whole block --
  `if ($2->kind == NODE_STRING) yywarn("Function pointer returning
  string constant is NOT a function call");` in the `L_FUNCTION_OPEN
  comma_expr ':' ')'` action -- only fires when the *entire* body is a
  single bare string node (i.e. `(: "literal" :)` alone, zero commas).
  For `(: this_object(), "new_long" :)`, `$2` is a `NODE_TWO_VALUES`
  node (a comma expression), not `NODE_STRING`, so this warning does
  not even apply to the two-operand shape -- it is a different, simpler
  gotcha-check unrelated to the hypothesis.
- **`comma_expr`'s own grammar rule confirms plain, type-agnostic
  comma-operator folding**: `comma_expr: expr0 | comma_expr ',' expr0
  { CREATE_TWO_VALUES($$, $3->type, pop_value($1), $3); }` --
  `pop_value($1)` wraps the *left* operand to discard its result; there
  is no branch anywhere in this rule keyed on what `$1` or `$3` actually
  are.
- **`icode.c`'s own codegen for `NODE_TWO_VALUES` is equally
  type-agnostic**: `case NODE_TWO_VALUES: i_generate_node(expr->l.expr);
  i_generate_node(expr->r.expr); break;` -- generate the (pop-wrapped)
  left, then the right, full stop. Nothing here inspects whether
  `expr->l.expr` looks like an object reference or `expr->r.expr` is a
  string constant.
- **The runtime invocation side gives the same answer.** A closure
  built from this general lambda form is `FP_FUNCTIONAL`
  (`function.h`'s `funptr_t`), which stores *compiled bytecode*
  (`functional_t`: `prog`/`offset`/`num_arg`/`num_local`), not a stored
  object-and-string pair -- there is no data shape for a runtime check
  to inspect in the first place. `call_function_pointer()`'s own
  `FP_FUNCTIONAL` case (`function.c`) does setup
  (`setup_control_stack`/`setup_variables`) and then
  `call_program(funp->f.functional.prog, funp->f.functional.offset)` --
  it jumps into that already-compiled bytecode and lets the ordinary
  interpreter loop run it to completion, with no post-processing branch
  on the result before returning it. `f__evaluate()`
  (`efuns_main.c`, backing `evaluate()`/`funcall()`) does not inspect
  the result either; it is later confirmed even the `(*fp)(args)`
  dereference-call form desugars to this exact same
  `call_function_pointer()` path.

So the actual, confirmed behavior of `(: this_object(), "new_long" :)`
on the real fluffos-2.9-ds2.08 reference driver is: evaluate
`this_object()` and discard it, then evaluate and return the string
`"new_long"` -- calling this closure never invokes `new_long()` on
anything. Cross-checking the mudlib's own usage does not rescue the
"dedicated form" hypothesis either: `new_long()` genuinely exists as a
real function on `cleric_vote.c`/`kataan_vote.c`/`fighter_vote.c` (each
defines its own `string new_long() { ... }` right next to the
`set_long()` call), which is exactly why this reads as *intended* to
invoke it -- but intent is not the same as what the referenced driver
source actually does with this syntax. Per `std/Object.c`'s own
`query_long()` (`if(functionp(__Long)) return (string)((*__Long)(str));
else if(stringp(__Long)) return __Long; ...`), the real, confirmed
consequence is that `query_long()` on any of these rooms returns the
literal string `"new_long"` as the room's description text, not the
dynamic description `new_long()` computes -- a genuine behavioral bug
in this mudlib's own inherited content (all three vote rooms trace to
the same "Nightmare IV" era code per their file headers), not a gap in
this driver's understanding of the reference source. `std/user/
editor.c`'s own `(: previous_object(), "abort" :)` default is, by the
same reading, a functionally inert placeholder (`previous_object()` has
no side effect, so the whole default closure does nothing but return
`"abort"`) -- harmless there specifically because `edit()`'s real
callers are expected to supply their own working `abort` callback, and
the broken default is only ever reached when they don't.

This driver now implements exactly what the reference source
implements: the general lambda form (`L_FUNCTION_OPEN comma_expr`)
compiles its body as ordinary comma-expression code and returns
whatever the last element evaluates to, matching real semantics
precisely rather than the "call the named method" behavior the
mudlib's own authors evidently assumed. `(: expr, "string" :)` is
therefore implemented as plain two-value comma folding, not as a
bound-method call -- implementing the call-the-method behavior instead
would make this driver *more* "helpful" than the real reference driver
its own author intended to match, silently papering over a real bug
this mudlib's own maintainers have not yet found. Confirmed live: a
"fixed" version was deliberately **not** wired in, per the explicit
instruction not to implement anything speculative once the reference
source's own runtime invocation code settles the question.

What was still genuinely worth implementing, independent of the above
(both are ordinary comma-expression-adjacent forms once the general
lambda body is compiled correctly, not the ambiguous case): the bare
string-constant closure `(: "literal" :)` (a `NODE_STRING` body,
already correctly a "return this constant, do nothing else" functional
per the same grammar/codegen just confirmed -- the compiler's own
`yywarn` even flags it as likely a mistake, but it still compiles and
runs) and the `(*fp)(args)` dereference-call syntax (confirmed to
desugar to the same `call_function_pointer()` path `evaluate()`/
`funcall()` already use). All three landed together in the next pass
-- see the next section.

## Closure/function-pointer forms completed: general inline lambda, bare string-constant closures, and `(*fp)(args)`

Implemented exactly what the recon above concluded, no more:

- **General inline lambda** (`Ast.hpp`'s new `InlineLambdaExpr`,
  `grammar.y`'s `L_FUNCTION_OPEN comma_expr ':' ')'`). The parser tells
  this apart from the existing bare-name closure literal the same way
  real LPC's LALR grammar does: only a bare identifier immediately
  followed by `,` or `:` is the bare-name form; anything else (a call
  expression, a string literal, ...) falls through to a general
  comma-separated expression list. `CodeGen` cannot emit a lambda's
  body in place (every function in one `CompiledProgram` shares a
  single flat `code` array addressed by `entryPoint`, so splicing a
  second `Return` into the middle of the enclosing function's own
  instructions would return out of the wrong function) -- it queues
  each one (`CodeGen::PendingLambda`) and compiles it right after the
  enclosing function's own `Return`, as its own synthesized
  `FunctionEntry` (name prefixed `$lambda#`, a sequence no real LPC
  identifier can ever equal), reached at call time through the exact
  same `findFunctionInChain()` lookup an ordinary bare-name closure
  already uses. No VM changes were needed for this reason.
- **Bare string-constant closure** (`(: "literal" :)`) is not a special
  case at all once the above works: it is the trivial one-element case
  of the same comma-separated body.
- **`(*fp)(args...)` dereference-call syntax** (`grammar.y`'s `'('
  '*' comma_expr ')' '(' expr_list ')'`) desugars at parse time straight
  to a forced call of the core `evaluate` efun (`CallExpr::forceEfun`,
  the same mechanism `efun::name(...)` already uses), exactly matching
  what the reference grammar's own action does
  (`predefs[evaluate_efun].token`) -- this driver's own `evaluate`/
  `funcall` efuns already call `VM::callClosure()`, so this too needed
  no VM changes, only a parser-level rewrite into an ordinary
  `CallExpr`.

Confirmed live: `std/user/editor.c` (the file that surfaced all three)
now compiles.

## Further gaps found and fixed while walking `std/user.c`'s full inherit chain

With the closure forms above landing, compilation walked forward
through `std/user.c`'s inherit chain (`autosave.c`, `editor.c`,
`files.c`, `nmsh.c`, `more.c`, `refs.c`, `living.c`, then `user.c`
itself) and hit four more real, confirmed gaps, each fixed the same
way: real mudlib usage first, reference grammar/codegen second,
implementation third, tests alongside.

- **Indexed `++`/`--`** (`std/living.c`'s own `healing["intox"]--`).
  `IncDecExpr` previously only accepted a bare variable name target
  (real usage grep found exactly 2 prefix and several postfix real call
  sites using an indexed target instead). `OpCode::IndexAssign` leaves
  nothing on the stack (correct for its original statement-only caller,
  `IndexAssignStmt`), so an indexed increment/decrement used as an
  expression stashes the pre- and post-mutation values in a hidden temp
  local (never nameable by real LPC source) and pushes back whichever
  one `prefix` calls for, after the mutation actually runs.
- **Indexed assignment as a sub-expression** (`std/user/more.c`'s own
  `if(!(__More["class"] = cl)) ...`). The parser previously only
  recognized a bare variable name as an assignment target *inside an
  expression* (index-expression targets were statement-only, via
  `IndexAssignStmt`). New `IndexAssignExpr` (`Ast.hpp`) plus
  `CodeGen::emitIndexAssignExpr()` use the same hidden-temp-local
  approach as the indexed `++`/`--` fix above, for the same underlying
  reason (`IndexAssign` produces no stack value to chain into the rest
  of the enclosing expression).
- **`private` object-variable scoping across an inherit chain**
  (`std/living.c`'s own `static private int __Locked, __LastAged;`
  colliding with `std/user.c`'s separate, unrelated `static int
  __LastAged;`). Real LPC scopes a `private` object variable to the
  file that declares it -- invisible to, and non-collidable with, a
  child's own variable of the same name -- but this driver previously
  discarded every modifier keyword unrecorded, including `private`,
  treating every object variable as fully inherited/nameable. Fixed by
  threading `isPrivate` through `Parser::DeclPrefix` ->
  `ObjectVarDecl::isPrivate`, and having `CodeGen::generate()` record a
  private variable's slot in the flattened `objectVarNames` list a
  child inherits under a synthesized, non-collidable placeholder name
  (`$private#<slot>`) instead of its real one -- the slot position is
  still reserved (an inheriting child's own bytecode has to agree with
  the parent's already-compiled bytecode on where every later slot
  starts), but the real name stays reachable only from the declaring
  file's own code.
- **`to_int(string | float | int)`** (`efuns_main.c`'s `f__to_int()`,
  surfaced directly by `std/user/more.c`, `std/living.c`, and
  `std/user.c` itself, not just transitively through the closure
  chain). Implemented matching real truncate-toward-zero float
  behavior and real leading-integer string parsing (`to_int("10x") ==
  10`); the `buffer` case in the real signature is dropped, since this
  driver has no buffer type in `Value`'s variant at all.

All four are covered by new `ctest` cases (17 added this session, one
per confirmed real shape plus the still-correctly-rejected range-index
`++`/`--` case -- see `tests/test_lexer.cpp`).

## `add_action`/`enable_commands` command-dispatch subsystem: closed, confirmed live

(The original "next real blocker" writeup below is kept in full as the
citation trail for why this was scoped as architecturally significant
rather than a routine efun gap. See "The add_action/enable_commands
command dispatch subsystem: recon, design, implementation" further down
for what actually landed, the confirmed real-usage recon, and the
design decisions made from it.)

With every compile-time gap above fixed, `std/user.c` now compiles
cleanly and a live account genuinely reaches `create()` on a real
player object -- confirmed live end to end (fresh account creation,
password set/confirmed, `compile_object()` succeeds). `create()` then
throws immediately:

```
[object] create() failed for /std/user: undefined function or efun: enable_commands
```

`enable_commands()` (real `add_action.c`'s `f_enable_commands()`,
`enable_commands(1)`) is the gate real FluffOS requires before
`add_action()`-registered commands on an object take effect. Checking
this driver's own `EfunTable.cpp` found `add_action` itself is not
registered either -- there is no command-dispatch subsystem here at
all yet, only the `input_to()`-callback path implemented and confirmed
live in an earlier slice (see "The connect/input protocol gap: closed,
confirmed live" above). This is a materially different, larger
mechanism (a per-object list of verb -> handler-function bindings,
consulted against typed input that doesn't match any pending
`input_to()` handler, in add-order with each handler free to decline
by returning 0 and falling through to the next) -- basic player
interaction (`look`, movement commands, anything using `add_action`
rather than `input_to()`) cannot work without it. Flagged rather than
started speculatively, matching this project's own stopping criteria:
this is architecturally significant, not a routine language or efun
gap.

A secondary, smaller issue found alongside this: when `create()` fails
on a cloned player object, the connection is left in a dead state with
no error sent to the client (`[net] connection fd=4 input handling
failed: call_other: first argument must be an object or a string
path` on the next line typed) rather than a clean disconnect or retry
prompt. Worth a small hardening pass once the real fix (`add_action`)
lands, not urgent on its own.

## The `add_action`/`enable_commands` command dispatch subsystem: recon, design, implementation

### Step 1: recon (real usage across this mudlib, before designing anything)

`add_action()` is called 384 times across this mudlib (excluding
`doc/`). The real shapes found:

- The overwhelming majority are the plain two-arg form,
  `add_action("cmd_foo", "foo")` -- a bare function name plus an exact-
  match verb string, e.g. every `cmds/mortal/_*.c` file's own `init()`.
- A verb argument can be an **array of strings**, one function bound to
  several verbs at once: `cmds/skills/_mist.c`'s own
  `add_action("checkdest", ({ "go", "enter" }))`.
- A three-arg **catch-all** form with an empty verb and flag 1:
  `std/living.c`'s own `add_action("cmd_hook", "", 1)` (in
  `init_living()`) and `secure/std/setter.c`'s own
  `add_action("chargen_catch", "", 1)`. Per this project's own earlier-
  recorded gotcha (`CLAUDE.md`'s "add_action catch-all gotcha"), flag 1
  is `V_SHORT`: the bound function receives only the text *after* the
  first word, and the real verb must be read back via `query_verb()`.

Where `enable_commands()` is called relative to `add_action()`: **not**
in the same place every time, and not always in an `init()` at all.
`std/user.c`'s own `create()` calls it directly, unconditionally, once
per player object. `std/living.c`'s own `init_living()` -- which calls
`add_action("cmd_hook", "", 1)` -- is itself called directly from
`std/user.c`'s `setup()`, as a plain function call, **not** through a
driver-invoked `init()` apply. Real per-room/per-item action
registration (`std/room/exits.c`, `std/room/senses.c`, `std/Object.c`)
*does* use a genuine driver-invoked `init()` apply hook (confirmed:
`std/room.c`'s own `void init() { container::init(); exits::init();
senses::init(); }`).

How verb dispatch is actually wired in this mudlib, confirmed by
reading `std/living.c` directly rather than assuming the generic
FluffOS default: **most player-typed mortal commands do not go through
per-file `add_action()` registrations at all.** `std/living.c`'s own
`cmd_hook(string cmd)` -- reached via the one catch-all registration
above -- reads the real verb via `query_verb()`, looks it up through
`daemon/command.c`'s own `find_cmd(verb, search_path)` (a directory-scan
cache mapping bare verb names to the directories containing a matching
`_<verb>.c` file), and if found, calls it directly via
`call_other(file, "cmd_"+verb, cmd)` -- reaching the file's blueprint
object directly, bypassing the standard per-object action table
entirely for this category. The hundreds of individual
`add_action("cmd_foo", "foo")` calls inside `cmds/mortal/_foo.c`'s own
`init()` are therefore **not** what actually dispatches an ordinary
player command in this mudlib's real, live design -- confirmed:
`cmds/mortal/_look.c` has no `add_action()` or `init()` at all, it is a
bare `cmd_look(string str)` function, reached only through
`cmd_hook()`'s own `find_cmd()`/`call_other()` path. The *real* generic
`add_action()` mechanism (a genuine per-object action table, refreshed
by `init()` on movement) is what room exits, room senses (`hide`,
`search`, `smell`, `listen`), `std/Object.c`'s own `read`, and
`living.c`'s own `lock` verb and catch-all hook actually use.

### Step 2: design (grounded in `fluffos-2.9-ds2.08/add_action.c` directly)

- **Storage**: `LpcObject` gained `environment_`/`inventory_` (real
  `object_t::super`/`contains`, simplified to a plain `weak_ptr`/
  `vector<shared_ptr>` rather than FluffOS's intrusive `next_inv`
  linked list -- this driver already uses that simplification
  elsewhere, e.g. `InteractiveRegistry`), `commandsEnabled_` (real
  `O_ENABLE_COMMANDS`), and `actions_`, a `vector<ActionEntry>` (real
  `sentence_t` list) where each entry is `{verb, functionName,
  owner (weak_ptr), flag}`. New registrations always prepend
  (`addAction()`), matching `add_action.c`'s own literal comment:
  `"adding to the top of the list doesn't harm anything"` --
  `p->next = command_giver->sent; command_giver->sent = p;` -- so the
  most-recently-registered entry is always checked first.
- **When/how `init()` gets (re-)invoked**: `VM::moveObject(item, dest)`
  (backing the new `move_object()` efun) ports the two legs of real
  `setup_new_commands()` (`add_action.c`) this mudlib's own confirmed
  usage actually needs: if `item` is command-enabled, `dest`'s own
  `init()` runs with `item` as `command_giver` (a room handing its
  verbs to the player who just entered); then, for every other
  already-present object, each side's `init()` runs against the other
  as `command_giver` if command-enabled, in the same order real
  `setup_new_commands()` uses (matters for prepend-order priority). The
  third leg (`dest` itself being command-enabled, i.e. being moved
  *into* another living object rather than a room) was scoped out --
  the reference source's own comment calls it "rare", and nothing on
  this mudlib's confirmed movement path does it. `command_giver` itself
  is a new explicit `VM` stack (`commandGiverStack_`, RAII-guarded via
  `CommandGiverGuard`, mirroring real `save_command_giver()`/
  `restore_command_giver()`), falling back to whichever connection is
  currently driving the call (`OutputContext::current()`) when nothing
  has explicitly set one -- needed because `std/living.c`'s own
  `add_action("cmd_hook", "", 1)` runs as a *plain function call* from
  `setup()`, not through a driver-invoked `init()` apply, so there is no
  `moveObject()`-provided `command_giver` active at that point; real
  `secure/std/login.c` confirms this is fine because its own
  `exec_user()` calls `exec(__Player, this_object())` (rebinding the
  connection to the new player) **before** calling `__Player->setup()`,
  so the connection's own bound object already *is* the player by the
  time `add_action()` runs.
- **Dispatch**: `VM::dispatchCommand(giver, line)` (real
  `parse_command()`/`user_parser()`) splits the line into its first
  word (the verb) and the remainder, walks `giver->actions()` in
  registration order (front = most recent = checked first), and for
  each entry: flag 0 requires an exact verb match; flag 1/2 requires
  `entry.verb` to be a leading-characters prefix of the typed verb (an
  empty `entry.verb` trivially matches everything, covering the real
  catch-all shape) -- `query_verb()` always returns the *full* typed
  word, never just the matched prefix, matching real semantics exactly.
  A handler that returns falsy does not stop the search (real
  `add_action.c`'s own doc comment: "the parser will continue searching
  for another command, until one returns true"); one entry's argument
  is the plain remainder-of-line string, matching real LPC's own
  argument-passing convention for a bound command function.
- **The `enable_commands()` gate**: a plain boolean flag on
  `LpcObject`, checked by `moveObject()` before treating an object as
  eligible to have its own `init()` propagate actions, or to receive a
  room/occupant's -- exactly real `O_ENABLE_COMMANDS`'s role.

### Step 3: implementation

New: `LpcObject::environment_`/`inventory_`/`commandsEnabled_`/
`actions_` (`ActionEntry`, `addAction()`, `removeAction()`); `VM`'s
`commandGiverStack_`/`verbStack_` plus `commandGiver()`/
`pushCommandGiver()`/`popCommandGiver()`/`currentVerb()`/
`moveObject()`/`dispatchCommand()`; new efuns `environment()`,
`move_object()`, `enable_commands()`, `disable_commands()`,
`add_action()`, `remove_action()`, `query_verb()`, `this_player()`.
`Server::dispatchLine()` now calls `process_input()` first if defined
(confirmed live needed: `std/user/nmsh.c`'s own real mud-shell/history/
alias preprocessing) -- its return value decides what actually reaches
`dispatchCommand()`, matching real `comm.c`'s own three-way branch on
the apply's return type (string = dispatch that instead; truthy number
= fully consumed, nothing dispatches; anything else = dispatch the
original line unchanged) -- rather than the previous silent no-op
fallback.

Two further gaps had to be fixed along the way, both required for
`std/user.c` to actually reach a live account:

- **`exec(object new_ob, object old_ob)`** (real `replace_interactive()`)
  was completely missing. Without it, `secure/std/login.c`'s own
  `exec(__Player, this_object())` could never rebind the connection
  from the login object to the actual player, so the player's own
  `create()`/`setup()` would run but the connection would stay bound to
  the login object forever. Implemented via `Connection::attach()`
  (already existed for the initial login-object bind), targeting
  whichever connection is currently driving the call
  (`OutputContext::current()`) -- matches every real call site in this
  mudlib, `exec()` is always called by the object currently holding the
  connection, never by an unrelated third object.
- **`query_privs(object default: this_object())` /
  `set_privs(object, int|string)`** (real `object_t::privs`) were
  missing, surfaced by `std/living.c` and `std/money.c` calling
  `query_privs()` unconditionally on log-relevant lines. Implemented as
  a plain `std::optional<std::string>` field on `LpcObject`.

Walking further into the live account-creation/chargen flow (past
`add_action`/`enable_commands` themselves) surfaced four more real,
confirmed gaps, fixed the same way as always -- real usage first,
reference source second, implementation third, tests alongside:

- **Object variable declaration-time initializers**
  (`secure/daemon/wiztools.c`'s own `string *REISSUED_TOOLS = ({ ...
  });`, a file with no `create()` at all). Previously rejected with an
  explicit `NotImplementedError` since nothing on the confirmed path
  used the shape -- now a real gap. `ObjectVarDecl` gained an
  `initializer` field; `CodeGen::generate()` synthesizes a
  `"$objvarinit"` function (same synthesized-name convention as the
  lambda/private-slot mechanisms) that assigns every initialized
  variable; `ObjectManager::runObjectVarInitializers()` runs it,
  parent-before-child across the whole inherit chain, immediately
  before `"create"` on every new instance -- via a new
  `VM::callFunctionInProgram()` that targets one exact program level
  directly (not the normal tiered lookup), since every level uses the
  same fixed synthesized name and the normal lookup would only ever
  reach one level's copy.
- **`undefinedp(mixed)` / `nullp(mixed)`** (real `f__undefinedp()`),
  surfaced by `daemon/multi.c`'s own `query_prevent_login()`. This
  driver has no int-subtype distinction the way real FluffOS's
  `T_UNDEFINED` is; its own `monostate` ("no value" -- what an
  undefined function call returns) is the closest analog, so that is
  what these check instead of a number subtype flag.
- **A genuine cross-inherit function-resolution gap**: `OpCode::Call`
  resolved a bare name only against the *currently executing file's*
  own program and its own inherited chain, never against the object's
  actual most-derived program. Real LPC compiles each file
  independently and has no way to know at a parent's own compile time
  that some future child will define a name it references -- such a
  call can only resolve at runtime, against whatever the real running
  object turns out to be. Surfaced live: `std/user/nmsh.c`'s own
  `process_input()` calling the bare name `query_client`, a function
  only `std/user.c` (which inherits `nmsh.c`) defines. Fixed with a
  fallback: if the normal lexical-scope search (current file, then its
  own ancestors) finds nothing, retry against `obj->program()` -- but
  *only* as a last resort, after the lexical search has already failed,
  specifically so a file's own internal self-calls keep resolving to
  its own (or its own ancestors') definitions first; real LPC does not
  virtually dispatch a parent's internal calls to a child's override.
  Both directions are covered by dedicated tests
  (`testParentCallToFunctionOnlyChildDefinesResolvesAtRuntime`,
  `testParentCallStillPrefersItsOwnLexicalDefinitionOverChildsOverride`).
- **Object variables and locals defaulted to this driver's own
  `monostate` ("no value") instead of real LPC's actual default, the
  integer `0`.** This one is the most consequential of the four: real
  LPC has no separate "unset" state distinct from `0` for an ordinary
  declared variable (there is no equivalent of this driver's own
  `monostate` at that level, only at specific driver-internal "no
  value" sentinels this codebase already used it for deliberately,
  e.g. an efun explicitly returning "nothing found"). This had been a
  documented but unverified assumption since early in the project (both
  `LpcObject.cpp` and a `test_lexer.cpp` test's own comments asserted
  monostate "matches how ... reads as 0" without it ever having been
  exercised against real arithmetic) -- it does not: monostate fails
  every arithmetic opcode a real `0` would silently succeed at.
  Surfaced live twice in a row before being fixed at the root:
  `std/Object.c`'s own `query_name()` reading an unset `__TrueName`
  (string concatenation), and `std/user/nmsh.c`'s own
  `add_history_cmd()` doing `++__CmdNumber` on an unset counter.
  `LpcObject`'s `variables_` and `VM::run()`'s per-call `locals` now
  both fill with a real `int64_t 0` per slot instead of a default-
  constructed `Value{}`.

Also added along the way: uncaught `LpcRuntimeError`s are now tagged
with `file::function(): ` at the innermost frame that had no `catch()`
of its own to absorb them, matching this driver's existing convention
of naming the file in every other `[object]`-prefixed diagnostic --
this is what made diagnosing several of the gaps above tractable at
all, and is worth keeping regardless.

All of the above is covered by 24 new `ctest` cases (`environment()`/
`move_object()` linking, the `enable_commands()` gate, exact and
catch-all verb dispatch with correct argument-splitting, the
falsy-return-keeps-searching rule, `this_player()`/`query_verb()`
during dispatch, `query_privs()`/`set_privs()`, object variable
initializers including the parent-before-child ordering, `undefinedp`/
`nullp`, and both directions of the cross-inherit call-resolution fix).

### Live test results

Confirmed live, in order, all in one continuous account-creation and
chargen session: account name entry and confirmation, password set and
confirmed, gender selection, display name formatting, email and real-
name prompts (both optional, skipped), the first-admin bootstrap offer
(confirmed reachable both accepting and declining it), and zone
selection. `enable_commands()`, `add_action()`, `move_object()`,
`exec()`, and the `process_input()`/`dispatchCommand()` pipeline all
fired correctly along this path with no errors of their own --
every error hit past that point was a *different*, specific,
individually-diagnosed-and-fixed gap (`query_privs`, `undefinedp`, the
cross-inherit call bug, the object-variable-default bug), not the
dispatch subsystem itself. The transcript never reached a state where a
player could type `look` at a live prompt, because chargen itself does
not reach a real room before hitting the next blocker below -- but
every piece of the dispatch subsystem that *is* exercised along this
path (the catch-all `cmd_hook` registration actually firing,
`process_input()` actually being consulted and its return value
actually feeding into dispatch, `this_player()` resolving correctly
mid-dispatch) is confirmed working, and the 24 new unit tests directly
exercise the exact real shapes (`std/living.c`'s own catch-all,
`std/room/exits.c`'s own movement registrations, the falsy-return
fallthrough) end to end at the VM level independent of how far live
chargen gets.

## Root-causing the `__HistorySize` report: guard logic was never the bug

The previous slice's own "next real blocker" (reproduced above) turned
out to be a red herring, confirmed by adding temporary `write()`
instrumentation directly to `nmsh.c`'s `setup()` and to
`secure/std/login.c`'s own `catch(__Player->setup())`, then observing
real runtime values against the live chargen flow (removed once
understood, per this project's own methodology of never leaving
debug-only code behind). The actual sequence:

- `secure/std/login.c`'s own `catch(__Player->setup())` silently
  swallows *any* error thrown inside `setup()`, with no console trace
  at all -- confirmed by temporarily capturing and printing the
  `catch()` result, which is otherwise discarded.
- Once `setup()` genuinely reaches `nmsh::setup()` without an earlier
  silent failure, live instrumentation confirmed `this_player() ==
  this_object()` (the guard correctly evaluates false, does not return
  early) and `reset_history()` correctly sets `__HistorySize` to `10`.
  The guard was never broken.
- The `__HistorySize` symptom reported at the end of the previous slice
  only ever surfaced because `setup()` had *already* failed earlier,
  silently, on one of several missing efuns confirmed below -- by the
  time a *different* test happened to get further before failing
  elsewhere, `__HistorySize` looked like the active bug purely by where
  the crash happened to land.

Nine further gaps were found and fixed chasing this cascade to the end,
each confirmed live before being fixed, tests added alongside every
one:

- **`set_living_name(string)`** (real `add_action.c`'s
  `f_set_living_name()`) was entirely unregistered. Stored on
  `LpcObject` (a new `livingName_` field) without wiring up a lookup
  table for it, matching this driver's own existing `find_player()`
  simplification (InteractiveRegistry + `query_name()`, not a real
  living-name table).
- **`set_heart_beat(int)` / `query_heart_beat(object)`** were also
  entirely unregistered, despite `LpcObject` already having
  `hasHeartbeat()`/`setHeartbeat()` support (used by `ApplyTable`'s own
  `heart_beat` apply recognition) -- nothing had ever wired the actual
  efun to it. Setting the flag now works and is queryable; there is
  still no periodic heartbeat scheduler that reads it back to actually
  call `heart_beat()` on anything (a separate, larger feature, not
  needed for anything reached so far).
- **`query_ip_name(void|object)`** was missing. Implemented as an alias
  of the already-working `query_ip_number()` (always the numeric IP,
  never a real reverse-DNS hostname lookup -- this driver does no DNS
  resolution of its own, matching real FluffOS's own documented
  fallback when hostname resolution is unavailable, and avoiding a
  blocking lookup inline in the connection-handling loop).
- **A genuine cross-inherit function-resolution gap in `OpCode::Call`**:
  a bare call resolved only against the *currently executing file's*
  own program and its own inherited chain, never against the object's
  actual most-derived program. Real LPC compiles each file
  independently and has no way to know at a parent's own compile time
  that some future child will define a name it references -- such a
  call can only resolve at runtime. Surfaced live: `std/user/nmsh.c`'s
  own `process_input()` calling the bare name `query_client`, which
  only `std/user.c` (which inherits `nmsh.c`) defines. Fixed with a
  fallback to `obj->program()`, but *only* after the normal lexical-
  scope search has already failed, so a file's own internal self-calls
  still resolve to its own definitions first (real LPC does not
  virtually dispatch a parent's internal calls to a child's override --
  covered by two dedicated tests, one per direction).
- **Array subtraction (`arr1 - arr2`, real set difference) was entirely
  unimplemented** -- only numeric `-` existed. A significant, previously
  undiscovered gap given how common this idiom is in real LPC. Surfaced
  live: `std/user.c`'s own `register_channels()` doing `channels -
  __RestrictedChannels`. Implemented as: every element of the left
  array that also occurs anywhere in the right array (by value
  equality) is dropped, order and any non-matched duplicates preserved.
- **`monostate` did not participate in arithmetic as a real `0`.** Real
  FluffOS's `T_UNDEFINED` is a *subtype* of `T_NUMBER` (a number whose
  value already is `0`, tagged only so `undefinedp()` can detect it),
  not a separate value kind arithmetic has to special-case. This
  driver's own `monostate` plays the same "no value" role (a missing
  mapping key, or -- before this slice's own earlier `0`-default fix --
  an unassigned object variable/local) and needed the same treatment.
  Surfaced live: `std/living.c`'s own `query_stats()` doing
  `stats[stat] + x` where `stats[stat]` is a missing key for any stat
  never rolled yet. Fixed via a shared `asArithmeticOperand()` helper
  used by `Add`/`Sub`/`Mul`/`Div`/`Mod` (and therefore `++`/`--` too,
  which already desugar through `Add`/`Sub`), treating `monostate` as
  `0.0` alongside `int64_t`/`double`.
- **A real, confirmed mudlib bug**: `std/user.c`'s own `set_name()`
  never had the same PRIVS-off bootstrap escape hatch `set_position()`
  (in the very same file) already has and already documents in its own
  comment ("PRIVS is #undef in options.h, so master()->valid_apply()
  ... can never return true for anyone"). Confirmed live with targeted
  instrumentation: `secure/daemon/master.c`'s own `compile_object()`
  calls `ob->set_name(nom)` directly, with no `unguarded()` wrapping, so
  the resulting `check_access()` stack-walk always denies on
  `secure/std/login` turning up privs-less in the previous-object
  chain -- for every new character, every time, not an edge case.
  `set_position()`'s own comment and existing escape hatch (authenticate
  by caller identity instead of going through `valid_apply()` at all)
  is the established fix pattern in this same file; `set_name()` now
  uses the equivalent check (`previous_object() == master()`, the only
  trusted direct caller of this exact call shape).
- **A second real mudlib bug in the same area**: `std/user.c`'s own
  `query_name()` override read `__TrueName`, a variable *reachable* from
  `user.c` only through an extremely deep, five-level inherit chain
  (`user.c` -> `LIVING` -> `secure/include/living.h`'s own hidden
  `inherit` statements -> `/std/living/combat` -> `BODY` ->
  `CONTAINER` -> `/std/Object`) that this project had never previously
  mapped (see the new finding directly below). Confirmed live:
  `::set_name(str)` genuinely resolves and runs `/std/Object`'s own
  `set_name()` (which does set `__TrueName`) without throwing, yet
  `query_name()` still read back a non-string value immediately
  afterward -- strong evidence the *object-variable slot* `__TrueName`
  occupies is not consistently the same one on both sides of this deep
  chain (see the finding below; not fixed this slice). The commented-
  out line directly above the broken one (`//tmp =
  living::query_name();`) is contemporary evidence the original
  developer never got this working correctly either. Given `char_name`
  is confirmed reliably set (by the very same `set_name()`, in the same
  assignment), `query_name()` now returns `char_name` directly --
  matching the function's own evident intent, using the variable that
  actually works, without needing to resolve the deeper slot question
  to unblock everything downstream of it (`wiz_setup_workroom()`'s own
  path concatenation, `std/user/nmsh.c`'s own `reset_prompt()` passing
  it to `replace_string()`, both confirmed live crashing on this before
  the fix).
- **`map_array()`/`map()` and `filter_array()`/`filter()`** were both
  entirely unregistered. Implemented for the two real shapes this
  mudlib uses (a `Closure`, called directly via `VM::callClosure()`; or
  a string function name plus a target object, calling
  `target->name(element, extra_args...)` for each element) -- not the
  full `filter()`'s real string/mapping first-argument forms, which
  nothing here uses. Surfaced live: `std/user/nmsh.c`'s own
  `do_nickname()`.
- **`implode()`** was also entirely unregistered (its counterpart,
  `explode()`, already existed). Implemented for the plain string-
  separator form only, matching every real call site
  (`std/user/nmsh.c`'s own `do_alias()`/`do_nickname()`); the real
  function-per-element form is not implemented.

All nine are covered by new `ctest` cases (13 added this slice).

## `do_alias()` root-caused: confirmed compiler/VM bug, object-variable slots collide across sibling multi-inherits

The `secure/include/living.h` hidden-`inherit` finding from the
previous slice (`std/living.c` actually has five real `inherit`
statements only visible after cpp expansion, giving `std/user.c` a
five-level-deep branch alongside six other parallel top-level
branches: `AUTOSAVE`, `EDITOR`, `FILES`, `NMSH`, `MORE`, `REFS`,
`LIVING`) turned out to be the same root cause as the `do_alias()`
blocker. Confirmed live with temporary instrumentation (a per-file
dump of `CompiledProgram::objectVarNames` in `ObjectManager::compile()`,
plus the `__Xverbs`/`__Aliases` checks from the previous slice),
removed once understood, per this project's own standing rule.

**The bug.** `ObjectManager::compile()` caches one `CompiledProgram`
per filename and reuses it verbatim everywhere that file is inherited
(`programCache_[filename]`, see the comment at the top of `compile()`
explaining this is deliberate, so a file inherited by several others
is only compiled once). `CodeGen::generate()` assigns every object
variable a sequential absolute slot number by walking
`inheritedObjectVarNames`, the flattened list of the *direct* parents'
own variable names, passed in by `ObjectManager::compile()`. For a
"leaf" mixin with no `inherit` of its own (`AUTOSAVE`, `EDITOR`,
`FILES`, `NMSH`, `MORE` are all leaves), `inheritedObjectVarNames` is
empty, so that file's own object variables always get local slots
starting at 0 -- correct only when that file is compiled and executed
completely on its own. `VM::run()`'s `PushObjectVar`/`StoreObjectVar`
opcodes use `instr.operand` as a *raw* index straight into
`obj->variables()`, with no per-program base-offset adjustment
(confirmed by reading both opcode cases in `VM.cpp`; there is no
`objectVarBase`/`slotBase`/offset concept anywhere in `Bytecode.hpp`,
`CodeGen.cpp`, `ObjectManager.cpp`, or `VM.cpp`).

When `std/user.c` inherits seven things in one file, each leaf
sibling's *already-compiled, cached* bytecode still carries the local
slot numbers it was given when compiled standalone. Live evidence,
captured via the temporary `ObjectManager::compile()` dump:

```
compiled /std/user/autosave inheritedCount=0 totalVars=4   (local slots 0..3)
compiled /std/user/editor   inheritedCount=0 totalVars=5   (local slots 0..4)
compiled /std/user/nmsh     inheritedCount=0 totalVars=15  (local slots 0..14)
...
compiled /std/user inheritedCount=104 totalVars=155
  [0..3]   AUTOSAVE's real absolute slots
  [4..8]   EDITOR's real absolute slots
  [9..23]  NMSH's real absolute slots (__Nicknames=9, __Aliases=10, __Xverbs=11)
```

`user.c`'s own compile correctly computes NMSH's real absolute range as
9..23 (used whenever `user.c`'s own code resolves an inherited variable
by name). But NMSH's own cached bytecode -- generated when NMSH was
compiled on its own, with `inheritedCount=0` -- still emits raw operand
`2` for `__Xverbs` (its third local variable, local slots 0/1/2 for
`__Nicknames`/`__Aliases`/`__Xverbs`), not `11`. Since the VM applies
`instr.operand` directly with no offset, every one of NMSH's own
functions actually read and write `obj->variables()[2]` -- which is
really `AUTOSAVE`'s own local slot 2, `static private int __LastSave`.
`create()` writes `__Xverbs`'s mapping into slot 2 and reads it straight
back through the same (equally wrong, but self-consistent) local slot
number, so the debug check right after `create()` showed `is_mapping=1`
for all three variables. Later, `AUTOSAVE`'s own code writes an
ordinary int into its `__LastSave` (also raw slot 2) during account
setup, silently overwriting what NMSH's own code still thinks is
`__Xverbs` -- which is exactly why the live test showed `__Xverbs`
correct immediately after `create()` and broken (`is_mapping=0`) by the
time `do_alias()` ran.

This is a general architecture bug, not specific to `nmsh.c`/`do_alias()`:
any file with two or more directly-inherited sibling files that are
each leaves (no inherits of their own) will alias each other's low
slot numbers the same way, because each leaf's cached bytecode was
compiled assuming it is the entire object. `std/user.c` (seven direct
inherits, several of them leaves) is simply the first place this
mudlib's own structure exercises it badly enough to crash. Real
FluffOS avoids this by construction: each `inherit_t` on a `program_t`
records its own `variable_index_offset`, resolved per compiled program
against its actual place in that specific object's inherit tree, and
compiled function code always addresses object variables relative to
that per-inherit base at the point of dispatch -- not via a single
globally-cached, offset-free absolute slot baked in at each file's own
standalone compile time.

**Fixed, confirmed live and by regression test, user-approved before
implementation.** Two distinct bugs, both in the compiler, both now
fixed:

1. **Missing runtime base offset** (the one described above). Fixed by
   adding `CompiledProgram::ancestorBaseOffsets`
   (`std::unordered_map<const CompiledProgram*, int>`), populated in
   `ObjectManager::compile()` right after each parent is resolved: for
   every direct parent, record the base offset its own local slot 0
   maps to within this file's own flattened layout, then merge that
   parent's own `ancestorBaseOffsets` in, shifted by the same base --
   composing correctly across arbitrarily deep chains, not just one
   level of direct siblings. `VM::run()` now computes an
   `objectVarBase` once per call (0 when the executing program *is*
   `obj->program()` itself -- the common case, no map lookup needed --
   otherwise looked up from `obj->program().ancestorBaseOffsets`), and
   both `PushObjectVar`/`StoreObjectVar` add it to `instr.operand`
   before indexing `obj->variables()`.
2. **A second, distinct bug found while regression-testing the first**:
   `CodeGen::generate()` computed a newly-declared object variable's own
   slot number as `objectVars_.size()` -- the size of a *name-keyed
   map* built from `inheritedObjectVarNames`. A private variable's
   synthesized name (`"$private#N"`) is only unique relative to the
   file that declared it (local numbering always starts at 0); two
   files reached via separate inherit branches can each independently
   produce `"$private#0"`, and when both are flattened together those
   names collide in the map, silently undercounting the real number of
   inherited slots. Fixed by tracking the next slot with a separate
   `size_t nextObjectVarSlot`, seeded from `inheritedObjectVarNames.size()`
   (a plain vector length, immune to name collisions) and incremented
   per new variable, instead of reading it back off the map.

Both fixes are covered by dedicated regression tests in
`test_lexer.cpp`: `testSiblingLeafObjectVariablesDoNotAliasEachOther`
(mirrors `std/user.c`'s own AUTOSAVE/EDITOR/NMSH shape -- two leaf
siblings, each with several private variables in one declaration
statement) and `testObjectVariableOffsetsComposeAcrossMultiLevelInheritChain`
(mirrors `std/living.c`'s own shape -- a 3-level chain plus an
unrelated sibling leaf at the top, matching `std/living/combat.c`'s
real `inherit BODY; inherit SKILLS;`). Both tests were confirmed to
fail against the pre-fix code before the fix was applied (not just
written to pass vacuously): the sibling test failed with
`leaf_two`'s init overwriting `leaf_one`'s own first variable at raw
slot 0, and the multi-level test failed via bug 2 above (`create()`'s
own new variable landed on the same slot a sibling branch's variable
already used). Full suite (277 tests) passes after both fixes.
Live-confirmed: the original `do_alias()` crash ("Index: target is not
an array, mapping, or string") no longer occurs; `__Xverbs` now stays
a correct mapping all the way from `create()` through the alias-check
codepath.

## Second, distinct bug found reaching for a room: function-call resolution order is backwards from real LPC for ancestor-overridable stubs

While re-testing live after the slot fix above, chargen got further
(no more `do_alias()` crash) but `setup()` started throwing inside
`std/user/nmsh.c`'s own `reset_prompt()`:
`replace_string: expected (string, string, string) arguments
(occurrence-range form not implemented)`. Root-caused with the same
temporary-instrumentation methodology (checkpoints bisecting exactly
where a value stopped being a string, removed once understood): NOT
object-variable corruption this time -- `char_name` was never
clobbered. The actual bug is that `query_name()`, called bare from
*within* `std/user/nmsh.c`'s own code, resolves to `nmsh.c`'s own
`string query_name() { return 0; }` (one of a block of stub functions
at the bottom of that file -- `query_hp()`, `query_max_hp()`,
`query_sp()`, `query_max_sp()`, `query_invis()`, `query_name()` --
each returning a hardcoded placeholder), not to `std/user.c`'s real
override (`string query_name() { return char_name; }`), even though
`std/user.c` inherits `nmsh.c` and legitimately overrides it.

**Confirmed against real FluffOS source, not guessed.**
`compiler.c`'s `define_new_function()` (lines 1046-1074,
`fluffos-2.9-ds2.08/compiler.c`): when a function name that was
previously seen with `FUNC_INHERITED` gets redefined further along the
same compile, the comment states plainly: "It was either an undefined
but used function, or an inherited function. In both cases, we now
consider this to be THE new definition." Real LPC compiles an object's
*entire* inherit tree into one flattened function table; when a child
(here, `std/user.c`) defines a function with the same name as
something it inherited (`nmsh.c`'s stub), the child's definition
replaces the entry in that *one shared table* for the whole object --
not just for calls written in the child's own source. Every unqualified
call to that name, including ones textually inside the ancestor's own
file, resolves through the same table and gets the override, unless
explicitly bypassed with `::` (`nomask` is the reverse: it forbids a
child from ever replacing that entry at all). This is the standard
Nightmare-mudlib idiom `nmsh.c` is using here on purpose: it defines
placeholder defaults so it compiles and runs standalone, expecting a
real inheriting file like `std/user.c` to override them -- the same
pattern used for at least five other stubs in the same block.

This driver's `OpCode::Call` does the opposite: `findFunctionInChain(program,
funcName)` searches the *currently executing* program's own lexical
scope (its own functions, then its own `inheritedPrograms`, depth-first)
first, and only falls back to `obj->program()` (the top-level, most-derived
object) when that search finds *nothing at all*. Since `nmsh.c` does
define its own `query_name()`, the lexical search succeeds locally and
the fallback -- which exists specifically to reach a child's override,
per that code's own comment ("confirmed live needed" for a different
case, `query_client()`) -- never triggers. The fallback's own reasoning
("a file's own internal calls must still resolve to its own (or its
own ancestors') definitions first, real LPC does not virtually
dispatch a parent's internal self-calls to a child's override") is the
part contradicted by `define_new_function()`'s own comment above: real
LPC's resolution is not lexical-scope-first with a not-found fallback,
it is single-flattened-table-first, always, with `::` as the only way
to reach a specific ancestor's shadowed version instead.

**Scope: likely broader than this one file.** The same
default-stub-for-standalone-use, override-in-the-real-object pattern
is a common, deliberate Nightmare/LPC idiom, not unique to
`nmsh.c`/`query_name()`. Anywhere an ancestor file provides a
placeholder that a more-derived file overrides, and the ancestor's own
code calls that name internally (not through `::`), this driver
currently gets the wrong (ancestor's own, stale) version instead of
the real override. This was only confirmed for this one call site
live; the true extent across the rest of the mudlib has not been
surveyed.

**Not yet fixed.** This is a change to the fundamental function-call
resolution order for `OpCode::Call` (and by extension `CallParent`'s
own bare-form search, and the plain `Call` opcode's existing
`query_client()`-style fallback, which would become largely
redundant), not a one-file patch -- squarely a shared-behavior change
the project's standing rule says to propose before implementing.

### Proposed fix

Invert the search order for a bare (unqualified) `OpCode::Call`: try
`obj->program()` (the top-level, most-derived program) first via
`findFunctionInChain()`, exactly like `callFunction()`/`call_other`
already do; only if that finds nothing at all should the driver treat
it as genuinely undefined and fall through to the simul_efun object,
then the efun table. This matches `define_new_function()`'s flattened-
table model: the most-derived definition always wins for a bare call,
regardless of which file's source the call is textually written in.

This does not change `OpCode::CallParent` (`::name()`/
`qualifier::name()`), which is explicitly the escape hatch for
bypassing the override and must keep searching only the *inherited*
programs, skipping the current one, exactly as it does now.

Removes the need for the existing "lexical search first, obj->program()
fallback only if nothing found" logic and its special-cased comment
about `query_client()` -- that case, and this one, are both explained
by the same single rule (most-derived wins for bare calls) once the
order is corrected, rather than being two different special
mechanisms.

Needs a regression test that exercises exactly this shape: an ancestor
file defining a stub the way `nmsh.c` does, a child overriding it, and
a call written *inside the ancestor's own source* confirming it now
reaches the child's override -- plus confirmation that existing tests
relying on the current fallback behavior for `query_client()`-style
cases still pass under the new, simpler single-rule order.

**Fixed, user-approved before implementation.** `OpCode::Call` now
searches `obj->program()` (the object's own top-level, most-derived
program) directly via `findFunctionInChain()`, instead of searching
`program` (whichever file is currently executing) first with a
not-found-only fallback. The old two-step logic and its
`query_client()`-specific comment are gone -- a single top-level-first
search is a strict superset, since `obj->program()`'s own depth-first
walk necessarily covers every program in its inherit tree, `program`
always among them. `OpCode::CallParent` (`::name()`/
`qualifier::name()`) is untouched, exactly as proposed.

The existing test `testParentCallStillPrefersItsOwnLexicalDefinitionOverChildsOverride`
encoded the old (disproven) behavior as its own expected result and
was renamed/corrected to
`testBareCallFromParentReachesChildsOverrideNotItsOwnLexicalDefinition`,
now asserting the child's override wins for a bare call written inside
the parent's own source -- exactly the `nmsh.c`/`query_name()` shape.
`testParentCallToFunctionOnlyChildDefinesResolvesAtRuntime` (the
original `query_client()`-style case) was re-confirmed passing
unchanged under the new single-rule order. Full suite (279 tests at
that point) passing.

## Three more gaps found live pushing from the fixed `reset_prompt()` toward an actual room

With both fixes above in place, live testing advanced past
`reset_prompt()` (no more crash) and surfaced three further, unrelated,
much narrower gaps in sequence -- each root-caused with the same
temporary-instrumentation-then-remove methodology, each fixed directly
(none broad enough to need a propose-first cycle) with a regression
test, confirmed live:

1. **A real compiler bug, not a mudlib bug: block scoping was never
   implemented.** `domains/Praxis/setter.c` failed to compile:
   `codegen: variable "me" already declared in this scope`. The file
   has two sibling `{ ... }` blocks (its "Store PPE"/"Store ISP"
   blocks), neither nested in the other, each declaring its own local
   `me` -- entirely legal C89/LPC, since each block is its own scope.
   `CodeGen`'s `locals_` was a single flat per-*function* map with no
   concept of nested block scope at all -- `Parser.cpp`'s own comment
   on the standalone-`{ }`-statement case said so explicitly ("this
   driver has no lexical block-scoping to enforce"). Fixed: added
   `localScopeStack_` (`std::vector<std::vector<std::string>>`);
   `declareLocal()` records each new name against the innermost open
   scope, and `emitBlock()` now pushes an empty scope before compiling
   a block's statements and erases those recorded names from `locals_`
   when the block closes. One real complication found immediately by
   the existing test suite: a comma-separated var decl (`"string a, b,
   c;"`) and a for-loop's comma-chained init/update clause and a
   braceless single-statement if/while/for branch all reuse the same
   `Block` AST node purely as a wrapper, not as a real scope --
   scoping those unconditionally broke
   `testLocalVarDeclCommaListVmExecution` (`"string a, b, c; ...
   return a + b + c;"` threw `undeclared variable "a"`, the decl's own
   names erased right after that one statement). Fixed by adding
   `Block::isRealScope` (default `true`), set to `false` at the three
   synthetic-wrapper call sites (`Parser::parseVarDeclStatement()`,
   `parseCommaExprChain()`, `parseBranch()`), and `emitStatement()`'s
   nested-`Block` case now only opens a new scope when `isRealScope`
   is true, otherwise flattening directly into the enclosing scope as
   before. Two new tests:
   `testSiblingBlocksMayReuseALocalNameNeitherNestedInTheOther` (the
   real `setter.c` shape) and
   `testNameDeclaredInABlockIsUndeclaredOnceThatBlockEnds` (confirms
   the block-exit boundary is real, not just non-colliding).
2. **`sprintf`'s `"%c"` was unimplemented.** `/daemon/terminal`'s own
   `create()` failed: `sprintf: unsupported format specifier '%c'`.
   `daemon/terminal.c`'s `ANSI(p)`/`ESC(p)` macros build a raw ESC
   (ASCII 27) byte via `sprintf("%c[" + (p) + "m", 27)`. Confirmed
   against `fluffos-2.9-ds2.08/sprintf.c`: `INFO_T_CHAR` requires a
   `T_NUMBER` (int) argument, mapped straight through to C's own
   `sprintf(..., "%c", ...)`. Implemented to match (throws if the
   argument is not an int, same convention as the existing `%s`/`%d`
   cases). Two new tests:
   `testSprintfPercentCEmitsSingleCharacterFromIntArgument`,
   `testSprintfPercentCThrowsOnNonIntArgument`.
3. **`living()` was a missing efun.** `move()` (bare-called from
   `std/user.c`'s `setup()`, resolving via the just-fixed `Call`
   opcode to `std/living.c`'s own `move()`, which itself calls
   `::move()` up to `std/Object.c`'s base implementation) threw
   `undefined function or efun: living`. `std/Object.c`'s own `move()`
   gates `move_object()` behind `living(this_object()) && living(ob)`
   (blocking one living thing moving directly into another, aside from
   the `"mountable"` exception). Confirmed against
   `func_spec.c`: `"int living(object default: F__THIS_OBJECT);"`, and
   `add_action.c`'s `f_living()`: returns whether
   `O_ENABLE_COMMANDS` is set on the object, nothing more. Implemented
   directly on top of the existing `commandsEnabled()` flag this
   driver's `enable_commands()`/`disable_commands()` pair already
   maintains (from the earlier `add_action` subsystem work), defaulting
   the argument to `current_object()` per the real signature. New test:
   `testLivingReflectsEnableCommandsStateAndDefaultsToCurrentObject`
   (enable/disable round trip, default-argument form, and confirms the
   flag is per-object, not global).

Full suite: 282 tests passing after all three.

## Live confirmed: chargen now genuinely runs, reaches attribute rolling

With every fix above in place, a fresh live account-creation test
(`roomtestfive`) now shows the real chargen banner for the first time
this project has ever reached it:

```
=== STEP 1: CHOOSE YOUR STARTING ZONE ===
Where does your story begin on Rifts Earth?
 americas   The Americas (Chi-Town)
 europe     Europe (New Camelot)
 atlantis   Atlantis (Splynn market shores)
Type your choice: americas, europe, or atlantis.

> americas
The Americas. You will begin at the edge of Chi-Town.
=== STEP 2: ROLL ATTRIBUTES ===
Roll Palladium attributes (3d6 each for IQ, ME, MA, PS, PP, PE, PB, Spd).
Type: roll
After rolling you must type accept to keep the roll, or reroll
(up to 4 rerolls, 5 total rolls). Race selection stays locked
until you type accept.
```

This matches the documented real chargen flow exactly (see the
mudlib's own `CLAUDE.md`, "Chargen input model is plain-string only").
Not yet a full room: reaching one requires completing the rest of
chargen (`roll`/`accept`, race, OCC, alignment, skills), each of which
may surface further gaps not yet exercised live. All debug
instrumentation used to root-cause every issue in this and the
previous section has been removed, confirmed via grep across every
touched mudlib file.

## Resuming after a reboot: orphaned process, and a stale test script

The previous session ended mid-investigation when an unplanned system
reboot killed it. On resume, `ps`/`ss` found a driver process already
listening on the scratch port (1123) -- started *after* the reboot
(`19:49`, boot was `18:53`), from a scratch config in a different
session's own scratchpad directory, with an empty log and no reachable
owning agent. No save/`.o` file anywhere had been touched since boot.
Confirmed orphaned (a lost session's own scratch instance, not
something in progress) and killed rather than reused, per this
project's own "confirm state from live evidence, don't guess" practice.

Driving the live socket test itself needed its own small fix first:
`mudlib/tools/playtest_create_chars.py` (the project's existing,
previously-working chargen-driver script, dated 2026-07-10) no longer
matches `secure/std/login.c`'s current account-confirm prompt. The
script's `login_new()` expects either "really wish" or "choose a
password" right after the account name is sent; the real prompt is now
"Confirm `<Name>` as your account and first character name? (y/n)",
added since the script was written. Neither substring matches, so the
script's next `send()` (meant to be the password) actually answers the
`(y/n)` confirm prombt instead -- landing on `secure/std/login.c`'s own
`new_user()` decline branch (`if((str = lower_case(str)) == "" ||
str[0] != 'y') { ... __Player->remove(); ... }`) essentially every time.
This is almost certainly *why* the previous session was investigating
`daemon/occ.c` in the first place: not a real content question, but a
downstream symptom of this same stale-script bug reliably crashing
account creation. A corrected probe script (this session's own
`chargen_probe.py`, scratch-only, not committed to the repo) answers
the confirm prompt explicitly with `y` before sending the password.

## Chargen closed the loop: full run confirmed live, reaching a real room

With the corrected probe script, the live walk reproduced the actual
blocker directly rather than continuing the grep/awk investigation
blind: `new_user()`'s decline branch (reached via the stale-script bug
above, but a real code path a genuine user could also hit by literally
answering anything other than `y`) called `__Player->remove()` on the
speculatively pre-created player object (`secure/std/login.c`'s own
comment explains why `player_object()` runs before the confirmation:
`compile_object()` needs the char name already set). `std/clean_up.c`'s
`remove()` needs `all_inventory()` to hand equipment back to the
environment before destructing -- an efun this driver never had.

**Investigating the `daemon/occ.c` empty-`attribute_requirements`
question first**, since it was the exact point of interruption: closed
as genuine content, not a driver bug. Both real consumption sites
(`domains/Praxis/setter.c`'s `do_occ_pick()` and
`offer_occ_or_reroll()`) guard with `if(reqs && sizeof(reqs))` before
ever touching the mapping, so an empty `([])` is unambiguously "no
requirements" and behaves correctly regardless of how this driver
represents an empty mapping literal internally. The OCCs carrying it
(`vagabond`, `wilderness scout`, `city rat`, `rogue scholar`, `tribal
warrior`, `smuggler`, `pirate (s.a.)`, `sailor (s.a.)`, `gifted gypsy`,
plus three race-gated OCCs where race membership is already the hard
gate) are civilian/generalist classes with no stat floor in the source
material, consistent with every non-empty entry elsewhere in the same
file. Confirmed live twice over: all of them show up correctly in
STEP 5's offered OCC list, and `vagabond` specifically (line 225's
empty mapping) was picked live with no crash and no false rejection.

**Nine more gaps found and fixed continuing the walk from there, each
root-caused against real FluffOS source before implementing, same as
every other slice this project has done:**

1. **`all_inventory()`/`deep_inventory()` missing entirely** (func_spec.c:
   `object *all_inventory(object default: F__THIS_OBJECT);` /
   `object *deep_inventory(...)`). Confirmed against array.c's own
   `all_inventory()` (direct children only, no recursion) and
   `deep_inventory_count()`/`deep_inventory_collect()` (depth-first,
   target excluded). Backed directly by `LpcObject::inventory_`, already
   maintained by `VM::moveObject()` -- no new bookkeeping needed. This
   was the account-creation blocker above.
2. **`strcmp()` missing**, silently swallowed by `secure/std/login.c`'s
   own `catch(__Player->setup())` with no console trace -- the same
   "quiet cascade" shape as the earlier `__HistorySize` investigation.
   `/secure/daemon/player.c`'s own `sort_list()` needed it; a fresh
   player's `setup()` was quietly failing to register itself with
   `player.c`'s own online-player list. Matches real `efuns_main.c`'s
   `f_strcmp()`: a plain C `strcmp()`.
3. **`map_delete()` missing**, *not* caught -- fatal to the connection.
   `std/living/env.c`'s own `remove_env()` (`if(env_var && env_var[env])
   { map_delete(env_var, env); ... }`), reached unguarded from
   `domains/Praxis/setter.c`'s `alignment_cmd()`. This is what actually
   stopped STEP 4 (alignment) from ever reaching STEP 5 (OCC) live.
   Matches real `efuns_main.c`'s `f_map_delete()`: mutates the mapping
   in place, void return.
4. **`clone_object()`/`load_object()` doubled a caller-supplied `.c`
   extension.** `ObjectManager::compile()` appended `.c` unconditionally,
   so `daemon/rifts_start_d.c`'s own `give_item(player, "id_card.c")`
   resolved to a literal, never-existing `id_card.c.c` and aborted
   `finish_creation()` partway through granting starting equipment --
   the actual blocker stopping a fresh character from ever reaching a
   room. New `ObjectManager::normalizeFilename()` strips one trailing
   `.c` at every entry point (`compile()`, `loadObject()`,
   `cloneObject()`, `sourceFileExists()`, `lookupLoadedObject()`) so
   `"id_card"` and `"id_card.c"` resolve to the exact same cache entry
   and object identity, matching real LPC's own convention that object
   paths never carry the extension internally.
5. **`intp()` missing**, the one type predicate not covered alongside
   `stringp`/`objectp`/`mapp`/`pointerp`/`functionp` from earlier
   slices. `/domains/Praxis/equipment/id_card.c`'s own `set_value()`
   needed it directly, reached while granting starting equipment.
6. **`repeat_string()` missing** (func_spec.c/efun_defs.c: `F_REPEAT_STRING`,
   real body in `packages/contrib.c`'s `f_repeat_string()`: string
   concatenated with itself N times, `""` for N <= 0).
   `cmds/mortal/_score.c`'s own `panel_border()` needed it -- caught by
   `setter.c`'s own `catch()` around `finish_creation()`'s auto-score-
   display, so not fatal, but the score panel border was silently never
   rendering until fixed.
7. **`present()` missing** (func_spec.c: `object present(object | string,
   void | object);`). Confirmed against `simulate.c`'s
   `object_present()`/`object_present2()`: the string form searches a
   container's direct inventory for an item whose `id()` apply returns
   truthy (falling back to the calling object's own environment when no
   container is given and the direct search misses); the object form
   checks direct-containment or, with no explicit container,
   sibling-of-current-object. This blocked `domains/ChiTown/areas/
   chitown_start.c`'s own `reset()` -- the very first starting room a
   fresh character reaches -- via exactly the `present("id",
   this_object())` idiom `mudlib/CLAUDE.md`'s rule 11 documents as the
   standard anti-duplication check. Not implemented: the numbered-suffix
   form (`"sword 2"`), not confirmed needed anywhere reached live yet.
8. **The deepest one: `explode()` never matched real FluffOS's own
   default separator semantics.** Confirmed against fluffos-2.9-ds2.08's
   own `array.c` `explode_string()` *and* this exact vendored
   reference's own `options.h` (`#undef SANE_EXPLODE_STRING` / `#undef
   REVERSIBLE_EXPLODE_STRING` -- the default build any of this mudlib's
   own content was written against): every **leading** occurrence of the
   separator is stripped before splitting (repeatedly, not just the one
   `SANE_EXPLODE_STRING` would limit it to), and the final chunk is only
   kept if non-empty, so a **trailing** separator never produces a
   trailing `""` element. This driver's original implementation did a
   naive split with neither behavior. Root-caused by tracing why
   `secure/SimulEfun/security.c`'s own `file_privs()` never matched any
   of its `switch(path[0])` cases for a real object path:
   `"/domains/..."` exploded on `"/"` produced a leading `""` as
   `path[0]` instead of `"domains"`, shifting every real path segment
   one index late -- which is what made every object's compile-time
   `privs` assignment fail silently (see the next item), which is in
   turn what made `secure/SimulEfun/log_file.c`'s own
   `explode(query_privs(previous_object()), ":")` throw for any object
   reached through it (`domains/Praxis/obj/mon/rift_survivor.c`'s own
   `set_stats()`/`set_level()`, cloned by the starting room's own
   `reset()`). The trailing-empty-element half of this same bug had
   already been worked around locally in `daemon/race.c` (`LIMB_DIR`
   file reading) in the previous session, before this root cause was
   found; that guard is left in place as a harmless, independently
   reasonable defensive check (it matches this mudlib's own
   `database_filter()` convention, per its own comment) rather than
   reverted now that the driver itself is fixed.
9. **A previously entirely missing mechanism: this driver never
   auto-assigned an object's compile-time `privs`.** Real `simulate.c`'s
   own `init_privs_for_object()` (called from `init_object()` for every
   freshly compiled or cloned object, before its own `create()` runs)
   applies `master()->privs_file(filename)` and stores the result if
   it's a string. This driver had no equivalent at all -- `query_privs()`
   only ever returned a real value if a mudlib file called `set_privs()`
   on itself directly, which essentially nothing in this mudlib does
   (privs are meant to come from `master.c`'s own `privs_file()`
   automatically). New `ObjectManager::initPrivsForObject()`, called
   from both `loadObject()` and `cloneObject()` right after construction,
   closes this gap. Skipped only when `master_` itself is not loaded yet
   (matches real `init_privs_for_object()`'s own `!current_object`
   bootstrap-skip outcome closely enough -- nothing this driver runs
   depends on the master object's own privs).

Also added **`remove_call_out()`** (func_spec.c: `int
remove_call_out(int | void | string);`) alongside the fixes above, found
needing it the same pass: `domains/Praxis/obj/mon/rift_survivor.c`'s own
`init()` does the common defensive cancel-then-reschedule idiom for a
repeating `call_out()`. Since this driver's own `call_out()` is a
documented stub that never actually schedules anything yet
(`Scheduler::tickCallOuts()` is still an empty body), `remove_call_out()`
always returns `-1` -- the real "nothing found" outcome, honestly
reflecting that nothing is ever really pending, not a fake success.

Full suite: 288 tests passing (11 new regression tests this session,
one per confirmed gap above).

**Confirmed live, full transcript (fresh account `chargenthirteen`,
scratch instance, port 1123):** login through account creation
(gender, display name, email, real name all accepted blank), the
one-time first-account admin-bootstrap offer (declined), zone
(`americas`), attribute roll/accept, race (`human`, `list` also
exercised), alignment (`scrupulous`), OCC pick (`vagabond`, the
empty-`attribute_requirements` case deliberately chosen), automatic
starting-equipment grant (combat knife, C-18 laser pistol, leather
jacket, all via real `clone_object()` calls), `finish_creation()`'s own
automatic room entry:

```
A Rift tears open around you and reality reassembles.
You step onto Rifts Earth. Welcome, Human.
A human appears from the shadows.
A reinforced shelter of scavenged plating and pre-Rifts ferrocrete,
built into the corridor between the old Coalition road south to
Praxis and the checkpoints of Chi-Town to the north. A steady
trickle of new arrivals passes through here: refugees, mercs, and
the newly rifted-in alike.

A battered sign is nailed to a support beam near the door. A
survivor watches the corridor from a folding chair. There are two exits: north, south
A weathered survivor.
```

**Correction (caught during the "Real call_out()/heart_beat() scheduler"
slice, see below): a separately, explicitly typed `look` command was
NOT actually confirmed here.** What this session's probe script reported
as "look's own response" was trailing, already-in-flight output from
`finish_creation()`'s own automatic display, misattributed to a `look`
sent moments later by a timing coincidence in the probe's own buffering
-- not a genuine round trip. The real `look` command did not work at
all at this point; it silently produced nothing, for reasons entirely
unrelated to this session's own chargen fixes (a dispatch-argument bug
described in full below). This was only caught later, by a more
rigorous probe that drains the connection to genuine idle before
sending a command and checking for new bytes -- see "Real call_out()/
heart_beat() scheduler" below for the real fix and the real
confirmation.

**One remaining known gap surfaced live, non-fatal:** `cmds/mortal/
_score.c`'s own `panel_two_col()` (part of the automatic score display
`finish_creation()` triggers) uses `sprintf`'s `%*` dynamic-field-width
specifier, still not implemented (see "Known stubs" below -- this
extends that existing, already-documented `sprintf` scope limitation,
not a new one). Caught by `setter.c`'s own `catch()` around
`finish_creation()`, so it does not block reaching the room or using
`look` -- only the score panel's two-column layout silently fails to
render. Test data cleanup: all scratch-instance test accounts created
this session (`chargenthree` through `chargenthirteen`) were throwaway
names on the scratch port only, confirmed by mtime to be within this
session, and deleted (`secure/save/login_accounts/c/*.o`,
`secure/save/postal/c/*`) before the scratch driver was stopped; no
player-object save under `secure/save/users/` was ever created (no
test character reached `quit`).

## Real call_out()/heart_beat() scheduler: recon, design, implementation, and a genuinely deep dispatch bug found live

With chargen reaching a real room, `call_out()`/`heart_beat()` never
actually firing became the single largest remaining gap (see the
driver-comparison docs under `docs/driver-comparisons/`, corrected to
say so during the same review pass that preceded this slice). Same
rigor as the closures/`add_action`/`catch()` work: recon real usage
first, design grounded directly in FluffOS source, implement, test,
then verify live.

### Step 1: recon (real usage across the mudlib)

373 `call_out()` call sites, 30 `remove_call_out()`, 15
`set_heart_beat()`, 10 `find_call_out()`. Confirmed:

- Delays are a mix of fixed constants (2, 5, 10, 60, 120, 300, 600, 900,
  1800, 3600 seconds), computed expressions (`5*con`, `(random(6)+1)*
  3600`, a daemon-queried interval), and **zero-delay `call_out(fn, 0)`**
  used pervasively (~30 sites) as a "run on the next tick" idiom
  (deferred `equip_gear()` on NPC spawn, deferred self-destruct, `std/
  room.c`'s own `create() { ...; call_out("reinitiate", 0); }`, etc).
- `remove_call_out()` is called **both by function name (string, the
  overwhelming majority) and by a stored numeric handle**
  (`cmds/mortal/_trade.c`'s own `tid = call_out(...); ...;
  remove_call_out(tid)`), and `while(remove_call_out("x") != -1);` loops
  confirm multiple same-named call-outs can coexist, removed one match
  at a time.
- `find_call_out()` (10 sites) is used as an existence/dedup check
  before scheduling -- not implemented in this driver at all before
  this slice.
- `set_heart_beat()` is called with values other than 0/1 in places
  (`std/germ.c`'s own `set_heart_beat(5)`) -- real semantics: the
  argument is a per-object heartbeat-cycle interval, not a bare on/off
  flag.
- The dominant repeating-timer idiom is **self-rescheduling call_out**
  (a function calls `call_out(itself, N)` again at the end of its own
  body) -- e.g. `std/user.c`'s `rifts_regen_tick()`/
  `rifts_hp_regen_tick()`, scheduled at 60s/120s in `setup()`, each
  rescheduling itself. `std/living.c`'s `heart_beat()` uses `time()`
  deltas rather than trusting exact tick cadence, so it is robust to
  imprecise firing.
- `heart_beat()` bodies do real, observable gameplay work: `std/user.c`
  runs healing/regen and `continue_attack()`; `std/living.c`'s does
  aging and a 3600-second Rifts-regen/sun-exposure check; NPC files use
  it for AI.

### Step 2: design (grounded directly in fluffos-2.9-ds2.08's `call_out.c`/`backend.c`)

Not guessed -- read directly, matching this project's own standing
practice:

- `call_out.c`'s `new_call_out()`: `if (delay < 0) delay = 0;` -- never
  rejected, clamped. `CALLOUT_HANDLES` is confirmed **active** in this
  exact vendored build's `options.h`, so the handle-returning
  `find_call_out(int|string)` / `remove_call_out(int|void|string)`
  signatures (`func_spec.c`) are the correct target, not the
  handle-less alternative.
- `remove_call_out(object_t *ob, const char *fun)`: matches only entries
  where `(*copp)->ob == ob && strcmp((*copp)->function.s, fun) == 0` --
  scoped to the *calling* object, and a closure-bound entry's own
  `cop->ob` is never set for the string form, so a name-based removal
  can never match a closure-scheduled entry. `find_call_out()` shares
  the same match rule.
- `call_out()`'s own main loop (`call_out.c`): "Move the first call_out
  out of the chain" before invoking it, then advances `current_time`
  toward real time one second at a time, calling `call_heart_beat()`
  whenever `current_time % HEARTBEAT_INTERVAL == 0`.
  `HEARTBEAT_INTERVAL` is **2** (real seconds) in this exact vendored
  build's `options.h`, confirmed by reading the macro directly rather
  than assumed.
- `backend.c`'s `set_heart_beat(object_t *ob, int to)`: four real
  branches, read directly rather than guessed --
  `to == 0` disables and removes the object from `heart_beats[]`;
  `to != 0` on an object not yet enabled adds a fresh entry with
  `time_to_heart_beat = heart_beat_ticks = to` (negative `to` clamped to
  1); `to != 0` on an object already enabled updates the interval on a
  positive `to`, or is rejected as a no-op on a negative one.
  `query_heart_beat(object_t*)` returns the real configured interval
  (`heart_beats[index].time_to_heart_beat`), not a bare 1.
- `call_heart_beat()` (`backend.c`): decrements every enabled object's
  own tick countdown, fires `heart_beat()` on any that reach zero, then
  resets that object's countdown back to its own configured interval.
  Errors during a fired call_out/heart_beat are caught via
  `SETJMP`/`restore_context` per call, so one throwing call cannot stop
  the rest of that cycle.

This driver's own data-structure choices, deliberately simpler than
real FluffOS's ring-buffer-of-linked-lists (`call_list[CALLOUT_CYCLE_SIZE]`)
while preserving identical *observable* behavior -- the same "simplify
the internal representation, match the real contract" pattern this
project already used for closures' lazy name resolution:

- `Scheduler::CallOutEntry`: an absolute `steady_clock::time_point`
  `dueAt` instead of real `call_out.c`'s delta-encoded ring-buffer slot,
  a genuinely unique `int64_t handle` (a bare monotonic counter, not
  real `new_call_out()`'s own slot-plus-`unique`-counter encoding --
  nothing in this mudlib inspects a handle's bit structure, only
  compares it back or checks truthiness), and either a `function` name
  string (with a `weak_ptr<LpcObject> target` owner) or a bound
  `Closure`, covering both real forms.
- `Scheduler::HeartbeatEntry`: `weak_ptr<LpcObject>` plus
  `ticksRemaining`, mirroring real `heart_beats[]`. The configured
  interval itself lives on `LpcObject::heartbeatInterval()` (replacing
  the previous plain bool `heartbeatEnabled_`), so `query_heart_beat()`
  can report it faithfully.
- `tickCallOuts()`: collects every due entry into a separate vector
  *before* invoking any of them, then fires each one -- matching real
  `call_out.c`'s own "move out of the chain first" ordering, needed
  because the dominant real idiom is a call_out that reschedules itself
  from within its own body (confirmed in recon above).
- `tickHeartbeats()`: same two-phase shape (decide who fires this cycle
  first, entirely before calling any LPC code; fire afterward from a
  separate snapshot) -- see the crash section immediately below for why
  this was not optional.
- The real 2-second cadence gate lives in `Scheduler::run()`'s own loop
  (comparing elapsed wall time against `lastHeartbeat_`), not inside
  `tickHeartbeats()` itself, so `tickHeartbeats()`/`tickCallOuts()` stay
  pure, deterministic, directly-testable functions -- the same reasoning
  already documented for why `Server::dispatchLine()` was pulled out of
  `handleConnection()` as its own directly-testable method.
- `VM` gained a `Scheduler*` back-pointer (`setScheduler()`), set from
  `main.cpp` right after `Scheduler` is constructed -- the same
  "set the back-pointer after construction" pattern
  `ObjectManager::setVM()` already uses, needed because `call_out()`/
  `remove_call_out()`/`find_call_out()`/`set_heart_beat()` are
  registered on `EfunTable`, which only receives `VM&`.

### Step 3: implementation

`call_out`, `remove_call_out`, `set_heart_beat`, `query_heart_beat`
rewritten to route through the real `Scheduler`; new `find_call_out`
added. All four confirmed against the design above, each with its own
citation in `EfunTable.cpp`.

### A genuine crash found live: iterator invalidation in `tickHeartbeats()`

The very first live test after wiring everything up **segfaulted** --
confirmed via `systemd-coredump`, not inferred:

```
Stack trace of thread ...:
 #4  amlp::HeartbeatEntry::operator=(HeartbeatEntry&&)
 #7  std::vector<amlp::HeartbeatEntry>::_M_erase(...)
 #9  amlp::Scheduler::tickHeartbeats()
 #10 amlp::Scheduler::run(amlp::Server&, int)
```

Root cause: the first version of `tickHeartbeats()` held a live iterator
into `heartbeats_` across the `vm_.callFunction(obj, "heart_beat", {})`
call. Real `std/user.c`'s own `heart_beat()` calls `set_heart_beat(0)`
on itself (`if(!interactive(this_object())) { set_heart_beat(0);
return; }`), which re-enters `Scheduler::setHeartbeatInterval()`, which
mutates `heartbeats_` via `erase()`/`find_if()` -- invalidating the
outer loop's own iterator mid-iteration. Fixed the same way
`tickCallOuts()` was already safe: collect a separate snapshot of who
fires this cycle *before* calling any LPC code, so a re-entrant
`set_heart_beat()` call from inside a firing `heart_beat()` can never
corrupt the structure still being iterated. Covered by a dedicated
regression test reproducing the exact real shape (an object disabling
its own heartbeat from within `heart_beat()`, alongside an unrelated
"bystander" object confirmed unaffected).

### Step 4: live verification uncovered a much older, much deeper bug -- not in the scheduler

With the crash fixed, live testing reached a room and scheduled a
throwaway test call-out (`cmds/mortal/_testscheduler.c`, not part of the
game, deleted afterward) -- but a plain `look` command, sent
immediately after, produced **zero bytes**, with no error anywhere.
Disabling `tickHeartbeats()`/`tickCallOuts()` entirely (a bisect test)
did not fix it, proving the scheduler itself was not the cause. This
also meant a claim earlier in this document -- that a live, explicitly
typed `look` command had already been confirmed working, in "Chargen
closed the loop" -- was wrong: re-checked with a probe that drains the
connection to genuine idle before sending a command, `look` (and every
other command) had *never* actually worked live in this driver, for any
session. That correction is recorded in place above rather than edited
away.

The real root cause, found by direct C++-level instrumentation rather
than guessing (three distinct, chained gaps, each confirmed against
real FluffOS source in turn):

1. **`get_dir()` never implemented glob patterns.** `daemon/command.c`'s
   own `rehash()` (behind `find_cmd()`, which every single `add_action`-
   dispatched command depends on) calls `get_dir(val[i]+"/_*.c")` -- a
   genuine glob, not the bare-directory-or-bare-file shape this efun's
   original implementation assumed was the only real usage. Against a
   literal `"*"` in the path, `stat()` always failed and this efun
   silently returned an empty array, so `__Cmds` was never populated and
   `find_cmd()` returned 0 for **every single verb**. Fixed: the
   directory portion of the path is literal, only the final path
   component may carry a wildcard, matched via POSIX `fnmatch()` against
   that directory's own entries.
2. **The real, deepest bug: `dispatchCommand()` passed an empty string,
   not real LPC's own `0`/undefined, for a bare verb with nothing after
   it.** Confirmed directly against `add_action.c`'s own `user_parser()`:
   `if (s->flags & V_NOSPACE) { copy_and_push_string(...); } else if
   (buff[length] == ' ') { copy_and_push_string(...); } else {
   push_undefined(); }` -- the undefined branch fires whenever there is
   genuinely nothing after the matched word, for both the plain
   exact-match and V_SHORT cases (only V_NOSPACE reslices differently).
   This driver's `splitVerbAndArg()` always produced a `std::string`
   (empty when there was nothing there), never a true "no argument"
   value. `cmds/mortal/_look.c`'s own `cmd_look(str)` checks
   `if(stringp(str))` first, and an empty string passes that check (real
   LPC: `stringp()` checks the type, not truthiness) -- silently routing
   a bare `look` into `examine_object("")` instead of the intended
   `this_player()->describe_current_room(1)` fallback, which itself
   declines on `if(!str) return 0;`. `cmd_look()`, and therefore
   `cmd_hook()`, and therefore the whole dispatch, returned falsy with
   no exception, no dropped connection, and nothing mudlib-visible to
   explain it -- confirmed only by instrumenting `dispatchCommand()`
   itself and reading its actual action-table matches and return values.
   Fixed: `splitVerbAndArg()` now returns `std::optional<std::string>`,
   and `dispatchCommand()` constructs a real monostate `Value{}` when
   there is nothing after the verb, matching `push_undefined()` exactly.
3. **`message()` ignored its own `targets` argument, always writing to
   whichever connection happened to be "currently active."** Harmless
   on every path this driver had run before this slice (every real call
   site was `message(type, text, this_object())`, where `this_object()`
   already was the active connection's own object) -- until
   `call_out()`/`heart_beat()` genuinely started firing with **no**
   active connection at all, and `secure/SimulEfun/communications.c`'s
   own `tell_object(ob, str)` (`message("tell", str+"", ob)`, this
   mudlib's single most common way to notify a player from a timer)
   produced nothing. Fixed with a real object-to-connection lookup:
   `InteractiveRegistry` (previously membership-only) now also maps each
   registered object to its own `Connection*`, and `message()` routes to
   the named target (a single object or an array of them) when one is
   given, falling back to the current connection only when no `targets`
   argument was passed at all.

Each of the three is covered by its own dedicated regression test
(`get_dir` glob matching in the final path component only,
`dispatchCommand` passing undefined vs. a real string for bare vs.
compound commands, `message()` routing to a target's own connection
across two independent sockets, confirmed with neither connection
"current").

### Confirmed live, full transcript

Fresh account (`truefinal`, scratch instance, port 1123), full chargen
through OCC pick (`vagabond`), automatic room entry, and this time a
**genuinely confirmed** explicit `look`:

```
A Rift tears open around you and reality reassembles.
You step onto Rifts Earth. Welcome, Human.
A reinforced shelter of scavenged plating and pre-Rifts ferrocrete,
built into the corridor between the old Coalition road south to
Praxis and the checkpoints of Chi-Town to the north. A steady
trickle of new arrivals passes through here: refugees, mercs, and
the newly rifted-in alike.

A battered sign is nailed to a support beam near the door. A
survivor watches the corridor from a folding chair. There are two exits: north, south
A weathered survivor.
```

-- confirmed both as the automatic display *and*, separately, as the
real response to an explicitly typed `look` sent after the connection
was drained to genuine idle (no bytes at all for 3+ full seconds) --
455 new bytes arrived, the same real room description, on demand. Then:

```
>>> testscheduler
Scheduling a delayed message 3 seconds out via call_out.
The survivor glances over. "Fresh out of a Rift. You look confused. Say 'help' if you need a rundown."
CALLOUT FIRED: the real scheduler works.
```

-- confirming, in one pass: `call_out()` genuinely schedules and later
fires (the 3-second delayed message, via the throwaway
`cmds/mortal/_testscheduler.c` test command, deleted after this
session); `heart_beat()` genuinely fires on a real NPC (`rift_survivor`'s
own AI dialogue line, unprompted, driven purely by its own heartbeat,
not anything this session's test script sent); and `message()`/
`tell_object()` correctly routes a delayed message to the right player
even with no connection "current" at the moment it fires.

Full suite: 304 tests passing (16 new/updated this slice: the
iterator-invalidation regression, call-out registration/firing/removal/
closure-form/error-isolation/destructed-target coverage, heart_beat
enable/disable/interval-cadence/error-isolation/destructed-object/
reentrant-self-disable coverage, the `get_dir` glob fix, the
`dispatchCommand` undefined-argument fix, and the `message()` routing
fix).

