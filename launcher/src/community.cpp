// SPDX-License-Identifier: GPL-3.0-only
// Part of recomp-common (ForkAde's fork): the public lobby's socket (community.h).
#include "community.h"

#include <curl/curl.h>

#include <algorithm>
#include <chrono>

namespace launcher {

namespace {
    // (libcurl's global setup, once)
    void GlobalInit() {
        static const bool done = (curl_global_init(CURL_GLOBAL_DEFAULT), true);
        (void)done;
    }
}  // namespace

CommunitySocket::~CommunitySocket() {
    Stop();
}

void CommunitySocket::Start(const std::string& url, const std::string& session) {
    Stop();
    stop_ = false;
    reason_ = 0;
    state_ = State::Connecting;
    thread_ = std::thread([this, url, session] { Run(url, session); });
}

void CommunitySocket::Stop() {
    stop_ = true;
    if (thread_.joinable()) {
        thread_.join();
    }
    if (state_ != State::Stopped) {
        state_ = State::Off;
    }
}

void CommunitySocket::Send(const std::string& text) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (out_.size() < 64) {
        out_.push_back(text);
    }
}

std::vector<std::string> CommunitySocket::Take() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> taken;
    taken.swap(in_);
    return taken;
}

void CommunitySocket::Run(std::string url, std::string session) {
    GlobalInit();
    int backoff = 1;
    while (!stop_) {
        state_ = State::Connecting;
        CURL* curl = curl_easy_init();
        bool again = curl && Session(curl, url, session, backoff);
        if (curl) {
            curl_easy_cleanup(curl);
        }
        if (stop_) {
            break;
        }
        if (!again) {
            state_ = State::Stopped;
            return;
        }
        // (waiting longer each time, in steps short enough to stop at once)
        state_ = State::Waiting;
        for (int left = backoff * 10; left > 0 && !stop_; left--) {
            retry_in_ = (left + 9) / 10;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        backoff = std::min(backoff * 2, 30);
    }
}

bool CommunitySocket::Session(void* handle, const std::string& url, const std::string& session, int& backoff) {
    CURL* curl = static_cast<CURL*>(handle);
    curl_slist* headers = curl_slist_append(nullptr, ("Sec-WebSocket-Protocol: forkade.v1, s." + session).c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);  // (the WebSocket handshake, then curl_ws_send/recv)
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ForkAde-launcher");
    // (the certificates from Windows' own store: the libcurl beside the launcher has no bundle of its own on a player's PC)
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, static_cast<long>(CURLSSLOPT_NATIVE_CA));
    const CURLcode connected = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    if (connected != CURLE_OK) {
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        reason_ = static_cast<int>(status);
        // (refused: this session may not come back; anything else: the network, tried again)
        return !(status == 401 || status == 403);
    }
    state_ = State::Open;
    reason_ = 0;
    backoff = 1;
    std::string message;
    char buffer[4096];
    auto last = std::chrono::steady_clock::now();
    while (!stop_) {
        bool busy = false;
        // What came: text gathered over its fragments; a close says why
        size_t got = 0;
        const curl_ws_frame* meta = nullptr;
        const CURLcode read = curl_ws_recv(curl, buffer, sizeof(buffer), &got, &meta);
        if (read == CURLE_OK && meta) {
            busy = true;
            if (meta->flags & CURLWS_CLOSE) {
                const int code = got >= 2 ? (static_cast<unsigned char>(buffer[0]) << 8 | static_cast<unsigned char>(buffer[1])) : 1005;
                reason_ = code;
                return !(code == 4001 || code == 4003 || code == 4004);
            }
            if (meta->flags & CURLWS_TEXT) {
                message.append(buffer, got);
                if (meta->bytesleft == 0 && !(meta->flags & CURLWS_CONT)) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    if (in_.size() < 1024) {
                        in_.push_back(std::move(message));
                    }
                    message.clear();
                }
            }
        } else if (read != CURLE_AGAIN) {
            reason_ = 0;
            return true;  // (the connection dropped: made again)
        }
        // What to send
        std::string next;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!out_.empty()) {
                next = std::move(out_.front());
                out_.pop_front();
            }
        }
        if (!next.empty()) {
            busy = true;
            size_t sent = 0;
            if (curl_ws_send(curl, next.data(), next.size(), &sent, 0, CURLWS_TEXT) != CURLE_OK) {
                return true;
            }
        }
        // (a ping now and then keeps routers from dropping a quiet connection)
        const auto now = std::chrono::steady_clock::now();
        if (now - last > std::chrono::seconds(25)) {
            last = now;
            size_t sent = 0;
            curl_ws_send(curl, "", 0, &sent, 0, CURLWS_PING);
        }
        if (!busy) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    size_t sent = 0;
    const unsigned char bye[2] = {0x03, 0xe8};  // (1000: normal)
    curl_ws_send(curl, bye, sizeof(bye), &sent, 0, CURLWS_CLOSE);
    return false;
}

}  // namespace launcher
