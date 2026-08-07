#pragma once
#include <memory>
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

private:
    std::string filename_;
    std::shared_ptr<CompiledProgram> program_;
    std::vector<Value> variables_;
    bool heartbeatEnabled_ = false;
};

} // namespace lpcdriver
