#pragma once
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "lpcdriver/vm/Value.hpp"

namespace lpcdriver {

class VM;
class Server;
class LpcObject;
struct Closure;

// One pending call_out(). Real call_out.c's pending_call_t stores either a
// string function name bound to an owning object, or a bound closure
// (funptr_t) with no separate owner field -- reproduced here as
// std::shared_ptr<Closure> closure being non-null for the closure form
// (owner comes from Closure::owner, matching real "ob->function.f->hdr.owner"),
// and target/function used for the string form. handle is this driver's own
// simplification of real call_out.c's cycle-slot-encoded handle (see
// Scheduler.cpp's own comment on newCallOutHandle()) -- unique and stable is
// all any real call site actually needs from it.
struct CallOutEntry {
    int64_t handle = 0;
    std::weak_ptr<LpcObject> target;      // owner, for the string-name form
    std::string function;                  // function name, string form only
    std::shared_ptr<Closure> closure;      // set instead of target/function for the closure form
    std::vector<Value> args;
    std::chrono::steady_clock::time_point dueAt;
};

// One object with set_heart_beat() currently enabled. Mirrors real
// backend.c's heart_beats[] array entry (ob + time_to_heart_beat +
// heart_beat_ticks) -- ticksRemaining is heart_beat_ticks, LpcObject's own
// heartbeatInterval() is time_to_heart_beat (the reset value).
struct HeartbeatEntry {
    std::weak_ptr<LpcObject> target;
    int ticksRemaining = 1;
};

class Scheduler {
public:
    explicit Scheduler(VM& vm);

    void run(Server& server, int maxIterations = 0);

    // Real FluffOS default (options.h: "#define HEARTBEAT_INTERVAL 2" --
    // real seconds between heartbeat cycles, confirmed against this exact
    // vendored fluffos-2.9-ds2.08 tree, not assumed).
    static constexpr std::chrono::seconds kHeartbeatCycle{2};

    // int64_t call_out(...) -- returns the new entry's handle, matching
    // real new_call_out()'s own CALLOUT_HANDLES return value (confirmed
    // active in this vendored build's options.h). delay is clamped to >= 0
    // by the caller (the call_out efun), matching real new_call_out()'s
    // own "if (delay < 0) delay = 0;" -- not reproduced twice here.
    int64_t addCallOut(CallOutEntry entry);

    // Real remove_call_out(object_t*, const char*): removes the first
    // pending entry owned by "owner" whose function name matches "function"
    // -- never matches a closure-bound entry (real cop->ob is only set for
    // the string form). Returns the remaining delay in whole seconds, or -1
    // if nothing matched, matching real semantics exactly (not just a
    // boolean).
    int64_t removeCallOutByName(const std::shared_ptr<LpcObject>& owner, const std::string& function);
    // Real remove_call_out_by_handle(): removes by handle regardless of
    // owner (a handle is already globally unique). Same -1-or-remaining
    // return convention.
    int64_t removeCallOutByHandle(int64_t handle);

    // Real find_call_out()/find_call_out_by_handle(): same lookup rules as
    // the two remove methods above, without removing anything.
    int64_t findCallOutByName(const std::shared_ptr<LpcObject>& owner, const std::string& function) const;
    int64_t findCallOutByHandle(int64_t handle) const;

    // Real set_heart_beat(object_t*, int): to == 0 disables (and forgets
    // the entry entirely, matching real backend.c freeing the heart_beats[]
    // slot); to != 0 on an object not yet enabled adds a fresh entry
    // (negative "to" clamps to 1); to != 0 on an object already enabled
    // updates the interval on a positive "to" or is rejected (a no-op) on a
    // negative one -- all four branches read directly from backend.c's own
    // set_heart_beat(), not guessed.
    void setHeartbeatInterval(const std::shared_ptr<LpcObject>& obj, int64_t to);

    // Process exactly one heartbeat cycle (decrement every enabled object's
    // countdown, fire heart_beat() on any that reach zero, reset it back to
    // its configured interval) and exactly one call_out sweep (fire every
    // entry whose dueAt has passed), each fired call isolated so one
    // object's runtime error cannot stop the rest -- matching real
    // SETJMP-per-call_out/per-heartbeat recovery in call_out.c/backend.c,
    // and this driver's own already-established per-connection/per-object
    // error-isolation convention (ObjectManager::loadObject()'s create()
    // guard, Server's per-line dispatch guard). Deliberately public and
    // free of any real-time gating of their own (see run()'s own comment
    // for where that gating lives) so unit tests can call either directly
    // and deterministically -- the same reasoning STATUS.md already
    // documents for why Server::dispatchLine() was pulled out as its own
    // directly-testable method.
    void tickHeartbeats();
    void tickCallOuts();

    static void requestShutdown();

private:
    int64_t newCallOutHandle();

    VM& vm_;
    std::vector<CallOutEntry> callOuts_;
    std::vector<HeartbeatEntry> heartbeats_;
    std::chrono::steady_clock::time_point lastHeartbeat_;
    int64_t nextHandle_ = 1;
};

} // namespace lpcdriver
