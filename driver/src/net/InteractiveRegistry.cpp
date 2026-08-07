#include "lpcdriver/net/InteractiveRegistry.hpp"
#include "lpcdriver/object/LpcObject.hpp"
#include <algorithm>

namespace lpcdriver {

namespace {
std::vector<std::weak_ptr<LpcObject>> g_registry;
}

void InteractiveRegistry::add(const std::shared_ptr<LpcObject>& obj) {
    if (!obj) return;
    g_registry.push_back(obj);
}

void InteractiveRegistry::remove(const std::shared_ptr<LpcObject>& obj) {
    if (!obj) return;
    g_registry.erase(
        std::remove_if(g_registry.begin(), g_registry.end(),
                        [&obj](const std::weak_ptr<LpcObject>& w) {
                            auto locked = w.lock();
                            return !locked || locked == obj;
                        }),
        g_registry.end());
}

std::vector<std::shared_ptr<LpcObject>> InteractiveRegistry::all() {
    std::vector<std::shared_ptr<LpcObject>> result;
    result.reserve(g_registry.size());
    for (auto& w : g_registry) {
        if (auto locked = w.lock()) result.push_back(std::move(locked));
    }
    return result;
}

} // namespace lpcdriver
