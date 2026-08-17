# STATUS

Older session entries (everything before the 5 most recent) live in
`STATUS-ARCHIVE.md`.

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
