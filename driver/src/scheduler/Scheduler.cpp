#include "lpcdriver/scheduler/Scheduler.hpp"
#include "lpcdriver/net/Server.hpp"
#include <thread>
#include <atomic>

namespace lpcdriver {

namespace {
std::atomic<bool> g_shutdownRequested{false};
}

Scheduler::Scheduler(VM& vm)
    : vm_(vm), lastHeartbeat_(std::chrono::steady_clock::now()) {}

void Scheduler::requestShutdown() {
    g_shutdownRequested.store(true);
}

void Scheduler::run(Server& server, int maxIterations) {
    g_shutdownRequested.store(false);
    int iterations = 0;

    for (;;) {
        server.pollOnce();
        tickHeartbeats();
        tickCallOuts();

        ++iterations;
        if (maxIterations > 0 && iterations >= maxIterations) break;
        if (g_shutdownRequested.load()) break;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void Scheduler::tickHeartbeats() {
}

void Scheduler::tickCallOuts() {
}

void Scheduler::addCallOut(CallOutEntry entry) {
    callOuts_.push_back(std::move(entry));
}

void Scheduler::removeCallOut(const std::shared_ptr<LpcObject>& obj, const std::string& function) {
    (void)obj;
    (void)function;
}

} // namespace lpcdriver
