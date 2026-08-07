#pragma once

namespace lpcdriver {

class Connection;

class OutputContext {
public:
    static void set(Connection* conn);
    static Connection* current();
};

} // namespace lpcdriver
