// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of recomp-common: https://github.com/tmcleroy/recomp-common
// The menu's rules, without a picture to compare: what rows there are, what the buttons do, and that
// drawing stays inside the picture at any size.
#include "menu.h"

#include <cstdio>
#include <cstdlib>
#include <map>

using namespace recomp;

#define CHECK(what)                                                    \
    do {                                                               \
        if (!(what)) {                                                 \
            fprintf(stderr, "%s:%d: not so: %s\n", __FILE__, __LINE__, #what); \
            exit(1);                                                   \
        }                                                              \
    } while (0)

namespace {
    std::map<size_t, int> g_values;

    MenuConfig Config() {
        MenuConfig c;
        c.groups = {
            {{MenuRow::Save}, {MenuRow::Load}, {MenuRow::Slot}},
            {{MenuRow::Pages}, {MenuRow::Speed}, {MenuRow::Action, 7, "Insert coin", "Puts a coin in."}},
            {{MenuRow::Action, 9, "Reset", "Starts again."}, {MenuRow::Quit}, {MenuRow::Resume}},
        };
        c.features = {
            {"Cheats", "Endless time", "The clock stands still."},
            {"Cheats", "Health", "Whose health is kept full.", {"Off", "Player 1", "Both"}},
            {"Picture", "Wide", "A wider picture."},
        };
        c.commands = {
            {"", "Start over", "Back to where the round began."},
            {"Training", "Begin", "Starts a session."},
        };
        c.value = [](size_t feature) { return g_values[feature]; };
        c.setValue = [](size_t feature, int value) { g_values[feature] = value; };
        c.speeds = {"Full", "1/2", "1/4"};
        c.speedDescription = "Left/Right: slow motion.";
        return c;
    }

    Menu::Chosen Press(Menu &menu, std::initializer_list<unsigned> presses) {
        Menu::Chosen chosen;
        for (unsigned press : presses) {
            chosen = menu.Buttons(press);
        }
        return chosen;
    }
} // namespace

int main() {
    // First page: Save, Load, Slot | Start over, Cheats, Picture, Training, Speed, Insert coin | Reset, Quit, Resume
    Menu menu;
    menu.Configure(Config());
    CHECK(Press(menu, {kMenuConfirm}).kind == Menu::Chosen::None); // (closed: nothing)
    menu.Open();
    CHECK(menu.IsOpen());
    CHECK(Press(menu, {kMenuConfirm}).kind == Menu::Chosen::Save);
    CHECK(Press(menu, {kMenuDown, kMenuConfirm}).kind == Menu::Chosen::Load);

    // The slot: 1-9, around, by Left/Right and Confirm
    Press(menu, {kMenuDown, kMenuLeft});
    CHECK(menu.Slot() == 9);
    Press(menu, {kMenuRight, kMenuConfirm});
    CHECK(menu.Slot() == 2);

    // A command of no category is on the first page
    Menu::Chosen chosen = Press(menu, {kMenuDown, kMenuConfirm});
    CHECK(chosen.kind == Menu::Chosen::Command && chosen.id == 0);

    // The Cheats page: an on/off feature changes by Confirm only, a choice by Left/Right (to its ends) and
    // Confirm (around)
    Press(menu, {kMenuDown, kMenuConfirm});
    Press(menu, {kMenuRight});
    CHECK(g_values[0] == 0);
    Press(menu, {kMenuConfirm});
    CHECK(g_values[0] == 1);
    Press(menu, {kMenuDown, kMenuRight, kMenuRight, kMenuRight});
    CHECK(g_values[1] == 2);
    Press(menu, {kMenuConfirm});
    CHECK(g_values[1] == 0);
    Press(menu, {kMenuLeft});
    CHECK(g_values[1] == 0);
    {
        // Choices the menu does not offer are passed over (MenuConfig::offered): Health's "Player 1"
        MenuConfig only = Config();
        only.offered = [](size_t feature, int choice) { return !(feature == 1 && choice == 1); };
        Menu narrowed;
        narrowed.Configure(std::move(only));
        narrowed.Open();
        g_values[1] = 0;
        Press(narrowed, {kMenuDown, kMenuDown, kMenuDown, kMenuDown, kMenuConfirm, kMenuDown, kMenuRight});
        CHECK(g_values[1] == 2);
        Press(narrowed, {kMenuLeft});
        CHECK(g_values[1] == 0);
        Press(narrowed, {kMenuConfirm});
        CHECK(g_values[1] == 2);
        Press(narrowed, {kMenuConfirm});
        CHECK(g_values[1] == 0);
        // (one chosen elsewhere stays reachable from where it is)
        g_values[1] = 1;
        Press(narrowed, {kMenuRight});
        CHECK(g_values[1] == 2);
        g_values[1] = 0;
    }
    // Back (the row after the two features), and the first page is where it was left
    Press(menu, {kMenuDown, kMenuConfirm});
    // ... on the Cheats row: two down is Training, whose page has the command
    Press(menu, {kMenuDown, kMenuDown, kMenuConfirm});
    chosen = Press(menu, {kMenuConfirm});
    CHECK(chosen.kind == Menu::Chosen::Command && chosen.id == 1);
    Press(menu, {kMenuDown, kMenuLeft}); // (Left on a row with no values: the first page)

    // Speed: Left/Right to the ends, Confirm around
    Press(menu, {kMenuDown, kMenuRight, kMenuRight, kMenuRight});
    CHECK(menu.Speed() == 2);
    Press(menu, {kMenuConfirm});
    CHECK(menu.Speed() == 0);
    Press(menu, {kMenuLeft});
    CHECK(menu.Speed() == 0);
    {
        // Full speed in the middle (slower ones before it, faster after): where the menu starts
        MenuConfig faster = Config();
        faster.speeds = {"1/2", "Full", "2x"};
        faster.normalSpeed = 1;
        Menu both;
        both.Configure(faster);
        CHECK(both.Speed() == 1);
        both.SetSpeed(2);
        both.Configure(faster);
        CHECK(both.Speed() == 2); // (chosen, kept)
    }

    // The host's rows hand back their ids
    chosen = Press(menu, {kMenuDown, kMenuConfirm});
    CHECK(chosen.kind == Menu::Chosen::Action && chosen.id == 7);
    chosen = Press(menu, {kMenuDown, kMenuConfirm});
    CHECK(chosen.kind == Menu::Chosen::Action && chosen.id == 9);
    CHECK(Press(menu, {kMenuDown, kMenuConfirm}).kind == Menu::Chosen::Quit);
    CHECK(menu.IsOpen());
    Press(menu, {kMenuDown, kMenuConfirm}); // Resume
    CHECK(!menu.IsOpen());

    // Opened again on the row it was closed on; Down from the last row is the first; Back closes
    menu.Open();
    CHECK(Press(menu, {kMenuDown, kMenuConfirm}).kind == Menu::Chosen::Save);
    Press(menu, {kMenuBack});
    CHECK(!menu.IsOpen());

    // Back in a page leaves the page (the menu stays open); a Mute row switches the sound, and is not
    // there without the two functions
    {
        MenuConfig c = Config();
        bool muted = false;
        c.groups.insert(c.groups.begin(), {{MenuRow::Mute}});
        Menu plain;
        plain.Configure(c);
        plain.Open();
        CHECK(Press(plain, {kMenuConfirm}).kind == Menu::Chosen::Save); // (no Mute row: the first is Save)
        c.muted = [&] { return muted; };
        c.setMuted = [&](bool to) { muted = to; };
        Menu sound;
        sound.Configure(c);
        sound.Open();
        CHECK(Press(sound, {kMenuConfirm}).kind == Menu::Chosen::None);
        CHECK(muted);
        Press(sound, {kMenuLeft});
        CHECK(!muted);
        std::vector<uint32_t> picture(640 * 480);
        sound.Draw(picture.data(), 640, 480);
        // (or on a page of its own, with Speed: the page is there for them alone, and they work there)
        MenuConfig paged = c;
        paged.groups = {{{MenuRow::Page, 0, "Settings"}, {MenuRow::Resume}}};
        paged.pages.push_back({"Settings", "", "", false, {MenuRow::Speed, MenuRow::Mute}});
        Menu inPage;
        inPage.Configure(paged);
        inPage.Open();
        const int speed = inPage.Speed();
        Press(inPage, {kMenuConfirm, kMenuRight}); // (into the page; its first row, Speed, a step faster)
        CHECK(inPage.Speed() == speed + 1);
        Press(inPage, {kMenuDown, kMenuConfirm});   // (Mute: on)
        CHECK(muted);
    }

    // With no speeds there is no Speed row; with nothing at all, nothing happens
    MenuConfig bare = Config();
    bare.speeds.clear();
    bare.groups = {{{MenuRow::Speed}, {MenuRow::Resume}}};
    Menu small;
    small.Configure(bare);
    small.Open();
    Press(small, {kMenuConfirm});
    CHECK(!small.IsOpen());
    Menu empty;
    empty.Open();
    CHECK(Press(empty, {kMenuConfirm}).kind == Menu::Chosen::None);

    // Drawing stays in the picture: pictures of many sizes with a guard either side (and the address
    // sanitizer the test is built with)
    for (int height : {1, 100, 224, 240, 448, 672, 720, 1080}) {
        for (int width : {1, 256, 320, 896, 1920}) {
            const size_t count = static_cast<size_t>(width) * height;
            std::vector<uint32_t> pixels(count + 2 * 64, 0x12345678);
            for (int page = 0; page < 2; page++) {
                Menu drawn;
                drawn.Configure(Config());
                drawn.Open();
                if (page) {
                    Press(drawn, {kMenuDown, kMenuDown, kMenuDown, kMenuDown, kMenuConfirm});
                }
                drawn.Note(page ? "" : "Nothing is kept in slot 1.");
                drawn.Draw(pixels.data() + 64, width, height);
            }
            for (size_t i = 0; i < 64; i++) {
                CHECK(pixels[i] == 0x12345678 && pixels[64 + count + i] == 0x12345678);
            }
            CHECK(pixels[64] != 0x12345678);
        }
    }
    // Pages under a page, and a page given a row of its own: Modes at the top, with Begin (a command of
    // its own), then the pages Show and Viewer under it; Left and Back go to where each was reached from
    {
        MenuConfig c = Config();
        c.groups.insert(c.groups.begin(), {{MenuRow::Page, 0, "Modes"}});
        c.commands.push_back({"Modes", "Begin", "Starts."});
        c.commands.push_back({"Show", "Start", "Starts the show."});
        c.features.push_back({"Show", "Parts", "What is shown.", {"All", "Some"}});
        c.features.push_back({"Viewer", "Place", "Where.", {"Here", "There"}});
        c.pages = {{"Modes", "", "Other ways to run the game.", false}, {"Show", "Modes", "", false}, {"Viewer", "Modes", "", false}};
        g_values.clear();
        Menu nested;
        nested.Configure(c);
        nested.Open();
        // (the first row is the Modes page; on it: Begin, Show, Viewer, Back)
        CHECK(Press(nested, {kMenuConfirm}).kind == Menu::Chosen::None);
        Menu::Chosen begin = Press(nested, {kMenuConfirm});
        CHECK(begin.kind == Menu::Chosen::Command && begin.id == 2);
        // Show: Start, Parts, Back
        Press(nested, {kMenuDown, kMenuConfirm});
        Menu::Chosen start = Press(nested, {kMenuConfirm});
        CHECK(start.kind == Menu::Chosen::Command && start.id == 3);
        Press(nested, {kMenuDown, kMenuRight});
        CHECK(g_values[3] == 1);
        // Back from Show is on Modes's Show row; Down and Confirm is then Viewer, whose first row is Place
        Press(nested, {kMenuDown, kMenuConfirm});
        Press(nested, {kMenuDown, kMenuConfirm, kMenuRight});
        CHECK(g_values[4] == 1);
        // Left twice, from rows that Left does not change: Modes, then the first page on its first row
        // again. The pages under Modes are not rows of the first page: Save is its second row
        Press(nested, {kMenuDown, kMenuLeft, kMenuLeft});
        // Right on a page's row goes into it as Confirm does, and Left comes back to that row: Modes, Show
        // (from the Begin row), back on Modes's Show row (Up from which is Begin), and the first page again
        Press(nested, {kMenuRight});
        Press(nested, {kMenuDown, kMenuRight});
        CHECK(Press(nested, {kMenuConfirm}).id == 3);
        Press(nested, {kMenuLeft});
        Menu::Chosen again = Press(nested, {kMenuUp, kMenuConfirm});
        CHECK(again.kind == Menu::Chosen::Command && again.id == 2);
        Press(nested, {kMenuLeft});
        CHECK(Press(nested, {kMenuDown, kMenuConfirm}).kind == Menu::Chosen::Save);
        // ... and Modes is not listed again among the pages: after Slot and Start over comes Cheats
        Press(nested, {kMenuDown, kMenuDown, kMenuDown, kMenuDown, kMenuConfirm});
        Press(nested, {kMenuConfirm});
        CHECK(g_values[0] == 1);
        g_values.clear();
    }

    puts("menu_test: all as it should be");
    return 0;
}
