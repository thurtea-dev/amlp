#pragma once
#include <memory>
#include <vector>

namespace lpcdriver {

class LpcObject;

// Real FluffOS's obj_list (a global intrusive doubly-linked list every
// loaded object -- blueprint or clone -- is threaded onto at construction
// and unthreaded from at destruct_object()) reduced to the same "global
// weak_ptr registry" shape LivingNameRegistry/InteractiveRegistry already
// use elsewhere in this driver. Backs objects() and the core (simul_efun-
// shadowed-for-bare-calls, but real via efun::livings()) livings() efun --
// see EfunTable.cpp's own registration comments and src/efun/instruct.md's
// corrected Tier 1 table for why both needed this and neither had it
// before.
//
// Populated by ObjectManager::loadObject()/cloneObject() right after each
// constructs its LpcObject (loadVirtualObject() needs no separate call
// site: the object it returns was itself already created via one of those
// two paths, from inside the master's own compile_object() apply).
// Cleared by ObjectManager::destructObject(). weak_ptr entries, the same
// tolerance every other registry in this driver already documents: a
// stale (destructed-and-dropped) entry found during all() is simply
// skipped, never surfaced as a live object.
class LiveObjectRegistry {
public:
    static void add(const std::shared_ptr<LpcObject>& obj);
    static void remove(const std::shared_ptr<LpcObject>& obj);
    static std::vector<std::shared_ptr<LpcObject>> all();
};

} // namespace lpcdriver
