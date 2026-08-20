#include "amlp/object/LpcObject.hpp"
#include <random>

namespace amlp {

LpcObject::LpcObject(std::string filename, std::shared_ptr<CompiledProgram> program)
    : filename_(std::move(filename)), program_(std::move(program)) {
    // Object variable storage is sized once here, at construction, rather
    // than by each caller that constructs an LpcObject (ObjectManager's
    // loadObject()/cloneObject()): this way every LpcObject is correctly
    // sized the moment it exists, with nothing for a caller to remember.
    //
    // Filled with a real int64_t 0 per slot, not a default-constructed
    // Value{} (monostate) -- real LPC has no separate "unset" state for
    // a declared variable distinct from the value 0: every declared
    // object variable, of whatever type, reads back as the integer 0
    // until first assigned (efuns_main.c/interpret.c's own svalue_t
    // defaulting; there is no equivalent of this driver's own
    // monostate at the ordinary-variable level, only at specific driver-
    // internal "no value" sentinels this codebase already uses monostate
    // for elsewhere, e.g. an efun explicitly returning "nothing found").
    // Previously left as monostate here on an unverified assumption that
    // it "reads as 0" -- it does not: monostate fails every arithmetic
    // opcode (Add, IncDec, ...) that a real 0 would silently succeed at.
    // Confirmed live: std/Object.c's own query_name() reading an unset
    // __TrueName, and std/user/nmsh.c's own add_history_cmd() doing
    // "++__CmdNumber" on an unset counter, both threw "unsupported
    // operand types" against a real 0-defaulted mudlib before this fix.
    variables_.resize(program_->objectVarNames.size(), Value(int64_t{0}));

    // real object_t's own time_of_ref starts at creation time (nothing in
    // real object.c ever leaves it at a raw-zero epoch for a live
    // object) -- gives a freshly created object a sane clean_up()-
    // eligibility baseline of "just touched", not "untouched since the
    // Unix epoch".
    touchTimeOfRef();
}

// real reset_object()'s own "Be sure to update time first !" step
// (object.c:825-828): "ob->time_reset = current_time + time_to_reset/2 +
// (mp_int)random_number((uint32)time_to_reset/2);" -- and its own
// unconditional "ob->flags |= O_RESET_STATE;" at the function's end
// (object.c:884), folded into the same call here since every real
// caller (H_CREATE_* dispatch at load/clone time, and a real or virtual
// reset firing) wants both together. Matches real random_number()'s own
// [0, n) range (see EfunTable.cpp's "random" efun for the same
// std::uniform_int_distribution idiom).
void LpcObject::armReset(std::chrono::seconds timeToReset) {
    auto half = timeToReset / 2;
    static std::mt19937 rng(std::random_device{}());
    std::chrono::seconds jitter{0};
    if (half.count() > 0) {
        std::uniform_int_distribution<long long> dist(0, half.count() - 1);
        jitter = std::chrono::seconds(dist(rng));
    }
    timeReset_ = std::chrono::steady_clock::now() + half + jitter;
    resetState_ = true;
}

} // namespace amlp
