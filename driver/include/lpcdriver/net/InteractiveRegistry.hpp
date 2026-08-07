#pragma once
#include <memory>
#include <vector>

namespace lpcdriver {

class LpcObject;

// Every object currently bound to a live connection -- real FluffOS's
// all_users[] array (comm.h/comm.c), which users() (array.c's
// f_users()/livings) and find_player() (otable.c) both search. This
// driver has no single place that already tracks "every connection"
// reachable from an efun (EfunTable lambdas only get a VM&, and VM has
// no Server reference -- constructing one would be circular, Server
// already depends on VM), so this is a small dedicated global registry,
// the same pattern OutputContext already uses for "the connection
// driving the currently executing call". Connection::attach()/close()
// keep it in sync; nothing else needs to.
//
// weak_ptr entries: a destructed/closed connection's object must not be
// kept alive by this registry alone, and a stale weak_ptr found during
// all() is simply skipped rather than surfaced as a live user.
class InteractiveRegistry {
public:
    static void add(const std::shared_ptr<LpcObject>& obj);
    static void remove(const std::shared_ptr<LpcObject>& obj);
    static std::vector<std::shared_ptr<LpcObject>> all();
};

} // namespace lpcdriver
