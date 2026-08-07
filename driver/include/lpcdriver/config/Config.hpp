#pragma once
#include <string>
#include <unordered_map>

namespace lpcdriver {

class Config {
public:
    bool loadFromFile(const std::string& path);

    const std::string& mudlibRoot() const { return mudlibRoot_; }
    const std::string& masterFile() const { return masterFile_; }
    int port() const { return port_; }
    int heartbeatIntervalMs() const { return heartbeatIntervalMs_; }
    int maxEvalCost() const { return maxEvalCost_; }
    const std::string& includeDir() const { return includeDir_; }
    // Empty means "not configured" -- no simul_efun tier at all, matching
    // this being genuinely optional infrastructure (real FluffOS treats
    // it as mandatory at boot, but that's not a useful default for a
    // driver still under incremental construction).
    const std::string& simulEfunFile() const { return simulEfunFile_; }
    // Fed to cpp as the quoted MUD_NAME predefine (see ObjectManager.cpp's
    // buildPredefinedMacroFlags()), matching real FluffOS's lex.c own
    // "add_quoted_predefine(\"MUD_NAME\", MUD_NAME)" sourced from
    // mudos.cfg's "name" setting -- confirmed live:
    // secure/SimulEfun/mud_info.c's own "string mud_name() { return
    // MUD_NAME; }".
    const std::string& mudName() const { return mudName_; }

private:
    std::string mudlibRoot_ = "./mudlib_stub";
    std::string masterFile_ = "/master";
    int port_ = 3000;
    int heartbeatIntervalMs_ = 2000;
    int maxEvalCost_ = 10000000;
    std::string includeDir_ = "secure/include";
    std::string simulEfunFile_ = "";
    std::string mudName_ = "AetherMUD";

    std::unordered_map<std::string, std::string> raw_;
};

} // namespace lpcdriver
