// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of recomp-common: https://github.com/tmcleroy/recomp-common
//
// A host with no game: sets the menu up as a port would, presses the buttons named on the command line and
// writes the picture as a PPM file. Shows everything a real host does except showing the picture in a window.
//
//   shot OUT.ppm [KEYS] [WIDTHxHEIGHT]     KEYS: u d l r (the directions), a (confirm), b (back)
//   shot menu.ppm dddda 896x672
#include "i18n.h"
#include "menu.h"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>

using namespace recomp;

namespace {
    // What a port's own rows are called here
    enum { kInsertCoin = 1, kReset };

    std::map<size_t, int> g_values = {{0, 1}}; // the features' values: a port keeps these in its settings

    MenuConfig Config() {
        MenuConfig config;
        config.groups = {
            {{MenuRow::Save}, {MenuRow::Load}, {MenuRow::Slot}},
            {{MenuRow::Pages}, {MenuRow::Speed}, {MenuRow::Action, kInsertCoin, "Insert coin", "Puts a coin in the first slot."}},
            {{MenuRow::Action, kReset, "Reset", "Starts the machine again, as if switched off and on."}, {MenuRow::Quit}, {MenuRow::Resume}},
        };
        config.features = {
            {"Cheats", "Endless time", "The round's clock stands still."},
            {"Cheats", "Health", "Whose health is kept full.", {"Off", "Player 1", "Player 2", "Both"}},
            {"Picture", "Widescreen", "Shows more of the stage at each side (16:9)."},
        };
        config.commands = {
            {"", "Start over", "Back to where the round began."},
            {"Training", "Training mode", "Straight to a practice round."},
        };
        config.value = [](size_t feature) { return g_values[feature]; };
        config.setValue = [](size_t feature, int value) { g_values[feature] = value; };
        config.speeds = {"Full", "1/2", "1/4"};
        config.speedDescription = "Left/Right: slow motion.";
        config.confirmName = "a button";
        config.closeName = "Esc";
        return config;
    }
} // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: shot OUT.ppm [KEYS] [WIDTHxHEIGHT]\n");
        return 2;
    }
    const char *keys = argc > 2 ? argv[2] : "";
    int width = 896, height = 672;
    if (argc > 3 && sscanf(argv[3], "%dx%d", &width, &height) != 2) {
        fprintf(stderr, "shot: the size is WIDTHxHEIGHT\n");
        return 2;
    }

    // (in the language RECOMP_LANGUAGE names, as a game the launcher starts: i18n.h)
    recomp::i18n::SetLanguage(recomp::i18n::Resolve("", recomp::i18n::FromEnvironment()));
    Menu menu;
    menu.Configure(Config());
    menu.Open();
    for (const char *key = keys; *key; key++) {
        const unsigned button = *key == 'u' ? kMenuUp : *key == 'd' ? kMenuDown : *key == 'l' ? kMenuLeft : *key == 'r' ? kMenuRight
                                : *key == 'a' ? kMenuConfirm : *key == 'b' ? kMenuBack : 0;
        const Menu::Chosen chosen = menu.Buttons(button);
        // A port carries out what was chosen and closes the menu if that went well
        switch (chosen.kind) {
        case Menu::Chosen::Save: printf("save a state in slot %d\n", menu.Slot()); break;
        case Menu::Chosen::Load: menu.Note("Nothing is kept in slot " + std::to_string(menu.Slot()) + "."); break;
        case Menu::Chosen::Action: printf("the port's own row %d\n", chosen.id); break;
        case Menu::Chosen::Command: printf("the game's command %d\n", chosen.id); break;
        case Menu::Chosen::Quit: printf("quit\n"); return 0;
        case Menu::Chosen::None: break;
        }
    }

    // The game's picture as it stood (here: a plain gradient), red in the low byte, with the menu over it
    std::vector<uint32_t> pixels(static_cast<size_t>(width) * height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const uint32_t r = 40 + 100 * x / width, g = 60 + 80 * y / height, b = 120;
            pixels[static_cast<size_t>(y) * width + x] = 0xFF000000u | (b << 16) | (g << 8) | r;
        }
    }
    if (menu.IsOpen()) {
        menu.Draw(pixels.data(), width, height);
    }
    FILE *out = fopen(argv[1], "wb");
    if (!out) {
        perror(argv[1]);
        return 1;
    }
    fprintf(out, "P6\n%d %d\n255\n", width, height);
    for (uint32_t pixel : pixels) {
        const unsigned char rgb[3] = {static_cast<unsigned char>(pixel), static_cast<unsigned char>(pixel >> 8), static_cast<unsigned char>(pixel >> 16)};
        fwrite(rgb, 1, 3, out);
    }
    fclose(out);
    return 0;
}
