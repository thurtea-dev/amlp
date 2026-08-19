# Account / login / character-select plan

**Status: scoping only. Nothing in this file has been implemented. No
mudlib code has been written yet.** This is a plan for a future session,
queued explicitly per the request that produced it (2026-08-19). It does
not compete with, block, or get worked ahead of the current Phase 0/1
driver priority (`ROADMAP.md` row 0.13a, `parse_*`). Read `CLAUDE.md`
before acting on any of this.

## What exists today

The bundled mudlib ("library", built on stock Lil) is intentionally
minimal scaffolding, not a real login system:

- `mudlib/single/master.c`'s `connect()` clones `/clone/login`
  unconditionally, catching only compile/instantiation errors, not doing
  any auth.
- `mudlib/clone/login.c`'s `logon()` prints a banner, clones
  `/clone/user`, `exec()`s the connection onto it, and destructs itself.
  Its own top-of-file comment reads `// needs fixed to handle passwords`
  -- the gap is already flagged in the source, not something this plan
  is discovering.
- `mudlib/clone/user.c` is a bare, ephemeral player object: a `name`
  string set post-hoc by `set_name()`, no persistence, no password, no
  concept of "account" vs "character" at all. Nothing survives a
  disconnect.
- There is no accounts directory, no character concept, and no login
  menu of any kind -- `logon()` runs straight through to the one real
  room (`mudlib/single/start_room.c`) with zero prompts in between.

## Driver hooks actually available (confirmed by reading the real source)

- **`master()->connect()`** (`src/net/Server.cpp:130-213`, `onNewConnection`):
  called once per new TCP connection, must return an object; a thrown
  error or non-object return closes just that connection, not the
  process (this isolation was a deliberate, already-shipped fix -- see
  the comment at `Server.cpp:147-156`). This is the only hook that
  decides *which* object a fresh connection is bound to, so any
  menu-driven login has to start from whatever object `connect()`
  returns (today: a fresh `/clone/login` clone every time, unconditionally).
- **`logon()`** (`Server.cpp:187-209`): applied once, zero arguments,
  immediately after `connect()`'s returned object is bound to the
  connection. A missing `logon()` is not an error (matches real
  FluffOS); a runtime error inside one closes only that connection.
  This is the natural entry point for a login state machine to kick off
  its first prompt, exactly as `mudlib/clone/login.c` already does today
  (just without any real auth logic behind it).
- **`input_to(function, flags, ...extra_args)`**
  (`src/efun/EfunTable.cpp:2059-2104`): fully working, including
  `INPUT_NOECHO` (bit 0x1) -- confirmed live in the driver code path,
  `conn->suppressEcho()` genuinely fires telnet echo suppression (Phase
  0.8's real IAC negotiation), not a no-op stub. This is exactly the
  primitive `core-lib`'s reference login (see below) builds its whole
  username -> password -> menu chain on top of, and it is already real
  in this driver. `call_out()`-based login timeouts (also used
  throughout that reference) are likewise already real (`Scheduler`).
- **`process_input()`** and **`add_action()`/`enable_commands()`**
  (`Server.cpp:282-330`, `VM.cpp` dispatch): already gate ordinary
  command dispatch behind `enable_commands()`, matching real semantics.
  Not directly needed for the login/menu phase itself (that phase should
  run entirely on `input_to()`, before `enable_commands()` is ever
  called), but this is what a character object calls once it actually
  takes over the connection for real gameplay -- exactly the pattern
  `mudlib/clone/user.c`'s own `setup()` already uses.
- **`exec(to, from)`** (`EfunTable.cpp:4998`ff): confirmed implemented,
  already used by `login.c` today to hand the live connection off from
  the login object to the player object. A menu-driven flow needs this
  same primitive to hand off from login-menu object to character object
  once one is chosen/created.
- **`save_object`/`restore_object`**
  (`EfunTable.cpp:6591-6660`+): both real and *round-trip safe with each
  other* today -- `save_object` writes this driver's own tab-delimited
  format, `restore_object` reads that format and also the real FluffOS
  space-delimited `.o` format. Row 0.7 on `ROADMAP.md` is only
  "partial" in the narrow sense that `save_object`'s own *output* isn't
  byte-for-byte real FluffOS `.o` yet -- for a same-driver
  save/restore round trip (exactly what account persistence needs), this
  is already fully functional. Confirmed directly by reading both efun
  bodies, not assumed.
- **`crypt(str, salt)`** (`EfunTable.cpp:5380-5400`): real, backed by the
  system `crypt()` call, already generates its own random salt when none
  is given. This is the real primitive for password hashing; no gap
  here.
- **`valid_read`/`valid_write`** (row 1.16, already implemented and
  wired into `save_object`/`restore_object`/`mkdir`/`get_dir`/etc):
  already fires for account-file paths today via
  `mudlib/inherit/master/valid.c` (currently a permissive
  unconditional-`1` stand-in). An account daemon can lean on this later
  for real per-account access control without any driver-side work.
- **`mkdir`/`get_dir`/`file_size`** (`EfunTable.cpp:6415-6560`): all
  real, sufficient for a simple on-disk account-directory layout (one
  file per account, optionally bucketed by first letter the way
  `core-lib`'s own `authenticationService` and several vendored corpora
  do).

## Confirmed driver gaps / things NOT to assume are available

- **No uid/euid model.** `getuid()`/`seteuid()`/`geteuid()` are not
  registered efuns at all (confirmed by grep against
  `src/efun/EfunTable.cpp`) -- every `#ifdef __PACKAGE_UIDS__` block in
  `login.c`/`user.c` is dead code under this driver (that macro is never
  defined in the bundled config), not a working-but-unused feature. Any
  account/character ownership model this plan produces has to be
  enforced through file-path convention and login-object-side checks,
  not a real uid hierarchy -- matches row 3.1 on `ROADMAP.md`
  (`privs_file`/uid/gid trust hierarchy) being entirely `[ ]` still.
- **`save_object`'s own on-disk format is not yet real FluffOS `.o`.**
  Fine for this driver's own round trip (see above), but worth flagging
  before anyone assumes account files could be handed to a real FluffOS
  driver unmodified -- they currently could not.
- **No telnet TTYPE/MXP negotiation**, only NAWS (Phase 0.8's own
  documented scope) -- `core-lib`'s reference `displayBanner()`/GMCP
  advertisement assumes more telnet capability than this driver
  currently negotiates. Not a blocker for a basic menu, just not
  something to copy wholesale.

## Reference corpora survey

Checked `temp/`'s newer corpora for a real account/login/character-select
flow worth reading as a pattern reference (not for copying code):

- **`temp/core-lib/secure/login.c` (+ `secure/login/core.c`,
  `menu-interactions.c`, `user-creation.c`) -- by far the most relevant
  reference found.** This is a modern (2017-2026 copyright header)
  LDMud-flavored lib (RealmsMUD), already unpacked on disk, not a zip.
  Read directly (not assumed): it is a real `input_to()`-driven state
  machine -- `logon()` -> `getUserLogin()` -> `enterPassword()` (with
  `INPUT_NOECHO`, a real login-attempt counter, IP temp-banning after
  repeated failures, `call_out("timeout", 90)` idle disconnects) ->
  `displayMenu()` on success, plus a parallel new-user path
  (`createUser()` -> `confirmUserName()` -> `setPassword()` ->
  `confirmPassword()` -> `execNewPlayer()`, which calls
  `authenticationService->saveUser()` then loads/`exec()`s a real player
  object). Auth itself is delegated to a separate
  `authenticationService` daemon (`userExists()`,
  `authenticateUser()`, `saveUser()`, `temporarilyBanIP()`), a clean
  separation this plan should mirror (a `/single/account_d.c` daemon,
  not auth logic embedded in the login object itself). It uses LDMud
  `virtual inherit` for its own three-way file split, which this
  driver's dialect support does not need to replicate structurally --
  the *shape* of the state machine is the useful part, not the LDMud
  inheritance mechanics. No character-select-among-multiple-characters
  step was found in what was read (`execNewPlayer()`/`execGuestPlayer()`
  each resolve straight to one player object) -- if genuine multi-character-
  per-account selection is wanted, that step still needs to be designed
  fresh, not lifted from this reference.
- **`temp/dead-souls`**: only login-shaped hit was
  `lib/www/cgi/login.c`, a web-CGI login form, unrelated to a telnet
  input_to() menu flow. Not useful as a reference for this plan.
- **`temp/final_realms_fluffos_v1.zip`**: contains
  `lib/secure/new_login.c`, `lib/secure/login.c`, `lib/secure/login.h`,
  `lib/obj/handlers/login_handler.c` -- real FluffOS-dialect login
  files, still zipped, not read in depth this pass. Worth a closer look
  in a future session specifically because it is FluffOS (this driver's
  primary target dialect) rather than LDMud, unlike `core-lib`. Flagged,
  not investigated further here.
- **`temp/skylib_fluffos_v3.zip`**: only hits were `bank_accounts/`
  (an in-game currency/bank system, not a login/auth account), unrelated
  to this plan.
- **`temp/tmi2_fluffos_v3.zip`**: has `lib/adm/daemons/logind.c` plus
  `lib/include/login.h`/`login_macros.h` -- a real FluffOS-dialect login
  daemon, still zipped, not read in depth this pass. Second candidate
  (after `final_realms`) worth checking in a future session for a
  FluffOS-native pattern to compare against `core-lib`'s LDMud one.

**Recommendation:** when this is picked up for real, read
`final_realms_fluffos_v1.zip`'s `lib/secure/login.c`/`new_login.c` and/or
`tmi2_fluffos_v3.zip`'s `logind.c` first, specifically because they are
FluffOS dialect (this project's actual target), and treat `core-lib`'s
already-read state-machine shape above as the structural cross-check, not
the primary source, since it is LDMud not FluffOS.

## Proposed architecture (design only, not implemented)

1. **Account storage.** A new singleton daemon, `/single/account_d.c`
   (naming to match this mudlib's existing `single/` convention,
   `master.c`/`simul_efun.c`/`start_room.c`), owning:
   - `account_exists(string name)`, `create_account(string name, string
     password)`, `check_password(string name, string password)` (via
     `crypt()`), each backed by one `save_object`/`restore_object` file
     per account under a new `/accounts/` tree (bucketed by first letter
     of the account name, matching the pattern several vendored corpora
     already use, e.g. `skylib`'s `save/bank_accounts/<letter>/`).
   - The account object itself (whatever `restore_object` populates)
     needs a defined variable shape up front: account name, crypt hash,
     creation timestamp, and a list of character names owned by this
     account (even if only one character per account is supported in
     the first real slice -- the list shape should exist from the start
     so a later multi-character slice doesn't need a save-format
     migration).
   - This is genuinely new mudlib code, not a driver gap -- every efun
     it needs (`crypt`, `save_object`, `restore_object`, `mkdir`,
     `file_size`) already exists and works, confirmed above.
2. **Login-menu object.** Rework `/clone/login.c` from its current
   linear banner-then-clone-user shape into a real `input_to()` state
   machine, structurally modeled on `core-lib`'s reading above but
   calling `account_d`/a to-be-designed `character_d` instead of that
   reference's own `authenticationService`/`loginModule`:
   `logon()` -> prompt for account name -> `input_to("gotAccountName", ...)`
   -> branch on `account_d->account_exists()` -> existing account:
   `input_to("gotPassword", INPUT_NOECHO, ...)` -> `check_password()` ->
   on success, character menu; new account: confirm name, set password
   (with confirm-password step, matching the reference's own
   `setPassword`/`confirmPassword` pair), `create_account()`, then
   straight into character creation (no menu needed yet if only one
   character exists). A login-attempt counter and `call_out()` idle
   timeout (both already proven patterns in the reference and already
   real primitives in this driver) should be part of this from the
   start, not bolted on later.
3. **Character object, distinct from the bare player object.** Today
   `/clone/user.c` conflates "the interactive session" with "the
   character" -- there is no persisted character state at all. This
   needs an explicit design decision (not made in this pass, flagged for
   the future session): whether the character object *is* a persisted,
   `save_object`-backed subclass/inherit of the current `user.c` (name,
   stats, inventory-on-disconconnect handling), with `user.c` reduced to
   pure connection/session plumbing, or whether "user" and "character"
   merge into one persisted object and the account daemon just tracks
   *which* character file to `restore_object()` into it after `exec()`.
   Either is buildable with what the driver already provides; this plan
   deliberately does not pick one yet.
4. **Character selection.** Only needed once multi-character-per-account
   is real (see the account storage note above) -- for a first working
   slice, one account can map to exactly one character and this step
   can be skipped entirely, matching the reference's own single-player-
   per-login shape. A real character-select menu (list existing
   characters, pick one or create new) is a natural second slice once
   the single-character path works end to end.

## Rough build ordering (first slice through later slices)

1. `/single/account_d.c`: account file format, `create_account`,
   `check_password`, `account_exists` -- no login integration yet,
   testable in isolation via `eval`.
2. Rework `/clone/login.c` into the real `input_to()` state machine
   (account name -> password, no character concept yet), wired to
   `account_d`, replacing the current unconditional clone-and-`exec()`.
   At this point login gates on a real password for the first time.
3. Design and land the character object shape (the open question in
   item 3 above), with `save_object`/`restore_object` persistence
   across disconnect/reconnect -- this is the point where "the wand of
   creation, the one room" scaffolding gets a first real persisted
   player state layered on top of it, without changing either.
4. Character creation flow for brand-new accounts (name a character,
   pick whatever minimal starting attributes are decided on).
5. Only after 1-4 work end to end: multi-character-per-account and a
   real character-select menu.

Each of these is sized to be its own session's real, verified,
tested-and-staged slice, matching this project's own established
discipline (`ROADMAP.md`'s row-by-row sequencing, `STATUS.md`'s per-
session verification bar) -- not something to build in one pass.

## Explicit non-status

Nothing above has been implemented. No new mudlib files exist yet. This
file is planning output only, written and staged (`git add`) per this
project's standing rule against ever running `git commit`/`git push`.
Whoever picks this up next should re-verify anything time-sensitive
above (driver efun table, `ROADMAP.md` row statuses) rather than trusting
this snapshot blindly, per this project's own established discipline.
