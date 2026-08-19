# STATUS

Older session entries (everything before the 5 most recent) live in
`STATUS-ARCHIVE.md`.

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
