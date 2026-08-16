#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "lpcdriver/vm/Bytecode.hpp"
#include "lpcdriver/vm/Value.hpp"

namespace lpcdriver {

class LpcObject : public std::enable_shared_from_this<LpcObject> {
public:
    LpcObject(std::string filename, std::shared_ptr<CompiledProgram> program);

    const std::string& filename() const { return filename_; }

    // Real load_virtual_object() (simulate.c) renames a master-apply-
    // returned object to the virtual path that was actually requested
    // (SETOBNAME + object-hash reinsertion) so file_name()/base_name()
    // and any later find_object()/load_object() for that same path all
    // agree on identity. See ObjectManager::loadVirtualObject().
    void rebindFilename(std::string filename) { filename_ = std::move(filename); }

    CompiledProgram& program() { return *program_; }
    const CompiledProgram& program() const { return *program_; }

    std::vector<Value>& variables() { return variables_; }

    // real object_t's O_HEART_BEAT flag plus its heart_beats[]-entry own
    // "time_to_heart_beat" (backend.c's set_heart_beat()/query_heart_beat()):
    // 0 means disabled; a nonzero interval is how many heartbeat cycles
    // (real default HEARTBEAT_INTERVAL == 2 real seconds each, see
    // Scheduler) elapse between this object's own heart_beat() calls, not
    // just an on/off flag -- confirmed live-needed distinction: std/germ.c's
    // own set_heart_beat(5) relies on a slower cadence than the default,
    // and query_heart_beat() must report that real interval back (backend.c's
    // own query_heart_beat(object_t*) returns heart_beats[index].time_to_heart_beat,
    // not a bare 1). The actual per-cycle countdown state lives in
    // Scheduler's own heartbeat list, not here -- this field is only the
    // configured interval a fresh Scheduler entry resets its countdown to.
    bool hasHeartbeat() const { return heartbeatInterval_ != 0; }
    int heartbeatInterval() const { return heartbeatInterval_; }
    void setHeartbeatInterval(int interval) { heartbeatInterval_ = interval; }

    // real object_t::living_name (set_living_name()). This driver's own
    // find_player() (EfunTable.cpp) does not consult this -- it walks
    // InteractiveRegistry and asks each object its own query_name()
    // instead, a deliberate, already-documented simplification -- so
    // nothing currently reads livingName_ back. It is still stored
    // (rather than a bare no-op efun) so a real consumer can be added
    // later without another round trip through this field's own
    // plumbing, and so query_living_name()-style introspection is at
    // least possible. Surfaced live: std/user.c's own setup() calling
    // set_living_name(query_name()) unconditionally, not gated behind
    // anything this driver's boot path could otherwise skip.
    const std::string& livingName() const { return livingName_; }
    void setLivingName(std::string name) { livingName_ = std::move(name); }

    // real object_t's "super" (environment) and "contains"/"next_inv"
    // (inventory), simplified from FluffOS's intrusive doubly-linked
    // list to a plain vector -- this driver already uses the same
    // simplification elsewhere (e.g. InteractiveRegistry) and nothing
    // confirmed live needs true O(1) unlink or FluffOS's specific
    // next_inv traversal order. environment_ is weak_ptr so an object
    // does not keep its own environment alive (matches real semantics:
    // environment() can legitimately return an object that is about to
    // be destructed once nothing else references it); inventory_ holds
    // real (owning) shared_ptrs, since being "in" something is exactly
    // the reference that keeps a mudlib object alive in this driver
    // (there is no separate global object table the way real FluffOS
    // has one). See VM::moveObject() for how these are kept consistent.
    std::weak_ptr<LpcObject> environment() const { return environment_; }
    void setEnvironment(std::weak_ptr<LpcObject> env) { environment_ = std::move(env); }
    std::vector<std::shared_ptr<LpcObject>>& inventory() { return inventory_; }
    const std::vector<std::shared_ptr<LpcObject>>& inventory() const { return inventory_; }

    // real O_ENABLE_COMMANDS flag (enable_commands()/disable_commands()).
    bool commandsEnabled() const { return commandsEnabled_; }
    void setCommandsEnabled(bool on) { commandsEnabled_ = on; }

    // real object_t's O_DESTRUCTED flag (object.h). Set once, by
    // ObjectManager::destructObject(), and checked at every "call into
    // this object from outside" entry point this driver has (see
    // VM::callFunction()/callClosure()/moveObject()/dispatchCommand(),
    // each citing this flag directly) -- matching real apply()'s own
    // "DEBUG_CHECK(ob->flags & O_DESTRUCTED, ...)" gate (interpret.c).
    // Previously this driver had no such flag at all: a destructed
    // LpcObject just kept working as an ordinary C++ object, reachable
    // and callable through any shared_ptr still pointing at it, until
    // the last one dropped -- confirmed live-reachable via a room's own
    // inventory (destruct() never unlinked the object from its
    // environment either, see destructObject()'s own comment), not just
    // a theoretical gap.
    bool isDestructed() const { return destructed_; }
    void setDestructed(bool d) { destructed_ = d; }

    // real object_t's O_ONCE_INTERACTIVE flag (object.h): set once, the
    // first time this object is ever bound to a connection
    // (Connection::attach(), which covers both the initial login-object
    // bind and a later exec() rebind onto the real player object), and
    // never cleared again -- not even once the connection later
    // disconnects. This is the flag userp()/query_once_interactive()
    // actually check (func_spec.c real aliases of the same efun),
    // distinct from interactive()'s own "is it interactive *right now*"
    // check (InteractiveRegistry membership, which does clear on
    // disconnect). See EfunTable.cpp's "userp" registration for why this
    // driver previously conflated the two.
    bool wasEverInteractive() const { return everInteractive_; }
    void setWasEverInteractive(bool v) { everInteractive_ = v; }

    // real object_t's O_HIDDEN flag (set_hide()/query_hidden()). Real
    // f_set_hide() (efuns_main.c) also maintains two global counters,
    // num_hidden and num_hidden_users, used elsewhere by real FluffOS's
    // own users()/heart_beat() bookkeeping for reporting how many
    // connected users are currently hidden -- not replicated here, since
    // this driver's users()/interactive() have no privileged-vs-
    // unprivileged visibility filtering of any kind yet (a hidden flag
    // with nothing reading it back is still a real improvement over
    // "undefined efun", matching this session's own explicitly narrow
    // scope: only the flag itself, not the users()-filtering feature it
    // would eventually gate). See EfunTable.cpp's "set_hide" registration
    // for the real valid_hide() master-apply gate this flag is set behind.
    bool isHidden() const { return hidden_; }
    void setHidden(bool h) { hidden_ = h; }

    // real object_t::privs (set_privs()/query_privs()) -- an arbitrary
    // per-object "privilege string" the mudlib sets and later checks for
    // permission gating (secure/daemon/master.c's own valid_write()-
    // style checks: "if(!(priv = query_privs(stack[i]))) return 0;").
    // std::optional distinguishes real FluffOS's "ob->privs == NULL"
    // (never set, or explicitly cleared with a non-string second
    // argument to set_privs()) from an explicitly-set value, matching
    // query_privs()'s own "return 0 if unset" contract precisely.
    const std::optional<std::string>& privs() const { return privs_; }
    void setPrivs(std::optional<std::string> privs) { privs_ = std::move(privs); }

    // real sentence_t list (add_action.c) -- one entry per add_action()
    // registration currently active on this object as a command_giver.
    // front() is the most recently added entry: real add_action()
    // always prepends ("adding to the top of the list doesn't harm
    // anything", add_action.c's own comment) so the newest registration
    // is checked first at dispatch time, matching real precedence
    // between an inner room's own verbs and a more general handler
    // registered earlier. owner is the object whose function actually
    // gets called (real sentence_t::ob) -- not necessarily this object,
    // since add_action() registers onto command_giver but the function
    // named runs against whichever object called add_action(). weak_ptr
    // so an action does not keep its owner alive by itself; a
    // destructed owner is simply skipped at dispatch time (see
    // VM::dispatchCommand()), the same "not specifically guarded, just
    // stops mattering once the shared_ptr count drops" approach this
    // driver already takes for destruct() elsewhere.
    struct ActionEntry {
        std::string verb;
        std::string functionName;
        std::weak_ptr<LpcObject> owner;
        int flag = 0; // 0 = exact, 1 = V_SHORT, 2 = V_NOSPACE (real add_action.c flag & 3)
    };
    std::vector<ActionEntry>& actions() { return actions_; }
    void addAction(ActionEntry entry) { actions_.insert(actions_.begin(), std::move(entry)); }
    // real remove_action(): erases the first (most-recently-added, i.e.
    // lowest index) entry whose owner/function/verb all match, mirroring
    // add_action.c's own "*s = tmp->next" unlink -- see EfunTable.cpp's
    // "remove_action" registration for the exact match rule.
    bool removeAction(const std::shared_ptr<LpcObject>& owner, const std::string& functionName,
                       const std::string& verb) {
        for (auto it = actions_.begin(); it != actions_.end(); ++it) {
            if (it->owner.lock() == owner && it->functionName == functionName && it->verb == verb) {
                actions_.erase(it);
                return true;
            }
        }
        return false;
    }

    // real object_t's "shadowed" and "shadowing" fields (object.h), named
    // to match exactly rather than instruct.md's own proposed
    // "shadow_"/"shadowedBy_" pair, which had the relationship backwards
    // in one direction -- confirmed directly against interpret.c's
    // apply_low() and efuns_main.c's f_shadow() before naming these:
    // shadowedBy() is "the object currently shadowing me" (real
    // ob->shadowed; a victim's own field), shadowing() is "the object I
    // am myself shadowing" (real ob->shadowing; a shadow's own field).
    // Both weak_ptr: neither side of a shadow relationship keeps the
    // other alive on its own, matching every other cross-object
    // reference in this class. See VM::callFunction()'s own comment for
    // the real two-phase chain walk these back.
    std::weak_ptr<LpcObject> shadowedBy() const { return shadowedBy_; }
    void setShadowedBy(std::weak_ptr<LpcObject> ob) { shadowedBy_ = std::move(ob); }
    std::weak_ptr<LpcObject> shadowing() const { return shadowing_; }
    void setShadowing(std::weak_ptr<LpcObject> ob) { shadowing_ = std::move(ob); }

    // real object.h's O_VIRTUAL flag: set only by
    // ObjectManager::loadVirtualObject()'s own construction path (an
    // object returned by master()->compile_object() rather than compiled
    // directly from an on-disk file), never cleared. Backs virtualp() --
    // see EfunTable.cpp's own registration and std/virtual.c's real
    // "if(virtualp(this_object())) return 0;" recursion guard, the one
    // confirmed live call site.
    bool isVirtual() const { return isVirtual_; }
    void setIsVirtual(bool v) { isVirtual_ = v; }

private:
    std::string filename_;
    std::shared_ptr<CompiledProgram> program_;
    std::vector<Value> variables_;
    int heartbeatInterval_ = 0;
    std::weak_ptr<LpcObject> environment_;
    std::vector<std::shared_ptr<LpcObject>> inventory_;
    bool commandsEnabled_ = false;
    bool destructed_ = false;
    bool everInteractive_ = false;
    bool hidden_ = false;
    std::vector<ActionEntry> actions_;
    std::optional<std::string> privs_;
    std::string livingName_;
    std::weak_ptr<LpcObject> shadowedBy_;
    std::weak_ptr<LpcObject> shadowing_;
    bool isVirtual_ = false;
};

} // namespace lpcdriver
