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
    std::shared_ptr<CompiledProgram> compile(const std::string& filename);

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
