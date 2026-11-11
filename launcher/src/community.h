// SPDX-License-Identifier: GPL-3.0-only
// Part of recomp-common (ForkAde's fork): the public lobby's socket (ForkAde's discord-community-lobby). A WebSocket
// through libcurl on a thread of its own: the session goes in the Sec-WebSocket-Protocol header (never in the URL),
// text events come in as they happen and are taken by the launcher's frame, lines to send are queued. A dropped
// connection is made again, waiting longer each time (1, 2, 4 ... 30 seconds), unless the service said this session
// may not come back (refused at connect, or closed with 4001, 4003 or 4004).
#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace launcher {

class CommunitySocket {
public:
    enum class State { Off, Connecting, Open, Waiting, Stopped };

    ~CommunitySocket();
    // url: ws:// or wss:// .../v1/community/socket; session: the launcher's, 64 hex digits
    void Start(const std::string& url, const std::string& session);
    void Stop();
    void Send(const std::string& text);    // (queued: sent once open)
    std::vector<std::string> Take();       // (the text events received since the last call)
    State state() const { return state_.load(); }
    // Why it stopped or waits: the close code (4001 sign in again, 4003 revoked, 4004 kicked), or the HTTP status of a
    // refused connection (401), 0 if none
    int reason() const { return reason_.load(); }
    // (the seconds until the next try, while Waiting)
    int retry_in() const { return retry_in_.load(); }

private:
    void Run(std::string url, std::string session);
    // One connection: false if it may not come back (refused, or closed with a code that says so)
    bool Session(void* curl, const std::string& url, const std::string& session, int& backoff);

    std::thread thread_;
    std::atomic<bool> stop_{false};
    std::atomic<State> state_{State::Off};
    std::atomic<int> reason_{0}, retry_in_{0};
    std::mutex mutex_;
    std::deque<std::string> out_;
    std::vector<std::string> in_;
};

}  // namespace launcher
