#include "lpcdriver/net/Connection.hpp"
#include "lpcdriver/net/InteractiveRegistry.hpp"
#include <unistd.h>
#include <errno.h>
#include <cstring>

namespace lpcdriver {

Connection::Connection(int fd) : fd_(fd) {}

Connection::~Connection() {
    close();
}

void Connection::attach(std::shared_ptr<LpcObject> obj) {
    if (boundObject_) InteractiveRegistry::remove(boundObject_);
    boundObject_ = std::move(obj);
    if (boundObject_) InteractiveRegistry::add(boundObject_);
}

void Connection::close() {
    if (boundObject_) {
        InteractiveRegistry::remove(boundObject_);
        boundObject_.reset();
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    closed_ = true;
}

void Connection::send(const std::string& data) {
    if (fd_ < 0) return;
    size_t total = 0;
    while (total < data.size()) {
        ssize_t n = ::write(fd_, data.data() + total, data.size() - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (n == 0) break;
        total += static_cast<size_t>(n);
    }
}

std::vector<std::string> Connection::pollLines() {
    std::vector<std::string> lines;
    if (fd_ < 0) return lines;

    char buf[4096];
    for (;;) {
        ssize_t n = ::read(fd_, buf, sizeof(buf));
        if (n > 0) {
            inputBuffer_.append(buf, static_cast<size_t>(n));
            continue;
        }
        if (n == 0) {
            closed_ = true;
            break;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        closed_ = true;
        break;
    }

    size_t pos;
    while ((pos = inputBuffer_.find('\n')) != std::string::npos) {
        std::string line = inputBuffer_.substr(0, pos);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(std::move(line));
        inputBuffer_.erase(0, pos + 1);
    }

    return lines;
}

void Connection::setPendingInputTo(std::shared_ptr<LpcObject> obj, std::string function,
                                    std::vector<Value> extraArgs) {
    pendingInputTo_ = PendingInputTo{std::move(obj), std::move(function), std::move(extraArgs)};
}

std::optional<PendingInputTo> Connection::takePendingInputTo() {
    std::optional<PendingInputTo> result = std::move(pendingInputTo_);
    pendingInputTo_.reset();
    return result;
}

} // namespace lpcdriver
