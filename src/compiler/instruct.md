# src/compiler/ - Lexer, Parser, CodeGen, AST

## What lives here

| File | Role |
|------|------|
| `Lexer.cpp` + `include/.../Lexer.hpp` | Hand-written tokenizer. Produces `Token` stream for Parser. |
| `Parser.cpp` + `include/.../Parser.hpp` | Recursive-descent parser. Consumes Token stream, emits `AstPtr` tree. |
| `CodeGen.cpp` + `include/.../CodeGen.hpp` | AST → bytecode `CompiledProgram`. Emits `OpCode` sequences into `FunctionEntry` byte vectors. |
| `include/.../Ast.hpp` | All AST node types (`BinaryExpr`, `CallExpr`, `InlineLambdaExpr`, …). |

The compiler is invoked by `ObjectManager::compile()` for every `.c` file the
driver loads. It runs the system `cpp` preprocessor first (with FluffOS-
compatible predefined macros), then tokenizes, parses, and generates bytecode
in one pass.

## Files to read before touching this directory

- `include/amlp/compiler/Ast.hpp` - full AST node catalogue
- `include/amlp/compiler/Lexer.hpp`
- `include/amlp/vm/Bytecode.hpp` - the opcodes CodeGen emits
- `ROADMAP.md` Phase 0 and Phase 1 rows that touch `src/compiler`
- `reference/fluffos-2.9-ds2.08/grammar.y` - the reference grammar
- `reference/fluffos-2.9-ds2.08/lex.c` - the reference lexer

## Phase 0 tasks

None directly in the compiler. The stub/gap work is in `src/efun` and
`src/vm`. However:
- Verify that `#include` preprocessing handles all macros the mudlib uses
  before Phase 1 begins.

## Phase 1 tasks - Dialect-aware frontend

**This is the biggest compiler change in the roadmap. Do not start until
Phase 0 is complete and the test suite is green.**

### 1. Make the Lexer dialect-aware

The `Lexer` must accept a `LpcDialect` value (from `src/dialect/`) and
change its tokenization rules accordingly.

**FluffOS/MudOS (current default)**
- `(: … :)` closure literals - already working
- `(*fp)(args)` dereference-call syntax - already working
- No changes needed for the default dialect

**LDMud additions**
- `#'name` - tokenize as a new `Token::LambdaRef` carrying the name after `#'`
- `#'efun_name` - same, but for efun references
- The `lambda` keyword (not a kKeyword today)
- `unbound_lambda` keyword
- `bind` keyword (not a statement; a function, but needs recognition)

**DGD additions**
- `nil` keyword (not `0`; maps to a new `Value::Nil` variant - see `src/vm`)
- `atomic` function modifier keyword
- `rlimits` statement keyword
- `parse_string` - handled as an efun, no lexer change needed

Concretely: add a `LpcDialect dialect_` member to `Lexer`; extend
`lexIdentOrKeyword()` to push `#'` into a `LambdaRef` token and to recognize
the new keywords when the matching dialect is active.

### 2. Make the Parser dialect-aware

`Parser` already takes a `Lexer`; add a `LpcDialect` parameter.

**LDMud additions**
- Parse `#'name` tokens into a new `LambdaRefExpr` AST node that CodeGen
  turns into a `MakeClosure` instruction with `FP_LDMUD_SYMBOL` kind.
- Parse `lambda(({ params }), body_array)` into a `LambdaExpr` AST node
  (the body is an LPC array literal used as code - see `src/vm/instruct.md`
  for how the VM executes it).
- Parse `unbound_lambda(({ params }), body_array)` similarly.
- Parse `replaces` as an optional qualifier on `inherit "path";`.
- Mapping width: `([ k:v1, v2, v3 ])` syntax where values per key > 1.

**DGD additions**
- Parse `atomic` as a function-declaration modifier (analogous to `nomask`).
- Parse `rlimits(ticks : stack) { body }` as a `RlimitsStmt` AST node.
- Parse `nil` as an `NilLiteral` AST node (not `IntLiteral{0}`).

### 3. Make CodeGen dialect-aware

- `LambdaRefExpr` → `MakeClosure` opcode with `FP_LDMUD_SYMBOL` closure kind
- `LambdaExpr` / `UnboundLambdaExpr` → synthesize a `FunctionEntry` from the
  body array and emit `MakeClosure` pointing at it (see `src/vm` for
  `Closure::Kind::LambdaArray`)
- `RlimitsStmt` → new `PushRlimits` / `PopRlimits` opcodes (see `src/vm`)
- `atomic` modifier → mark `FunctionEntry::isAtomic = true` (see `src/vm`)
- `NilLiteral` → `PushNil` opcode

## Phase 2 tasks

### async/await (Phase 2.6)

- Add `async` as a function-declaration modifier (like `nomask`/`varargs`).
- Add `await expr` as a statement/expression.
- CodeGen: `await` emits a new `Suspend` opcode that the VM/scheduler handles
  as a coroutine yield point.

## Testing

All compiler tests currently live in `test/test_lexer.cpp`. New tests should
be added to that file or a new `test_parser.cpp` / `test_codegen.cpp`.

```bash
ctest --test-dir build -R "lexer|parser|codegen" --output-on-failure
```

## Key invariants

- The compiler must never crash on malformed input - always throw
  `LpcRuntimeError` (from `src/core`) with a clear source/line/column message.
- Adding a new keyword must not break any existing mudlib file that uses that
  word as a plain identifier. Check first. The comment in `Lexer.cpp` about
  `"array"` is the canonical example of this hazard.
- CodeGen changes must be accompanied by a corresponding `OpCode` addition in
  `Bytecode.hpp` AND a `VM::run()` case for it AND a regression test.
