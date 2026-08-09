# STATUS

Older session entries (everything before the 5 most recent) live in
`docs/STATUS-ARCHIVE.md` (mirrored at `driver/STATUS-ARCHIVE.md`).

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
