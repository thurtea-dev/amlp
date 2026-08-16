#pragma once
#include <ctime>
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

// The per-connection "pending notify_fail message" slot -- real
// FluffOS's interactive_t::default_err_message (a "union string_or_func",
// comm.h), set by notify_fail(string|function) and consulted only if
// command dispatch for the current input line ends with no add_action
// handler ultimately claiming it (real notify_no_command(), add_action.c
// -- see Server::dispatchLine()'s own wiring for exactly where this
// driver mirrors that). A plain Value already covers both real forms (a
// string, or a function/Closure) without needing a bespoke union the
// way real FluffOS does.

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

    // Real set_call()'s own "if (flags & I_NOECHO) add_binary_message(ob,
    // telnet_yes_echo, ...)" (comm.c): sends IAC WILL ECHO immediately
    // (telling the client the server is taking over echo, so the client
    // stops local-echoing -- real telnet's own confusing-but-correct
    // "WILL ECHO" direction) and marks the connection so the next line
    // pulled in pollLines() re-enables it. Idempotent: calling this again
    // while already suppressed does not resend the negotiation bytes,
    // matching set_call()'s own single-fire-per-registration behavior
    // (only the input_to() efun call site, gated on the flag, decides
    // when this runs at all).
    void suppressEcho();

    int terminalWidth() const { return terminalWidth_; }
    int terminalHeight() const { return terminalHeight_; }

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

    // Real notify_fail()'s own "clear_notify() first, then store"
    // (add_action.c): overwriting is sufficient here since a plain
    // Value assignment already releases whatever was set before, no
    // separate clear step needed the way real FluffOS's own manual
    // ref-counting requires.
    void setPendingNotifyFail(Value message) { pendingNotifyFail_ = std::move(message); }

    // Real notify_no_command()'s own consult-and-clear -- one-shot, the
    // same "return the optional and reset the slot" shape
    // takePendingInputTo() already uses just above.
    std::optional<Value> takePendingNotifyFail() {
        std::optional<Value> result = std::move(pendingNotifyFail_);
        pendingNotifyFail_.reset();
        return result;
    }

    // Real clear_notify(): called unconditionally at the very start of
    // every new input line's own dispatch (process_user_command(),
    // comm.c), before even checking for a pending input_to() handler --
    // a notify_fail() set during an earlier, unrelated dispatch must
    // never leak into a later one.
    void clearPendingNotifyFail() { pendingNotifyFail_.reset(); }

    // Real comm.c's own "ip->last_time = current_time" -- set once when
    // the interactive struct is first set up (new_user(), the same
    // moment this driver constructs a Connection) and then re-set every
    // time a full command line is pulled off the buffer
    // (get_user_command(), before process_user_command() even runs --
    // see Server::dispatchLine()'s own call to this). query_idle()
    // (EfunTable.cpp) reads it back as "current_time - last_time".
    void touchActivity() { lastActivityTime_ = std::time(nullptr); }
    std::time_t lastActivityTime() const { return lastActivityTime_; }

private:
    // Telnet IAC state machine (Phase 0.8), confirmed directly against
    // fluffos-2.9-ds2.08/comm.c's own copy_chars() byte-by-byte states
    // before implementing (that real function is what net/instruct.md's
    // own "telnet_neg()" citation actually refers to -- no function by
    // that name exists anywhere in this vendored source; another stale
    // instruct.md citation, corrected here rather than silently trusted).
    // Persistent across pollLines() calls so a telnet sequence split
    // across two separate TCP reads still parses correctly.
    enum class TelnetState { Data, Iac, Will, Wont, Do, Dont, Sb, SbIac };
    void processTelnetBytes(const std::string& raw, std::string& plainOut);
    void handleNegotiation(TelnetState kind, unsigned char option);
    void handleSubnegotiation();

    int fd_;
    std::string inputBuffer_;
    std::shared_ptr<LpcObject> boundObject_;
    bool closed_ = false;
    std::optional<PendingInputTo> pendingInputTo_;
    std::optional<Value> pendingNotifyFail_;
    std::time_t lastActivityTime_ = 0;
    TelnetState telnetState_ = TelnetState::Data;
    std::string sbBuffer_;
    bool echoSuppressed_ = false;
    int terminalWidth_ = 0;
    int terminalHeight_ = 0;
};

} // namespace lpcdriver
