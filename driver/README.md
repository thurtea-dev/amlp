# lpcdriver

A minimal, from-scratch LPC driver (LPMud-style game driver) written in
modern C++20. It compiles a small subset of the LPC language to bytecode
and executes it on a stack-based VM, with a TCP front end so a mudlib can
be driven interactively over `telnet`/`nc`.

This is a work-in-progress bring-up. Each development slice added one
capability on top of a previously verified one; nothing below is
aspirational -- everything listed as "supported" has been built, unit
tested, and exercised over a live TCP connection.

## Quick start

```bash
cmake -B build -S .
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/lpcdriver config/driver.cfg
```

Then, in another terminal:

```bash
nc localhost 3000
```

Type any line and press enter to see it echoed back. Type `quit` to see
the driver take the alternate branch.

## Bring-up order (slices completed so far)

1. **Hello-world compile/execute path** -- lexer, parser, codegen, and VM
   skeleton; a `create()` function that calls `write()`.
2. **`call_other()` and the `->` operator** -- cross-object calls, plus
   `clone_object()` so objects can be instantiated from source files.
3. **Networking** -- a non-blocking TCP server; `master->connect()` is
   invoked per new connection and its return value (an object) is bound
   to that connection; input lines are dispatched to `receive_message()`.
4. **Variables, parameters, assignment** -- local variable declarations,
   typed function parameters, the `=` operator, and using a variable as
   an expression (so `write(msg)` echoes real client input).
5. **Control flow** -- `if` / `else`, `while`, and the comparison
   operators `== != < <= > >=`, backed by `Jump` / `JumpIfFalse` VM
   opcodes with codegen backpatching. The mudlib demonstrates this with
   an `if (msg == "quit") { ... } else { ... }` branch in
   `receive_message()`.

## Supported LPC language features

| Feature | Status |
|---|---|
| Function declarations (typed return, typed params) | Supported |
| Local variable declarations | Supported (no globals yet) |
| Assignment (`=`) | Supported |
| Variable-as-expression | Supported |
| String literals | Supported (with `\n`, `\t`, `\"`, `\\` escapes) |
| Integer literals | Supported (pushed directly via `PushInt`) |
| Comparison operators `== != < <= > >=` | Supported, on ints/floats and strings (`==`/`!=` only for strings) |
| `if` / `else` | Supported (block or single-statement branches) |
| `while` | Supported |
| Function calls (efuns) | Supported |
| `call_other()` | Supported |
| `->` operator (call_other sugar) | Supported |
| Arrays, mappings | **Not implemented** |
| Global (object) variables | **Not implemented** |
| `for`, `switch`, `break`, `continue` | **Not implemented** |
| Short-circuit `&&` / `||` | **Not implemented** |
| Floats as literals (float *values* exist in `Value`, no float literal syntax yet) | **Not implemented** |

## Supported efuns

| Efun | Behavior |
|---|---|
| `write(string)` | Sends text to the connection currently bound to the calling context (or stdout if none) |
| `call_other(object, string, ...)` | Dispatches a function call to another object |
| `clone_object(string filename)` | Compiles (or reuses cached bytecode for) a source file and instantiates a fresh object, running its `create()` |
| `this_object()` | Stubbed -- always returns void this slice |

## Driver applies (mudlib hooks)

| Apply | Behavior |
|---|---|
| `create()` | Called once per object load/clone |
| `connect()` (on the master object) | Called per new TCP connection; must return an object, which is bound to that connection |
| `receive_message(string)` | Called on the connection's bound object for every newline-terminated line of input |

`ApplyTable::isKnownApply()` also recognizes (but the driver does not yet
call) `init`, `clean_up`, `heart_beat`, `logon`, `disconnect`, `id`,
`short`, `long`, `catch_tell`, `compile_object`, `valid_read`,
`valid_write`, `valid_socket`, `get_root_uid`, `epilog`, `flag` --
these are reserved apply names for future slices.

## What's stubbed / not yet real

- **Heartbeats**: `LpcObject::hasHeartbeat()` / `setHeartbeat()` exist,
  and `Scheduler::tickHeartbeats()` runs every poll loop, but it is
  currently a no-op. No object's `heart_beat()` is ever called yet.
- **Call-outs**: `Scheduler::addCallOut()` / `removeCallOut()` and the
  `CallOutEntry` struct exist, but `tickCallOuts()` is a no-op. There is
  no `call_out()` efun yet.
- **Disconnect on quit**: typing `quit` prints "Goodbye." but the
  connection is not closed by the driver; scope for this slice was
  control flow only, not connection lifecycle changes.
- **Arrays / mappings**: `Value` and `Array`/`Mapping` structs exist at
  the VM level, but there is no array/mapping literal syntax, indexing,
  or the `Index`/`IndexAssign`/`MakeArray`/`MakeMapping` opcodes are
  declared but unimplemented in the VM.
- **Object variables (globals)**: `LpcObject::variables()` exists as a
  storage slot but nothing in the compiler ever declares or reads/writes
  it yet.
- **`this_object()`**: registered but always returns void.

## Project layout

See the file tree below. Source is organized by subsystem
(`core`, `config`, `compiler`, `vm`, `object`, `efun`, `apply`, `net`,
`scheduler`), each built as its own static library and linked into the
`lpcdriver` executable. Unit tests live in `tests/` and cover the lexer,
parser, and codegen for every feature listed as "Supported" above.
