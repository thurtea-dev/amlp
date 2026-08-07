#include "lpcdriver/net/Server.hpp"
#include "lpcdriver/config/Config.hpp"
#include "lpcdriver/vm/VM.hpp"
#include "lpcdriver/object/ObjectManager.hpp"
#include "lpcdriver/object/LpcObject.hpp"
#include "lpcdriver/net/OutputContext.hpp"
#include "lpcdriver/core/Errors.hpp"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <algorithm>

namespace lpcdriver {

namespace {
bool setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}
}

Server::Server(Config& config, VM& vm, ObjectManager& objects, Scheduler& scheduler)
    : config_(config), vm_(vm), objects_(objects), scheduler_(scheduler) {}

Server::~Server() {
    if (listenFd_ >= 0) {
        ::close(listenFd_);
    }
}

bool Server::listen() {
    listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        std::cerr << "[net] socket() failed: " << std::strerror(errno) << "\n";
        return false;
    }

    int opt = 1;
    ::setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(config_.port()));

    if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[net] bind() failed on port " << config_.port()
                   << ": " << std::strerror(errno) << "\n";
        ::close(listenFd_);
        listenFd_ = -1;
        return false;
    }

    if (::listen(listenFd_, 16) < 0) {
        std::cerr << "[net] listen() failed: " << std::strerror(errno) << "\n";
        ::close(listenFd_);
        listenFd_ = -1;
        return false;
    }

    if (!setNonBlocking(listenFd_)) {
        std::cerr << "[net] warning: failed to set listen socket non-blocking\n";
    }

    std::cout << "[net] listening on port " << config_.port() << "\n";
    return true;
}

void Server::onNewConnection(int clientFd) {
    setNonBlocking(clientFd);
    int one = 1;
    ::setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    auto conn = std::make_shared<Connection>(clientFd);

    OutputContext::set(conn.get());

    // A runtime error out of master->connect() (a bad clone_object(), a
    // missing efun somewhere in the login chain, etc) must fail *this*
    // connection attempt, not the whole driver process -- the same
    // "one object's runtime error is not a process crash" guarantee
    // ObjectManager::loadObject()/cloneObject() already give create(),
    // just not previously extended to this call site. Without this, one
    // player hitting a bug in the login flow silently disconnects every
    // other player on the mud too (confirmed live: attempting
    // /secure/std/login.c before it actually compiled took the whole
    // process down on the very first connection).
    Value result;
    try {
        result = vm_.applyMaster("connect", {});
    } catch (const std::exception& e) {
        OutputContext::set(nullptr);
        std::cerr << "[net] master->connect() failed: " << e.what() << "\n";
        ::close(clientFd);
        return;
    }

    if (!std::holds_alternative<std::shared_ptr<LpcObject>>(result.data)) {
        OutputContext::set(nullptr);
        std::cerr << "[net] master->connect() did not return an object; "
                      "closing connection\n";
        ::close(clientFd);
        return;
    }

    auto loginObj = std::get<std::shared_ptr<LpcObject>>(result.data);
    if (!loginObj) {
        OutputContext::set(nullptr);
        std::cerr << "[net] master->connect() returned a null object\n";
        ::close(clientFd);
        return;
    }

    conn->attach(loginObj);
    std::cout << "[net] connection fd=" << clientFd
               << " bound to " << loginObj->filename() << "\n";

    // Real FluffOS calls logon() on the freshly bound object immediately
    // after binding, before anything else touches the connection --
    // backend.c's logon(): "apply(APPLY_LOGON, ob, 0, ORIGIN_DRIVER);"
    // (zero arguments), invoked from comm.c's new_user_handler() right
    // after "ob->interactive = master_ob->interactive;" and friends. A
    // missing logon() is not an error ("function not existing is no
    // longer fatal" -- backend.c's own comment); findFunctionInChain
    // already makes VM::callFunction() silently return void in that
    // case, so no special-casing is needed here for that part.
    //
    // A *runtime* error inside a defined logon() follows the same
    // per-connection failure isolation as master->connect() above and
    // the per-line dispatch below: it closes only this connection, not
    // the whole driver.
    try {
        vm_.callFunction(loginObj, "logon", {});
    } catch (const std::exception& e) {
        OutputContext::set(nullptr);
        std::cerr << "[net] connection fd=" << clientFd
                   << " logon() failed: " << e.what() << "\n";
        conn->close();
        return;
    }

    OutputContext::set(nullptr);
    connections_.push_back(std::move(conn));
}

void Server::acceptNewConnections() {
    for (;;) {
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        int clientFd = ::accept(listenFd_, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientFd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            if (errno == EINTR) continue;
            std::cerr << "[net] accept() failed: " << std::strerror(errno) << "\n";
            break;
        }
        onNewConnection(clientFd);
    }
}

// Real FluffOS's process_user_command() (comm.c): a pending input_to()
// registration always takes the next raw line ahead of anything else --
// "if (call_function_interactive(ip, user_command)) { goto exit; } ...
// process_input(ip, user_command);" -- call_function_interactive() is
// checked first and, if it consumed the line (a handler was pending),
// process_input() is skipped entirely for that line. Only when nothing
// is pending does process_input() run instead.
void Server::dispatchLine(VM& vm, Connection& conn, const std::string& line) {
    if (conn.hasPendingInputTo()) {
        std::optional<PendingInputTo> pending = conn.takePendingInputTo();
        auto target = pending->object.lock();
        // Real FluffOS's own O_DESTRUCTED check in
        // call_function_interactive(): a handler whose object died
        // before its next line arrived is simply dropped, not an error.
        if (target) {
            std::vector<Value> callArgs;
            callArgs.reserve(1 + pending->extraArgs.size());
            callArgs.emplace_back(line);
            for (auto& extra : pending->extraArgs) callArgs.push_back(extra);
            vm.callFunction(target, pending->function, std::move(callArgs));
        }
        return;
    }

    // comm.c's process_input() (static void process_input(interactive_t*,
    // char*), confirmed by direct reading): if the object's own
    // process_input() apply exists, its *return value* decides what
    // actually reaches parse_command() -- a string return is the line to
    // dispatch instead of the original (real mud-shell/alias/history
    // preprocessing, confirmed live in this mudlib's own std/user/
    // nmsh.c), a truthy non-string return means the input was fully
    // consumed and nothing dispatches at all, and anything else
    // (function genuinely undefined, or returns a falsy number) falls
    // through to dispatching the original line unchanged -- matching
    // real comm.c's "if (!ret) ... parse_command(user_command, ...)" /
    // "if (ret->type == T_STRING) parse_command(buf, ...)" / "if
    // (ret->type != T_NUMBER || !ret->u.number) parse_command(user_command,
    // ...)" three-way branch exactly.
    auto obj = conn.boundObject();
    if (!obj) return;

    std::string toDispatch = line;
    Value processed = vm.callFunction(obj, "process_input", {Value(line)});
    if (std::holds_alternative<std::string>(processed.data)) {
        toDispatch = std::get<std::string>(processed.data);
    } else if (std::holds_alternative<int64_t>(processed.data)) {
        if (std::get<int64_t>(processed.data) != 0) return; // fully consumed
    }
    // monostate (process_input undefined) or any other falsy/non-string
    // return: dispatch the original line, untouched.

    // real parse_command(): matches against command_giver's own action
    // table (see VM::dispatchCommand()'s own comment for the exact
    // add_action/enable_commands semantics this backs). A dispatched-but-
    // unhandled line (no action matched, or every match declined) is not
    // an error at this layer -- real notify_no_command()'s own "what?"
    // style default failure message is a mudlib-level concern
    // (std/living.c's own cmd_hook() already sends one via
    // "if(query_client()) receive(\"<error>\");" plus its own SOUL_D/
    // CHAT_D fallback chain), not something this driver needs to inject
    // on its own.
    vm.dispatchCommand(obj, toDispatch);
}

void Server::handleConnection(Connection& conn) {
    auto lines = conn.pollLines();
    if (lines.empty()) return;

    auto obj = conn.boundObject();
    if (!obj) return;

    OutputContext::set(&conn);
    for (const auto& line : lines) {
        // Same reasoning as onNewConnection()'s own try/catch: a runtime
        // error handling one player's input line must close only that
        // player's connection, not crash the driver out from under
        // everyone else currently connected.
        try {
            dispatchLine(vm_, conn, line);
        } catch (const std::exception& e) {
            std::cerr << "[net] connection fd=" << conn.fd()
                       << " input handling failed: " << e.what() << "\n";
            conn.close();
            break;
        }
    }
    OutputContext::set(nullptr);
}

void Server::pollOnce() {
    if (listenFd_ < 0) return;

    acceptNewConnections();

    for (auto& conn : connections_) {
        if (conn->isOpen()) {
            handleConnection(*conn);
        }
    }

    connections_.erase(
        std::remove_if(connections_.begin(), connections_.end(),
                        [](const std::shared_ptr<Connection>& c) { return c->closed(); }),
        connections_.end());
}

} // namespace lpcdriver
