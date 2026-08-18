#include "amlp/vm/Value.hpp"

namespace amlp {

bool isTruthy(const Value& v) {
    if (std::holds_alternative<std::monostate>(v.data)) return false;
    // DGD's nil (ROADMAP.md row 1.10's minimal real piece): real DGD's
    // own VAL_TRUE(v) macro (data.h) -- "(v)->number != 0 || (v)->type >
    // T_FLOAT || ..." -- is false for a nil value (type T_NIL == 0,
    // number == 0, neither disjunct holds), i.e. nil is falsy.
    if (std::holds_alternative<Nil>(v.data)) return false;
    if (auto* i = std::get_if<int64_t>(&v.data)) return *i != 0;
    if (auto* d = std::get_if<double>(&v.data)) return *d != 0.0;
    if (auto* s = std::get_if<std::string>(&v.data)) return !s->empty();
    if (auto* obj = std::get_if<std::shared_ptr<LpcObject>>(&v.data)) return static_cast<bool>(*obj);
    if (auto* arr = std::get_if<std::shared_ptr<Array>>(&v.data)) return static_cast<bool>(*arr);
    if (auto* map = std::get_if<std::shared_ptr<Mapping>>(&v.data)) return static_cast<bool>(*map);
    if (auto* fn = std::get_if<std::shared_ptr<Closure>>(&v.data)) return static_cast<bool>(*fn);
    return false;
}

bool valuesEqual(const Value& a, const Value& b) {
    if (a.data.index() != b.data.index()) return false;
    // DGD's nil (ROADMAP.md row 1.10's minimal real piece): both sides
    // already confirmed to hold Nil by the index check above, and Nil
    // carries no data to compare, so any two nils are equal -- matching
    // real DGD's own VAL_NIL(v) macro ("(v)->type == nil.type &&
    // (v)->number == 0", always true for a value already known to be
    // nil-typed). This also means nil is never equal to plain int 0
    // under this driver's std::monostate/int64_t handling below (the
    // index check above already separates them), matching real DGD's
    // own strict-typechecking behavior where nil.type (T_NIL) and
    // int 0's type (T_INT) are different tags -- see data.cpp's own
    // "nil.type = (stricttc) ? T_NIL : T_INT;" comment in Value.hpp.
    if (std::holds_alternative<Nil>(a.data)) return true;
    if (auto* ai = std::get_if<int64_t>(&a.data)) return *ai == std::get<int64_t>(b.data);
    if (auto* ad = std::get_if<double>(&a.data)) return *ad == std::get<double>(b.data);
    if (auto* as = std::get_if<std::string>(&a.data)) return *as == std::get<std::string>(b.data);
    if (auto* ao = std::get_if<std::shared_ptr<LpcObject>>(&a.data)) return *ao == std::get<std::shared_ptr<LpcObject>>(b.data);
    if (auto* af = std::get_if<std::shared_ptr<Closure>>(&a.data)) return *af == std::get<std::shared_ptr<Closure>>(b.data);
    if (std::holds_alternative<std::monostate>(a.data)) return true;
    return false;
}

} // namespace amlp
