#include "lpcdriver/vm/VM.hpp"
#include "lpcdriver/object/ObjectManager.hpp"
#include "lpcdriver/object/LpcObject.hpp"
#include "lpcdriver/config/Config.hpp"
#include "lpcdriver/core/Errors.hpp"
#include "lpcdriver/efun/EfunTable.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace lpcdriver {

namespace {

// Pushes obj as the current object for as long as this guard is alive,
// on both of VM's call-tracking stacks -- real FluffOS's
// setup_fake_frame() (interpret.c), which runs unconditionally at the
// top of call_function_pointer() before its type-specific switch (i.e.
// for every closure kind, not just the ones that recurse into more LPC
// bytecode): "previous_ob = current_object; current_object =
// fun->hdr.owner". VM::run() uses this for every LPC function
// activation; VM::callClosure() additionally needs it around a
// closure's own core-efun invocation specifically (see callClosure()'s
// own comment) -- unlike the local/simul_efun-function branches, that
// path does not go through run() at all, so without this guard
// vm.currentObject()/previous_object() would still reflect whichever
// object's run() frame happens to be innermost (whoever called
// evaluate()), not the closure's own owner, breaking any efun that
// looks at "the current object" (save_object() being exactly the one
// that surfaced this live: secure/daemon/account_d.c's own "unguarded((:
// save_object, path :))" was saving master.c's own variables instead of
// account_d.c's).
//
// Object-change detection mirrors real FRAME_OB_CHANGE: only push a new
// objectChangeStack_ entry when obj actually differs from the
// immediately enclosing frame, not on every same-object call.
class ObjectFrameGuard {
public:
    ObjectFrameGuard(std::vector<std::shared_ptr<lpcdriver::LpcObject>>& callStack,
                      std::vector<std::shared_ptr<lpcdriver::LpcObject>>& objectChangeStack,
                      const std::shared_ptr<lpcdriver::LpcObject>& obj)
        : callStack_(callStack), objectChangeStack_(objectChangeStack) {
        objectChanged_ = callStack_.empty() || callStack_.back() != obj;
        if (objectChanged_) {
            objectChangeStack_.push_back(callStack_.empty() ? nullptr : callStack_.back());
        }
        callStack_.push_back(obj);
    }
    ~ObjectFrameGuard() {
        callStack_.pop_back();
        if (objectChanged_) objectChangeStack_.pop_back();
    }
    ObjectFrameGuard(const ObjectFrameGuard&) = delete;
    ObjectFrameGuard& operator=(const ObjectFrameGuard&) = delete;

private:
    std::vector<std::shared_ptr<lpcdriver::LpcObject>>& callStack_;
    std::vector<std::shared_ptr<lpcdriver::LpcObject>>& objectChangeStack_;
    bool objectChanged_ = false;
};

// Resolves a bare function-call name against a program's own functions
// first, then depth-first against each program it inherits (which may
// itself inherit further -- see Bytecode.hpp's CompiledProgram comment).
// This is the run-time half of OpCode::Call; the compile-time half is
// CodeGen::emitCallExpr(), which never tries to decide locally-vs-
// inherited-vs-efun itself.
struct FunctionLookupResult {
    const CompiledProgram* program = nullptr;
    const FunctionEntry* fn = nullptr;
};

FunctionLookupResult findFunctionInChain(const CompiledProgram& program, const std::string& name) {
    for (const auto& fn : program.functions) {
        if (fn.name == name) return FunctionLookupResult{&program, &fn};
    }
    for (const auto& parent : program.inheritedPrograms) {
        if (!parent) continue;
        FunctionLookupResult found = findFunctionInChain(*parent, name);
        if (found.program) return found;
    }
    return FunctionLookupResult{};
}

// Run-time half of OpCode::CallParent (see Bytecode.hpp's own comment
// and Ast.hpp's CallExpr::parentCall): resolves "::name(...)"/
// "qualifier::name(...)", which must skip *this* program's own
// functions entirely and search only inherited ones, even if this
// program itself defines a same-named function (the entire point of the
// syntax -- e.g. an overridden create() explicitly calling its parent's
// create() too). Bare form (no qualifier) walks every immediate parent
// depth-first via the same findFunctionInChain() the plain Call opcode
// uses, just starting one level down; a qualifier restricts the search
// to the one immediate parent whose own "inherit" path's last path
// component matches it (e.g. "daemon::create()" for
// "inherit \"/std/daemon\";").
std::string pathBasename(const std::string& path) {
    size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

FunctionLookupResult findParentFunction(const CompiledProgram& program, const std::string& name,
                                         const std::string* qualifier) {
    if (!qualifier) {
        for (const auto& parent : program.inheritedPrograms) {
            if (!parent) continue;
            FunctionLookupResult found = findFunctionInChain(*parent, name);
            if (found.program) return found;
        }
        return FunctionLookupResult{};
    }

    for (size_t i = 0; i < program.inherits.size() && i < program.inheritedPrograms.size(); ++i) {
        if (pathBasename(program.inherits[i]) != *qualifier) continue;
        const auto& parent = program.inheritedPrograms[i];
        if (!parent) continue;
        return findFunctionInChain(*parent, name);
    }
    return FunctionLookupResult{};
}

// Ports FluffOS's inter_sscanf() (interpret.c) for the subset this driver
// supports: literal text, "%%", "%s", "%d", and the "%*" skip modifier
// (which matches but does not consume an output slot). "%f", "%x", and
// "%(regexp)" are deliberately not implemented -- this mudlib's early-boot
// files never use them (confirmed by grepping every sscanf() call in
// secure/daemon/master.c) -- and throw rather than silently mishandling
// them if some other file ever does.
//
// The real algorithm's "%s directly followed by another %-specifier with
// no literal text between them" case (its own lookahead heuristic per
// specifier type) is also not implemented for the same reason: nothing in
// this mudlib's sscanf calls needs it, every %s is terminated by either
// literal text or the end of the format string.
struct SscanfOutcome {
    int64_t matchCount = 0;
    std::vector<Value> assigned; // one entry per *consumed* (non-skip) slot, in order
};

SscanfOutcome runSscanf(const std::string& in0, const std::string& fmt0, size_t maxAssigns) {
    SscanfOutcome out;
    size_t ip = 0, fp = 0;
    const size_t inLen = in0.size(), fmtLen = fmt0.size();

    for (;;) {
        // Match literal text up to the next '%' (or end of format).
        while (fp < fmtLen && fmt0[fp] != '%') {
            if (ip >= inLen || in0[ip] != fmt0[fp]) return out;
            ++ip; ++fp;
        }

        if (fp >= fmtLen) {
            // Format exhausted. Any leftover input becomes one final match
            // in the next output slot, if one is still available.
            if (ip < inLen && out.assigned.size() < maxAssigns) {
                out.assigned.emplace_back(Value(in0.substr(ip)));
                ++out.matchCount;
            }
            return out;
        }

        ++fp; // consume '%'
        if (fp < fmtLen && fmt0[fp] == '%') {
            // Literal "%%".
            if (ip >= inLen || in0[ip] != '%') return out;
            ++ip; ++fp;
            ++out.matchCount;
            continue;
        }
        if (fp >= fmtLen) {
            throw LpcRuntimeError("sscanf: format string cannot end in '%'");
        }

        bool skip = (fmt0[fp] == '*');
        if (skip) ++fp;
        if (fp >= fmtLen) {
            throw LpcRuntimeError("sscanf: format string cannot end in '%'");
        }
        char spec = fmt0[fp++];

        if (spec == 'd') {
            const char* start = in0.c_str() + ip;
            char* endPtr = nullptr;
            long long value = std::strtoll(start, &endPtr, 10);
            if (endPtr == start) return out; // no digits matched
            ip += static_cast<size_t>(endPtr - start);
            if (!skip) {
                if (out.assigned.size() >= maxAssigns) {
                    throw LpcRuntimeError("sscanf: too few output arguments for format string");
                }
                out.assigned.emplace_back(Value(static_cast<int64_t>(value)));
            }
            ++out.matchCount;
            continue;
        }

        if (spec != 's') {
            throw NotImplementedError(
                std::string("sscanf: format specifier '%") + spec +
                "' (only %s, %d, %% are supported this slice)");
        }

        // %s. If the format is now exhausted, the rest of in_string is the
        // match (real inter_sscanf's "we have reached the end of the
        // format string" case).
        if (fp >= fmtLen) {
            if (!skip) {
                if (out.assigned.size() >= maxAssigns) {
                    throw LpcRuntimeError("sscanf: too few output arguments for format string");
                }
                out.assigned.emplace_back(Value(in0.substr(ip)));
            }
            ++out.matchCount;
            return out;
        }

        if (fmt0[fp] == '%') {
            throw NotImplementedError(
                "sscanf: \"%s\" directly adjacent to another format specifier is not supported this slice");
        }

        // %s terminated by literal text: find where that literal text
        // next occurs in the remaining input, and take everything before
        // it as the match.
        size_t delimStart = fp;
        while (fp < fmtLen && fmt0[fp] != '%') ++fp;
        std::string delim = fmt0.substr(delimStart, fp - delimStart);

        size_t found = in0.find(delim, ip);
        if (found == std::string::npos) return out;

        if (!skip) {
            if (out.assigned.size() >= maxAssigns) {
                throw LpcRuntimeError("sscanf: too few output arguments for format string");
            }
            out.assigned.emplace_back(Value(in0.substr(ip, found - ip)));
        }
        ip = found + delim.size();
        ++out.matchCount;
        // fp already sits at the '%' (or fmtLen) that starts the next
        // segment; the top of the loop picks up from there.
    }
}

} // namespace

VM::VM(ObjectManager& objects, Config& config)
    : objects_(objects), config_(config) {}

Value VM::callFunction(const std::shared_ptr<LpcObject>& obj,
                        const std::string& functionName,
                        std::vector<Value> args) {
    if (!obj) return Value{};

    // External entry points (call_other, ObjectManager's create() call,
    // applyMaster()) must resolve inherited-but-not-locally-overridden
    // functions the same way a bare in-file call does, or calling an
    // object that only picked up a function via "inherit" (extremely
    // common in this mudlib, e.g. every DAEMON-inheriting command) would
    // silently do nothing instead of running it. Unlike OpCode::Call's
    // resolution, this deliberately does not fall back to the efun table
    // -- call_other("some/object", "sizeof") calling the sizeof() efun on
    // an unrelated object would not be a call_other at all.
    FunctionLookupResult found = findFunctionInChain(obj->program(), functionName);
    if (!found.program) return Value{};
    return run(*found.program, *found.fn, std::move(args), obj);
}

Value VM::applyMaster(const std::string& applyName, std::vector<Value> args) {
    auto master = objects_.masterObject();
    if (!master) {
        throw LpcRuntimeError("applyMaster(" + applyName + "): master object not loaded");
    }
    return callFunction(master, applyName, std::move(args));
}

std::shared_ptr<LpcObject> VM::cloneObject(const std::string& filename) {
    return objects_.cloneObject(filename);
}

void VM::destructObject(const std::shared_ptr<LpcObject>& obj) {
    objects_.destructObject(obj);
}

std::shared_ptr<LpcObject> VM::masterObject() const {
    return objects_.masterObject();
}

std::shared_ptr<LpcObject> VM::findObject(const std::string& filename) const {
    // See VM.hpp's own comment: real find_object() compiles+loads on a
    // miss, which is exactly ObjectManager::loadObject()'s existing
    // cache-by-filename behavior (also used for the master and
    // simul_efun objects at boot).
    return objects_.loadObject(filename);
}

std::shared_ptr<LpcObject> VM::lookupObject(const std::string& filename) const {
    return objects_.lookupLoadedObject(filename);
}

std::shared_ptr<LpcObject> VM::currentObject() const {
    return callStack_.empty() ? nullptr : callStack_.back();
}

std::shared_ptr<LpcObject> VM::previousObject(int idx) const {
    if (idx < 0 || static_cast<size_t>(idx) >= objectChangeStack_.size()) return nullptr;
    return objectChangeStack_[objectChangeStack_.size() - 1 - static_cast<size_t>(idx)];
}

std::vector<std::shared_ptr<LpcObject>> VM::allPreviousObjects() const {
    std::vector<std::shared_ptr<LpcObject>> result;
    for (auto it = objectChangeStack_.rbegin(); it != objectChangeStack_.rend(); ++it) {
        if (*it) result.push_back(*it);
    }
    return result;
}

// See VM.hpp's own comment for the overall contract. The lazy-
// resolve-by-name simplification here (versus real FluffOS's
// FP_LOCAL/FP_SIMUL/FP_EFUN classification baked in at the "(: :)"
// literal's own construction time) is safe for this driver's current
// scope specifically because every closure actually reachable in this
// mudlib is built and called within the same short-lived scope --
// e.g. "unguarded((: file_size, p :))" constructs the closure and
// hands it straight to unguarded(), which calls it immediately via
// evaluate(); nothing stores one in an object variable, redefines the
// named function in between, and calls it later expecting the
// original binding to have survived. If that ever changes, this
// comment is the place to revisit it.
Value VM::callClosure(const std::shared_ptr<Closure>& closure, std::vector<Value> extraArgs) {
    if (!closure) {
        throw LpcRuntimeError("evaluate(): not a function value");
    }
    auto owner = closure->owner.lock();
    if (!owner) {
        throw LpcRuntimeError("evaluate(): owner of function pointer is destructed");
    }

    std::vector<Value> args;
    args.reserve(closure->boundArgs.size() + extraArgs.size());
    for (const auto& a : closure->boundArgs) args.push_back(a);
    for (auto& a : extraArgs) args.push_back(std::move(a));

    FunctionLookupResult found = findFunctionInChain(owner->program(), closure->functionName);
    if (found.program) {
        return run(*found.program, *found.fn, std::move(args), owner);
    }

    auto simulEfun = objects_.simulEfunObject();
    if (simulEfun) {
        FunctionLookupResult simulFound = findFunctionInChain(simulEfun->program(), closure->functionName);
        if (simulFound.program) {
            return run(*simulFound.program, *simulFound.fn, std::move(args), simulEfun);
        }
    }

    if (EfunTable::instance().exists(closure->functionName)) {
        // Unlike the local/simul_efun branches above, calling a core
        // efun does not recurse into run() at all, so without this
        // guard vm.currentObject() would still read whatever object's
        // run() frame is innermost (whoever called evaluate()) instead
        // of this closure's own owner -- see ObjectFrameGuard's own
        // comment, this is exactly the live bug it fixes.
        ObjectFrameGuard objectFrameGuard(callStack_, objectChangeStack_, owner);
        return EfunTable::instance().call(closure->functionName, *this, args);
    }

    throw LpcRuntimeError("evaluate(): undefined function or efun: " + closure->functionName);
}

std::string VM::resolveMudlibPath(const std::string& lpcPath) const {
    return config_.mudlibRoot() + lpcPath;
}

Value VM::run(const CompiledProgram& program, const FunctionEntry& fn,
              std::vector<Value> args, const std::shared_ptr<LpcObject>& obj) {
    evalCost_ = 0;

    // Tracks real FluffOS's current_object for the duration of this one
    // LPC function activation (see VM.hpp's currentObject() comment).
    // RAII rather than an explicit pop before every return: run() has
    // several return points plus exception unwinding (a rethrown
    // LpcRuntimeError with no active catch frame, or EvalCostError,
    // both propagate straight out of the while loop below), and a
    // destructor is the only pop that reliably covers all of them. See
    // ObjectFrameGuard's own comment for the real-semantics citation.
    ObjectFrameGuard objectFrameGuard(callStack_, objectChangeStack_, obj);

    std::vector<Value> locals(fn.numLocals);
    for (size_t i = 0; i < args.size() && i < locals.size(); ++i) {
        locals[i] = std::move(args[i]);
    }

    std::vector<Value> localStack;
    size_t ip = fn.entryPoint;

    // catch(expr) support (see Ast.hpp's CatchExpr and Bytecode.hpp's
    // PushCatchFrame/PopCatchFrame comments). One stack per run() call
    // (i.e. per LPC function invocation), not a VM-wide member: a
    // function with no catch() of its own has an empty stack here and
    // any error simply propagates out of this call via the rethrow
    // below, exactly like today, which is also how a catch() in a
    // *caller* still traps an error thrown deep inside a *callee* that
    // has no catch() of its own -- the callee's own run() call finds its
    // own catchFrames empty, rethrows, and the resulting C++ exception
    // unwinds straight out of that nested run() call (see OpCode::Call
    // below) back into this function's own try/catch, which does have
    // an active frame. .back()/.pop_back() naturally gives innermost-
    // first behavior for catch() nested within one function body too.
    struct CatchFrame {
        size_t resumeIp;
        size_t stackDepth;
    };
    std::vector<CatchFrame> catchFrames;

    while (ip < program.code.size()) {
      try {
        const Instruction& instr = program.code[ip];
        ++evalCost_;
        if (evalCost_ > 1000000) {
            // Not LpcRuntimeError on purpose -- see EvalCostError's own
            // comment, this must not be catchable by catch().
            throw EvalCostError("eval cost exceeded");
        }

        switch (instr.op) {
            case OpCode::PushConst: {
                if (instr.operand < 0 ||
                    static_cast<size_t>(instr.operand) >= program.stringPool.size()) {
                    throw LpcRuntimeError("PushConst: bad string pool index");
                }
                localStack.emplace_back(Value(program.stringPool[instr.operand]));
                ++ip;
                break;
            }

            case OpCode::PushInt: {
                localStack.emplace_back(Value(static_cast<int64_t>(instr.operand)));
                ++ip;
                break;
            }

            case OpCode::PushFloat: {
                if (instr.operand < 0 ||
                    static_cast<size_t>(instr.operand) >= program.floatPool.size()) {
                    throw LpcRuntimeError("PushFloat: bad float pool index");
                }
                localStack.emplace_back(Value(program.floatPool[instr.operand]));
                ++ip;
                break;
            }

            case OpCode::PushLocal: {
                if (instr.operand < 0 || static_cast<size_t>(instr.operand) >= locals.size()) {
                    throw LpcRuntimeError("PushLocal: bad local slot index");
                }
                localStack.push_back(locals[instr.operand]);
                ++ip;
                break;
            }

            case OpCode::StoreLocal: {
                if (instr.operand < 0 || static_cast<size_t>(instr.operand) >= locals.size()) {
                    throw LpcRuntimeError("StoreLocal: bad local slot index");
                }
                if (localStack.empty()) {
                    throw LpcRuntimeError("StoreLocal: stack underflow");
                }
                locals[instr.operand] = localStack.back();
                localStack.pop_back();
                ++ip;
                break;
            }

            case OpCode::PushObjectVar: {
                auto& vars = obj->variables();
                if (instr.operand < 0 || static_cast<size_t>(instr.operand) >= vars.size()) {
                    throw LpcRuntimeError("PushObjectVar: bad object variable slot index");
                }
                localStack.push_back(vars[instr.operand]);
                ++ip;
                break;
            }

            case OpCode::StoreObjectVar: {
                auto& vars = obj->variables();
                if (instr.operand < 0 || static_cast<size_t>(instr.operand) >= vars.size()) {
                    throw LpcRuntimeError("StoreObjectVar: bad object variable slot index");
                }
                if (localStack.empty()) {
                    throw LpcRuntimeError("StoreObjectVar: stack underflow");
                }
                vars[instr.operand] = localStack.back();
                localStack.pop_back();
                ++ip;
                break;
            }

            case OpCode::Eq:
            case OpCode::Neq: {
                if (localStack.size() < 2) {
                    throw LpcRuntimeError("Eq/Neq: stack underflow");
                }
                Value rhs = localStack.back(); localStack.pop_back();
                Value lhs = localStack.back(); localStack.pop_back();
                bool eq = valuesEqual(lhs, rhs);
                bool result = (instr.op == OpCode::Eq) ? eq : !eq;
                localStack.emplace_back(Value(static_cast<int64_t>(result ? 1 : 0)));
                ++ip;
                break;
            }

            case OpCode::Lt:
            case OpCode::Lte:
            case OpCode::Gt:
            case OpCode::Gte: {
                if (localStack.size() < 2) {
                    throw LpcRuntimeError("comparison: stack underflow");
                }
                Value rhs = localStack.back(); localStack.pop_back();
                Value lhs = localStack.back(); localStack.pop_back();

                double lv, rv;
                if (std::holds_alternative<int64_t>(lhs.data)) lv = static_cast<double>(std::get<int64_t>(lhs.data));
                else if (std::holds_alternative<double>(lhs.data)) lv = std::get<double>(lhs.data);
                else throw LpcRuntimeError("comparison: left operand is not numeric");

                if (std::holds_alternative<int64_t>(rhs.data)) rv = static_cast<double>(std::get<int64_t>(rhs.data));
                else if (std::holds_alternative<double>(rhs.data)) rv = std::get<double>(rhs.data);
                else throw LpcRuntimeError("comparison: right operand is not numeric");

                bool result = false;
                switch (instr.op) {
                    case OpCode::Lt:  result = lv < rv; break;
                    case OpCode::Lte: result = lv <= rv; break;
                    case OpCode::Gt:  result = lv > rv; break;
                    case OpCode::Gte: result = lv >= rv; break;
                    default: break;
                }
                localStack.emplace_back(Value(static_cast<int64_t>(result ? 1 : 0)));
                ++ip;
                break;
            }

            case OpCode::Not: {
                if (localStack.empty()) {
                    throw LpcRuntimeError("Not: stack underflow");
                }
                Value v = localStack.back(); localStack.pop_back();
                localStack.emplace_back(Value(static_cast<int64_t>(isTruthy(v) ? 0 : 1)));
                ++ip;
                break;
            }

            case OpCode::Sub:
            case OpCode::Mul:
            case OpCode::Div:
            case OpCode::Mod: {
                if (localStack.size() < 2) {
                    throw LpcRuntimeError("arithmetic: stack underflow");
                }
                Value rhs = localStack.back(); localStack.pop_back();
                Value lhs = localStack.back(); localStack.pop_back();

                bool eitherDouble = std::holds_alternative<double>(lhs.data) ||
                                     std::holds_alternative<double>(rhs.data);

                double lv, rv;
                if (std::holds_alternative<int64_t>(lhs.data)) lv = static_cast<double>(std::get<int64_t>(lhs.data));
                else if (std::holds_alternative<double>(lhs.data)) lv = std::get<double>(lhs.data);
                else throw LpcRuntimeError("arithmetic: left operand is not numeric");

                if (std::holds_alternative<int64_t>(rhs.data)) rv = static_cast<double>(std::get<int64_t>(rhs.data));
                else if (std::holds_alternative<double>(rhs.data)) rv = std::get<double>(rhs.data);
                else throw LpcRuntimeError("arithmetic: right operand is not numeric");

                if ((instr.op == OpCode::Div || instr.op == OpCode::Mod) && rv == 0.0) {
                    throw LpcRuntimeError(instr.op == OpCode::Div
                        ? "Div: division by zero"
                        : "Mod: modulo by zero");
                }

                double result = 0.0;
                switch (instr.op) {
                    case OpCode::Sub: result = lv - rv; break;
                    case OpCode::Mul: result = lv * rv; break;
                    case OpCode::Div: result = lv / rv; break;
                    case OpCode::Mod: result = std::fmod(lv, rv); break;
                    default: break;
                }

                if (eitherDouble) {
                    localStack.emplace_back(Value(result));
                } else {
                    localStack.emplace_back(Value(static_cast<int64_t>(result)));
                }
                ++ip;
                break;
            }

            case OpCode::BitAnd: {
                if (localStack.size() < 2) {
                    throw LpcRuntimeError("BitAnd: stack underflow");
                }
                Value rhs = localStack.back(); localStack.pop_back();
                Value lhs = localStack.back(); localStack.pop_back();

                if (std::holds_alternative<std::shared_ptr<Array>>(lhs.data) &&
                    std::holds_alternative<std::shared_ptr<Array>>(rhs.data)) {
                    auto leftArr = std::get<std::shared_ptr<Array>>(lhs.data);
                    auto rightArr = std::get<std::shared_ptr<Array>>(rhs.data);
                    auto result = std::make_shared<Array>();
                    // Set intersection: every element of the left array
                    // that also occurs (by LPC value equality) anywhere in
                    // the right array, preserving the left array's order
                    // and duplicate count. This is a simplified stand-in
                    // for FluffOS's intersect_array() (array.c), which
                    // additionally sorts and de-duplicates its result --
                    // not replicated here since nothing this driver
                    // currently runs depends on that exact ordering, only
                    // on membership (e.g. master.c's
                    // "sizeof(privs & ok)").
                    if (leftArr && rightArr) {
                        for (const auto& item : leftArr->items) {
                            bool found = false;
                            for (const auto& other : rightArr->items) {
                                if (valuesEqual(item, other)) { found = true; break; }
                            }
                            if (found) result->items.push_back(item);
                        }
                    }
                    localStack.emplace_back(Value(result));
                } else if (std::holds_alternative<int64_t>(lhs.data) &&
                           std::holds_alternative<int64_t>(rhs.data)) {
                    int64_t result = std::get<int64_t>(lhs.data) & std::get<int64_t>(rhs.data);
                    localStack.emplace_back(Value(result));
                } else {
                    throw LpcRuntimeError("BitAnd: operands must both be ints or both be arrays");
                }
                ++ip;
                break;
            }

            case OpCode::BitOr:
            case OpCode::BitXor: {
                if (localStack.size() < 2) {
                    throw LpcRuntimeError("BitOr/BitXor: stack underflow");
                }
                Value rhs = localStack.back(); localStack.pop_back();
                Value lhs = localStack.back(); localStack.pop_back();

                if (!std::holds_alternative<int64_t>(lhs.data) ||
                    !std::holds_alternative<int64_t>(rhs.data)) {
                    throw LpcRuntimeError(
                        std::string(instr.op == OpCode::BitOr ? "BitOr" : "BitXor") +
                        ": operands must both be ints (array union is not implemented this slice)");
                }
                int64_t result = (instr.op == OpCode::BitOr)
                    ? (std::get<int64_t>(lhs.data) | std::get<int64_t>(rhs.data))
                    : (std::get<int64_t>(lhs.data) ^ std::get<int64_t>(rhs.data));
                localStack.emplace_back(Value(result));
                ++ip;
                break;
            }

            case OpCode::ForeachKeys: {
                if (localStack.empty()) {
                    throw LpcRuntimeError("ForeachKeys: stack underflow");
                }
                Value v = localStack.back(); localStack.pop_back();
                if (std::holds_alternative<std::shared_ptr<Array>>(v.data)) {
                    localStack.push_back(v);
                } else if (auto* map = std::get_if<std::shared_ptr<Mapping>>(&v.data)) {
                    auto keysArr = std::make_shared<Array>();
                    if (*map) {
                        for (const auto& entry : (*map)->entries) {
                            keysArr->items.push_back(entry.first);
                        }
                    }
                    localStack.emplace_back(Value(keysArr));
                } else {
                    throw LpcRuntimeError("foreach: collection must be an array or mapping");
                }
                ++ip;
                break;
            }

            case OpCode::Jump: {
                if (instr.operand < 0 || static_cast<size_t>(instr.operand) > program.code.size()) {
                    throw LpcRuntimeError("Jump: bad target");
                }
                ip = static_cast<size_t>(instr.operand);
                break;
            }

            case OpCode::JumpIfFalse: {
                if (localStack.empty()) {
                    throw LpcRuntimeError("JumpIfFalse: stack underflow");
                }
                Value cond = localStack.back();
                localStack.pop_back();
                if (!isTruthy(cond)) {
                    if (instr.operand < 0 || static_cast<size_t>(instr.operand) > program.code.size()) {
                        throw LpcRuntimeError("JumpIfFalse: bad target");
                    }
                    ip = static_cast<size_t>(instr.operand);
                } else {
                    ++ip;
                }
                break;
            }

            case OpCode::PushCatchFrame: {
                if (instr.operand < 0 || static_cast<size_t>(instr.operand) > program.code.size()) {
                    throw LpcRuntimeError("PushCatchFrame: bad resume target");
                }
                catchFrames.push_back(CatchFrame{
                    static_cast<size_t>(instr.operand), localStack.size()});
                ++ip;
                break;
            }

            case OpCode::PopCatchFrame: {
                // Only reached on normal completion of the guarded
                // region -- see this opcode's own Bytecode.hpp comment.
                // CodeGen always emits a matching PushCatchFrame before
                // any PopCatchFrame, so an empty stack here would be a
                // codegen bug, not a real runtime condition to recover
                // from.
                if (catchFrames.empty()) {
                    throw LpcRuntimeError("PopCatchFrame: no active catch frame");
                }
                catchFrames.pop_back();
                localStack.emplace_back(Value(static_cast<int64_t>(0)));
                ++ip;
                break;
            }

            case OpCode::Add: {
                if (localStack.size() < 2) {
                    throw LpcRuntimeError("Add: stack underflow");
                }
                Value rhs = localStack.back(); localStack.pop_back();
                Value lhs = localStack.back(); localStack.pop_back();

                if (std::holds_alternative<std::string>(lhs.data) &&
                    std::holds_alternative<std::string>(rhs.data)) {
                    localStack.emplace_back(
                        Value(std::get<std::string>(lhs.data) + std::get<std::string>(rhs.data)));
                } else if (std::holds_alternative<std::shared_ptr<Array>>(lhs.data) &&
                           std::holds_alternative<std::shared_ptr<Array>>(rhs.data)) {
                    auto leftArr = std::get<std::shared_ptr<Array>>(lhs.data);
                    auto rightArr = std::get<std::shared_ptr<Array>>(rhs.data);
                    auto result = std::make_shared<Array>();
                    if (leftArr) {
                        result->items.insert(result->items.end(), leftArr->items.begin(), leftArr->items.end());
                    }
                    if (rightArr) {
                        result->items.insert(result->items.end(), rightArr->items.begin(), rightArr->items.end());
                    }
                    localStack.emplace_back(Value(result));
                } else if ((std::holds_alternative<int64_t>(lhs.data) || std::holds_alternative<double>(lhs.data)) &&
                           (std::holds_alternative<int64_t>(rhs.data) || std::holds_alternative<double>(rhs.data))) {
                    bool eitherDouble = std::holds_alternative<double>(lhs.data) ||
                                        std::holds_alternative<double>(rhs.data);
                    double lv = std::holds_alternative<int64_t>(lhs.data)
                                    ? static_cast<double>(std::get<int64_t>(lhs.data))
                                    : std::get<double>(lhs.data);
                    double rv = std::holds_alternative<int64_t>(rhs.data)
                                    ? static_cast<double>(std::get<int64_t>(rhs.data))
                                    : std::get<double>(rhs.data);
                    if (eitherDouble) {
                        localStack.emplace_back(Value(lv + rv));
                    } else {
                        localStack.emplace_back(Value(static_cast<int64_t>(lv + rv)));
                    }
                } else {
                    throw LpcRuntimeError("Add: unsupported operand types");
                }
                ++ip;
                break;
            }

            case OpCode::MakeArray: {
                int argc = instr.argCount;
                if (argc < 0 || static_cast<size_t>(argc) > localStack.size()) {
                    throw LpcRuntimeError("MakeArray: bad arg count");
                }
                auto arr = std::make_shared<Array>();
                arr->items.assign(localStack.end() - argc, localStack.end());
                localStack.erase(localStack.end() - argc, localStack.end());
                localStack.emplace_back(Value(arr));
                ++ip;
                break;
            }

            case OpCode::MakeMapping: {
                int entryCount = instr.argCount;
                if (entryCount < 0 ||
                    static_cast<size_t>(entryCount) * 2 > localStack.size()) {
                    throw LpcRuntimeError("MakeMapping: bad arg count");
                }
                size_t total = static_cast<size_t>(entryCount) * 2;
                size_t base = localStack.size() - total;
                auto map = std::make_shared<Mapping>();
                for (int i = 0; i < entryCount; ++i) {
                    Value key = localStack[base + static_cast<size_t>(i) * 2];
                    Value value = localStack[base + static_cast<size_t>(i) * 2 + 1];
                    map->entries.emplace_back(std::move(key), std::move(value));
                }
                localStack.erase(localStack.end() - static_cast<long>(total), localStack.end());
                localStack.emplace_back(Value(map));
                ++ip;
                break;
            }

            case OpCode::Index: {
                if (localStack.size() < 2) {
                    throw LpcRuntimeError("Index: stack underflow");
                }
                Value indexVal = localStack.back(); localStack.pop_back();
                Value targetVal = localStack.back(); localStack.pop_back();
                // See CodeGen.cpp's own comment: argCount is repurposed
                // as a "from the end" flags bitmask for this opcode,
                // bit 0 for the single index here.
                bool indexFromEnd = (instr.argCount & 0x1) != 0;

                if (auto* arr = std::get_if<std::shared_ptr<Array>>(&targetVal.data)) {
                    if (!*arr) {
                        throw LpcRuntimeError("Index: target array is null");
                    }
                    if (!std::holds_alternative<int64_t>(indexVal.data)) {
                        throw LpcRuntimeError("Index: array index must be an integer");
                    }
                    int64_t i = std::get<int64_t>(indexVal.data);
                    // real eoperators.c's f_index()/reverse indexing:
                    // "<N" means index (length - N), computed against
                    // the target's own runtime length.
                    if (indexFromEnd) i = static_cast<int64_t>((*arr)->items.size()) - i;
                    if (i < 0 || static_cast<size_t>(i) >= (*arr)->items.size()) {
                        throw LpcRuntimeError("Index: array index out of bounds");
                    }
                    localStack.push_back((*arr)->items[static_cast<size_t>(i)]);
                } else if (auto* map = std::get_if<std::shared_ptr<Mapping>>(&targetVal.data)) {
                    if (!*map) {
                        throw LpcRuntimeError("Index: target mapping is null");
                    }
                    bool hit = false;
                    Value found;
                    for (const auto& entry : (*map)->entries) {
                        if (valuesEqual(entry.first, indexVal)) {
                            found = entry.second;
                            hit = true;
                            break;
                        }
                    }
                    localStack.push_back(hit ? found : Value{});
                } else if (auto* str = std::get_if<std::string>(&targetVal.data)) {
                    if (!std::holds_alternative<int64_t>(indexVal.data)) {
                        throw LpcRuntimeError("Index: string index must be an integer");
                    }
                    int64_t i = std::get<int64_t>(indexVal.data);
                    if (indexFromEnd) i = static_cast<int64_t>(str->size()) - i;
                    if (i < 0 || static_cast<size_t>(i) >= str->size()) {
                        throw LpcRuntimeError("Index: string index out of bounds");
                    }
                    unsigned char ch = static_cast<unsigned char>((*str)[static_cast<size_t>(i)]);
                    localStack.push_back(Value(static_cast<int64_t>(ch)));
                } else {
                    throw LpcRuntimeError("Index: target is not an array, mapping, or string");
                }
                ++ip;
                break;
            }

            case OpCode::IndexAssign: {
                if (localStack.size() < 3) {
                    throw LpcRuntimeError("IndexAssign: stack underflow");
                }
                Value value = localStack.back(); localStack.pop_back();
                Value indexVal = localStack.back(); localStack.pop_back();
                Value targetVal = localStack.back(); localStack.pop_back();

                if (auto* arr = std::get_if<std::shared_ptr<Array>>(&targetVal.data)) {
                    if (!*arr) {
                        throw LpcRuntimeError("IndexAssign: target array is null");
                    }
                    if (!std::holds_alternative<int64_t>(indexVal.data)) {
                        throw LpcRuntimeError("IndexAssign: array index must be an integer");
                    }
                    int64_t i = std::get<int64_t>(indexVal.data);
                    if (i < 0 || static_cast<size_t>(i) >= (*arr)->items.size()) {
                        throw LpcRuntimeError("IndexAssign: array index out of bounds");
                    }
                    (*arr)->items[static_cast<size_t>(i)] = value;
                } else if (auto* map = std::get_if<std::shared_ptr<Mapping>>(&targetVal.data)) {
                    if (!*map) {
                        throw LpcRuntimeError("IndexAssign: target mapping is null");
                    }
                    bool found = false;
                    for (auto& entry : (*map)->entries) {
                        if (valuesEqual(entry.first, indexVal)) {
                            entry.second = value;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        (*map)->entries.emplace_back(indexVal, value);
                    }
                } else {
                    throw LpcRuntimeError("IndexAssign: target is not an array or mapping");
                }
                ++ip;
                break;
            }

            case OpCode::RangeIndex: {
                if (localStack.size() < 3) {
                    throw LpcRuntimeError("RangeIndex: stack underflow");
                }
                Value endVal = localStack.back(); localStack.pop_back();
                Value startVal = localStack.back(); localStack.pop_back();
                Value targetVal = localStack.back(); localStack.pop_back();

                if (!std::holds_alternative<int64_t>(startVal.data)) {
                    throw LpcRuntimeError("RangeIndex: start index must be an integer");
                }
                if (!std::holds_alternative<int64_t>(endVal.data)) {
                    throw LpcRuntimeError("RangeIndex: end index must be an integer");
                }
                int64_t rawStart = std::get<int64_t>(startVal.data);
                int64_t rawEnd = std::get<int64_t>(endVal.data);
                // See CodeGen.cpp's own comment: argCount is repurposed
                // as a "from the end" flags bitmask for this opcode,
                // bit 0 for the start bound, bit 1 for the end bound.
                bool startFromEnd = (instr.argCount & 0x1) != 0;
                bool endFromEnd = (instr.argCount & 0x2) != 0;

                // A literal negative start with no "<" prefix is still
                // rejected exactly as before "from the end" indexing
                // existed (this driver's own pre-existing behavior, not
                // real modern FluffOS's -- real eoperators.c's
                // OLD_RANGE_BEHAVIOR-gated auto-wrap is deprecated
                // there too, "use arr[x..<y]" instead). Only a start
                // that is negative *after* resolving "<N" against the
                // target's own length clamps to 0 instead of throwing
                // (real eoperators.c: "if (from < 0) from = 0;") -- a
                // legitimate outcome for "<N" when N is at least the
                // target's own length, not a caller mistake the way a
                // bare negative literal is.
                if (!startFromEnd && rawStart < 0) {
                    throw LpcRuntimeError("RangeIndex: start index must be non-negative");
                }

                // real eoperators.c's f_range(): "if (code & 0x10) from
                // = len - from;" / "if (code & 0x01) to = len - to;",
                // each resolved against the target's own runtime length.
                auto resolveBounds = [&](int64_t len) {
                    int64_t start = startFromEnd ? (len - rawStart) : rawStart;
                    int64_t end = endFromEnd ? (len - rawEnd) : rawEnd;
                    if (start < 0) start = 0;
                    return std::pair<int64_t, int64_t>(start, end);
                };

                if (auto* str = std::get_if<std::string>(&targetVal.data)) {
                    int64_t len = static_cast<int64_t>(str->size());
                    auto [start, end] = resolveBounds(len);
                    int64_t clampedEnd = std::min(end, len - 1);
                    if (start > clampedEnd) {
                        localStack.push_back(Value(std::string()));
                    } else {
                        localStack.push_back(Value(str->substr(
                            static_cast<size_t>(start),
                            static_cast<size_t>(clampedEnd - start + 1))));
                    }
                } else if (auto* arr = std::get_if<std::shared_ptr<Array>>(&targetVal.data)) {
                    if (!*arr) {
                        throw LpcRuntimeError("RangeIndex: target array is null");
                    }
                    int64_t len = static_cast<int64_t>((*arr)->items.size());
                    auto [start, end] = resolveBounds(len);
                    int64_t clampedEnd = std::min(end, len - 1);
                    auto result = std::make_shared<Array>();
                    for (int64_t i = start; i <= clampedEnd; ++i) {
                        result->items.push_back((*arr)->items[static_cast<size_t>(i)]);
                    }
                    localStack.push_back(Value(result));
                } else {
                    throw LpcRuntimeError("RangeIndex: target is not an array or string");
                }
                ++ip;
                break;
            }

            case OpCode::Call: {
                if (instr.operand < 0 ||
                    static_cast<size_t>(instr.operand) >= program.stringPool.size()) {
                    throw LpcRuntimeError("Call: bad function name index");
                }
                const std::string& funcName = program.stringPool[instr.operand];

                int argc = instr.argCount;
                if (argc < 0 || static_cast<size_t>(argc) > localStack.size()) {
                    throw LpcRuntimeError("Call: bad arg count for " + funcName);
                }
                std::vector<Value> callArgs(localStack.end() - argc, localStack.end());
                localStack.erase(localStack.end() - argc, localStack.end());

                FunctionLookupResult found = findFunctionInChain(program, funcName);
                if (found.program) {
                    Value result = run(*found.program, *found.fn, std::move(callArgs), obj);
                    localStack.push_back(std::move(result));
                    ++ip;
                    break;
                }

                // Tier 3: the configured simul_efun object, matching real
                // FluffOS's own resolution order (local/inherited, then
                // simul_efun, then the real efun table -- see lex.c's
                // F_SIMUL_EFUN handling and function.c's
                // call_simul_efun()). Unlike the local/inherited case
                // above, this runs against the simul_efun object's own
                // variables() (via "simulEfun" as the obj argument, not
                // the caller's obj) -- a simul_efun function's object
                // variables belong to the simul_efun object itself, this
                // is not the same "shared flat variable space" situation
                // inherit deliberately sets up.
                auto simulEfun = objects_.simulEfunObject();
                if (simulEfun) {
                    FunctionLookupResult simulFound =
                        findFunctionInChain(simulEfun->program(), funcName);
                    if (simulFound.program) {
                        Value result = run(*simulFound.program, *simulFound.fn,
                                            std::move(callArgs), simulEfun);
                        localStack.push_back(std::move(result));
                        ++ip;
                        break;
                    }
                }

                if (EfunTable::instance().exists(funcName)) {
                    Value result = EfunTable::instance().call(funcName, *this, callArgs);
                    localStack.push_back(std::move(result));
                } else {
                    throw LpcRuntimeError("undefined function or efun: " + funcName);
                }
                ++ip;
                break;
            }

            case OpCode::CallParent: {
                if (instr.operand < 0 ||
                    static_cast<size_t>(instr.operand) >= program.stringPool.size()) {
                    throw LpcRuntimeError("CallParent: bad function name index");
                }
                const std::string& funcName = program.stringPool[instr.operand];

                if (ip + 1 >= program.code.size() ||
                    program.code[ip + 1].op != OpCode::CallParentQualifierSlot) {
                    throw LpcRuntimeError("CallParent: missing qualifier data instruction");
                }
                int32_t qualifierIdx = program.code[ip + 1].operand;
                std::string qualifierStorage;
                const std::string* qualifier = nullptr;
                if (qualifierIdx >= 0) {
                    if (static_cast<size_t>(qualifierIdx) >= program.stringPool.size()) {
                        throw LpcRuntimeError("CallParent: bad qualifier string index");
                    }
                    qualifierStorage = program.stringPool[qualifierIdx];
                    qualifier = &qualifierStorage;
                }

                int argc = instr.argCount;
                if (argc < 0 || static_cast<size_t>(argc) > localStack.size()) {
                    throw LpcRuntimeError("CallParent: bad arg count for " + funcName);
                }
                std::vector<Value> callArgs(localStack.end() - argc, localStack.end());
                localStack.erase(localStack.end() - argc, localStack.end());

                FunctionLookupResult found = findParentFunction(program, funcName, qualifier);
                if (!found.program) {
                    throw LpcRuntimeError(
                        (qualifier ? (*qualifier + "::") : std::string("::")) + funcName +
                        "(): undefined function in inherited program");
                }
                Value result = run(*found.program, *found.fn, std::move(callArgs), obj);
                localStack.push_back(std::move(result));
                ip += 2; // past CallParent and its CallParentQualifierSlot data instruction
                break;
            }

            case OpCode::PushClosure: {
                if (instr.operand < 0 ||
                    static_cast<size_t>(instr.operand) >= program.stringPool.size()) {
                    throw LpcRuntimeError("PushClosure: bad function name index");
                }
                const std::string& funcName = program.stringPool[instr.operand];

                int argc = instr.argCount;
                if (argc < 0 || static_cast<size_t>(argc) > localStack.size()) {
                    throw LpcRuntimeError("PushClosure: bad bound-arg count for " + funcName);
                }
                auto closure = std::make_shared<Closure>();
                closure->owner = obj; // weak_ptr from shared_ptr, real current_object at bind time
                closure->functionName = funcName;
                closure->boundArgs.assign(localStack.end() - argc, localStack.end());
                localStack.erase(localStack.end() - argc, localStack.end());

                localStack.push_back(Value(closure));
                ++ip;
                break;
            }

            case OpCode::Sscanf: {
                int n = instr.operand;
                if (n < 0) {
                    throw LpcRuntimeError("Sscanf: bad var count");
                }
                if (localStack.size() < 2) {
                    throw LpcRuntimeError("Sscanf: stack underflow");
                }
                Value formatVal = localStack.back(); localStack.pop_back();
                Value targetVal = localStack.back(); localStack.pop_back();

                if (!std::holds_alternative<std::string>(targetVal.data) ||
                    !std::holds_alternative<std::string>(formatVal.data)) {
                    throw LpcRuntimeError("sscanf: first two arguments must be strings");
                }
                if (ip + 1 + static_cast<size_t>(n) > program.code.size()) {
                    throw LpcRuntimeError("Sscanf: truncated var-slot table");
                }

                SscanfOutcome outcome = runSscanf(std::get<std::string>(targetVal.data),
                                                   std::get<std::string>(formatVal.data),
                                                   static_cast<size_t>(n));

                for (size_t i = 0; i < outcome.assigned.size(); ++i) {
                    const Instruction& slotSpec = program.code[ip + 1 + i];
                    bool isObjectVar = slotSpec.argCount != 0;
                    int32_t slot = slotSpec.operand;
                    if (isObjectVar) {
                        auto& vars = obj->variables();
                        if (slot < 0 || static_cast<size_t>(slot) >= vars.size()) {
                            throw LpcRuntimeError("Sscanf: bad object variable slot index");
                        }
                        vars[static_cast<size_t>(slot)] = outcome.assigned[i];
                    } else {
                        if (slot < 0 || static_cast<size_t>(slot) >= locals.size()) {
                            throw LpcRuntimeError("Sscanf: bad local slot index");
                        }
                        locals[static_cast<size_t>(slot)] = outcome.assigned[i];
                    }
                }

                localStack.push_back(Value(outcome.matchCount));
                ip += 1 + static_cast<size_t>(n);
                break;
            }

            case OpCode::CallEfun: {
                if (instr.operand < 0 ||
                    static_cast<size_t>(instr.operand) >= program.stringPool.size()) {
                    throw LpcRuntimeError("CallEfun: bad efun name index");
                }
                const std::string& efunName = program.stringPool[instr.operand];

                int argc = instr.argCount;
                if (argc < 0 || static_cast<size_t>(argc) > localStack.size()) {
                    throw LpcRuntimeError("CallEfun: bad arg count for " + efunName);
                }

                std::vector<Value> efunArgs(
                    localStack.end() - argc, localStack.end());
                localStack.erase(localStack.end() - argc, localStack.end());

                Value result = EfunTable::instance().call(efunName, *this, efunArgs);
                localStack.push_back(std::move(result));
                ++ip;
                break;
            }

            case OpCode::Pop: {
                if (!localStack.empty()) localStack.pop_back();
                ++ip;
                break;
            }

            case OpCode::Dup: {
                if (localStack.empty()) {
                    throw LpcRuntimeError("Dup: stack underflow");
                }
                localStack.push_back(localStack.back());
                ++ip;
                break;
            }

            case OpCode::Return: {
                if (localStack.empty()) return Value{};
                return localStack.back();
            }

            case OpCode::Halt:
                return Value{};

            default:
                throw NotImplementedError(
                    "VM::run opcode " + std::to_string(static_cast<int>(instr.op)));
        }
      } catch (const LpcRuntimeError& e) {
        // No active catch() anywhere in this call: behave exactly as
        // before catch() existed, propagate to whatever wraps this
        // run() call (a caller's own active catch frame if this was a
        // nested call, or the outermost ObjectManager/Server.cpp safety
        // net if not -- see run()'s own comment above catchFrames).
        if (catchFrames.empty()) throw;

        // Unwind to the innermost still-active catch() (LIFO, matching
        // real FluffOS's own nested do_catch() call stack -- see
        // catchFrames' own comment): discard whatever the guarded
        // expression had partially pushed (real LPC's own stack-pointer
        // restoration on longjmp, confirmed against interpret.c's
        // do_catch()/restore_context()), push the error message as
        // catch(expr)'s result, then resume right after the whole catch
        // expression, exactly where PopCatchFrame's own success path
        // would also have landed.
        CatchFrame frame = catchFrames.back();
        catchFrames.pop_back();
        localStack.resize(frame.stackDepth);
        localStack.emplace_back(Value(std::string(e.what())));
        ip = frame.resumeIp;
      }
    }

    return Value{};
}

} // namespace lpcdriver
