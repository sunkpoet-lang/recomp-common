// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of recomp-common: https://github.com/tmcleroy/recomp-common
// The menu bar of a port's window, and the same menus as a popup where the player right-clicks: the view
// for a mouse and a keyboard, beside the in-game menu (menu.h) for a gamepad.
//
// It knows nothing of any game, of the in-game menu or of SDL. The host says what the menus hold (entries
// with a label, the key that does the same, a mark, whether they can be used now, and submenus), tells it
// where the pointer is, what was clicked and which of menu.h's buttons were pressed, and carries out the id
// of what was chosen. It draws itself, the bar along the top of the canvas and the menus that are open
// below it, with text.h's font, at a whole number of pixels to a font dot.
//
// C++17, no dependencies.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace recomp {

// An entry of a menu, or a line between entries
struct BarEntry {
    enum Mark {
        NoMark,
        Check, // on
        Dot,   // the one chosen of several
    };
    std::string label;    // "" with no children: a line between entries
    std::string shortcut; // the key that does the same ("F5"), shown at the right
    Mark mark = NoMark;
    bool enabled = true; // (not: shown dim, and clicking it does nothing)
    int id = 0;          // handed back when chosen (an entry with children is never chosen)
    std::vector<BarEntry> children;

    static BarEntry Line() {
        return {};
    }
    bool IsLine() const {
        return label.empty() && children.empty();
    }
};

// A menu of the bar: its name and its entries
struct BarMenu {
    std::string title;
    std::vector<BarEntry> entries;
};

class MenuBar {
public:
    // The bar's height, in font dots (the text's 7, the row below it for g, j, p, q and y, and room around)
    static constexpr int kHeightDots = 12;

    struct Box {
        float x, y, w, h;
        bool Contains(float px, float py) const {
            return px >= x && py >= y && px < x + w && py < y + h;
        }
    };
    struct Chosen {
        bool chosen = false;
        int id = 0; // the entry's
    };

    // What the menus hold. May be called again at any time (as what is on changes, say): what is open stays
    // open where it still exists.
    void Configure(std::vector<BarMenu> menus);
    // The canvas the bar is drawn on (the bar at its top, the menus over what is below) and the pixels to a
    // font dot (a whole number, 1 or more)
    void SetArea(int width, int height, int dot);
    int Height() const {
        return kHeightDots * m_dot;
    }
    // Whether the bar itself is drawn and can be clicked (not: only the popup can open, as in fullscreen
    // before the pointer reaches the top)
    void ShowBar(bool shown);
    bool BarShown() const {
        return m_barShown;
    }

    // A bar's menu opened, by its place (as F10 opens the first); the right-click menu at a point
    void OpenMenu(size_t menu);
    void OpenPopup(float x, float y);
    void Close();
    bool IsOpen() const {
        return m_levels > 0;
    }

    // The pointer moved (whether what is drawn changed): over an entry it is the one picked, over one with
    // a submenu that opens, over another of the bar's names while one is open that one opens instead
    bool Pointer(float x, float y);
    // A left click: on a name of the bar, its menu opens or closes; on an entry, it is chosen or its submenu
    // opens (a dim entry or a line: nothing, the menu stays open); anywhere else, what is open closes
    Chosen Click(float x, float y);
    // menu.h's buttons newly pressed (kMenu... bits), while a menu is open: Up and Down pick (past dim
    // entries and lines, around), Right opens a submenu or goes to the next menu of the bar, Left closes a
    // submenu or goes to the menu before, Confirm chooses (or opens a submenu), Back closes a level
    Chosen Buttons(unsigned pressed);
    // Whether a point is on the bar (when shown) or on a menu that is open: what a click there is for
    bool Hit(float x, float y) const;

    // Where things are: a name of the bar, and the menus open, the first (the bar's, or the popup's) first
    Box TitleBox(size_t menu) const;
    std::vector<Box> Boxes() const;
    // The entry picked in each menu open (-1: none), the first first; for the popup, the first is the
    // popup's own list of the bar's menus
    std::vector<int> Picked() const;

    // Over a picture of 32-bit pixels (red in the low byte), the size SetArea was given
    void Draw(uint32_t *pixels, int width, int height) const;

private:
    struct List {
        const std::vector<BarEntry> *entries;
        Box box;
        int picked;
    };
    std::vector<List> Lists() const;
    const std::vector<BarEntry> &Tops() const {
        return m_tops;
    }
    Box ListBox(const std::vector<BarEntry> &entries, float x, float y) const;
    Box Place(Box box) const; // moved to be inside the canvas
    float EntryHeight(const BarEntry &entry) const;
    int EntryAt(const List &list, float y) const;
    int FirstUsable(const std::vector<BarEntry> &entries) const;
    int Step(const std::vector<BarEntry> &entries, int from, int by) const;
    bool Opens(const BarEntry &entry) const {
        return entry.enabled && !entry.children.empty();
    }
    void Trim(); // what is open made to fit the menus as they are now

    std::vector<BarMenu> m_menus;
    std::vector<BarEntry> m_tops; // (the menus as entries with children: the popup's list, and the bar's)
    int m_width = 0, m_height = 0, m_dot = 1;
    bool m_barShown = true;
    bool m_popup = false; // what is open is the popup (not a menu of the bar)
    float m_popupX = 0, m_popupY = 0;
    // What is open: m_levels lists, the first the bar's names (or the popup's list), each next one the
    // children of the entry picked in the one before; m_picked has an entry picked for each
    int m_levels = 0;
    std::vector<int> m_picked;
};

} // namespace recomp
