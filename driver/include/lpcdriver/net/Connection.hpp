#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "lpcdriver/vm/Value.hpp"

namespace lpcdriver {

class LpcObject;

// The per-connection "pending input_to callback" slot -- real FluffOS's
// interactive_t::input_to (a "sentence_t", see comm.c) reduced to just
// what this driver needs: which object registered it (simulate.c's
// input_to(): "s->ob = current_object", a weak_ptr since the object can
// be destructed out from under a still-pending registration, matching
// real FluffOS's own O_DESTRUCTED check in call_function_interactive()),
// which function to call, and any extra arguments captured at
// registration time (simulate.c's "command_giver->interactive->carryover").
struct PendingInputTo {
    std::weak_ptr<LpcObject> object;
    std::string function;
    std::vector<Value> extraArgs;
};

class Connection {
public:
    explicit Connection(int fd);
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    int fd() const { return fd_; }
    bool isOpen() const { return fd_ >= 0; }

    // Registers the object in InteractiveRegistry (real FluffOS's
    // all_users[]/users() and find_player() need to find it later) in
    // addition to just recording it here.
    void attach(std::shared_ptr<LpcObject> obj);
    std::shared_ptr<LpcObject> boundObject() const { return boundObject_; }

    void send(const std::string& data);
    std::vector<std::string> pollLines();

    bool closed() const { return closed_; }
    void close();

    // Registers/overwrites the pending input_to handler for this
    // connection (real FluffOS's set_call(), simulate.c).
    void setPendingInputTo(std::shared_ptr<LpcObject> obj, std::string function,
                            std::vector<Value> extraArgs);
    bool hasPendingInputTo() const { return pendingInputTo_.has_value(); }

    // Returns and clears the pending handler in one step. Real FluffOS's
    // call_function_interactive() clears interactive_t::input_to *before*
    // invoking the registered function specifically so that function is
    // free to call input_to() again itself to set up the next prompt
    // (comm.c: "We must [clear] all references to input_to fields before
    // the call to apply(), because someone might want to set up a new
    // input_to()") -- returning-and-clearing atomically here gives
    // callers that same ordering for free.
    std::optional<PendingInputTo> takePendingInputTo();

private:
    int fd_;
    std::string inputBuffer_;
    std::shared_ptr<LpcObject> boundObject_;
    bool closed_ = false;
    std::optional<PendingInputTo> pendingInputTo_;
};

} // namespace lpcdriver
