// SPDX-License-Identifier: GPL-3.0-only
// Part of recomp-common (ForkAde's fork): signing in to the community and reading the lobby's events (community.h).
#include "community.h"

#include <curl/curl.h>

#include <algorithm>
#include <cstdio>

namespace launcher {

namespace {
    // (an HTTP request through libcurl: the status, and the body in `body`; 0 if none came)
    long Http(const std::string& method, const std::string& url, std::string& body) {
        static const bool ready = (curl_global_init(CURL_GLOBAL_DEFAULT), true);
        (void)ready;
        CURL* curl = curl_easy_init();
        if (!curl) {
            return 0;
        }
        std::string got;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "ForkAde-launcher");
        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* data, size_t size, size_t n, void* into) {
            static_cast<std::string*>(into)->append(data, size * n);
            return size * n;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &got);
        long status = 0;
        if (curl_easy_perform(curl) == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        }
        curl_easy_cleanup(curl);
        body = std::move(got);
        return status;
    }

    // A small JSON reader: enough for the service's answers and events (objects, arrays, strings with escapes,
    // numbers, true, false, null)
    struct Json {
        enum Kind { Null, Bool, Number, String, Array, Object } kind = Null;
        bool flag = false;
        double number = 0;
        std::string text;
        std::vector<Json> items;
        std::vector<std::pair<std::string, Json>> fields;

        const Json& operator[](const char* key) const {
            static const Json none;
            for (const auto& [k, v] : fields) {
                if (k == key) {
                    return v;
                }
            }
            return none;
        }
        std::string str() const { return kind == String ? text : std::string(); }
        int64_t integer() const { return kind == Number ? static_cast<int64_t>(number) : 0; }
    };

    struct Reader {
        const std::string& s;
        size_t at = 0;
        bool bad = false;

        void space() {
            while (at < s.size() && (s[at] == ' ' || s[at] == '\n' || s[at] == '\r' || s[at] == '\t')) {
                at++;
            }
        }
        static void utf8(std::string& out, uint32_t c) {
            if (c < 0x80) {
                out += static_cast<char>(c);
            } else if (c < 0x800) {
                out += static_cast<char>(0xC0 | (c >> 6));
                out += static_cast<char>(0x80 | (c & 0x3F));
            } else if (c < 0x10000) {
                out += static_cast<char>(0xE0 | (c >> 12));
                out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (c & 0x3F));
            } else {
                out += static_cast<char>(0xF0 | (c >> 18));
                out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
                out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (c & 0x3F));
            }
        }
        uint32_t hex4() {
            uint32_t v = 0;
            for (int i = 0; i < 4; i++, at++) {
                if (at >= s.size()) {
                    bad = true;
                    return 0;
                }
                const char c = s[at];
                v = v * 16 + static_cast<uint32_t>(c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 0);
            }
            return v;
        }
        std::string string() {
            std::string out;
            at++;  // (the opening quote)
            while (at < s.size() && s[at] != '"') {
                if (s[at] == '\\' && at + 1 < s.size()) {
                    const char e = s[at + 1];
                    at += 2;
                    if (e == 'u') {
                        uint32_t c = hex4();
                        if (c >= 0xD800 && c < 0xDC00 && at + 1 < s.size() && s[at] == '\\' && s[at + 1] == 'u') {
                            at += 2;
                            c = 0x10000 + ((c - 0xD800) << 10) + (hex4() - 0xDC00);
                        }
                        utf8(out, c);
                    } else {
                        out += e == 'n' ? '\n' : e == 't' ? '\t' : e == 'r' ? '\r' : e == 'b' ? '\b' : e == 'f' ? '\f' : e;
                    }
                } else {
                    out += s[at++];
                }
            }
            at++;  // (the closing quote)
            return out;
        }
        Json value(int depth = 0) {
            Json v;
            space();
            if (at >= s.size() || depth > 32) {
                bad = true;
                return v;
            }
            const char c = s[at];
            if (c == '{') {
                v.kind = Json::Object;
                at++;
                space();
                while (at < s.size() && s[at] != '}' && !bad) {
                    space();
                    if (s[at] != '"') {
                        bad = true;
                        break;
                    }
                    std::string key = string();
                    space();
                    if (at < s.size() && s[at] == ':') {
                        at++;
                    }
                    v.fields.emplace_back(std::move(key), value(depth + 1));
                    space();
                    if (at < s.size() && s[at] == ',') {
                        at++;
                    }
                }
                at++;
            } else if (c == '[') {
                v.kind = Json::Array;
                at++;
                space();
                while (at < s.size() && s[at] != ']' && !bad) {
                    v.items.push_back(value(depth + 1));
                    space();
                    if (at < s.size() && s[at] == ',') {
                        at++;
                    }
                }
                at++;
            } else if (c == '"') {
                v.kind = Json::String;
                v.text = string();
            } else if (c == 't' || c == 'f') {
                v.kind = Json::Bool;
                v.flag = c == 't';
                at += c == 't' ? 4 : 5;
            } else if (c == 'n') {
                at += 4;
            } else {
                v.kind = Json::Number;
                size_t used = 0;
                try {
                    v.number = std::stod(s.substr(at, 32), &used);
                } catch (...) {
                    bad = true;
                }
                at += used ? used : 1;
            }
            return v;
        }
    };

    Json ParseJson(const std::string& text) {
        Reader r{text};
        Json v = r.value();
        return r.bad ? Json{} : v;
    }

    std::string Quoted(const std::string& text) {
        std::string out = "\"";
        for (const unsigned char c : text) {
            if (c == '"' || c == '\\') {
                out += '\\';
                out += static_cast<char>(c);
            } else if (c < 0x20) {
                char hex[8];
                std::snprintf(hex, sizeof(hex), "\\u%04x", c);
                out += hex;
            } else {
                out += static_cast<char>(c);
            }
        }
        return out + "\"";
    }

    int Busy(long status, const Json& body) {
        if (status != 429) {
            return 0;
        }
        const int said = static_cast<int>(body["retry"].integer());
        return std::clamp(said > 0 ? said : 10, 1, 600);
    }

    CommunityPerson Person(const Json& j) {
        return {j["nickname"].str(), j["flag"].str(), j["role"].str(), j["state"].str(), j["game"].str()};
    }

    CommunityMessage Message(const Json& j) {
        return {j["mid"].integer(), j["at"].integer(), j["nickname"].str(), j["flag"].str(), j["role"].str(), j["text"].str()};
    }
}  // namespace

SignInRequest signin_request(const std::string& base) {
    SignInRequest r;
    std::string body;
    const long status = Http("POST", base + "/v1/auth/request", body);
    const Json j = ParseJson(body);
    r.busy = Busy(status, j);
    r.id = j["id"].str();
    r.secret = j["secret"].str();
    r.ok = status == 200 && !r.id.empty() && !r.secret.empty();
    return r;
}

std::string signin_page(const std::string& base, const std::string& id) {
    return base + "/v1/auth/discord/start?for=" + id;
}

SignInState signin_poll(const std::string& base, const SignInRequest& request) {
    SignInState s;
    std::string body;
    const long status = Http("GET", base + "/v1/auth/request/" + request.id + "?secret=" + request.secret, body);
    const Json j = ParseJson(body);
    s.busy = Busy(status, j);
    s.state = status == 0 ? std::string() : j["state"].str();
    s.session = j["session"].str();
    s.nickname = j["nickname"].str();
    s.flag = j["flag"].str();
    s.role = j["role"].str();
    return s;
}

CommunityEvent parse_community_event(const std::string& json) {
    CommunityEvent e;
    const Json j = ParseJson(json);
    if (j.kind != Json::Object) {
        return e;
    }
    e.type = j["type"].str();
    e.you = Person(j["you"]);
    for (const Json& p : j["presence"].items) {
        e.presence.push_back(Person(p));
    }
    for (const Json& m : j["history"].items) {
        e.history.push_back(Message(m));
    }
    if (e.type == "message") {
        e.history.push_back(Message(j["message"]));
    }
    e.mid = j["mid"].integer();
    e.minutes = static_cast<int>(j["minutes"].integer());
    e.wait = static_cast<int>(j["wait"].integer());
    e.error = j["error"].str();
    return e;
}

std::string community_say(const std::string& text) {
    return "{\"type\":\"say\",\"text\":" + Quoted(text) + "}";
}

std::string community_state(const std::string& state, const std::string& game) {
    return "{\"type\":\"state\",\"state\":" + Quoted(state) + (state == "playing" && !game.empty() ? ",\"game\":" + Quoted(game) : std::string()) + "}";
}

}  // namespace launcher
