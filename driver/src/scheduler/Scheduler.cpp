#include "lpcdriver/scheduler/Scheduler.hpp"
#include "lpcdriver/net/Server.hpp"
#include "lpcdriver/object/LpcObject.hpp"
#include "lpcdriver/vm/VM.hpp"
#include <algorithm>
#include <atomic>
#include <iostream>
#include <thread>

namespace lpcdriver {

namespace {
std::atomic<bool> g_shutdownRequested{false};
}

Scheduler::Scheduler(VM& vm)
    : vm_(vm), lastHeartbeat_(std::chrono::steady_clock::now()) {}

void Scheduler::requestShutdown() {
    g_shutdownRequested.store(true);
}

void Scheduler::run(Server& server, int maxIterations) {
    g_shutdownRequested.store(false);
    int iterations = 0;

    for (;;) {
        server.pollOnce();

        // Real backend.c only calls call_heart_beat() once real elapsed
        // time crosses a HEARTBEAT_INTERVAL boundary ("if(!(current_time %
        // HEARTBEAT_INTERVAL)) call_heart_beat();" in call_out.c's own main
        // loop) -- reproduced here as a plain elapsed-wall-time gate around
        // tickHeartbeats() rather than inside it, so tickHeartbeats() itself
        // stays a deterministic, directly-testable "process one cycle"
        // function (see its own header comment).
        auto now = std::chrono::steady_clock::now();
        if (now - lastHeartbeat_ >= kHeartbeatCycle) {
            lastHeartbeat_ = now;
            tickHeartbeats();
        }
        tickCallOuts();

        ++iterations;
        if (maxIterations > 0 && iterations >= maxIterations) break;
        if (g_shutdownRequested.load()) break;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int64_t Scheduler::newCallOutHandle() {
    // Real new_call_out() encodes the handle from the ring-buffer slot plus
    // a per-process "unique" counter (call_out.c: "tm += CALLOUT_CYCLE_SIZE
    // * ++unique; cop->handle = tm;") -- purely so it can find the entry
    // again in O(1) by decoding the slot back out of the handle. This
    // driver has no equivalent ring buffer (callOuts_ is a plain vector,
    // see the class's own header comment), so a bare monotonic counter is
    // an equivalent, simpler encoding: still unique, still stable, and
    // nothing in this mudlib inspects a handle's bit structure -- every
    // real call site only ever stores it and later compares it back
    // (cmds/mortal/_trade.c's own "tid = call_out(...); ...;
    // remove_call_out(tid)") or checks it for truthiness/non- -1.
    return nextHandle_++;
}

int64_t Scheduler::addCallOut(CallOutEntry entry) {
    entry.handle = newCallOutHandle();
    int64_t handle = entry.handle;
    callOuts_.push_back(std::move(entry));
    return handle;
}

namespace {
// Real time_left() (call_out.c): remaining whole seconds until an entry
// would have fired. This driver stores an absolute dueAt time_point rather
// than real call_out.c's delta-linked-list-of-slots representation, so the
// equivalent is just dueAt - now, clamped to 0 (a due-but-not-yet-fired
// entry reports 0 remaining, not negative).
int64_t remainingSeconds(const std::chrono::steady_clock::time_point& dueAt) {
    auto now = std::chrono::steady_clock::now();
    if (dueAt <= now) return 0;
    return std::chrono::duration_cast<std::chrono::seconds>(dueAt - now).count();
}
} // namespace

int64_t Scheduler::removeCallOutByName(const std::shared_ptr<LpcObject>& owner, const std::string& function) {
    if (!owner) return -1;
    // Real remove_call_out(object_t*, const char*): "(*copp)->ob == ob &&
    // strcmp((*copp)->function.s, fun) == 0" -- a closure-bound entry's own
    // real cop->ob is never set for the string form, so it can never match
    // here either (entry.closure being non-null already means entry.target
    // was never populated for that entry).
    for (auto it = callOuts_.begin(); it != callOuts_.end(); ++it) {
        if (!it->closure && it->target.lock() == owner && it->function == function) {
            int64_t remaining = remainingSeconds(it->dueAt);
            callOuts_.erase(it);
            return remaining;
        }
    }
    return -1;
}

int64_t Scheduler::removeCallOutByHandle(int64_t handle) {
    for (auto it = callOuts_.begin(); it != callOuts_.end(); ++it) {
        if (it->handle == handle) {
            int64_t remaining = remainingSeconds(it->dueAt);
            callOuts_.erase(it);
            return remaining;
        }
    }
    return -1;
}

int64_t Scheduler::findCallOutByName(const std::shared_ptr<LpcObject>& owner, const std::string& function) const {
    if (!owner) return -1;
    for (const auto& entry : callOuts_) {
        if (!entry.closure && entry.target.lock() == owner && entry.function == function) {
            return remainingSeconds(entry.dueAt);
        }
    }
    return -1;
}

int64_t Scheduler::findCallOutByHandle(int64_t handle) const {
    for (const auto& entry : callOuts_) {
        if (entry.handle == handle) return remainingSeconds(entry.dueAt);
    }
    return -1;
}

void Scheduler::setHeartbeatInterval(const std::shared_ptr<LpcObject>& obj, int64_t to) {
    if (!obj) return;

    // Real set_heart_beat(): "if (!to) { ... remove from heart_beats[] ...
    // ob->flags &= ~O_HEART_BEAT; return 1; }"
    if (to == 0) {
        obj->setHeartbeatInterval(0);
        auto it = std::find_if(heartbeats_.begin(), heartbeats_.end(),
            [&obj](const HeartbeatEntry& e) { return e.target.lock() == obj; });
        if (it != heartbeats_.end()) heartbeats_.erase(it);
        return;
    }

    auto it = std::find_if(heartbeats_.begin(), heartbeats_.end(),
        [&obj](const HeartbeatEntry& e) { return e.target.lock() == obj; });
    if (it != heartbeats_.end()) {
        // Real: "if (ob->flags & O_HEART_BEAT) { if (to < 0) return 0; ...
        // heart_beats[index].time_to_heart_beat = heart_beats[index].heart_beat_ticks = to; }"
        // -- already enabled: a negative update is rejected outright,
        // leaving the existing interval and countdown untouched.
        if (to < 0) return;
        obj->setHeartbeatInterval(static_cast<int>(to));
        it->ticksRemaining = static_cast<int>(to);
        return;
    }

    // Real: "if (to < 0) to = 1;" -- only the fresh-enable branch clamps a
    // negative interval, to 1 (fire every cycle), rather than rejecting it.
    int interval = to < 0 ? 1 : static_cast<int>(to);
    obj->setHeartbeatInterval(interval);
    heartbeats_.push_back(HeartbeatEntry{obj, interval});
}

void Scheduler::tickHeartbeats() {
    // Decide who fires this cycle and prune dead entries FIRST, entirely
    // before calling any LPC code -- heart_beat() itself is free to call
    // set_heart_beat() on any object, including itself (real std/user.c's
    // own "if(!interactive(this_object())) { set_heart_beat(0); return; }"),
    // which mutates heartbeats_ via setHeartbeatInterval()'s own erase()/
    // find_if(). An earlier version of this function held a live iterator
    // into heartbeats_ across the vm_.callFunction() call below and
    // crashed for exactly this reason: a heart_beat() call re-entering
    // setHeartbeatInterval() invalidated the outer loop's own iterator --
    // confirmed live (a genuine SIGSEGV in vector::erase(), caught during
    // this slice's own live-verification pass, not a hypothetical
    // concern). Collecting a separate snapshot first, the same way
    // tickCallOuts() below already does, closes this the same way call_out
    // firing was already safe against a callout that reschedules itself.
    std::vector<std::shared_ptr<LpcObject>> due;
    for (auto it = heartbeats_.begin(); it != heartbeats_.end();) {
        auto obj = it->target.lock();
        if (!obj) {
            // Real call_heart_beat() relies on O_DESTRUCTED objects having
            // already been pruned from heart_beats[] by remove_all_call_out()-
            // style cleanup at destruct() time; this driver has no
            // O_DESTRUCTED flag (see LpcObject's own known-stub note), so a
            // dead weak_ptr here is the equivalent signal -- just as valid a
            // "this object is gone" check, pruned the same way.
            it = heartbeats_.erase(it);
            continue;
        }
        if (--it->ticksRemaining < 1) {
            it->ticksRemaining = obj->heartbeatInterval();
            due.push_back(obj);
        }
        ++it;
    }

    for (auto& obj : due) {
        try {
            vm_.callFunction(obj, "heart_beat", {});
        } catch (const std::exception& e) {
            std::cerr << "[heart_beat] " << obj->filename() << ": " << e.what() << "\n";
        }
    }
}

void Scheduler::tickCallOuts() {
    auto now = std::chrono::steady_clock::now();
    // Real call_out.c's own main loop: "Move the first call_out out of the
    // chain" before invoking it, so a call_out whose own body reschedules
    // itself (the dominant repeating-timer idiom this mudlib uses, e.g.
    // std/user.c's own rifts_regen_tick()) never corrupts the structure
    // being iterated. Collect due entries first, erase them from callOuts_,
    // then fire each one -- same ordering, expressed without an intrusive
    // linked list.
    std::vector<CallOutEntry> due;
    for (auto it = callOuts_.begin(); it != callOuts_.end();) {
        if (it->dueAt <= now) {
            due.push_back(std::move(*it));
            it = callOuts_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto& entry : due) {
        if (entry.closure) {
            // Real call_function_pointer(): a destructed closure owner
            // fails the call, not a silent skip -- but that failure is
            // still just one call_out's own error, isolated the same way.
            try {
                vm_.callClosure(entry.closure, entry.args);
            } catch (const std::exception& e) {
                std::cerr << "[call_out] (closure): " << e.what() << "\n";
            }
            continue;
        }
        auto obj = entry.target.lock();
        if (!obj) continue; // real: "if (!ob || (ob->flags & O_DESTRUCTED)) { free_call(cop); }" -- silently dropped, not an error.
        try {
            vm_.callFunction(obj, entry.function, entry.args);
        } catch (const std::exception& e) {
            std::cerr << "[call_out] " << obj->filename() << "::" << entry.function << "(): " << e.what() << "\n";
        }
    }
}

} // namespace lpcdriver
