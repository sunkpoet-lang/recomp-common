// SPDX-License-Identifier: GPL-3.0-only
// Part of recomp-common (ForkAde's fork): the small font and the palette for an online match's readout (text.h).
#include "text.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static int failures = 0;
#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        if (!(x)) {                                                                                                    \
            std::fprintf(stderr, "text_test: %s:%d: %s\n", __FILE__, __LINE__, #x);                                  \
            failures++;                                                                                                \
        }                                                                                                              \
    } while (0)

// The dots a text draws in the small font on a black canvas, as rows of '#' and '.'
static std::vector<std::string> Drawn(const char *s) {
    using namespace recomp::text;
    const int width = static_cast<int>(SmallTextWidth(s)), height = kSmallLineHeight;
    std::vector<uint32_t> pixels(static_cast<size_t>(width * height), 0xFF000000u);
    const Canvas canvas{pixels.data(), width, height};
    canvas.SmallText(0, 0, {255, 255, 255, 255}, s);
    std::vector<std::string> rows(static_cast<size_t>(height), std::string(static_cast<size_t>(width), '.'));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (pixels[static_cast<size_t>(y * width + x)] & 0xFF) {
                rows[static_cast<size_t>(y)][static_cast<size_t>(x)] = '#';
            }
        }
    }
    return rows;
}

int main() {
    using namespace recomp::text;
    CHECK(SmallTextWidth("") == 0);
    CHECK(SmallTextWidth("ROLLBACK 10") == 44);
    CHECK(SmallGlyph('a') == SmallGlyph('A'));
    CHECK(SmallGlyph('?') == -1);
    const std::vector<std::string> ping = Drawn("PING 17");
    const std::vector<std::string> want = {
        "##..###.##...##......#..###.",
        "#.#..#..#.#.#.......##....#.",
        "##...#..#.#.#.#......#...#..",
        "#....#..#.#.#.#......#...#..",
        "#...###.#.#..##.....###..#..",
        "............................",
    };
    CHECK(ping.size() == want.size());
    for (size_t row = 0; row < ping.size() && row < want.size(); row++) {
        CHECK(ping[row] == want[row]);
        if (ping[row] != want[row]) {
            std::fprintf(stderr, "  row %zu: %s (want %s)\n", row, ping[row].c_str(), want[row].c_str());
        }
    }
    // (a character outside the table: a blank cell, the next one in its place)
    const std::vector<std::string> blank = Drawn("?1");
    CHECK(blank[0].substr(0, 4) == "....");
    CHECK(blank[0].substr(4, 3) == ".#.");
    // (8 and 0, 5 and S, 0 and O are told apart)
    for (const char *pair : {"80", "5S", "0O", "8B", "0D"}) {
        const std::vector<std::string> two = Drawn(pair);
        bool same = true;
        for (const std::string &row : two) {
            same = same && row.substr(0, 3) == row.substr(4, 3);
        }
        CHECK(!same);
    }
    // (the players' colours told apart, both bright enough over the band)
    CHECK(Palette::kPlayer1.b > Palette::kPlayer1.r && Palette::kPlayer2.r > Palette::kPlayer2.b);
    CHECK(Palette::kBand.a > 0 && Palette::kBand.a < 255);
    if (failures) {
        std::fprintf(stderr, "text_test: %d failure(s)\n", failures);
        return 1;
    }
    std::printf("text_test: ok\n");
    return 0;
}
