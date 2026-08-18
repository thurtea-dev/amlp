#include <iostream>
#include <cstdlib>
#include <csignal>
#include "amlp/config/Config.hpp"
#include "amlp/object/ObjectManager.hpp"
#include "amlp/vm/VM.hpp"
#include "amlp/net/Server.hpp"
#include "amlp/scheduler/Scheduler.hpp"
#include "amlp/efun/EfunTable.hpp"
#include "amlp/dialect/FluffOsBootApi.hpp"
#include "amlp/dialect/MasterUidBoot.hpp"

namespace {
void handleSignal(int) {
    amlp::Scheduler::requestShutdown();
}
}

int main(int argc, char** argv) {
    std::string configPath = (argc > 1) ? argv[1] : "config/driver.cfg";

    int maxIterations = 0;
    if (argc > 2) {
        maxIterations = std::atoi(argv[2]);
    } else if (const char* envVal = std::getenv("AMLP_MAX_ITERATIONS")) {
        maxIterations = std::atoi(envVal);
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    amlp::Config config;
    if (!config.loadFromFile(configPath)) {
        std::cerr << "Failed to load config: " << configPath << "\n";
        return 1;
    }

    amlp::registerCoreEfuns();

    amlp::ObjectManager objectManager(config);
    amlp::VM vm(objectManager, config);
    objectManager.setVM(&vm);

    amlp::Scheduler scheduler(vm);
    vm.setScheduler(&scheduler);
    amlp::Server server(config, vm, objectManager, scheduler);

    std::cout << "amlp booting...\n";
    std::cout << "  mudlib_root = " << config.mudlibRoot() << "\n";
    std::cout << "  master_file = " << config.masterFile() << "\n";
    std::cout << "  port        = " << config.port() << "\n";

    if (!objectManager.loadMasterObject()) {
        std::cerr << "Failed to load master object: " << config.masterFile() << "\n";
        return 1;
    }

    std::cout << "Driver booted. Master object loaded: " << config.masterFile() << "\n";

    // Real per-dialect boot-time master UID query (see
    // src/dialect/MasterUidBoot.hpp). Hardcoded to FluffOS here --
    // Config::dialect() (ROADMAP row 1.1) is not wired up yet, so
    // FluffOsBootApi is the only dialect this driver actually runs as
    // today; swap this for a DialectFactory-selected BootApi once row
    // 1.1 lands a real config-driven switch.
    amlp::FluffOsBootApi bootApi(config);
    if (auto uid = amlp::queryMasterUid(vm, bootApi)) {
        std::cout << "  master " << bootApi.masterUidApply() << "() = \"" << *uid << "\"\n";
    } else {
        std::cout << "  master does not define " << bootApi.masterUidApply() << "()\n";
    }

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

    std::cout << "amlp shutting down.\n";
    return 0;
}
