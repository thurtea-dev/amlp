#pragma once
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include "lpcdriver/vm/Value.hpp"

namespace lpcdriver {

class VM;
class Server;
class LpcObject;

struct CallOutEntry {
    std::weak_ptr<LpcObject> target;
    std::string function;
    std::vector<Value> args;
    std::chrono::steady_clock::time_point dueAt;
};

class Scheduler {
public:
    explicit Scheduler(VM& vm);

    void run(Server& server, int maxIterations = 0);

    void addCallOut(CallOutEntry entry);
    void removeCallOut(const std::shared_ptr<LpcObject>& obj, const std::string& function);

    static void requestShutdown();

private:
    void tickHeartbeats();
    void tickCallOuts();

    VM& vm_;
    std::vector<CallOutEntry> callOuts_;
    std::chrono::steady_clock::time_point lastHeartbeat_;
};

} // namespace lpcdriver
