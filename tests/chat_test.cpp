// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy and ForkAde contributors. Part of recomp-common (ForkAde's fork).
//
// The in-game chat (src/chat.h) against ForkAde's test vectors (tests/chat-vectors.txt, a copy of ForkAde's
// docs/chat-vectors.txt), two channels over a network that loses, repeats and reorders, the log, and the 5x7
// font's Spanish letters (src/text.h) against tests/chat-text.golden.
//
//   build/chat_test [VECTORS] [GOLDEN]        (--write-golden GOLDEN: write the picture instead)
#include "chat.h"
#include "text.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

using namespace recomp::chat;

static int failures = 0;
#define CHECK(x)                                                                    \
    do {                                                                            \
        if (!(x)) {                                                                 \
            std::fprintf(stderr, "%s:%d: FAILED: %s\n", __FILE__, __LINE__, #x);    \
            failures++;                                                             \
        }                                                                           \
    } while (0)

static std::string Unhex(const std::string &hex) {
    std::string out;
    if (hex == "-") {
        return out;
    }
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        out += static_cast<char>(std::stoi(hex.substr(i, 2), nullptr, 16));
    }
    return out;
}

static std::vector<uint64_t> Numbers(const std::string &list) {
    std::vector<uint64_t> out;
    if (list == "-") {
        return out;
    }
    std::stringstream in(list);
    std::string one;
    while (std::getline(in, one, ',')) {
        out.push_back(std::stoull(one));
    }
    return out;
}

// What the font draws for a text, as the reference says: the character itself, or "?"
static std::string Drawn(const std::string &text) {
    std::string out;
    for (size_t at = 0, used = 0; at < text.size(); at += used) {
        const uint32_t code = recomp::text::NextChar(text.data() + at, text.size() - at, used);
        bool known = code >= 32 && code <= 126;
        for (const auto &extra : recomp::text::kSpanish) {
            known = known || extra.code == code;
        }
        out += known ? text.substr(at, used) : "?";
    }
    return out;
}

static int Vectors(const char *path) {
    std::ifstream file(path);
    if (!file) {
        std::fprintf(stderr, "chat_test: no vectors at %s\n", path);
        failures++;
        return 0;
    }
    int cases = 0;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::stringstream in(line);
        std::string kind, a, b, c;
        in >> kind >> a >> b >> c;
        cases++;
        if (kind == "clean") {
            std::string out;
            const bool ok = Clean(Unhex(b), std::stoul(a), out);
            CHECK(ok == (c != "!"));
            if (ok && c != "!") {
                CHECK(out == Unhex(c));
            }
            if (!(ok == (c != "!") && (!ok || out == Unhex(c)))) {
                std::fprintf(stderr, "  (clean %s %s)\n", a.c_str(), b.c_str());
            }
        } else if (kind == "drawn") {
            CHECK(Drawn(Unhex(a)) == Unhex(b));
        } else if (kind == "line") {
            const std::string payload = Unhex(a);
            uint32_t number = 0;
            std::string text;
            const bool ok = DecodeLine(reinterpret_cast<const uint8_t *>(payload.data()), payload.size(), number, text);
            CHECK(ok == (b != "!"));
            if (ok && b != "!") {
                CHECK(number == std::stoul(b));
                CHECK(text == Unhex(c));
                // (and encoded again: the same bytes, any extra ones aside)
                const std::vector<uint8_t> again = EncodeLine(number, text);
                CHECK(std::string(again.begin(), again.end()) == payload.substr(0, again.size()));
            }
        } else if (kind == "ack") {
            const std::string payload = Unhex(a);
            uint32_t count = 0;
            const bool ok = DecodeAck(reinterpret_cast<const uint8_t *>(payload.data()), payload.size(), count);
            CHECK(ok == (b != "!"));
            if (ok && b != "!") {
                CHECK(count == std::stoul(b));
                const std::vector<uint8_t> again = EncodeAck(count);
                CHECK(std::string(again.begin(), again.end()) == payload.substr(0, 4));
            }
        } else if (kind == "hello") {
            const std::string hello = Unhex(a);
            std::vector<uint8_t> compared;
            std::string name;
            SplitHello(std::vector<uint8_t>(hello.begin(), hello.end()), compared, name);
            CHECK(std::string(compared.begin(), compared.end()) == Unhex(b));
            CHECK(name == Unhex(c));
        } else if (kind == "recv") {
            // (a channel given lines with these numbers: those it takes, and the counts it acknowledges)
            std::vector<uint64_t> acks;
            Channel channel([&](char k, const std::vector<uint8_t> &payload) {
                uint32_t count = 0;
                if (k == 'A' && DecodeAck(payload.data(), payload.size(), count)) {
                    acks.push_back(count);
                }
            });
            std::vector<uint64_t> taken;
            uint64_t now = 0;
            for (const uint64_t number : Numbers(a)) {
                const std::vector<uint8_t> payload = EncodeLine(static_cast<uint32_t>(number), "n" + std::to_string(number));
                channel.Receive('C', payload.data(), payload.size(), now += 10000); // (apart: the rate aside)
                for (const std::string &heard : channel.TakeHeard()) {
                    taken.push_back(std::stoull(heard.substr(1)));
                }
            }
            CHECK(taken == Numbers(b));
            CHECK(acks == Numbers(c));
        } else if (kind == "rate") {
            Rate rate;
            std::string flags;
            for (const uint64_t t : Numbers(a)) {
                flags += rate.Allow(t) ? 'y' : 'n';
            }
            CHECK(flags == b);
        } else {
            std::fprintf(stderr, "chat_test: unknown case %s\n", kind.c_str());
            failures++;
        }
    }
    return cases;
}

// Two channels over a network that loses, repeats and reorders packets: every line arrives once, in order
static void Network(unsigned seed) {
    std::mt19937 random(seed);
    struct Packet {
        int to;
        char kind;
        std::vector<uint8_t> payload;
        uint64_t at;
    };
    std::vector<Packet> flying;
    uint64_t now = 1000;
    auto sender = [&](int to) {
        return [&, to](char kind, const std::vector<uint8_t> &payload) {
            const int roll = static_cast<int>(random() % 100);
            if (roll < 30) {
                return; // (lost)
            }
            const uint64_t delay = 20 + random() % 200; // (and so out of order)
            flying.push_back({to, kind, payload, now + delay});
            if (roll >= 85) {
                flying.push_back({to, kind, payload, now + delay + random() % 300}); // (and again)
            }
        };
    };
    Channel side[2] = {Channel(sender(1)), Channel(sender(0))};
    const std::vector<std::string> said[2] = {{"brb bathroom", "¿otra? ¡vámonos, niño!", "gg", "one more", "last"},
                                              {"rematch?", "ok", "ñandú"}};
    std::vector<std::string> heard[2];
    size_t next[2] = {0, 0};
    for (int step = 0; step < 4000; step++, now += 16) {
        for (int s = 0; s < 2; s++) {
            if (next[s] < said[s].size() && step % 100 == 10 * s + 5) {
                if (side[s].Say(said[s][next[s]], now) == Channel::Said::Sent) {
                    next[s]++;
                }
            }
            side[s].Pump(now);
        }
        std::vector<Packet> due;
        std::vector<Packet> later;
        for (Packet &p : flying) {
            (p.at <= now ? due : later).push_back(std::move(p));
        }
        flying.swap(later);
        std::shuffle(due.begin(), due.end(), random);
        for (const Packet &p : due) {
            side[p.to].Receive(p.kind, p.payload.data(), p.payload.size(), now);
        }
        for (int s = 0; s < 2; s++) {
            for (std::string &line : side[s].TakeHeard()) {
                heard[s].push_back(line);
            }
        }
    }
    CHECK(heard[1] == said[0]);
    CHECK(heard[0] == said[1]);
    CHECK(side[0].Unacknowledged().empty());
    CHECK(side[1].Unacknowledged().empty());
}

static void Sending() {
    std::vector<std::vector<uint8_t>> sent;
    Channel channel([&](char, const std::vector<uint8_t> &payload) { sent.push_back(payload); });
    std::string line;
    CHECK(channel.Say("   ", 0) == Channel::Said::Empty);
    CHECK(channel.Say("ok\xff", 0) == Channel::Said::Empty);
    CHECK(sent.empty());
    CHECK(channel.Say("  hola  ", 100, &line) == Channel::Said::Sent && line == "hola");
    CHECK(channel.Say("dos", 200) == Channel::Said::Sent);
    CHECK(channel.Say("tres", 300) == Channel::Said::Sent);
    CHECK(channel.Say("cuatro", 400) == Channel::Said::Wait); // (over the rate: kept in the input)
    CHECK(channel.Say("cuatro", 5100) == Channel::Said::Sent);
    CHECK(channel.Unacknowledged().size() == 4);
    const size_t before = sent.size();
    channel.Pump(5200); // (not yet: 250 ms after the last sending)
    CHECK(sent.size() == before);
    channel.Pump(5350);
    CHECK(sent.size() == before + 4);
    const std::vector<uint8_t> ack = EncodeAck(2);
    channel.Receive('A', ack.data(), ack.size(), 5400);
    CHECK((channel.Unacknowledged() == std::vector<std::string>{"tres", "cuatro"}));
    // (a long line: 60 characters, cut at a character, not a byte)
    std::string many;
    for (int i = 0; i < 70; i++) {
        many += "ñ";
    }
    CHECK(channel.Say(many, 20000, &line) == Channel::Said::Sent && line.size() == 120);
    // (a peer over the rate: lines taken and acknowledged, but not shown)
    std::vector<uint32_t> acks;
    Channel receiver([&](char kind, const std::vector<uint8_t> &payload) {
        uint32_t count = 0;
        if (kind == 'A' && DecodeAck(payload.data(), payload.size(), count)) {
            acks.push_back(count);
        }
    });
    for (uint32_t n = 0; n < 5; n++) {
        const std::vector<uint8_t> payload = EncodeLine(n, "spam");
        receiver.Receive('C', payload.data(), payload.size(), 1000 + n);
    }
    CHECK(receiver.TakeHeard().size() == 3);
    CHECK((acks == std::vector<uint32_t>{1, 2, 3, 4, 5}));
}

static void Logging() {
    namespace fs = std::filesystem;
    const fs::path folder = fs::temp_directory_path() / "recomp-chat-test" / u8"Señal de prueba";
    std::error_code error;
    fs::remove_all(folder.parent_path(), error);
    const auto u8 = folder.u8string();
    const std::string dir(u8.begin(), u8.end());
    {
        Log silent;
        silent.Start(dir, "kof98", "0.2.0", true);
    }
    CHECK(!fs::exists(folder)); // (a match with no messages: no file, not even the folder)
    std::string path;
    {
        Log log;
        log.Start(dir, "kof98", "0.2.0", true);
        log.Names("Daniel", "Rodrigo");
        log.Sent("Daniel", "¿otra?");
        log.Received("Rodrigo", "va \"dale\"");
        log.Undelivered("Daniel", "brb");
        path = log.Path();
        // (written as it goes: readable before the log is closed)
        std::ifstream early(PathOf(path));
        std::string first;
        std::getline(early, first);
        CHECK(first.find("\"game\":\"kof98\"") != std::string::npos);
    }
    std::ifstream file(PathOf(path));
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    CHECK(lines.size() == 4);
    if (lines.size() == 4) {
        CHECK(lines[0].find("\"host\":true") != std::string::npos && lines[0].find("\"them\":\"Rodrigo\"") != std::string::npos);
        CHECK(lines[1].find("\"dir\":\"sent\"") != std::string::npos && lines[1].find("\"text\":\"¿otra?\"") != std::string::npos);
        CHECK(lines[2].find("\"text\":\"va \\\"dale\\\"\"") != std::string::npos);
        CHECK(lines[3].find("\"dir\":\"undelivered\"") != std::string::npos);
        // (the time with its zone: 2026-11-03T21:41:05-06:00)
        const size_t t = lines[1].find("\"t\":\"") + 5;
        CHECK(lines[1].size() > t + 25 && lines[1][t + 4] == '-' && lines[1][t + 10] == 'T' &&
              (lines[1][t + 19] == '-' || lines[1][t + 19] == '+') && lines[1][t + 22] == ':');
    }
    CHECK(PathOf(path).filename().u8string().find(u8"_kof98.jsonl") != std::u8string::npos);
    {
        Log off; // (logging off: an empty folder)
        off.Start("", "kof98", "0.2.0", false);
        off.Sent("Daniel", "hola");
        CHECK(off.Path().empty());
    }
    fs::remove_all(folder.parent_path(), error);
}

// The Spanish letters as drawn, against the golden picture (one text row per dot row, # for a dot)
static std::string Picture() {
    const char *text = "¿otra? ¡vámonos, niño! ÁÉÍÓÚÜÑ áéíóúü 😀";
    const int width = static_cast<int>(recomp::text::TextWidth(text, 1)) + 2, height = 12;
    std::vector<uint32_t> pixels(static_cast<size_t>(width) * height, 0xFF000000u);
    const recomp::text::Canvas canvas{pixels.data(), width, height};
    canvas.Text(1, 2, 1, 0, {255, 255, 255, 255}, text);
    std::string out;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            out += (pixels[static_cast<size_t>(y) * width + x] & 0xFF) ? '#' : '.';
        }
        out += '\n';
    }
    CHECK(recomp::text::CharCount(text, std::strlen(text)) == 39);
    CHECK(recomp::text::TextLines(text, 1, 6 * 20) == 3);
    return out;
}

int main(int argc, char **argv) {
    if (argc == 3 && !std::strcmp(argv[1], "--write-golden")) {
        std::ofstream(argv[2], std::ios::binary) << Picture();
        return 0;
    }
    const char *vectors = argc > 1 ? argv[1] : "tests/chat-vectors.txt";
    const char *golden = argc > 2 ? argv[2] : "tests/chat-text.golden";
    const int cases = Vectors(vectors);
    for (unsigned seed = 1; seed <= 20; seed++) {
        Network(seed);
    }
    Sending();
    Logging();
    std::ifstream file(golden, std::ios::binary);
    std::stringstream expected;
    expected << file.rdbuf();
    CHECK(!expected.str().empty() && Picture() == expected.str());
    if (failures == 0) {
        std::printf("chat_test: all as it should be (%d vectors, 20 lossy networks)\n", cases);
    }
    return failures ? 1 : 0;
}
