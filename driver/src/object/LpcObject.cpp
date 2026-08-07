#include "lpcdriver/object/LpcObject.hpp"

namespace lpcdriver {

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
}

} // namespace lpcdriver
