#include "lpcdriver/efun/EfunTable.hpp"
#include "lpcdriver/core/Errors.hpp"
#include "lpcdriver/vm/VM.hpp"
#include "lpcdriver/object/LpcObject.hpp"
#include "lpcdriver/net/OutputContext.hpp"
#include "lpcdriver/net/Connection.hpp"
#include "lpcdriver/net/InteractiveRegistry.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <random>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

namespace lpcdriver {

EfunTable& EfunTable::instance() {
    static EfunTable table;
    return table;
}

void EfunTable::registerEfun(const std::string& name, EfunFn fn) {
    table_[name] = std::move(fn);
}

Value EfunTable::call(const std::string& name, VM& vm, std::vector<Value>& args) const {
    auto it = table_.find(name);
    if (it == table_.end()) {
        throw LpcRuntimeError("undefined efun: " + name);
    }
    return it->second(vm, args);
}

bool EfunTable::exists(const std::string& name) const {
    return table_.count(name) > 0;
}

namespace {

// Recursive, self-delimiting save format used by the save_object()/
// restore_object() efuns below -- see their own comment for why this
// driver does not attempt to match real FluffOS's own on-disk save
// format. Every value is tagged with a one-character kind and, for the
// variable-length kinds (string/array/mapping), an explicit element/
// byte count, so nested arrays and mappings round-trip without needing
// to escape delimiter characters inside string content.
void serializeValue(std::ostream& out, const Value& v) {
    if (auto* iv = std::get_if<int64_t>(&v.data)) {
        out << 'I' << *iv << ';';
    } else if (auto* dv = std::get_if<double>(&v.data)) {
        out << 'F' << *dv << ';';
    } else if (auto* sv = std::get_if<std::string>(&v.data)) {
        out << 'S' << sv->size() << ':' << *sv;
    } else if (auto* av = std::get_if<std::shared_ptr<Array>>(&v.data)) {
        size_t count = *av ? (*av)->items.size() : 0;
        out << 'A' << count << ':';
        if (*av) {
            for (const auto& item : (*av)->items) serializeValue(out, item);
        }
    } else if (auto* mv = std::get_if<std::shared_ptr<Mapping>>(&v.data)) {
        size_t count = *mv ? (*mv)->entries.size() : 0;
        out << 'M' << count << ':';
        if (*mv) {
            for (const auto& entry : (*mv)->entries) {
                serializeValue(out, entry.first);
                serializeValue(out, entry.second);
            }
        }
    } else {
        // Object references and closures cannot survive a save/restore
        // round trip (a saved object reference has nothing to point to
        // after a reboot) -- real save_object() cannot serialize these
        // either, so this driver just writes void in their place rather
        // than throwing and failing the whole save.
        out << 'N';
    }
}

// Reads one value starting at pos (mutated to just past what was
// consumed), the exact inverse of serializeValue() above.
Value deserializeValue(const std::string& s, size_t& pos) {
    if (pos >= s.size()) return Value{};
    char kind = s[pos++];
    switch (kind) {
        case 'N':
            return Value{};
        case 'I': {
            size_t end = s.find(';', pos);
            int64_t v = std::stoll(s.substr(pos, end - pos));
            pos = end + 1;
            return Value(v);
        }
        case 'F': {
            size_t end = s.find(';', pos);
            double v = std::stod(s.substr(pos, end - pos));
            pos = end + 1;
            return Value(v);
        }
        case 'S': {
            size_t colon = s.find(':', pos);
            size_t len = static_cast<size_t>(std::stoull(s.substr(pos, colon - pos)));
            std::string v = s.substr(colon + 1, len);
            pos = colon + 1 + len;
            return Value(v);
        }
        case 'A': {
            size_t colon = s.find(':', pos);
            size_t count = static_cast<size_t>(std::stoull(s.substr(pos, colon - pos)));
            pos = colon + 1;
            auto arr = std::make_shared<Array>();
            arr->items.reserve(count);
            for (size_t i = 0; i < count; ++i) arr->items.push_back(deserializeValue(s, pos));
            return Value(arr);
        }
        case 'M': {
            size_t colon = s.find(':', pos);
            size_t count = static_cast<size_t>(std::stoull(s.substr(pos, colon - pos)));
            pos = colon + 1;
            auto map = std::make_shared<Mapping>();
            map->entries.reserve(count);
            for (size_t i = 0; i < count; ++i) {
                Value key = deserializeValue(s, pos);
                Value val = deserializeValue(s, pos);
                map->entries.emplace_back(std::move(key), std::move(val));
            }
            return Value(map);
        }
        default:
            throw LpcRuntimeError("restore_object: corrupt save data (unknown value kind)");
    }
}

// Real object.c's save_object() normalization (confirmed live: see
// save_object's own comment on why this exists): strip a trailing
// ".c", strip a trailing ".o" if already present so this stays
// idempotent, then always append ".o".
std::string normalizeSavePath(const std::string& path) {
    std::string result = path;
    if (result.size() >= 2 && result.compare(result.size() - 2, 2, ".c") == 0) {
        result.erase(result.size() - 2);
    }
    if (result.size() >= 2 && result.compare(result.size() - 2, 2, ".o") == 0) {
        result.erase(result.size() - 2);
    }
    return result + ".o";
}

} // namespace

void registerCoreEfuns() {
    auto& t = EfunTable::instance();

    t.registerEfun("write", [](VM&, std::vector<Value>& args) -> Value {
        if (!args.empty() && std::holds_alternative<std::string>(args[0].data)) {
            const std::string& s = std::get<std::string>(args[0].data);
            if (Connection* conn = OutputContext::current()) {
                conn->send(s);
            } else {
                std::cout << s;
            }
        }
        return Value{int64_t{1}};
    });

    // object this_object() -- was a permanent void stub before
    // VM::currentObject() existed (added for input_to()'s own needs);
    // now a direct read of it. Confirmed live: secure/std/login.c's
    // whole account-creation flow sends its prompts via "message(type,
    // text, this_object())".
    t.registerEfun("this_object", [](VM& vm, std::vector<Value>&) -> Value {
        auto ob = vm.currentObject();
        if (!ob) return Value{};
        return Value(ob);
    });

    // object clone_object(string) and its real alias "new" (func_spec.c:
    // "object clone_object _new(string, ...);") -- confirmed live:
    // secure/daemon/master.c's own player_object() calls "new(OB_USER)"
    // to create the actual player object behind a virtual player path.
    auto cloneObjectImpl = [](VM& vm, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            throw LpcRuntimeError("clone_object: expected a string filename argument");
        }
        const std::string& filename = std::get<std::string>(args[0].data);
        auto obj = vm.cloneObject(filename);
        if (!obj) {
            throw LpcRuntimeError("clone_object: failed to load " + filename);
        }
        return Value(obj);
    };
    t.registerEfun("clone_object", cloneObjectImpl);
    t.registerEfun("new", cloneObjectImpl);

    // int sizeof(mixed) -- real FluffOS's sizeof() also measures a
    // string's length (func_spec.c literally defines strlen/strstr... no
    // wait, strlen is its own alias: "int strlen sizeof(string);" --
    // confirmed live: secure/daemon/account_d.c's own "!name ||
    // !sizeof(name)" idiom, used throughout this mudlib as the standard
    // "is this string empty" check, was silently always taking the
    // empty-string branch before this string case existed here, since a
    // missing case fell through to a plain 0 rather than throwing --
    // masked because "0" also happens to be account_exists()'s own
    // correct answer for a brand new account, not because the string
    // branch was actually running).
    auto sizeofImpl = [](VM&, std::vector<Value>& args) -> Value {
        if (!args.empty()) {
            if (auto* arr = std::get_if<std::shared_ptr<Array>>(&args[0].data)) {
                return Value(static_cast<int64_t>(*arr ? (*arr)->items.size() : 0));
            }
            if (auto* map = std::get_if<std::shared_ptr<Mapping>>(&args[0].data)) {
                return Value(static_cast<int64_t>(*map ? (*map)->entries.size() : 0));
            }
            if (auto* str = std::get_if<std::string>(&args[0].data)) {
                return Value(static_cast<int64_t>(str->size()));
            }
        }
        return Value(int64_t{0});
    };
    t.registerEfun("sizeof", sizeofImpl);
    t.registerEfun("strlen", sizeofImpl);

    t.registerEfun("explode", [](VM&, std::vector<Value>& args) -> Value {
        if (args.size() < 2 || !std::holds_alternative<std::string>(args[0].data) ||
            !std::holds_alternative<std::string>(args[1].data)) {
            throw LpcRuntimeError("explode: expected (string, string) arguments");
        }
        const std::string& str = std::get<std::string>(args[0].data);
        const std::string& sep = std::get<std::string>(args[1].data);

        auto result = std::make_shared<Array>();
        if (sep.empty()) {
            result->items.emplace_back(str);
            return Value(result);
        }

        size_t start = 0;
        for (;;) {
            size_t pos = str.find(sep, start);
            if (pos == std::string::npos) {
                result->items.emplace_back(str.substr(start));
                break;
            }
            result->items.emplace_back(str.substr(start, pos - start));
            start = pos + sep.size();
        }
        return Value(result);
    });

    t.registerEfun("keys", [](VM&, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::shared_ptr<Mapping>>(args[0].data)) {
            throw LpcRuntimeError("keys: expected a mapping argument");
        }
        auto map = std::get<std::shared_ptr<Mapping>>(args[0].data);
        auto result = std::make_shared<Array>();
        if (map) {
            for (const auto& entry : map->entries) {
                result->items.push_back(entry.first);
            }
        }
        return Value(result);
    });

    // mixed *values(mapping) -- keys()'s own value-side counterpart,
    // same entry order.
    t.registerEfun("values", [](VM&, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::shared_ptr<Mapping>>(args[0].data)) {
            throw LpcRuntimeError("values: expected a mapping argument");
        }
        auto map = std::get<std::shared_ptr<Mapping>>(args[0].data);
        auto result = std::make_shared<Array>();
        if (map) {
            for (const auto& entry : map->entries) {
                result->items.push_back(entry.second);
            }
        }
        return Value(result);
    });

    // string read_file(string file, void|int start, void|int numLines).
    // Real signature and behavior confirmed against the FluffOS
    // reference driver's file.c: returns 0 (not an error) if the file
    // does not exist, start defaults to 1 (first line) and clamps up to
    // 1, and numLines defaults to "the rest of the file"; a negative
    // numLines also returns 0.
    t.registerEfun("read_file", [](VM& vm, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            throw LpcRuntimeError("read_file: expected a string filename argument");
        }
        int64_t start = 1;
        if (args.size() > 1 && std::holds_alternative<int64_t>(args[1].data)) {
            start = std::get<int64_t>(args[1].data);
        }
        if (start < 1) start = 1;

        int64_t numLines = 0; // 0 means "rest of the file"
        if (args.size() > 2 && std::holds_alternative<int64_t>(args[2].data)) {
            numLines = std::get<int64_t>(args[2].data);
        }
        if (numLines < 0) return Value(int64_t{0});

        std::string path = vm.resolveMudlibPath(std::get<std::string>(args[0].data));
        struct stat st;
        if (::stat(path.c_str(), &st) != 0 || S_ISDIR(st.st_mode)) {
            return Value(int64_t{0});
        }
        std::ifstream f(path);
        if (!f) return Value(int64_t{0});

        // Whole-file read is the common case (every call in this mudlib's
        // early-boot files uses it) and is exactly what a plain slurp
        // already gives, so it skips the line-splitting path entirely.
        if (args.size() <= 1) {
            std::ostringstream buf;
            buf << f.rdbuf();
            return Value(buf.str());
        }

        std::ostringstream result;
        std::string line;
        int64_t lineNo = 0;
        int64_t linesTaken = 0;
        while (std::getline(f, line)) {
            ++lineNo;
            if (lineNo < start) continue;
            if (numLines > 0 && linesTaken >= numLines) break;
            result << line << "\n";
            ++linesTaken;
        }
        return Value(result.str());
    });

    // int write_file(string file, string content, void|int flags).
    // Appends content to the file, creating it if it does not exist
    // (real FluffOS default; flags == 1 truncates first, per file.c) --
    // matching real semantics rather than always-truncate keeps repeated
    // log-style writes (this mudlib's own dominant use, e.g.
    // "write_file(DIR_LOGS+\"/crashes\", ...)") from clobbering earlier
    // entries.
    t.registerEfun("write_file", [](VM& vm, std::vector<Value>& args) -> Value {
        if (args.size() < 2 || !std::holds_alternative<std::string>(args[0].data) ||
            !std::holds_alternative<std::string>(args[1].data)) {
            throw LpcRuntimeError("write_file: expected (string file, string content) arguments");
        }
        bool truncate = args.size() > 2 && std::holds_alternative<int64_t>(args[2].data) &&
                        std::get<int64_t>(args[2].data) == 1;

        std::string path = vm.resolveMudlibPath(std::get<std::string>(args[0].data));
        std::ofstream f(path, truncate ? std::ios::trunc : std::ios::app);
        if (!f) return Value(int64_t{0});
        f << std::get<std::string>(args[1].data);
        return Value(int64_t{1});
    });

    t.registerEfun("call_other", [](VM& vm, std::vector<Value>& args) -> Value {
        if (args.size() < 2) {
            throw LpcRuntimeError("call_other: requires (target, function_name, ...) arguments");
        }
        if (!std::holds_alternative<std::string>(args[1].data)) {
            throw LpcRuntimeError("call_other: second argument must be a string function name");
        }

        std::shared_ptr<LpcObject> target;
        if (std::holds_alternative<std::shared_ptr<LpcObject>>(args[0].data)) {
            target = std::get<std::shared_ptr<LpcObject>>(args[0].data);
            if (!target) {
                throw LpcRuntimeError("call_other: target object is null (destructed?)");
            }
        } else if (std::holds_alternative<std::string>(args[0].data)) {
            // real FluffOS's f__call_other() (efuns_main.c): a string
            // target is resolved with find_object() -- "ob =
            // find_object(arg[0].u.string); if (!ob ...) error(\"call_
            // other() couldn't find object\n\")". find_object() itself
            // (simulate.c) compiles and loads the file on a cache miss
            // rather than only ever looking one up, which is exactly
            // what VM::findObject() wraps ObjectManager::loadObject()
            // to provide -- see both of their own comments. A path that
            // fails to compile, or whose create() throws, still surfaces
            // as this same "couldn't find object" error either way,
            // matching real find_object() returning 0 for either reason.
            target = vm.findObject(std::get<std::string>(args[0].data));
            if (!target) {
                throw LpcRuntimeError("call_other() couldn't find object");
            }
        } else {
            throw LpcRuntimeError("call_other: first argument must be an object or a string path");
        }

        const std::string& functionName = std::get<std::string>(args[1].data);
        std::vector<Value> forwardedArgs(args.begin() + 2, args.end());
        return vm.callFunction(target, functionName, std::move(forwardedArgs));
    });

    // object master() -- func_spec.c: "object master();". Just the
    // already-loaded master object; no different from what
    // VM::applyMaster() already dispatches against.
    t.registerEfun("master", [](VM& vm, std::vector<Value>&) -> Value {
        auto master = vm.masterObject();
        if (!master) return Value{};
        return Value(master);
    });

    // void receive(string) -- efuns_main.c's f_receive(): "if
    // (current_object->interactive) add_message(current_object, ...)".
    // Writes straight to whichever connection is driving the currently
    // executing call, same as this driver's own write() efun already
    // does via OutputContext -- current_object->interactive and "the
    // connection whose OutputContext is active" are the same thing here,
    // since every call into an interactive object's code (logon(),
    // input_to() dispatch, etc) happens with that connection's
    // OutputContext set for its whole duration.
    t.registerEfun("receive", [](VM&, std::vector<Value>& args) -> Value {
        if (!args.empty() && std::holds_alternative<std::string>(args[0].data)) {
            const std::string& s = std::get<std::string>(args[0].data);
            if (Connection* conn = OutputContext::current()) {
                conn->send(s);
            }
        }
        return Value{};
    });

    // int input_to(string func, void|int flags, void|mixed extra_args...)
    // -- simulate.c's input_to()/f_input_to() (efuns_main.c): the flag
    // slot is only consumed when arg[1] is actually a number ("!(arg[1].
    // type == T_NUMBER)" gate in f_input_to), otherwise every argument
    // after the function name is carried over verbatim and handed back
    // as extra leading... no, *trailing* arguments to the callback
    // (simulate.c: "command_giver->interactive->carryover = x", appended
    // after the raw input line -- see comm.c's
    // call_function_interactive()). Registers against
    // OutputContext::current() (this driver's stand-in for
    // command_giver->interactive) and vm.currentObject() (real FluffOS's
    // current_object, "s->ob = current_object").
    //
    // Echo/escape flags (I_NOECHO, I_NOESC, I_SINGLE_CHAR, I_NORMAL) are
    // accepted, positionally consumed exactly like the real efun, and
    // then discarded: this driver does not negotiate telnet echo
    // suppression yet, so there is nothing to apply them to.
    t.registerEfun("input_to", [](VM& vm, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            throw LpcRuntimeError("input_to: expected a string function name as the first argument");
        }

        // "if (!command_giver || ...) return 0" -- simulate.c.
        Connection* conn = OutputContext::current();
        if (!conn) return Value(int64_t{0});

        auto currentObj = vm.currentObject();
        if (!currentObj) return Value(int64_t{0});

        const std::string& function = std::get<std::string>(args[0].data);

        size_t extraStart = 1;
        if (args.size() > 1 && std::holds_alternative<int64_t>(args[1].data)) {
            extraStart = 2;
        }
        std::vector<Value> extraArgs(args.begin() + static_cast<long>(extraStart), args.end());

        conn->setPendingInputTo(currentObj, function, std::move(extraArgs));
        return Value(int64_t{1});
    });

    // int call_out(string|function func, int|float delay, mixed
    // extra_args...) -- registers a pending call, matching the real
    // efun's argument shape (call_out.c/efuns_main.c), including the
    // closure-instead-of-function-name-string form real call_out()
    // itself also accepts (confirmed live in this mudlib: daemon/
    // intermud.c's own "call_out((: Setup :), 2)"). This driver's
    // Scheduler already has the CallOutEntry storage this needs
    // (scheduler/Scheduler.hpp), but Scheduler::tickCallOuts() is still
    // an intentionally empty stub (see STATUS.md's "Known stubs"
    // section) with no wiring yet from EfunTable through to a live
    // Scheduler instance -- registering here would silently never fire.
    // Scoped down to exactly what secure/std/login.c's logon() needs to
    // not throw ("call_out(\"idle\", LOGON_TIMEOUT)"): accept and
    // validate the real argument shape (either form), return a call-out
    // handle, and do nothing else. Actually scheduling call-outs is
    // follow-up work tracked by the existing Scheduler stub, not a new
    // gap introduced here.
    t.registerEfun("call_out", [](VM&, std::vector<Value>& args) -> Value {
        bool validTarget = !args.empty() &&
            (std::holds_alternative<std::string>(args[0].data) ||
             std::holds_alternative<std::shared_ptr<Closure>>(args[0].data));
        if (args.size() < 2 || !validTarget) {
            throw LpcRuntimeError(
                "call_out: expected (string|function target, int|float delay, ...) arguments");
        }
        bool delayIsNumeric = std::holds_alternative<int64_t>(args[1].data) ||
                               std::holds_alternative<double>(args[1].data);
        if (!delayIsNumeric) {
            throw LpcRuntimeError("call_out: second argument must be an int or float delay");
        }
        return Value(int64_t{1});
    });

    // string capitalize(string) -- efuns_main.c's f_capitalize(): the
    // first character uppercased if (and only if) it is currently
    // lowercase; everything else, including an already-uppercase or
    // non-alphabetic first character, is left alone.
    t.registerEfun("capitalize", [](VM&, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            throw LpcRuntimeError("capitalize: expected a string argument");
        }
        std::string s = std::get<std::string>(args[0].data);
        if (!s.empty() && s[0] >= 'a' && s[0] <= 'z') {
            s[0] = static_cast<char>(s[0] - 'a' + 'A');
        }
        return Value(s);
    });

    // string sprintf(string fmt, mixed args...) -- real FluffOS's
    // sprintf() is a large efun (field widths, padding, table columns,
    // a dozen-plus specifiers). Confirmed by grep across secure/std/
    // login.c, secure/daemon/account_d.c, and daemon/banish.c (every
    // file reachable from the login/account-creation path this driver
    // exercises): only bare "%s" (string) and "%d" (int) are ever used,
    // positionally, with no width/precision/flags and no literal "%%".
    // Scoped to exactly that; throws rather than silently mishandling
    // anything else, matching this codebase's existing convention for
    // other partially-implemented efuns.
    t.registerEfun("sprintf", [](VM&, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            throw LpcRuntimeError("sprintf: expected a string format argument");
        }
        const std::string& fmt = std::get<std::string>(args[0].data);
        std::string result;
        size_t argIdx = 1;
        for (size_t i = 0; i < fmt.size(); ++i) {
            if (fmt[i] != '%') {
                result += fmt[i];
                continue;
            }
            if (i + 1 >= fmt.size()) {
                throw LpcRuntimeError("sprintf: trailing '%' with no specifier");
            }
            char spec = fmt[++i];
            if (argIdx >= args.size()) {
                throw LpcRuntimeError("sprintf: too few arguments for format string");
            }
            const Value& argVal = args[argIdx++];
            if (spec == 's') {
                if (!std::holds_alternative<std::string>(argVal.data)) {
                    throw LpcRuntimeError("sprintf: %s argument is not a string");
                }
                result += std::get<std::string>(argVal.data);
            } else if (spec == 'd') {
                if (!std::holds_alternative<int64_t>(argVal.data)) {
                    throw LpcRuntimeError("sprintf: %d argument is not an int");
                }
                result += std::to_string(std::get<int64_t>(argVal.data));
            } else {
                throw LpcRuntimeError(
                    std::string("sprintf: unsupported format specifier '%") + spec +
                    "' (only %s and %d are implemented)");
            }
        }
        return Value(result);
    });

    // void message(mixed type, mixed msg, mixed targets, void|mixed
    // excludes) -- real message() routes msg to one or more objects
    // (or a whole room's inventory, matched by category through
    // catch_tell()) with type used for client-side categorization/
    // filtering this driver does not implement. Confirmed live: every
    // "message(...)" call reachable from secure/std/login.c's account-
    // creation flow uses the same one shape -- a plain string msg sent
    // to a single object target, always "this_object()" (the login
    // object driving the currently active connection). Scoped to
    // exactly that: msg is written straight to the connection actually
    // driving the current call (OutputContext::current(), same as
    // receive()/write()), and type/targets/excludes are accepted for
    // signature compatibility but not otherwise inspected -- there is
    // no reverse "object -> its connection" lookup in this driver to
    // route a message to a *different* object's connection than the
    // one currently active, and nothing on this driver's current path
    // needs one.
    t.registerEfun("message", [](VM&, std::vector<Value>& args) -> Value {
        if (args.size() < 2 || !std::holds_alternative<std::string>(args[1].data)) {
            throw LpcRuntimeError("message: expected (mixed type, string msg, mixed targets, ...) arguments");
        }
        if (Connection* conn = OutputContext::current()) {
            conn->send(std::get<std::string>(args[1].data));
        }
        return Value{};
    });

    // string lower_case(string) -- efuns_main.c's f_lower_case(): every
    // ASCII uppercase letter folded to lowercase, everything else
    // unchanged.
    t.registerEfun("lower_case", [](VM&, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            throw LpcRuntimeError("lower_case: expected a string argument");
        }
        std::string s = std::get<std::string>(args[0].data);
        for (char& c : s) {
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        }
        return Value(s);
    });

    // string replace_string(string str, string pattern, string
    // replacement) -- efuns_main.c's f_replace_string(): every
    // non-overlapping occurrence of pattern replaced left to right (the
    // real efun also takes optional first/last occurrence-index bounds
    // via a 4th/5th argument; nothing in this mudlib's boot/login path
    // uses that form, so it is not implemented here -- matching this
    // codebase's existing convention of throwing rather than silently
    // mishandling an unsupported shape, see sscanf's %f/%x handling).
    // An empty pattern is a no-op in the real efun ("if (!plen) ...
    // just return it").
    t.registerEfun("replace_string", [](VM&, std::vector<Value>& args) -> Value {
        if (args.size() != 3 ||
            !std::holds_alternative<std::string>(args[0].data) ||
            !std::holds_alternative<std::string>(args[1].data) ||
            !std::holds_alternative<std::string>(args[2].data)) {
            throw LpcRuntimeError("replace_string: expected (string, string, string) arguments "
                                   "(occurrence-range form not implemented)");
        }
        const std::string& str = std::get<std::string>(args[0].data);
        const std::string& pattern = std::get<std::string>(args[1].data);
        const std::string& replacement = std::get<std::string>(args[2].data);
        if (pattern.empty()) return Value(str);

        std::string result;
        size_t start = 0;
        for (;;) {
            size_t pos = str.find(pattern, start);
            if (pos == std::string::npos) {
                result.append(str, start, std::string::npos);
                break;
            }
            result.append(str, start, pos - start);
            result += replacement;
            start = pos + pattern.size();
        }
        return Value(result);
    });

    // mixed evaluate(mixed f, mixed extra_args...) and its alias
    // mixed funcall(mixed f, mixed extra_args...) -- efuns_main.c's
    // f__evaluate(), registered under both names in the real driver too
    // (func_spec.c: "mixed evaluate _evaluate(mixed, ...); mixed funcall
    // _evaluate(mixed, ...);"). A non-function first argument is a
    // silent no-op returning void, not an error -- real f__evaluate():
    // "if (arg->type != T_FUNCTION) { pop_n_elems(...); return; }".
    auto evaluateImpl = [](VM& vm, std::vector<Value>& args) -> Value {
        if (args.empty()) return Value{};
        auto* closurePtr = std::get_if<std::shared_ptr<Closure>>(&args[0].data);
        if (!closurePtr || !*closurePtr) return Value{};
        std::vector<Value> extra(args.begin() + 1, args.end());
        return vm.callClosure(*closurePtr, std::move(extra));
    };
    t.registerEfun("evaluate", evaluateImpl);
    t.registerEfun("funcall", evaluateImpl);

    // int functionp(mixed) -- true only for a real Closure value (real
    // FluffOS's f_functionp() also distinguishes several function-
    // pointer sub-kinds via a bitmask return; this driver has only the
    // one kind, so a plain 0/1 is enough for every "if(functionp(x))"
    // truth-check use this mudlib actually makes, e.g. std/Object.c's
    // own query_long()/query_short()).
    t.registerEfun("functionp", [](VM&, std::vector<Value>& args) -> Value {
        bool isFn = !args.empty() &&
            std::holds_alternative<std::shared_ptr<Closure>>(args[0].data) &&
            std::get<std::shared_ptr<Closure>>(args[0].data) != nullptr;
        return Value(static_cast<int64_t>(isFn ? 1 : 0));
    });

    // int objectp(mixed) -- true only for a real (non-null) object
    // reference.
    t.registerEfun("objectp", [](VM&, std::vector<Value>& args) -> Value {
        bool isObj = !args.empty() &&
            std::holds_alternative<std::shared_ptr<LpcObject>>(args[0].data) &&
            std::get<std::shared_ptr<LpcObject>>(args[0].data) != nullptr;
        return Value(static_cast<int64_t>(isObj ? 1 : 0));
    });

    // int stringp(mixed) -- true only for a string value (func_spec.c:
    // "int stringp(mixed);"). Confirmed live: secure/SimulEfun/
    // base_name.c's own "if(stringp(val)) name = val; else name =
    // file_name(val);" -- base_name() accepts either a path string or
    // an object reference.
    t.registerEfun("stringp", [](VM&, std::vector<Value>& args) -> Value {
        bool isStr = !args.empty() && std::holds_alternative<std::string>(args[0].data);
        return Value(static_cast<int64_t>(isStr ? 1 : 0));
    });

    // int mapp(mixed) -- true only for a mapping value.
    t.registerEfun("mapp", [](VM&, std::vector<Value>& args) -> Value {
        bool isMap = !args.empty() && std::holds_alternative<std::shared_ptr<Mapping>>(args[0].data);
        return Value(static_cast<int64_t>(isMap ? 1 : 0));
    });

    // int to_int(string | float | int) -- efuns_main.c's f__to_int(),
    // confirmed against the reference source directly (func_spec.cpp:
    // "int to_int _to_int(string | float | int | buffer);"). Surfaced as
    // undefined during a live boot test on std/user.c's own inherit
    // chain (std/user/more.c, std/living.c, std/user.c itself all call
    // it directly, not just through editor.c, so this is a real,
    // separate gap, not one that resolves itself once the closure chain
    // compiles). This driver has no buffer type (see Value.hpp's
    // ValueVariant), so that case is dropped; the other three match
    // f__to_int() exactly: an int argument passes through unchanged, a
    // float truncates toward zero (real f__to_int() does a plain C
    // "(long) sp->u.real" cast, not round-to-nearest), and a string
    // parses a leading base-10 integer via strtol() semantics, ignoring
    // trailing non-digit garbage and returning 0 for a string with no
    // parseable leading number at all (real f__to_int()'s own comment:
    // "this means to_int(\"10x\") == 10"). Any other argument type
    // throws, matching the declared signature rejecting it under real
    // FluffOS's exact_types argument checking.
    t.registerEfun("to_int", [](VM&, std::vector<Value>& args) -> Value {
        if (args.empty()) return Value(static_cast<int64_t>(0));
        const Value& v = args[0];
        if (std::holds_alternative<int64_t>(v.data)) {
            return Value(std::get<int64_t>(v.data));
        }
        if (std::holds_alternative<double>(v.data)) {
            return Value(static_cast<int64_t>(std::get<double>(v.data)));
        }
        if (std::holds_alternative<std::string>(v.data)) {
            const std::string& s = std::get<std::string>(v.data);
            try {
                size_t consumed = 0;
                long parsed = std::stol(s, &consumed, 10);
                if (consumed == 0) return Value(static_cast<int64_t>(0));
                return Value(static_cast<int64_t>(parsed));
            } catch (const std::exception&) {
                return Value(static_cast<int64_t>(0));
            }
        }
        throw LpcRuntimeError("Bad argument 1 to to_int()");
    });

    // int pointerp(mixed) / int arrayp(mixed) -- real aliases of the
    // same efun (func_spec.c: "int pointerp(mixed); int arrayp
    // pointerp(mixed);") -- true only for an array value.
    auto pointerpImpl = [](VM&, std::vector<Value>& args) -> Value {
        bool isArr = !args.empty() && std::holds_alternative<std::shared_ptr<Array>>(args[0].data);
        return Value(static_cast<int64_t>(isArr ? 1 : 0));
    };
    t.registerEfun("pointerp", pointerpImpl);
    t.registerEfun("arrayp", pointerpImpl);

    // mixed *allocate(int size, void|mixed initial) -- an array of size
    // elements, each set to initial (default int 0, real func_spec.c's
    // own default -- confirmed live: secure/SimulEfun/copy.c's own
    // recursive array copy relies on a freshly allocate()'d array
    // starting at a stable default before each slot is overwritten).
    t.registerEfun("allocate", [](VM&, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<int64_t>(args[0].data)) {
            throw LpcRuntimeError("allocate: expected an int size argument");
        }
        int64_t size = std::get<int64_t>(args[0].data);
        if (size < 0) throw LpcRuntimeError("allocate: size must be non-negative");
        Value initial = args.size() > 1 ? args[1] : Value(int64_t{0});
        auto result = std::make_shared<Array>();
        result->items.assign(static_cast<size_t>(size), initial);
        return Value(result);
    });

    // mapping allocate_mapping(int|mixed* size, void|mixed) -- real
    // FluffOS pre-sizes the mapping's hash table with this hint (and,
    // given an array instead of a plain int, keys the new mapping with
    // each of that array's elements -- func_spec.c's "int | mixed *"
    // first-argument type). Nothing on this driver's current path
    // passes an array here (confirmed by grep), so only the plain-int
    // capacity-hint form is implemented, and the hint itself has no
    // observable effect on this driver's Mapping (a plain vector of
    // entries, not a hash table) -- always just an empty mapping.
    t.registerEfun("allocate_mapping", [](VM&, std::vector<Value>&) -> Value {
        return Value(std::make_shared<Mapping>());
    });

    // string file_name(object default: this_object()) -- efuns_main.c's
    // f_file_name(): "add_slash(sp->u.ob->obname)", a leading '/' added
    // if not already present. This driver's LpcObject::filename() is
    // already stored with a leading slash (every path this driver
    // compiles from -- config paths, inherit targets, clone_object()
    // arguments -- is written that way in this mudlib's own source), so
    // no add_slash-equivalent is needed. Real obname also carries a
    // "#<clone id>" suffix distinguishing multiple clones of the same
    // file; this driver's LpcObject has no clone-id concept (see
    // ObjectManager::cloneObject()), so two clones of the same file are
    // not distinguishable through file_name() here -- nothing this
    // driver runs yet depends on telling them apart this way (base_name()
    // strips any "#<id>" suffix right back off again regardless).
    t.registerEfun("file_name", [](VM& vm, std::vector<Value>& args) -> Value {
        std::shared_ptr<LpcObject> ob;
        if (!args.empty() && std::holds_alternative<std::shared_ptr<LpcObject>>(args[0].data)) {
            ob = std::get<std::shared_ptr<LpcObject>>(args[0].data);
        } else {
            ob = vm.currentObject();
        }
        if (!ob) return Value{};
        return Value(ob->filename());
    });

    // int strsrch(string str, string needle, void|int start) -- first
    // index of needle in str at or after start, or -1. Real func_spec.c
    // also allows needle to be an int char code and a "search
    // backwards" flag; neither is used anywhere on this driver's
    // current boot/login/account-creation path (confirmed by grep), so
    // only the string-needle/forward-search form is implemented, and
    // strstr (a real alias -- func_spec.c: "int strstr strsrch(...);")
    // is registered the same way.
    auto strsrchImpl = [](VM&, std::vector<Value>& args) -> Value {
        if (args.size() < 2 ||
            !std::holds_alternative<std::string>(args[0].data) ||
            !std::holds_alternative<std::string>(args[1].data)) {
            throw LpcRuntimeError("strsrch: expected (string, string, void|int start) arguments");
        }
        const std::string& str = std::get<std::string>(args[0].data);
        const std::string& needle = std::get<std::string>(args[1].data);
        int64_t start = 0;
        if (args.size() > 2 && std::holds_alternative<int64_t>(args[2].data)) {
            start = std::get<int64_t>(args[2].data);
        }
        if (start < 0 || static_cast<size_t>(start) > str.size()) {
            return Value(int64_t{-1});
        }
        size_t pos = str.find(needle, static_cast<size_t>(start));
        return Value(pos == std::string::npos ? int64_t{-1} : static_cast<int64_t>(pos));
    };
    t.registerEfun("strsrch", strsrchImpl);
    t.registerEfun("strstr", strsrchImpl);

    // object previous_object(int idx default: 0) and its -1 ("every
    // frame") form -- see VM::previousObject()/allPreviousObjects()'s
    // own comments for the real semantics this reproduces
    // (efuns_main.c's f_previous_object()).
    t.registerEfun("previous_object", [](VM& vm, std::vector<Value>& args) -> Value {
        int64_t idx = 0;
        if (!args.empty() && std::holds_alternative<int64_t>(args[0].data)) {
            idx = std::get<int64_t>(args[0].data);
        }
        if (idx == -1) {
            auto objs = vm.allPreviousObjects();
            auto arr = std::make_shared<Array>();
            for (auto& o : objs) arr->items.emplace_back(o);
            return Value(arr);
        }
        if (idx < 0) return Value{};
        auto ob = vm.previousObject(static_cast<int>(idx));
        if (!ob) return Value{};
        return Value(ob);
    });

    // void error(string msg) -- raises a real runtime error carrying
    // msg, catchable by catch() exactly like any other LpcRuntimeError
    // this driver already throws internally (func_spec.c: "void error
    // _error(string);").
    t.registerEfun("error", [](VM&, std::vector<Value>& args) -> Value {
        std::string msg = "error";
        if (!args.empty() && std::holds_alternative<std::string>(args[0].data)) {
            msg = std::get<std::string>(args[0].data);
        }
        throw LpcRuntimeError(msg);
    });

    // void set_eval_limit(int x) -- real FluffOS raises (or, with -1,
    // restores) the per-call eval-cost ceiling, guarded so only master()
    // can call it at all (secure/SimulEfun/SimulEfun.c's own wrapper:
    // "if (previous_object() != master()) return;", forwarding to the
    // real efun::set_eval_limit(x) only for master). Confirmed live:
    // master.c's own player_object() brackets a load_object() with
    // "set_eval_limit(1000000000); ... set_eval_limit(-1);" so a slow
    // first compile of a new player file does not trip the ordinary
    // eval-cost guard. This driver's own eval-cost ceiling
    // (VM::run()'s hardcoded 1,000,000-instruction-per-call check) is
    // already far above anything this driver's own test scripts hit,
    // and evalCost_ resets to 0 at the start of every run() call rather
    // than accumulating across nested calls, so there is no real
    // "ceiling" state here to actually raise or lower yet -- this efun
    // is accepted (so callers do not throw "undefined efun") and
    // otherwise a no-op, not wired to change that ceiling.
    t.registerEfun("set_eval_limit", [](VM&, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<int64_t>(args[0].data)) {
            throw LpcRuntimeError("set_eval_limit: expected an int argument");
        }
        return Value{};
    });

    // void destruct(object ob) -- removes ob from the object table
    // (VM::destructObject(), a thin wrapper over the ObjectManager
    // method that already existed for this). Confirmed live:
    // secure/std/login.c's own internal_remove() ends a failed login
    // attempt with "destruct(this_object())" on the login shell itself
    // -- when that happens to be the object bound to the connection
    // actually driving the current call (OutputContext::current()'s own
    // boundObject(), the same connection-lookup approach this driver's
    // other connection-scoped efuns use), this also closes that
    // connection, matching real remove_interactive()'s own end result
    // for a destructed interactive object (nothing left to route
    // further input to). Destructing some other, non-connection-bound
    // object just removes it from the object table, same as real
    // destruct_object() does before backend.c gets around to actually
    // freeing it.
    t.registerEfun("destruct", [](VM& vm, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::shared_ptr<LpcObject>>(args[0].data)) {
            throw LpcRuntimeError("destruct: expected an object argument");
        }
        auto ob = std::get<std::shared_ptr<LpcObject>>(args[0].data);
        if (!ob) return Value{};

        vm.destructObject(ob);

        if (Connection* conn = OutputContext::current()) {
            if (conn->boundObject() == ob) {
                conn->close();
            }
        }
        return Value{};
    });

    // object find_object(string path, void|int compile) / object
    // load_object(string path) -- real func_spec.c: "object
    // find_object(string, int default: 0); object load_object
    // find_object(string, int default: 1);" -- the same underlying
    // lookup, just a different default for the second, normally-hidden
    // "compile a miss" argument depending on which name it is called
    // by. A bare find_object(path) therefore only ever looks (see
    // ObjectManager::lookupLoadedObject()'s own comment for why this is
    // a distinct, deliberately non-compiling lookup from VM::findObject()
    // -- call_other()'s own string-target overload, which always
    // compiles a miss, confirmed separately against simulate.c's own
    // find_object() body); load_object(path), or find_object(path, 1),
    // compiles on a miss.
    auto findObjectImpl = [](VM& vm, std::vector<Value>& args, bool compileDefault) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            throw LpcRuntimeError("find_object: expected a string path argument");
        }
        bool compile = compileDefault;
        if (args.size() > 1 && std::holds_alternative<int64_t>(args[1].data)) {
            compile = std::get<int64_t>(args[1].data) != 0;
        }
        const std::string& path = std::get<std::string>(args[0].data);
        auto ob = compile ? vm.findObject(path) : vm.lookupObject(path);
        if (!ob) return Value{};
        return Value(ob);
    };
    t.registerEfun("find_object", [findObjectImpl](VM& vm, std::vector<Value>& args) -> Value {
        return findObjectImpl(vm, args, false);
    });
    t.registerEfun("load_object", [findObjectImpl](VM& vm, std::vector<Value>& args) -> Value {
        return findObjectImpl(vm, args, true);
    });

    // int time() -- seconds since the Unix epoch, same as the real efun.
    t.registerEfun("time", [](VM&, std::vector<Value>&) -> Value {
        return Value(static_cast<int64_t>(std::time(nullptr)));
    });

    // string ctime(int|void clock) -- real efun wraps C's own ctime(),
    // including its trailing newline; clock defaults to time() (real
    // func_spec.c: "string ctime(int|void);", efuns_main.c's own
    // f_ctime() calls time(0) when no argument was given).
    t.registerEfun("ctime", [](VM&, std::vector<Value>& args) -> Value {
        std::time_t clock = std::time(nullptr);
        if (!args.empty() && std::holds_alternative<int64_t>(args[0].data)) {
            clock = static_cast<std::time_t>(std::get<int64_t>(args[0].data));
        }
        char* s = std::ctime(&clock);
        return Value(std::string(s ? s : ""));
    });

    // int userp(object ob) / int query_once_interactive(object ob) --
    // real aliases (func_spec.c: "int userp(object); int
    // query_once_interactive userp(object);") for "has ob ever been
    // interactive" (O_ONCE_INTERACTIVE, set once and never cleared,
    // unlike interactive()'s own "is it interactive *right now*"
    // O_ONLINE-style check). This driver has no O_ONCE_INTERACTIVE
    // equivalent (InteractiveRegistry only tracks *currently* live
    // connections, cleared on disconnect -- see its own comment), so
    // this is approximated as "is it interactive right now", correct
    // for every object this driver's own login/account-creation path
    // actually calls userp() against (a connection's own login/player
    // object, always still connected at the point it is checked) but
    // not for an object that was once connected and has since
    // disconnected.
    auto userpImpl = [](VM&, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::shared_ptr<LpcObject>>(args[0].data)) {
            return Value(int64_t{0});
        }
        auto ob = std::get<std::shared_ptr<LpcObject>>(args[0].data);
        if (!ob) return Value(int64_t{0});
        for (auto& live : InteractiveRegistry::all()) {
            if (live == ob) return Value(int64_t{1});
        }
        return Value(int64_t{0});
    };
    t.registerEfun("userp", userpImpl);
    t.registerEfun("query_once_interactive", userpImpl);

    // string crypt(string str, string|int salt) -- real efuns_port.c's
    // f_crypt(): if salt is a string of length >= 2, use it directly;
    // otherwise generate a random 8-character salt from the same
    // charset the reference driver uses, then hash via the system's
    // own crypt(3) (this driver links -lcrypt for it -- see
    // CMakeLists.txt). Confirmed live: secure/std/login.c's own
    // confirm_password() calls "crypt(str2, 0)" to hash a new account's
    // chosen password before saving it.
    t.registerEfun("crypt", [](VM&, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            throw LpcRuntimeError("crypt: expected a string argument");
        }
        const std::string& str = std::get<std::string>(args[0].data);
        std::string salt;
        if (args.size() > 1 && std::holds_alternative<std::string>(args[1].data) &&
            std::get<std::string>(args[1].data).size() >= 2) {
            salt = std::get<std::string>(args[1].data);
        } else {
            static const char choice[] =
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ./";
            static const int choiceLen = static_cast<int>(sizeof(choice) - 1);
            static std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<int> dist(0, choiceLen - 1);
            for (int i = 0; i < 8; ++i) salt += choice[static_cast<size_t>(dist(rng))];
        }
        char* result = ::crypt(str.c_str(), salt.c_str());
        if (!result) throw LpcRuntimeError("crypt: system crypt() failed");
        return Value(std::string(result));
    });

    // mixed copy(mixed val) -- deep-copies an array or mapping (breaking
    // aliasing with the original); every other value kind in this
    // driver's Value model (int, float, string, object reference,
    // closure) is already copied by plain C++ value/shared_ptr-handle
    // semantics wherever a Value is copied, so copy() is the identity
    // for those, matching real LPC's own "only arrays/mappings actually
    // alias" behavior.
    t.registerEfun("copy", [](VM&, std::vector<Value>& args) -> Value {
        if (args.empty()) return Value{};
        if (auto* arr = std::get_if<std::shared_ptr<Array>>(&args[0].data)) {
            auto result = std::make_shared<Array>();
            if (*arr) result->items = (*arr)->items;
            return Value(result);
        }
        if (auto* map = std::get_if<std::shared_ptr<Mapping>>(&args[0].data)) {
            auto result = std::make_shared<Mapping>();
            if (*map) result->entries = (*map)->entries;
            return Value(result);
        }
        return args[0];
    });

    // int member_array(mixed needle, string|mixed* haystack, void|int
    // start) -- efuns_main.c's f_member_array(): first matching index,
    // or -1. The optional 4th "search backwards" flag argument is not
    // implemented (nothing in this mudlib's boot/login/account path
    // uses it -- confirmed by grep, matching this codebase's existing
    // convention of throwing rather than silently mishandling an
    // unsupported shape).
    t.registerEfun("member_array", [](VM&, std::vector<Value>& args) -> Value {
        if (args.size() < 2) {
            throw LpcRuntimeError("member_array: expected (needle, haystack, void|int start) arguments");
        }
        int64_t start = 0;
        if (args.size() > 2 && std::holds_alternative<int64_t>(args[2].data)) {
            start = std::get<int64_t>(args[2].data);
        }
        if (start < 0) {
            throw LpcRuntimeError("member_array: start index must be non-negative");
        }

        if (auto* haystackStr = std::get_if<std::string>(&args[1].data)) {
            if (!std::holds_alternative<int64_t>(args[0].data)) {
                throw LpcRuntimeError("member_array: needle must be an int char code when searching a string");
            }
            int64_t code = std::get<int64_t>(args[0].data);
            for (size_t i = static_cast<size_t>(start); i < haystackStr->size(); ++i) {
                if (static_cast<unsigned char>((*haystackStr)[i]) == code) {
                    return Value(static_cast<int64_t>(i));
                }
            }
            return Value(int64_t{-1});
        }

        if (auto* haystackArr = std::get_if<std::shared_ptr<Array>>(&args[1].data)) {
            if (!*haystackArr) return Value(int64_t{-1});
            const auto& items = (*haystackArr)->items;
            for (size_t i = static_cast<size_t>(start); i < items.size(); ++i) {
                if (valuesEqual(items[i], args[0])) {
                    return Value(static_cast<int64_t>(i));
                }
            }
            return Value(int64_t{-1});
        }

        throw LpcRuntimeError("member_array: haystack must be a string or an array");
    });

    // int interactive(object ob) -- true if ob is currently bound to a
    // live connection. Real FluffOS checks "ob->interactive != 0"
    // (efuns_main.c's f_interactive()); this driver's InteractiveRegistry
    // (net/InteractiveRegistry.hpp) is exactly that same membership,
    // populated by Connection::attach()/close().
    t.registerEfun("interactive", [](VM&, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::shared_ptr<LpcObject>>(args[0].data)) {
            return Value(int64_t{0});
        }
        auto ob = std::get<std::shared_ptr<LpcObject>>(args[0].data);
        if (!ob) return Value(int64_t{0});
        for (auto& live : InteractiveRegistry::all()) {
            if (live == ob) return Value(int64_t{1});
        }
        return Value(int64_t{0});
    });

    // string query_ip_number(void|object ob) -- comm.c's real
    // query_ip_number(): "inet_ntoa(ob->interactive->addr.sin_addr)",
    // defaulting to command_giver when ob is omitted. This driver has
    // no separate "command_giver" concept from "the connection driving
    // the current call" (OutputContext::current(), the same stand-in
    // used throughout this driver's other connection-scoped efuns --
    // receive(), input_to()), so the object argument is accepted for
    // signature compatibility but not actually used to look up a
    // *different* connection's address; only the current one's own
    // peer address is queried, via getpeername() on its fd.
    t.registerEfun("query_ip_number", [](VM&, std::vector<Value>&) -> Value {
        Connection* conn = OutputContext::current();
        if (!conn) return Value{};
        sockaddr_in addr{};
        socklen_t len = sizeof(addr);
        if (::getpeername(conn->fd(), reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
            return Value{};
        }
        char buf[INET_ADDRSTRLEN];
        if (!::inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf))) {
            return Value{};
        }
        return Value(std::string(buf));
    });

    // object *users() -- every object currently bound to a live
    // connection (real array.c's f_users(), backed by all_users[]).
    t.registerEfun("users", [](VM&, std::vector<Value>&) -> Value {
        auto result = std::make_shared<Array>();
        for (auto& ob : InteractiveRegistry::all()) {
            result->items.emplace_back(ob);
        }
        return Value(result);
    });

    // object find_player(string name) -- real semantics search a
    // dedicated "living name" hash table populated by
    // set_living_name()/enable_commands() (add_action.c's
    // find_living_object()), not the interactive list directly. This
    // driver implements neither of those, so this instead searches
    // InteractiveRegistry (same set users() returns) and asks each
    // object its own query_name() -- the convention every object this
    // mudlib actually calls find_player() against already follows
    // (secure/std/login.c's own query_name(), later std/user.c's own).
    // An object with no query_name(), or whose query_name() throws or
    // returns something other than a matching string, is silently
    // skipped rather than treated as an error -- matching real
    // find_player()'s own "just doesn't match" outcome for anything not
    // in the living-name table.
    t.registerEfun("find_player", [](VM& vm, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            return Value{};
        }
        const std::string& name = std::get<std::string>(args[0].data);
        for (auto& ob : InteractiveRegistry::all()) {
            Value nameVal;
            try {
                nameVal = vm.callFunction(ob, "query_name", {});
            } catch (const std::exception&) {
                continue;
            }
            if (auto* s = std::get_if<std::string>(&nameVal.data)) {
                if (*s == name) return Value(ob);
            }
        }
        return Value{};
    });

    // int file_size(string file) -- file.c's real file_size(): -1 if the
    // path does not exist, -2 if it is a directory, else the byte size.
    t.registerEfun("file_size", [](VM& vm, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            throw LpcRuntimeError("file_size: expected a string path argument");
        }
        std::string path = vm.resolveMudlibPath(std::get<std::string>(args[0].data));
        struct stat st;
        if (::stat(path.c_str(), &st) != 0) return Value(int64_t{-1});
        if (S_ISDIR(st.st_mode)) return Value(int64_t{-2});
        return Value(static_cast<int64_t>(st.st_size));
    });

    // string *get_dir(string path, void|int flags) -- file.c's real
    // get_dir() supports glob patterns and stat-flag bits; this mudlib's
    // own boot/account-creation usage is always a bare directory path
    // with no glob and no flags (confirmed by grep), so only that shape
    // is implemented: a directory's entry names (excluding "." and
    // ".."), or a single-element array naming the file itself if path
    // is a plain file, or an empty array if nothing matches. Throws
    // rather than silently mishandling if ever called with flags,
    // matching this codebase's existing convention for other partially-
    // implemented efuns.
    t.registerEfun("get_dir", [](VM& vm, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            throw LpcRuntimeError("get_dir: expected a string path argument");
        }
        if (args.size() > 1 && !(std::holds_alternative<int64_t>(args[1].data) &&
                                   std::get<int64_t>(args[1].data) == 0)) {
            throw LpcRuntimeError("get_dir: flags argument not implemented");
        }
        std::string path = vm.resolveMudlibPath(std::get<std::string>(args[0].data));
        auto result = std::make_shared<Array>();

        struct stat st;
        if (::stat(path.c_str(), &st) != 0) return Value(result);

        if (S_ISDIR(st.st_mode)) {
            DIR* dir = ::opendir(path.c_str());
            if (!dir) return Value(result);
            struct dirent* entry;
            while ((entry = ::readdir(dir)) != nullptr) {
                std::string name = entry->d_name;
                if (name == "." || name == "..") continue;
                result->items.emplace_back(name);
            }
            ::closedir(dir);
        } else {
            size_t slash = path.find_last_of('/');
            result->items.emplace_back(slash == std::string::npos ? path : path.substr(slash + 1));
        }
        return Value(result);
    });

    // int rm(string file) -- file.c's real rm()/remove_file(): 1 on
    // success, 0 if the file did not exist or could not be removed.
    t.registerEfun("rm", [](VM& vm, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            throw LpcRuntimeError("rm: expected a string path argument");
        }
        std::string path = vm.resolveMudlibPath(std::get<std::string>(args[0].data));
        return Value(static_cast<int64_t>(::remove(path.c_str()) == 0 ? 1 : 0));
    });

    // int mkdir(string dir) -- 1 on success, 0 on failure (real
    // file.c's mkdir() returns the same 1/0 shape; this driver does not
    // replicate its exact errno-based failure text, only the 1/0
    // result every call site actually branches on).
    t.registerEfun("mkdir", [](VM& vm, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            throw LpcRuntimeError("mkdir: expected a string path argument");
        }
        std::string path = vm.resolveMudlibPath(std::get<std::string>(args[0].data));
        return Value(static_cast<int64_t>(::mkdir(path.c_str(), 0755) == 0 ? 1 : 0));
    });

    // int save_object(string file) / int restore_object(string file) --
    // real FluffOS serializes an object's own non-static, non-nosave
    // variables to a specific on-disk text format (each line "varname
    // value", values written in LPC literal syntax -- see save.c) and
    // restore_object() parses that same format back, matching each line
    // to a same-named variable on the *calling* object (current_object,
    // not some other target). This driver does not implement that exact
    // on-disk format: no other tool needs to read these files, and
    // nothing this driver runs needs to interoperate with a real
    // FluffOS save file, only to round-trip its own. Values are instead
    // serialized with this driver's own simple recursive, self-
    // delimiting format (see serializeValue()/deserializeValue() just
    // below) covering every Value kind this driver has (int, float,
    // string, array, mapping -- arbitrarily nested, not just the flat
    // shapes secure/daemon/account_d.c's own account records happen to
    // use) except object references and closures, which real
    // save_object() cannot serialize either (an object reference saved
    // to disk cannot survive a reboot, and neither real FluffOS nor
    // this driver attempts it). __SAVE_EXTENSION__ (".o") is appended
    // by the *caller* in this mudlib's own code (e.g. account_d.c's
    // account_path()+__SAVE_EXTENSION__), so these two efuns use the
    // path normalized the same way real object.c's save_object() itself
    // normalizes it (confirmed live: daemon/banish.c's own
    // restore_banish() calls "restore_object(SAVE_BANISH)" with no
    // extension at all, relying on the efun to add one -- a real
    // pre-existing daemon/save/banish.o on disk was unreachable through
    // this driver's own restore_object() until this normalization was
    // added, since it only ever tried the literal, extension-less path
    // it was given): strip a trailing ".c" if present, strip a
    // trailing ".o" if *already* present (so this stays idempotent
    // rather than ever producing "name.o.o"), then always append ".o".
    t.registerEfun("save_object", [](VM& vm, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            throw LpcRuntimeError("save_object: expected a string path argument");
        }
        auto obj = vm.currentObject();
        if (!obj) throw LpcRuntimeError("save_object: no current object to save");

        // Matches real save_object(): it does not create missing parent
        // directories either, so a save into one still fails cleanly
        // (ofstream simply won't open) rather than silently landing
        // somewhere else -- callers that need the directory to exist
        // make it themselves first (account_d.c's own ensure_dirs()).
        std::string path = vm.resolveMudlibPath(normalizeSavePath(std::get<std::string>(args[0].data)));
        std::ofstream f(path, std::ios::trunc);
        if (!f) return Value(int64_t{0});

        const auto& names = obj->program().objectVarNames;
        auto& vars = obj->variables();
        for (size_t i = 0; i < names.size() && i < vars.size(); ++i) {
            f << names[i] << '\t';
            serializeValue(f, vars[i]);
            f << '\n';
        }
        return Value(int64_t{1});
    });

    t.registerEfun("restore_object", [](VM& vm, std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0].data)) {
            throw LpcRuntimeError("restore_object: expected a string path argument");
        }
        auto obj = vm.currentObject();
        if (!obj) throw LpcRuntimeError("restore_object: no current object to restore into");

        std::string path = vm.resolveMudlibPath(normalizeSavePath(std::get<std::string>(args[0].data)));
        std::ifstream f(path);
        if (!f) return Value(int64_t{0});

        const auto& names = obj->program().objectVarNames;
        auto& vars = obj->variables();

        std::string line;
        while (std::getline(f, line)) {
            size_t tab = line.find('\t');
            if (tab == std::string::npos) continue;
            std::string name = line.substr(0, tab);

            size_t slot = names.size();
            for (size_t i = 0; i < names.size(); ++i) {
                if (names[i] == name) { slot = i; break; }
            }
            if (slot >= names.size() || slot >= vars.size()) continue;

            size_t pos = tab + 1;
            vars[slot] = deserializeValue(line, pos);
        }
        return Value(int64_t{1});
    });
}

} // namespace lpcdriver
