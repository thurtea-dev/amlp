# STATUS

Snapshot as of the slice that designed and implemented the
`add_action()`/`enable_commands()` command-dispatch subsystem (real
`command_giver`, per-object action tables, `move_object()`/`init()`
propagation, verb matching including the `V_SHORT` catch-all form) from
scratch, grounded in `fluffos-2.9-ds2.08/add_action.c` directly and this
mudlib's own real `std/living.c`/`std/room/exits.c`/`std/room/senses.c`/
`std/Object.c` usage. Confirmed live end to end: a fresh account now
reaches `create()`, proceeds through account setup and the early
chargen prompts, and the dispatch subsystem itself is confirmed
reaching real handler functions (`cmd_hook`, `process_input`'s own
callees) via the new mechanism. Along the way this pass also fixed a
run of further real, confirmed gaps the deeper walk surfaced (object
variable declaration-time initializers, `undefinedp()`/`nullp()`, a
genuine cross-inherit function-resolution gap, and -- the most
consequential -- object variables and locals defaulting to this
driver's own "no value" sentinel instead of real LPC's actual `0`
default, which had been silently wrong since early in the project).
Snapshot updated after root-causing the `__HistorySize` modulo-by-zero
report from the previous slice: it was never actually a bug in the
`this_player() != this_object()` guard (confirmed live, with temporary
instrumentation, that the guard behaves correctly and `reset_history()`
correctly sets `__HistorySize`). The real cause was a cascade of
several other missing efuns/driver bugs that each independently
prevented `setup()` from ever cleanly reaching that code, with
`catch(__Player->setup())` in `secure/std/login.c` silently swallowing
every earlier failure with no console trace. This slice fixed nine more
confirmed gaps found chasing that cascade (see "Root-causing the
__HistorySize report" below for the full trail), including a real
mudlib bug (`std/user.c`'s own `set_name()` missing the same PRIVS-off
bootstrap escape hatch `set_position()` already has) and a genuinely
significant new finding, not yet fixed: object variable slots may not
be correctly preserved across deep, multi-branch inheritance chains
(discovered via `std/living.c`'s own hidden `inherit` statements,
buried in `secure/include/living.h` rather than the `.c` file itself).
Live testing now reaches several steps further into chargen than
before, and stops at a new, distinct issue in `std/user/nmsh.c`'s own
`do_alias()`. Updated again after root-causing that issue: it was not a
`nmsh.c` bug at all, but a general compiler architecture bug in how
object-variable slots are resolved for sibling multi-inherits (see
"`do_alias()` root-caused" below) -- confirmed via live instrumentation,
proposed for direction, user-approved, and now implemented and fixed
(two distinct bugs, both covered by new regression tests, full suite
passing, `do_alias()` crash confirmed gone live). Updated again after
that fix uncovered a second, distinct, and likely broader bug reaching
for a room: this driver's function-call resolution order for a bare
call is backwards from real LPC's actual flattened-function-table
semantics (confirmed by reading `fluffos-2.9-ds2.08/compiler.c`'s
`define_new_function()` directly, not guessed) -- an ancestor file's
own deliberate placeholder/stub function (`std/user/nmsh.c`'s
`query_name()`, one of several such stubs) is not correctly overridden
by the real inheriting file's version for calls written inside the
ancestor's own source. See "Second, distinct bug found reaching for a
room" below. Proposed for direction, user-approved, and now
implemented and fixed, with the one existing test that encoded the old
(disproven) behavior corrected rather than just deleted. Three further,
much narrower gaps then surfaced live in immediate succession pushing
toward an actual room -- a real compiler bug (block scoping was never
implemented at all), a missing `sprintf` specifier (`%c`), and a
missing efun (`living()`) -- each root-caused, fixed, and covered by a
regression test; see "Three more gaps found live pushing from the
fixed `reset_prompt()` toward an actual room" below. Full suite: 282
tests passing. Live-confirmed: chargen now genuinely runs end to end
through zone selection and into attribute rolling for the first time
this project has reached it -- see "Live confirmed: chargen now
genuinely runs" below for the transcript. Not yet a full room: the
rest of chargen (roll/accept, race, OCC, alignment, skills) has not
been exercised live yet and may surface further gaps.

**Updated after closing that loop.** Resumed after an unplanned
reboot; found and killed an orphaned scratch driver process from a
different, no-longer-reachable session before starting fresh (see
"Resuming after a reboot: orphaned process, and a stale test script"
below for how that was confirmed safe). Pushed the live chargen walk
the rest of the way: race, alignment, and OCC selection all confirmed
live (including the exact `daemon/occ.c` empty-`attribute_requirements`
question the prior session had been mid-investigation of when the
reboot hit -- confirmed genuine content, not a driver bug, both by
static analysis and by live exercise of an empty-requirements OCC), then
nine more real, confirmed driver gaps chasing `finish_creation()` all
the way to an actual room: two missing efuns
(`all_inventory()`/`deep_inventory()`) whose absence crashed account
creation itself any time a player declined the name-confirm prompt,
`strcmp()` and `map_delete()` (the latter fatal to alignment selection),
a `clone_object()`/`load_object()` path-normalization bug (a caller-
supplied trailing `.c` produced a literal, never-existing `.c.c` file
lookup), a missing `intp()` type predicate, a missing `repeat_string()`
efun, a missing `present()` efun (blocking the very first starting
room's own `reset()`), and -- the deepest one -- this driver's
`explode()` never matched real FluffOS's own default leading/trailing-
separator semantics at all, which silently broke every compile-time
`privs` assignment for every object with a `/`-leading path (a new,
previously entirely missing mechanism, `ObjectManager::
initPrivsForObject()`, added alongside it). Also added
`remove_call_out()` (a stub matching `call_out()`'s own already-stubbed
non-scheduling behavior). Full suite: 288 tests passing. **Live-
confirmed end to end for the first time this project has reached it:**
a fresh account now runs the complete chargen flow -- login, account
creation, gender/display name/email/real name, zone, attribute
roll/accept, race, alignment, OCC pick, automatic starting-equipment
grant -- and lands in a real starting room (`domains/ChiTown/areas/
chitown_start.c`) with a live NPC present and a full room description,
`finish_creation()`'s own automatic display. See "Chargen closed the
loop: full run confirmed live, reaching a real room" below for the full
transcript and the complete gap-by-gap trail. **Correction, recorded
here rather than silently edited away:** this section originally also
claimed an explicit, separately-typed `look` command was confirmed
working at this point. It was not -- that claim came from a buffer-
timing artifact in this project's own probe script (trailing,
already-in-flight output from the automatic display was misattributed
to a `look` command sent moments later), not a genuine live result.
The real `look` command did not actually work at all until the
dispatch-argument bug described in "Real call_out()/heart_beat()
scheduler" below was found and fixed, several sessions later. See that
section for the real confirmation, and its own opening paragraph for
how the mistake was caught.

## Working now

- Clean build via `cmake -B build -S . && cmake --build build`.
- `ctest --test-dir build` passes (253 unit test cases in one binary,
  covering lexer, parser, codegen, and VM execution for every supported
  feature).
- Driver boots, compiles and loads `secure/daemon/master.c`, runs its
  `create()`, starts the non-blocking TCP listener, and now also
  compiles and loads the real `secure/SimulEfun/SimulEfun.c` simul_efun
  object (config's `simul_efun_file`) at boot.
- Function-call resolution is a full four-tier chain matching real
  FluffOS's own order (local -> inherited -> simul_efun object -> core
  efun table). `efun::name(...)` bypasses all of that straight to the
  core efun table, matching real LPC's own explicit escape hatch
  (needed by simul_efun files that shadow a real efun's name, e.g.
  `secure/SimulEfun/misc.c`'s own `destruct()` wrapper).
- Variables, assignment (including compound assignment `+= -= *= /= %=`,
  and embedded assignment inside a `&&`/`||` chain -- e.g.
  `stringp(val) && val=load_object(val) && ...` -- which real LPC
  resolves at its own precedence, not by stopping at the next `&&`),
  `if`/`else`, `while`, `for` (all three clauses optional), a bare `;`
  null statement (a loop whose entire body is the condition's own side
  effects), `break`/`continue` (including correctly-scoped nested
  loops), comparisons, logical `&&`/`||` (short-circuiting), plain `&`
  (bitwise on ints, set intersection on arrays), plain `|`/`^`
  (bitwise-only, int operands), ternary, prefix/postfix `++`/`--` on
  bare variables, C-style type casts (parsed and discarded as a no-op).
- Float literals (`1.5`, `.5`), stored in a dedicated `floatPool`
  alongside the existing `stringPool`.
- `call_other()`/`->`, with the function-name argument a full
  expression, and `->` chaining correctly with `[` indexing in either
  order and any number of times (`inv[i]->query_property(...)`,
  `a->b()->c()`, etc) -- previously a one-shot check before the index
  loop, not a shared loop, so index-then-arrow left the arrow
  unconsumed.
- Array/mapping literals accept a trailing comma before the closing
  `})`/`])`, real LPC's normal style.
- `inherit "path";`, single- and multi-level, flattened object-variable
  layout, inherit-cycle detection.
- `sscanf(str, fmt, ...vars)`: literal text, `%s`, `%d`, `%%`, `%*` skip
  modifier. `%f`/`%x`/`%(regexp)` throw rather than silently mishandling.
- `clone_object()`, arrays, mappings, indexing (read/write, range),
  string/array concatenation, arithmetic, unary negation/not, function
  prototypes, modifier keywords, object variables, adjacent string
  literal concatenation.
- Real file I/O: `read_file()` / `write_file()`, resolved against the
  configured mudlib root.
- A runtime error thrown out of an object's `create()` fails that one
  object's load with a clear `[object]`-prefixed message instead of
  crashing the whole driver process. This same guarantee now also
  covers `master->connect()` and per-line input dispatch in
  `net/Server.cpp` -- previously uncaught there, so one bad connection
  (or one player hitting a runtime bug mid-session) took the entire
  driver process down for every other connected player. Confirmed live:
  this is exactly what happened attempting `secure/std/login.c` before
  it actually compiled.
- The driver's own `cpp` invocation injects FluffOS's full driver-level
  predefined macro table (`option_defs.c`), plus the smaller
  runtime-computed set from `lex.c`'s `add_predefines()` (`MUD_NAME`,
  `__PORT__`, `__VERSION__`, `__ARCH__`, `SIZEOFINT`, `MAX_INT`, etc).
  `MUD_NAME`/`__PORT__` are sourced from `Config` (new `mud_name` key in
  `driver.cfg`, default `AetherMUD`), the rest are fixed values.
- A real socket connection has been driven end-to-end through
  `master->connect()` (which `clone_object()`s `secure/std/login.c`) via
  a raw Python socket test (no telnet/nc binary in this environment).
- `logon()` is called on every new connection immediately after
  `master->connect()` binds the object, with zero arguments (`net/
  Server.cpp`'s `onNewConnection()`, matching `backend.c`'s own
  `logon()`: `apply(APPLY_LOGON, ob, 0, ORIGIN_DRIVER);`). A runtime
  error in a defined `logon()` closes only that connection, same
  isolation guarantee as `connect()` and per-line dispatch already had.
- `input_to(string func, void|int flags, void|mixed extra_args...)` is
  a real efun: it records the calling object (`VM::currentObject()`,
  new -- a `callStack_` the VM now maintains across nested `run()`
  calls, real FluffOS's own `current_object`) plus any extra args as
  the pending handler on the connection driving the call
  (`OutputContext::current()`, standing in for `command_giver`).
  Numeric echo/no-escape flags (`I_NOECHO` etc) are positionally
  consumed like the real efun and then discarded -- this driver does
  not negotiate telnet echo suppression yet.
- Per-connection pending-handler storage is a new `Connection` slot
  (`net/Connection.hpp`'s `PendingInputTo`: a `weak_ptr<LpcObject>` plus
  function name and extra args), matching real FluffOS's
  `interactive_t::input_to` sentence.
- Per-line dispatch (`Server::dispatchLine()`, a new `static` method,
  deliberately pulled out of `handleConnection()` so it is directly
  unit-testable without a real listening socket) now matches `comm.c`'s
  `process_user_command()` order exactly: a pending `input_to()`
  handler is checked and consumed *first* (cleared before the call, so
  the handler is free to register the next one itself), and only when
  nothing was pending does `process_input(line)` run instead. The old
  fixed `receive_message(line)` apply is gone.
- New efuns: `receive()` (writes straight to the current connection,
  same as `write()` -- real FluffOS's `f_receive()` is `current_object-
  >interactive`-scoped, which is the same connection here), `call_out()`
  (validates the real 2+-arg shape and returns a handle; does not
  actually schedule anything yet -- `Scheduler::tickCallOuts()` is still
  the pre-existing empty stub, unchanged this slice), `master()`,
  `lower_case()`, `replace_string()` (3-arg replace-all form only).
- `call_other()` now accepts a string target, not just an already-
  resolved object -- confirmed against `simulate.c`'s real
  `find_object()`, which (unlike the easy-to-confuse-it-with
  `find_object2()` in the same file) compiles and loads the file on a
  cache miss rather than only ever looking one up. This is exactly why
  real `master.c`'s own `preload()` can force-load a daemon with
  nothing more than `call_other(str, "???")`. `VM::findObject()` wraps
  the already-existing `ObjectManager::loadObject()` (same cache
  `master`/`simul_efun` already load through) to provide this; no new
  preload/boot-order infrastructure was needed.
- `::name(...)` / `qualifier::name(...)` -- explicit calls to an
  *inherited* definition of a function, bypassing the current program's
  own local definition even when one exists (`grammar.y`'s
  `function_name` production). Found live compiling
  `secure/daemon/account_d.c`'s own `::create();` -- confirmed
  extremely common (887 files across the mudlib use the bare or
  qualified form, almost always an overridden `create()`/`init()`
  running its parent's own setup too). New `OpCode::CallParent` (plus a
  trailing `CallParentQualifierSlot` data instruction, the same shape
  `Sscanf`'s var-slot table already uses) resolves the bare form by
  walking every immediate parent's own inherit chain (skipping this
  program's own functions), and the qualified form by matching the
  qualifier against each immediate parent's own `inherit` path's
  basename (`daemon::create()` for `inherit "/std/daemon";`).
- `(: name, bound_args... :)` closure/function-pointer literals -- see
  the dedicated section below for the full recon/design/citation
  writeup. New `Value` variant `Closure` (owner object, bare function
  name, bound args), new `ClosureLiteralExpr` AST node, new
  `OpCode::PushClosure`, and `VM::callClosure()` (the "apply a closure
  with extra args" mechanism `evaluate()`/`funcall()` and, later,
  `call_out()` all share). Only the bare-identifier-name form is
  implemented, confirmed to be the only shape this mudlib's boot/login/
  account-creation path actually uses.
- Real `master()->apply_unguarded()`'s own shape -- `previous_object(0)`
  called from inside a closure-invoked core efun -- surfaced a genuine
  bug in `VM::callClosure()`'s first cut: a closure resolving to a core
  efun (not a local/simul_efun function) never went through `run()` at
  all, so `vm.currentObject()` stayed whatever the *caller of
  `evaluate()`* was instead of the closure's own owner, confirmed
  against real `setup_fake_frame()` (`interpret.c`), which unconditionally
  sets `current_object = fun->hdr.owner` for every closure kind,
  including a core-efun-bound one. Concretely: `secure/daemon/
  account_d.c`'s own `unguarded((: save_object, path :))` was saving
  `master.c`'s own variables to the account's save path instead of
  `account_d.c`'s. Fixed with a new `ObjectFrameGuard` RAII helper
  (shared by `VM::run()` and `VM::callClosure()`'s core-efun branch) and
  covered by a dedicated regression test
  (`testEvaluateOfEfunBoundClosureSetsCurrentObjectToClosureOwnerNotCaller`).
- New efuns needed to reach and validate the above live: `evaluate()`/
  `funcall()` (invoke a closure), `call_out()` extended to accept a
  closure as well as a function-name string, `master()`, `previous_object()`
  (backed by a new, separate `objectChangeStack_` -- real FRAME_OB_CHANGE
  semantics, only pushed on an actual cross-object call, not every
  same-object one), `error()`, `stringp()`/`objectp()`/`mapp()`/
  `pointerp()`/`arrayp()`/`functionp()` (type predicates), `file_name()`,
  `strsrch()`/`strstr()`, `interactive()`/`users()`/`find_player()`
  (backed by a new `InteractiveRegistry`, populated by
  `Connection::attach()`/`close()` -- see its own header), `allocate()`,
  `allocate_mapping()`, `values()`, `capitalize()`, `sprintf()` (`%s`/`%d`
  only, confirmed the only specifiers this mudlib's login/account path
  uses), `message()` (routes to the currently active connection,
  ignoring `type`/`targets`/`excludes` -- see its own comment for why
  that is enough for every call site actually on this path),
  `set_eval_limit()` (accepted, currently a no-op), `destruct()`
  (closes the connection too, if the destructed object was the one
  bound to it), `find_object()`/`load_object()` (look-only vs.
  compile-on-miss, real aliases of the same efun with different
  argument defaults), `time()`, `ctime()`, `userp()`/
  `query_once_interactive()`, `crypt()` (via the system's own `crypt(3)`,
  `-lcrypt` linked). Also fixed: `sizeof()`/`strlen()` did not handle a
  plain string argument at all (fell through to a silent `0`) --
  `!sizeof(some_string)` is this mudlib's standard "is this string
  empty" idiom, used constantly, and was being silently mis-evaluated
  everywhere until this was caught; `this_object()` was a permanent
  void stub before `VM::currentObject()` existed for `input_to()`'s own
  needs, now a real read of it.
- `save_object()`/`restore_object()` now use a recursive, self-
  delimiting serialization format covering every `Value` kind
  (int/float/string/array/mapping, arbitrarily nested) instead of the
  original flat int/float/string/string-array set, found live needing
  to grow when `daemon/banish.c`'s own `create()` saved a mapping
  variable. Also now normalize the save path the same way real
  `object.c`'s `save_object()` does (strip a trailing `.c`, strip a
  trailing `.o` if already present, always append `.o`) -- found live
  when `daemon/banish.c`'s own `restore_banish()` called
  `restore_object(SAVE_BANISH)` with no extension at all, relying on
  the efun to add one; without this fix a real pre-existing
  `daemon/save/banish.o` on disk was silently unreachable. A real,
  pre-existing save file in this mudlib's own space-separated LPC-
  literal text format (not this driver's format) is not parsed -- every
  line is silently skipped (no tab separator matches), so the object's
  variables keep whatever defaults its own `create()` already set
  rather than being populated from the real historical file; this is a
  deliberate simplification (see "Known stubs" below), not a crash, and
  was sufficient for everything reached live so far.
- `master()->compile_object()` (virtual objects): real FluffOS's
  `int_load_object()` fallback (`simulate.c`) when a `load_object()`/
  `find_object()` path has no matching `.c` file on disk -- the master
  apply `compile_object(path, 0)` gets a chance to hand back a real
  object anyway (`secure/daemon/master.c`'s own player-object branch
  clones `/std/user` and returns that clone for a virtual
  `/secure/save/users/<letter>/<name>` path). New `ObjectManager::
  loadVirtualObject()`, called from `loadObject()` exactly when the
  requested path's own `.c` file genuinely does not exist (mirroring
  `int_load_object()`'s own `stat()` check order): calls
  `master()->compile_object(path, 0)` via `VM::applyMaster()`, and if
  that returns a real object, rebinds it to the requested virtual path
  (`LpcObject::rebindFilename()`, matching real `load_virtual_object()`'s
  own `SETOBNAME` + object-hash reinsertion) and caches it there so a
  second lookup for the same path returns the same object without
  re-invoking `compile_object()`. Guarded against recursion
  (`virtualCompiling_`) and against firing before `master_`/`vm_` exist
  yet (matching real `load_virtual_object()`'s own `if (!master_ob)
  return 0;`). New `new()` efun, a real alias of `clone_object()`
  (func_spec.c: `object clone_object _new(string, ...);`) -- confirmed
  live: `master.c`'s own `player_object()` calls `new(OB_USER)`.
- The `status` type keyword -- real `lex.c`'s own `{"status",
  L_BASIC_TYPE, TYPE_NUMBER}`, a legacy synonym for `int`. Needs no
  CodeGen/VM handling beyond being parseable (this driver's `Value`
  model is already dynamically typed regardless of declared type).
  Found live compiling `std/user.c`'s own `static status snoop,
  earmuffs;`.
- Function declarations with only modifiers and no return type at all
  (real LPC's own implicit-`mixed`-return convention) -- found live
  compiling `std/user.c`'s own `private static register_channels();`
  (prototype) and `static private register_channels() { ... }`
  (definition), neither naming a type. `Parser::parseDeclPrefix()` now
  treats the type keyword as optional: if the token after any modifiers
  isn't itself a keyword, the declaration's name follows directly.
- A bare `{ ... }` block used as a standalone statement, not attached
  to any `if`/`while`/`for` -- real, standard LPC/C syntax for local-
  variable scoping. Found live compiling `std/user.c`'s own quit()-
  adjacent cleanup code. No CodeGen changes needed:
  `CodeGen::emitStatement()` already flattens any `Block` node it
  encounters as a statement (originally added for comma-separated local
  var decls), which is exactly right here too.
- `<N` from-the-end indexing (real LPC: `grammar.y`'s `expr4 '[' '<'
  comma_expr ...` family, confirmed against `eoperators.c`'s
  `f_range()`/`f_extract_range()`: the actual index used is `length -
  N`, clamped to 0 if still negative after resolving). Found live
  compiling `std/user.c`'s own `files[j][<2..] != ".o"`. Supported on
  both bounds of a range index and the single-index form, in any
  combination (`arr[<a]`, `arr[<a..b]`, `arr[a..<b]`, `arr[<a..<b]`,
  `arr[<a..]`) -- `IndexExpr` gained `indexFromEnd`/`rangeEndFromEnd`
  flags, carried through `OpCode::Index`/`OpCode::RangeIndex` via their
  otherwise-unused `argCount` field (repurposed as a 2-bit flags mask)
  since neither opcode needed a real argument count. A bare negative
  literal with no `<` prefix still throws exactly as before (see
  `testStringRangeIndexNegativeStartThrows`, unaffected) -- only a
  start that resolves negative *after* an actual `<N` conversion clamps
  to 0, matching real `eoperators.c`'s own distinction.
- Compound assignment on an indexed target (`target[index] += value`
  etc, including a chained/nested target like
  `player_data["general"]["quest points"] += ...`) -- found live
  compiling that exact `std/user.c` line. `IndexAssignStmt` gained
  `isCompound`/`compoundOp` fields mirroring `AssignExpr`'s own; the
  compound case emits target and index *twice* (once to read the
  current value via `Index`, once to write the combined result via
  `IndexAssign`) rather than duplicating them on the stack, since
  there is no "duplicate the top two stack entries as a pair" opcode.
  Harmless for every real target/index this mudlib's own call sites
  use (plain variable reads and string-literal keys, no side effects)
  but would double any side effect a more exotic target/index
  expression happened to have -- flagged in `IndexAssignStmt`'s own
  comment rather than silently assumed safe in general.

## `(: name, bound_args... :)` closures: recon, design, implementation

Consulted directly against the FluffOS reference driver before
designing, not inferred:

- **Grammar** (`grammar.y`): the lexer's own fast path (`lex.c`,
  around the `function_flag`/`L_NEW_FUNCTION_OPEN` handling) recognizes
  `(: identifier :)` / `(: identifier, args... :)` at the character
  level -- if the identifier immediately after `(:` is followed
  directly by `:` or `,`, it is classified right there as FP_L_VAR/
  FP_G_VAR/FP_LOCAL/FP_SIMUL/FP_EFUN by name lookup and handed to
  `grammar.y`'s `l_new_function_open` production. Anything else (a
  non-bare-identifier expression, e.g. `this_object()` or a string
  literal) falls back to `old_func()`, hitting the general
  `L_FUNCTION_OPEN comma_expr ':' ')'` production instead -- a real,
  distinct "inline lambda" form that compiles the comma-expression
  itself as the closure's own body code (`functional_t`), not a
  bind-by-name at all.
- **Representation** (`function.h`): `funptr_t` / `funptr_hdr_t` --
  `ref`, `type` (FP_*), `owner` (the object active when the literal was
  constructed, i.e. `current_object` at bind time), and `args` (bound
  arguments, a real `array_t*`). The type-specific union holds an efun
  table index, a local function index, a simul_efun index, or (for the
  inline-lambda form) a `functional_t` with its own tiny compiled
  program.
- **Invocation** (`function.c`'s `call_function_pointer()`): checks
  `owner` isn't destructed, calls `setup_fake_frame()` (unconditionally,
  for every closure kind -- "`previous_ob = current_object; current_object
  = fun->hdr.owner`", real FRAME_OB_CHANGE semantics), merges `hdr.args`
  in *before* whatever the caller's own extra arguments were
  (`merge_arg_lists()`, confirmed by reading its own stack-shuffling
  loop), then dispatches on `type`. `efuns_main.c`'s `f__evaluate()`
  (registered as both `evaluate` and `funcall`) is the generic "call a
  closure with extra args" efun; a non-`T_FUNCTION` argument is a
  silent no-op, not an error.

### Real usage across the mudlib (261 `(: ... :)` occurrences, 109 files)

- **Bare name, no bound args** (53 occurrences) -- `(: living :)`,
  `(: Setup :)`, `(: intp :)`. An efun, local function, or simul_efun
  referenced with nothing pre-applied.
- **Bare name with bound args** (138 occurrences, the dominant shape,
  ~53%) -- `(: file_size, p :)`. Overwhelmingly `unguarded((: efun_name,
  args... :))` (137 of these alone), the shape that blocked
  `account_d.c` live; also `call_out((: local_func :), delay)` (5
  occurrences, e.g. `daemon/intermud.c`'s own `call_out((: Setup :),
  2)`).
- **Object-bound, `(: obj_expr, "funcname" :)`** (part of the 36
  "other/complex" bucket) -- e.g. `std/Object.c`'s own `set_long((:
  this_object(), "new_long" :))`. Not the lexer fast path (the first
  token after `(:` is a call expression, not a bare identifier), so
  this compiles through the general inline-lambda production instead.
  Not implemented: not used anywhere on this driver's own boot/login/
  account-creation path (`domains/Praxis/*_vote.c` room files, well
  past what this driver currently reaches).
- **`$1`/`$2` positional and `$(name)` captured-variable placeholder
  lambdas** (7 occurrences) -- e.g. `daemon/services/who.c`'s own
  `filter(users(), (: $1 && environment($1) :))`. Not implemented: all
  7 are in `daemon/intermud.c`, `daemon/services/who.c`,
  `daemon/services/auth.c`, `secure/daemon/events.c`, `secure/daemon/
  chat.c`, `secure/std/client.c` -- none on this driver's current path.
- **Bare string-constant closures, `(: "literal" :)`** (35
  occurrences) -- e.g. `set_die((: "on_death" :))`. A real, if unusual,
  form (`grammar.y` explicitly warns on it: "Function pointer returning
  string constant is NOT a function call"). Not implemented: every use
  found is a `std/obj/*`-level callback (death/wear/remove hooks), not
  reached by this driver's current login/account-creation path.
- **Variable-holding-a-function bare form**, `(: gtmp1, gtmp2 :)`
  (`daemon/refs.c`) -- excluded on inspection: real `grammar.y` itself
  rejects a *bound-args* form naming a local/global variable
  ("Can't give parameters to functional."), and `refs.c` is not
  referenced from anywhere reachable in this mudlib at all. Likely
  dead/legacy code even in the real driver.

**Consumers confirmed on this driver's own reachable path:**
`unguarded()` (a simul_efun, `secure/SimulEfun/security.c`, wrapping
`master()->apply_unguarded()` -> `evaluate()`) and `call_out()`. `map()`/
`filter()`/`sort_array()`/`set_heart_beat()` all take closures in real
FluffOS too and are used elsewhere in this mudlib (chat/mail/channel
daemons), but none are reached from the boot/login/account-creation
path this driver currently exercises, so none of the three were
implemented as closure-consumers this slice -- `VM::callClosure()` is
already the generic mechanism any of them would need, so adding one
later is just "for each element, `vm.callClosure(closure, {element})`",
not a new design question.

**Storage**: every closure actually reachable in this mudlib is
constructed and consumed inline at the same call site (`unguarded((:
file_size, p :))`, `call_out((: Setup :), 2)`) -- none are assigned to
a variable, stored in an array/mapping, and invoked later through a
separate generic-apply code path. This is what justifies this driver's
lazy-resolve-by-name simplification (see `Value.hpp`'s `Closure`
comment): real FluffOS bakes FP_LOCAL/FP_SIMUL/FP_EFUN in at
construction time, this driver re-resolves the bare name against the
same tiered lookup `Call` already uses, at the moment the closure is
actually invoked. Observably identical for every closure this mudlib's
own boot/login/account-creation path builds.

## `catch(expr)`: implemented as a real VM-level control-flow construct

Not a function call -- confirmed directly against the FluffOS reference
driver rather than inferred: grammar.y gives it its own production
(`catch: L_CATCH expr_or_block`, not `function_call`), icode.c compiles
it to a dedicated `F_CATCH`/`F_END_CATCH` opcode pair bracketing the
guarded code, and interpret.c's `do_catch()`/`F_END_CATCH` show the
real semantics: the guarded expression's own result is always discarded
(`trees.c`'s `insert_pop_value()` on the catch argument), success
evaluates to int `0` (not empty string), and a runtime error evaluates
`catch(...)` to the error message string instead of propagating.

This driver's version:

- `Ast.hpp`'s `CatchExpr` node (only the `catch(expr)` parenthesized
  form -- real LPC's `catch { block }` alternative is not implemented,
  nothing in this mudlib uses it), recognized in the Parser by literal
  text the same way `efun::` and `sscanf` already are, not a reserved
  keyword.
- `Bytecode.hpp`'s `PushCatchFrame`/`PopCatchFrame` opcode pair, mirroring
  `F_CATCH`/`F_END_CATCH` exactly: `PushCatchFrame`'s operand is a
  forward-patched "resume here" jump target (same
  `emitJumpPlaceholder()`/`patchJumpToHere()` machinery `if`/`while`
  already use), `PopCatchFrame` is only ever reached on success and
  pushes `0`.
- `VM::run()` keeps a `catchFrames` stack local to each call (one LPC
  function invocation), wraps its own per-instruction dispatch in a
  `try`/`catch (const LpcRuntimeError&)`, and on an exception with an
  active frame: truncates the operand stack back to the depth recorded
  at `PushCatchFrame` time, pushes the error message, and resumes at the
  recorded instruction pointer. An empty `catchFrames` rethrows exactly
  as before `catch()` existed -- this is also what makes a caller's
  `catch()` correctly trap an error thrown inside a callee that has no
  `catch()` of its own: the callee's own (empty) `catchFrames` rethrows,
  the exception unwinds straight out of its nested `run()` call, and
  lands in the caller's own `try`/`catch`.
- `EvalCostError` (new, in `core/Errors.hpp`) is deliberately *not* a
  subclass of `LpcRuntimeError`, so the eval-cost-exceeded guard cannot
  be caught by `catch()` -- confirmed against `do_catch()`'s own "Can't
  catch eval cost too big error" handling, a runaway loop wrapped in
  `catch()` must still be stoppable.
- `throw()` is not implemented (not asked for this slice, and nothing in
  the boot/connect path uses it yet).

Confirmed live: `master->connect()` now runs its real body successfully
end to end. A socket connection genuinely reaches
`[net] connection fd=N bound to /secure/std/login` -- `catch(ob =
clone_object(OB_LOGIN))` succeeds, `err` stays falsy, `ob` is correctly
the cloned login object, exactly `master.c`'s own real two-outcome
shape.

## The connect/input protocol gap: closed, confirmed live

Real FluffOS's own connect-time sequence (`logon()` called with zero
args right after `connect()` binds the object, per-line dispatch
preferring a pending `input_to()` handler over `process_input()`) is
now implemented and confirmed live end to end, not just unit-tested in
isolation. A raw Python socket connection against the real mudlib
(`secure/std/login.c`, via `etc/driver.cfg`) gets:

```
   _____          __  .__                                   .___
  /  _  \   _____/  |_|  |__   ___________  _____  __ __  __| _/
 /  /_\  \_/ __ \   __\  |  \_/ __ \_  __ \/     \|  |  \/ __ |
/    |    \  ___/|  | |   Y  \  ___/|  | \/  Y Y  \  |  / /_/ |
\____|__  /\___  >__| |___|  /\___  >__|  |__|_|  /____/\____ |
        \/     \/          \/     \/            \/           \/

     A post-apocalyptic roleplaying game.
     ...
What account name do you wish?
```

-- `logon()`'s own `receive(read_file(WELCOME))` and
`receive("\nWhat account name do you wish? ")` calls, followed by its
`input_to("get_name")` registration, all real, all live. Sending a name
back over the socket correctly dispatches to `get_name()` (not
`process_input()`), which runs `convert_name()` (a simul_efun,
resolved through the existing tier-3 lookup, needing only the new
`lower_case()`/`replace_string()` efuns to actually execute) and then
`continue_login()`.

## The `(: ... :)` closures blocker: closed, confirmed live

The previous slice's blocker -- `secure/daemon/account_d.c` failing to
compile on `unguarded((: file_size, p :))` -- is fixed, and the live
walkthrough now goes dramatically further than closures alone required.
A raw Python socket connection against the real mudlib was driven
through the *entire* new-account flow:

```
What account name do you wish? aetherwalker

Account name: aetherwalker
Note: this will also be your FIRST character's name, ...
Confirm Aetherwalker as your account and first character name? (y/n) y

Please choose a password of at least 5 letters: hunterpass

Please confirm your password choice: hunterpass

(connection closed here -- see next real blocker below)
```

Every one of those prompts is real `secure/std/login.c` code running
end to end: `continue_login()` -> `BANISH_D->valid_name()`/
`allow_logon()` (both real daemon files, auto-compiled on first
`call_other()`) -> `new_user()` -> `choose_password()` ->
`confirm_password()` -> real `crypt()` hashing -> `ACCOUNT_D->
create_account()` -> `save_account()` -> `unguarded((: save_object, path
:))`. A genuine account save file landed on disk with the right
content:

```
__NoClean	I1;
account_name	S12:aetherwalker
password	S13:<real crypt hash>
characters	A1:S12:aetherwalker
email	S0:
last_character	S12:aetherwalker
pending_approval	I0;
```

(this driver's own recursive save format -- see "Working now" above --
not real FluffOS's own on-disk syntax; `S12:aetherwalker` is a length-
prefixed string, `A1:...` an array of 1 element, `I0;` a plain int.)

(`account_name`/`characters`/etc -- confirming the
`VM::callClosure()`/`ObjectFrameGuard` fix above: this is
`account_d.c`'s own data, not `master.c`'s, which is what actually
landed there before that fix.)

## `master()->compile_object()` (virtual objects): closed, confirmed live

The previous slice's blocker is fixed (see "Working now" above for the
implementation) and confirmed live: `master()->player_object("...")`
now correctly reaches `compile_object()`'s own `DIR_USERS` branch,
which calls `new(OB_USER)` and genuinely attempts to compile
`/std/user.c` for the first time -- progressing well past where the
driver stopped before, through several further gaps (also fixed this
slice, see "Working now"): the `status` type keyword, modifier-only
function declarations, a bare block statement, `<N` indexing, and
compound indexed assignment, each found by literally trying to compile
`std/user.c` and reading whatever line the parser choked on next.

## Object-bound/string-constant closures and `(*fp)(args)`: closed, confirmed live

(Recon and disconfirmation below are unchanged from when this section
was still "the next real blocker" -- kept in full as the citation trail
for *why* the general lambda form was implemented the way it was. See
"Closure/function-pointer forms completed" further down for what
actually landed and how.)

`std/user.c`'s own body now compiles cleanly. It `inherit`s several
files, including `std/user/editor.c`, which does not:

```
[object] compile error in .../std/user/editor.c: parse error: expected
":" in closure literal at line 40 (got "(")
[object] .../std/user.c: failed to compile inherited file
"/std/user/editor"
```

The actual source:

```
if(!abort) abort = (: previous_object(), "abort" :);
```

This is real LPC's **object-bound closure** form (confirmed against
`grammar.y`'s recon from the closures slice: since `previous_object()`
is a call expression, not a bare identifier, the lexer's fast path
(`lex.c`'s `function_flag` handling) does not fire, and this falls
through to the general `L_FUNCTION_OPEN comma_expr ':' ')'` production
-- the "inline lambda" form, not the bare-name-with-bound-args form
this driver already implements). The same file also uses two more
forms this driver does not implement: a bare string-constant closure
(`this_object()->more(EDITOR_HELP, "help", (: "return_to_edit" :))`)
and the `(*fp)(args)` dereference-call syntax (`(*__Abort)()`,
`(*__Callback)(__Arguments)`) -- confirmed by reading the same
`icode.c`/`grammar.y` machinery used for the closures slice's own recon,
not new territory syntactically.

What stopped this from being implemented alongside those (a routine
parser addition, the same as the bare-name form was): reading
`icode.c`'s `NODE_FUNCTION_CONSTRUCTOR` codegen for the general lambda
form (`i_generate_node(expr->l.expr)`, where `expr->l.expr` is the
parsed body expression -- here the comma-expression `previous_object(),
"abort"`) says the compiled closure body just *executes that expression
and returns its value* when called -- for a plain C-style comma
expression, that means: evaluate `previous_object()` and discard it,
then evaluate and return the string `"abort"`. Taken literally, calling
this closure would do nothing but yield the string `"abort"`, never
actually invoking anything on the object.

That reading cannot be reconciled with how the *identical* syntax is
used elsewhere in this mudlib and is clearly intended to work: `std/
Object.c`'s own `set_long((: this_object(), "new_long" :))` (confirmed
during the closures slice's own recon, at the time deferred as
"unconfirmed, not reachable yet") and several `domains/Praxis/
*_vote.c` room files use exactly this "(: obj, \"funcname\" :)" shape
specifically so that querying the room's description calls
`new_long()` on it dynamically. If evaluating the closure only ever
returns the literal string `"new_long"`, `query_long()` would return
that literal string as the room's actual description text -- clearly
not the intended behavior for real, actively-maintained content in
this mudlib.

Both readings can't be right at once. A follow-up pass re-checked this
specifically against the runtime invocation code, not just the parser/
codegen side, to test the hypothesis that `(: expr, "string" :)` (an
arbitrary first operand, a string-literal second operand) is a
dedicated "bind this function name on this object" closure form,
distinct from generic comma-expression folding, checked before falling
back to the general lambda path. It is not. Specifically:

- **`grammar.y` has no such production.** The full set of alternatives
  under the closure-literal nonterminal (`l_new_function_open ':' ')'`,
  `l_new_function_open ',' expr_list2 ':' ')'`, `L_FUNCTION_OPEN
  comma_expr ':' ')'`, and the mapping/array-literal cases) was read in
  full; none inspect operand *count* or operand *type* the way the
  hypothesis requires. The one type-based check in this whole block --
  `if ($2->kind == NODE_STRING) yywarn("Function pointer returning
  string constant is NOT a function call");` in the `L_FUNCTION_OPEN
  comma_expr ':' ')'` action -- only fires when the *entire* body is a
  single bare string node (i.e. `(: "literal" :)` alone, zero commas).
  For `(: this_object(), "new_long" :)`, `$2` is a `NODE_TWO_VALUES`
  node (a comma expression), not `NODE_STRING`, so this warning does
  not even apply to the two-operand shape -- it is a different, simpler
  gotcha-check unrelated to the hypothesis.
- **`comma_expr`'s own grammar rule confirms plain, type-agnostic
  comma-operator folding**: `comma_expr: expr0 | comma_expr ',' expr0
  { CREATE_TWO_VALUES($$, $3->type, pop_value($1), $3); }` --
  `pop_value($1)` wraps the *left* operand to discard its result; there
  is no branch anywhere in this rule keyed on what `$1` or `$3` actually
  are.
- **`icode.c`'s own codegen for `NODE_TWO_VALUES` is equally
  type-agnostic**: `case NODE_TWO_VALUES: i_generate_node(expr->l.expr);
  i_generate_node(expr->r.expr); break;` -- generate the (pop-wrapped)
  left, then the right, full stop. Nothing here inspects whether
  `expr->l.expr` looks like an object reference or `expr->r.expr` is a
  string constant.
- **The runtime invocation side gives the same answer.** A closure
  built from this general lambda form is `FP_FUNCTIONAL`
  (`function.h`'s `funptr_t`), which stores *compiled bytecode*
  (`functional_t`: `prog`/`offset`/`num_arg`/`num_local`), not a stored
  object-and-string pair -- there is no data shape for a runtime check
  to inspect in the first place. `call_function_pointer()`'s own
  `FP_FUNCTIONAL` case (`function.c`) does setup
  (`setup_control_stack`/`setup_variables`) and then
  `call_program(funp->f.functional.prog, funp->f.functional.offset)` --
  it jumps into that already-compiled bytecode and lets the ordinary
  interpreter loop run it to completion, with no post-processing branch
  on the result before returning it. `f__evaluate()`
  (`efuns_main.c`, backing `evaluate()`/`funcall()`) does not inspect
  the result either; it is later confirmed even the `(*fp)(args)`
  dereference-call form desugars to this exact same
  `call_function_pointer()` path.

So the actual, confirmed behavior of `(: this_object(), "new_long" :)`
on the real fluffos-2.9-ds2.08 reference driver is: evaluate
`this_object()` and discard it, then evaluate and return the string
`"new_long"` -- calling this closure never invokes `new_long()` on
anything. Cross-checking the mudlib's own usage does not rescue the
"dedicated form" hypothesis either: `new_long()` genuinely exists as a
real function on `cleric_vote.c`/`kataan_vote.c`/`fighter_vote.c` (each
defines its own `string new_long() { ... }` right next to the
`set_long()` call), which is exactly why this reads as *intended* to
invoke it -- but intent is not the same as what the referenced driver
source actually does with this syntax. Per `std/Object.c`'s own
`query_long()` (`if(functionp(__Long)) return (string)((*__Long)(str));
else if(stringp(__Long)) return __Long; ...`), the real, confirmed
consequence is that `query_long()` on any of these rooms returns the
literal string `"new_long"` as the room's description text, not the
dynamic description `new_long()` computes -- a genuine behavioral bug
in this mudlib's own inherited content (all three vote rooms trace to
the same "Nightmare IV" era code per their file headers), not a gap in
this driver's understanding of the reference source. `std/user/
editor.c`'s own `(: previous_object(), "abort" :)` default is, by the
same reading, a functionally inert placeholder (`previous_object()` has
no side effect, so the whole default closure does nothing but return
`"abort"`) -- harmless there specifically because `edit()`'s real
callers are expected to supply their own working `abort` callback, and
the broken default is only ever reached when they don't.

This driver now implements exactly what the reference source
implements: the general lambda form (`L_FUNCTION_OPEN comma_expr`)
compiles its body as ordinary comma-expression code and returns
whatever the last element evaluates to, matching real semantics
precisely rather than the "call the named method" behavior the
mudlib's own authors evidently assumed. `(: expr, "string" :)` is
therefore implemented as plain two-value comma folding, not as a
bound-method call -- implementing the call-the-method behavior instead
would make this driver *more* "helpful" than the real reference driver
its own author intended to match, silently papering over a real bug
this mudlib's own maintainers have not yet found. Confirmed live: a
"fixed" version was deliberately **not** wired in, per the explicit
instruction not to implement anything speculative once the reference
source's own runtime invocation code settles the question.

What was still genuinely worth implementing, independent of the above
(both are ordinary comma-expression-adjacent forms once the general
lambda body is compiled correctly, not the ambiguous case): the bare
string-constant closure `(: "literal" :)` (a `NODE_STRING` body,
already correctly a "return this constant, do nothing else" functional
per the same grammar/codegen just confirmed -- the compiler's own
`yywarn` even flags it as likely a mistake, but it still compiles and
runs) and the `(*fp)(args)` dereference-call syntax (confirmed to
desugar to the same `call_function_pointer()` path `evaluate()`/
`funcall()` already use). All three landed together in the next pass
-- see the next section.

## Closure/function-pointer forms completed: general inline lambda, bare string-constant closures, and `(*fp)(args)`

Implemented exactly what the recon above concluded, no more:

- **General inline lambda** (`Ast.hpp`'s new `InlineLambdaExpr`,
  `grammar.y`'s `L_FUNCTION_OPEN comma_expr ':' ')'`). The parser tells
  this apart from the existing bare-name closure literal the same way
  real LPC's LALR grammar does: only a bare identifier immediately
  followed by `,` or `:` is the bare-name form; anything else (a call
  expression, a string literal, ...) falls through to a general
  comma-separated expression list. `CodeGen` cannot emit a lambda's
  body in place (every function in one `CompiledProgram` shares a
  single flat `code` array addressed by `entryPoint`, so splicing a
  second `Return` into the middle of the enclosing function's own
  instructions would return out of the wrong function) -- it queues
  each one (`CodeGen::PendingLambda`) and compiles it right after the
  enclosing function's own `Return`, as its own synthesized
  `FunctionEntry` (name prefixed `$lambda#`, a sequence no real LPC
  identifier can ever equal), reached at call time through the exact
  same `findFunctionInChain()` lookup an ordinary bare-name closure
  already uses. No VM changes were needed for this reason.
- **Bare string-constant closure** (`(: "literal" :)`) is not a special
  case at all once the above works: it is the trivial one-element case
  of the same comma-separated body.
- **`(*fp)(args...)` dereference-call syntax** (`grammar.y`'s `'('
  '*' comma_expr ')' '(' expr_list ')'`) desugars at parse time straight
  to a forced call of the core `evaluate` efun (`CallExpr::forceEfun`,
  the same mechanism `efun::name(...)` already uses), exactly matching
  what the reference grammar's own action does
  (`predefs[evaluate_efun].token`) -- this driver's own `evaluate`/
  `funcall` efuns already call `VM::callClosure()`, so this too needed
  no VM changes, only a parser-level rewrite into an ordinary
  `CallExpr`.

Confirmed live: `std/user/editor.c` (the file that surfaced all three)
now compiles.

## Further gaps found and fixed while walking `std/user.c`'s full inherit chain

With the closure forms above landing, compilation walked forward
through `std/user.c`'s inherit chain (`autosave.c`, `editor.c`,
`files.c`, `nmsh.c`, `more.c`, `refs.c`, `living.c`, then `user.c`
itself) and hit four more real, confirmed gaps, each fixed the same
way: real mudlib usage first, reference grammar/codegen second,
implementation third, tests alongside.

- **Indexed `++`/`--`** (`std/living.c`'s own `healing["intox"]--`).
  `IncDecExpr` previously only accepted a bare variable name target
  (real usage grep found exactly 2 prefix and several postfix real call
  sites using an indexed target instead). `OpCode::IndexAssign` leaves
  nothing on the stack (correct for its original statement-only caller,
  `IndexAssignStmt`), so an indexed increment/decrement used as an
  expression stashes the pre- and post-mutation values in a hidden temp
  local (never nameable by real LPC source) and pushes back whichever
  one `prefix` calls for, after the mutation actually runs.
- **Indexed assignment as a sub-expression** (`std/user/more.c`'s own
  `if(!(__More["class"] = cl)) ...`). The parser previously only
  recognized a bare variable name as an assignment target *inside an
  expression* (index-expression targets were statement-only, via
  `IndexAssignStmt`). New `IndexAssignExpr` (`Ast.hpp`) plus
  `CodeGen::emitIndexAssignExpr()` use the same hidden-temp-local
  approach as the indexed `++`/`--` fix above, for the same underlying
  reason (`IndexAssign` produces no stack value to chain into the rest
  of the enclosing expression).
- **`private` object-variable scoping across an inherit chain**
  (`std/living.c`'s own `static private int __Locked, __LastAged;`
  colliding with `std/user.c`'s separate, unrelated `static int
  __LastAged;`). Real LPC scopes a `private` object variable to the
  file that declares it -- invisible to, and non-collidable with, a
  child's own variable of the same name -- but this driver previously
  discarded every modifier keyword unrecorded, including `private`,
  treating every object variable as fully inherited/nameable. Fixed by
  threading `isPrivate` through `Parser::DeclPrefix` ->
  `ObjectVarDecl::isPrivate`, and having `CodeGen::generate()` record a
  private variable's slot in the flattened `objectVarNames` list a
  child inherits under a synthesized, non-collidable placeholder name
  (`$private#<slot>`) instead of its real one -- the slot position is
  still reserved (an inheriting child's own bytecode has to agree with
  the parent's already-compiled bytecode on where every later slot
  starts), but the real name stays reachable only from the declaring
  file's own code.
- **`to_int(string | float | int)`** (`efuns_main.c`'s `f__to_int()`,
  surfaced directly by `std/user/more.c`, `std/living.c`, and
  `std/user.c` itself, not just transitively through the closure
  chain). Implemented matching real truncate-toward-zero float
  behavior and real leading-integer string parsing (`to_int("10x") ==
  10`); the `buffer` case in the real signature is dropped, since this
  driver has no buffer type in `Value`'s variant at all.

All four are covered by new `ctest` cases (17 added this session, one
per confirmed real shape plus the still-correctly-rejected range-index
`++`/`--` case -- see `tests/test_lexer.cpp`).

## `add_action`/`enable_commands` command-dispatch subsystem: closed, confirmed live

(The original "next real blocker" writeup below is kept in full as the
citation trail for why this was scoped as architecturally significant
rather than a routine efun gap. See "The add_action/enable_commands
command dispatch subsystem: recon, design, implementation" further down
for what actually landed, the confirmed real-usage recon, and the
design decisions made from it.)

With every compile-time gap above fixed, `std/user.c` now compiles
cleanly and a live account genuinely reaches `create()` on a real
player object -- confirmed live end to end (fresh account creation,
password set/confirmed, `compile_object()` succeeds). `create()` then
throws immediately:

```
[object] create() failed for /std/user: undefined function or efun: enable_commands
```

`enable_commands()` (real `add_action.c`'s `f_enable_commands()`,
`enable_commands(1)`) is the gate real FluffOS requires before
`add_action()`-registered commands on an object take effect. Checking
this driver's own `EfunTable.cpp` found `add_action` itself is not
registered either -- there is no command-dispatch subsystem here at
all yet, only the `input_to()`-callback path implemented and confirmed
live in an earlier slice (see "The connect/input protocol gap: closed,
confirmed live" above). This is a materially different, larger
mechanism (a per-object list of verb -> handler-function bindings,
consulted against typed input that doesn't match any pending
`input_to()` handler, in add-order with each handler free to decline
by returning 0 and falling through to the next) -- basic player
interaction (`look`, movement commands, anything using `add_action`
rather than `input_to()`) cannot work without it. Flagged rather than
started speculatively, matching this project's own stopping criteria:
this is architecturally significant, not a routine language or efun
gap.

A secondary, smaller issue found alongside this: when `create()` fails
on a cloned player object, the connection is left in a dead state with
no error sent to the client (`[net] connection fd=4 input handling
failed: call_other: first argument must be an object or a string
path` on the next line typed) rather than a clean disconnect or retry
prompt. Worth a small hardening pass once the real fix (`add_action`)
lands, not urgent on its own.

## The `add_action`/`enable_commands` command dispatch subsystem: recon, design, implementation

### Step 1: recon (real usage across this mudlib, before designing anything)

`add_action()` is called 384 times across this mudlib (excluding
`doc/`). The real shapes found:

- The overwhelming majority are the plain two-arg form,
  `add_action("cmd_foo", "foo")` -- a bare function name plus an exact-
  match verb string, e.g. every `cmds/mortal/_*.c` file's own `init()`.
- A verb argument can be an **array of strings**, one function bound to
  several verbs at once: `cmds/skills/_mist.c`'s own
  `add_action("checkdest", ({ "go", "enter" }))`.
- A three-arg **catch-all** form with an empty verb and flag 1:
  `std/living.c`'s own `add_action("cmd_hook", "", 1)` (in
  `init_living()`) and `secure/std/setter.c`'s own
  `add_action("chargen_catch", "", 1)`. Per this project's own earlier-
  recorded gotcha (`CLAUDE.md`'s "add_action catch-all gotcha"), flag 1
  is `V_SHORT`: the bound function receives only the text *after* the
  first word, and the real verb must be read back via `query_verb()`.

Where `enable_commands()` is called relative to `add_action()`: **not**
in the same place every time, and not always in an `init()` at all.
`std/user.c`'s own `create()` calls it directly, unconditionally, once
per player object. `std/living.c`'s own `init_living()` -- which calls
`add_action("cmd_hook", "", 1)` -- is itself called directly from
`std/user.c`'s `setup()`, as a plain function call, **not** through a
driver-invoked `init()` apply. Real per-room/per-item action
registration (`std/room/exits.c`, `std/room/senses.c`, `std/Object.c`)
*does* use a genuine driver-invoked `init()` apply hook (confirmed:
`std/room.c`'s own `void init() { container::init(); exits::init();
senses::init(); }`).

How verb dispatch is actually wired in this mudlib, confirmed by
reading `std/living.c` directly rather than assuming the generic
FluffOS default: **most player-typed mortal commands do not go through
per-file `add_action()` registrations at all.** `std/living.c`'s own
`cmd_hook(string cmd)` -- reached via the one catch-all registration
above -- reads the real verb via `query_verb()`, looks it up through
`daemon/command.c`'s own `find_cmd(verb, search_path)` (a directory-scan
cache mapping bare verb names to the directories containing a matching
`_<verb>.c` file), and if found, calls it directly via
`call_other(file, "cmd_"+verb, cmd)` -- reaching the file's blueprint
object directly, bypassing the standard per-object action table
entirely for this category. The hundreds of individual
`add_action("cmd_foo", "foo")` calls inside `cmds/mortal/_foo.c`'s own
`init()` are therefore **not** what actually dispatches an ordinary
player command in this mudlib's real, live design -- confirmed:
`cmds/mortal/_look.c` has no `add_action()` or `init()` at all, it is a
bare `cmd_look(string str)` function, reached only through
`cmd_hook()`'s own `find_cmd()`/`call_other()` path. The *real* generic
`add_action()` mechanism (a genuine per-object action table, refreshed
by `init()` on movement) is what room exits, room senses (`hide`,
`search`, `smell`, `listen`), `std/Object.c`'s own `read`, and
`living.c`'s own `lock` verb and catch-all hook actually use.

### Step 2: design (grounded in `fluffos-2.9-ds2.08/add_action.c` directly)

- **Storage**: `LpcObject` gained `environment_`/`inventory_` (real
  `object_t::super`/`contains`, simplified to a plain `weak_ptr`/
  `vector<shared_ptr>` rather than FluffOS's intrusive `next_inv`
  linked list -- this driver already uses that simplification
  elsewhere, e.g. `InteractiveRegistry`), `commandsEnabled_` (real
  `O_ENABLE_COMMANDS`), and `actions_`, a `vector<ActionEntry>` (real
  `sentence_t` list) where each entry is `{verb, functionName,
  owner (weak_ptr), flag}`. New registrations always prepend
  (`addAction()`), matching `add_action.c`'s own literal comment:
  `"adding to the top of the list doesn't harm anything"` --
  `p->next = command_giver->sent; command_giver->sent = p;` -- so the
  most-recently-registered entry is always checked first.
- **When/how `init()` gets (re-)invoked**: `VM::moveObject(item, dest)`
  (backing the new `move_object()` efun) ports the two legs of real
  `setup_new_commands()` (`add_action.c`) this mudlib's own confirmed
  usage actually needs: if `item` is command-enabled, `dest`'s own
  `init()` runs with `item` as `command_giver` (a room handing its
  verbs to the player who just entered); then, for every other
  already-present object, each side's `init()` runs against the other
  as `command_giver` if command-enabled, in the same order real
  `setup_new_commands()` uses (matters for prepend-order priority). The
  third leg (`dest` itself being command-enabled, i.e. being moved
  *into* another living object rather than a room) was scoped out --
  the reference source's own comment calls it "rare", and nothing on
  this mudlib's confirmed movement path does it. `command_giver` itself
  is a new explicit `VM` stack (`commandGiverStack_`, RAII-guarded via
  `CommandGiverGuard`, mirroring real `save_command_giver()`/
  `restore_command_giver()`), falling back to whichever connection is
  currently driving the call (`OutputContext::current()`) when nothing
  has explicitly set one -- needed because `std/living.c`'s own
  `add_action("cmd_hook", "", 1)` runs as a *plain function call* from
  `setup()`, not through a driver-invoked `init()` apply, so there is no
  `moveObject()`-provided `command_giver` active at that point; real
  `secure/std/login.c` confirms this is fine because its own
  `exec_user()` calls `exec(__Player, this_object())` (rebinding the
  connection to the new player) **before** calling `__Player->setup()`,
  so the connection's own bound object already *is* the player by the
  time `add_action()` runs.
- **Dispatch**: `VM::dispatchCommand(giver, line)` (real
  `parse_command()`/`user_parser()`) splits the line into its first
  word (the verb) and the remainder, walks `giver->actions()` in
  registration order (front = most recent = checked first), and for
  each entry: flag 0 requires an exact verb match; flag 1/2 requires
  `entry.verb` to be a leading-characters prefix of the typed verb (an
  empty `entry.verb` trivially matches everything, covering the real
  catch-all shape) -- `query_verb()` always returns the *full* typed
  word, never just the matched prefix, matching real semantics exactly.
  A handler that returns falsy does not stop the search (real
  `add_action.c`'s own doc comment: "the parser will continue searching
  for another command, until one returns true"); one entry's argument
  is the plain remainder-of-line string, matching real LPC's own
  argument-passing convention for a bound command function.
- **The `enable_commands()` gate**: a plain boolean flag on
  `LpcObject`, checked by `moveObject()` before treating an object as
  eligible to have its own `init()` propagate actions, or to receive a
  room/occupant's -- exactly real `O_ENABLE_COMMANDS`'s role.

### Step 3: implementation

New: `LpcObject::environment_`/`inventory_`/`commandsEnabled_`/
`actions_` (`ActionEntry`, `addAction()`, `removeAction()`); `VM`'s
`commandGiverStack_`/`verbStack_` plus `commandGiver()`/
`pushCommandGiver()`/`popCommandGiver()`/`currentVerb()`/
`moveObject()`/`dispatchCommand()`; new efuns `environment()`,
`move_object()`, `enable_commands()`, `disable_commands()`,
`add_action()`, `remove_action()`, `query_verb()`, `this_player()`.
`Server::dispatchLine()` now calls `process_input()` first if defined
(confirmed live needed: `std/user/nmsh.c`'s own real mud-shell/history/
alias preprocessing) -- its return value decides what actually reaches
`dispatchCommand()`, matching real `comm.c`'s own three-way branch on
the apply's return type (string = dispatch that instead; truthy number
= fully consumed, nothing dispatches; anything else = dispatch the
original line unchanged) -- rather than the previous silent no-op
fallback.

Two further gaps had to be fixed along the way, both required for
`std/user.c` to actually reach a live account:

- **`exec(object new_ob, object old_ob)`** (real `replace_interactive()`)
  was completely missing. Without it, `secure/std/login.c`'s own
  `exec(__Player, this_object())` could never rebind the connection
  from the login object to the actual player, so the player's own
  `create()`/`setup()` would run but the connection would stay bound to
  the login object forever. Implemented via `Connection::attach()`
  (already existed for the initial login-object bind), targeting
  whichever connection is currently driving the call
  (`OutputContext::current()`) -- matches every real call site in this
  mudlib, `exec()` is always called by the object currently holding the
  connection, never by an unrelated third object.
- **`query_privs(object default: this_object())` /
  `set_privs(object, int|string)`** (real `object_t::privs`) were
  missing, surfaced by `std/living.c` and `std/money.c` calling
  `query_privs()` unconditionally on log-relevant lines. Implemented as
  a plain `std::optional<std::string>` field on `LpcObject`.

Walking further into the live account-creation/chargen flow (past
`add_action`/`enable_commands` themselves) surfaced four more real,
confirmed gaps, fixed the same way as always -- real usage first,
reference source second, implementation third, tests alongside:

- **Object variable declaration-time initializers**
  (`secure/daemon/wiztools.c`'s own `string *REISSUED_TOOLS = ({ ...
  });`, a file with no `create()` at all). Previously rejected with an
  explicit `NotImplementedError` since nothing on the confirmed path
  used the shape -- now a real gap. `ObjectVarDecl` gained an
  `initializer` field; `CodeGen::generate()` synthesizes a
  `"$objvarinit"` function (same synthesized-name convention as the
  lambda/private-slot mechanisms) that assigns every initialized
  variable; `ObjectManager::runObjectVarInitializers()` runs it,
  parent-before-child across the whole inherit chain, immediately
  before `"create"` on every new instance -- via a new
  `VM::callFunctionInProgram()` that targets one exact program level
  directly (not the normal tiered lookup), since every level uses the
  same fixed synthesized name and the normal lookup would only ever
  reach one level's copy.
- **`undefinedp(mixed)` / `nullp(mixed)`** (real `f__undefinedp()`),
  surfaced by `daemon/multi.c`'s own `query_prevent_login()`. This
  driver has no int-subtype distinction the way real FluffOS's
  `T_UNDEFINED` is; its own `monostate` ("no value" -- what an
  undefined function call returns) is the closest analog, so that is
  what these check instead of a number subtype flag.
- **A genuine cross-inherit function-resolution gap**: `OpCode::Call`
  resolved a bare name only against the *currently executing file's*
  own program and its own inherited chain, never against the object's
  actual most-derived program. Real LPC compiles each file
  independently and has no way to know at a parent's own compile time
  that some future child will define a name it references -- such a
  call can only resolve at runtime, against whatever the real running
  object turns out to be. Surfaced live: `std/user/nmsh.c`'s own
  `process_input()` calling the bare name `query_client`, a function
  only `std/user.c` (which inherits `nmsh.c`) defines. Fixed with a
  fallback: if the normal lexical-scope search (current file, then its
  own ancestors) finds nothing, retry against `obj->program()` -- but
  *only* as a last resort, after the lexical search has already failed,
  specifically so a file's own internal self-calls keep resolving to
  its own (or its own ancestors') definitions first; real LPC does not
  virtually dispatch a parent's internal calls to a child's override.
  Both directions are covered by dedicated tests
  (`testParentCallToFunctionOnlyChildDefinesResolvesAtRuntime`,
  `testParentCallStillPrefersItsOwnLexicalDefinitionOverChildsOverride`).
- **Object variables and locals defaulted to this driver's own
  `monostate` ("no value") instead of real LPC's actual default, the
  integer `0`.** This one is the most consequential of the four: real
  LPC has no separate "unset" state distinct from `0` for an ordinary
  declared variable (there is no equivalent of this driver's own
  `monostate` at that level, only at specific driver-internal "no
  value" sentinels this codebase already used it for deliberately,
  e.g. an efun explicitly returning "nothing found"). This had been a
  documented but unverified assumption since early in the project (both
  `LpcObject.cpp` and a `test_lexer.cpp` test's own comments asserted
  monostate "matches how ... reads as 0" without it ever having been
  exercised against real arithmetic) -- it does not: monostate fails
  every arithmetic opcode a real `0` would silently succeed at.
  Surfaced live twice in a row before being fixed at the root:
  `std/Object.c`'s own `query_name()` reading an unset `__TrueName`
  (string concatenation), and `std/user/nmsh.c`'s own
  `add_history_cmd()` doing `++__CmdNumber` on an unset counter.
  `LpcObject`'s `variables_` and `VM::run()`'s per-call `locals` now
  both fill with a real `int64_t 0` per slot instead of a default-
  constructed `Value{}`.

Also added along the way: uncaught `LpcRuntimeError`s are now tagged
with `file::function(): ` at the innermost frame that had no `catch()`
of its own to absorb them, matching this driver's existing convention
of naming the file in every other `[object]`-prefixed diagnostic --
this is what made diagnosing several of the gaps above tractable at
all, and is worth keeping regardless.

All of the above is covered by 24 new `ctest` cases (`environment()`/
`move_object()` linking, the `enable_commands()` gate, exact and
catch-all verb dispatch with correct argument-splitting, the
falsy-return-keeps-searching rule, `this_player()`/`query_verb()`
during dispatch, `query_privs()`/`set_privs()`, object variable
initializers including the parent-before-child ordering, `undefinedp`/
`nullp`, and both directions of the cross-inherit call-resolution fix).

### Live test results

Confirmed live, in order, all in one continuous account-creation and
chargen session: account name entry and confirmation, password set and
confirmed, gender selection, display name formatting, email and real-
name prompts (both optional, skipped), the first-admin bootstrap offer
(confirmed reachable both accepting and declining it), and zone
selection. `enable_commands()`, `add_action()`, `move_object()`,
`exec()`, and the `process_input()`/`dispatchCommand()` pipeline all
fired correctly along this path with no errors of their own --
every error hit past that point was a *different*, specific,
individually-diagnosed-and-fixed gap (`query_privs`, `undefinedp`, the
cross-inherit call bug, the object-variable-default bug), not the
dispatch subsystem itself. The transcript never reached a state where a
player could type `look` at a live prompt, because chargen itself does
not reach a real room before hitting the next blocker below -- but
every piece of the dispatch subsystem that *is* exercised along this
path (the catch-all `cmd_hook` registration actually firing,
`process_input()` actually being consulted and its return value
actually feeding into dispatch, `this_player()` resolving correctly
mid-dispatch) is confirmed working, and the 24 new unit tests directly
exercise the exact real shapes (`std/living.c`'s own catch-all,
`std/room/exits.c`'s own movement registrations, the falsy-return
fallthrough) end to end at the VM level independent of how far live
chargen gets.

## Root-causing the `__HistorySize` report: guard logic was never the bug

The previous slice's own "next real blocker" (reproduced above) turned
out to be a red herring, confirmed by adding temporary `write()`
instrumentation directly to `nmsh.c`'s `setup()` and to
`secure/std/login.c`'s own `catch(__Player->setup())`, then observing
real runtime values against the live chargen flow (removed once
understood, per this project's own methodology of never leaving
debug-only code behind). The actual sequence:

- `secure/std/login.c`'s own `catch(__Player->setup())` silently
  swallows *any* error thrown inside `setup()`, with no console trace
  at all -- confirmed by temporarily capturing and printing the
  `catch()` result, which is otherwise discarded.
- Once `setup()` genuinely reaches `nmsh::setup()` without an earlier
  silent failure, live instrumentation confirmed `this_player() ==
  this_object()` (the guard correctly evaluates false, does not return
  early) and `reset_history()` correctly sets `__HistorySize` to `10`.
  The guard was never broken.
- The `__HistorySize` symptom reported at the end of the previous slice
  only ever surfaced because `setup()` had *already* failed earlier,
  silently, on one of several missing efuns confirmed below -- by the
  time a *different* test happened to get further before failing
  elsewhere, `__HistorySize` looked like the active bug purely by where
  the crash happened to land.

Nine further gaps were found and fixed chasing this cascade to the end,
each confirmed live before being fixed, tests added alongside every
one:

- **`set_living_name(string)`** (real `add_action.c`'s
  `f_set_living_name()`) was entirely unregistered. Stored on
  `LpcObject` (a new `livingName_` field) without wiring up a lookup
  table for it, matching this driver's own existing `find_player()`
  simplification (InteractiveRegistry + `query_name()`, not a real
  living-name table).
- **`set_heart_beat(int)` / `query_heart_beat(object)`** were also
  entirely unregistered, despite `LpcObject` already having
  `hasHeartbeat()`/`setHeartbeat()` support (used by `ApplyTable`'s own
  `heart_beat` apply recognition) -- nothing had ever wired the actual
  efun to it. Setting the flag now works and is queryable; there is
  still no periodic heartbeat scheduler that reads it back to actually
  call `heart_beat()` on anything (a separate, larger feature, not
  needed for anything reached so far).
- **`query_ip_name(void|object)`** was missing. Implemented as an alias
  of the already-working `query_ip_number()` (always the numeric IP,
  never a real reverse-DNS hostname lookup -- this driver does no DNS
  resolution of its own, matching real FluffOS's own documented
  fallback when hostname resolution is unavailable, and avoiding a
  blocking lookup inline in the connection-handling loop).
- **A genuine cross-inherit function-resolution gap in `OpCode::Call`**:
  a bare call resolved only against the *currently executing file's*
  own program and its own inherited chain, never against the object's
  actual most-derived program. Real LPC compiles each file
  independently and has no way to know at a parent's own compile time
  that some future child will define a name it references -- such a
  call can only resolve at runtime. Surfaced live: `std/user/nmsh.c`'s
  own `process_input()` calling the bare name `query_client`, which
  only `std/user.c` (which inherits `nmsh.c`) defines. Fixed with a
  fallback to `obj->program()`, but *only* after the normal lexical-
  scope search has already failed, so a file's own internal self-calls
  still resolve to its own definitions first (real LPC does not
  virtually dispatch a parent's internal calls to a child's override --
  covered by two dedicated tests, one per direction).
- **Array subtraction (`arr1 - arr2`, real set difference) was entirely
  unimplemented** -- only numeric `-` existed. A significant, previously
  undiscovered gap given how common this idiom is in real LPC. Surfaced
  live: `std/user.c`'s own `register_channels()` doing `channels -
  __RestrictedChannels`. Implemented as: every element of the left
  array that also occurs anywhere in the right array (by value
  equality) is dropped, order and any non-matched duplicates preserved.
- **`monostate` did not participate in arithmetic as a real `0`.** Real
  FluffOS's `T_UNDEFINED` is a *subtype* of `T_NUMBER` (a number whose
  value already is `0`, tagged only so `undefinedp()` can detect it),
  not a separate value kind arithmetic has to special-case. This
  driver's own `monostate` plays the same "no value" role (a missing
  mapping key, or -- before this slice's own earlier `0`-default fix --
  an unassigned object variable/local) and needed the same treatment.
  Surfaced live: `std/living.c`'s own `query_stats()` doing
  `stats[stat] + x` where `stats[stat]` is a missing key for any stat
  never rolled yet. Fixed via a shared `asArithmeticOperand()` helper
  used by `Add`/`Sub`/`Mul`/`Div`/`Mod` (and therefore `++`/`--` too,
  which already desugar through `Add`/`Sub`), treating `monostate` as
  `0.0` alongside `int64_t`/`double`.
- **A real, confirmed mudlib bug**: `std/user.c`'s own `set_name()`
  never had the same PRIVS-off bootstrap escape hatch `set_position()`
  (in the very same file) already has and already documents in its own
  comment ("PRIVS is #undef in options.h, so master()->valid_apply()
  ... can never return true for anyone"). Confirmed live with targeted
  instrumentation: `secure/daemon/master.c`'s own `compile_object()`
  calls `ob->set_name(nom)` directly, with no `unguarded()` wrapping, so
  the resulting `check_access()` stack-walk always denies on
  `secure/std/login` turning up privs-less in the previous-object
  chain -- for every new character, every time, not an edge case.
  `set_position()`'s own comment and existing escape hatch (authenticate
  by caller identity instead of going through `valid_apply()` at all)
  is the established fix pattern in this same file; `set_name()` now
  uses the equivalent check (`previous_object() == master()`, the only
  trusted direct caller of this exact call shape).
- **A second real mudlib bug in the same area**: `std/user.c`'s own
  `query_name()` override read `__TrueName`, a variable *reachable* from
  `user.c` only through an extremely deep, five-level inherit chain
  (`user.c` -> `LIVING` -> `secure/include/living.h`'s own hidden
  `inherit` statements -> `/std/living/combat` -> `BODY` ->
  `CONTAINER` -> `/std/Object`) that this project had never previously
  mapped (see the new finding directly below). Confirmed live:
  `::set_name(str)` genuinely resolves and runs `/std/Object`'s own
  `set_name()` (which does set `__TrueName`) without throwing, yet
  `query_name()` still read back a non-string value immediately
  afterward -- strong evidence the *object-variable slot* `__TrueName`
  occupies is not consistently the same one on both sides of this deep
  chain (see the finding below; not fixed this slice). The commented-
  out line directly above the broken one (`//tmp =
  living::query_name();`) is contemporary evidence the original
  developer never got this working correctly either. Given `char_name`
  is confirmed reliably set (by the very same `set_name()`, in the same
  assignment), `query_name()` now returns `char_name` directly --
  matching the function's own evident intent, using the variable that
  actually works, without needing to resolve the deeper slot question
  to unblock everything downstream of it (`wiz_setup_workroom()`'s own
  path concatenation, `std/user/nmsh.c`'s own `reset_prompt()` passing
  it to `replace_string()`, both confirmed live crashing on this before
  the fix).
- **`map_array()`/`map()` and `filter_array()`/`filter()`** were both
  entirely unregistered. Implemented for the two real shapes this
  mudlib uses (a `Closure`, called directly via `VM::callClosure()`; or
  a string function name plus a target object, calling
  `target->name(element, extra_args...)` for each element) -- not the
  full `filter()`'s real string/mapping first-argument forms, which
  nothing here uses. Surfaced live: `std/user/nmsh.c`'s own
  `do_nickname()`.
- **`implode()`** was also entirely unregistered (its counterpart,
  `explode()`, already existed). Implemented for the plain string-
  separator form only, matching every real call site
  (`std/user/nmsh.c`'s own `do_alias()`/`do_nickname()`); the real
  function-per-element form is not implemented.

All nine are covered by new `ctest` cases (13 added this slice).

## `do_alias()` root-caused: confirmed compiler/VM bug, object-variable slots collide across sibling multi-inherits

The `secure/include/living.h` hidden-`inherit` finding from the
previous slice (`std/living.c` actually has five real `inherit`
statements only visible after cpp expansion, giving `std/user.c` a
five-level-deep branch alongside six other parallel top-level
branches: `AUTOSAVE`, `EDITOR`, `FILES`, `NMSH`, `MORE`, `REFS`,
`LIVING`) turned out to be the same root cause as the `do_alias()`
blocker. Confirmed live with temporary instrumentation (a per-file
dump of `CompiledProgram::objectVarNames` in `ObjectManager::compile()`,
plus the `__Xverbs`/`__Aliases` checks from the previous slice),
removed once understood, per this project's own standing rule.

**The bug.** `ObjectManager::compile()` caches one `CompiledProgram`
per filename and reuses it verbatim everywhere that file is inherited
(`programCache_[filename]`, see the comment at the top of `compile()`
explaining this is deliberate, so a file inherited by several others
is only compiled once). `CodeGen::generate()` assigns every object
variable a sequential absolute slot number by walking
`inheritedObjectVarNames`, the flattened list of the *direct* parents'
own variable names, passed in by `ObjectManager::compile()`. For a
"leaf" mixin with no `inherit` of its own (`AUTOSAVE`, `EDITOR`,
`FILES`, `NMSH`, `MORE` are all leaves), `inheritedObjectVarNames` is
empty, so that file's own object variables always get local slots
starting at 0 -- correct only when that file is compiled and executed
completely on its own. `VM::run()`'s `PushObjectVar`/`StoreObjectVar`
opcodes use `instr.operand` as a *raw* index straight into
`obj->variables()`, with no per-program base-offset adjustment
(confirmed by reading both opcode cases in `VM.cpp`; there is no
`objectVarBase`/`slotBase`/offset concept anywhere in `Bytecode.hpp`,
`CodeGen.cpp`, `ObjectManager.cpp`, or `VM.cpp`).

When `std/user.c` inherits seven things in one file, each leaf
sibling's *already-compiled, cached* bytecode still carries the local
slot numbers it was given when compiled standalone. Live evidence,
captured via the temporary `ObjectManager::compile()` dump:

```
compiled /std/user/autosave inheritedCount=0 totalVars=4   (local slots 0..3)
compiled /std/user/editor   inheritedCount=0 totalVars=5   (local slots 0..4)
compiled /std/user/nmsh     inheritedCount=0 totalVars=15  (local slots 0..14)
...
compiled /std/user inheritedCount=104 totalVars=155
  [0..3]   AUTOSAVE's real absolute slots
  [4..8]   EDITOR's real absolute slots
  [9..23]  NMSH's real absolute slots (__Nicknames=9, __Aliases=10, __Xverbs=11)
```

`user.c`'s own compile correctly computes NMSH's real absolute range as
9..23 (used whenever `user.c`'s own code resolves an inherited variable
by name). But NMSH's own cached bytecode -- generated when NMSH was
compiled on its own, with `inheritedCount=0` -- still emits raw operand
`2` for `__Xverbs` (its third local variable, local slots 0/1/2 for
`__Nicknames`/`__Aliases`/`__Xverbs`), not `11`. Since the VM applies
`instr.operand` directly with no offset, every one of NMSH's own
functions actually read and write `obj->variables()[2]` -- which is
really `AUTOSAVE`'s own local slot 2, `static private int __LastSave`.
`create()` writes `__Xverbs`'s mapping into slot 2 and reads it straight
back through the same (equally wrong, but self-consistent) local slot
number, so the debug check right after `create()` showed `is_mapping=1`
for all three variables. Later, `AUTOSAVE`'s own code writes an
ordinary int into its `__LastSave` (also raw slot 2) during account
setup, silently overwriting what NMSH's own code still thinks is
`__Xverbs` -- which is exactly why the live test showed `__Xverbs`
correct immediately after `create()` and broken (`is_mapping=0`) by the
time `do_alias()` ran.

This is a general architecture bug, not specific to `nmsh.c`/`do_alias()`:
any file with two or more directly-inherited sibling files that are
each leaves (no inherits of their own) will alias each other's low
slot numbers the same way, because each leaf's cached bytecode was
compiled assuming it is the entire object. `std/user.c` (seven direct
inherits, several of them leaves) is simply the first place this
mudlib's own structure exercises it badly enough to crash. Real
FluffOS avoids this by construction: each `inherit_t` on a `program_t`
records its own `variable_index_offset`, resolved per compiled program
against its actual place in that specific object's inherit tree, and
compiled function code always addresses object variables relative to
that per-inherit base at the point of dispatch -- not via a single
globally-cached, offset-free absolute slot baked in at each file's own
standalone compile time.

**Fixed, confirmed live and by regression test, user-approved before
implementation.** Two distinct bugs, both in the compiler, both now
fixed:

1. **Missing runtime base offset** (the one described above). Fixed by
   adding `CompiledProgram::ancestorBaseOffsets`
   (`std::unordered_map<const CompiledProgram*, int>`), populated in
   `ObjectManager::compile()` right after each parent is resolved: for
   every direct parent, record the base offset its own local slot 0
   maps to within this file's own flattened layout, then merge that
   parent's own `ancestorBaseOffsets` in, shifted by the same base --
   composing correctly across arbitrarily deep chains, not just one
   level of direct siblings. `VM::run()` now computes an
   `objectVarBase` once per call (0 when the executing program *is*
   `obj->program()` itself -- the common case, no map lookup needed --
   otherwise looked up from `obj->program().ancestorBaseOffsets`), and
   both `PushObjectVar`/`StoreObjectVar` add it to `instr.operand`
   before indexing `obj->variables()`.
2. **A second, distinct bug found while regression-testing the first**:
   `CodeGen::generate()` computed a newly-declared object variable's own
   slot number as `objectVars_.size()` -- the size of a *name-keyed
   map* built from `inheritedObjectVarNames`. A private variable's
   synthesized name (`"$private#N"`) is only unique relative to the
   file that declared it (local numbering always starts at 0); two
   files reached via separate inherit branches can each independently
   produce `"$private#0"`, and when both are flattened together those
   names collide in the map, silently undercounting the real number of
   inherited slots. Fixed by tracking the next slot with a separate
   `size_t nextObjectVarSlot`, seeded from `inheritedObjectVarNames.size()`
   (a plain vector length, immune to name collisions) and incremented
   per new variable, instead of reading it back off the map.

Both fixes are covered by dedicated regression tests in
`test_lexer.cpp`: `testSiblingLeafObjectVariablesDoNotAliasEachOther`
(mirrors `std/user.c`'s own AUTOSAVE/EDITOR/NMSH shape -- two leaf
siblings, each with several private variables in one declaration
statement) and `testObjectVariableOffsetsComposeAcrossMultiLevelInheritChain`
(mirrors `std/living.c`'s own shape -- a 3-level chain plus an
unrelated sibling leaf at the top, matching `std/living/combat.c`'s
real `inherit BODY; inherit SKILLS;`). Both tests were confirmed to
fail against the pre-fix code before the fix was applied (not just
written to pass vacuously): the sibling test failed with
`leaf_two`'s init overwriting `leaf_one`'s own first variable at raw
slot 0, and the multi-level test failed via bug 2 above (`create()`'s
own new variable landed on the same slot a sibling branch's variable
already used). Full suite (277 tests) passes after both fixes.
Live-confirmed: the original `do_alias()` crash ("Index: target is not
an array, mapping, or string") no longer occurs; `__Xverbs` now stays
a correct mapping all the way from `create()` through the alias-check
codepath.

## Second, distinct bug found reaching for a room: function-call resolution order is backwards from real LPC for ancestor-overridable stubs

While re-testing live after the slot fix above, chargen got further
(no more `do_alias()` crash) but `setup()` started throwing inside
`std/user/nmsh.c`'s own `reset_prompt()`:
`replace_string: expected (string, string, string) arguments
(occurrence-range form not implemented)`. Root-caused with the same
temporary-instrumentation methodology (checkpoints bisecting exactly
where a value stopped being a string, removed once understood): NOT
object-variable corruption this time -- `char_name` was never
clobbered. The actual bug is that `query_name()`, called bare from
*within* `std/user/nmsh.c`'s own code, resolves to `nmsh.c`'s own
`string query_name() { return 0; }` (one of a block of stub functions
at the bottom of that file -- `query_hp()`, `query_max_hp()`,
`query_sp()`, `query_max_sp()`, `query_invis()`, `query_name()` --
each returning a hardcoded placeholder), not to `std/user.c`'s real
override (`string query_name() { return char_name; }`), even though
`std/user.c` inherits `nmsh.c` and legitimately overrides it.

**Confirmed against real FluffOS source, not guessed.**
`compiler.c`'s `define_new_function()` (lines 1046-1074,
`fluffos-2.9-ds2.08/compiler.c`): when a function name that was
previously seen with `FUNC_INHERITED` gets redefined further along the
same compile, the comment states plainly: "It was either an undefined
but used function, or an inherited function. In both cases, we now
consider this to be THE new definition." Real LPC compiles an object's
*entire* inherit tree into one flattened function table; when a child
(here, `std/user.c`) defines a function with the same name as
something it inherited (`nmsh.c`'s stub), the child's definition
replaces the entry in that *one shared table* for the whole object --
not just for calls written in the child's own source. Every unqualified
call to that name, including ones textually inside the ancestor's own
file, resolves through the same table and gets the override, unless
explicitly bypassed with `::` (`nomask` is the reverse: it forbids a
child from ever replacing that entry at all). This is the standard
Nightmare-mudlib idiom `nmsh.c` is using here on purpose: it defines
placeholder defaults so it compiles and runs standalone, expecting a
real inheriting file like `std/user.c` to override them -- the same
pattern used for at least five other stubs in the same block.

This driver's `OpCode::Call` does the opposite: `findFunctionInChain(program,
funcName)` searches the *currently executing* program's own lexical
scope (its own functions, then its own `inheritedPrograms`, depth-first)
first, and only falls back to `obj->program()` (the top-level, most-derived
object) when that search finds *nothing at all*. Since `nmsh.c` does
define its own `query_name()`, the lexical search succeeds locally and
the fallback -- which exists specifically to reach a child's override,
per that code's own comment ("confirmed live needed" for a different
case, `query_client()`) -- never triggers. The fallback's own reasoning
("a file's own internal calls must still resolve to its own (or its
own ancestors') definitions first, real LPC does not virtually
dispatch a parent's internal self-calls to a child's override") is the
part contradicted by `define_new_function()`'s own comment above: real
LPC's resolution is not lexical-scope-first with a not-found fallback,
it is single-flattened-table-first, always, with `::` as the only way
to reach a specific ancestor's shadowed version instead.

**Scope: likely broader than this one file.** The same
default-stub-for-standalone-use, override-in-the-real-object pattern
is a common, deliberate Nightmare/LPC idiom, not unique to
`nmsh.c`/`query_name()`. Anywhere an ancestor file provides a
placeholder that a more-derived file overrides, and the ancestor's own
code calls that name internally (not through `::`), this driver
currently gets the wrong (ancestor's own, stale) version instead of
the real override. This was only confirmed for this one call site
live; the true extent across the rest of the mudlib has not been
surveyed.

**Not yet fixed.** This is a change to the fundamental function-call
resolution order for `OpCode::Call` (and by extension `CallParent`'s
own bare-form search, and the plain `Call` opcode's existing
`query_client()`-style fallback, which would become largely
redundant), not a one-file patch -- squarely a shared-behavior change
the project's standing rule says to propose before implementing.

### Proposed fix

Invert the search order for a bare (unqualified) `OpCode::Call`: try
`obj->program()` (the top-level, most-derived program) first via
`findFunctionInChain()`, exactly like `callFunction()`/`call_other`
already do; only if that finds nothing at all should the driver treat
it as genuinely undefined and fall through to the simul_efun object,
then the efun table. This matches `define_new_function()`'s flattened-
table model: the most-derived definition always wins for a bare call,
regardless of which file's source the call is textually written in.

This does not change `OpCode::CallParent` (`::name()`/
`qualifier::name()`), which is explicitly the escape hatch for
bypassing the override and must keep searching only the *inherited*
programs, skipping the current one, exactly as it does now.

Removes the need for the existing "lexical search first, obj->program()
fallback only if nothing found" logic and its special-cased comment
about `query_client()` -- that case, and this one, are both explained
by the same single rule (most-derived wins for bare calls) once the
order is corrected, rather than being two different special
mechanisms.

Needs a regression test that exercises exactly this shape: an ancestor
file defining a stub the way `nmsh.c` does, a child overriding it, and
a call written *inside the ancestor's own source* confirming it now
reaches the child's override -- plus confirmation that existing tests
relying on the current fallback behavior for `query_client()`-style
cases still pass under the new, simpler single-rule order.

**Fixed, user-approved before implementation.** `OpCode::Call` now
searches `obj->program()` (the object's own top-level, most-derived
program) directly via `findFunctionInChain()`, instead of searching
`program` (whichever file is currently executing) first with a
not-found-only fallback. The old two-step logic and its
`query_client()`-specific comment are gone -- a single top-level-first
search is a strict superset, since `obj->program()`'s own depth-first
walk necessarily covers every program in its inherit tree, `program`
always among them. `OpCode::CallParent` (`::name()`/
`qualifier::name()`) is untouched, exactly as proposed.

The existing test `testParentCallStillPrefersItsOwnLexicalDefinitionOverChildsOverride`
encoded the old (disproven) behavior as its own expected result and
was renamed/corrected to
`testBareCallFromParentReachesChildsOverrideNotItsOwnLexicalDefinition`,
now asserting the child's override wins for a bare call written inside
the parent's own source -- exactly the `nmsh.c`/`query_name()` shape.
`testParentCallToFunctionOnlyChildDefinesResolvesAtRuntime` (the
original `query_client()`-style case) was re-confirmed passing
unchanged under the new single-rule order. Full suite (279 tests at
that point) passing.

## Three more gaps found live pushing from the fixed `reset_prompt()` toward an actual room

With both fixes above in place, live testing advanced past
`reset_prompt()` (no more crash) and surfaced three further, unrelated,
much narrower gaps in sequence -- each root-caused with the same
temporary-instrumentation-then-remove methodology, each fixed directly
(none broad enough to need a propose-first cycle) with a regression
test, confirmed live:

1. **A real compiler bug, not a mudlib bug: block scoping was never
   implemented.** `domains/Praxis/setter.c` failed to compile:
   `codegen: variable "me" already declared in this scope`. The file
   has two sibling `{ ... }` blocks (its "Store PPE"/"Store ISP"
   blocks), neither nested in the other, each declaring its own local
   `me` -- entirely legal C89/LPC, since each block is its own scope.
   `CodeGen`'s `locals_` was a single flat per-*function* map with no
   concept of nested block scope at all -- `Parser.cpp`'s own comment
   on the standalone-`{ }`-statement case said so explicitly ("this
   driver has no lexical block-scoping to enforce"). Fixed: added
   `localScopeStack_` (`std::vector<std::vector<std::string>>`);
   `declareLocal()` records each new name against the innermost open
   scope, and `emitBlock()` now pushes an empty scope before compiling
   a block's statements and erases those recorded names from `locals_`
   when the block closes. One real complication found immediately by
   the existing test suite: a comma-separated var decl (`"string a, b,
   c;"`) and a for-loop's comma-chained init/update clause and a
   braceless single-statement if/while/for branch all reuse the same
   `Block` AST node purely as a wrapper, not as a real scope --
   scoping those unconditionally broke
   `testLocalVarDeclCommaListVmExecution` (`"string a, b, c; ...
   return a + b + c;"` threw `undeclared variable "a"`, the decl's own
   names erased right after that one statement). Fixed by adding
   `Block::isRealScope` (default `true`), set to `false` at the three
   synthetic-wrapper call sites (`Parser::parseVarDeclStatement()`,
   `parseCommaExprChain()`, `parseBranch()`), and `emitStatement()`'s
   nested-`Block` case now only opens a new scope when `isRealScope`
   is true, otherwise flattening directly into the enclosing scope as
   before. Two new tests:
   `testSiblingBlocksMayReuseALocalNameNeitherNestedInTheOther` (the
   real `setter.c` shape) and
   `testNameDeclaredInABlockIsUndeclaredOnceThatBlockEnds` (confirms
   the block-exit boundary is real, not just non-colliding).
2. **`sprintf`'s `"%c"` was unimplemented.** `/daemon/terminal`'s own
   `create()` failed: `sprintf: unsupported format specifier '%c'`.
   `daemon/terminal.c`'s `ANSI(p)`/`ESC(p)` macros build a raw ESC
   (ASCII 27) byte via `sprintf("%c[" + (p) + "m", 27)`. Confirmed
   against `fluffos-2.9-ds2.08/sprintf.c`: `INFO_T_CHAR` requires a
   `T_NUMBER` (int) argument, mapped straight through to C's own
   `sprintf(..., "%c", ...)`. Implemented to match (throws if the
   argument is not an int, same convention as the existing `%s`/`%d`
   cases). Two new tests:
   `testSprintfPercentCEmitsSingleCharacterFromIntArgument`,
   `testSprintfPercentCThrowsOnNonIntArgument`.
3. **`living()` was a missing efun.** `move()` (bare-called from
   `std/user.c`'s `setup()`, resolving via the just-fixed `Call`
   opcode to `std/living.c`'s own `move()`, which itself calls
   `::move()` up to `std/Object.c`'s base implementation) threw
   `undefined function or efun: living`. `std/Object.c`'s own `move()`
   gates `move_object()` behind `living(this_object()) && living(ob)`
   (blocking one living thing moving directly into another, aside from
   the `"mountable"` exception). Confirmed against
   `func_spec.c`: `"int living(object default: F__THIS_OBJECT);"`, and
   `add_action.c`'s `f_living()`: returns whether
   `O_ENABLE_COMMANDS` is set on the object, nothing more. Implemented
   directly on top of the existing `commandsEnabled()` flag this
   driver's `enable_commands()`/`disable_commands()` pair already
   maintains (from the earlier `add_action` subsystem work), defaulting
   the argument to `current_object()` per the real signature. New test:
   `testLivingReflectsEnableCommandsStateAndDefaultsToCurrentObject`
   (enable/disable round trip, default-argument form, and confirms the
   flag is per-object, not global).

Full suite: 282 tests passing after all three.

## Live confirmed: chargen now genuinely runs, reaches attribute rolling

With every fix above in place, a fresh live account-creation test
(`roomtestfive`) now shows the real chargen banner for the first time
this project has ever reached it:

```
=== STEP 1: CHOOSE YOUR STARTING ZONE ===
Where does your story begin on Rifts Earth?
 americas   The Americas (Chi-Town)
 europe     Europe (New Camelot)
 atlantis   Atlantis (Splynn market shores)
Type your choice: americas, europe, or atlantis.

> americas
The Americas. You will begin at the edge of Chi-Town.
=== STEP 2: ROLL ATTRIBUTES ===
Roll Palladium attributes (3d6 each for IQ, ME, MA, PS, PP, PE, PB, Spd).
Type: roll
After rolling you must type accept to keep the roll, or reroll
(up to 4 rerolls, 5 total rolls). Race selection stays locked
until you type accept.
```

This matches the documented real chargen flow exactly (see the
mudlib's own `CLAUDE.md`, "Chargen input model is plain-string only").
Not yet a full room: reaching one requires completing the rest of
chargen (`roll`/`accept`, race, OCC, alignment, skills), each of which
may surface further gaps not yet exercised live. All debug
instrumentation used to root-cause every issue in this and the
previous section has been removed, confirmed via grep across every
touched mudlib file.

## Resuming after a reboot: orphaned process, and a stale test script

The previous session ended mid-investigation when an unplanned system
reboot killed it. On resume, `ps`/`ss` found a driver process already
listening on the scratch port (1123) -- started *after* the reboot
(`19:49`, boot was `18:53`), from a scratch config in a different
session's own scratchpad directory, with an empty log and no reachable
owning agent. No save/`.o` file anywhere had been touched since boot.
Confirmed orphaned (a lost session's own scratch instance, not
something in progress) and killed rather than reused, per this
project's own "confirm state from live evidence, don't guess" practice.

Driving the live socket test itself needed its own small fix first:
`mudlib/tools/playtest_create_chars.py` (the project's existing,
previously-working chargen-driver script, dated 2026-07-10) no longer
matches `secure/std/login.c`'s current account-confirm prompt. The
script's `login_new()` expects either "really wish" or "choose a
password" right after the account name is sent; the real prompt is now
"Confirm `<Name>` as your account and first character name? (y/n)",
added since the script was written. Neither substring matches, so the
script's next `send()` (meant to be the password) actually answers the
`(y/n)` confirm prombt instead -- landing on `secure/std/login.c`'s own
`new_user()` decline branch (`if((str = lower_case(str)) == "" ||
str[0] != 'y') { ... __Player->remove(); ... }`) essentially every time.
This is almost certainly *why* the previous session was investigating
`daemon/occ.c` in the first place: not a real content question, but a
downstream symptom of this same stale-script bug reliably crashing
account creation. A corrected probe script (this session's own
`chargen_probe.py`, scratch-only, not committed to the repo) answers
the confirm prompt explicitly with `y` before sending the password.

## Chargen closed the loop: full run confirmed live, reaching a real room

With the corrected probe script, the live walk reproduced the actual
blocker directly rather than continuing the grep/awk investigation
blind: `new_user()`'s decline branch (reached via the stale-script bug
above, but a real code path a genuine user could also hit by literally
answering anything other than `y`) called `__Player->remove()` on the
speculatively pre-created player object (`secure/std/login.c`'s own
comment explains why `player_object()` runs before the confirmation:
`compile_object()` needs the char name already set). `std/clean_up.c`'s
`remove()` needs `all_inventory()` to hand equipment back to the
environment before destructing -- an efun this driver never had.

**Investigating the `daemon/occ.c` empty-`attribute_requirements`
question first**, since it was the exact point of interruption: closed
as genuine content, not a driver bug. Both real consumption sites
(`domains/Praxis/setter.c`'s `do_occ_pick()` and
`offer_occ_or_reroll()`) guard with `if(reqs && sizeof(reqs))` before
ever touching the mapping, so an empty `([])` is unambiguously "no
requirements" and behaves correctly regardless of how this driver
represents an empty mapping literal internally. The OCCs carrying it
(`vagabond`, `wilderness scout`, `city rat`, `rogue scholar`, `tribal
warrior`, `smuggler`, `pirate (s.a.)`, `sailor (s.a.)`, `gifted gypsy`,
plus three race-gated OCCs where race membership is already the hard
gate) are civilian/generalist classes with no stat floor in the source
material, consistent with every non-empty entry elsewhere in the same
file. Confirmed live twice over: all of them show up correctly in
STEP 5's offered OCC list, and `vagabond` specifically (line 225's
empty mapping) was picked live with no crash and no false rejection.

**Nine more gaps found and fixed continuing the walk from there, each
root-caused against real FluffOS source before implementing, same as
every other slice this project has done:**

1. **`all_inventory()`/`deep_inventory()` missing entirely** (func_spec.c:
   `object *all_inventory(object default: F__THIS_OBJECT);` /
   `object *deep_inventory(...)`). Confirmed against array.c's own
   `all_inventory()` (direct children only, no recursion) and
   `deep_inventory_count()`/`deep_inventory_collect()` (depth-first,
   target excluded). Backed directly by `LpcObject::inventory_`, already
   maintained by `VM::moveObject()` -- no new bookkeeping needed. This
   was the account-creation blocker above.
2. **`strcmp()` missing**, silently swallowed by `secure/std/login.c`'s
   own `catch(__Player->setup())` with no console trace -- the same
   "quiet cascade" shape as the earlier `__HistorySize` investigation.
   `/secure/daemon/player.c`'s own `sort_list()` needed it; a fresh
   player's `setup()` was quietly failing to register itself with
   `player.c`'s own online-player list. Matches real `efuns_main.c`'s
   `f_strcmp()`: a plain C `strcmp()`.
3. **`map_delete()` missing**, *not* caught -- fatal to the connection.
   `std/living/env.c`'s own `remove_env()` (`if(env_var && env_var[env])
   { map_delete(env_var, env); ... }`), reached unguarded from
   `domains/Praxis/setter.c`'s `alignment_cmd()`. This is what actually
   stopped STEP 4 (alignment) from ever reaching STEP 5 (OCC) live.
   Matches real `efuns_main.c`'s `f_map_delete()`: mutates the mapping
   in place, void return.
4. **`clone_object()`/`load_object()` doubled a caller-supplied `.c`
   extension.** `ObjectManager::compile()` appended `.c` unconditionally,
   so `daemon/rifts_start_d.c`'s own `give_item(player, "id_card.c")`
   resolved to a literal, never-existing `id_card.c.c` and aborted
   `finish_creation()` partway through granting starting equipment --
   the actual blocker stopping a fresh character from ever reaching a
   room. New `ObjectManager::normalizeFilename()` strips one trailing
   `.c` at every entry point (`compile()`, `loadObject()`,
   `cloneObject()`, `sourceFileExists()`, `lookupLoadedObject()`) so
   `"id_card"` and `"id_card.c"` resolve to the exact same cache entry
   and object identity, matching real LPC's own convention that object
   paths never carry the extension internally.
5. **`intp()` missing**, the one type predicate not covered alongside
   `stringp`/`objectp`/`mapp`/`pointerp`/`functionp` from earlier
   slices. `/domains/Praxis/equipment/id_card.c`'s own `set_value()`
   needed it directly, reached while granting starting equipment.
6. **`repeat_string()` missing** (func_spec.c/efun_defs.c: `F_REPEAT_STRING`,
   real body in `packages/contrib.c`'s `f_repeat_string()`: string
   concatenated with itself N times, `""` for N <= 0).
   `cmds/mortal/_score.c`'s own `panel_border()` needed it -- caught by
   `setter.c`'s own `catch()` around `finish_creation()`'s auto-score-
   display, so not fatal, but the score panel border was silently never
   rendering until fixed.
7. **`present()` missing** (func_spec.c: `object present(object | string,
   void | object);`). Confirmed against `simulate.c`'s
   `object_present()`/`object_present2()`: the string form searches a
   container's direct inventory for an item whose `id()` apply returns
   truthy (falling back to the calling object's own environment when no
   container is given and the direct search misses); the object form
   checks direct-containment or, with no explicit container,
   sibling-of-current-object. This blocked `domains/ChiTown/areas/
   chitown_start.c`'s own `reset()` -- the very first starting room a
   fresh character reaches -- via exactly the `present("id",
   this_object())` idiom `mudlib/CLAUDE.md`'s rule 11 documents as the
   standard anti-duplication check. Not implemented: the numbered-suffix
   form (`"sword 2"`), not confirmed needed anywhere reached live yet.
8. **The deepest one: `explode()` never matched real FluffOS's own
   default separator semantics.** Confirmed against fluffos-2.9-ds2.08's
   own `array.c` `explode_string()` *and* this exact vendored
   reference's own `options.h` (`#undef SANE_EXPLODE_STRING` / `#undef
   REVERSIBLE_EXPLODE_STRING` -- the default build any of this mudlib's
   own content was written against): every **leading** occurrence of the
   separator is stripped before splitting (repeatedly, not just the one
   `SANE_EXPLODE_STRING` would limit it to), and the final chunk is only
   kept if non-empty, so a **trailing** separator never produces a
   trailing `""` element. This driver's original implementation did a
   naive split with neither behavior. Root-caused by tracing why
   `secure/SimulEfun/security.c`'s own `file_privs()` never matched any
   of its `switch(path[0])` cases for a real object path:
   `"/domains/..."` exploded on `"/"` produced a leading `""` as
   `path[0]` instead of `"domains"`, shifting every real path segment
   one index late -- which is what made every object's compile-time
   `privs` assignment fail silently (see the next item), which is in
   turn what made `secure/SimulEfun/log_file.c`'s own
   `explode(query_privs(previous_object()), ":")` throw for any object
   reached through it (`domains/Praxis/obj/mon/rift_survivor.c`'s own
   `set_stats()`/`set_level()`, cloned by the starting room's own
   `reset()`). The trailing-empty-element half of this same bug had
   already been worked around locally in `daemon/race.c` (`LIMB_DIR`
   file reading) in the previous session, before this root cause was
   found; that guard is left in place as a harmless, independently
   reasonable defensive check (it matches this mudlib's own
   `database_filter()` convention, per its own comment) rather than
   reverted now that the driver itself is fixed.
9. **A previously entirely missing mechanism: this driver never
   auto-assigned an object's compile-time `privs`.** Real `simulate.c`'s
   own `init_privs_for_object()` (called from `init_object()` for every
   freshly compiled or cloned object, before its own `create()` runs)
   applies `master()->privs_file(filename)` and stores the result if
   it's a string. This driver had no equivalent at all -- `query_privs()`
   only ever returned a real value if a mudlib file called `set_privs()`
   on itself directly, which essentially nothing in this mudlib does
   (privs are meant to come from `master.c`'s own `privs_file()`
   automatically). New `ObjectManager::initPrivsForObject()`, called
   from both `loadObject()` and `cloneObject()` right after construction,
   closes this gap. Skipped only when `master_` itself is not loaded yet
   (matches real `init_privs_for_object()`'s own `!current_object`
   bootstrap-skip outcome closely enough -- nothing this driver runs
   depends on the master object's own privs).

Also added **`remove_call_out()`** (func_spec.c: `int
remove_call_out(int | void | string);`) alongside the fixes above, found
needing it the same pass: `domains/Praxis/obj/mon/rift_survivor.c`'s own
`init()` does the common defensive cancel-then-reschedule idiom for a
repeating `call_out()`. Since this driver's own `call_out()` is a
documented stub that never actually schedules anything yet
(`Scheduler::tickCallOuts()` is still an empty body), `remove_call_out()`
always returns `-1` -- the real "nothing found" outcome, honestly
reflecting that nothing is ever really pending, not a fake success.

Full suite: 288 tests passing (11 new regression tests this session,
one per confirmed gap above).

**Confirmed live, full transcript (fresh account `chargenthirteen`,
scratch instance, port 1123):** login through account creation
(gender, display name, email, real name all accepted blank), the
one-time first-account admin-bootstrap offer (declined), zone
(`americas`), attribute roll/accept, race (`human`, `list` also
exercised), alignment (`scrupulous`), OCC pick (`vagabond`, the
empty-`attribute_requirements` case deliberately chosen), automatic
starting-equipment grant (combat knife, C-18 laser pistol, leather
jacket, all via real `clone_object()` calls), `finish_creation()`'s own
automatic room entry:

```
A Rift tears open around you and reality reassembles.
You step onto Rifts Earth. Welcome, Human.
A human appears from the shadows.
A reinforced shelter of scavenged plating and pre-Rifts ferrocrete,
built into the corridor between the old Coalition road south to
Praxis and the checkpoints of Chi-Town to the north. A steady
trickle of new arrivals passes through here: refugees, mercs, and
the newly rifted-in alike.

A battered sign is nailed to a support beam near the door. A
survivor watches the corridor from a folding chair. There are two exits: north, south
A weathered survivor.
```

**Correction (caught during the "Real call_out()/heart_beat() scheduler"
slice, see below): a separately, explicitly typed `look` command was
NOT actually confirmed here.** What this session's probe script reported
as "look's own response" was trailing, already-in-flight output from
`finish_creation()`'s own automatic display, misattributed to a `look`
sent moments later by a timing coincidence in the probe's own buffering
-- not a genuine round trip. The real `look` command did not work at
all at this point; it silently produced nothing, for reasons entirely
unrelated to this session's own chargen fixes (a dispatch-argument bug
described in full below). This was only caught later, by a more
rigorous probe that drains the connection to genuine idle before
sending a command and checking for new bytes -- see "Real call_out()/
heart_beat() scheduler" below for the real fix and the real
confirmation.

**One remaining known gap surfaced live, non-fatal:** `cmds/mortal/
_score.c`'s own `panel_two_col()` (part of the automatic score display
`finish_creation()` triggers) uses `sprintf`'s `%*` dynamic-field-width
specifier, still not implemented (see "Known stubs" below -- this
extends that existing, already-documented `sprintf` scope limitation,
not a new one). Caught by `setter.c`'s own `catch()` around
`finish_creation()`, so it does not block reaching the room or using
`look` -- only the score panel's two-column layout silently fails to
render. Test data cleanup: all scratch-instance test accounts created
this session (`chargenthree` through `chargenthirteen`) were throwaway
names on the scratch port only, confirmed by mtime to be within this
session, and deleted (`secure/save/login_accounts/c/*.o`,
`secure/save/postal/c/*`) before the scratch driver was stopped; no
player-object save under `secure/save/users/` was ever created (no
test character reached `quit`).

## Real call_out()/heart_beat() scheduler: recon, design, implementation, and a genuinely deep dispatch bug found live

With chargen reaching a real room, `call_out()`/`heart_beat()` never
actually firing became the single largest remaining gap (see the
driver-comparison docs under `docs/driver-comparisons/`, corrected to
say so during the same review pass that preceded this slice). Same
rigor as the closures/`add_action`/`catch()` work: recon real usage
first, design grounded directly in FluffOS source, implement, test,
then verify live.

### Step 1: recon (real usage across the mudlib)

373 `call_out()` call sites, 30 `remove_call_out()`, 15
`set_heart_beat()`, 10 `find_call_out()`. Confirmed:

- Delays are a mix of fixed constants (2, 5, 10, 60, 120, 300, 600, 900,
  1800, 3600 seconds), computed expressions (`5*con`, `(random(6)+1)*
  3600`, a daemon-queried interval), and **zero-delay `call_out(fn, 0)`**
  used pervasively (~30 sites) as a "run on the next tick" idiom
  (deferred `equip_gear()` on NPC spawn, deferred self-destruct, `std/
  room.c`'s own `create() { ...; call_out("reinitiate", 0); }`, etc).
- `remove_call_out()` is called **both by function name (string, the
  overwhelming majority) and by a stored numeric handle**
  (`cmds/mortal/_trade.c`'s own `tid = call_out(...); ...;
  remove_call_out(tid)`), and `while(remove_call_out("x") != -1);` loops
  confirm multiple same-named call-outs can coexist, removed one match
  at a time.
- `find_call_out()` (10 sites) is used as an existence/dedup check
  before scheduling -- not implemented in this driver at all before
  this slice.
- `set_heart_beat()` is called with values other than 0/1 in places
  (`std/germ.c`'s own `set_heart_beat(5)`) -- real semantics: the
  argument is a per-object heartbeat-cycle interval, not a bare on/off
  flag.
- The dominant repeating-timer idiom is **self-rescheduling call_out**
  (a function calls `call_out(itself, N)` again at the end of its own
  body) -- e.g. `std/user.c`'s `rifts_regen_tick()`/
  `rifts_hp_regen_tick()`, scheduled at 60s/120s in `setup()`, each
  rescheduling itself. `std/living.c`'s `heart_beat()` uses `time()`
  deltas rather than trusting exact tick cadence, so it is robust to
  imprecise firing.
- `heart_beat()` bodies do real, observable gameplay work: `std/user.c`
  runs healing/regen and `continue_attack()`; `std/living.c`'s does
  aging and a 3600-second Rifts-regen/sun-exposure check; NPC files use
  it for AI.

### Step 2: design (grounded directly in fluffos-2.9-ds2.08's `call_out.c`/`backend.c`)

Not guessed -- read directly, matching this project's own standing
practice:

- `call_out.c`'s `new_call_out()`: `if (delay < 0) delay = 0;` -- never
  rejected, clamped. `CALLOUT_HANDLES` is confirmed **active** in this
  exact vendored build's `options.h`, so the handle-returning
  `find_call_out(int|string)` / `remove_call_out(int|void|string)`
  signatures (`func_spec.c`) are the correct target, not the
  handle-less alternative.
- `remove_call_out(object_t *ob, const char *fun)`: matches only entries
  where `(*copp)->ob == ob && strcmp((*copp)->function.s, fun) == 0` --
  scoped to the *calling* object, and a closure-bound entry's own
  `cop->ob` is never set for the string form, so a name-based removal
  can never match a closure-scheduled entry. `find_call_out()` shares
  the same match rule.
- `call_out()`'s own main loop (`call_out.c`): "Move the first call_out
  out of the chain" before invoking it, then advances `current_time`
  toward real time one second at a time, calling `call_heart_beat()`
  whenever `current_time % HEARTBEAT_INTERVAL == 0`.
  `HEARTBEAT_INTERVAL` is **2** (real seconds) in this exact vendored
  build's `options.h`, confirmed by reading the macro directly rather
  than assumed.
- `backend.c`'s `set_heart_beat(object_t *ob, int to)`: four real
  branches, read directly rather than guessed --
  `to == 0` disables and removes the object from `heart_beats[]`;
  `to != 0` on an object not yet enabled adds a fresh entry with
  `time_to_heart_beat = heart_beat_ticks = to` (negative `to` clamped to
  1); `to != 0` on an object already enabled updates the interval on a
  positive `to`, or is rejected as a no-op on a negative one.
  `query_heart_beat(object_t*)` returns the real configured interval
  (`heart_beats[index].time_to_heart_beat`), not a bare 1.
- `call_heart_beat()` (`backend.c`): decrements every enabled object's
  own tick countdown, fires `heart_beat()` on any that reach zero, then
  resets that object's countdown back to its own configured interval.
  Errors during a fired call_out/heart_beat are caught via
  `SETJMP`/`restore_context` per call, so one throwing call cannot stop
  the rest of that cycle.

This driver's own data-structure choices, deliberately simpler than
real FluffOS's ring-buffer-of-linked-lists (`call_list[CALLOUT_CYCLE_SIZE]`)
while preserving identical *observable* behavior -- the same "simplify
the internal representation, match the real contract" pattern this
project already used for closures' lazy name resolution:

- `Scheduler::CallOutEntry`: an absolute `steady_clock::time_point`
  `dueAt` instead of real `call_out.c`'s delta-encoded ring-buffer slot,
  a genuinely unique `int64_t handle` (a bare monotonic counter, not
  real `new_call_out()`'s own slot-plus-`unique`-counter encoding --
  nothing in this mudlib inspects a handle's bit structure, only
  compares it back or checks truthiness), and either a `function` name
  string (with a `weak_ptr<LpcObject> target` owner) or a bound
  `Closure`, covering both real forms.
- `Scheduler::HeartbeatEntry`: `weak_ptr<LpcObject>` plus
  `ticksRemaining`, mirroring real `heart_beats[]`. The configured
  interval itself lives on `LpcObject::heartbeatInterval()` (replacing
  the previous plain bool `heartbeatEnabled_`), so `query_heart_beat()`
  can report it faithfully.
- `tickCallOuts()`: collects every due entry into a separate vector
  *before* invoking any of them, then fires each one -- matching real
  `call_out.c`'s own "move out of the chain first" ordering, needed
  because the dominant real idiom is a call_out that reschedules itself
  from within its own body (confirmed in recon above).
- `tickHeartbeats()`: same two-phase shape (decide who fires this cycle
  first, entirely before calling any LPC code; fire afterward from a
  separate snapshot) -- see the crash section immediately below for why
  this was not optional.
- The real 2-second cadence gate lives in `Scheduler::run()`'s own loop
  (comparing elapsed wall time against `lastHeartbeat_`), not inside
  `tickHeartbeats()` itself, so `tickHeartbeats()`/`tickCallOuts()` stay
  pure, deterministic, directly-testable functions -- the same reasoning
  already documented for why `Server::dispatchLine()` was pulled out of
  `handleConnection()` as its own directly-testable method.
- `VM` gained a `Scheduler*` back-pointer (`setScheduler()`), set from
  `main.cpp` right after `Scheduler` is constructed -- the same
  "set the back-pointer after construction" pattern
  `ObjectManager::setVM()` already uses, needed because `call_out()`/
  `remove_call_out()`/`find_call_out()`/`set_heart_beat()` are
  registered on `EfunTable`, which only receives `VM&`.

### Step 3: implementation

`call_out`, `remove_call_out`, `set_heart_beat`, `query_heart_beat`
rewritten to route through the real `Scheduler`; new `find_call_out`
added. All four confirmed against the design above, each with its own
citation in `EfunTable.cpp`.

### A genuine crash found live: iterator invalidation in `tickHeartbeats()`

The very first live test after wiring everything up **segfaulted** --
confirmed via `systemd-coredump`, not inferred:

```
Stack trace of thread ...:
 #4  lpcdriver::HeartbeatEntry::operator=(HeartbeatEntry&&)
 #7  std::vector<lpcdriver::HeartbeatEntry>::_M_erase(...)
 #9  lpcdriver::Scheduler::tickHeartbeats()
 #10 lpcdriver::Scheduler::run(lpcdriver::Server&, int)
```

Root cause: the first version of `tickHeartbeats()` held a live iterator
into `heartbeats_` across the `vm_.callFunction(obj, "heart_beat", {})`
call. Real `std/user.c`'s own `heart_beat()` calls `set_heart_beat(0)`
on itself (`if(!interactive(this_object())) { set_heart_beat(0);
return; }`), which re-enters `Scheduler::setHeartbeatInterval()`, which
mutates `heartbeats_` via `erase()`/`find_if()` -- invalidating the
outer loop's own iterator mid-iteration. Fixed the same way
`tickCallOuts()` was already safe: collect a separate snapshot of who
fires this cycle *before* calling any LPC code, so a re-entrant
`set_heart_beat()` call from inside a firing `heart_beat()` can never
corrupt the structure still being iterated. Covered by a dedicated
regression test reproducing the exact real shape (an object disabling
its own heartbeat from within `heart_beat()`, alongside an unrelated
"bystander" object confirmed unaffected).

### Step 4: live verification uncovered a much older, much deeper bug -- not in the scheduler

With the crash fixed, live testing reached a room and scheduled a
throwaway test call-out (`cmds/mortal/_testscheduler.c`, not part of the
game, deleted afterward) -- but a plain `look` command, sent
immediately after, produced **zero bytes**, with no error anywhere.
Disabling `tickHeartbeats()`/`tickCallOuts()` entirely (a bisect test)
did not fix it, proving the scheduler itself was not the cause. This
also meant a claim earlier in this document -- that a live, explicitly
typed `look` command had already been confirmed working, in "Chargen
closed the loop" -- was wrong: re-checked with a probe that drains the
connection to genuine idle before sending a command, `look` (and every
other command) had *never* actually worked live in this driver, for any
session. That correction is recorded in place above rather than edited
away.

The real root cause, found by direct C++-level instrumentation rather
than guessing (three distinct, chained gaps, each confirmed against
real FluffOS source in turn):

1. **`get_dir()` never implemented glob patterns.** `daemon/command.c`'s
   own `rehash()` (behind `find_cmd()`, which every single `add_action`-
   dispatched command depends on) calls `get_dir(val[i]+"/_*.c")` -- a
   genuine glob, not the bare-directory-or-bare-file shape this efun's
   original implementation assumed was the only real usage. Against a
   literal `"*"` in the path, `stat()` always failed and this efun
   silently returned an empty array, so `__Cmds` was never populated and
   `find_cmd()` returned 0 for **every single verb**. Fixed: the
   directory portion of the path is literal, only the final path
   component may carry a wildcard, matched via POSIX `fnmatch()` against
   that directory's own entries.
2. **The real, deepest bug: `dispatchCommand()` passed an empty string,
   not real LPC's own `0`/undefined, for a bare verb with nothing after
   it.** Confirmed directly against `add_action.c`'s own `user_parser()`:
   `if (s->flags & V_NOSPACE) { copy_and_push_string(...); } else if
   (buff[length] == ' ') { copy_and_push_string(...); } else {
   push_undefined(); }` -- the undefined branch fires whenever there is
   genuinely nothing after the matched word, for both the plain
   exact-match and V_SHORT cases (only V_NOSPACE reslices differently).
   This driver's `splitVerbAndArg()` always produced a `std::string`
   (empty when there was nothing there), never a true "no argument"
   value. `cmds/mortal/_look.c`'s own `cmd_look(str)` checks
   `if(stringp(str))` first, and an empty string passes that check (real
   LPC: `stringp()` checks the type, not truthiness) -- silently routing
   a bare `look` into `examine_object("")` instead of the intended
   `this_player()->describe_current_room(1)` fallback, which itself
   declines on `if(!str) return 0;`. `cmd_look()`, and therefore
   `cmd_hook()`, and therefore the whole dispatch, returned falsy with
   no exception, no dropped connection, and nothing mudlib-visible to
   explain it -- confirmed only by instrumenting `dispatchCommand()`
   itself and reading its actual action-table matches and return values.
   Fixed: `splitVerbAndArg()` now returns `std::optional<std::string>`,
   and `dispatchCommand()` constructs a real monostate `Value{}` when
   there is nothing after the verb, matching `push_undefined()` exactly.
3. **`message()` ignored its own `targets` argument, always writing to
   whichever connection happened to be "currently active."** Harmless
   on every path this driver had run before this slice (every real call
   site was `message(type, text, this_object())`, where `this_object()`
   already was the active connection's own object) -- until
   `call_out()`/`heart_beat()` genuinely started firing with **no**
   active connection at all, and `secure/SimulEfun/communications.c`'s
   own `tell_object(ob, str)` (`message("tell", str+"", ob)`, this
   mudlib's single most common way to notify a player from a timer)
   produced nothing. Fixed with a real object-to-connection lookup:
   `InteractiveRegistry` (previously membership-only) now also maps each
   registered object to its own `Connection*`, and `message()` routes to
   the named target (a single object or an array of them) when one is
   given, falling back to the current connection only when no `targets`
   argument was passed at all.

Each of the three is covered by its own dedicated regression test
(`get_dir` glob matching in the final path component only,
`dispatchCommand` passing undefined vs. a real string for bare vs.
compound commands, `message()` routing to a target's own connection
across two independent sockets, confirmed with neither connection
"current").

### Confirmed live, full transcript

Fresh account (`truefinal`, scratch instance, port 1123), full chargen
through OCC pick (`vagabond`), automatic room entry, and this time a
**genuinely confirmed** explicit `look`:

```
A Rift tears open around you and reality reassembles.
You step onto Rifts Earth. Welcome, Human.
A reinforced shelter of scavenged plating and pre-Rifts ferrocrete,
built into the corridor between the old Coalition road south to
Praxis and the checkpoints of Chi-Town to the north. A steady
trickle of new arrivals passes through here: refugees, mercs, and
the newly rifted-in alike.

A battered sign is nailed to a support beam near the door. A
survivor watches the corridor from a folding chair. There are two exits: north, south
A weathered survivor.
```

-- confirmed both as the automatic display *and*, separately, as the
real response to an explicitly typed `look` sent after the connection
was drained to genuine idle (no bytes at all for 3+ full seconds) --
455 new bytes arrived, the same real room description, on demand. Then:

```
>>> testscheduler
Scheduling a delayed message 3 seconds out via call_out.
The survivor glances over. "Fresh out of a Rift. You look confused. Say 'help' if you need a rundown."
CALLOUT FIRED: the real scheduler works.
```

-- confirming, in one pass: `call_out()` genuinely schedules and later
fires (the 3-second delayed message, via the throwaway
`cmds/mortal/_testscheduler.c` test command, deleted after this
session); `heart_beat()` genuinely fires on a real NPC (`rift_survivor`'s
own AI dialogue line, unprompted, driven purely by its own heartbeat,
not anything this session's test script sent); and `message()`/
`tell_object()` correctly routes a delayed message to the right player
even with no connection "current" at the moment it fires.

Full suite: 304 tests passing (16 new/updated this slice: the
iterator-invalidation regression, call-out registration/firing/removal/
closure-form/error-isolation/destructed-target coverage, heart_beat
enable/disable/interval-cadence/error-isolation/destructed-object/
reentrant-self-disable coverage, the `get_dir` glob fix, the
`dispatchCommand` undefined-argument fix, and the `message()` routing
fix).

## Known stubs / scope limitations (intentional, not bugs)

- Object-bound closures (`(: obj_expr, "funcname" :)`), bare string-
  constant closures (`(: "literal" :)`), and the `(*fp)(args)`
  dereference-call syntax are all implemented now (see "Closure/
  function-pointer forms completed" above) -- this bullet is
  historical, kept for the git-blame trail rather than deleted.
- `ApplyTable::isKnownApply()` recognizes `disconnect` (never called yet)
  alongside `heart_beat`, which is now real -- see "Real call_out()/
  heart_beat() scheduler" below. This bullet is historical for
  `heart_beat`, kept for the git-blame trail rather than deleted.
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
- `sscanf()`'s "%s" directly adjacent to another "%"-specifier with no
  literal text between them is not implemented.
- Postfix/prefix `++`/`--` only support a bare variable name target, not
  an index expression (`arr[i]++`).
- `throw()` is not implemented (`catch()` is).
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
- `save_object()`/`restore_object()` use this driver's own recursive
  serialization format (see "Working now" above), not real FluffOS's
  on-disk text format. A real, pre-existing save file in that format
  (e.g. `daemon/save/banish.o`, which ships with this mudlib) is not
  parsed -- every line is silently skipped (no tab separator matches
  this driver's own format), so the object's variables simply keep
  whatever defaults its own `create()` already set, which has been
  sufficient for everything reached live so far but means real
  historical save data never actually loads.
- `find_player()`, `userp()`/`query_once_interactive()`, and
  `interactive()` are backed by `InteractiveRegistry`, which only
  tracks *currently* live connections (cleared on disconnect), not real
  FluffOS's separate "has this object ever been interactive"
  (O_ONCE_INTERACTIVE) or living-name-table concepts. Correct for every
  object this driver's own login/account-creation path actually checks
  (always still connected at the point it is checked); wrong for an
  object that was once connected and has since disconnected.
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
- `destruct()` only closes the connection currently bound to the
  destructed object, if it is the one driving the current call; it does
  not otherwise remove a destructed object from `InteractiveRegistry`
  if reached some other way, and this driver has no `O_DESTRUCTED`
  flag/guard on every apply the way real FluffOS does (an already-
  "destructed" `LpcObject` in this driver just keeps working as a plain
  C++ object until its last `shared_ptr` reference actually drops).
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
- `set_heart_beat()`/`query_heart_beat()` correctly store and report
  the flag, but nothing reads it back yet -- there is no periodic
  heartbeat scheduler in this driver at all, so setting the flag has no
  runtime effect beyond being queryable.
- `set_living_name()` stores the name on the object but wires up no
  lookup table for it, matching `find_player()`'s own pre-existing
  simplification (InteractiveRegistry + `query_name()`, not a real
  living-name table).
