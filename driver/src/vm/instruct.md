# src/vm/ — VM Interpreter, Value Type, Bytecode

## What lives here

| File | Role |
|------|------|
| `VM.cpp` + `include/.../VM.hpp` | Stack-based bytecode interpreter. `VM::run()` is the main eval loop. |
| `Value.cpp` + `include/.../Value.hpp` | `Value` variant type: int/float/string/array/mapping/object/closure. Also `LpcThrownValue`. |
| `include/.../Bytecode.hpp` | `OpCode` enum + `FunctionEntry` + `CompiledProgram`. |

## Files to read before touching this directory

- `include/lpcdriver/vm/Value.hpp` — the full `ValueVariant` definition
- `include/lpcdriver/vm/Bytecode.hpp` — every OpCode and CompiledProgram
- `include/lpcdriver/vm/VM.hpp` — public VM API
- `docs/ROADMAP.md` Phase 0 and Phase 1 rows
- Reference: `fluffos-2.9-ds2.08/interpret.c` — the reference VM (the inline
  comments throughout `VM.cpp` cite specific line/function names from it)

## Phase 0 tasks

### 0.4 — Real `set_eval_limit` accumulated-cost model

Currently `maxEvalCost_` is checked as a flat per-`run()` ceiling and
`set_eval_limit` is a no-op.

**What to build:**
- Add a `int64_t evalCost_` field to VM that accumulates across all nested
  `run()` / `callFunction()` / `callClosure()` calls within a single player
  command dispatch.
- Reset it to 0 at the top of `Server::dispatchLine()` before the call into
  the VM (the real `process_user_command()` reset point).
- `set_eval_limit(n)` sets `config_.maxEvalCost_` for the rest of the current
  command execution, matching real FluffOS's own `set_eval_limit()` semantics.
- Throw a descriptive `LpcRuntimeError` when the limit is hit, matching the
  real "Too long eval" error string.

**Reference:** `fluffos-2.9-ds2.08/interpret.c` — `eval_cost` global and the
`MAX_COST` check in `eval_instruction()`.

## Phase 1 tasks

### 1.7 — LDMud `lambda()` / `unbound_lambda()` / `bind()` closure kinds

**New `Closure::Kind` enum values** (add to `Value.hpp`):
- `Kind::MudosStyle` — current `(: :)` closures (rename from the implicit
  default)
- `Kind::LambdaArray` — LDMud `lambda()`: body is a `std::shared_ptr<Array>`
  that the VM executes as LPC bytecode (see below)
- `Kind::UnboundLambda` — like `LambdaArray` but has no bound `owner`; must be
  `bind()`-ed before calling
- `Kind::LdmudSymbol` — `#'name` reference: name is baked at construction; kind
  is one of `FP_EFUN`, `FP_LOCAL`, `FP_SIMUL` — resolve and cache on first call

**`VM::callClosure()` extensions:**
- `LambdaArray`: extract the `Array` body, interpret each element as either a
  literal value or a sub-instruction array (this is LDMud's own "arrays-as-code"
  format; see LDMud source `closure.c`).
- `UnboundLambda`: throw "unbound lambda called without bind()" if the owner
  is unset.
- `LdmudSymbol`: resolve name on first call (same tiered lookup as `Call`
  opcode), cache the resolved `FP_*` kind + index in the `Closure` struct to
  avoid re-resolving on subsequent calls.

### 1.10 — DGD `nil` as a distinct type

Add `struct Nil {}` to the `ValueVariant` in `Value.hpp`:
```cpp
using ValueVariant = std::variant<
    std::monostate,   // void (uninitialized)
    Nil,              // DGD nil — absence of value, distinct from 0
    int64_t,
    double,
    std::string,
    ...
>;
```

**Semantic rules:**
- `nil == nil` → true
- `nil == 0` → false (unlike `monostate`, which coerces to 0 in FluffOS mode)
- `isTruthy(nil)` → false
- Arithmetic on `nil` → throw type error
- Only active when `LpcDialect::DGD` is set; in FluffOS/LDMud modes `nil`
  does not exist and the `Nil` variant is never constructed.

### 1.11 — DGD `rlimits` statement

New opcodes in `Bytecode.hpp`:
- `PushRlimits` — operands: ticks (int32), stack_depth (int32)
- `PopRlimits` — restore the previous limits

In `VM::run()`:
- `PushRlimits`: push the current `(evalCost_, maxStackDepth_)` pair onto a
  per-VM `rlimitsStack_`; set the new limits.
- `PopRlimits`: pop the saved pair and restore.
- Throw "eval cost limit exceeded" / "stack too deep" when either limit is hit.

### 1.12 — DGD `atomic` function modifier: checkpoint/rollback

**What to build:**
1. Add `bool isAtomic` flag to `FunctionEntry` in `Bytecode.hpp`.
2. In `VM::callFunction()`, before running an `isAtomic` function, snapshot
   the current `variables()` of every object currently on the call stack
   (the "write set"). Use a lightweight copy: `vector<pair<LpcObject*,
   vector<Value>>>`.
3. Wrap the body in a try/catch. On any `LpcRuntimeError`:
   - Restore all snapshotted `variables()` from the checkpoint.
   - Flush any buffered output queued during the atomic call.
   - Re-throw the error to the nearest enclosing `catch()` frame, as DGD does.
4. On success: discard the snapshot, commit buffered output normally.

**Reference:** DGD source `src/call_out.c`, `src/interpret.c` atomic notes;
chattheatre.github.io/lpc-doc/dgd/unusual.html.

## Phase 2 tasks

### 2.5 + 2.6 — C++20 coroutine scheduler + LPC `async`/`await`

New `OpCode::Suspend` — when the VM hits this opcode it:
1. Saves the full stack frame (locals, program counter, call-stack) into a
   heap-allocated `TaskFrame`.
2. Returns a sentinel `Value::Suspended` to the caller.
3. The `Scheduler` receives the `TaskFrame` and re-enqueues it after the
   `await`-ed delay.
4. On resume, `VM::resumeTask(TaskFrame&)` restores the frame and continues.

Use C++20 stackless coroutines (`co_await`) inside the C++ scheduler to model
the suspend/resume cleanly without OS threads.

### 2.10 — Closure bake-at-construction

Instead of storing just `functionName` (a string to re-resolve at every call),
resolve to a `FP_*` kind + index at `MakeClosure` opcode time:
- `FP_LOCAL` — function found in the current object's own program
- `FP_INHERITED` — found in an inherited program (store program index)
- `FP_SIMUL` — found in the simul_efun object
- `FP_EFUN` — found in the efun table (store efun table index)

Cache result in `Closure::resolvedKind` + `Closure::resolvedIndex` (add these
fields). `callClosure()` fast-paths on a non-zero `resolvedKind` rather than
re-running the tiered name walk.

## Testing

The VM test suite is currently spread across `tests/test_lexer.cpp` and inline
integration tests through `Server::dispatchLine`. Add a dedicated
`tests/test_vm.cpp` that exercises:
- Every `OpCode` directly (compile a tiny program, run it, check result)
- `throw()` / `catch()` interaction
- `rlimits` limit enforcement
- `atomic` rollback on error

```bash
ctest --test-dir driver/build -R "vm" --output-on-failure
```

## Key invariants

- The `Value` variant must never grow a new alternative without updating
  `isTruthy()`, `valuesEqual()`, and every `switch`/`visit` pattern in the VM.
- `Nil` variant is only constructed when `LpcDialect::DGD` is active.
- The four-tier call resolution in `findFunctionInChain()` must not be bypassed
  by any new opcode without a documented reason.
- `LpcThrownValue` is a subclass of `LpcRuntimeError` — every catch site that
  handles `LpcRuntimeError` must decide whether to re-throw or consume a
  `LpcThrownValue`. See the comment in `Value.hpp`.
