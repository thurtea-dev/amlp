#include <iostream>
#include <cstdlib>
#include <csignal>
#include "amlp/config/Config.hpp"
#include "amlp/object/ObjectManager.hpp"
#include "amlp/vm/VM.hpp"
#include "amlp/net/Server.hpp"
#include "amlp/scheduler/Scheduler.hpp"
#include "amlp/efun/EfunTable.hpp"
#include "amlp/dialect/DialectSelect.hpp"
#include "amlp/dialect/MasterUidBoot.hpp"

namespace {
void handleSignal(int) {
    amlp::Scheduler::requestShutdown();
}
}

int main(int argc, char** argv) {
    // No fallback config path: this used to default to "config/driver.cfg",
    // a path that has never existed anywhere in this repo (found during a
    // dependency grep, not acted on until now) -- a silently-dead branch,
    // since every real invocation of this binary, throughout this
    // project's own history (README.md, every live-verification session
    // recorded in STATUS.md), already passes an explicit config path.
    // Repointing the default at etc/driver.cfg (now the one canonical
    // config, see STATUS.md's consolidation entry) was considered instead
    // of removing it, but would still silently assume the process runs
    // from the repo root -- an assumption nothing else here makes, and
    // one real invocation pattern has never actually relied on. Failing
    // loudly with a usage message is more honest than guessing a path.
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config-path> [max-iterations]\n";
        return 1;
    }
    std::string configPath = argv[1];

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
    // src/dialect/MasterUidBoot.hpp). BootApi is now genuinely
    // config-driven -- Config::dialect() defaults to "fluffos", matching
    // this driver's own prior hardcoded behavior exactly for any config
    // file that never sets the key. "dgd" throws (DgdBootApi does not
    // exist yet, out of scope) -- see DialectSelect.hpp.
    auto bootApi = amlp::makeBootApiForConfig(config);
    if (auto uid = amlp::queryMasterUid(vm, *bootApi)) {
        std::cout << "  master " << bootApi->masterUidApply() << "() = \"" << *uid << "\"\n";
    } else {
        std::cout << "  master does not define " << bootApi->masterUidApply() << "()\n";
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
