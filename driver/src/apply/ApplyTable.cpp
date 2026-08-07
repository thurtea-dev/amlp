#include "lpcdriver/apply/ApplyTable.hpp"

namespace lpcdriver {

const std::unordered_set<std::string>& ApplyTable::known() {
    static const std::unordered_set<std::string> names = {
        "create", "init", "clean_up", "heart_beat",
        "connect", "logon", "disconnect",
        "id", "short", "long",
        "catch_tell", "receive_message", "process_input",
        "compile_object", "valid_read", "valid_write",
        "valid_socket", "get_root_uid", "epilog", "flag"
    };
    return names;
}

bool ApplyTable::isKnownApply(const std::string& name) {
    return known().count(name) > 0;
}

} // namespace lpcdriver
