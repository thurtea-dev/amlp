#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace lpcdriver {

enum class OpCode : uint8_t {
    PushConst,
    PushInt,
    // A float literal ("1.5", ".5"). Not folded into PushInt's own
    // operand (an int32_t cannot hold an arbitrary double) or into
    // PushConst's stringPool (a float is not a string) -- indexes into
    // CompiledProgram::floatPool instead, mirroring PushConst/stringPool
    // exactly.
    PushFloat,
    PushLocal,
    StoreLocal,
    PushObjectVar,
    StoreObjectVar,
    Add, Sub, Mul, Div, Mod,
    // Plain "&": bitwise AND on two ints, set intersection on two arrays
    // (see Ast.hpp's BinOp::BitAnd comment). Named distinctly from the
    // logical &&/|| in BinOp since those never reach a plain opcode at
    // all (they desugar to jumps, see CodeGen::emitLogicalExpr).
    BitAnd,
    // Plain "|"/"^": int-only bitwise OR/XOR this slice (see Ast.hpp's
    // BinOp::BitOr/BitXor comment -- real FluffOS's "|" is also array
    // union, not replicated here since nothing this driver runs yet
    // needs it, only the plain-int flags-bitmask shape).
    BitOr,
    BitXor,
    Eq, Neq, Lt, Lte, Gt, Gte,
    Not,
    Jump,
    JumpIfFalse,
    // A bare-name call ("foo(...)"), resolved at run time in this order:
    // this program's own functions, then each inherited program's
    // functions (recursively, see VM.cpp's findFunctionInChain), then the
    // efun table. This is the only opcode plain CallExpr codegen emits;
    // CallEfun is reserved for compiler-forced efun calls that must never
    // be shadowed by a same-named local function (currently just the "->"
    // / call_other() translation, see emitCallOtherExpr).
    Call,
    CallEfun,
    // "::name(...)" / "qualifier::name(...)" -- an explicit call to an
    // *inherited* definition, bypassing whatever the currently executing
    // program's own local definition of `name` might be (see Ast.hpp's
    // CallExpr::parentCall comment). operand = function name string-pool
    // index, argCount = number of args, same shape as Call/CallEfun.
    // Always immediately followed by one CallParentQualifierSlot data
    // instruction (never executed directly, exactly like Sscanf's own
    // trailing SscanfVarSlot entries) carrying the qualifier: its operand
    // is a string-pool index for "qualifier::name(...)", or -1 for the
    // bare "::name(...)" form (search the whole inherited chain).
    CallParent,
    CallParentQualifierSlot,
    // "(: name, bound_args... :)" -- constructs a Closure value (see
    // Value.hpp's Closure comment) bound to the currently executing
    // object, the given bare name, and however many already-evaluated
    // bound-arg values are on top of the stack. operand = function name
    // string-pool index, argCount = number of bound args (same shape as
    // Call/CallEfun; no trailing data instruction needed since a
    // closure literal, unlike CallParent, has nothing else to encode).
    PushClosure,
    CallApply,
    MakeArray,
    MakeMapping,
    Index,
    IndexAssign,
    RangeIndex,
    // sscanf(target, format, ...vars). operand = number of trailing output
    // vars (N); immediately followed in the instruction stream by N
    // SscanfVarSlot entries, one per var, which the Sscanf handler reads
    // directly and always skips over -- they are data, not opcodes to
    // execute, and reaching one through normal ip advancement is a bug.
    Sscanf,
    SscanfVarSlot,
    // Pops a value; pushes it back unchanged if it is an array (foreach
    // over an array walks its elements directly), or pushes an array of
    // its keys if it is a mapping (foreach over a mapping walks keys, or
    // (key, value) pairs for the two-variable form -- see
    // CodeGen::emitForeachStmt). This is the one piece of foreach's
    // desugaring that genuinely needs a runtime type check rather than
    // just the existing opcode set.
    ForeachKeys,
    // catch(expr) (see Ast.hpp's CatchExpr comment). Mirrors real
    // FluffOS's F_CATCH/F_END_CATCH pair (icode.c's NODE_CATCH case):
    // PushCatchFrame's operand is a forward-patched jump target -- the
    // instruction immediately after the matching PopCatchFrame, i.e.
    // "resume here on error" -- recorded exactly the same way
    // Jump/JumpIfFalse's own targets are (see CodeGen::emitJumpPlaceholder/
    // patchJumpToHere). On normal completion, execution just flows
    // through the guarded bytecode into PopCatchFrame, landing at that
    // same instruction on its own; the operand only matters for the
    // error path, where VM::run() jumps there directly after unwinding.
    PushCatchFrame,
    // Only reached on normal (non-error) completion of the guarded
    // region -- an error unwinds straight to PushCatchFrame's recorded
    // target without ever executing this opcode. Pushes int 0 (real
    // LPC's own "no error" result, confirmed against interpret.c's
    // F_END_CATCH: "catch_value = const0; ... push_number(0)"). The
    // guarded expression's own result was already discarded by an
    // explicit Pop CodeGen emits right before this (matching real
    // FluffOS's own insert_pop_value() on the catch argument, see
    // trees.c) -- catch(expr) never evaluates to expr's own value, only
    // to 0 or the error message.
    PopCatchFrame,
    Return,
    Pop,
    Dup,
    Halt
};

struct Instruction {
    OpCode op;
    int32_t operand = 0;
    int32_t argCount = 0;
};

struct FunctionEntry {
    std::string name;
    uint32_t entryPoint = 0;
    uint8_t numArgs = 0;
    uint8_t numLocals = 0;
};

struct CompiledProgram {
    std::vector<Instruction> code;
    std::vector<FunctionEntry> functions;
    std::vector<std::string> stringPool;
    std::vector<double> floatPool;
    std::vector<std::string> objectVarNames;
    // Raw "inherit \"path\";" targets as parsed, before resolution.
    std::vector<std::string> inherits;
    // The above paths resolved and compiled, in the same order, by
    // ObjectManager::compile() after CodeGen produces this program (CodeGen
    // itself has no file-loading capability). Empty until then. Only the
    // immediate parents are stored here, but since each of those programs
    // carries its own inheritedPrograms too (if it itself inherits
    // something), function-call and object-variable resolution both walk
    // the full chain -- see VM.cpp's findFunctionInChain.
    std::vector<std::shared_ptr<CompiledProgram>> inheritedPrograms;

    // Every program transitively anywhere in this program's own inherit
    // tree (immediate parents and their own ancestors, recursively),
    // mapped to the absolute base offset at which that program's own
    // object variables start within *this* program's flattened
    // objectVarNames/variables() layout. A program's own bytecode always
    // addresses its object variables with slot numbers relative only to
    // its own direct inherit chain (correct when that program runs
    // completely on its own); when one of its functions instead runs as
    // part of a *different*, larger object -- because it was reached via
    // inheritance, and CompiledProgram is cached and reused verbatim
    // everywhere a file is inherited (see ObjectManager::compile()'s own
    // comment) -- the VM must add this offset before indexing
    // LpcObject::variables(), or two unrelated sibling files that are
    // each inherited directly (no parents of their own) will silently
    // alias each other's low slot numbers. Populated by
    // ObjectManager::compile() right after CodeGen::generate() returns,
    // by combining each direct parent's own running prefix offset with
    // that parent's own ancestorBaseOffsets (recursive composition, not
    // just one level). Does not include an entry for this program
    // itself: code belonging to *this* program running directly against
    // an object whose own program() is this same program needs no
    // adjustment (offset 0), which VM::run() treats as the fast-path
    // default rather than a map lookup.
    //
    // Scope note: this assumes single-copy (non-diamond) inheritance --
    // the same ancestor file reached via two different inherit paths
    // within one object gets only one entry here (the second path's
    // offset silently wins), matching how real LPC would actually give
    // that ancestor two separate flattened copies at two different
    // offsets instead. No diamond shape has been found in this mudlib's
    // actual inherit graph as of this fix; if one turns up, this map
    // needs to become multi-valued (or inheritance resolution needs to
    // duplicate the ancestor's slots per path) rather than silently
    // picking one.
    std::unordered_map<const CompiledProgram*, int> ancestorBaseOffsets;
};

} // namespace lpcdriver
