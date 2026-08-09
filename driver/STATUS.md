# STATUS

Older session entries (everything before the 5 most recent) live in
`docs/STATUS-ARCHIVE.md` (mirrored at `driver/STATUS-ARCHIVE.md`).

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
- Postfix/prefix `++`/`--` only support a bare variable name target, not
  an index expression (`arr[i]++`).
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
- `sprintf()` implements bare `%s`/`%d`/`%c` (the third added later, see
  "Three more gaps found live" above), positionally, with no field
  width/precision/flags and no literal `%%` -- throws on anything else.
  Confirmed still missing live this session: `%*` (dynamic field width),
  needed by `cmds/mortal/_score.c`'s own `panel_two_col()` -- caught by
  `setter.c`'s own `catch()` around `finish_creation()`'s automatic
  score display, so non-fatal (the score panel's two-column layout
  silently fails to render), not blocking reaching a room or `look`.
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
  object that was once connected and has since disconnected~~ --
  `userp()`/`query_once_interactive()` fixed, see the new dated entry at
  the top of this file: a real, sticky `LpcObject::wasEverInteractive()`
  flag, set once by `Connection::attach()` and never cleared.
  `interactive()`/`find_player()`/`users()` are unchanged and still
  correctly scoped to currently-connected objects only, matching real
  FluffOS semantics for those three. Real living-name-table concepts
  (`set_living_name()`'s own lookup, see its own bullet below) remain a
  separate, still-open simplification.
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
  former environment but not relocated anywhere. Also still not done:
  the broader "any stale object-typed value silently reads back as 0"
  semantics real FluffOS enforces at many more read sites (array/mapping
  entries, comparisons, etc, not just applies) -- this driver only gates
  the actual call/apply entry points, not every place an object
  reference could be read.
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
- `set_living_name()` stores the name on the object but wires up no
  lookup table for it, matching `find_player()`'s own pre-existing
  simplification (InteractiveRegistry + `query_name()`, not a real
  living-name table).
