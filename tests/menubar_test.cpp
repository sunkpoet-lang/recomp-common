// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of recomp-common: https://github.com/tmcleroy/recomp-common
// The menu bar's rules (menubar.h): where its names and menus are, what the pointer, clicks and buttons do,
// that menus stay inside the canvas, that drawing stays inside the picture at any size, and that the in-game
// menu's rows become the bar's entries and do the same.
#include "menubar.h"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "menu.h"

using namespace recomp;

#define CHECK(what)                                                    \
    do {                                                               \
        if (!(what)) {                                                 \
            fprintf(stderr, "%s:%d: not so: %s\n", __FILE__, __LINE__, #what); \
            exit(1);                                                   \
        }                                                              \
    } while (0)

namespace {
    enum { kQuickSave = 1, kQuickLoad, kQuit, kSlot1, kSlot2, kSlot3, kFull, kSize1, kSize2, kMute, kAbout };

    BarEntry Entry(const char *label, int id, const char *shortcut = "", bool enabled = true) {
        BarEntry e;
        e.label = label;
        e.id = id;
        e.shortcut = shortcut;
        e.enabled = enabled;
        return e;
    }

    BarEntry Sub(const char *label, std::vector<BarEntry> children) {
        BarEntry e;
        e.label = label;
        e.children = std::move(children);
        return e;
    }

    std::vector<BarMenu> Menus() {
        BarEntry size2 = Entry("2x", kSize2);
        size2.mark = BarEntry::Dot;
        return {
            {"Game",
             {Sub("Save state", {Entry("Slot 1", kSlot1), Entry("Slot 2", kSlot2), Entry("Slot 3", kSlot3)}),
              Entry("Quick load", kQuickLoad, "F9", false), Entry("Quick save", kQuickSave, "F5"), BarEntry::Line(),
              Entry("Quit", kQuit)}},
            {"Video", {Sub("Window size", {Entry("1x", kSize1), size2}), Entry("Fullscreen", kFull, "F11")}},
            {"Audio", {Entry("Mute", kMute)}},
            {"Help", {Entry("About", kAbout)}},
        };
    }

    // The middle of an entry of a list open
    void EntryPoint(const MenuBar &bar, size_t list, int entry, float &x, float &y, int dot) {
        const MenuBar::Box b = bar.Boxes()[list];
        x = b.x + b.w / 2;
        y = b.y + 2 * dot + entry * 11 * dot + 5 * dot;
    }
} // namespace

int main() {
    const int dot = 2, width = 640, height = 470;
    MenuBar bar;
    bar.Configure(Menus());
    bar.SetArea(width, height, dot);
    CHECK(!bar.IsOpen());
    CHECK(bar.Height() == 24);

    // The bar's names, side by side from the left, all on the bar
    for (size_t t = 0; t < 4; t++) {
        const MenuBar::Box b = bar.TitleBox(t);
        CHECK(b.y == 0 && b.h == bar.Height() && b.w > 0);
        if (t > 0) {
            CHECK(b.x >= bar.TitleBox(t - 1).x + bar.TitleBox(t - 1).w);
        }
    }
    CHECK(bar.Hit(5, 5));
    CHECK(!bar.Hit(300, 300)); // (the picture, with nothing open)

    // A click on a name opens its menu below it, with nothing picked; pointing at another name opens that one
    const MenuBar::Box game = bar.TitleBox(0), video = bar.TitleBox(1);
    CHECK(!bar.Click(game.x + 3, 5).chosen);
    CHECK(bar.IsOpen() && bar.Boxes().size() == 1);
    CHECK(bar.Boxes()[0].x == game.x && bar.Boxes()[0].y == bar.Height());
    CHECK(bar.Picked()[0] == 0 && bar.Picked()[1] == -1);
    CHECK(bar.Pointer(video.x + 3, 5));
    CHECK(bar.Picked()[0] == 1 && bar.Boxes()[0].x == video.x);
    CHECK(!bar.Pointer(video.x + 4, 6)); // (the same name: nothing changes)
    // A click on the open menu's name closes it
    bar.Click(video.x + 3, 5);
    CHECK(!bar.IsOpen());

    // Pointing at an entry with a submenu opens it beside; choosing in it hands back its id and closes all
    bar.Click(game.x + 3, 5);
    float x, y;
    EntryPoint(bar, 0, 0, x, y, dot);
    CHECK(bar.Pointer(x, y));
    CHECK(bar.Boxes().size() == 2);
    CHECK(bar.Boxes()[1].x >= bar.Boxes()[0].x + bar.Boxes()[0].w - dot);
    EntryPoint(bar, 1, 2, x, y, dot);
    bar.Pointer(x, y);
    CHECK(bar.Picked().back() == 2);
    MenuBar::Chosen chosen = bar.Click(x, y);
    CHECK(chosen.chosen && chosen.id == kSlot3);
    CHECK(!bar.IsOpen());

    // A dim entry, or a line, is not picked and does nothing: the menu stays open
    bar.Click(game.x + 3, 5);
    EntryPoint(bar, 0, 1, x, y, dot);
    bar.Pointer(x, y);
    CHECK(bar.Picked()[1] == -1);
    CHECK(!bar.Click(x, y).chosen && bar.IsOpen());
    {
        const MenuBar::Box b = bar.Boxes()[0];
        const float lineY = b.y + 2 * dot + 3 * 11 * dot + 2 * dot;
        CHECK(!bar.Click(b.x + 20, lineY).chosen && bar.IsOpen());
    }
    // A click on the picture closes what is open, and chooses nothing
    CHECK(!bar.Click(300, 400).chosen && !bar.IsOpen());

    // The buttons: Down past the dim entry and the line, around; Right into a submenu and Left out of it;
    // Right on an entry with none, the next menu; Left on the first level, the one before (around)
    bar.OpenMenu(0);
    CHECK(bar.Picked()[1] == 0); // (opened from the keyboard: its first usable entry)
    bar.Buttons(kMenuDown);
    CHECK(bar.Picked()[1] == 2); // (not 1, dim)
    bar.Buttons(kMenuDown);
    CHECK(bar.Picked()[1] == 4); // (not 3, a line)
    bar.Buttons(kMenuDown);
    CHECK(bar.Picked()[1] == 0); // (around)
    bar.Buttons(kMenuUp);
    CHECK(bar.Picked()[1] == 4);
    bar.Buttons(kMenuDown);
    bar.Buttons(kMenuRight);
    CHECK(bar.Picked().size() == 3 && bar.Picked()[2] == 0);
    bar.Buttons(kMenuLeft);
    CHECK(bar.Picked().size() == 2 && bar.Picked()[0] == 0);
    bar.Buttons(kMenuDown);
    bar.Buttons(kMenuRight); // (Quick save has no submenu: the next menu, Video)
    CHECK(bar.Picked()[0] == 1);
    bar.Buttons(kMenuLeft);
    bar.Buttons(kMenuLeft); // (Game, then around to Help)
    CHECK(bar.Picked()[0] == 3);
    chosen = bar.Buttons(kMenuConfirm);
    CHECK(chosen.chosen && chosen.id == kAbout && !bar.IsOpen());
    // Back closes a level, then the menu
    bar.OpenMenu(1);
    bar.Buttons(kMenuConfirm); // (into Window size)
    CHECK(bar.Picked().size() == 3);
    bar.Buttons(kMenuBack);
    CHECK(bar.Picked().size() == 2);
    bar.Buttons(kMenuBack);
    CHECK(!bar.IsOpen());
    // A menu of only dim entries: nothing to pick, and nothing chosen
    {
        MenuBar dim;
        dim.Configure({{"Off", {Entry("A", 1, "", false), Entry("B", 2, "", false)}}});
        dim.SetArea(width, height, dot);
        dim.OpenMenu(0);
        CHECK(dim.Picked()[1] == -1);
        dim.Buttons(kMenuDown);
        CHECK(dim.Picked()[1] == -1);
        CHECK(!dim.Buttons(kMenuConfirm).chosen && dim.IsOpen());
    }

    // The popup where the player right-clicks: its list of the bar's menus at the point...
    bar.OpenPopup(100, 100);
    CHECK(bar.IsOpen() && bar.Boxes().size() == 1);
    CHECK(bar.Boxes()[0].x == 100 && bar.Boxes()[0].y == 100);
    EntryPoint(bar, 0, 1, x, y, dot); // (Video)
    bar.Pointer(x, y);
    CHECK(bar.Boxes().size() == 2);
    EntryPoint(bar, 1, 1, x, y, dot); // (Fullscreen)
    chosen = bar.Click(x, y);
    CHECK(chosen.chosen && chosen.id == kFull);
    // ...and near the bottom right corner, up and to the left of it, all inside the canvas
    bar.OpenPopup(width - 3, height - 3);
    {
        const MenuBar::Box b = bar.Boxes()[0];
        CHECK(b.x + b.w <= width && b.y + b.h <= height);
        CHECK(b.x < width - 3 && b.y < height - 3);
    }
    bar.Buttons(kMenuDown); // (Game)
    bar.Buttons(kMenuRight);
    bar.Buttons(kMenuRight); // (into Save state)
    CHECK(bar.Boxes().size() == 3);
    for (const MenuBar::Box &b : bar.Boxes()) {
        CHECK(b.x >= 0 && b.y >= 0 && b.x + b.w <= width && b.y + b.h <= height);
    }
    // (a submenu with no room at the right opens at the left of its menu)
    CHECK(bar.Boxes()[1].x < bar.Boxes()[0].x);
    // Left closes the popup's levels, but on its first goes nowhere; Back there closes it
    bar.Buttons(kMenuLeft);
    bar.Buttons(kMenuLeft);
    CHECK(bar.Picked().size() == 1);
    bar.Buttons(kMenuLeft);
    CHECK(bar.IsOpen());
    bar.Buttons(kMenuBack);
    CHECK(!bar.IsOpen());
    // A menu by the right edge of the bar keeps inside the canvas too
    bar.SetArea(160, height, dot);
    bar.OpenMenu(3);
    CHECK(bar.Boxes()[0].x + bar.Boxes()[0].w <= 160);
    bar.SetArea(width, height, dot);
    bar.Close();

    // Configured again with what is open gone: what is past it closes
    bar.OpenMenu(1);
    bar.Buttons(kMenuConfirm);
    CHECK(bar.Picked().size() == 3);
    std::vector<BarMenu> fewer = Menus();
    fewer[1].entries[0].children.clear(); // (Window size has no submenu now)
    fewer[1].entries[0].id = kSize1;
    bar.Configure(fewer);
    CHECK(bar.IsOpen() && bar.Picked().size() == 2);
    bar.Configure({Menus()[0]});
    CHECK(!bar.IsOpen()); // (Video is gone)

    // With the bar hidden (fullscreen) it is not hit nor drawn, and the popup still works
    bar.Configure(Menus());
    bar.ShowBar(false);
    CHECK(!bar.Hit(5, 5));
    CHECK(!bar.Click(game.x + 3, 5).chosen && !bar.IsOpen());
    bar.OpenPopup(50, 50);
    CHECK(bar.IsOpen());
    bar.Close();
    bar.ShowBar(true);

    // Drawing: inside the picture at any size (the sanitizers catch a pixel outside), the bar drawn over its
    // strip, the picture below untouched with nothing open
    const int sizes[][3] = {{640, 470, 2}, {384, 235, 1}, {1152, 705, 3}, {60, 40, 1}, {1, 1, 1}};
    for (const auto &s : sizes) {
        std::vector<uint32_t> pixels(static_cast<size_t>(s[0]) * s[1], 0xFF102030u);
        bar.SetArea(s[0], s[1], s[2]);
        bar.Close();
        bar.Draw(pixels.data(), s[0], s[1]);
        if (s[1] > bar.Height()) {
            CHECK(pixels[0] != 0xFF102030u);
            CHECK(pixels[static_cast<size_t>(bar.Height()) * s[0]] == 0xFF102030u);
        }
        bar.OpenMenu(1);
        bar.Buttons(kMenuConfirm);
        bar.Draw(pixels.data(), s[0], s[1]);
        bar.OpenPopup(static_cast<float>(s[0]) / 2, static_cast<float>(s[1]) / 2);
        bar.Buttons(kMenuDown);
        bar.Buttons(kMenuRight);
        bar.Draw(pixels.data(), s[0], s[1]);
    }

    // The in-game menu's rows as the bar's entries (AddMenuEntries, ChooseMenuEntry)
    {
        static std::map<size_t, int> values;
        static bool muted = false;
        MenuConfig config;
        MenuRow coin{MenuRow::Action, 7, "Insert coin", "Puts a coin in."};
        coin.bar = "Input";
        config.groups = {
            {{MenuRow::Save}, {MenuRow::Load}, {MenuRow::Slot}},
            {{MenuRow::Pages}, {MenuRow::Speed}, coin, {MenuRow::Mute}},
            {{MenuRow::Action, 9, "Reset", "Starts again."}, {MenuRow::Quit}, {MenuRow::Resume}},
        };
        config.features = {
            {"Cheats", "Endless time", "The clock stands still."},
            {"Cheats", "Health", "Whose health is kept full.", {"Off", "Player 1", "Both"}},
            {"Picture", "Wide", "A wider picture."},
            {"Hidden", "Secret", "Under Cheats."},
        };
        config.commands = {
            {"", "Start over", "Back to where the round began."},
            {"Training", "Begin", "Starts a session."},
        };
        config.pages = {{"Hidden", "Cheats", ""}};
        config.value = [](size_t feature) { return values[feature]; };
        config.setValue = [](size_t feature, int value) { values[feature] = value; };
        config.speeds = {"Full", "Half"};
        config.muted = [] { return muted; };
        config.setMuted = [](bool m) { muted = m; };
        Menu menu;
        menu.Configure(config);

        const auto build = [&](const BarOptions &options = {}) {
            std::vector<BarMenu> menus = {{"Game", {}}, {"Video", {Entry("Fullscreen", kFull)}}, {"Audio", {}}, {"Input", {}}, {"Features", {}}, {"Help", {}}};
            AddMenuEntries(menus, config, menu, options);
            return menus;
        };
        const auto labels = [](const std::vector<BarEntry> &entries) {
            std::string s;
            for (const BarEntry &e : entries) {
                s += (e.IsLine() ? std::string("-") : e.label) + "|";
            }
            return s;
        };
        const auto find = [](const std::vector<BarEntry> &entries, const std::string &label) -> const BarEntry * {
            for (const BarEntry &e : entries) {
                if (e.label == label) {
                    return &e;
                }
            }
            return nullptr;
        };

        std::vector<BarMenu> menus = build();
        CHECK(menus.size() == 6); // (no menu added: every row's is there)
        // Each row in its menu, by its kind or its `bar`, with a line between groups; Resume left out
        CHECK(labels(menus[0].entries) == "Save state|Load state|Slot|-|Reset|Quit game|");
        CHECK(labels(menus[1].entries) == "Fullscreen|"); // (the host's own, untouched)
        CHECK(labels(menus[2].entries) == "Mute|");
        CHECK(labels(menus[3].entries) == "Insert coin|");
        CHECK(labels(menus[4].entries) == "Start over|Cheats|Picture|Training|Speed|");
        // Save state's slots, the menu's slot marked; a page holds its commands, the pages under it, its features
        CHECK(menus[0].entries[0].children.size() == 9);
        CHECK(menus[0].entries[0].children[0].mark == BarEntry::Dot && menus[0].entries[0].children[1].mark == BarEntry::NoMark);
        const BarEntry *cheats = find(menus[4].entries, "Cheats");
        CHECK(labels(cheats->children) == "Hidden|Endless time|Health|");
        CHECK(find(cheats->children, "Health")->children.size() == 3);
        CHECK(labels(find(menus[4].entries, "Training")->children) == "Begin|");
        CHECK(find(menus[4].entries, "Speed")->children[0].mark == BarEntry::Dot);

        // A feature's mark follows its value, and choosing changes it
        CHECK(find(cheats->children, "Endless time")->mark == BarEntry::NoMark);
        Menu::Chosen did = ChooseMenuEntry(find(cheats->children, "Endless time")->id, config, menu);
        CHECK(did.kind == Menu::Chosen::None && values[0] == 1);
        menus = build();
        cheats = find(menus[4].entries, "Cheats");
        CHECK(find(cheats->children, "Endless time")->mark == BarEntry::Check);
        ChooseMenuEntry(find(cheats->children, "Health")->children[2].id, config, menu);
        CHECK(values[1] == 2);
        menus = build();
        cheats = find(menus[4].entries, "Cheats");
        CHECK(find(cheats->children, "Health")->children[2].mark == BarEntry::Dot);

        // Save and Load hand back what Menu would, with the slot chosen; Slot and Speed change the menu's
        did = ChooseMenuEntry(menus[0].entries[0].children[2].id, config, menu);
        CHECK(did.kind == Menu::Chosen::Save && menu.Slot() == 3);
        did = ChooseMenuEntry(menus[0].entries[1].children[4].id, config, menu);
        CHECK(did.kind == Menu::Chosen::Load && menu.Slot() == 5);
        ChooseMenuEntry(menus[0].entries[2].children[8].id, config, menu);
        CHECK(menu.Slot() == 9);
        ChooseMenuEntry(find(menus[4].entries, "Speed")->children[1].id, config, menu);
        CHECK(menu.Speed() == 1);
        // Actions with their ids, Quit, a command by its place, and the sound
        CHECK(ChooseMenuEntry(menus[3].entries[0].id, config, menu).id == 7);
        did = ChooseMenuEntry(find(menus[0].entries, "Reset")->id, config, menu);
        CHECK(did.kind == Menu::Chosen::Action && did.id == 9);
        CHECK(ChooseMenuEntry(find(menus[0].entries, "Quit game")->id, config, menu).kind == Menu::Chosen::Quit);
        did = ChooseMenuEntry(find(find(menus[4].entries, "Training")->children, "Begin")->id, config, menu);
        CHECK(did.kind == Menu::Chosen::Command && did.id == 1);
        ChooseMenuEntry(menus[2].entries[0].id, config, menu);
        CHECK(muted);
        menus = build();
        CHECK(menus[2].entries[0].mark == BarEntry::Check);
        // The host's own ids are not the menu's
        CHECK(!IsMenuEntry(kFull) && IsMenuEntry(menus[2].entries[0].id));
        CHECK(ChooseMenuEntry(kFull, config, menu).kind == Menu::Chosen::None);

        // Load state's empty slots are dim, and a slot's detail follows its number
        BarOptions options;
        options.slotUsed = [](int slot) { return slot == 2; };
        options.slotDetail = [](int slot) { return slot == 2 ? std::string("21:40") : std::string(); };
        menus = build(options);
        CHECK(!menus[0].entries[1].children[0].enabled && menus[0].entries[1].children[1].enabled);
        CHECK(menus[0].entries[1].children[1].label == "Slot 2  21:40");
        CHECK(menus[0].entries[0].children[0].enabled); // (saving is to any slot)
        // A row whose menu is not there adds it at the end; without muted and setMuted there is no Mute
        config.groups[2][0].bar = "Debug";
        config.muted = nullptr;
        menus = build();
        CHECK(menus.size() == 7 && menus[6].title == "Debug" && labels(menus[6].entries) == "Reset|");
        CHECK(menus[2].entries.empty());
        // and what it makes works in the bar
        MenuBar fromMenu;
        fromMenu.Configure(menus);
        fromMenu.SetArea(640, 470, 2);
        fromMenu.OpenMenu(4);
        CHECK(fromMenu.IsOpen());
    }

    printf("menubar_test: all as it should be\n");
    return 0;
}
