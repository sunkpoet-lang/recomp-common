// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of recomp-common: https://github.com/tmcleroy/recomp-common
// The menu bar and the right-click menu (menubar.h).
#include "menubar.h"

#include <algorithm>

#include "menu.h"
#include "text.h"

namespace recomp {

using text::Canvas;
using text::Color;
using text::kCharWidth;
using text::TextWidth;

namespace {
    // Sizes in font dots
    constexpr float kEntryDots = 11; // an entry's height
    constexpr float kLineDots = 5;   // a line between entries
    constexpr float kEdgeDots = 2;   // above the first entry and below the last
    constexpr float kMarkDots = 10;  // the room for the mark, left of the label
    constexpr float kArrowDots = 9;  // the room for a submenu's arrow, at the right
    constexpr float kTitleDots = 10; // around a name of the bar
    constexpr float kLeastDots = 70; // the narrowest a menu is

    constexpr Color kWhite{255, 255, 255, 255}, kDim{160, 164, 176, 255}, kOff{96, 100, 112, 255};
    constexpr Color kPanel{20, 22, 30, 255}, kBar{32, 34, 44, 255}, kPicked{255, 255, 255, 45};
    constexpr Color kEdge{255, 255, 255, 60}, kShadow{0, 0, 0, 90};
} // namespace

void MenuBar::Configure(std::vector<BarMenu> menus) {
    m_menus = std::move(menus);
    m_tops.clear();
    for (const BarMenu &menu : m_menus) {
        BarEntry top;
        top.label = menu.title;
        top.children = menu.entries;
        m_tops.push_back(std::move(top));
    }
    Trim();
}

void MenuBar::SetArea(int width, int height, int dot) {
    m_width = std::max(width, 0);
    m_height = std::max(height, 0);
    m_dot = std::max(dot, 1);
}

void MenuBar::ShowBar(bool shown) {
    m_barShown = shown;
}

void MenuBar::OpenMenu(size_t menu) {
    if (menu >= m_tops.size()) {
        return;
    }
    m_popup = false;
    m_levels = 2;
    m_picked = {static_cast<int>(menu), FirstUsable(m_tops[menu].children)};
}

void MenuBar::OpenPopup(float x, float y) {
    if (m_tops.empty()) {
        return;
    }
    m_popup = true;
    m_popupX = x;
    m_popupY = y;
    m_levels = 1;
    m_picked = {-1};
}

void MenuBar::Close() {
    m_levels = 0;
    m_picked.clear();
}

void MenuBar::Trim() {
    if (m_levels == 0) {
        return;
    }
    // (walked from the first: where a level's entry is gone, or no longer opens, what is past it closes)
    const std::vector<BarEntry> *entries = &m_tops;
    int level = 0;
    if (!m_popup) {
        if (m_picked[0] < 0 || m_picked[0] >= static_cast<int>(m_tops.size())) {
            Close();
            return;
        }
        entries = &m_tops[static_cast<size_t>(m_picked[0])].children;
        level = 1;
    }
    for (; level < m_levels; level++) {
        int &picked = m_picked[static_cast<size_t>(level)];
        if (picked >= static_cast<int>(entries->size())) {
            picked = -1;
        }
        if (level + 1 < m_levels) {
            if (picked < 0 || !Opens((*entries)[static_cast<size_t>(picked)])) {
                m_levels = level + 1;
                break;
            }
            entries = &(*entries)[static_cast<size_t>(picked)].children;
        }
    }
    m_picked.resize(static_cast<size_t>(m_levels));
}

float MenuBar::EntryHeight(const BarEntry &entry) const {
    return (entry.IsLine() ? kLineDots : kEntryDots) * m_dot;
}

MenuBar::Box MenuBar::ListBox(const std::vector<BarEntry> &entries, float x, float y) const {
    const float d = static_cast<float>(m_dot);
    float label = 0, shortcut = 0, h = 2 * kEdgeDots * d;
    for (const BarEntry &entry : entries) {
        label = std::max(label, TextWidth(entry.label.c_str(), d));
        shortcut = std::max(shortcut, TextWidth(entry.shortcut.c_str(), d));
        h += EntryHeight(entry);
    }
    const float w = kMarkDots * d + label + (shortcut > 0 ? 3 * kCharWidth * d + shortcut : 0) + kArrowDots * d;
    return {x, y, std::max(w, kLeastDots * d), h};
}

MenuBar::Box MenuBar::Place(Box box) const {
    box.x = std::max(0.0f, std::min(box.x, m_width - box.w));
    box.y = std::max(0.0f, std::min(box.y, m_height - box.h));
    return box;
}

MenuBar::Box MenuBar::TitleBox(size_t menu) const {
    const float d = static_cast<float>(m_dot);
    float x = 2 * d;
    for (size_t i = 0; i < m_tops.size(); i++) {
        const float w = TextWidth(m_tops[i].label.c_str(), d) + kTitleDots * d;
        if (i == menu) {
            return {x, 0, w, static_cast<float>(Height())};
        }
        x += w;
    }
    return {0, 0, 0, 0};
}

std::vector<MenuBar::List> MenuBar::Lists() const {
    std::vector<List> lists;
    if (m_levels == 0) {
        return lists;
    }
    const float d = static_cast<float>(m_dot);
    const std::vector<BarEntry> *entries;
    int level;
    if (m_popup) {
        // (the popup's list from the point clicked, or up and to the left of it where there is no room)
        Box box = ListBox(m_tops, m_popupX, m_popupY);
        if (box.x + box.w > m_width) {
            box.x -= box.w;
        }
        if (box.y + box.h > m_height) {
            box.y -= box.h;
        }
        lists.push_back({&m_tops, Place(box), m_picked[0]});
        level = 1;
    } else {
        entries = &m_tops[static_cast<size_t>(m_picked[0])].children;
        const Box title = TitleBox(static_cast<size_t>(m_picked[0]));
        lists.push_back({entries, Place(ListBox(*entries, title.x, title.y + title.h)), m_picked[1]});
        level = 2;
    }
    for (; level < m_levels; level++) {
        // (a submenu beside the entry it is of: at its right, or at its left where there is no room)
        const List &parent = lists.back();
        float y = parent.box.y + kEdgeDots * d;
        for (int i = 0; i < parent.picked; i++) {
            y += EntryHeight((*parent.entries)[static_cast<size_t>(i)]);
        }
        entries = &(*parent.entries)[static_cast<size_t>(parent.picked)].children;
        Box box = ListBox(*entries, parent.box.x + parent.box.w - d, y - kEdgeDots * d);
        if (box.x + box.w > m_width) {
            box.x = parent.box.x - box.w + d;
        }
        lists.push_back({entries, Place(box), m_picked[static_cast<size_t>(level)]});
    }
    return lists;
}

std::vector<MenuBar::Box> MenuBar::Boxes() const {
    std::vector<Box> boxes;
    for (const List &list : Lists()) {
        boxes.push_back(list.box);
    }
    return boxes;
}

std::vector<int> MenuBar::Picked() const {
    return m_picked;
}

int MenuBar::EntryAt(const List &list, float y) const {
    float top = list.box.y + kEdgeDots * m_dot;
    for (size_t i = 0; i < list.entries->size(); i++) {
        const float h = EntryHeight((*list.entries)[i]);
        if (y >= top && y < top + h) {
            return static_cast<int>(i);
        }
        top += h;
    }
    return -1;
}

int MenuBar::FirstUsable(const std::vector<BarEntry> &entries) const {
    return Step(entries, -1, 1);
}

int MenuBar::Step(const std::vector<BarEntry> &entries, int from, int by) const {
    const int n = static_cast<int>(entries.size());
    if (from < 0) {
        from = by > 0 ? -1 : n;
    }
    for (int k = 1; k <= n; k++) {
        const int i = ((from + by * k) % n + n) % n;
        if (!entries[static_cast<size_t>(i)].IsLine() && entries[static_cast<size_t>(i)].enabled) {
            return i;
        }
    }
    return from >= 0 && from < n ? from : -1;
}

bool MenuBar::Hit(float x, float y) const {
    if (m_barShown && x >= 0 && x < m_width && y >= 0 && y < Height()) {
        return true;
    }
    for (const List &list : Lists()) {
        if (list.box.Contains(x, y)) {
            return true;
        }
    }
    return false;
}

bool MenuBar::Pointer(float x, float y) {
    if (m_levels == 0) {
        return false;
    }
    const std::vector<int> before = m_picked;
    const int levelsBefore = m_levels;
    if (!m_popup && m_barShown && y >= 0 && y < Height()) {
        for (size_t t = 0; t < m_tops.size(); t++) {
            if (TitleBox(t).Contains(x, y) && static_cast<int>(t) != m_picked[0]) {
                OpenMenu(t);
                m_picked[1] = -1;
                return true;
            }
        }
        return false;
    }
    const std::vector<List> lists = Lists();
    for (size_t i = lists.size(); i-- > 0;) {
        if (!lists[i].box.Contains(x, y)) {
            continue;
        }
        const size_t level = i + (m_popup ? 0 : 1);
        int e = EntryAt(lists[i], y);
        if (e >= 0) {
            const BarEntry &entry = (*lists[i].entries)[static_cast<size_t>(e)];
            if (entry.IsLine() || !entry.enabled) {
                e = -1;
            }
        }
        if (e >= 0 && e == m_picked[level] && m_levels > static_cast<int>(level) + 1) {
            break; // (still on the entry whose submenu is open: what is open past it stays)
        }
        m_picked[level] = e;
        m_levels = static_cast<int>(level) + 1;
        m_picked.resize(level + 1);
        if (e >= 0 && Opens((*lists[i].entries)[static_cast<size_t>(e)])) {
            m_levels++;
            m_picked.push_back(-1); // (a submenu opened by pointing has nothing picked in it yet)
        }
        break;
    }
    return m_picked != before || m_levels != levelsBefore;
}

MenuBar::Chosen MenuBar::Click(float x, float y) {
    const std::vector<List> lists = Lists();
    for (size_t i = lists.size(); i-- > 0;) {
        if (!lists[i].box.Contains(x, y)) {
            continue;
        }
        const size_t level = i + (m_popup ? 0 : 1);
        const int e = EntryAt(lists[i], y);
        if (e < 0) {
            return {};
        }
        const BarEntry &entry = (*lists[i].entries)[static_cast<size_t>(e)];
        if (entry.IsLine() || !entry.enabled) {
            return {};
        }
        if (Opens(entry)) {
            m_picked[level] = e;
            m_levels = static_cast<int>(level) + 2;
            m_picked.resize(static_cast<size_t>(m_levels), -1);
            m_picked[level + 1] = -1;
            return {};
        }
        const int id = entry.id;
        Close();
        return {true, id};
    }
    if (m_barShown && x >= 0 && x < m_width && y >= 0 && y < Height()) {
        for (size_t t = 0; t < m_tops.size(); t++) {
            if (TitleBox(t).Contains(x, y)) {
                if (m_levels > 0 && !m_popup && m_picked[0] == static_cast<int>(t)) {
                    Close();
                } else {
                    OpenMenu(t);
                    m_picked[1] = -1;
                }
                return {};
            }
        }
    }
    Close();
    return {};
}

MenuBar::Chosen MenuBar::Buttons(unsigned pressed) {
    if (m_levels == 0 || pressed == 0) {
        return {};
    }
    const std::vector<List> lists = Lists();
    const List &deepest = lists.back();
    const size_t level = static_cast<size_t>(m_levels - 1);
    const int first = m_popup ? 0 : 1; // (the level that is the first menu)
    const int count = static_cast<int>(m_tops.size());
    int &picked = m_picked[level];
    const auto openPicked = [&]() {
        m_picked.push_back(FirstUsable((*deepest.entries)[static_cast<size_t>(picked)].children));
        m_levels++;
    };
    if (pressed & (kMenuUp | kMenuDown)) {
        picked = Step(*deepest.entries, picked, (pressed & kMenuUp) ? -1 : 1);
    } else if (pressed & kMenuRight) {
        if (picked >= 0 && Opens((*deepest.entries)[static_cast<size_t>(picked)])) {
            openPicked();
        } else if (!m_popup) {
            OpenMenu(static_cast<size_t>((m_picked[0] + 1) % count));
        }
    } else if (pressed & kMenuLeft) {
        if (static_cast<int>(level) > first) {
            m_levels--;
            m_picked.pop_back();
        } else if (!m_popup) {
            OpenMenu(static_cast<size_t>((m_picked[0] + count - 1) % count));
        }
    } else if (pressed & kMenuConfirm) {
        if (picked >= 0) {
            const BarEntry &entry = (*deepest.entries)[static_cast<size_t>(picked)];
            if (Opens(entry)) {
                openPicked();
            } else if (entry.enabled && !entry.IsLine()) {
                const int id = entry.id;
                Close();
                return {true, id};
            }
        }
    } else if (pressed & kMenuBack) {
        if (static_cast<int>(level) > first) {
            m_levels--;
            m_picked.pop_back();
        } else {
            Close();
        }
    }
    return {};
}

void MenuBar::Draw(uint32_t *pixels, int width, int height) const {
    const Canvas canvas{pixels, width, height};
    const float d = static_cast<float>(m_dot);
    if (m_barShown) {
        canvas.Fill(0, 0, static_cast<float>(width), static_cast<float>(Height()), kBar);
        canvas.Fill(0, Height() - d, static_cast<float>(width), d, kEdge);
        for (size_t t = 0; t < m_tops.size(); t++) {
            const Box box = TitleBox(t);
            if (m_levels > 0 && !m_popup && m_picked[0] == static_cast<int>(t)) {
                canvas.Fill(box.x, box.y, box.w, box.h - d, kPicked);
            }
            canvas.Text(box.x + kTitleDots / 2 * d, 2 * d, d, 0, kWhite, m_tops[t].label.c_str());
        }
    }
    for (const List &list : Lists()) {
        const Box &b = list.box;
        canvas.Fill(b.x + 2 * d, b.y + 2 * d, b.w, b.h, kShadow);
        canvas.Fill(b.x, b.y, b.w, b.h, kPanel);
        canvas.Fill(b.x, b.y, b.w, d, kEdge);
        canvas.Fill(b.x, b.y + b.h - d, b.w, d, kEdge);
        canvas.Fill(b.x, b.y + d, d, b.h - 2 * d, kEdge);
        canvas.Fill(b.x + b.w - d, b.y + d, d, b.h - 2 * d, kEdge);
        float y = b.y + kEdgeDots * d;
        for (size_t i = 0; i < list.entries->size(); i++) {
            const BarEntry &entry = (*list.entries)[i];
            const float h = EntryHeight(entry);
            if (entry.IsLine()) {
                canvas.Fill(b.x + 3 * d, y + 2 * d, b.w - 6 * d, d, kEdge);
                y += h;
                continue;
            }
            if (static_cast<int>(i) == list.picked) {
                canvas.Fill(b.x + d, y, b.w - 2 * d, h, kPicked);
            }
            const Color color = entry.enabled ? kWhite : kOff;
            const float ty = y + 2 * d, mx = b.x + 3 * d;
            if (entry.mark == BarEntry::Check) {
                static constexpr int kTick[][2] = {{0, 3}, {1, 4}, {2, 5}, {3, 4}, {4, 3}, {5, 2}, {6, 1}};
                for (const auto &p : kTick) {
                    canvas.Fill(mx + p[0] * d, ty + p[1] * d, d, d, color);
                }
            } else if (entry.mark == BarEntry::Dot) {
                canvas.Fill(mx + 2 * d, ty + 2 * d, 3 * d, 3 * d, color);
            }
            canvas.Text(b.x + kMarkDots * d, ty, d, 0, color, entry.label.c_str());
            if (!entry.shortcut.empty()) {
                const float sx = b.x + b.w - kArrowDots * d - TextWidth(entry.shortcut.c_str(), d);
                canvas.Text(sx, ty, d, 0, entry.enabled ? kDim : kOff, entry.shortcut.c_str());
            }
            if (!entry.children.empty()) {
                canvas.Text(b.x + b.w - kArrowDots * d + 2 * d, ty, d, 0, color, ">");
            }
            y += h;
        }
    }
}

} // namespace recomp
