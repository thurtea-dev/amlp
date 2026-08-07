#pragma once
#include <memory>
#include <string>
#include <vector>
#include "lpcdriver/vm/Value.hpp"
#include "lpcdriver/vm/Bytecode.hpp"

namespace lpcdriver {

class ObjectManager;
class LpcObject;
class Config;

class VM {
public:
    VM(ObjectManager& objects, Config& config);

    Value callFunction(const std::shared_ptr<LpcObject>& obj,
                        const std::string& functionName,
                        std::vector<Value> args);

    Value applyMaster(const std::string& applyName, std::vector<Value> args);

    std::shared_ptr<LpcObject> cloneObject(const std::string& filename);

    // real destruct(): removes obj from the object table (thin wrapper
    // over ObjectManager::destructObject(), which already exists for
    // this). This driver has no O_DESTRUCTED flag/deferred-free scheme
    // (LpcObject lifetime is plain shared_ptr refcounting): calling a
    // function on an already-"destructed" object here is not
    // specifically guarded against the way real FluffOS guards every
    // apply() with an O_DESTRUCTED check, it simply keeps working
    // against a live C++ object until the last shared_ptr reference
    // actually drops. Nothing this driver runs yet depends on that
    // distinction (see the destruct efun's own comment in EfunTable.cpp
    // for the interactive-connection-specific handling layered on top
    // of this).
    void destructObject(const std::shared_ptr<LpcObject>& obj);

    // real FluffOS's master() efun (func_spec.c: "object master();") --
    // just the already-loaded master object, no different from what
    // applyMaster() already dispatches against.
    std::shared_ptr<LpcObject> masterObject() const;

    // real FluffOS's find_object(): an already-loaded lookup that falls
    // back to *compiling and loading the file on a miss*
    // (simulate.c's find_object(): "if ((ob = lookup_object_hash(
    // tmpbuf))) return ob; ob = load_object(tmpbuf, 0); ..." -- this is
    // NOT the separate "look only, never load" find_object2() also in
    // that same file, which is easy to confuse it with from
    // efuns_main.c's f__call_other() alone. This is exactly why real
    // master.c's own preload() function can force-load a daemon with
    // nothing more than "call_other(str, \"???\")": the call_other
    // itself, via find_object(), is what compiles it. Used by the
    // call_other efun's string-target overload; implemented as a thin
    // wrapper over ObjectManager::loadObject(), which already has
    // exactly this compile-if-needed/cache-by-filename behavior.
    std::shared_ptr<LpcObject> findObject(const std::string& filename) const;

    // Look-only counterpart to findObject() above -- see
    // ObjectManager::lookupLoadedObject()'s own comment. Backs the
    // find_object() efun's default (no-compile) behavior.
    std::shared_ptr<LpcObject> lookupObject(const std::string& filename) const;

    // The object whose function body is currently executing (the
    // top of the C++-recursion call stack run() maintains -- see
    // run()'s own StackGuard comment). This is real FluffOS's
    // current_object, needed by efuns like input_to() (simulate.c's
    // input_to(): "s->ob = current_object;") that must know which
    // object registered them, not just which connection is driving the
    // call. Returns null if called outside any run() (e.g. efuns
    // invoked directly from ObjectManager::loadObject()'s create() call
    // do go through run(), so this is populated correctly there too).
    std::shared_ptr<LpcObject> currentObject() const;

    // real FluffOS's previous_object(int idx = 0) (efuns_main.c's
    // f_previous_object()): the object idx object-changing calls back
    // up the chain from here (0 = whoever was current_object right
    // before the most recent call_other/simul_efun-tier crossing into a
    // different object; the object doing that crossing's "prev_ob").
    // Backed by objectChangeStack_, a *separate* stack from
    // currentObject()'s callStack_: real FluffOS only pushes a
    // FRAME_OB_CHANGE frame when a call actually crosses into a
    // different object, not for every same-object local/inherited call
    // -- see run()'s own comment for how that distinction is detected.
    // Returns null if idx is out of range (no such frame -- e.g. idx is
    // past the top-level driver-originated call, which has no LPC
    // caller at all).
    std::shared_ptr<LpcObject> previousObject(int idx) const;

    // real previous_object(-1) / all_previous_objects(): every entry in
    // objectChangeStack_, nearest first, with the top-level "no LPC
    // caller" case simply absent (the real driver's own array
    // construction only counts frames that had a non-null prev_ob, see
    // efuns_main.c's f_previous_object() -1 branch: "i = previous_ob ?
    // 1 : 0" before ever incrementing further).
    std::vector<std::shared_ptr<LpcObject>> allPreviousObjects() const;

    // real FluffOS's evaluate()/funcall() (efuns_main.c's f__evaluate(),
    // registered under both names -- func_spec.c: "mixed evaluate
    // _evaluate(mixed, ...); mixed funcall _evaluate(mixed, ...);"):
    // invokes a Closure value with its own bound args followed by
    // extraArgs (see Value.hpp's Closure comment for the bound-args-
    // first ordering), resolving the closure's bare function name
    // lazily against its owner object -- local/inherited, then the
    // simul_efun object, then the core efun table, the same tiered
    // order OpCode::Call already uses for a bare name (see this
    // method's own .cpp comment for why re-resolving lazily here is a
    // deliberate, safe simplification rather than real FluffOS's
    // eager FP_LOCAL/FP_SIMUL/FP_EFUN classification at construction
    // time). Throws if the closure's owner object has been destructed
    // (real call_function_pointer()'s own "Owner ... is destructed"
    // check) or if the bare name resolves to nothing at all.
    Value callClosure(const std::shared_ptr<Closure>& closure, std::vector<Value> extraArgs);

    // Resolves an absolute-from-mudlib-root LPC path (e.g. the "cfg"
    // argument to read_file()/write_file()) to a real filesystem path,
    // the same way ObjectManager::compile() resolves a ".c" source file's
    // path -- just without appending ".c", since these are plain data
    // files, not compilable objects. File-I/O efuns use this so mudlib
    // paths never depend on the driver's own current working directory.
    std::string resolveMudlibPath(const std::string& lpcPath) const;

private:
    Value run(const CompiledProgram& program, const FunctionEntry& fn,
              std::vector<Value> args, const std::shared_ptr<LpcObject>& obj);

    ObjectManager& objects_;
    Config& config_;
    std::vector<Value> stack_;
    int evalCost_ = 0;
    // One entry per still-active run() call, innermost last -- see
    // currentObject() and run()'s StackGuard.
    std::vector<std::shared_ptr<LpcObject>> callStack_;
    // One entry per still-active *object-changing* call, innermost
    // last -- see previousObject()/allPreviousObjects() and run()'s own
    // comment on how an object change is detected.
    std::vector<std::shared_ptr<LpcObject>> objectChangeStack_;
};

} // namespace lpcdriver
