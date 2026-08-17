# amlp

A C++20 LPC game driver: lexer, parser, bytecode VM, and a TCP server for running an LPC mudlib.

## Build and run

```bash
cmake -B build -S .
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/amlp etc/driver.cfg
```

Connect with `nc localhost 1129`. Enter a name, then try `look`, `north`, `south`, `say <text>`, `boot <name>`.

`etc/driver.cfg` points at `test/mudlib_stub/`, a minimal mudlib used only to exercise the driver. `mudlib/` is Lil, bundled for driver tooling (the `eval` command), not a game mudlib.

## Status

`ROADMAP.md` tracks phases and tasks. `STATUS.md` is the dated development log. Both override anything below if they disagree.

## Language features

Full control flow (if/while/do-while/for/foreach/switch/ternary), arrays and mappings with range indexing, object variables, closures, catch/throw, inheritance with `::`, `call_other()`/`->`, `clone_object()`, `destruct()`, `input_to()`, `add_action()`/`enable_commands()`, and a real `call_out()`/`heart_beat()` scheduler.

## Efuns

Count grows most sessions. Run `grep -c 'registerEfun(' src/efun/EfunTable.cpp` for the current number. Target is ~300. See `ROADMAP.md` row 0.13 and `src/efun/instruct.md`.

## Applies

Full recognized set is in `src/apply/ApplyTable.cpp`. Currently fired: `create()`, `init()`, `connect()`, `logon()`, `process_input()`, `net_dead()`, `compile_object()`, `heart_beat()`, `id()`. Recognized but not yet fired: `clean_up`, `receive_message`, `catch_tell`, `valid_read`, `valid_write`, `valid_socket`, `get_root_uid`, `epilog`, `flag`, `short`, `long`.

## Layout

Source by subsystem under `src/<name>/` and `include/amlp/<name>/`, each its own CMake library. Tests in `test/test_lexer.cpp` (plain `assert()`, not gtest, see `test/instruct.md`). `src/dialect/`, `src/persist/`, `src/jit/`, `src/lsp/`, `src/gc/`, `src/security/`, `src/proto/` are placeholders for later phases, not wired into the build.
