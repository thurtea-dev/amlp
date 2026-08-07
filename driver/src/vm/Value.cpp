#include "lpcdriver/vm/Value.hpp"

namespace lpcdriver {

bool isTruthy(const Value& v) {
    if (std::holds_alternative<std::monostate>(v.data)) return false;
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
    if (auto* ai = std::get_if<int64_t>(&a.data)) return *ai == std::get<int64_t>(b.data);
    if (auto* ad = std::get_if<double>(&a.data)) return *ad == std::get<double>(b.data);
    if (auto* as = std::get_if<std::string>(&a.data)) return *as == std::get<std::string>(b.data);
    if (auto* ao = std::get_if<std::shared_ptr<LpcObject>>(&a.data)) return *ao == std::get<std::shared_ptr<LpcObject>>(b.data);
    if (auto* af = std::get_if<std::shared_ptr<Closure>>(&a.data)) return *af == std::get<std::shared_ptr<Closure>>(b.data);
    if (std::holds_alternative<std::monostate>(a.data)) return true;
    return false;
}

} // namespace lpcdriver
