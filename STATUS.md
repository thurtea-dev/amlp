# STATUS

Older session entries (everything before the 5 most recent) live in
`STATUS-ARCHIVE.md`.

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
