#pragma once
#include <string>
#include <unordered_map>
#include "lpcdriver/compiler/Ast.hpp"
#include "lpcdriver/vm/Bytecode.hpp"

namespace lpcdriver {

class CodeGen {
public:
    // inheritedObjectVarNames is the flattened, in-order list of object
    // variable names declared by this program's inherit chain (already
    // resolved by the caller -- ObjectManager -- since CodeGen only ever
    // sees one file's own AST and has no file-loading capability of its
    // own). Passing it in lets this file's own PushObjectVar/StoreObjectVar
    // slot numbers, and any bare references to an inherited variable by
    // name, line up with the same slots the inherited program's own
    // bytecode already uses -- see ObjectManager::compile() for how the
    // two are kept consistent.
    CompiledProgram generate(const Program& program,
                              const std::vector<std::string>& inheritedObjectVarNames = {});

private:
    // Distinguishes a bare identifier resolving to a per-call local
    // (which also covers parameters, since they already share the same
    // slot space as locals) versus a per-object variable. A local always
    // shadows an object variable of the same name, matching real LPC's
    // local-wins-over-global precedence: resolveVariable() checks
    // locals_ before objectVars_.
    enum class VarKind { Local, ObjectVar };
    struct ResolvedVar { VarKind kind; int slot; };
    ResolvedVar resolveVariable(const std::string& name) const;

    int internString(const std::string& s);
    int internFloat(double d);
    void emitExpr(const AstNode& expr);
    void emitCallExpr(const CallExpr& call);
    void emitCallOtherExpr(const CallOtherExpr& callOther);
    void emitSscanfExpr(const SscanfExpr& sscanf);
    void emitBinaryExpr(const BinaryExpr& bin);
    void emitLogicalExpr(const BinaryExpr& bin);
    void emitTernaryExpr(const TernaryExpr& tern);
    void emitCatchExpr(const CatchExpr& catchExpr);
    void emitAssignExpr(const AssignExpr& assign);
    void emitIndexAssignExpr(const IndexAssignExpr& assign);
    void emitIncDecExpr(const IncDecExpr& incDec);
    void emitStatement(const AstNode& stmt);
    void emitReturnStmt(const ReturnStmt& stmt);
    void emitVarDeclStmt(const VarDeclStmt& stmt);
    void emitAssignStmt(const AssignStmt& stmt);
    void emitIndexAssignStmt(const IndexAssignStmt& stmt);
    void emitIfStmt(const IfStmt& stmt);
    void emitWhileStmt(const WhileStmt& stmt);
    void emitForStmt(const ForStmt& stmt);
    void emitForeachStmt(const ForeachStmt& stmt);
    void emitSwitchStmt(const SwitchStmt& stmt);
    void emitBreakStmt();
    void emitContinueStmt();
    void emitBlock(const Block& block);

    int declareLocal(const std::string& name);

    size_t emitJumpPlaceholder(OpCode op);
    void patchJumpTo(size_t jumpInstrIndex, size_t target);
    void patchJumpToHere(size_t jumpInstrIndex);

    // One entry per loop currently being compiled (nested loops push
    // their own), holding the as-yet-unpatched Jump placeholders emitted
    // by any break/continue inside it. Each loop's own emitWhileStmt()/
    // emitForStmt() patches its entry's jumps once it knows where they
    // should land (loop exit for break; the condition re-check for a
    // while's continue, the update clause for a for's) and pops it.
    // isSwitch marks a switch statement's own frame: break targets the
    // innermost frame regardless of kind, but continue must skip past
    // any switch frames to reach the nearest *loop* -- "continue" inside
    // a switch nested in a loop still continues the loop, real LPC/C
    // switch has no continue semantics of its own. See emitContinueStmt.
    struct LoopContext {
        std::vector<size_t> breakJumps;
        std::vector<size_t> continueJumps;
        bool isSwitch = false;
    };
    std::vector<LoopContext> loopStack_;
    // Gives each foreach statement's hidden bookkeeping locals
    // ("$foreach_orig_<n>" etc, see emitForeachStmt) a unique name within
    // the function being compiled, so sibling and nested foreach loops
    // never collide. Reset per function alongside locals_/loopStack_.
    int foreachCounter_ = 0;
    // Same idea as foreachCounter_, for switch's own hidden subject local.
    int switchCounter_ = 0;
    // Same idea again, for emitIndexAssignExpr()'s hidden temp local
    // (holds the newly assigned value so it can be pushed back onto the
    // stack after OpCode::IndexAssign, which itself leaves nothing
    // behind -- see that function's own comment).
    int indexAssignCounter_ = 0;

    CompiledProgram* out_ = nullptr;
    std::unordered_map<std::string, int> locals_;
    int nextLocalSlot_ = 0;
    // Per-object variable slots, populated once per generate() call from
    // Program::objectVars before any function body is compiled, and left
    // untouched afterward (unlike locals_, which is per-function).
    std::unordered_map<std::string, int> objectVars_;

    // An InlineLambdaExpr (see Ast.hpp) encountered while compiling some
    // enclosing function's body cannot have its own bytecode emitted
    // in-place: that would splice the lambda's Return into the middle of
    // the enclosing function's own instruction stream, since every
    // function in a CompiledProgram shares one flat code_ array
    // addressed only by entryPoint (see generate()'s own per-function
    // loop). Instead, emitExpr() records the pending body here and emits
    // an ordinary PushClosure referencing a synthesized name; generate()
    // drains this list right after finishing the enclosing function's
    // own Return, compiling each one exactly like a normal top-level
    // function (own locals_/entryPoint/FunctionEntry) so it lands at a
    // distinct, self-contained offset. The synthesized name is never a
    // legal LPC identifier (see nextLambdaId_'s use in CodeGen.cpp), so
    // it can never collide with a real function and is only ever reached
    // via the Closure's stored functionName at call time -- the exact
    // same findFunctionInChain() lookup an ordinary bare-name closure
    // already uses, no VM.cpp changes needed.
    struct PendingLambda {
        std::string name;
        const InlineLambdaExpr* expr = nullptr;
    };
    std::vector<PendingLambda> pendingLambdas_;
    int nextLambdaId_ = 0;
    void emitPendingLambdas();
};

} // namespace lpcdriver
