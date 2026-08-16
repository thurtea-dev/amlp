# lpcdriver

A custom C++20 LPC driver (LPMud-style game driver): a hand-written lexer,
recursive-descent parser, bytecode codegen, and stack-based VM, with a
non-blocking TCP front end so a mudlib can be driven interactively over
`telnet`/`nc`. It targets FluffOS-dialect LPC (the dialect this repo's own
mudlib, `mudlib/nightmare3_fluffos_v2/`, is written in).

This snapshot is current as of 2026-08-17. For anything that changes more
often than this file gets rewritten, prefer these over trusting a number
here: `docs/ROADMAP.md` (per-row completion checkboxes), `docs/STATUS.md`
(dated session log, the actual source of truth for what is done), and
`driver/src/<module>/instruct.md` (per-subsystem task detail, but see the
note below before trusting its framing).

Root `CLAUDE.md`'s "Driver orientation" section explains why `instruct.md`
files and `driver/tests/instruct.md` in particular should not be read at
face value; read that first if this is your first time in this directory.

## Two ways to build and run

Both are real, both are independently maintained, and neither is the
"correct" one to prefer over the other. Pick based on what you're doing:
exercising the driver itself against a small, fast, hand-written mudlib
(sandbox), or running it against this repo's real game content (real
mudlib).

### Sandbox: `driver/mudlib_stub`, built from inside `driver/`

The small hand-written mudlib under `driver/mudlib_stub/` (login flow, one
player class, two rooms, a handful of commands) exists purely to exercise
the driver end to end without needing the real mudlib's own size or
dependencies.

```bash
cd driver
cmake -B build -S .
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/lpcdriver config/driver.cfg
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
disconnect handling. `driver/config/driver.cfg` points `mudlib_root` at
`./mudlib_stub` and listens on port 1129.

### Real mudlib, built from the repo root

The same binary, pointed at this repo's actual game content
(`mudlib/nightmare3_fluffos_v2/lib/`) instead of the sandbox.

```bash
cmake -B driver/build -S driver
cmake --build driver/build -j4
ctest --test-dir driver/build --output-on-failure
driver/build/lpcdriver etc/driver.cfg
```

`etc/driver.cfg` (repo root) points `mudlib_root` at the real mudlib and
listens on port 1122. Booting the real mudlib's own master object needs a
local `mudlib/nightmare3_fluffos_v2/lib/secure/cfg/groups.cfg`, which is
deliberately gitignored (only `groups.cfg.example` is checked in) since a
real one carries real admin/wizard account data; create your own from the
example file before expecting a full boot.

Both paths build the identical `lpcdriver` binary and share the same test
suite; only the config file and working directory differ.

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

See `driver/src/compiler/instruct.md` and `driver/src/vm/instruct.md` for
what is deliberately still out of scope (DGD/LDMud dialect syntax, `async`/
`await`, and the rest of the Phase 1-3 work in `docs/ROADMAP.md`).

## Efuns

153 registered as of this writing (grep `registerEfun(` in
`driver/src/efun/EfunTable.cpp` for the exact current count; it grows most
sessions). Target is parity with FluffOS's own ~300 for the core efun set,
with growth tracked in `docs/ROADMAP.md` row 0.13 and the corrected Tier 1
priority list in `driver/src/efun/instruct.md`.

## Scheduler: real, not a stub

`driver/src/scheduler/Scheduler.cpp` runs a real `call_out()` and
`heart_beat()` scheduler: pending call-outs fire on their own delay (by
handle or by name), heartbeat-enabled objects get `heart_beat()` called on
the configured interval, and a runtime error in one fired callback never
stops the rest from running. See `driver/src/scheduler/instruct.md`.

## Driver applies (mudlib hooks)

The full recognized-apply set lives in `driver/src/apply/ApplyTable.cpp`'s
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
territory, `docs/ROADMAP.md` row 3.1), not silently-broken hooks.

## Known stubs / scope limitations

Kept in exactly one place so it cannot drift out of sync with itself:
`docs/STATUS.md`'s own "Known stubs / scope limitations" section is the
current, maintained list. Do not duplicate it here.

## Project layout

Source is organized by subsystem (`core`, `config`, `compiler`, `vm`,
`object`, `efun`, `apply`, `net`, `scheduler`), each built as its own CMake
static library and linked into the `lpcdriver` executable. Headers live
under `include/lpcdriver/<subsystem>/`, sources under `src/<subsystem>/`.
Unit tests live in `tests/test_lexer.cpp`, a single hand-rolled file using
bare `assert()`, not gtest (see `driver/tests/instruct.md` and root
`CLAUDE.md` before trusting anything else that file's own text says about
testing conventions).

`src/dialect/`, `src/persist/`, `src/jit/`, `src/lsp/`, `src/gc/`,
`src/security/`, and `src/proto/` are Phase 1-3 subsystems from
`docs/ROADMAP.md`: each has an `instruct.md` describing the planned work,
but none has any source yet, and none is wired into `CMakeLists.txt`.

See `docs/ROADMAP.md` for the full phased plan with per-task status
checkboxes, and `prompt.md` at the repo root for ready-to-run task prompts
citing specific rows.
