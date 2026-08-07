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

    bool hasHeartbeat() const { return heartbeatEnabled_; }
    void setHeartbeat(bool on) { heartbeatEnabled_ = on; }

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

private:
    std::string filename_;
    std::shared_ptr<CompiledProgram> program_;
    std::vector<Value> variables_;
    bool heartbeatEnabled_ = false;
    std::weak_ptr<LpcObject> environment_;
    std::vector<std::shared_ptr<LpcObject>> inventory_;
    bool commandsEnabled_ = false;
    std::vector<ActionEntry> actions_;
    std::optional<std::string> privs_;
    std::string livingName_;
};

} // namespace lpcdriver
