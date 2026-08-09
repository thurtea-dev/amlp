# STATUS

Current driver status: 2026-08-09

## Verified

- Build: `cmake -B build -S . && cmake --build build`
- Tests: `ctest --test-dir build` passes, 353 tests, no regressions.
- Live verification uses port 1129 and the minimal `driver/mudlib_stub/`.
  The stub mudlib supports login, two rooms, movement, item cloning, chat,
  command dispatch, `net_dead()`, and targeted messaging.
- A real Nightmare3 account has completed login, account creation, chargen,
  starting-equipment grants, and entry to a real room. Explicit `look`,
  delayed `call_out()`, NPC `heart_beat()`, and cross-connection `message()`
  have also been verified live.

## Implemented

- Core LPC values and execution: ints, floats, strings, arrays, mappings,
  objects, closures, arithmetic and logical operators, ternary, casts,
  indexing/ranges, assignment and compound assignment, loops, `switch`,
  `foreach`, short-circuit evaluation, `catch()`/`throw()`, and function
  prototypes/modifiers.
- Inheritance and calls: multi-level `inherit`, cycle detection, flattened
  variables, parent-qualified calls, local/inherited/simul-efun/core-efun
  resolution, `call_other()`/`->`, `efun::name()`, virtual objects through
  `master()->compile_object()`, `clone_object()`, and `new()`.
- Runtime and efuns: file I/O, save/restore (including real FluffOS save
  files), `input_to()`, `receive()`, `master()`, `previous_object()`, type
  predicates, object lookup, interactive/user queries, `destruct()`,
  `add_action()`/`enable_commands()`, `move_object()`, `init()`, `message()`,
  `tell_object()`, `call_out()`, `remove_call_out()`, and heartbeat scheduling.
- Connection lifecycle: `logon()`, targeted connection lookup, sticky
  `userp()`/`query_once_interactive()` semantics, correct `interactive()`
  semantics, real `net_dead()` on link death, and closing the destructed
  object's own connection.
- Safety and compatibility: destructed-object guards at call/apply entry
  points, runtime-error isolation for object load/connect/input dispatch,
  real FluffOS predefined macros, path normalization, and LPC zero defaults.
- Recent language fixes include `status`, modifier-only declarations, bare
  blocks, from-end indexing (`<N`), indexed compound assignment, `do-while`,
  extended `sprintf()`, object-bound/string closures, function-pointer calls,
  and major multi-inherit variable/function-resolution bugs.

## Intentional limitations

- Array `&` keeps left-side order and duplicates; array `|` is not supported.
- Indexed `++`/`--`, indexed assignments used as expressions, and indexed
  compound assignments may evaluate target/index expressions twice.
- `replace_string()` supports only its three-argument form. `implode()` only
  supports a string separator. `map()`/`filter()` support only the closure and
  string-plus-object forms currently used by this mudlib.
- `sprintf()` and `sscanf()` support only their implemented subsets; unsupported
  forms throw. `set_eval_limit()` is accepted but remains a no-op.
- `to_int(buffer)` is unavailable because the value model has no buffer type.
- `restore_object()` reads real FluffOS files, but LPC class/struct values are
  unsupported. `save_object()` continues to write this driver's own format.
- Virtual-object fallback is wired through `loadObject()` only; clone and
  inherit paths do not yet cover every FluffOS virtual-object case.
- `message()` routes targets, but does not apply message-type or exclude
  filtering. `set_living_name()` has no dedicated living-name table.
- DNS lookup is not performed: `query_ip_name()` returns the numeric address.
- Full FluffOS stale-object semantics and destructed-inventory relocation are
  not replicated; objects are guarded at call/apply boundaries and unlinked
  from their old environment.
- Reconnect/take-over save-flag behavior remains paused and is mudlib-specific.
