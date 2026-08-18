# library mudlib -- rename/rebuild plan (2026-08-18)

**Executed 2026-08-18 (continued).** Everything below is the plan as
originally scoped, kept as the historical record of what was decided
and why -- not rewritten to describe the after-state. See STATUS.md for
the full account of what actually happened, including two real gaps
this plan's own "drop" analysis got wrong, only caught by actually
booting the rebuilt driver rather than trusting the static grep this
plan relied on: `log/` (dropped here as unread by anything kept, but
`single/master.c`'s own `log_error()` genuinely writes to `LOG_DIR`) and
`inherit/clean_up.c` (not listed in either table here at all --
`include/command.h`, a kept file, does `inherit CLEAN_UP;`, needed by
every command file that includes it). Both restored during execution.
The "Undecided" section's own dest.c/rm.c/update.c/codefor.c question
resolved to drop, matching this document's own stated default of
"minimal... nothing else unless explicitly kept".

Originally: scoping and planning pass only. Nothing has been moved,
renamed, or deleted. Written per an explicit request to report a
concrete before/after file plan before touching anything -- timing is
"after current ROADMAP work settles", not that session.

## Why this needs a plan rather than a straight rename

`mudlib/` is not something this project authored. `mudlib/readme`'s own
text is the real upstream Lil readme ("a very minimal mudlib named Lil
intended for use in bootstrapping a mudlib to be built from scratch"),
and `STATUS.md` has used it repeatedly, across many sessions, as a real
third-party conformance corpus -- diffing `EfunTable.cpp`'s registered
efun names against Lil's own real efun test suite
(`mudlib/single/tests/efuns/*.c`, one file per efun, confirmed by
`mudlib/readme`'s own description of that directory), and confirming
genuine mudlib compatibility end to end (real login, real commands)
rather than only synthetic test fixtures. Renaming/rebuilding this in
place would throw that value away unless the current tree is preserved
somewhere first.

## Step 1: preserve the current tree, untouched

Move (not copy-and-leave-both) the entirety of the current `mudlib/`
to `temp/lil/`, matching this project's own existing convention for
vendored reference material: `temp/core-lib/`, `temp/dead-souls/`,
`temp/lima/`, `temp/nightmare3/`, `temp/es2_mudlib/`, `temp/wiz_tools/`
are all real vendored mudlib/tool corpora that already live directly
under `temp/` (not `temp/reference/`, which `CLAUDE.md` reserves
specifically for the vendored FluffOS *driver* source,
`temp/reference/fluffos-2.9-ds2.08/`), gitignored (`.gitignore`'s own
`temp/` line), cited by path but never modified going forward.

`temp/lil/` becomes the new home for the efun-conformance-diffing work
(`single/tests/efuns/*.c`, `single/tests/compiler/*.c`) and for cross-
checking any future real-mudlib-compatibility question against genuine
Lil behavior -- exactly the role `mudlib/` played before, just moved to
where every other vendored corpus already lives, and no longer the
thing the driver actually boots by default.

This step moves everything currently under `mudlib/` as-is, including
this project's own current additions (`wand_of_creation.c`,
`WAND_OF_CREATION_SCOPING.md`, the `config.h` tweaks) -- those get
re-added fresh in the new `mudlib/` in step 2 below, so their presence
in the frozen `temp/lil/` copy is just harmless history, not a second
place they need to be kept in sync.

## Step 2: rebuild `mudlib/` as the new "library" mudlib

Default is minimal: driver-required boot plumbing, the wand of creation
(already built and verified live), one freshly authored starting room
(Lil has no real "room" concept to carry over -- see the finding below),
and nothing else unless explicitly kept. Every file below was actually
read this pass, not assumed from its name.

### Keep (driver-required to boot and log in)

| File | Why |
|---|---|
| `single/master.c` | The configured `master_file`. Boots the driver, defines `connect()`/`compile_object()`/`crash()`/`flag()`. `flag()`'s own `"test"` case (`"/command/tests"->main()`) should be dropped along with the test suite it calls into, see below. |
| `inherit/master/valid.c` | `master.c`'s own `inherit "/inherit/master/valid"` -- the `valid_*` gate functions. |
| `single/simul_efun.c` | The configured `simul_efun_file`. |
| `clone/login.c` | `LOGIN_OB`, created by `master.c`'s `connect()`. |
| `clone/user.c` | The real player object `login.c` clones; `commandHook()` is the verb dispatcher every `command/*.c` file below depends on. |
| `inherit/base.c` | `BASE`, inherited by `user.c` and (per its own header comment) `wand_of_creation.c`'s freshly-created objects -- the `move()`/`remove()` wrapper this driver's `call_other` semantics require (`->move()` never falls back to the `move_object()` efun, see `wand_of_creation.c`'s own comment on this exact point). |
| `include/globals.h`, `include/config.h`, `include/command.h`, `include/lpctypes.h` | `include_dir`/`global_include_file` config, and directly `#include`d throughout. `config.h` already carries this project's own "AMLP note" (stock Lil ships it empty) -- keep that note. |
| `etc/motd` | `login.c`'s own `cat("/etc/motd")` call. |
| `clone/wand_of_creation.c` | Already built, live-verified. The reason this rebuild is happening at all. |
| `data/` | The wand's own `create <name>` write target (`/data/created/`). Already emptied this session (the two prior live-test artifacts, `readme`/`rusty_gear.c`, were removed as scratch output, not fixtures). |

### New (does not exist in stock Lil, needs writing)

- **A starting room.** Confirmed directly: stock Lil has no room concept
  at all. `VOID_OB` (`globals.h`: `"/single/void"`) is what
  `login.c` actually moves a fresh player into, and `single/void.c`'s
  entire body is `void dummy() {}` -- a bookkeeping placeholder, not a
  real location with a `short()`/`long()` description or exits. The
  "one login room" from the original request is new content, not
  something to carry over from Lil.
- Whatever top-level `readme`/identity text replaces Lil's own (see
  the naming section below).

### Drop (Lil's own bundled self-test/example content, not gameplay)

All of this is exactly what makes `temp/lil/` worth preserving in step
1 -- it stays fully available there, just not duplicated live in the
new mudlib.

| File(s) | What it actually is |
|---|---|
| `single/tests/efuns/*.c` (165 files), `single/tests/compiler/*.c` (38 files, `succeed.c` + `fail/*.c`), `single/tests/operators/range.c` | The real efun/compiler conformance suite `mudlib/readme` itself describes ("efun tests are in `./single/efuns`... named after the efun"). This is the corpus, not gameplay content. |
| `command/tests.c` | The `"efun <name>"` runner that drives the suite above (confirmed: iterates `get_dir()` over `single/tests/efuns/`, `catch()`-compiles the `fail/` cases expecting them to fail). |
| `inherit/tests.c` | `describe_test()`/`query_test_info()` -- support code for the suite above, confirmed trivial (10 lines). |
| `single/inh.c` | `void ifun() {}` -- confirmed trivial, an inheritance-test fixture stub for the suite above. |
| `test_control.c` | Confirmed genuinely empty (0 bytes). Dead weight regardless. |
| `etc/config.test` | Confirmed: a legacy MudOS v21 `key : value`-with-spaces config file, a different format from this driver's own `etc/driver_lil.cfg`. Not read by this driver at all. |
| `log/author_stats`, `log/domain_stats`, `log/readme` | Lil's own logging convention, nothing in `src/` or the kept files above writes to or reads these. |
| `single/void.c` | Superseded by a real starting room (see above). |
| Every per-directory `readme` (`clone/`, `command/`, `data/`, `include/`, `inherit/`, `log/`, `single/`, `single/tests/efuns/`, `u/`, and `mudlib/readme` itself) | Lil's own documentation about itself. Superseded by this project's own docs once the rebuild has an identity to describe. |
| `u/readme` (and the empty `u/` directory it is the only content of) | Lil's own per-player home-directory convention. Nothing currently uses it; drop unless a later decision wants user save-directories. |
| `.edrc` | A single config value (`148`, a line-length setting) for the `ed()` line editor. Only matters if `command/ed.c` is kept -- see the undecided list below. |

### Undecided -- flagged, not defaulted, needs a decision at execution time

Small, standalone commands, each cheap to keep or drop on its own merits
independent of the test-suite/gameplay split above:

- `command/quit.c`, `command/say.c`, `command/who.c`,
  `command/shutdown.c` -- minimal, generically useful MU* commands (8-19
  lines each). Recommend keeping all four; they cost almost nothing and
  give the stripped mudlib basic usability beyond just the wand.
- `command/eval.c` -- not just Lil example content: this is the exact
  tool `STATUS.md`'s row 0.15 fix used for its own live verification
  this session (`eval return 5+5`, etc. against the real running
  driver). Recommend keeping for that reason alone, independent of
  whether it counts as "gameplay".
- `command/dest.c`, `command/rm.c`, `command/update.c`,
  `command/codefor.c` -- dev-utility commands (destruct-by-id,
  delete-file, recompile-and-reload-a-file, and a `codefor.c` whose
  purpose was not investigated in depth this pass). Genuinely useful for
  the same kind of live poking `eval`/`update` already get used for, but
  also genuinely Lil example content -- no strong pull either way,
  listed here rather than defaulted.
- `command/speed.c` (206 lines, a benchmark harness, `inherit
  "/single/inh"`) and `command/ed.c` -- recommend dropping both.
  `speed.c` is self-test tooling, same category as the conformance
  suite. `ed.c` almost certainly does not work in this driver at all:
  this driver's own `src/efun/instruct.md` already documents the `ed()`
  efun as out of scope (architecture mismatch, same family as
  `get_char`/`origin`/`resolve`), so a command built on it has nothing
  real to call.

## Naming

"library" was given as the new name, casing/style not yet decided.
Stock Lil's own convention (`etc/driver_lil.cfg`: `mud_name: Lil`,
capitalized as a proper name) suggests `mud_name: Library` if the same
style is wanted, but this is a one-line config change either way and
not worth blocking the rest of the plan on -- confirm at execution time.

Also unresolved: whether "renamed" means the `mudlib/` directory path
itself moves too (matching "rename the lil mudlib sub", read literally)
or only the in-mud identity strings change while the directory stays at
`mudlib/`. This plan assumes the directory path itself stays `mudlib/`
(so `etc/driver_lil.cfg`'s own `mudlib_root: mudlib` and every other
path reference in `src/`/`test/` needs no change beyond the config's own
`mud_name` key and the banner text below) -- simpler and lower-risk than
also moving the directory, but should be confirmed, not assumed, before
execution.

## Other real references that would need updating, found by grep

Not part of the file move/drop above, but in scope for "rename" once
executed -- listed here so nothing gets missed:

- `etc/driver_lil.cfg`: `mud_name: Lil` (and possibly the filename
  itself, `driver_lil.cfg` -> `driver_library.cfg`, matching the
  `_<mudlib-name>` suffix convention the current name already follows).
- `clone/login.c`, `clone/user.c`: `"Welcome to Lil!"`/`"Welcome to
  Lil.\n> "` banner text (both files are being kept per the table
  above, only these literal strings need editing).
- `README.md`: `"mudlib/ is Lil, bundled for driver tooling"`.
- `wand_of_creation.c`'s own header comment (`"// mudlib: Lil"`) --
  cosmetic, but should track the rename since the file is being kept.
- `STATUS.md`'s historical entries that say "Lil" are a dated log of
  what was true *at the time* -- these should not be rewritten; they
  stay accurate as history regardless of what the mudlib is called
  going forward.

## Test-suite impact

Low. The three `testWandOfCreation*` tests (`test/test_lexer.cpp`) read
`mudlib/clone/wand_of_creation.c` directly off disk via
`AMLP_SOURCE_DIR` (fixed this same session, see `STATUS.md`) -- as long
as that file still exists at that exact path after the rebuild (it does,
per the keep list above), those tests are unaffected by anything else
moving or being renamed around it. No other test reads real files under
`mudlib/` at all (confirmed: `readMudlibFile()` is the only such helper,
grepped directly).
