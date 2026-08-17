# AetherMUD - Claude Code Prompts

Ready-to-paste prompts for continuing the world-class LPC driver development.
Copy the block for the phase/task you want and paste it directly into Claude Code.

---

## How to use these prompts

1. Open the repository in Claude Code.
2. Copy the prompt block for the task you want.
3. Paste it as your first message.
4. Claude Code will read the referenced files, then implement, build, and test.

**Always run the full test suite before and after each prompt:**
```bash
cmake -B driver/build -S driver && cmake --build driver/build
ctest --test-dir driver/build --output-on-failure
```

---

## Phase 0 - Stabilize the current base

### P0-A: `sscanf` full format set

```
Read docs/ROADMAP.md (Phase 0 row 0.2) and driver/src/efun/instruct.md
(Phase 0 task 0.2). Then read driver/src/efun/EfunTable.cpp and find
the sscanf implementation. The current gaps are: %f (float), %x (hex
int), %(regexp) capture, and adjacent %s%... without a literal between.

Implement each missing format specifier one at a time, with a regression
test per specifier added to driver/tests/test_lexer.cpp. Reference the
real implementation in driver/reference/fluffos-2.9-ds2.08/
efuns_main.c f_sscanf() and sscanf_regexp(). Build and confirm all
existing tests still pass after each specifier is added.
```

### P0-B: FluffOS `.o` save format

```
Read docs/ROADMAP.md (Phase 0 row 0.7) and driver/src/efun/instruct.md
(Phase 0 task 0.7). Then read driver/src/efun/EfunTable.cpp and find
the save_object/restore_object implementation. The current format is
a custom binary format incompatible with FluffOS .o files.

Implement the FluffOS .o text format:
- One line per variable: varname value\n
- Integers: bare decimal
- Strings: double-quoted with \ escapes
- Arrays: ({item,item,...})
- Mappings: ([key:value,...])
- Nested structures: recursive

Add a save_format config key (default "custom") to Config. When set to
"fluffos", save_object uses the new serializer and restore_object can
read both formats (detect by first byte). Add 5+ regression tests
covering all value kinds including nested arrays and mappings. Build and
run full test suite.
```

### P0-C: Full telnet IAC negotiation

```
Read docs/ROADMAP.md (Phase 0 row 0.8) and driver/src/net/instruct.md
(Phase 0 task 0.8). Then read driver/src/net/Connection.cpp and
driver/src/net/Connection.hpp.

Implement:
1. IAC parser in Connection::pollLines() - strip all \xFF sequences
   from the raw read buffer before splitting into lines; handle the
   three-byte IAC WILL/WONT/DO/DONT sequences
2. Echo suppression - when input_to() is called with the I_NOECHO flag
   (flag value 1), send IAC WILL ECHO; when the input_to completes,
   send IAC WONT ECHO
3. NAWS (option 31) - when client sends IAC DO NAWS respond
   IAC WILL NAWS; parse the IAC SB NAWS subneg; store terminalWidth_
   and terminalHeight_ on Connection; expose query_screen_width() /
   query_screen_height() efuns

Reference: RFC 854 (Telnet), RFC 857 (Echo), RFC 1073 (NAWS),
and fluffos-2.9-ds2.08/comm.c telnet_neg(). Add tests using socketpair()
in driver/tests/test_net.cpp. Build and run full test suite.
```

### P0-D: `map`/`filter`/`sort_array` as closure consumers

```
Read docs/ROADMAP.md (Phase 0 row 0.9) and driver/src/efun/instruct.md
(Phase 0 task 0.9). Then read driver/src/efun/EfunTable.cpp and find
the existing map_array/filter_array/sort_array implementations.

Verify each one:
- accepts both a string function name and a Closure value as the callback
- calls VM::callClosure() for closure args
- calls VM::callFunction() on the current object for string args
- handles errors in the callback without crashing the whole efun
- registers both the short and long spellings (map/map_array,
  filter/filter_array)

Add regression tests for: closure callback, string name callback, error
in callback is propagated, empty array input, single-element array.
Reference: fluffos-2.9-ds2.08/array.c f_map_array(), f_filter_array(),
f_sort_array(). Build and run full test suite.
```

### P0-E: Grow efun table to FluffOS parity (~300)

```
Read docs/ROADMAP.md (Phase 0 row 0.13) and driver/src/efun/instruct.md
(Tier 1 list in Phase 0 task 0.13).

Audit the current driver/src/efun/EfunTable.cpp against the reference
list in fluffos-2.9-ds2.08/func_spec.c. Produce a list of the 20 most-
called efuns that are not yet implemented, ordered by real usage count
across the mudlib (grep for call sites in mudlib/nightmare3_fluffos_v2/).

Implement the top 10 from that list in one PR:
- Each efun must match the FluffOS reference spec in func_spec.c
- Each efun must have at least one regression test
- No existing tests may be broken

Build and run full test suite.
```

### P0-F: PCRE regexp efuns

```
Read docs/ROADMAP.md (Phase 0 row 0.11) and driver/src/efun/instruct.md
(Phase 0 task 0.11).

Add PCRE2 as a dependency (pkg_check_modules(PCRE2 REQUIRED libpcre2-8)
in driver/CMakeLists.txt). Implement:
- regexp(string str, string pattern) -> int (1 if matches, 0 if not)
- regexplode(string str, string pattern) -> string array
- reg_assoc(string str, string* patterns, mixed* tokens, mixed default)
  -> mixed array

Reference: fluffos-2.9-ds2.08/regexp.c and the PCRE2 API
(pcre2_match, pcre2_compile). Add tests for: basic match, no match,
capture groups in regexplode, reg_assoc tokenization. Build and run
full test suite.
```

---

## Phase 1 - Dialect universality

### P1-A: Dialect scaffold (start here first)

```
Read docs/ROADMAP.md Phase 1 section and driver/src/dialect/instruct.md.
Also read driver/src/config/instruct.md (Phase 1 tasks), driver/src/apply/
instruct.md (Phase 1 task 1.4), and driver/src/compiler/instruct.md
(Phase 1 tasks).

Create the dialect foundation:
1. Create driver/src/dialect/ as a new CMake library
2. Implement LpcDialect.hpp with the enum and dialectFromString()
3. Add dialect config key to Config (default "fluffos")
4. Implement FluffOsBootApi, LdmudBootApi, DgdBootApi, DialectFactory
5. Wire DialectFactory into main.cpp after Config::loadFromFile()
6. Add dialect to CMakeLists.txt

Do NOT yet implement dialect-specific lexer/parser/VM changes - only
the enum, config, and boot API abstraction. Add tests in
driver/tests/test_dialect_fluffos.cpp confirming that the FluffOS boot
API returns the correct apply names. Build and run full test suite.
```

### P1-B: LDMud `#'symbol` and `lambda()` closure kinds

```
Read docs/ROADMAP.md Phase 1 rows 1.7 and 1.8, driver/src/vm/instruct.md
(Phase 1 tasks 1.7 and 1.8), and driver/src/compiler/instruct.md
(Phase 1 tasks 1a and 1b for LDMud).

Prerequisite: Phase 1-A must be merged first.

Implement (LDMud dialect mode only, guard with LpcDialect::LdMud checks):
1. Lexer: recognize #'name tokens as LambdaRef
2. Parser: parse #'name into LambdaRefExpr AST node
3. Parser: parse lambda(({params}), body) into LambdaExpr AST node
4. CodeGen: LambdaRefExpr -> MakeClosure with FP_LDMUD_SYMBOL kind
5. Value.hpp: add Closure::Kind enum with MudosStyle and LdmudSymbol values
6. VM::callClosure(): handle LdmudSymbol by resolving on first call
   and caching the result

Add tests in driver/tests/test_dialect_ldmud.cpp. Build and run
full test suite (all 374+ existing tests must still pass).
```

### P1-C: DGD `nil` type and `atomic` functions

```
Read docs/ROADMAP.md Phase 1 rows 1.10 and 1.12, driver/src/vm/instruct.md
(Phase 1 tasks 1.10 and 1.12), and driver/src/compiler/instruct.md
(Phase 1 tasks for DGD).

Prerequisite: Phase 1-A must be merged first.

Implement (DGD dialect mode only):
1. Add Nil struct to ValueVariant in Value.hpp
2. Update isTruthy(), valuesEqual() for Nil
3. Lexer: recognize nil as a keyword in DGD mode
4. Parser: parse nil as NilLiteral AST node
5. CodeGen: NilLiteral -> PushNil opcode
6. VM::run(): implement PushNil opcode
7. atomic modifier: FunctionEntry::isAtomic flag; VM::callFunction()
   checkpoints and rolls back on error for atomic functions

Add tests in driver/tests/test_dialect_dgd.cpp covering: nil != 0,
nil coercion rules, atomic function rollback on error. Build and run
full test suite.
```

### P1-D: DGD driver+auto object boot path

```
Read docs/ROADMAP.md Phase 1 row 1.15, driver/src/apply/instruct.md
(Phase 1 task 1.15), and driver/src/dialect/instruct.md.

Prerequisite: Phase 1-A and Phase 1-C must be merged.

Implement:
1. DgdBootApi fully wired: driver object callbacks, auto object path
2. ObjectManager::compile() auto-inject: when DgdBootApi::hasAutoObject(),
   prepend inherit "/kernel/auto"; to every compiled file except
   the auto object itself
3. Boot sequence: load driver object, call initialize(), not master+simul

Add config keys: auto_object (default /kernel/auto),
driver_object (default /kernel/driver). Add tests confirming that
in DGD mode every loaded object gets the auto inherit injected.
Build and run full test suite.
```

---

## Phase 2 - Architecture differentiation

### P2-A: Apply cache

```
Read docs/ROADMAP.md Phase 2 row 2.9 and driver/src/apply/instruct.md
(Phase 2 task 2.9).

Implement an apply cache on VM:
1. Add ApplyCache: unordered_map<pair<LpcObject*, string>, FunctionEntry*>
2. In callFunction()'s findFunctionInChain() walk: check cache first on
   hit; populate on miss
3. ObjectManager::loadObject() recompile: invalidate cache entries for
   the recompiled object and all objects inheriting from it
4. ObjectManager::destructObject(): invalidate all entries for the object

Add a benchmark test: create a hot-loop of 10,000 callFunction() calls
to the same function and verify measurable speedup vs. uncached.
Build and run full test suite.
```

### P2-B: World statedump

```
Read docs/ROADMAP.md Phase 2 row 2.1 and driver/src/persist/instruct.md.

Create driver/src/persist/ as a new CMake library. Implement:
1. StateSerializer::dumpState() using CBOR via nlohmann/json::to_cbor()
   - Serialize all live objects (filename + variable values by name)
   - Serialize all pending call_outs (target, function, dueAt offset, args)
   - Serialize all heartbeat registrations
2. StateSerializer::restoreState() - full round-trip restore
3. dump_state() efun (master-only)
4. Config::statedumpFile() key and statedumpInterval() periodic trigger
   in main.cpp's run loop

Add tests in driver/tests/test_persist.cpp for full round-trip of all
Value kinds including nested arrays and mappings. Build and run full
test suite.
```

### P2-C: LSP server

```
Read docs/ROADMAP.md Phase 2 row 2.19 and driver/src/lsp/instruct.md.

Create driver/src/lsp/ as a new CMake library. Implement the first two
LSP capabilities:
1. JSON-RPC framing (Content-Length header, stdin/stdout transport)
2. initialize request/response
3. textDocument/publishDiagnostics - compile in-memory, collect errors
4. textDocument/completion - efun names + local function names

Wire --lsp flag in main.cpp: when present, start LspServer::run()
instead of the game server.

Add tests in driver/tests/test_lsp.cpp: send didOpen with a syntax
error, assert correct diagnostic published. Build and run full
test suite.
```

### P2-D: LLVM JIT backend

```
Read docs/ROADMAP.md Phase 2 row 2.11 and driver/src/jit/instruct.md.

Prerequisites: Phase 0 and Phase 1 complete; interpreter 100% correct
for all three dialects.

Create driver/src/jit/ as an optional CMake library
(option LPCDRIVER_ENABLE_JIT). Implement the first-pass JIT targeting
pure arithmetic functions only (no Call/CallEfun initially):
1. JitCompiler class with hit counting and threshold
2. BytecodeToIr.cpp: translate PushInt/PushFloat/PushLocal/StoreLocal/
   arithmetic opcodes/JumpIfFalse/Return to LLVM IR
3. Integration in VM::callFunction(): check shouldCompile, run native
   on hit
4. Graceful fallback to interpreter on JIT compile failure

Add tests in driver/tests/test_jit.cpp: enable JIT, run arithmetic
function 200 times, verify identical output to interpreter. Build and
run full test suite with and without LPCDRIVER_ENABLE_JIT.
```

---

## Phase 3 - Production hardening

### P3-A: Security model

```
Read docs/ROADMAP.md Phase 3 row 3.1 and driver/src/security/instruct.md.

Create driver/src/security/ as an optional CMake library
(option LPCDRIVER_ENABLE_SECURITY, default OFF).

Implement SecurityManager with:
1. FluffOsBootApi route: privs_file apply for path and call_other checks
2. allowRead/allowWrite checks in file efuns (read_file, write_file etc.)
3. allowClone check in ObjectManager::cloneObject()
4. allowCallOther check in VM::callFunction() for cross-object calls
5. query_privs() / set_privs() efuns

Add tests in driver/tests/test_security.cpp for access-denied cases.
Build and run full test suite with and without LPCDRIVER_ENABLE_SECURITY.
```

### P3-B: GMCP + MSDP protocols

```
Read docs/ROADMAP.md Phase 3 row 3.4 and driver/src/proto/instruct.md.
Also read driver/src/net/instruct.md Phase 3 task 3.4.

Prerequisites: Phase 0.8 telnet IAC negotiation must be complete.

Create driver/src/proto/ as a new CMake library. Implement:
1. GmcpHandler: IAC SB 201 framing, onReceive(), send()
2. MsdpHandler: IAC SB 69 MSDP_VAR/MSDP_VAL encoding
3. Wire both into Connection: gmcpEnabled_, msdpEnabled_ flags,
   processTelnetOption() routing
4. New efuns: gmcp_send(), msdp_send(), query_gmcp(), query_msdp()

Add tests in driver/tests/test_proto.cpp using socketpair(). Build
and run full test suite.
```

### P3-C: Generational GC (Layer 1 - no-op wrapper)

```
Read docs/ROADMAP.md Phase 3 row 3.3 and driver/src/gc/instruct.md.

This is the first layer of the 5-layer GC migration (Layer 1: no-op
wrapper). DO NOT change any observable behavior.

Create driver/src/gc/ as a new CMake library. Implement:
1. GcObject header struct (Kind, generation, marked, pinned)
2. GcHeap singleton with allocate(), addRoot(), removeRoot(), collect()
   - collect() is a no-op in Layer 1 (just logs what it would collect)
3. Type aliases: GcArray = GcObject wrapping std::vector<Value>
4. All existing shared_ptr<Array> sites still work unchanged

The goal of Layer 1 is zero observable behavior change + the API
existing + tests confirming the API compiles and links. Build and
run full test suite.
```

### P3-D: Conformance test suite

```
Read docs/ROADMAP.md Phase 3 row 3.5 and driver/tests/instruct.md
(Phase 3 tasks).

Create driver/tests/conformance/ directory. Implement a dialect-agnostic
test runner that:
1. Takes a directory of .c test files as input
2. Compiles and runs each one via the driver's ObjectManager + VM
3. Calls run_tests() on the resulting object
4. Reports pass/fail per test function

Create 10 conformance test files covering:
- Basic types (int, float, string, array, mapping, closure)
- Arithmetic operators
- String efuns (explode, implode, sprintf, sscanf)
- Array efuns (map, filter, sort_array, sizeof)
- Object lifecycle (clone_object, destruct, find_object)
- call_out scheduling
- catch/throw
- Closures and function pointers
- Inheritance and ::
- save_object/restore_object round-trip

These tests must pass on FluffOS 2.9 as well (the reference driver).
Build and run full test suite.
```

---

## Stress testing with dead-souls drivers

### STRESS-A: Install and run reference drivers

```
Read docs/stress-test/instruct.md for instructions on downloading and
building the reference LPC drivers from dead-souls.net/files/.

Set up the comparison benchmark:
1. Build AetherMUD driver: cmake -B driver/build -S driver &&
   cmake --build driver/build
2. Download and build FluffOS 2.9, LDMud, and DGD per the instructions
   in docs/stress-test/instruct.md
3. Run the benchmark suite in docs/stress-test/bench/ against all four
   drivers and produce a comparison report

Focus on: boot time, login time, call_out throughput, heartbeat
throughput under load, memory usage after 1 hour of simulation.
```

### STRESS-B: Boot a foreign mudlib on AetherMUD

```
Read docs/stress-test/instruct.md. Download one of the dead-souls
mudlib packages from dead-souls.net/files/ (pick Dead Souls 3.x).

Attempt to boot it on the AetherMUD driver:
1. Point driver.cfg mudlib= at the dead-souls mudlib root
2. Set master_file= to the dead-souls master path
3. Run driver/build/lpcdriver etc/driver.cfg
4. Document every compilation error that appears in the driver log
5. For each error, determine if it is a gap in AetherMUD's efun
   table or a dialect difference

Produce a gap report: docs/stress-test/dead-souls-gap-report.md
listing every unimplemented efun or semantic difference found.
This report drives the next round of Phase 0 efun additions.
```

---

## General maintenance prompts

### UPDATE-STATUS: Sync STATUS.md after any implementation

```
Read driver/STATUS.md and docs/ROADMAP.md. Then read all modified
source files in the most recent git commit. Update driver/STATUS.md
with a dated entry describing exactly what was implemented:
- What was verified against the reference source (cite filename + function)
- What test was added and what it proves
- What remains as a known stub or gap
- Current test count

Also update the checkbox in docs/ROADMAP.md for any completed row.
Commit the updated STATUS.md and ROADMAP.md.
```

### AUDIT-EFUNS: Efun coverage audit

```
Read driver/src/efun/EfunTable.cpp and list every registered efun name.
Read driver/reference/fluffos-2.9-ds2.08/func_spec.c and
list every efun defined there. Produce a gap analysis:

1. Efuns in func_spec.c but not in EfunTable.cpp (missing)
2. Efuns in EfunTable.cpp but not in func_spec.c (driver-specific)
3. Efuns in both - check that the signature matches

For the top 20 missing efuns by usage frequency (grep call sites in
mudlib/nightmare3_fluffos_v2/), produce a priority-ordered backlog.
Write the result to docs/efun-coverage-audit.md.
```

### AUDIT-TESTS: Test coverage audit

```
Read driver/tests/instruct.md and driver/tests/test_lexer.cpp (and
any other test files in driver/tests/). Read driver/src/efun/EfunTable.cpp
and list every registered efun.

For each registered efun, determine whether it has at least one
regression test. Produce a list of efuns with no test coverage.
Then implement one regression test for each untested efun - minimum
happy path + one error/edge case per efun. Build and confirm all
tests pass.
```
