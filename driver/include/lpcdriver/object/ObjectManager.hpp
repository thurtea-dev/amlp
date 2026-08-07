#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "lpcdriver/object/LpcObject.hpp"

namespace lpcdriver {

class Config;
class VM;

class ObjectManager {
public:
    explicit ObjectManager(Config& config);

    void setVM(VM* vm) { vm_ = vm; }

    bool loadMasterObject();
    std::shared_ptr<LpcObject> masterObject() const { return master_; }

    // Non-fatal by design (see .cpp): a missing/misconfigured simul_efun
    // file should not prevent the rest of the driver from booting while
    // this is still under incremental construction, unlike master_file.
    // Returns false if config_.simulEfunFile() is empty (not configured)
    // or the load failed; either way simulEfunObject() then just stays
    // nullptr and the simul_efun function-resolution tier is skipped.
    bool loadSimulEfunObject();
    std::shared_ptr<LpcObject> simulEfunObject() const { return simulEfunObject_; }

    std::shared_ptr<LpcObject> loadObject(const std::string& filename);
    std::shared_ptr<LpcObject> cloneObject(const std::string& filename);

    // Look-only, no compile-on-miss: real func_spec.c's own
    // "find_object(string, int default: 0)" -- unlike VM::findObject()
    // (which wraps loadObject() for call_other()'s string-target
    // overload, real find_object()'s own unconditional-compile
    // behavior as actually implemented in simulate.c), a bare
    // find_object(path) call from LPC code defaults to *not* compiling
    // a miss, only "load_object(path)" (a real alias with default arg
    // 1 instead of 0) does. See the find_object efun in EfunTable.cpp
    // for how both are exposed from this one lookup.
    std::shared_ptr<LpcObject> lookupLoadedObject(const std::string& filename) const;

    void destructObject(const std::shared_ptr<LpcObject>& obj);

private:
    // Strips one trailing ".c" if present, so "id_card" and "id_card.c"
    // resolve to the exact same cache entry and object identity. Real
    // LPC object paths never carry the ".c" extension internally (this
    // driver appends it itself to find the source file on disk -- see
    // compile()'s own "filename + \".c\"" below), but plenty of real
    // mudlib call sites still pass one anyway out of habit (e.g. daemon/
    // rifts_start_d.c's own give_item(player, "id_card.c")). Without this,
    // such a call resolved to a literal "id_card.c.c" file that never
    // exists. Same normalization shape as save_object()'s own path
    // handling (EfunTable.cpp), applied on the load/compile side instead.
    static std::string normalizeFilename(const std::string& filename);

    std::shared_ptr<CompiledProgram> compile(const std::string& filename);

    // Runs "$objvarinit" (see CodeGen::generate()'s own comment) on obj
    // for program and, first, every program it inherits (parent-before-
    // child, matching real LPC's own object-variable-initializer
    // ordering) -- called once per new instance, immediately before
    // "create", from both loadObject() and cloneObject() below. Each
    // level uses VM::callFunctionInProgram() rather than the normal
    // tiered lookup specifically so a parent's own initializers still
    // run even when a child also declares (and therefore also
    // synthesizes its own same-named "$objvarinit" for) initialized
    // variables of its own.
    void runObjectVarInitializers(const std::shared_ptr<LpcObject>& obj, const CompiledProgram& program);

    // real int_load_object()'s own virtual-object fallback (simulate.c):
    // when a plain load_object()/find_object() names a path with no
    // matching ".c" file on disk, the real driver does not treat that
    // as a hard failure -- it calls the master apply
    // compile_object(path, 0) and, if that returns a real object,
    // rebinds it to the requested path and uses it as though it had
    // been compiled from there (secure/daemon/master.c uses exactly
    // this for player objects: "/secure/save/users/t/name" has no .c
    // file at all, compile_object()'s own DIR_USERS branch instead
    // clones /std/user and returns that clone). See ObjectManager.cpp's
    // own comment on this method for the full citation and the
    // rebinding this replicates.
    std::shared_ptr<LpcObject> loadVirtualObject(const std::string& filename);
    bool sourceFileExists(const std::string& filename) const;

    // Real simulate.c's own init_privs_for_object(), called from
    // init_object() for every freshly compiled or cloned object before
    // its own create() runs: "push_malloced_string(add_slash(ob->obname));
    // value = apply_master_ob(APPLY_PRIVS_FILE, 1); ob->privs = (value &&
    // value->type == T_STRING) ? value->u.string : NULL;" -- this
    // driver had no equivalent at all (LpcObject's own privs_ optional
    // simply stayed unset for every object unless a mudlib file called
    // set_privs() on itself directly), confirmed live needing this:
    // secure/SimulEfun/log_file.c's own "explode(query_privs(
    // previous_object()), \":\")" throws when query_privs() returns 0
    // (this driver's correct "unset" value, matching real semantics --
    // see query_privs()'s own EfunTable.cpp comment) for any object
    // whose compile-time privs were never actually assigned. Skipped
    // (leaving privs unset, the real driver's own "!current_object"
    // bootstrap branch outcome) when master_ itself is not loaded yet --
    // this only matters for master.c's own bootstrap load, nothing this
    // driver has run yet depends on the master object's own privs.
    void initPrivsForObject(const std::shared_ptr<LpcObject>& obj, const std::string& filename);

    Config& config_;
    VM* vm_ = nullptr;
    std::shared_ptr<LpcObject> master_;
    std::shared_ptr<LpcObject> simulEfunObject_;
    std::unordered_map<std::string, std::shared_ptr<CompiledProgram>> programCache_;
    std::unordered_map<std::string, std::shared_ptr<LpcObject>> loaded_;
    // Filenames currently mid-compile, so an inherit cycle (A inherits B
    // inherits A) is caught as a compile error instead of infinite
    // recursion. See compile()'s recursive inherit resolution.
    std::unordered_set<std::string> compiling_;
    // Same idea as compiling_ above, but for loadVirtualObject()'s own
    // compile_object() apply -- guards against compile_object() somehow
    // calling load_object() back on the exact same still-unresolved
    // virtual path (real driver has no such recursion either; this is
    // this driver's own safety net, not a replicated mechanism).
    std::unordered_set<std::string> virtualCompiling_;
};

} // namespace lpcdriver
