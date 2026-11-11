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

// ---- Signing in (the service's /v1/auth/request): a request the browser completes through Discord, then asked about
// with its secret until it says how it went. `base` is the service's address ("https://..."; no trailing slash).
struct SignInRequest {
    bool ok = false;
    std::string id, secret;
    int busy = 0; // (429: the seconds to wait)
};
SignInRequest signin_request(const std::string& base);
// (where the browser is sent for this request)
std::string signin_page(const std::string& base, const std::string& id);
struct SignInState {
    // "pending" (not done in the browser yet), "waiting" (asked to join, not approved), "not-member", "done" (with the
    // session and who), "expired", or "" (no answer: the network)
    std::string state, session, nickname, flag, role;
    int busy = 0;
};
SignInState signin_poll(const std::string& base, const SignInRequest& request);

// ---- The lobby's events, as the service sends them (JSON): only what the launcher shows
struct CommunityPerson {
    std::string nickname, flag, role, state;  // (state: "lobby", "playing", "watching")
    std::string game;                         // (playing: the game's id, when said; community-three-panel-layout)
    std::string watch;                        // (playing a match open to spectators: its watch id; match-spectators)
};
struct CommunityMessage {
    int64_t mid = 0, at = 0;  // (at: milliseconds since 1970, UTC)
    std::string nickname, flag, role, text;
};
struct CommunityEvent {
    // "hello" (you, presence, history), "message" (message), "presence" (presence), "deleted" (mid), "muted"
    // (minutes), "refused" (error, wait), or "" (not understood)
    std::string type;
    CommunityPerson you;
    std::vector<CommunityPerson> presence;
    std::vector<CommunityMessage> history;  // (hello: the history; message: the one message)
    int64_t mid = 0;
    int minutes = 0, wait = 0;
    std::string error;
};
CommunityEvent parse_community_event(const std::string& json);
// A chat line as the service takes it ({"type":"say","text":...}), and the player's state
std::string community_say(const std::string& text);
// (game: the id of the game being played, sent with "playing" only; the service keeps [a-z0-9_]{1,24})
// (watch: the match's watch id, from the match service, with a game only)
std::string community_state(const std::string& state, const std::string& game = "", const std::string& watch = "");

}  // namespace launcher
