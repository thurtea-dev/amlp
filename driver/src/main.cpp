#include <iostream>
#include <cstdlib>
#include <csignal>
#include "lpcdriver/config/Config.hpp"
#include "lpcdriver/object/ObjectManager.hpp"
#include "lpcdriver/vm/VM.hpp"
#include "lpcdriver/net/Server.hpp"
#include "lpcdriver/scheduler/Scheduler.hpp"
#include "lpcdriver/efun/EfunTable.hpp"

namespace {
void handleSignal(int) {
    lpcdriver::Scheduler::requestShutdown();
}
}

int main(int argc, char** argv) {
    std::string configPath = (argc > 1) ? argv[1] : "config/driver.cfg";

    int maxIterations = 0;
    if (argc > 2) {
        maxIterations = std::atoi(argv[2]);
    } else if (const char* envVal = std::getenv("LPCDRIVER_MAX_ITERATIONS")) {
        maxIterations = std::atoi(envVal);
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    lpcdriver::Config config;
    if (!config.loadFromFile(configPath)) {
        std::cerr << "Failed to load config: " << configPath << "\n";
        return 1;
    }

    lpcdriver::registerCoreEfuns();

    lpcdriver::ObjectManager objectManager(config);
    lpcdriver::VM vm(objectManager, config);
    objectManager.setVM(&vm);

    lpcdriver::Scheduler scheduler(vm);
    vm.setScheduler(&scheduler);
    lpcdriver::Server server(config, vm, objectManager, scheduler);

    std::cout << "lpcdriver booting...\n";
    std::cout << "  mudlib_root = " << config.mudlibRoot() << "\n";
    std::cout << "  master_file = " << config.masterFile() << "\n";
    std::cout << "  port        = " << config.port() << "\n";

    if (!objectManager.loadMasterObject()) {
        std::cerr << "Failed to load master object: " << config.masterFile() << "\n";
        return 1;
    }

    std::cout << "Driver booted. Master object loaded: " << config.masterFile() << "\n";

    // Non-fatal: an unconfigured or currently-uncompilable simul_efun
    // file should not block the rest of the driver from booting (see
    // ObjectManager::loadSimulEfunObject()'s comment).
    if (!config.simulEfunFile().empty()) {
        if (objectManager.loadSimulEfunObject()) {
            std::cout << "Simul_efun object loaded: " << config.simulEfunFile() << "\n";
        } else {
            std::cerr << "Warning: failed to load simul_efun object "
                       << config.simulEfunFile() << " (continuing without it)\n";
        }
    }

    if (!server.listen()) {
        std::cerr << "Failed to start network listener on port " << config.port() << "\n";
        return 1;
    }

    std::cout << "Ready for connections. Try: telnet localhost " << config.port() << "\n";
    if (maxIterations > 0) {
        std::cout << "(test mode: will exit after " << maxIterations << " poll iterations)\n";
    } else {
        std::cout << "(press Ctrl-C to stop)\n";
    }

    scheduler.run(server, maxIterations);

    std::cout << "lpcdriver shutting down.\n";
    return 0;
}
