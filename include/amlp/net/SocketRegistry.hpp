#pragma once
#include <memory>
#include <string>
#include <vector>
#include "amlp/vm/Value.hpp"
#include "amlp/net/LpcSocket.hpp"

namespace amlp {

class LpcObject;

// Global table of every live LPC efun socket -- real lpc_socks[] (a
// fixed-size C array in socket_efuns.c) reduced to the same "global map,
// no Server/VM reference needed" shape InteractiveRegistry already uses
// for connections. Owns every LpcSocket's real lifetime (a shared_ptr
// map entry); Server::pollSockets() only ever borrows a raw/shared
// reference to poll and fire callbacks, never outlives a call to
// SocketRegistry::all().
//
// Handle allocation is a monotonic counter, never reused within a
// session -- net/instruct.md's own explicit invariant for this row
// ("Socket handles returned by socket_create are globally unique
// integers (never reused within a session). Use a monotonic counter in
// SocketRegistry.") kept exactly as specified, a deliberate divergence
// from real find_new_socket()'s own array-slot-reuse behavior: nothing
// in this driver's LPC-visible contract depends on handle reuse, and
// reuse would only reintroduce a stale-handle class of bug real FluffOS
// itself has to guard against with owner_ob/state checks on every call.
//
// Known gap, not fixed this session: real close_referencing_sockets(ob)
// (every socket a destructed object still owns force-closes when that
// object is destructed) has no equivalent here. A dangling LpcSocket
// whose owner has been destructed simply stops firing callbacks (its
// weak_ptr<LpcObject> lock() fails, matching PendingInputTo's own
// established handling) but is not itself removed from this registry or
// closed -- it lingers, still holding its fd open, until something else
// (a peer disconnect, an explicit socket_close() from elsewhere, driver
// shutdown) removes it. Flagged here rather than silently left
// undocumented.
class SocketRegistry {
public:
    // int socket_create(int mode, string|function read_callback,
    //                    void|string|function close_callback)
    // Real signature/min-max args confirmed against efun_defs.c's own
    // F_SOCKET_CREATE entry (2-3 args, ret TYPE_NUMBER); mode values are
    // exactly real MUD=0/STREAM=1/DATAGRAM=2/STREAM_BINARY=3/
    // DATAGRAM_BINARY=4 (lib/secure/include/network.h). Returns a new
    // handle (>= 0) on success, or a negative SocketErr code.
    static int create(int mode, Value readCallback, Value closeCallback,
                       const std::shared_ptr<LpcObject>& owner);

    // int socket_bind(int fd, int port, void|string addr)
    // addr omitted (hasAddr false) binds INADDR_ANY, matching real
    // socket_bind()'s own "if (!addr) ... INADDR_ANY" branch.
    static int bind(int handle, int port, const std::string& addr, bool hasAddr,
                     const std::shared_ptr<LpcObject>& caller);

    // int socket_listen(int fd, string|function connect_callback)
    // Real socket_listen()'s own second argument becomes the socket's
    // read_callback (there is no separate "listen callback" slot) --
    // confirmed directly: "set_read_callback(fd, callback);".
    static int listen(int handle, Value callback, const std::shared_ptr<LpcObject>& caller);

    // int socket_accept(int fd, string|function read_callback,
    //                    string|function write_callback)
    // Returns a new handle (>= 0) for the accepted connection, or a
    // negative SocketErr code -- matching real socket_accept()'s own
    // "find_new_socket()"-then-populate shape, not an in-place mutation
    // of the listening socket's own entry.
    static int accept(int handle, Value readCallback, Value writeCallback,
                       const std::shared_ptr<LpcObject>& caller);

    // int socket_connect(int fd, string address, string|function read_callback,
    //                     string|function write_callback)
    // address is real socket_name_to_sin()'s own "host port" form
    // (space-separated, numeric dotted-quad host only -- real FluffOS
    // does no DNS resolution here either, matching this driver's own
    // pre-existing query_ip_name() precedent).
    static int connect(int handle, const std::string& address, Value readCallback,
                        Value writeCallback, const std::shared_ptr<LpcObject>& caller);

    // int socket_write(int fd, mixed message, void|string address)
    // message must be a string -- real STREAM/DATAGRAM T_BUFFER/T_ARRAY
    // forms are not implemented (no buffer type, see LpcSocket.hpp's own
    // comment); a non-string message returns SocketErr::ETypeNotSupp,
    // matching real socket_write()'s own "default: return
    // EETYPENOTSUPP;" rather than throwing.
    static int write(int handle, const Value& message, const std::string& address,
                      bool hasAddress, const std::shared_ptr<LpcObject>& caller);

    // int socket_close(int fd)
    // Real signature: the LPC-visible efun always calls the internal C
    // socket_close(fd, flags) with flags == 0 (no SC_DO_CALLBACK) -- a
    // deliberate LPC-initiated close never fires close_callback, only a
    // driver-detected asynchronous failure does (Server::pollSockets()'s
    // own poll-loop close path, mirroring real SC_DO_CALLBACK's only
    // internal call site being the write-error branch of
    // socket_write_select_handler()). Simplification versus real
    // STATE_FLUSHING: closes the fd immediately even if a partial write
    // is still pending, rather than deferring the close until it drains
    // -- real FluffOS defers close specifically so a blocked write can
    // still finish delivering; this driver accepts dropping any
    // undelivered remainder instead, judged an acceptable simplification
    // for "basics" scope (no live call site anywhere in this mudlib
    // relies on STATE_FLUSHING's drain-before-close behavior).
    static int close(int handle, const std::shared_ptr<LpcObject>& caller);

    // string socket_error(int error)
    // Mirrors real socket_err.c's own error_strings[] table exactly
    // (same order, same text) plus real socket_error()'s own
    // "error = -(error + 1)" index formula.
    static std::string errorString(int error);

    // mixed *socket_status(void|int fd)
    // Real return shape per lib/packages/sockets_spec.c's own doc
    // comment: [fd, state-string, mode-string, local-addr, remote-addr,
    // owner]. fd < 0 (hasFd false) real behavior returns an array of
    // every socket's own status array; this is exposed as a plain Value
    // by the EfunTable.cpp registration, not here.
    static Value statusOne(const LpcSocket& sock);

    static std::shared_ptr<LpcSocket> find(int handle);
    static std::vector<std::shared_ptr<LpcSocket>> all();

    // Used only by Server::pollSockets()'s own poll-detected-failure
    // path (peer EOF, a read/write error, a partial write that can never
    // flush) -- closes the fd and erases the registry entry, but fires
    // no callback itself (no VM access here); the caller reads
    // closeCallback/owner *before* calling this and fires it afterward,
    // the same "read the one-shot state, then act on it" split
    // Connection::takeWindowSizeUpdate() already established.
    static void forceRemove(int handle);
};

}  // namespace amlp
