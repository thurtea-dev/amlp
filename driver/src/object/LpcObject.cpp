#include "lpcdriver/object/LpcObject.hpp"

namespace lpcdriver {

LpcObject::LpcObject(std::string filename, std::shared_ptr<CompiledProgram> program)
    : filename_(std::move(filename)), program_(std::move(program)) {
    // Object variable storage is sized once here, at construction, rather
    // than by each caller that constructs an LpcObject (ObjectManager's
    // loadObject()/cloneObject()): this way every LpcObject is correctly
    // sized the moment it exists, with nothing for a caller to remember.
    // Default-constructed Value{} per slot (monostate) matches how a
    // declared-but-not-yet-assigned LPC object variable reads as 0.
    variables_.resize(program_->objectVarNames.size());
}

} // namespace lpcdriver
