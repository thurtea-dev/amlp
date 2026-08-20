# STATUS

Older session entries (everything before the 5 most recent) live in
`STATUS-ARCHIVE.md`.

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
(confirmed both non-negotiable rules -- `git add` only, no commits/pushes;
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
counted in row 1.7's own prior stock-take) -- the only other hits found
anywhere are `temp/ldmud`'s own bundled driver test fixtures
(`test/t-deprecated-sefuns/master.c`, `test/t-language/master.c`), the
driver vendor's own regression scaffolding, not independent gameplay
corpora. Confirmed real FluffOS has the identical reset/clean_up
mechanism too (`temp/reference/fluffos-2.9-ds2.08/backend.c:196-302`'s
own `look_for_objects_to_swap()`, `object.c:1896-1927`'s own
`reset_object()`/`call_create()` -- same `O_WILL_RESET`/`O_RESET_STATE`/
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
running at creation) -- `cloneObject()` also sets the new `isClone()`
flag unconditionally, matching real `O_CLONE`.

New `Scheduler::tickResetsAndCleanup()`, gated by the same real
`kHeartbeatCycle` 2-second window `ALARM_TIME`'s own real doc comment
says reset/clean_up genuinely share with heart_beat -- dispatches
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
`ready_for_clean_up` latch has no exact analog -- the more conservative,
never-double-touch choice, and the one place the two real drivers'
own sections do not agree byte-for-byte. Real per-cycle cross-object
batching (LDMud's `!did_reset`, FluffOS's whole-list-every-5-minutes
sweep) is deliberately not replicated -- a real-driver performance
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
bodies that `write_file()` a log -- removed afterward): `TIME_TO_RESET`/
`TIME_TO_CLEAN_UP` temporarily shrunk to 4/6 real seconds for this one
verification build only (reverted to the real 1800/3600 immediately
after, full suite re-confirmed passing at both settings) so a real
30-60-minute wait was not required to observe a genuine timer-driven
fire. The real `Scheduler::tickResetsAndCleanup()` timer fired
`reset()` once on the newly cloned object, then `clean_up()` once with
the real clone argument (`0`), both observed purely by polling the log
file the LPC bodies themselves wrote -- `eval` used only to clone/touch
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
kept command file -- `who.c`/`say.c`/`quit.c`/`shutdown.c`) had been
dormant dead code until this session, since this driver never called
`clean_up()` at all before now -- confirmed it does NOT fire
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
drivers by design -- that is its whole purpose) refreshed in place with
today's real numbers rather than 2026-08-18's stale ones: Phase 0 is
now 16/16 (100%, `parse_*`/row 0.13a's own checkbox flipped since this
file was last updated -- all 8 real efun names implemented including
real two-object `OBJ`/`LIV`/`OBS`/`LVS` matching, confirmed by reading
`STATUS.md`'s own strictly-chronological log directly rather than
trusting an ambiguous-looking mid-cell note in `ROADMAP.md` that turned
out to describe an intermediate, since-superseded state); Phase 1 real
blockers now 5/11 (45%, up from 4/11, row 1.7 itself having since
flipped to checked-partial); efun surface 248 of 270 (up from 247);
test count 694 (this session's own baseline before the reset/clean_up
work above, itself now 700). Explicitly scoped by the user to README.md
+ INSTALL.md + CREDITS.md + COMPARISON.md only -- `ROADMAP.md`/
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
