# STATUS

Older session entries (everything before the 5 most recent) live in
`STATUS-ARCHIVE.md`.

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
