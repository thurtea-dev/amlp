#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace lpcdriver {

class LpcObject;
struct Array;
struct Mapping;
struct Closure;

using ValueVariant = std::variant<
    std::monostate,
    int64_t,
    double,
    std::string,
    std::shared_ptr<LpcObject>,
    std::shared_ptr<Array>,
    std::shared_ptr<Mapping>,
    std::shared_ptr<Closure>
>;

struct Value {
    ValueVariant data;

    Value() = default;
    template <typename T>
    Value(T v) : data(std::move(v)) {}

    bool isVoid() const { return std::holds_alternative<std::monostate>(data); }
};

bool isTruthy(const Value& v);
bool valuesEqual(const Value& a, const Value& b);

struct Array {
    std::vector<Value> items;
};

struct Mapping {
    std::vector<std::pair<Value, Value>> entries;
};

// A real LPC "function" value -- the "(: name, bound_args... :)" closure
// literal (real FluffOS's funptr_t, see function.h: funptr_hdr_t's
// "owner"/"args" fields plus a type-specific union for FP_EFUN/FP_LOCAL/
// FP_SIMUL/FP_FUNCTIONAL). This driver only implements the bare-
// identifier-name form real usage across this mudlib actually needs
// (confirmed by grepping every "(:" call site, not guessed): an efun,
// local function, or simul_efun referenced by its bare name, with zero
// or more already-bound arguments. Real FluffOS resolves which of
// those four kinds a name is *at construction time* (baked into the
// funptr_t's own type tag via lex.c's identifier classification); this
// driver instead stores just the bare name and re-resolves it lazily at
// *call* time via the same tiered lookup (local/inherited -> simul_efun
// -> efun table) OpCode::Call already uses. This is a deliberate
// simplification, not an oversight: every closure actually reachable in
// this mudlib is constructed and called within the same short-lived
// scope (never persisted across a redefinition or module reload), so
// the two approaches are behaviorally identical for anything this
// driver runs -- see VM::callClosure()'s own comment. Object-bound
// closures ("(: obj, \"func\" :)"), $1/$(name)-placeholder inline
// lambdas, and bare string-constant closures ("(: \"literal\" :)") are
// real LPC forms too but are not implemented: none of them are used
// anywhere on this driver's current boot/login/account-creation path
// (see STATUS.md's closure recon notes for the full count).
struct Closure {
    // The object active when this closure literal was constructed
    // (real funptr_hdr_t::owner, "current_object" at bind time).
    // weak_ptr: a closure outliving its owner's destruction must fail
    // at call time, not keep the object alive artificially, matching
    // real call_function_pointer()'s own "Owner of function pointer is
    // destructed" check.
    std::weak_ptr<LpcObject> owner;
    // The bare name written in the literal.
    std::string functionName;
    // Arguments already bound at construction time ("(: file_size, p
    // :)"'s "p"). Placed *before* any additional call-time arguments
    // when invoked -- confirmed against real FluffOS's own
    // merge_arg_lists() (function.c): bound args are shifted in ahead
    // of whatever is already on the stack from the call site, not
    // appended after.
    std::vector<Value> boundArgs;
};

} // namespace lpcdriver
