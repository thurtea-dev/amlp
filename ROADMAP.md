# AMLP - World-Class LPC Driver Roadmap

Goal: transform AMLP from a single-mudlib FluffOS-targeting driver into
the best LPC runtime available - a FluffOS/LDMud-level driver, done
better than either, on their own terms and surpassing both on the
dimensions neither addressed. Scope clarified 2026-08-18: DGD is a
comparison-only reference dialect, not a required target for parity or
completion -- real, done-better FluffOS/LDMud compatibility is the actual
goal, not three-way parity across all three. See Phase 1's own header
for what this changes there.

Each source directory carries an `instruct.md` that owns the detailed task
list for that subsystem. This file is the master sequencing reference.

---

## Phase 0 - Stabilize the current base

**Prerequisite for everything else. Do not start Phase 1 until Phase 0 is
complete and the full test suite is passing with no regressions.**

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 0.1 | `throw()` efun - carry a Value to the nearest `catch()` | `src/efun` | [x] |
| 0.2 | `sscanf` full format set: `%f`, `%x`, `%(regexp)`, adjacent `%s` | `src/efun` | [x] (partial: `%(regexp)` still pending; 0.11's PCRE2 wrapper it depended on is now done) |
| 0.3 | `sprintf` `%*` dynamic field width | `src/efun` | [x] |
| 0.4 | `set_eval_limit` as a real accumulated-cost model (not no-op) | `src/vm` | [x] |
| 0.5 | Full `O_DESTRUCTED` apply guards on every cross-object call | `src/object` | [x] |
| 0.6 | Shadow support: `shadow(ob, flag)` efun + shadow chain traversal | `src/object` | [x] |
| 0.7 | `save_object`/`restore_object` in FluffOS `.o` text format | `src/efun` | [x] (partial: restore-side only; `save_object` still writes this driver's own custom format) |
| 0.8 | Full telnet IAC negotiation, echo suppression, NAWS | `src/net` | [x] |
| 0.9 | `map`/`filter`/`sort_array` as real closure consumers | `src/efun` | [x] (partial: all mudlib shapes covered) |
| 0.10 | `socket_*` family basics (create/connect/write/read/close) | `src/net` + `src/efun` | [x] (partial: STREAM/DATAGRAM only, no MUD mode or binary modes; no `socket_read` efun -- real FluffOS has none, reads are callback-only) |
| 0.11 | `regexp`/`regexplode`/`reg_assoc` PCRE efuns | `src/efun` | [x] |
| 0.12 | Every efun has at least one regression test | `tests` | [x] (audited 2026-08-20: all 167 then-registered efuns confirmed covered; a moving target as 0.13 grows the table, each new efun needs its own test at the time it's added) |
| 0.13 | Grow efun table to FluffOS parity (~300 efuns) | `src/efun` | [x] (non-`parse_*` scope closed 2026-08-24: 240 registered against this exact vendored reference build's own 270 real `efun_defs.c` names. Final accounting of the real gap (`comm -23` reconciliation, not the raw registered-count difference -- see STATUS.md's own reconciliation): 40 non-`parse_*` names, all confirmed genuine documented exclusions (architecture mismatch or zero-call-site deferral, individually re-verified 2026-08-24), with no further batch work remaining against that portion, plus the separately-tracked 8-name `parse_*` package. `origin()` and `reload_object()` both implemented 2026-08-23; `functions`/`variables`/`fetch_variable`/`store_variable` implemented 2026-08-24, correcting an earlier "no implementation exists" miscall for all four (real bodies in `packages/contrib.c`); `socket_release`/`socket_acquire` implemented 2026-08-24 (continued), correcting an earlier "Tier 3, out of basics scope" miscall. The `parse_*` package is carved out as its own row, 0.13a) |
| 0.13a | `parse_*` natural-language sentence/grammar-rule parser package (`parse_init`, `parse_refresh`, `parse_sentence`, `parse_add_rule`, `parse_add_synonym`, `parse_my_rules`, `parse_dump`, `parse_remove`) | `src/efun` | [ ] (not started, deliberately deferred. Real implementation source: `packages/parser.c`, 3419 lines -- FluffOS's real "parser" package, confirmed substantial and genuinely implemented in the reference build, not unverifiable or architecture-mismatched. 66 combined call-site hits across the 8 names in the vendored mudlib corpora. Sized well beyond a normal batch item -- comparable to or larger than everything else row 0.13 has implemented combined -- so it warrants its own explicit go-ahead before being taken on, not an assumed default. See STATUS.md's 2026-08-23/2026-08-24 entries for the scoping history.) |
| 0.14 | `global include file` config support (auto-`#include` prepended to every compiled object) | `src/config` + `src/object` | [x] |
| 0.15 | `ObjectManager::compile()`'s `programCache_` has no invalidation path -- a recompiled file's own new source is silently ignored | `src/object` | [x] |

**0.15 scope note:** found live (2026-08-21) while confirming `mudlib`'s
own `eval` command works end to end. `ObjectManager::compile(filename)`
checks `programCache_` first and returns the cached `CompiledProgram` for
any filename it has ever successfully compiled before, unconditionally,
with no path anywhere that invalidates or replaces that entry -- confirmed
directly by reading the function, not inferred from the symptom alone.
Real, reproducible failure mode: `mudlib/command/eval.c` writes a new
`/tmp_eval_file.c`, destructs the previous `/tmp_eval_file` instance, then
calls `"/tmp_eval_file"->eval()` again -- the destruct correctly clears
the *instance*, but the *compiled program* for that filename is still
sitting in `programCache_` from the first call, so every subsequent `eval`
silently re-runs the first call's own stale bytecode against the new
instance instead of the newly-written source. Confirmed live: three
different real `eval` expressions in one session (`5+5`, an array
literal, `this_player()`) all returned the first call's own cached result.
Same shape hit live again (2026-08-18) verifying `wand_of_creation.c`'s
own `cmd_create()`, which does the identical rm()+destruct()+write_file()
cycle for a reused item name.

**Fixed 2026-08-18.** `ObjectManager::compile()` now re-reads the target
file's raw (pre-preprocessing) source on every call, before trusting a
`programCache_` hit: the file's current bytes are compared against the
exact source text that produced the cached entry (`programSource_`, a new
parallel map keyed the same way as `programCache_`). Identical content
still takes the fast path (no wasted recompile); a genuine difference
falls through and recompiles fresh, replacing both map entries. Deliberately
a byte comparison, not an mtime check: two writes to the same path within
one filesystem timestamp tick are routine (this row's own regression tests
hit it), and would look unchanged to `stat()` despite genuinely different
content. If the source file has vanished entirely since the cached compile
(a purge with nothing written back), the last good compiled program is
still served rather than treated as an error -- matches real semantics,
where an already-compiled program does not stop working just because its
own source file was later deleted. Existing objects already holding the
old `CompiledProgram` (own `shared_ptr`, `LpcObject::program_`) are
unaffected either way: replacing `programCache_`'s own entry never mutates
the old `CompiledProgram` object itself. Scope held to exactly what was
asked: only the recompiled file's own text is checked -- a `#include`d
header changing independently, with the including file's own bytes
untouched, is not detected (nothing currently needs it; the demonstrated
failure mode is always the file itself being rewritten). An inheriting
file already compiled against the old version of something it inherits
also keeps that old version until it is itself recompiled (its own
`inheritedPrograms` entry was resolved and copied at *its* own compile
time, not re-resolved retroactively) -- unchanged from this row's own
prior scoping note, and outside what either real repro (`eval.c`,
`wand_of_creation.c`, both leaf files with no inherits) needed. 4 new
regression tests in `test/test_lexer.cpp`, driving `ObjectManager::
loadObject()`/`cloneObject()` directly (the two real callers of `compile()`)
rather than through either mudlib file: same-path recompile after
destruct+rewrite via `loadObject()`; the same via `cloneObject()` (which
has no `sourceFileExists()` pre-gate, unlike `loadObject()`, so it is the
one that actually reaches `compile()`'s own "source vanished" fallback);
unchanged-source reuse (`programPtr()` identity, confirming the fast path
still avoids wasteful recompiles); and the vanished-source fallback itself
via `cloneObject()`. Verified live against the real running `amlp` binary
(`etc/driver_lil.cfg`), the same way the wand's own live verification was
done: three `eval` calls in one session, all against the same
`/tmp_eval_file` path (`return 5+5`, `return 9999`, `return 5+5` again),
each correctly returning its own fresh result (`10`, `9999`, `10`) instead
of the first call's own stale bytecode.

---

## Phase 1 - Dialect universality

**Goal: a FluffOS/MudOS- and LDMud-level driver, done better than
either -- not three-way parity with DGD.** Scope clarified 2026-08-18:
DGD is a comparison-only reference dialect from here on, not a required
target for Phase 1 completion. It stays genuinely useful for that
purpose -- `temp/dgd/`'s own real source is still worth reading whenever
a FluffOS/LDMud design question benefits from seeing how a third,
architecturally different driver solved the same problem (real DGD's own
`nil`/planes/LWO model already informed rows 1.10-1.14's own research
below) -- but no DGD-only row (`1.11`-`1.15`, and any future one) blocks
Phase 1 completion, and none should be read as carrying the same weight
as an actual FluffOS/LDMud gap. The real research already recorded in
each stays on the record; only the priority changes.

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 1.1 | `LpcDialect` enum + config key `dialect` | `src/config` + `src/dialect` | [x] (the enum -- `LpcDialect`, `dialectName()`/`dialectFromString()` -- implemented 2026-08-24 in `src/dialect/LpcDialect.hpp`/`.cpp`; the `dialect` config key implemented 2026-08-24 (continued) in `Config::dialect()` (`src/config/Config.hpp`/`.cpp`, plain `"fluffos"`/`"ldmud"`/`"dgd"` string, kept out of the `LpcDialect` enum itself to avoid a `config`<->`dialect` library cycle since `src/dialect` already depends on `src/config`; parsed via `dialectFromString()` at the one real call site that needs the enum, `src/dialect/DialectSelect.cpp`). Defaults to `"fluffos"`, matching this driver's prior behavior exactly for every config file that never sets the key. Actually consumed by exactly one call site so far -- `main.cpp`'s `masterUidApply()` boot query, see row 1.4 -- every other applies still hardcodes FluffOS or is unimplemented) |
| 1.2 | Dialect-aware Lexer: `#'`, `'name`, `atomic`, `rlimits`, `nil` tokens | `src/compiler` + `src/dialect` | [ ] (scoped 2026-08-18. `lambda`/`unbound_lambda` removed from this row's real scope entirely, no keyword needed for either; `'name` symbol literals added, previously undocumented. **First slice implemented 2026-08-18 (continued), greenlit from the scoping note's own recommendation:** (1) `Lexer` now takes an `LpcDialect dialect` constructor parameter defaulting to `LpcDialect::FluffOS` -- confirmed zero behavior change for every pre-existing call site (this driver's own ~100 direct test constructions, all still passing with no dialect argument at all); `ObjectManager::compile()` is the one real call site now passing a genuine, config-derived dialect (`dialectFromString(config_.dialect())`, row 1.1's own machinery). (2) DGD's real `atomic` function-declaration modifier (`temp/dgd/src/comp/parser.y`'s own `"ATOMIC { $$ = C_ATOMIC; }"`, confirmed in the scoping note) is now lexed as `TokenType::Keyword` only when `dialect_ == LpcDialect::DGD`; under FluffOS/LDMud it stays a plain `Ident`, matching this file's own "array" precedent for not reserving a word neither real dialect reserves. 2 new regression tests confirm all three dialects directly at the token level. **`nil` implemented 2026-08-18 (continued) as this row's next greenlit slice -- see row 1.10 for the full detail (that row now owns the `Value`-level half of this work).** `#'`, `'name`, `rlimits` all remain unimplemented, exactly as scoped -- none of them touched this pass. **Mapping width investigated 2026-08-18 (continued) and confirmed bigger than this row's own "moderate" classification -- not implemented, see row 1.9 for the full finding; it now owns this literal syntax too, not split across two rows.** **`#'name` bare-name closure literal first slice implemented 2026-08-18 (continued, real Phase 1 commitment, not scoping-only): picked as the highest real-world-compatibility item among Phase 1's three remaining actual blockers (`#'`/`'name` closures, mapping width, connect/disconnect) -- see this session's own STATUS.md entry for the full assessment and reasoning. `#` is not lexable at all today outside real preprocessor directives (confirmed: falls straight to `tokenize()`'s own "unrecognized character" branch, not merely misread as an `Ident` the way `atomic`/`nil` were) -- new `Lexer::lexHashQuote()`, dispatched only when `dialect_ == LpcDialect::LdMud && peekNext() == '\''`, combines `#` + `'` into one two-character `Symbol` token (`"#'"`), matching the existing `"->"`/`"::"`/`"++"` multi-char-symbol precedent rather than a new `TokenType`. Deliberately the bare-name form only -- no operator spellings (`#'+`, `#'==`, ...), no `#'[` index/range forms, no `#'({` aggregate, no `#'efun::`/`#'sefun::`/`#'lfun::`/`#'var::`/`#'Name::` scope prefixes, all of which stay exactly as unimplemented as before. Also fixed the one genuinely architecture-touching prerequisite the row 1.2/1.3 scoping note below had already identified but left unsolved: real system `cpp` hard-errors on any line whose first non-whitespace character is `#` and is not a real directive it recognizes, which a bare `"#'this_player;"` statement on its own line -- a completely ordinary way to write it -- would trigger, taking down the *whole file's* preprocessing regardless of what the Lexer/Parser are taught to recognize. Fixed in `ObjectManager.cpp` (`maskHashQuote()`/`unmaskHashQuote()`): every `#'` occurrence in the raw source is masked to a plain-identifier marker text before staging for `cpp`, then unmasked back in `runPreprocessor()`'s own output -- runs unconditionally for every file regardless of dialect (nothing dialect-specific about the fix itself), so a file with no `#'` anywhere is untouched either way. See row 1.3's own entry for the Parser-side half and why it needed zero new CodeGen/VM work.**) |
| 1.3 | Dialect-aware Parser: LDMud `#'`/`'name` literals, mapping width, DGD `atomic`/`rlimits`/`nil` | `src/compiler` + `src/dialect` | [ ] (scoped 2026-08-18, same pass as row 1.2. See the scoping note below the table for the full breakdown, the corrected mapping-width and `rlimits` grammars, and the proposed smallest first slice. **First slice implemented 2026-08-18 (continued):** `Parser` now takes the same `LpcDialect dialect` constructor parameter, same default/zero-behavior-change shape as `Lexer` (confirmed against this driver's own ~80 direct test constructions, all still passing unmodified). `atomic` is recognized as a real function-declaration modifier -- reusing `parseDeclPrefix()`'s existing generic modifier-consumption loop verbatim, no new AST node kind -- gated independently in `Parser::isModifierKeyword()` on `dialect_ == LpcDialect::DGD` (belt and suspenders alongside the Lexer's own gate, not solely relying on it), and excluded from `isTypeKeyword()`'s default-true classification so it can never be misread as a declared type. Confirmed end to end: `atomic void go() { return; } ` compiles and the function is genuinely callable under `dialect: dgd`, while the identical source fails to compile under both `dialect: fluffos` (default) and `dialect: ldmud` -- "atomic" is misread as an object-variable name in both, a real parse error, not a silent misparse, caught by `ObjectManager::compile()`'s own existing try/catch. What `atomic` *means* once accepted (VM-level checkpoint/rollback) stays row 1.12's own, separate, still-unstarted concern -- this slice only lets the keyword parse. **`nil` implemented 2026-08-18 (continued), same double-gating discipline -- `NilLiteral` recognized in `parsePrimary()`, independently gated on `dialect_ == LpcDialect::DGD` there too, not solely trusting the Lexer's own gate; see row 1.10 for the full citation trail and the `Value`-level half of this work.** `#'`, `'name`, `rlimits`, and DGD's own `&ident(args)` closure syntax all remain untouched, exactly as scoped. **Mapping width investigated 2026-08-18 (continued), confirmed bigger than this row's own "moderate" classification -- not implemented, see row 1.9.** **`#'name` bare-name closure literal first slice implemented 2026-08-18 (continued) -- same real Phase 1 commitment as row 1.2's own entry, full reasoning there and in this session's own STATUS.md entry. `parsePrimary()` recognizes the Lexer's own combined `"#'"` token (double-gated on `dialect_ == LpcDialect::LdMud` here too, the same belt-and-suspenders discipline `"atomic"`/`"nil"` already established -- the Lexer only ever emits `"#'"` under LdMud, but this is the one place that actually decides "is this a real closure literal", not something that should silently trust the Lexer's own gate to hold forever), expects an `Ident` for the function name, and builds a **`ClosureLiteralExpr`** -- the exact same AST node `"(: name :)"` already builds a few lines below it, with `boundArgs` left empty (real LDMud semantics for this bare form: any arguments are supplied by whoever later calls the closure, not baked in at the literal site, identical in spirit to a bound-arg-less `"(: name :)"`). Reusing that node rather than inventing a new one meant this slice needed **zero** CodeGen or VM changes past the Parser check itself -- `CodeGen.cpp`'s existing `PushClosure` emission and every downstream consumer (`funcall`/`evaluate`, `map`/`filter`/`sort_array` callbacks) already handle any `ClosureLiteralExpr` generically by function name, with no FluffOS-vs-LDMud distinction anywhere in that path. 2 new regression tests (`testLexerHashQuoteClosureOnlyRecognizedUnderLdmudDialect`, `testCompileHashQuoteClosureAcceptedOnlyUnderLdmudDialectAndEvaluatesCorrectly` -- the latter also directly exercises the row 1.2 cpp-masking fix, with a bare `"#'lower_case;"` statement on its own line, the exact shape that fix was needed for). Confirmed end to end against the real running driver, not just the test suite: three separate `eval` expressions under a real `dialect: ldmud` config, all against the real bundled `mudlib/` -- `funcall(#'lower_case, "HELLO WORLD")` returns `"hello world"`, `mixed f = #'upper_case; return funcall(f, "hi there")` returns `"HI THERE"` (storing the closure in a variable before calling it), and `capitalize(funcall(#'lower_case, "YES"))` returns `"Yes"` (composed with another real efun). Still deliberately unimplemented, matching row 1.2's own scoping: operator spellings, index/range forms, the aggregate-array closure, and every scope prefix.**) |
| 1.4 | Pluggable boot API: FluffOS master/simul_efun, LDMud master, DGD driver+auto | `src/apply` + `src/dialect` | [ ] (partial: `BootApi` abstract base plus `FluffOsBootApi`/`LdmudBootApi` implemented 2026-08-24 in `src/dialect`, deliberately trimmed to omit `connectApply()`/`netDeadApply()` and `DgdBootApi` per the still-open design note on this row -- see `src/dialect/instruct.md`. `masterUidApply()` alone is now wired for real: `main.cpp` calls the new `queryMasterUid(vm, bootApi)` helper (`src/dialect/MasterUidBoot.cpp`) right after master loads, routed through `VM::applyMaster()` against the real master object, 2026-08-24 (continued). `main.cpp`'s `BootApi` is now genuinely config-driven, 2026-08-24 (continued further) -- `makeBootApiForConfig()` (`src/dialect/DialectSelect.cpp`) selects `FluffOsBootApi`/`LdmudBootApi` from `Config::dialect()` (row 1.1), defaulting to FluffOS for any config that never sets the key; DGD throws `NotImplementedError` rather than silently picking a wrong dialect. Deliberately a small free function, not a `DialectFactory` class -- see `DialectSelect.hpp`'s own comment on why a factory is premature with only one real consumer. No `DialectFactory`, and every other `ApplyTable`/`VM::applyMaster()` call site (`Server.cpp`'s `"connect"`, `ObjectManager.cpp`'s `"compile_object"`/`"privs_file"`) still hardcodes its apply name string directly rather than routing through `BootApi` -- only the one UID call site was in scope) |
| 1.5 | LDMud shadows: `shadow(ob)`, `unshadow()`, `query_allow_shadow` | `src/efun` | [x] (rescoped 2026-08-17: the row's own original title carried the same signature error `src/dialect/instruct.md`/`src/apply/instruct.md` had already flagged elsewhere -- `shadow(ob,1)` is FluffOS's shape, not LDMud's. Re-read the real sources directly: `temp/ldmud/src/func_spec` ("int shadow(object) no_lightweight;", one argument) + `temp/ldmud/src/simulate.c`'s own `f_shadow()`/`validate_shadowing()`/`f_unshadow()` (LDMud 3.6.8, this project's vendored clone). Confirmed divergences from the already-implemented FluffOS `shadow(ob, flag)` (row 0.6): one argument only (no query flag -- LDMud's own separate query mechanism, `query_shadowing()`, is **obsolete** in this exact 3.6.8 clone, relocated to `temp/ldmud/doc/obsolete/query_shadowing`, so the row's own "`query_shadowing`" target does not apply to a current LDMud and is correctly dropped, not implemented, for the dialect); returns int 1/0, never the shadowed object; master apply is `query_allow_shadow()`, not `valid_shadow()`; and -- a genuine driver-level asymmetry confirmed by reading both `validate_shadowing()`s side by side -- LDMud's has no "cannot shadow the master object" guard at all (FluffOS's does, already ported under 0.6), master protection under LDMud being purely an advisory mudlib convention inside `query_allow_shadow()`'s own body, not a driver mechanism. Also confirmed real LDMud has a second, genuinely FluffOS-absent efun, `void unshadow(void)` (`temp/ldmud/src/simulate.c`'s `f_unshadow()`; grepped the full vendored `fluffos-2.9-ds2.08` tree for "unshadow", zero hits) -- it only splices current_object out when current_object is itself shadowing something (reconnecting that victim to whoever was shadowing current_object), a genuine no-op when current_object is merely being shadowed by someone else with no victim of its own, confirmed from the C directly rather than the doc's looser prose. The row's third original item, "shadow-chain `call_other`", turned out to already be done and dialect-agnostic -- `VM::callFunction()`'s existing shadow-chain walk (implemented under row 0.6, see `src/object/instruct.md`) matches both dialects' documented call_other-redirection model (`doc/efun/shadow`'s own "if A shadows B... not even object B can call_other() itself" is already exercised by the existing `!= current_object` re-entry guard), so nothing further needed there. What was actually small and unambiguous: the `shadow()`/`unshadow()` efun-level signature and semantics work, implemented 2026-08-17 in `src/efun/EfunTable.cpp`, gated on `vm.config().dialect() == "ldmud"` exactly like row 1.6's `replace_program()` branch, with 4 new regression tests. Directory column corrected from `src/object` to `src/efun`, the only place any of this actually lives) |
| 1.6 | LDMud `replace_program()` no-argument, sole-inherit auto-select | `src/efun` | [x] (rescoped 2026-08-17: the row's original premise -- a `replaces` directive in `inherit` -- **does not exist anywhere in real LDMud**. Re-grepped `temp/ldmud/src/prolang.y`'s own `inheritance_qualifier`/`inheritance_modifier` productions: the complete inherit-modifier set is `static`/`private`/`public`/`protected`/`nosave`/`nomask`/`deprecated`/`virtual`/`visible`, no `replaces` token anywhere in the grammar (`temp/ldmud/src` grep for the bare word `replaces` also turns up nothing but unrelated English prose in comments -- `array.c`, `ed.c`, `lex.c`, `object.c`, `efuns.c`, `swap.c`). The real, genuine per-dialect divergence lives entirely inside `replace_program()` itself, confirmed by reading both real implementations in full: `temp/ldmud/doc/efun/replace_program` + `temp/ldmud/src/object.c`'s `v_replace_program()` (LDMud 3.2.9+) accept `void replace_program()` with **no** argument, auto-selecting the object's sole inherited program when it has exactly one, throwing "requires argument for object with more than one inherit" when it has more, and "called with no inherited program" when it has none; real FluffOS's `temp/reference/fluffos-2.9-ds2.08/replace_program.c` own `f_replace_program()` has no such form at all -- it unconditionally rejects a missing/non-string arg via `bad_arg(1, ...)` before anything else runs, mandatory argument always. `replace_program(string)` itself was already implemented FluffOS-scoped under row 0.13 (`src/efun/EfunTable.cpp`, `f_replace_program`-equivalent); this row's actual, narrower scope is the LDMud-only zero-arg form. Small and unambiguous once rescoped -- implemented 2026-08-17 in the same `replace_program` efun registration, gated on `vm.config().dialect() == "ldmud"` (row 1.1's `Config::dialect()`, already reachable from `src/efun` with no new library dependency), with 3 new regression tests: sole-inherit auto-select applies and swaps correctly, more-than-one-inherit throws, and the fluffos-dialect zero-arg call is confirmed still rejected exactly as before this fix, unchanged. Directory column corrected from `src/compiler` + `src/object` to `src/efun`, the only place any of this actually lives) |
| 1.7 | LDMud `lambda()` / `unbound_lambda()` / `bind_lambda()` closure kinds | `src/vm` + `src/compiler` | [ ] (name corrected 2026-08-17: `bind()` does not exist as an LDMud efun -- the real name is `bind_lambda(closure cl [, object ob])`, confirmed against `temp/ldmud/doc/efun/bind_lambda` and `temp/ldmud/src/closure.c`'s own `v_bind_lambda()` (3.6.8). Investigated in real depth this pass, alongside row 1.5's shadow work, per the same "shadow()/bind_lambda() divergence" note in `src/dialect/instruct.md`/`src/apply/instruct.md` -- **not implemented, scope confirmed genuinely bigger than a normal batch item, same category as the still-open `parse_*` and connect/disconnect design questions.** Real `v_bind_lambda()` switches on the closure's own kind (`sp->x.closure_type`): `CLOSURE_LAMBDA`/`CLOSURE_IDENTIFIER` are unconditionally unbindable; `CLOSURE_LFUN` rebinds in place; `CLOSURE_BOUND_LAMBDA` either rebinds in place or, if the closure is shared (`ref > 1`), copy-on-write clones it first; `CLOSURE_UNBOUND_LAMBDA` allocates a new bound-lambda wrapper; the default branch (efun/simul-efun/operator closures) rebinds directly. This driver's own `Closure` (`include/amlp/vm/Value.hpp`) has none of these kinds -- one flat struct with a single `owner` field, no `CLOSURE_LAMBDA`/`UNBOUND_LAMBDA`/`BOUND_LAMBDA` distinction and no reference-counted sharing to copy-on-write around -- because `lambda()`/`unbound_lambda()`/`#'symbol` (this same row's other two items) are themselves entirely unimplemented; there is nothing yet for a real `bind_lambda()` to bind. Real `bind_lambda()` also gates a non-`this_object()` target through `privilege_violation("bind_lambda", this_object(), ob)`, a whole master-apply subsystem (`doc/concepts/privilege`) this driver has no equivalent of at all -- a different gate family from the UID-based FluffOS applies already implemented. A narrowed "just rebind this driver's existing FluffOS-style `(: name :)` closures' own `owner` field, skip the privilege check, skip the lambda-specific branches" version was considered and rejected as a mismatch: it would answer to the name `bind_lambda()` while implementing none of the closure-kind matrix that makes the real efun what it is, and the missing privilege check is a real permissiveness gap, not a harmless simplification like the shadow work's already-documented `nomask` skip. Options for whoever picks this up: (a) treat this as its own explicitly-scoped go-ahead item the way `parse_*` (row 0.13a) was carved out, sized to real closure-kind work (`lambda()`/`unbound_lambda()` literal support in `src/compiler`+`src/vm` first, `bind_lambda()` as a natural follow-on once real bound/unbound lambda values exist to rebind); (b) implement a minimal, explicitly-labeled `bind_lambda()` now that only covers this driver's existing FluffOS-style closures (owner-reassignment, dialect-gated, no privilege check), clearly documented as a partial stand-in until (a) lands; (c) leave it deferred exactly as before, now with the real scope on record instead of a guess. **Decision (2026-08-17): (c).** No partial stand-in. `lambda()`/`unbound_lambda()` themselves are entirely unimplemented prerequisites -- there is no real lambda value yet for any `bind_lambda()`, minimal or otherwise, to rebind, so a stand-in would only be exercising this driver's pre-existing FluffOS-style closures under a name that means something structurally different in real LDMud. Stays deferred alongside `parse_*` (row 0.13a) and the connect/disconnect design question (row 1.4/1.16) until row 1.7's own prerequisite closure-kind work gets an explicit go-ahead) |
| 1.8 | LDMud `#'symbol` references baked at construction | `src/compiler` + `src/vm` | [ ] |
| 1.9 | LDMud mapping width > 1: `m_allocate`, `m_indices`, `m_values` | `src/vm` + `src/efun` + `src/compiler` | [ ] (investigated 2026-08-18 as row 1.2/1.3's next candidate slice after `atomic`/`nil`, same discipline -- **confirmed genuinely bigger than the "moderate, self-contained" classification the scoping pass gave the literal grammar alone, not implemented, stopped and reported rather than forced.** The grammar itself was re-confirmed exactly as the scoping pass already had it: `temp/ldmud/src/prolang.y`'s own `m_expr_list2`/`m_expr_values` (same-key values separated by `;`, entries by `,`, width-consistency enforced -- "Inconsistent number of values in mapping literal") plus the separate `([: width_expr ])` empty-mapping-of-given-width literal (`'(' '[' ':' expr0 ']' ')'`, `F_M_ALLOCATE`) -- confirmed LDMud-only by also reading DGD's own mapping grammar this time (`temp/dgd/src/comp/parser.y`'s `assoc_exp: exp ':' exp`, a strict single key-value pair, no width concept, no semicolon anywhere), so this is correctly scoped to `dialect: ldmud` only, not DGD. What makes it bigger: real LDMud mappings are genuinely N-columns-wide at the *value* level, not just at the literal-syntax level -- confirmed via `temp/ldmud/src/func_spec`'s own real declarations: `m_allocate(int, int default: F_CONST1)` (second arg is width), `m_values(mapping, int default: F_CONST0)` (second arg selects *which* column), plus `m_entry`, `m_reallocate`, `m_add` (variadic), `m_contains` (variadic) -- a full efun family, none of which exist in this driver at all (confirmed by grep; this driver's own `allocate_mapping` is FluffOS's differently-shaped capacity-hint efun, unrelated). This driver's `Mapping` (`include/amlp/vm/Value.hpp`) is `std::vector<std::pair<Value, Value>>` -- one value per key, no width dimension whatsoever -- and that shape is load-bearing throughout: `MakeMapping`/`Index`/`IndexAssign` opcodes, `sizeof`/`map_delete`, the `mapping + mapping` union-merge already implemented in `VM.cpp`'s `Add` opcode, and `save_object`/`restore_object` serialization all assume exactly one value per key. Giving mapping width real, working semantics -- not just syntax acceptance that silently discards every value past the first, which would fail this pass's own "confirm end to end that it works" bar the same way a `bind_lambda()` stand-in exercising only pre-existing FluffOS-style closures was already rejected as a mismatch for row 1.7 -- requires the same `Mapping`-structure rework this row was always going to need, cascading through every one of those call sites plus the new efun family above. Not separable into a small parser-only slice the way `nil`'s minimal VM coupling turned out to be: `nil` only needed two `Value.cpp` cases and a `PushNil` opcode because every arithmetic/comparison opcode already had a generic type-mismatch fallback to fall into for free; mapping width has no equivalent free lunch, since indexing and iteration over a mapping are exactly the operations that would need to change shape. Recommends: this row absorbs the literal-syntax half of what the row 1.2/1.3 scoping note had filed under Parser scope -- they were always one feature artificially split across two rows -- and stays a single, explicitly-scoped go-ahead item, same category as `parse_*` (row 0.13a) and `valid_read`/`valid_write`, not picked up piecemeal) |
| 1.10 | DGD `nil` as distinct type in `Value` variant | `src/vm` + `src/compiler` | [x] (implemented 2026-08-18 as ROADMAP row 1.2/1.3's own next greenlit slice, same gated-per-dialect pattern as row 1.2/1.3's `atomic`. Read the real DGD source directly before writing anything: `temp/dgd/src/comp/parser.y`'s own `"NIL { $$ = Node::createNil(); }"`, `temp/dgd/src/comp/node.cpp`'s `Node::createNil()`, and `temp/dgd/src/data.h`'s own `T_NIL`/`VAL_TRUE`/`VAL_NIL` macros. One real nuance not previously on record: DGD's own `nil` is only a genuinely distinct runtime value under **strict typechecking** (`temp/dgd/src/data.cpp`'s own `"nil.type = (stricttc) ? T_NIL : T_INT;"`, `temp/dgd/src/comp/node.cpp`'s own `"nil_node = (flag) ? N_NIL : N_INT;"`) -- a global DGD driver config setting (`conf[TYPECHECKING].num == 2`, `temp/dgd/src/config.cpp`), not a per-file pragma. Under any other typechecking level, real DGD's own `nil` literally compiles as, and runtime-behaves as, plain integer 0 -- no distinct representation at all. This implementation targets strict-mode DGD's own behavior, the only mode where `nil` is worth having a row for. New `Nil` (`include/amlp/vm/Value.hpp`, an empty stateless struct, added to `ValueVariant`), gated `nil` keyword (`Lexer::lexIdentOrKeyword()`, same `LpcDialect::DGD`-only double-gating discipline as `atomic`), `NilLiteral` AST node (`Ast.hpp`) and `parsePrimary()` handling (also independently dialect-gated in the Parser, not solely trusting the Lexer's gate), new `PushNil` opcode (`Bytecode.hpp`/`VM.cpp`, no operand, real DGD's own nil being a stateless singleton too). Confirmed the "minimal real piece" genuinely stayed minimal, not bigger than expected: `isTruthy()`/`valuesEqual()` needed one explicit case each (`nil` is falsy, matching real `VAL_TRUE`; `nil == nil` is true, matching real `VAL_NIL`; `nil == 0` is false for free, since `valuesEqual()` already rejects mismatched variant alternatives before either explicit check runs, exactly matching real strict-mode DGD's own distinct `T_NIL`/`T_INT` type tags) -- and arithmetic/comparison on `nil` already throws a clear type error with **zero** additional code, since `asArithmeticOperand()` (`VM.cpp`) never special-cased anything but `int64_t`/`double`/`std::monostate`, so `Add`/`Sub`/`Mul`/`Div`/`Mod`/`Lt`/`Lte`/`Gt`/`Gte`'s own pre-existing generic "unsupported operand types"/"operand is not numeric" fallbacks already cover `Nil` correctly with no VM.cpp changes beyond the new opcode. Confirmed end to end exactly like `atomic`: a real `nil` literal compiles and evaluates correctly under `dialect: dgd` (returns a genuine `Nil`-holding `Value`, is falsy, `nil == nil` is true, `nil == 0` is false), and the identical source fails to compile under both `dialect: fluffos` (default) and `dialect: ldmud` -- not a parse error this time (unlike `atomic`, a bare `nil` identifier is syntactically valid as a variable reference), but a genuine `CodeGen::resolveVariable()` "undeclared variable" failure at codegen time, caught by the same `ObjectManager::compile()` try/catch, exactly the "gets misread as an identifier" alternative outcome this slice's own task anticipated. 2 new regression tests (`testLexerNilKeywordOnlyRecognizedUnderDgdDialect`, `testCompileNilLiteralAcceptedOnlyUnderDgdDialectAndEvaluatesCorrectly` -- the latter exercises four separate scenarios, `getNil`/`nilIsFalsy`/`nilEqualsNil`/`nilEqualsZero`, inside that one registered test, which is where an earlier report of this work miscounted "4 new regression tests" -- corrected 2026-08-18 (continued) after the discrepancy was flagged against the real 614-to-616 delta). Not covered by this pass, deliberately out of "minimal real piece" scope: `nil` interaction with `sprintf`/`save_object`/`restore_object` serialization was not exercised or tested -- untested, not confirmed broken, simply beyond what "compiles and evaluates correctly" required here. `#'`, `'name`, mapping width, and `rlimits` remain untouched, exactly as scoped in the row 1.2/1.3 note below) |
| 1.11 | DGD `rlimits` statement: per-task tick + stack depth limits | `src/vm` + `src/compiler` | [ ] (**comparison-only, not a Phase 1 blocker** -- scope clarified 2026-08-18 (continued): DGD is a reference dialect for comparison, not a required Phase 1 target; see the Phase 1 header above. The research below stays accurate and useful if this is ever picked up, it just does not carry FluffOS/LDMud-gap weight. investigated 2026-08-18 as the last "moderate" candidate from the row 1.2/1.3 list, same discipline as `atomic`/`nil` -- **confirmed genuinely bigger, not implemented, stopped rather than forced, same category as mapping width.** Grammar itself is small and unchanged from the prior finding (`temp/dgd/src/comp/parser.y:566`: `RLIMITS '(' expr ';' expr ')' compound_stmt`). The real runtime semantics are not: `temp/dgd/src/interpret.cpp`'s own `Frame::newRlimits()` -- read in full -- is a *relative*, nested resource-budget system, not a flat ceiling: the stack arg is depth *relative to the current call depth* (0 = inherit parent, negative = unlimited, positive = current+N), the ticks arg is sub-allocated out of the parent's own remaining ticks and refunded on scope exit, and ticks are additionally right-shifted by `level` -- DGD's own "plane" nesting depth, the same mechanism behind `atomic`'s rollback (row 1.12, also unimplemented) -- coupling rlimits directly to another unbuilt subsystem, not just its own. Authorization for both the non-privileged compile-time and runtime paths is not an arithmetic rule at all but two driver-object hooks, `compile_rlimits`/`runtime_rlimits` (`Frame::checkRlimits()`'s own `DGD::callDriver(this, "runtime_rlimits", 3)`), which depend on a driver-object concept this project has zero infrastructure for (`DgdBootApi` does not exist, row 1.15 is 0%). This driver also has no existing call-depth counter at all to build the "stack" half on (unlike `nil`, which got real behavior for free from already-generic VM code, or `atomic`, whose own semantics are cleanly deferred to a separate row). A simplified version that skipped both driver hooks and used a flat push/pop ceiling instead of the real relative/refund/level-scaled model would not be a faithful implementation of what this row is actually for, the same reasoning that ruled out shims for `bind_lambda()` and mapping width. Recommends: stays its own explicitly-scoped item, likely sequenced after `atomic`'s own VM-level work (row 1.12) and some real driver-object plumbing (row 1.15), not picked up in isolation) |
| 1.12 | DGD `atomic` function modifier: VM-level checkpoint/rollback | `src/vm` | [ ] (**comparison-only, not a Phase 1 blocker** -- scope clarified 2026-08-18 (continued): DGD is a reference dialect for comparison, not a required Phase 1 target; see the Phase 1 header above. The research below stays accurate and useful if this is ever picked up, it just does not carry FluffOS/LDMud-gap weight. investigated 2026-08-18 as part of a Phase 1 scan pass for the next smallest actionable item -- **confirmed genuinely bigger than a normal batch item, not implemented.** `src/vm/instruct.md`'s own existing plan (snapshot every on-stack object's `variables()` into a `vector<pair<LpcObject*, vector<Value>>>`, try/catch, restore on error) does not match what real DGD actually does, confirmed by reading `temp/dgd/src/interpret.cpp` directly rather than trusting that plan: real atomic rollback is not a call-stack-local variable snapshot at all, it is one call site (`interpret.cpp:2390-2403`) into DGD's own global "planes" data-versioning subsystem -- `Object::newPlane()` + `new Dataplane(f.data, ++f.level)` on entry, `Dataplane::commit(f.level, &val)` + `Object::commitPlane()` on success (`interpret.cpp:2456-2462`) -- which copy-on-write-versions *the entire object universe* reachable from the call, not just the current call's own local variables, and is the identical mechanism row 1.11's own investigation already found underneath `rlimits`' own `level`-scaled tick accounting (`f.rlim->ticks >>= 1` on atomic entry, confirmed at `interpret.cpp:2396`, `*= 2` on commit at `interpret.cpp:2461` -- the same halving/doubling row 1.11 already flagged). Also touches object lifecycle directly: a destructed object referenced in an atomic call's own arguments is live-wiped from the caller's stack frame mid-unwind (`interpret.cpp:195-197`, `"wipe out objects in arguments to atomic function call"`), and uncaught atomic errors route through a dedicated driver-object callback, `atomic_error` (`Frame::atomicError()`, `interpret.cpp:2962`, `error.h:95,103`'s own `atomicFrame` tracking). This driver has no "planes" concept, no per-object versioning, and no call-depth-scoped `level` counter at all (row 1.11's own already-recorded finding) -- a flat push/pop variable-snapshot stand-in would not be a faithful implementation of what real DGD atomic actually is, the same reasoning that already ruled out shims for `bind_lambda()`/mapping width/a syntax-only `rlimits`. Confirms and sharpens row 1.11's own existing "sequenced after atomic's own VM-level work" framing: they share one real prerequisite, the planes subsystem, neither is smaller in isolation than the other) |
| 1.13 | DGD `parse_string` kfun | `src/efun` | [ ] (**comparison-only, not a Phase 1 blocker** -- scope clarified 2026-08-18 (continued): DGD is a reference dialect for comparison, not a required Phase 1 target; see the Phase 1 header above. The research below stays accurate and useful if this is ever picked up, it just does not carry FluffOS/LDMud-gap weight -- the size comparison to `parse_*` (0.13a) is still a genuine, real observation about this row's own scope, not a claim that it needs the same priority `parse_*` does. investigated 2026-08-18, same scan pass -- **confirmed genuinely bigger than a normal batch item, same category as FluffOS's own `parse_*` package (row 0.13a) in size, not implemented.** `src/compiler/instruct.md`'s own existing note ("handled as an efun, no lexer change needed") undersells it badly -- confirmed by reading the real vendored source directly rather than trusting that note: real DGD's `parse_string()` is not a plain string-processing efun at all, it is backed by an entire dedicated grammar-compiler subsystem, `temp/dgd/src/parser/` -- a DFA lexer generator (`dfa.cpp`, 1634 lines), an LALR-style shift-reduce grammar/parser generator (`grammar.cpp` 1134 lines + `srp.cpp` 1076 lines), and the `parse_string()` glue itself (`parse.cpp`, 1043 lines) -- 5143 lines total (`wc -l`), comparable in size to the already-excluded FluffOS `parse_*` package (3419 lines, `packages/parser.c`). Also has its own persistent per-object state slot, `class Parser *parser; /* parse_string data */` (`temp/dgd/src/data.h:193`, allocated/freed at `data.cpp:903,971`), meaning it is not even stateless across calls the way a normal efun is. None of this driver's own compiler/parser infrastructure (hand-written recursive-descent `Lexer`/`Parser`, `src/compiler`) is the right shape to host a second, LPC-string-driven, runtime-constructed DFA+grammar engine -- this would be new infrastructure, not a call to existing infrastructure the way every other row 1.x efun-level item has been. Same "needs its own explicit go-ahead as a dedicated row" verdict already given to `parse_*` (0.13a) and `valid_read`/`valid_write` (1.16), for the same reason: sized well beyond a normal batch item) |
| 1.14 | DGD lightweight objects (LWOs): value-semantics object kind | `src/vm` + `src/object` | [ ] (**comparison-only, not a Phase 1 blocker** -- scope clarified 2026-08-18 (continued): DGD is a reference dialect for comparison, not a required Phase 1 target; see the Phase 1 header above. The research below stays accurate and useful if this is ever picked up, it just does not carry FluffOS/LDMud-gap weight. investigated 2026-08-18, same scan pass -- **confirmed genuinely bigger than a normal batch item, not implemented.** Read `temp/dgd/src/data.h`/`interpret.h` directly rather than assuming from the row's own title: real DGD's `T_LWOBJECT` (`data.h:64`) is a third fundamental value kind alongside `T_OBJECT` (persistent, by-reference) and the existing indexed types, folded directly into the same `T_INDEXED(t)` range as arrays/mappings (`data.h:79`, `"(t) >= T_ARRAY"` -- an LWO indexes like an array/mapping, not like an object) and reference-counted via its own `array`-shaped storage (`PUT_LWOVAL`/`PUT_LWO` macros, `interpret.h:173-176`). Crucially, it is not merely a new `Value` alternative the way `nil` (row 1.10) was -- real call dispatch itself is dual-shaped around it: `Frame::funcall()`/`Frame::call()` both take a *second*, independent `LWO *lwobj` parameter alongside `Object *obj` (`interpret.h:256-257`), and every `Frame` carries its own `lwobj` field with its own real ref/deref lifecycle (`interpret.h:277`, `f.lwobj->ref()`/`->del()` seen live in `interpret.cpp`'s own call path). There is also a real, one-directional conversion between the two kinds, `Object::upgradeLWO(LWO*, Object*)` (`data.h:165`) -- an LWO can be promoted to a genuine persistent object at runtime, a real semantic bridge, not just cosmetic. This driver's own `LpcObject` model (`include/amlp/object/LpcObject.hpp`) is `shared_ptr`-identity-based throughout -- `VM::callFunction()`'s call dispatch, `Value`'s own object alternative, equality, `call_other`, everywhere an object is handled -- with no parallel value-semantics kind and no second call-dispatch parameter anywhere. Giving LWOs real, working semantics (not just a `Value` variant tag that nothing actually copies-by-value) would mean reworking call dispatch, indexing, and object-identity handling throughout `src/vm`, the same shape of cross-cutting rework mapping width (row 1.9) was already correctly ruled out for on identical grounds -- not separable into a small, isolated slice) |
| 1.15 | DGD driver+auto object boot path | `src/apply` + `src/dialect` | [ ] (**comparison-only, not a Phase 1 blocker** -- scope clarified 2026-08-18 (continued): DGD is a reference dialect for comparison, not a required Phase 1 target; see the Phase 1 header above. The research below stays accurate and useful if this is ever picked up, it just does not carry FluffOS/LDMud-gap weight -- in particular, row 1.4's own still-open connect/disconnect design question is a real FluffOS/LDMud-relevant blocker in its own right and keeps its full weight regardless of this row's own deprioritization. confirmed 2026-08-18, same scan pass, already on record rather than newly found: `src/dialect/instruct.md`'s own `DgdBootApi` section (its "open design note", cross-referenced directly from row 1.4's own status text) explicitly scopes this row together with row 1.4 -- real DGD has no single `"connect"`/`"disconnect"` master apply at all, instead a three-way port-type fork (`telnet_connect`/`binary_connect`/`datagram_connect`) plus `close()` called on the persistent user object itself, a shape `BootApi`'s existing single-string `connectApply()`/`netDeadApply()` abstraction cannot model without redesigning that abstraction for all three dialects at once (`src/dialect/instruct.md` lines ~178-250, own citations to `applied_spec`'s real DGD driver-callback name list already in that file). Same open design question as row 1.4's own still-omitted `DgdBootApi`/row 1.16's `disconnect` -- not a separate, independently smaller item, no new investigation changes that) |
| 1.16 | LDMud master apply name table | `src/apply` + `src/dialect` + `src/efun` | [ ] (partial: `get_master_uid` implemented and tested 2026-08-24 via `LdmudBootApi::masterUidApply()`/`queryMasterUid()` (row 1.4). `query_allow_shadow` implemented 2026-08-17 (row 1.5, `src/efun/EfunTable.cpp`'s `shadow()` LDMud branch). `valid_snoop` implemented 2026-08-17 this same pass, alongside a full re-scope of `snoop()` itself -- `temp/ldmud/src/comm.c`'s own `set_snoop()` ("The function calls master->valid_snoop() to test if the snoop is allowed") calls it for both the start and stop forms, confirmed by reading the C directly rather than `doc/efun/snoop`'s own stale prose, which also wrongly claims an object return (`func_spec`'s own `"int snoop(object, void|object);"` and `v_snoop()`'s own `put_number(sp, i)` are authoritative: plain int, 1/0/-1, never the object). Gated on `vm.config().dialect() == "ldmud"` in the same `snoop` registration, 5 new regression tests. `valid_query_snoop` intentionally not implemented alongside it: the LPC-visible efun it used to gate, `query_snoop()`, is itself obsolete in this exact 3.6.8 clone (`temp/ldmud/doc/obsolete/query_snoop`) -- its real replacement is `interactive_info(ob, II_SNOOP_*)` (confirmed live call sites in `comm.c`'s `f_interactive_info()`), a materially different, much larger efun this driver has no equivalent of at all; implementing `valid_query_snoop` with nothing that calls it would be dead code. **2026-08-18: all three of the row's remaining items investigated against the real 3.6.8 driver source in full, none turned out small -- each has its own blocking issue, recorded below rather than forced.**
- `get_bb_uid`: **dead in this exact vendored build, not implementable against anything real.** `doc/master/get_bb_uid` claims it is called by `process_string()` when there is no current object, but `temp/ldmud/src/efuns.c`'s own `f_process_string()`/`process_value()` -- read in full -- make no euid/`get_bb_uid` call of any kind. Grepped the generated constant `STR_GET_BB_UID` (from `string_spec`'s own `"GET_BB_UID \"get_bb_uid\""` entry) across every `.c`/`.h` in `temp/ldmud/src`: zero call sites anywhere in the driver. The only two places the name appears at all are `string_spec` (the string-table declaration) and `applied_spec` (a compile-time argument/return-type spec for the compiler to type-check a master.c that happens to define the lfun -- confirmed by `applied_spec`'s own header comment, not itself a call-site generator). This is a real doc-vs-code divergence in LDMud itself, the same category already found for `unshadow()`/`snoop()` in rows 1.5/1.16 -- the C is authoritative, the doc is stale. Implementing a `get_bb_uid` accessor would be scaffolding with no real driver-side consumer to model faithfully, not a divergence to port.
- `make_path_absolute`: **blocked on a missing prerequisite.** Grepped `STR_ABS_PATH` (`string_spec`'s `"ABS_PATH \"make_path_absolute\""`) across the whole driver: exactly one real call site, `temp/ldmud/src/ed.c:1128`, inside the built-in line editor's own relative-filename resolution (`ed`'s file-open command, when the typed filename does not start with `/`). This driver has no `ed()` efun at all -- confirmed zero `"ed"` registrations in `EfunTable.cpp`, and already on record in `src/efun/instruct.md` as excluded ("`get_char`/`ed`/`origin`/`resolve`/the driver-internal-dump family", previously-documented architecture-scope reasons). `make_path_absolute()` has no other real caller to model against until `ed()` itself exists -- same shape as `bind_lambda()` needing `lambda()`/`unbound_lambda()` first.
- `valid_read`/`valid_write`: **genuinely bigger than a normal batch item, not LDMud-specific at all.** Read `temp/ldmud/src/simulate.c`'s own `check_valid_path()` in full: one shared function backing both applies (`apply_master(STR_VALID_WRITE/READ, 4)`, args `path, uid-or-0, func, ob` in that push order, confirmed matching `doc/master/valid_read`/`valid_write`'s own SYNOPSIS exactly), called from every file-touching efun -- `doc/master/valid_read`/`valid_write`'s own lists name `copy_file`, `ed_start`, `file_size`, `get_dir`, `print_file`/`cat`, `read_bytes`, `read_file`, `restore_object`, `tail` (read) and `copy_file`, `rename_from`/`rename_to`, `ed_start`, `garbage_collection`, `mkdir`, `memdump`, `objdump`, `opcdump`, `remove_file`/`rm`, `rmdir`, `save_object`, `write_bytes`, `write_file` (write). Checked whether this could be a small LDMud-only branch on an existing FluffOS mechanism the way `shadow()`/`snoop()`/`replace_program()` were: it cannot -- FluffOS has the identical real applies too (`temp/reference/fluffos-2.9-ds2.08/applies.h`'s own `APPLY_VALID_READ`(33)/`APPLY_VALID_WRITE`(38)), and this driver currently gates **zero** of its 11 already-implemented file efuns (`read_file`, `write_file`, `read_bytes`, `write_bytes`, `save_object`, `restore_object`, `rm`, `mkdir`, `rmdir`, `rename`, `get_dir`, confirmed by grep) with either dialect's version. This is a whole missing cross-cutting security feature for both dialects at once -- a new shared path-check helper plus wiring into 11+ call sites, each needing its own literal `func` string -- not a single-efun signature divergence. Same category as `parse_*`: sized well beyond a normal batch item, needs its own explicit go-ahead as a dedicated row rather than folding into 1.16's remaining-items list.

No implementation this pass: all three blocked, none forced. `get_bb_uid` stays recorded as dead/unimplementable against this exact vendored source; `make_path_absolute` stays deferred behind `ed()`; `valid_read`/`valid_write` is recommended as its own future row (a real, well-scoped, dialect-shared file-permission feature) rather than staying folded into 1.16, whenever it gets an explicit go-ahead. `valid_query_snoop` (if `interactive_info()` is ever taken on as its own row) stays alongside it as previously noted) |

**Phase 1 scan pass (2026-08-18): every remaining row checked, nothing
picked up this pass.** Went through every still-`[ ]` Phase 1 row looking
for the next smallest actionable dialect divergence, excluding the three
already-decided-bigger items (`parse_*`/row 0.13a, the connect/disconnect
design question underneath rows 1.4/1.15/1.16, and row 1.7's own
`bind_lambda`/closure-kind work, which row 1.8's `#'symbol` and the
`'name` addendum above both confirm belong to the same family, not a
separate smaller one). Every other still-open row was already either
fully accounted for by an existing note (1.9 mapping width, 1.11
`rlimits`, both previously confirmed genuinely bigger) or freshly
investigated against the real vendored source this pass and found to
belong in that same category: 1.12 (`atomic` checkpoint/rollback -- real
DGD's own global "planes" data-versioning subsystem, not a call-stack
snapshot, and the same prerequisite row 1.11 already found underneath
`rlimits`), 1.13 (`parse_string` -- backed by a real 5,143-line DFA +
LALR grammar/parser-generator subsystem, `temp/dgd/src/parser/`, the
same category as the already-excluded `parse_*`), 1.14 (LWOs -- a third,
value-semantics call-dispatch kind threaded through DGD's own `Frame`
itself, not an isolated `Value` variant addition), and 1.15 (already
on record in `src/dialect/instruct.md` as the same connect/disconnect
design question, not independently investigated as new). See each row's
own note above for the full citations. Nothing in Phase 1 was small and
unambiguous this pass -- stopped and reported rather than forcing a pick,
per this project's own established discipline for `bind_lambda()`/
mapping width/`rlimits`. Next actionable work in Phase 1 requires an
explicit go-ahead on one of the four now-identified bigger items (`#'`/
`'name`/closures as a unit, mapping width, `rlimits`+`atomic`'s shared
planes prerequisite, or `parse_string`), or the connect/disconnect
design question being resolved first.

**Scope correction (2026-08-18, continued):** the framing directly above
("four now-identified bigger items", listed together as equally-weighted
Phase 1 blockers) is now out of date on priority, not on the research
itself. DGD was clarified this same pass as a comparison-only reference
dialect, not a required Phase 1 target (see the Phase 1 header above and
rows 1.11-1.15's own individual notes) -- of the four items listed here,
`rlimits`+`atomic`'s shared planes prerequisite and `parse_string` are
both DGD-only and therefore no longer Phase 1 blockers at all, comparison
research only. `#'`/`'name`/closures as a unit is LDMud, not DGD, and
keeps its real weight as an actual FluffOS/LDMud-family gap; likewise
mapping width (LDMud) and the connect/disconnect design question
(genuinely cross-dialect, not DGD-specific). The actual remaining
Phase 1 blockers after this correction: `#'`/`'name`/closures, mapping
width, and connect/disconnect -- three items, not four.

**Self-assessment and real commitment (2026-08-18, continued further):**
weighed all three remaining blockers against actual real-world mudlib
compatibility impact, not size -- `#'` closures is LDMud's own function-
pointer/callback idiom, used as pervasively in real LDMud codebases
(`sort_array`/`filter`/`map` callbacks, event/delegation patterns) as
this driver's already-supported FluffOS `(: name :)` is for FluffOS
ones; unsupported, it is a compile-time failure for most real LDMud
mudlib code, not a missing-feature gap in one corner of it. Mapping
width only affects mudlibs that specifically use LDMud's wide-mapping
idiom, narrower in reach. Connect/disconnect is a real but partial gap
-- `connect` itself is already name-compatible between FluffOS and
LDMud (both use the same master apply, confirmed in
`src/dialect/instruct.md`), so real LDMud mudlibs already log in
correctly today; only link-death/disconnect notification is actually
broken (wrong apply name, wrong target object, wrong signature) -- a
real bug, but not a whole-mudlib blocker the way an unparseable core
syntax is. Picked `#'` bare-name closures as the highest real-usage-
unlock item on that basis -- see row 1.2's own entry (and row 1.3's) for
the first slice actually implemented this same pass, not just scoped.

**1.2/1.3 scoping note (2026-08-18, scoping pass only, nothing implemented):**
rows 1.2/1.3 are the load-bearing prerequisite for most of what is left in
Phase 1 -- mapping width (1.9), `#'symbol` (1.8), DGD `nil`/`rlimits`/
`atomic` (1.10-1.12) all need real grammar support before their own
VM/efun-side rows mean anything. Read the real grammar/lexer sources
directly (`temp/ldmud/src/prolang.y`, `temp/ldmud/src/lex.c`,
`temp/ldmud/src/func_spec`, `temp/dgd/src/comp/parser.y`,
`temp/dgd/src/lex/lex.cpp`), not the existing plan above, which turned
out wrong on several load-bearing points -- corrected below.

**Current architecture, confirmed by reading the actual code, not
assumed:** `Lexer` (`src/compiler/Lexer.cpp`) produces only six generic
`TokenType`s (`Ident, Number, String, Symbol, Keyword, End`) -- there is
no per-keyword or per-operator enum at all; `kKeywords` is a single flat
`unordered_set<string>` (currently: type names, `mixed`/`function`,
control flow, `static`/`private`/`public`/`protected`/`nomask`/
`varargs`, `inherit`, `foreach`/`in`, `switch`/`case`/`default`), and
`Parser.cpp` does its own semantic dispatch by comparing `token.text`
directly (confirmed: `modifierKeywords`, `Parser.cpp:80-83`, already a
generic consume-any-of-this-list loop function declarations run through
for `static`/`private`/`nomask`/`varargs` etc). This is good news for
scoping: most single-keyword additions are shallow in this specific
codebase (extend two string sets, no token-type explosion), unlike a
bison-derived grammar. **Neither `Lexer` nor `Parser` takes an
`LpcDialect` parameter today** -- confirmed via both constructors
(`Lexer(std::string source)`, `Parser(std::vector<Token> tokens)`) and
their one real call site, `ObjectManager::compile()`
(`src/object/ObjectManager.cpp:464-466`) -- so dialect-conditional
keyword recognition cannot exist at all yet; this is the genuine
prerequisite everything else in 1.2/1.3 sits on top of.

**Corrections to the existing plan, confirmed by reading the real
sources directly:**
- **`lambda()`/`unbound_lambda()` are not keywords and need no new
  grammar at all -- remove them from 1.2/1.3's scope entirely.**
  `temp/ldmud/src/func_spec:497,500`: `closure lambda(null|mixed *,
  mixed);` / `closure unbound_lambda(null|mixed *, mixed);` -- ordinary
  efuns, the identical mechanism already confirmed for `bind_lambda()`
  and `replace_program()` in rows 1.5-1.7. `temp/ldmud/doc/LPC/closures`'s
  own worked example, `f = lambda( ({ 'x }), ({ #'environment, 'x }) );`,
  is plain function-call syntax with an array literal argument -- nothing
  for the Parser to add beyond what array literals and normal calls
  already do. What the old plan called `LambdaExpr`/`UnboundLambdaExpr`
  AST nodes do not need to exist; the real work is entirely `src/efun` +
  `src/vm` (a real `lambda`/`unbound_lambda` efun that builds a closure
  from the array-encoded body -- row 1.7/1.8 territory, not this row's).
- **`#'name` is real, but far richer than "a name after `#'`", and is
  blocked by a genuine architecture issue this driver has and real LDMud
  does not.** Read `temp/ldmud/src/lex.c`'s own `closure()` function
  (triggered from the main lexer's `case '#': if (*yyp == '\'') return
  closure(yyp);`, confirmed at `lex.c:6158-6162`) and `symbol_operator()`
  (`lex.c:1147-1677`) in full: `#'` covers roughly 50 distinct operator
  spellings (`#'+`, `#'+=`, `#'++`, `#'<<=`, `#'==`, ... ), the `#'[...]`
  index/range/map-index family (`#'[`, `#'[<`, `#'[,..]`, ...), the
  aggregate-array closure `#'({`, three real scope prefixes
  (`#'efun::name`, `#'sefun::name`, `#'lfun::name`, plus `#'var::name`
  for a global-variable closure -- `temp/ldmud/doc/LPC/closures:37-46`),
  and `#'Name::fun` for a direct reference to an inherited function. All
  of this resolves to one token, `L_CLOSURE`. **Architecture problem
  specific to this driver:** `ObjectManager::compile()` shells out to the
  real system `cpp` binary before this driver's own `Lexer` ever sees the
  source (`runPreprocessor()`, `src/object/ObjectManager.cpp:350-403`,
  confirmed: `cmd = "cpp -I ... -x c ..."`, standard non-traditional GCC
  cpp). Real LDMud has its own integrated preprocessor and never hits
  this; this driver's own architecture does. Standard GCC `cpp -x c`
  hard-errors ("invalid preprocessing directive") on any line whose first
  non-whitespace character is `#` and does not match a real preprocessor
  directive -- and `result.ok` is gated purely on `cpp`'s exit code
  (`ObjectManager.cpp:401-403`). A bare `#'this_player;` statement
  written on its own line (a completely ordinary way to write it) would
  make the *whole file* fail preprocessing before this driver's Lexer/
  Parser ever run, regardless of what they are taught to recognize. This
  needs a real decision (escape/mask `#'`-shaped lines before handing the
  file to `cpp` and restore them after, or stop shelling out to real
  `cpp` for LDMud-dialect files, or something else) as part of scoping
  this row properly, not just a lexer/parser change -- **this is the one
  item in this note that is architecture-touching, not isolated.**
- **`'name` (a bare leading quote, one or more) is a second, separate,
  previously entirely undocumented LDMud construct: a `symbol` value.**
  `temp/ldmud/src/lex.c`'s own `'` case (`lex.c:6184`ff, comment "':
  Character constant or lambda symbol") produces `L_SYMBOL`
  (`yylval.symbol.name`/`.quotes`), used directly in the `lambda()`
  example above (`'x` names a parameter) and by real efuns
  `symbol_function()`/`symbol_variable()`/`symbolp()`
  (`temp/ldmud/doc/efun/symbol_function` etc). This is a distinct LPC
  *type*, `symbol`, with no equivalent anywhere in this driver's `Value`
  variant (`int64_t, double, string, object, array, mapping, closure` --
  `include/amlp/vm/Value.hpp:16-25`) -- a real, previously-missed gap this
  scoping pass surfaced, not previously on the roadmap in any row. Needs
  its own `Value` variant member (a `src/vm` decision, out of this row's
  own scope to resolve, only to flag) before the Lexer/Parser side can be
  more than a token that has nowhere real to go. **Addendum (2026-08-18,
  re-checked as a Phase 1 scan-pass candidate, still not implemented):**
  read `lex.c`'s own `'` case in full rather than stopping at the citation
  above -- the real lexing is not just "read a name after a quote". A
  bare `'x'` (matched quote pair) is a plain *character constant*
  (`L_NUMBER`, unrelated to symbols at all), so the lexer needs genuine
  lookahead to tell it apart from `'x` (symbol). More significantly,
  `'({` is its own third production, `L_QUOTED_AGGREGATE` (`lex.c:6260-
  6264`) -- a *quoted array literal*, the exact "code as data" shape
  `lambda()`'s own worked example builds its quoted body out of (`({
  #'environment, 'x })`, cited above). So the bare-name symbol case
  (`L_SYMBOL`) and the quoted-array-literal case (`L_QUOTED_AGGREGATE`)
  share one lexer production and cannot be cleanly separated -- this is
  not an independent literal type sitting next to the closure-kind work,
  it is part of the same quoted-code family `lambda()`/`unbound_lambda()`/
  `#'` already are, reinforcing rather than contradicting row 1.7's own
  "stays deferred alongside... bind_lambda/closure work" decision, not a
  smaller item that decision's phrasing might otherwise seem to leave
  open.
- **Mapping width literal syntax is not `([ k: v1, v2, v3 ])` --
  commas cannot work, since the grammar already uses `,` to separate
  different key entries.** Read `temp/ldmud/src/prolang.y`'s own
  `m_expr_list2`/`m_expr_values` productions (`prolang.y:17224-17256`)
  directly: the real separator between multiple values *for the same
  key* is `;`, and `,` only ever separates *different* key entries --
  `([ "a": 1; 2; 3, "b": 4; 5; 6 ])`, not `([ "a": 1, 2, 3, "b": 4, 5, 6
  ])`. The grammar also enforces that every entry in one literal have the
  same width ("Inconsistent number of values in mapping literal",
  `prolang.y:17244`) -- a real semantic check, not just syntax. There is
  also a separate, simpler literal for an *empty* mapping of a given
  width, `([: width_expr ])` (`prolang.y:15200-15224`,
  `F_M_ALLOCATE`) -- not previously documented anywhere in this repo.
  Row 1.9 (`m_allocate`/`m_indices`/`m_values`) owns the VM/efun runtime
  side of mapping width; this row owns the two literal syntaxes.
- **DGD `rlimits` is not `rlimits(ticks : stack) { body }` -- wrong
  separator and wrong argument order.** Read `temp/dgd/src/comp/parser.y`
  directly (`parser.y:566-582`): the real grammar is `RLIMITS '(' expr
  ';' expr ')' compound_stmt` -- semicolon, not colon -- and the first
  expression is the **stack** limit, the second is **ticks** (confirmed
  by the real error messages checking each in that order: "bad type for
  stack rlimit" on the first, "bad type for ticks rlimit" on the second).
  Real shape: `rlimits (stack_expr; ticks_expr) { body }`.
- **DGD `atomic` is confirmed real and exactly as simple as the old plan
  assumed.** `temp/dgd/src/comp/parser.y:326-328`: `ATOMIC { $$ =
  C_ATOMIC; }`, parsed in the same `non_private` modifier-list production
  as `STATIC`/`NOMASK`/`VARARGS` (`parser.y:301-330`) -- this driver's own
  `modifierKeywords` mechanism (`Parser.cpp:80-83`) already generically
  consumes exactly this shape. The smallest, most mechanically contained
  real addition found in this entire scoping pass: no new AST node kind
  needed at the lexer/parser layer at all, purely extending two existing
  string sets. What `atomic` *means* (checkpoint/rollback) is row 1.12's
  own, separate, still-unstarted VM concern -- landing the keyword alone
  is syntactically real and inert until then, the same "recognized but
  not yet semantically wired" shape already accepted elsewhere in this
  project (e.g. the shadow work's documented `nomask`-check skip).
- **DGD `nil` is confirmed real and simple.** `temp/dgd/src/comp/
  parser.y:92,718`: `NIL` token, `Node::createNil()` -- parsed as an
  ordinary primary-expression literal, the same shape as any other
  literal token. Needs one new AST node kind (`NilLiteral`) and a
  CodeGen decision for what it emits until row 1.10 (`nil` as a distinct
  `Value` variant member) lands -- the smallest *new-AST-node* addition
  found, second only to `atomic` in overall size, but not quite as
  inert since a literal has to produce *something* today, which is a
  real (if small) coupling to row 1.10 that `atomic` does not have.
- **DGD's own closure/function-pointer syntax is a third, distinct
  family, not currently on this roadmap in any row.** `temp/dgd/src/comp/
  parser.y:771-794`: `&ident(args)` and `&(*cast_exp)(args)` (a
  "call template", DGD's own function-pointer literal, unrelated to
  FluffOS's `(: :)` or LDMud's `#'`), plus `RARROW`/`LARROW` (`->`/`<-`)
  for DGD's own persistent-object-call and inherited-super-call
  conventions respectively. Zero DGD lexer/parser work exists yet (`atomic`/
  `rlimits`/`nil` above are the only DGD syntax currently tracked anywhere
  on this roadmap) -- flagging this now, sized for whenever DGD's own
  dialect work actually gets picked up (row 1.15 and beyond), not
  something to size or implement in this pass.

**Classification -- small/isolated vs. architecture-touching:**
- **Small, isolated, no new AST/CodeGen surface:** DGD `atomic` modifier
  keyword (extends two existing string sets only).
- **Small, one new AST node, minor CodeGen coupling to row 1.10:** DGD
  `nil` keyword/`NilLiteral`.
- ~~Moderate, self-contained new grammar inside the existing
  mapping-literal parse path, no new AST node kind beyond a width
  field~~ -- **corrected 2026-08-18 after actually investigating it as
  a candidate slice: genuinely bigger, not moderate.** The literal
  grammar itself (`([ k: v1; v2 ])`, `([: N ])`) is exactly as small as
  described here, but real LDMud mappings are N-columns-wide at the
  *value* level too (`temp/ldmud/src/func_spec`'s own `m_allocate(int,
  int default: F_CONST1)`/`m_values(mapping, int default: F_CONST0)`/
  `m_entry`/`m_reallocate`/`m_add`/`m_contains`, none implemented in
  this driver at all), and this driver's own `Mapping`
  (`std::vector<std::pair<Value, Value>>`, one value per key, no width
  dimension) is load-bearing throughout `MakeMapping`/`Index`/
  `IndexAssign`/`sizeof`/`map_delete`/the `mapping + mapping` union-merge/
  save-restore serialization. A parser-only slice that accepted the
  syntax but silently discarded every value past the first would not
  pass this project's own "confirm end to end that it works" bar --
  same reasoning that already ruled out a partial `bind_lambda()` stand-in
  for row 1.7. See ROADMAP.md row 1.9 for the full finding; that row now
  owns this literal syntax too, not split across two rows.
- **Moderate, one new statement-level AST node
  (`RlimitsStmt`), self-contained, does not touch expression grammar:**
  DGD `rlimits (stack; ticks) { body }`, now with the corrected grammar
  above; its own `PushRlimits`/`PopRlimits` opcodes remain row 1.11's
  VM-side concern.
- **Large, new AST node, and coupled to an entirely new `Value` variant
  member (`symbol`) not previously tracked on this roadmap at all:**
  `'name` symbol literals.
- **Large, rich internal grammar (~50 operator spellings, index/range
  forms, scope prefixes) but self-contained as one token kind, EXCEPT for
  a genuine architecture problem in this driver's own preprocessing
  pipeline (real `cpp` hard-errors on a line-initial `#'`) that has to be
  decided before this can land at all:** `#'` closure literals.
- **Removed from this row's scope entirely, zero lexer/parser work
  needed:** `lambda()`/`unbound_lambda()` (ordinary efuns, `src/efun` +
  `src/vm` territory, rows 1.7/1.8).
- **Newly discovered, out of scope for now, sized for future DGD dialect
  work:** DGD's own `&ident(args)` closure syntax and `->`/`<-`
  operators.

**Proposed smallest-possible first slice -- greenlit and implemented
2026-08-18 (see rows 1.2/1.3 above for the implementation detail):**
two parts, in order, chosen so neither needs redoing once the fuller
scope above is tackled.
1. **Dialect plumbing only, zero behavior change.** Add an `LpcDialect`
   parameter to `Lexer`'s and `Parser`'s constructors (both currently
   take none at all -- confirmed above), threaded through from
   `ObjectManager::compile()`'s existing `Config&` via
   `dialectFromString(config_.dialect())` (row 1.1's own machinery,
   already used identically in `DialectSelect.cpp`), defaulting every
   existing call site to `LpcDialect::FluffOS` -- the same "provably a
   no-op for every config that never sets `dialect`" shape row 1.1's own
   `Config::dialect()` used. No new keyword, no new AST node, nothing
   user-visible changes this step; it only gives every later, real
   addition (`atomic`, `nil`, mapping width, `#'`, `'name`, ...) a clean,
   independent, dialect-gated landing spot instead of needing this same
   plumbing threaded through piecemeal on each one's own turn.
2. **DGD `atomic` as the first real per-dialect token**, riding on that
   plumbing. The single smallest real syntax addition found in this
   entire pass: extend `kKeywords` (Lexer) and `modifierKeywords`
   (Parser) with `"atomic"`, gated on `dialect_ == LpcDialect::DGD`,
   using the already-generic modifier-consumption loop at
   `Parser.cpp:80-83` verbatim. No new `TokenType`, no new AST node kind,
   no CodeGen change, no coupling to any other unfinished row (row 1.12's
   own VM semantics stay separately deferred, exactly like the shadow
   work's already-accepted `nomask`-skip precedent) -- a function
   declared `atomic` under `dialect: dgd` parses instead of failing, and
   is silently a no-op function until row 1.12 lands, which is a real,
   testable, zero-risk before/after. `nil`/`NilLiteral` was considered as
   the first real slice instead of `atomic` and is a close second (also
   small, also isolated from the architecture-touching items) but was not
   chosen for the recommendation: it needs a CodeGen emission decision
   the moment it exists (there is no equally inert "recognized but
   nothing happens yet" landing for a literal the way there is for a
   modifier keyword), a real, if small, coupling to row 1.10 that
   `atomic` avoids entirely. Both are viable; `atomic` is the
   marginally smaller and cleaner of the two.

Not proposed as part of any first slice, deliberately: `#'` (architecture
decision needed first), `'name`/symbol (new `Value` variant member needed
first), `lambda`/`unbound_lambda` (not this row's scope at all, see
correction above), mapping width and `rlimits` (each a real, self-contained
next step once the plumbing lands, but neither is smaller than `atomic`).

**Update (2026-08-18, continued further):** `#'`'s own architecture
decision has since been made and implemented (mask/unmask around the
`cpp` preprocessing call, see row 1.2's own entry) -- the sentence above
is accurate history of what this note proposed at the time, not the
current state. `'name`/symbol, `lambda`/`unbound_lambda`, mapping width,
and `rlimits` remain exactly as unimplemented as this note originally
found them.

---

## Phase 2 - Architecture differentiation

**Goal: surpass all three drivers on the dimensions none of them addressed.**

### 2a - Persistence

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 2.1 | World-level statedump: serialize full heap to compact binary snapshot | `src/persist` | [ ] |
| 2.2 | Object swapout: page inactive objects to disk; demand-page on access | `src/persist` + `src/object` | [ ] |
| 2.3 | Hotboot: fd-passing exec into new binary without dropping connections | `src/persist` + `src/net` | [ ] |
| 2.4 | Dual persistence: per-object `save_object` AND world snapshot coexist | `src/persist` + `src/efun` | [ ] |

### 2b - Concurrency

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 2.5 | C++20 coroutine scheduler: cooperative suspend/resume of LPC tasks | `src/scheduler` | [ ] |
| 2.6 | LPC `async`/`await` keyword pair backed by coroutine scheduler | `src/compiler` + `src/vm` + `src/scheduler` | [ ] |
| 2.7 | `call_out_future(delay)` - awaitable call_out | `src/efun` + `src/scheduler` | [ ] |
| 2.8 | Open Hydra: speculative parallel tasks on disjoint object graphs | `src/scheduler` | [ ] |

### 2c - Apply cache + JIT

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 2.9 | Apply cache: hash (object × function-name) → FunctionEntry; invalidate on recompile | `src/apply` + `src/vm` | [ ] |
| 2.10 | Closure bake-at-construction: resolve `FP_*` kind + index at bind time | `src/vm` + `src/compiler` | [ ] |
| 2.11 | LLVM JIT backend: compile hot bytecode functions to native via LLVM IR | `src/jit` | [ ] |

### 2d - Efun breadth beyond FluffOS

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 2.12 | Full PCRE regexp suite (already started in Phase 0 - extend) | `src/efun` | [ ] |
| 2.13 | TLS support (OpenSSL/BoringSSL) for game + MXP/WebSocket | `src/net` | [ ] |
| 2.14 | WebSocket framing on top of TLS | `src/net` | [ ] |
| 2.15 | SQLite built-in: `db_connect`/`db_exec`/`db_fetch`/`db_close` efuns | `src/efun` | [ ] |
| 2.16 | Hash efuns: SHA-256/SHA-512/MD5/bcrypt/BLAKE2 | `src/efun` | [ ] |
| 2.17 | `json_encode`/`json_decode` efun pair | `src/efun` | [ ] |
| 2.18 | `http_get`/`http_post` async efuns (non-blocking, via async scheduler) | `src/efun` + `src/scheduler` | [ ] |

### 2e - Developer experience

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 2.19 | LSP server for LPC (`--lsp` flag): hover, go-to-def, diagnostics | `src/lsp` | [ ] |
| 2.20 | Structured error objects: JSON-serializable source/line/column/message | `src/core` | [ ] |
| 2.21 | Hot-reload: recompile + migrate one `.c` file while server is live | `src/object` + `src/vm` | [ ] |
| 2.22 | LPC-native test runner: `assert_equal`/`assert_throws` efun suite | `src/efun` + mudlib `std/test.c` | [ ] |

---

## Phase 3 - Production hardening

| # | Task | Directory | Status |
|---|------|-----------|--------|
| 3.1 | Full `privs_file` / uid/gid object trust hierarchy | `src/security` | [ ] |
| 3.2 | Filesystem jail per object domain; capability grants for `call_other` | `src/security` | [ ] |
| 3.3 | Generational GC replacing `shared_ptr`-everywhere | `src/gc` | [ ] |
| 3.4 | Full telnet option negotiation + GMCP, MSDP, MSSP, MTTS, MXP | `src/proto` + `src/net` | [ ] |
| 3.5 | Conformance test suite (any driver can certify FluffOS/LDMud/DGD dialect) | `tests` | [ ] |
| 3.6 | LPC language specification document | `docs` | [ ] |
| 3.7 | Driver API reference + mudlib porting guide for each dialect | `docs` | [ ] |
| 3.8 | Boot a real third-party FluffOS mudlib against this driver (dead-souls.net's TMI2, LPUniversity, or LIL) | `tests` | [ ] |

**3.8 scope note:** a distinct compatibility test from `test/test_lexer.cpp`'s own regression suite, not a replacement for it -- that suite exercises this driver's own AMLP-derived mudlib content; this row exercises an independently-written real-world mudlib this driver has never seen. Not started. Involves, roughly in order: download and unpack one of the three archives (dead-souls.net's `tmi2_fluffos_v3.zip`, `lpuni_fluffos_v1.zip`, or `lil_0.3.zip`); identify real mismatches against this driver's own current conventions (master-object apply table, simul_efun resolution, directory-layout/`#include` path assumptions, any efun that mudlib calls but this table doesn't register); then whatever driver-side compat work those actually-found gaps call for, scoped to what's found, not speculative. No work done yet -- filed as a row, per this session's own request, so it has a place to land rather than being reintroduced ad hoc later.

---

## Build and test

```bash
cmake -B build -S .
cmake --build build
ctest --test-dir build --output-on-failure
```

Current baseline: **625 tests passing** (as of 2026-08-18). Every new slice
must pass the full suite before merging.

---

## Sequencing principle

- **Never break the current test baseline** (see above). All work is incremental slices.
- **Phase 0 before Phase 1.** A buggy foundation makes dialect work meaningless.
- **Phase 1 before Phase 2.** Dialect abstraction unlocks concurrent dialect work.
- **Read the instruct.md in the target directory first.** Each one lists exact
  files to read, exact reference sources, and the precise scope of its tasks.
- **One slice = one PR.** Keep changes small, reviewable, and revertable.
