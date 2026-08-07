#include "lpcdriver/object/ObjectManager.hpp"
#include "lpcdriver/config/Config.hpp"
#include "lpcdriver/core/Errors.hpp"
#include "lpcdriver/compiler/Lexer.hpp"
#include "lpcdriver/compiler/Parser.hpp"
#include "lpcdriver/compiler/CodeGen.hpp"
#include "lpcdriver/vm/VM.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

namespace lpcdriver {

namespace {

struct PreprocessResult {
    bool ok = false;
    std::string output;
    std::string errorOutput;
};

// FluffOS injects these into every compile itself (option_defs.c's
// predefined-macro table), independent of anything the mudlib's own
// headers define -- so real mudlib code freely uses them as bare
// identifiers (e.g. secure/daemon/master.c's
// "str+__SAVE_EXTENSION__") expecting the driver to have already
// substituted them, the same way __FILE__/__LINE__ work in C. This
// driver shells out to the system's real cpp instead of a bespoke
// preprocessor, so the same effect is achieved by passing each one as a
// "-D" flag. Values are copied verbatim from the FluffOS reference
// driver's option_defs.c; entries with an empty value there are
// feature-flags mudlib code checks with #ifdef, not values it expects to
// read.
struct PredefinedMacro { const char* name; const char* value; };
constexpr PredefinedMacro kFluffosPredefinedMacros[] = {
    {"__STRIP_BEFORE_PROCESS_INPUT__", ""},
    {"__NO_LIGHT__", ""},
    {"__CUSTOM_CRYPT__", ""},
    {"__RECEIVE_SNOOP__", ""},
    {"__ARGUMENTS_IN_TRACEBACK__", ""},
    {"__NEXT_MALLOC_DEBUG__", ""},
    {"__ARRAY_STATS__", ""},
    {"__LOCALS_IN_TRACEBACK__", ""},
    {"__SYSMALLOC__", ""},
    {"__CFG_MAX_CALL_DEPTH__", "150"},
    {"__SAVE_EXTENSION__", "\\\".o\\\""},
    {"__LOG_CATCHES__", ""},
    {"__NONINTERACTIVE_STDERR_WRITE__", ""},
    {"__SMALL_STRING_SIZE__", "100"},
    {"__CALLOUT_CYCLE_SIZE__", "32"},
    {"__PACKAGE_MATH__", ""},
    {"__PACKAGE_DEVELOP__", ""},
    {"__SUPPRESS_ARGUMENT_WARNINGS__", ""},
    {"__LARGEST_PRINTABLE_STRING__", "8192"},
    {"__CFG_LIVING_HASH_SIZE__", "256"},
    {"__CACHE_STATS__", ""},
    {"__CONFIG_FILE_DIR__", "\\\"./\\\""},
    {"__PARSE_DEBUG__", ""},
    {"__TRAP_CRASHES__", ""},
    {"__RESTRICTED_ED__", ""},
    {"__STRING_STATS__", ""},
    {"__CALLOUT_HANDLES__", ""},
    {"__FLUFFOS__", ""},
    {"__CFG_COMPILER_STACK_SIZE__", "600000"},
    {"__LARGE_STRING_SIZE__", "1000"},
    {"__HEARTBEAT_INTERVAL__", "1"},
    {"__PACKAGE_MATRIX__", ""},
    {"__PRIVS__", ""},
    {"__COMMAND_BUF_SIZE__", "2000"},
    {"__OLD_ED__", ""},
    {"__PACKAGE_PARSER__", ""},
    {"__THIS_PLAYER_IN_CALL_OUT__", ""},
    {"__PACKAGE_CONTRIB__", ""},
    {"__ALLOW_INHERIT_AFTER_GLOBAL_VARIABLES__", ""},
    {"__CFG_MAX_LOCAL_VARIABLES__", "50"},
    {"__MESSAGE_BUFFER_SIZE__", "4096"},
    {"__NO_WIZARDS__", ""},
    {"__SAVE_GZ_EXTENSION__", "\\\".o.gz\\\""},
    {"__PACKAGE_SOCKETS__", ""},
    {"__NUM_EXTERNAL_CMDS__", "100"},
    {"__HEART_BEAT_CHUNK__", "32"},
    {"__INTERACTIVE_CATCH_TELL__", ""},
    {"__MAX_SAVE_SVALUE_DEPTH__", "100"},
    {"__DEFAULT_PRAGMAS__", "0"},
    {"__NO_ANSI__", ""},
    {"__HAS_STATUS_TYPE__", ""},
    {"__CFG_EVALUATOR_STACK_SIZE__", "3000"},
    {"__PACKAGE_MUDLIB_STATS__", ""},
    {"__WARN_TAB__", ""},
    {"__ALLOW_INHERIT_AFTER_FUNCTION__", ""},
    {"__APPLY_CACHE_BITS__", "11"},
    {"__MUDLIB_ERROR_HANDLER__", ""},
};

// A second, smaller set of predefines FluffOS injects from lex.c's own
// add_predefines() rather than option_defs.c's static table above --
// still driver-injected the same way, but each one is computed at boot
// (driver version string, build arch, etc) instead of being a fixed
// per-build feature flag. Confirmed live: secure/SimulEfun/mud_info.c's
// own "string mud_name() { return MUD_NAME; }" -- MUD_NAME (and
// __PORT__, the other config-dependent one here) are handled separately
// below since their value comes from this driver's own Config, not a
// fixed literal.
constexpr PredefinedMacro kFluffosRuntimePredefinedMacros[] = {
    {"MUDOS", ""},
    // No spaces in any of these three values (unlike real lex.c's own
    // "FluffOS v2.9-ds2.08 for Linux." __VERSION__ string): the cpp
    // invocation below is built as one unquoted shell command string via
    // popen(), so an embedded space would get word-split into extra
    // (nonexistent) input filenames -- "cpp: fatal error: too many input
    // files" -- exactly like MUD_NAME below would if it were ever
    // configured with a space in it. Not fixed at the quoting level
    // here, just avoided in the one value this code controls.
    {"__VERSION__", "\\\"2.9-ds2.08\\\""},
    {"__ARCH__", "\\\"Linux\\\""},
    {"__COMPILER__", "\\\"g++\\\""},
    {"__OPTIMIZATION__", "\\\"-O2\\\""},
    // sizeof(long) and the resulting max signed value on a 64-bit Linux
    // build, matching real lex.c's own "sizeof(long)"/"(long)1<<63 - 1"
    // computation rather than a value copied from someone else's build.
    {"SIZEOFINT", "8"},
    {"MAX_INT", "9223372036854775807"},
    // HAS_ED / HAS_PRINTF / HAS_RUSAGE / HAS_DEBUG_LEVEL are deliberately
    // not defined here: real lex.c only defines each one when the
    // corresponding driver feature (the "ed" package, F_PRINTF, rusage
    // reporting, debug levels) was actually compiled in, and none of
    // those exist in this driver yet -- defining them would tell mudlib
    // code a capability exists when calling it would just throw
    // NotImplementedError.
};

std::string buildPredefinedMacroFlags(const Config& config) {
    std::ostringstream flags;
    for (const auto& macro : kFluffosPredefinedMacros) {
        flags << " -D" << macro.name << "=" << macro.value;
    }
    for (const auto& macro : kFluffosRuntimePredefinedMacros) {
        flags << " -D" << macro.name << "=" << macro.value;
    }
    flags << " -D__PORT__=" << config.port();
    flags << " -DMUD_NAME=\\\"" << config.mudName() << "\\\"";
    return flags.str();
}

// Real LPC/FluffOS resolves a quoted #include path that starts with '/'
// against the mudlib root, the same convention as every other absolute
// LPC path in this codebase (inherit "/path";, clone_object("/path"),
// etc) -- confirmed live: secure/SimulEfun/SimulEfun.c #includes ~50
// other files this way ("#include \"/secure/SimulEfun/identify.c\"").
// A real system cpp has no concept of a mudlib root and treats a
// leading '/' as the actual filesystem root, so without this rewrite it
// fails outright ("No such file or directory"). This driver shells out
// to a real cpp (see the module comment above runPreprocessor), so the
// fix has to happen before cpp ever sees the source: rewrite each such
// #include line to an actual absolute filesystem path by prepending the
// mudlib root, here, on the raw source text.
std::string rewriteAbsoluteIncludes(const std::string& source, const std::string& mudlibRoot) {
    std::istringstream in(source);
    std::ostringstream out;
    std::string line;
    while (std::getline(in, line)) {
        size_t hashPos = line.find_first_not_of(" \t");
        if (hashPos != std::string::npos && line[hashPos] == '#') {
            size_t incPos = line.find("include", hashPos + 1);
            if (incPos != std::string::npos) {
                size_t quotePos = line.find('"', incPos);
                if (quotePos != std::string::npos && quotePos + 1 < line.size() &&
                    line[quotePos + 1] == '/') {
                    line.insert(quotePos + 1, mudlibRoot);
                }
            }
        }
        out << line << "\n";
    }
    return out.str();
}

struct StagedSource {
    bool ok = false;
    std::string tempPath;
    std::string errorMessage;
};

// Writes the rewritten source (see rewriteAbsoluteIncludes()) to a fresh
// temp file for cpp to actually read, so the on-disk mudlib source is
// never touched. A synthetic "# 1 \"originalPath\"" line is prepended so
// every line-marker cpp itself emits from here on still names the real
// source file, not the temp path -- otherwise this driver's own
// compile-error messages (and any manual line-marker unwinding) would
// point at a throwaway /tmp file instead of the real one.
StagedSource stageSourceForPreprocessing(const std::string& originalPath,
                                          const std::string& mudlibRoot) {
    StagedSource result;

    std::ifstream f(originalPath);
    if (!f) {
        result.errorMessage = "source file not found: " + originalPath;
        return result;
    }
    std::ostringstream buf;
    buf << f.rdbuf();
    f.close();

    std::string rewritten =
        "# 1 \"" + originalPath + "\"\n" + rewriteAbsoluteIncludes(buf.str(), mudlibRoot);

    char tmpPathTemplate[] = "/tmp/lpcdriver_src_XXXXXX";
    int fd = mkstemp(tmpPathTemplate);
    if (fd == -1) {
        result.errorMessage = "failed to create temp file for staged source";
        return result;
    }
    std::string tmpPath = tmpPathTemplate;

    ssize_t written = write(fd, rewritten.data(), rewritten.size());
    close(fd);
    if (written < 0 || static_cast<size_t>(written) != rewritten.size()) {
        std::remove(tmpPath.c_str());
        result.errorMessage = "failed to write staged source to temp file";
        return result;
    }

    result.ok = true;
    result.tempPath = tmpPath;
    return result;
}

// cpp emits line marker directives like `# 1 "file.h"`; the Lexer has no
// concept of these and they are not real LPC syntax, so drop any line
// whose first non-whitespace character is '#'.
std::string stripLineMarkers(const std::string& text) {
    std::istringstream in(text);
    std::ostringstream out;
    std::string line;
    while (std::getline(in, line)) {
        size_t i = line.find_first_not_of(" \t");
        if (i != std::string::npos && line[i] == '#') continue;
        out << line << "\n";
    }
    return out.str();
}

PreprocessResult runPreprocessor(const std::string& sourcePath, const std::string& includeDir,
                                  const std::string& originalSourceDir, const Config& config) {
    PreprocessResult result;

    char errPathTemplate[] = "/tmp/lpcdriver_cpp_stderr_XXXXXX";
    int errFd = mkstemp(errPathTemplate);
    if (errFd == -1) {
        result.errorOutput = "failed to create temp file for cpp stderr output";
        return result;
    }
    close(errFd);
    std::string errPath = errPathTemplate;

    // sourcePath is a staged copy in /tmp (see stageSourceForPreprocessing),
    // not the real file's own directory, so a same-directory quoted
    // #include (e.g. secure/SimulEfun/SimulEfun.c's own "#include
    // \"SimulEfun.h\"") would otherwise resolve against /tmp instead of
    // where the real file lives. Passing the original directory as an
    // extra -I restores that lookup.
    std::string cmd = "cpp -I '" + originalSourceDir + "' -I '" + includeDir + "'" +
                       buildPredefinedMacroFlags(config) +
                       " -x c '" + sourcePath + "' 2>'" + errPath + "'";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        result.errorOutput = "failed to launch cpp preprocessor";
        std::remove(errPath.c_str());
        return result;
    }

    std::ostringstream outBuf;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), pipe)) > 0) {
        outBuf.write(buf, static_cast<std::streamsize>(n));
    }
    int status = pclose(pipe);

    std::ifstream errFile(errPath);
    std::ostringstream errBuf;
    errBuf << errFile.rdbuf();
    std::remove(errPath.c_str());
    result.errorOutput = errBuf.str();

    // Success is the exit code alone, not "stderr is empty": cpp writes
    // real warnings there too (e.g. GCC's cpp warns, but does not fail,
    // on the "#endif LABEL" trailing-token style used throughout this
    // mudlib's own secure/include/debug.h), and treating every warning
    // as a hard preprocessing failure was rejecting files with no actual
    // error in them (hit live loading secure/SimulEfun/SimulEfun.c).
    bool exitedOk = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    result.output = stripLineMarkers(outBuf.str());
    result.ok = exitedOk;
    return result;
}

} // namespace

ObjectManager::ObjectManager(Config& config) : config_(config) {}

std::shared_ptr<CompiledProgram> ObjectManager::compile(const std::string& filename) {
    auto cached = programCache_.find(filename);
    if (cached != programCache_.end()) return cached->second;

    if (compiling_.count(filename)) {
        std::cerr << "[object] inherit cycle detected involving " << filename << "\n";
        return nullptr;
    }
    compiling_.insert(filename);
    struct InProgressGuard {
        std::unordered_set<std::string>& set;
        const std::string& filename;
        ~InProgressGuard() { set.erase(filename); }
    } inProgressGuard{compiling_, filename};

    std::string path = config_.mudlibRoot() + filename + ".c";
    std::ifstream f(path);
    if (!f) {
        std::cerr << "[object] source file not found: " << path << "\n";
        return nullptr;
    }
    f.close();

    std::string includeDir = config_.includeDir();
    if (includeDir.empty() || includeDir[0] != '/') {
        includeDir = config_.mudlibRoot() + "/" + includeDir;
    }

    StagedSource staged = stageSourceForPreprocessing(path, config_.mudlibRoot());
    if (!staged.ok) {
        std::cerr << "[object] " << staged.errorMessage << "\n";
        return nullptr;
    }
    std::string originalSourceDir = path.substr(0, path.find_last_of('/'));
    PreprocessResult preprocessed = runPreprocessor(staged.tempPath, includeDir, originalSourceDir, config_);
    std::remove(staged.tempPath.c_str());
    if (!preprocessed.ok) {
        std::cerr << "[object] preprocessing failed for " << path << ":\n"
                   << preprocessed.errorOutput << "\n";
        return nullptr;
    }

    try {
        Lexer lexer(preprocessed.output);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        auto ast = parser.parseProgram();

        // Resolve "inherit \"path\";" targets by recursively compiling each
        // one (reusing this same cache, so a file inherited by several
        // others is only compiled once) *before* generating this file's
        // own code, so CodeGen can flatten the parents' object variables
        // in ahead of this file's own -- see CodeGen::generate()'s
        // inheritedObjectVarNames parameter. Only single-level lookups are
        // done directly here; each parent's *own* inherits, if any, were
        // already resolved recursively when that parent itself went
        // through this same function, so multi-level chains still work
        // (see Bytecode.hpp's CompiledProgram::inheritedPrograms comment).
        std::vector<std::shared_ptr<CompiledProgram>> parents;
        std::vector<std::string> inheritedObjectVarNames;
        for (const auto& inheritPath : ast->inherits) {
            auto parentProgram = compile(inheritPath);
            if (!parentProgram) {
                std::cerr << "[object] " << path << ": failed to compile inherited file \""
                          << inheritPath << "\"\n";
                return nullptr;
            }
            parents.push_back(parentProgram);
            inheritedObjectVarNames.insert(inheritedObjectVarNames.end(),
                                            parentProgram->objectVarNames.begin(),
                                            parentProgram->objectVarNames.end());
        }

        CodeGen codegen;
        auto program = std::make_shared<CompiledProgram>(
            codegen.generate(*ast, inheritedObjectVarNames));
        program->inheritedPrograms = std::move(parents);

        programCache_[filename] = program;
        return program;
    } catch (const LpcRuntimeError& e) {
        std::cerr << "[object] compile error in " << path << ": " << e.what() << "\n";
        return nullptr;
    } catch (const std::exception& e) {
        std::cerr << "[object] unexpected error compiling " << path << ": " << e.what() << "\n";
        return nullptr;
    }
}

bool ObjectManager::loadMasterObject() {
    master_ = loadObject(config_.masterFile());
    return master_ != nullptr;
}

bool ObjectManager::loadSimulEfunObject() {
    if (config_.simulEfunFile().empty()) return false;
    simulEfunObject_ = loadObject(config_.simulEfunFile());
    return simulEfunObject_ != nullptr;
}

bool ObjectManager::sourceFileExists(const std::string& filename) const {
    std::string path = config_.mudlibRoot() + filename + ".c";
    struct stat st;
    // Matches real int_load_object()'s own check (simulate.c): "stat(
    // real_name, &c_st) == -1 || S_ISDIR(c_st.st_mode)" -- a directory
    // sharing the requested name does not count as a source file
    // either.
    return ::stat(path.c_str(), &st) == 0 && !S_ISDIR(st.st_mode);
}

std::shared_ptr<LpcObject> ObjectManager::loadVirtualObject(const std::string& filename) {
    // See simulate.c's load_virtual_object(): "if (!master_ob) { ...
    // return 0; }" -- no master loaded yet (e.g. this call is itself
    // trying to load the master file), nothing to ask.
    if (!master_ || !vm_) return nullptr;

    if (virtualCompiling_.count(filename)) {
        std::cerr << "[object] compile_object() recursion detected for " << filename << "\n";
        return nullptr;
    }
    virtualCompiling_.insert(filename);
    struct InProgressGuard {
        std::unordered_set<std::string>& set;
        const std::string& filename;
        ~InProgressGuard() { set.erase(filename); }
    } inProgressGuard{virtualCompiling_, filename};

    // real simulate.c: "push_malloced_string(add_slash(name));
    // push_number(clone); ... apply_master_ob(APPLY_COMPILE_OBJECT,
    // argc);" -- clone is 0 here (int_load_object()'s own call site
    // always passes 0; only clone_object() on an already-virtual
    // object passes a nonzero clone count, a path this driver does not
    // implement -- see loadVirtualObject()'s own header comment).
    // master.c's own compile_object(string str) declares only one
    // parameter, so the extra int argument is simply unused by it, the
    // same non-strict-arg-count LPC calling convention every other
    // apply in this driver already relies on.
    Value result;
    try {
        result = vm_->applyMaster("compile_object", {Value(filename), Value(int64_t{0})});
    } catch (const std::exception& e) {
        std::cerr << "[object] compile_object(" << filename << ") failed: " << e.what() << "\n";
        return nullptr;
    }

    if (!std::holds_alternative<std::shared_ptr<LpcObject>>(result.data)) {
        // Real driver: "if (!v || (v->type != T_OBJECT)) return 0;" --
        // compile_object() declining (returning 0/void) means this
        // path genuinely does not exist, virtual or otherwise. Not
        // logged as an error here: this is the normal "no such object"
        // outcome for the overwhelming majority of missing-file
        // lookups, which are not virtual paths at all.
        return nullptr;
    }
    auto ob = std::get<std::shared_ptr<LpcObject>>(result.data);
    if (!ob) return nullptr;

    // real load_virtual_object(): renames the returned object to the
    // requested virtual path and reinserts it into the object hash
    // under that name (SETOBNAME + enter_object_hash()) -- from this
    // point on the object IS "filename" as far as file_name()/
    // base_name() and any later find_object()/load_object() for the
    // same path are concerned, indistinguishable from having been
    // compiled there directly.
    ob->rebindFilename(filename);
    loaded_[filename] = ob;
    return ob;
}

std::shared_ptr<LpcObject> ObjectManager::loadObject(const std::string& filename) {
    auto existing = loaded_.find(filename);
    if (existing != loaded_.end()) return existing->second;

    if (!sourceFileExists(filename)) {
        auto virtualObj = loadVirtualObject(filename);
        if (virtualObj) return virtualObj;
        std::cerr << "[object] source file not found: "
                   << config_.mudlibRoot() << filename << ".c"
                   << " (and compile_object() did not provide a virtual object)\n";
        return nullptr;
    }

    auto program = compile(filename);
    if (!program) return nullptr;

    auto obj = std::make_shared<LpcObject>(filename, program);
    loaded_[filename] = obj;

    // A runtime error thrown out of create() (a missing efun, a bad
    // sscanf(), etc.) must fail this one object's load, not crash the
    // whole driver process -- compile()'s own try/catch below only covers
    // the lex/parse/codegen phase, not this call, since compiling a file
    // successfully and having its create() throw are different failures
    // (see the compile-error path's own [object] message for the other
    // one).
    if (vm_) {
        try {
            runObjectVarInitializers(obj, *program);
            vm_->callFunction(obj, "create", {});
        } catch (const std::exception& e) {
            std::cerr << "[object] create() failed for " << filename << ": " << e.what() << "\n";
            loaded_.erase(filename);
            return nullptr;
        }
    }

    return obj;
}

void ObjectManager::runObjectVarInitializers(const std::shared_ptr<LpcObject>& obj,
                                              const CompiledProgram& program) {
    if (!vm_) return;
    for (const auto& parent : program.inheritedPrograms) {
        if (parent) runObjectVarInitializers(obj, *parent);
    }
    vm_->callFunctionInProgram(obj, program, "$objvarinit", {});
}

std::shared_ptr<LpcObject> ObjectManager::cloneObject(const std::string& filename) {
    auto program = compile(filename);
    if (!program) return nullptr;

    auto obj = std::make_shared<LpcObject>(filename, program);

    if (vm_) {
        try {
            runObjectVarInitializers(obj, *program);
            vm_->callFunction(obj, "create", {});
        } catch (const std::exception& e) {
            std::cerr << "[object] create() failed for " << filename << ": " << e.what() << "\n";
            return nullptr;
        }
    }

    return obj;
}

std::shared_ptr<LpcObject> ObjectManager::lookupLoadedObject(const std::string& filename) const {
    auto it = loaded_.find(filename);
    return it != loaded_.end() ? it->second : nullptr;
}

void ObjectManager::destructObject(const std::shared_ptr<LpcObject>& obj) {
    if (!obj) return;
    loaded_.erase(obj->filename());
}

} // namespace lpcdriver
