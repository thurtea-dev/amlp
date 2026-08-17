# amlp

A custom C++20 LPC driver (LPMud-style game driver): a hand-written lexer,
recursive-descent parser, bytecode codegen, and stack-based VM, with a
non-blocking TCP front end so a mudlib can be driven interactively over
`telnet`/`nc`. It targets FluffOS-dialect LPC. `mudlib/` in this repo is
Lil, a small LPC-family scripting language bundled for driver-side tooling
(the `eval` command and similar); it is not a bundled game mudlib. The
sandbox mudlib the driver actually boots and plays against for its own
testing is `test/mudlib_stub/`, described below.

This snapshot is current as of 2026-08-17. For anything that changes more
often than this file gets rewritten, prefer these over trusting a number
here: `ROADMAP.md` (per-row completion checkboxes), `STATUS.md`
(dated session log, the actual source of truth for what is done), and
`src/<module>/instruct.md` (per-subsystem task detail, but see the
note below before trusting its framing).

Root `CLAUDE.md`'s "Driver orientation" section explains why `instruct.md`
files and `test/instruct.md` in particular should not be read at
face value; read that first if this is your first time in this directory.

## How to build and run

The small hand-written sandbox mudlib under `test/mudlib_stub/` (login
flow, one player class, two rooms, a handful of commands) exists purely to
exercise the driver end to end without needing a full production mudlib.
It is the only mudlib this repo boots and plays against; there is no
second, larger game mudlib bundled here.

```bash
cmake -B build -S .
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/amlp etc/driver.cfg
```

Then, in another terminal:

```bash
nc localhost 1129
```

Type a name at the prompt, then try `look`, `north`, `south`, `say
<anything>`, and `boot <name>` (kicks another connected player, if one is
in the room). Disconnecting (Ctrl-C, or closing the terminal) fires the
real `net_dead()` apply on your player object, which announces the
link-death to anyone else in the room, exactly like a real mudlib's
disconnect handling. `etc/driver.cfg` points `mudlib_root` at
`./test/mudlib_stub` and listens on port 1129.

## Supported LPC language features

Full control flow: `if`/`else`, `while`, `do`/`while`, `for`, `foreach`
(including the two-variable mapping form), `switch`/`case`/`default`,
`break`, `continue`, ternary (`?:`), short-circuit `&&`/`||`. Arrays and
mappings, with literal syntax, indexing (`arr[i]`, `map[k]`), range
indexing (`arr[a..b]`), and compound/indexed assignment. Object (global)
variables. String, int, float, and object value types. Closures
(`(: name, bound_args... :)` and the `(*fp)(args)` dereference-call form).
`catch`/`throw`. Inheritance, including `::`-qualified parent calls.
`call_other()` and its `->` sugar. `clone_object()`, `destruct()`,
`input_to()`, `add_action()`/`enable_commands()` command dispatch. A real
`call_out()`/`heart_beat()` scheduler (see below), not a stub.

See `src/compiler/instruct.md` and `src/vm/instruct.md` for
what is deliberately still out of scope (DGD/LDMud dialect syntax, `async`/
`await`, and the rest of the Phase 1-3 work in `ROADMAP.md`).

## Efuns

153 registered as of this writing (grep `registerEfun(` in
`src/efun/EfunTable.cpp` for the exact current count; it grows most
sessions). Target is parity with FluffOS's own ~300 for the core efun set,
with growth tracked in `ROADMAP.md` row 0.13 and the corrected Tier 1
priority list in `src/efun/instruct.md`.

## Scheduler: real, not a stub

`src/scheduler/Scheduler.cpp` runs a real `call_out()` and
`heart_beat()` scheduler: pending call-outs fire on their own delay (by
handle or by name), heartbeat-enabled objects get `heart_beat()` called on
the configured interval, and a runtime error in one fired callback never
stops the rest from running. See `src/scheduler/instruct.md`.

## Driver applies (mudlib hooks)

The full recognized-apply set lives in `src/apply/ApplyTable.cpp`'s
`known()`. As of this writing, the driver actually calls:

| Apply | Fired by |
|---|---|
| `create()` | Every object load/clone |
| `init()` | `move_object()`, both legs (destination's own actions offered to the mover, and vice versa for already-present command-enabled occupants) |
| `connect()` (master) | Every new TCP connection; must return an object, bound to that connection |
| `logon()` | Once, right after `connect()` binds the connection |
| `process_input(string)` | Every input line, when nothing is currently claiming it via `input_to()` |
| `net_dead()` | Link death (EOF/read error), before the connection is torn down |
| `compile_object()` (master) | A virtual-object compile fallback, `ObjectManager::loadObject()` only |
| `heart_beat()` | Every heartbeat-enabled object, each scheduler tick |
| `id(string)` | Object-identification lookups (`present()`-style matching) |

Recognized but not yet called by any driver code path: `clean_up`,
`receive_message` (superseded by `process_input` above; kept as a
recognized name, not fired), `catch_tell`, `valid_read`, `valid_write`,
`valid_socket`, `get_root_uid`, `epilog`, `flag`, `short`, `long`. These
are reserved names for future work (several are Phase 3 security-model
territory, `ROADMAP.md` row 3.1), not silently-broken hooks.

## Known stubs / scope limitations

Kept in exactly one place so it cannot drift out of sync with itself:
`STATUS.md`'s own "Known stubs / scope limitations" section is the
current, maintained list. Do not duplicate it here.

## Project layout

Source is organized by subsystem (`core`, `config`, `compiler`, `vm`,
`object`, `efun`, `apply`, `net`, `scheduler`), each built as its own CMake
static library and linked into the `amlp` executable. Headers live
under `include/amlp/<subsystem>/`, sources under `src/<subsystem>/`.
Unit tests live in `test/test_lexer.cpp`, a single hand-rolled file using
bare `assert()`, not gtest (see `test/instruct.md` and root
`CLAUDE.md` before trusting anything else that file's own text says about
testing conventions).

`src/dialect/`, `src/persist/`, `src/jit/`, `src/lsp/`, `src/gc/`,
`src/security/`, and `src/proto/` are Phase 1-3 subsystems from
`ROADMAP.md`: each has an `instruct.md` describing the planned work,
but none has any source yet, and none is wired into `CMakeLists.txt`.

See `ROADMAP.md` for the full phased plan with per-task status
checkboxes, and `prompt.md` at the repo root for ready-to-run task prompts
citing specific rows.
