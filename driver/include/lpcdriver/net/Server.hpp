#pragma once
#include <memory>
#include <string>
#include <vector>
#include "lpcdriver/net/Connection.hpp"

namespace lpcdriver {

class Config;
class VM;
class ObjectManager;
class Scheduler;

class Server {
public:
    Server(Config& config, VM& vm, ObjectManager& objects, Scheduler& scheduler);
    ~Server();

    bool listen();
    void pollOnce();

    size_t connectionCount() const { return connections_.size(); }

    // The per-line input_to()-or-process_input() dispatch decision (see
    // comm.c's process_user_command()/call_function_interactive()),
    // pulled out of handleConnection() as a free-standing, directly
    // testable step: it only touches the VM and the one connection
    // passed in, no Server instance state, so unit tests can drive it
    // without a real listening socket. handleConnection() is what wraps
    // this in the per-connection error-isolation try/catch; this method
    // itself just throws normally on a runtime error like any other VM
    // call.
    static void dispatchLine(VM& vm, Connection& conn, const std::string& line);

private:
    void acceptNewConnections();
    void handleConnection(Connection& conn);
    void onNewConnection(int clientFd);

    Config& config_;
    VM& vm_;
    ObjectManager& objects_;
    Scheduler& scheduler_;
    int listenFd_ = -1;
    std::vector<std::shared_ptr<Connection>> connections_;
};

} // namespace lpcdriver
