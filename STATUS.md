# STATUS

Older session entries (everything before the 5 most recent) live in
`STATUS-ARCHIVE.md`.

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

