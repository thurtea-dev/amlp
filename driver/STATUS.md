# STATUS

Snapshot as of the slice that finished implementing the general
closure/function-pointer forms the previous slice found and deferred
(`(: comma_expr :)` inline lambdas, bare string-constant closures, and
`(*fp)(args)` dereference-call syntax), then kept working straight
through `std/user.c`'s entire inherit chain until it actually compiled
and a live account could reach `create()`. That pass surfaced and fixed
several more small, well-understood parser/codegen gaps along the way
(indexed `++`/`--`, indexed assignment used as a sub-expression rather
than a statement, `private` object-variable scoping across an inherit
chain, the `to_int()` efun) before landing on the next real blocker:
the entire `add_action`/`enable_commands` command-dispatch subsystem is
unimplemented (see "The next real blocker" below) -- an architecturally
significant chunk of work, not a routine gap, flagged rather than
started speculatively.

## Working now

- Clean build via `cmake -B build -S . && cmake --build build`.
- `ctest --test-dir build` passes (229 unit test cases in one binary,
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

## The next real blocker: `add_action`/`enable_commands` command-dispatch subsystem is entirely unimplemented

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

## Known stubs / scope limitations (intentional, not bugs)

- Object-bound closures (`(: obj_expr, "funcname" :)`), bare string-
  constant closures (`(: "literal" :)`), and the `(*fp)(args)`
  dereference-call syntax are not implemented -- see the next real
  blocker above. Only the bare-name-with-bound-args closure form
  (`(: name, args... :)`) exists.
- `ApplyTable::isKnownApply()` recognizes several applies (`heart_beat`,
  `disconnect`, etc.) that the driver never actually calls yet.
- `Scheduler::tickHeartbeats()` / `tickCallOuts()` are empty function
  bodies, present only to establish where that logic will go. `call_out()`
  (both the string-function-name and closure forms) validates its
  arguments and returns a handle but does not actually schedule
  anything yet. Nothing on the login/account-creation path needs a
  call-out to actually fire (`logon()`'s own `call_out("idle",
  LOGON_TIMEOUT)` is a 180-second idle-disconnect timer, never reached
  in a normal walkthrough).
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
- `sprintf()` implements only bare `%s`/`%d`, positionally, with no
  field width/precision/flags and no literal `%%` -- confirmed the only
  shapes used anywhere on this driver's login/account-creation path;
  throws on anything else.
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
- `message()` ignores its `type`/`targets`/`excludes` arguments and
  always writes straight to the connection currently driving the call
  (`OutputContext::current()`) -- there is no reverse "object -> its
  connection" lookup in this driver to route a message to a *different*
  object's connection. Confirmed the only shape this driver's login/
  account-creation path uses (always `message(type, text,
  this_object())`).
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
- Object variables declared but never explicitly assigned (e.g. no
  `create()`, or a `create()` that does not set every declared
  variable) stay `void`/monostate rather than real LPC's own auto-
  zeroed default for a declared type (`int` -> `0`, `string` -> `0`
  read as falsy, etc). Reading one before any assignment and then using
  it in an arithmetic/string context throws instead of silently acting
  like `0`. Not yet hit by anything on this driver's confirmed real
  path (every object variable reached so far is set in `create()`
  first), but worth fixing before this stops being true.
