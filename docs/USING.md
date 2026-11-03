# Using recomp-common in a port

## Taking the copy

1. Clone this repository beside your project's (`../recomp-common`), or anywhere and point
   `RECOMP_COMMON` at it.
2. Copy `tools/sync-common.sh` into a `tools/` folder of your project. The copy of the library goes
   in a `recomp-common/` folder beside that `tools/` folder.
3. Run it:

   ```sh
   tools/sync-common.sh
   ```

   It copies `src/` exactly as committed at this repository's HEAD and writes
   `recomp-common/COMMIT`: the commit, and a SHA-256 of each file.
4. Commit `recomp-common/` and the script in your project.
5. Add `recomp-common/menu.cpp` to your build (C++17). A project in C also adds
   `recomp-common/menu_c.cpp`, compiles those two files with a C++ compiler and links with it (or
   links the C++ standard library).
6. Add the check to whatever you run before committing, and to CI:

   ```sh
   tools/sync-common.sh --check
   ```

   It fails if the copy is not what `COMMIT` records. With a clone of this repository at hand it
   also compares each file with that commit.

**Never edit the copy.** To change the menu, change it here (and run `make test`), then run the sync
script in each project and commit the new copy there. Projects move to a new version only when they
run the script, so they can be on different versions; the `COMMIT` file says which.

The copy is GPL-3.0 code. Keep the notice at the top of each file, and ship the license text with
your project.

## The interface (C++)

Everything is in `menu.h`, namespace `recomp`. `examples/shot.cpp` uses all of it.

```cpp
recomp::MenuConfig config;

// The first page, in groups; a line is drawn between groups.
config.groups = {
    {{MenuRow::Save}, {MenuRow::Load}, {MenuRow::Slot}},
    {{MenuRow::Pages}, {MenuRow::Speed}, {MenuRow::Action, kInsertCoin, "Insert coin", "Puts a coin in the first slot."}},
    {{MenuRow::Action, kReset, "Reset", "Starts the machine again."}, {MenuRow::Quit}, {MenuRow::Resume}},
};

// The game's optional features and commands. The menu makes a page for each category.
config.features = {{"Cheats", "Endless time", "The round's clock stands still."},
                   {"Cheats", "Health", "Whose health is kept full.", {"Off", "Player 1", "Both"}}};
config.commands = {{"Training", "Training mode", "Straight to a practice round."}};
config.value = [](size_t feature) { return /* 0 = off, or which choice */ 0; };
config.setValue = [](size_t feature, int value) { /* change it; write the settings file */ };

// The Speed row: the names, the first being full speed. What a speed means is the port's affair.
config.speeds = {"Full", "1/2", "1/4"};
config.speedDescription = "Left/Right: slow motion.";

// What the menu's buttons are called on this console, for the hint line.
config.confirmName = "A, B or C";
config.closeName = "Esc";

menu.Configure(std::move(config));
```

| Row kind | What it is | What `Buttons` returns when chosen |
|---|---|---|
| `Save`, `Load` | "Save state", "Load state" | `Chosen::Save`, `Chosen::Load` (use `Slot()`) |
| `Slot` | The slot, 1 to 9 | nothing: the menu keeps it (`Slot()`, `SetSlot()`) |
| `Speed` | One of `speeds`; left out if there are none | nothing: the menu keeps it (`Speed()`, `SetSpeed()`) |
| `Pages` | Stands for several rows: the commands of no category, then a row per category | a command: `Chosen::Command` with its index |
| `Action` | The port's own row: a title, a description, an id | `Chosen::Action` with the id |
| `Quit` | "Quit game" | `Chosen::Quit` |
| `Resume` | Closes the menu | nothing |

While the menu is open, each time buttons are newly pressed:

```cpp
unsigned buttons = 0;   // kMenuUp, kMenuDown, kMenuLeft, kMenuRight, kMenuConfirm, kMenuBack
const recomp::Menu::Chosen chosen = menu.Buttons(buttons);
switch (chosen.kind) {
case recomp::Menu::Chosen::Save:    /* save to menu.Slot(); menu.Close() if it worked */ break;
case recomp::Menu::Chosen::Load:    /* or menu.Note("Nothing is kept in slot 3.") if it did not */ break;
case recomp::Menu::Chosen::Action:  /* chosen.id is the row's id */ break;
case recomp::Menu::Chosen::Command: /* chosen.id is its place in config.commands */ break;
case recomp::Menu::Chosen::Quit:    /* end the game */ break;
default: break;
}
```

The port closes the menu after carrying out what was chosen (so that a failure can leave it open
with a note). Resume, and Back on the first page, close it themselves.

To show it, copy the game's picture as it stood when the menu opened and draw over the copy each
time something changes:

```cpp
shown = picture;                                  // 32-bit pixels, red in the low byte
menu.Draw(shown.data(), width, height);           // then show `shown` however you show pictures
```

Sizes follow the picture's height (one font dot is `height / 300` pixels, at least 1), so draw at
the size the player sees, not at the console's resolution, if you want sharp text in a big window.

### How the buttons behave

- Up and Down choose a row and wrap around.
- Confirm chooses a row, switches a feature, goes to the next of several values (around), or goes
  into a page.
- Left and Right go through a row's values and stop at the ends. On a row with no values, Left
  leaves a page for the one it was reached from, and Right on a page's row goes into that page.
- Back leaves a page for the one it was reached from; on the first page it closes the menu.
- A `MenuRow::Mute` row ("Mute", Off or On) switches the game's sound off and on through `MenuConfig::muted` and
  `setMuted`. The Speed and Mute rows may be on a page rather than the first (`MenuPage::rows`). Give Space, Enter and a gamepad's A to Confirm, and Backspace and B to Back.
- The menu opens on the page and the row it was closed on.

## The interface (C)

`menu_c.h` is the same thing as plain functions and structs: `rmenu_new`, `rmenu_configure` (rows
carry a `group` number instead of being nested; feature choices are a `NULL`-ended array of names),
`rmenu_open`, `rmenu_buttons` (returns `RMENU_SAVE`, `RMENU_ACTION` and so on, with the id through a
pointer), `rmenu_draw`. Everything given to `rmenu_configure` is copied except the two function
pointers and `user`. `tests/c_test.c` is a complete example.

## What a port is expected to do

These are not enforced by the library; they are what makes every port feel the same.

- **Esc, F1 and a gamepad button open and close the menu.** Nothing but the menu's "Quit game" row
  (and closing the window) ends the game; Esc does not.
- **The first page** is save states, then what the game offers, then Reset / Quit game / Resume,
  with Resume last.
- **The game stands still** while the menu is open, and buttons held as it closes do not reach the
  game until they are let go.
- **With the menu closed nothing changes:** no cost per frame, no change to input timing.
- **A test flag** that opens the menu at a frame, presses given buttons and writes the picture
  (`--menu FRAME:KEYS[:shot.png]`), so the menu can be checked without a window.

## Text on its own

`text.h` (namespace `recomp::text`) is a header with no dependencies: a `Canvas` over a pixel
buffer with `Fill` (a blended rectangle) and `Text` (wrapped at spaces to a width), and `TextWidth`
and `TextLines` to measure. Ports use it for readouts over the picture as well.

## Settings: the rules a launcher and its game share

`settings.h` (namespace `recomp`; `settings_c.h` for C) is the one place that says what the
`[features]` section of a port's `settings.ini` means:

```ini
[features]
preset = enhanced        ; pure | enhanced | custom
free_play = 0            ; a feature's own value wins over the preset: 0, 1, or the number of a choice
```

- **The game lists its features** for the launcher: `--list-features` prints
  `recomp::FeatureListText(features)`, a feature a line (name, category, flags, title, description
  and, when it is a choice of several things, their names with `|` between them, tab-separated).
  Features flagged `kFeatureSession` last one run and are not listed or saved.
- **Both read values the same way:** `recomp::FeatureValues(ini, features)` gives every feature's
  value: the file's if it has one, else the preset's (on only in `enhanced`, for features flagged
  `kFeatureEnhanced`). Of a category's `kFeatureExclusive` features only one is on.
- **Both change a value the same way:** `recomp::SetFeature(ini, features, index, value)` makes the
  preset `custom`, writes down the features that were following the preset so that nothing else
  changes, and never rewrites a value that was already in the file. `recomp::SetPreset` chooses a
  preset.
- `recomp::Ini` is the file itself: `load`, `get`, `set`, `save`. It keeps every line it was not
  asked to change, comments and other sections included, so the launcher, the game and the player
  can all write to one file.

With every feature off (the `pure` preset, or no file) a game must run exactly as the original.

## The launcher

A port's launcher is the shared one plus one file of its own. `launcher/example` is a complete
example with no game; a port's `launcher/CMakeLists.txt` is its `CMakeLists.txt` with other names:

```cmake
set(LAUNCHER_GAME_NAME "My Game")          # as the player reads it
set(LAUNCHER_GAME_EXE mygame)              # the game program, beside the launcher (mygame or mygame_app)
set(LAUNCHER_APP_ID org.example.mygame)
set(LAUNCHER_HOME_ENV MYGAME_HOME)         # a variable naming a folder to use instead of the player's
set(LAUNCHER_DISCLAIMER "Not affiliated with ...")
set(LAUNCHER_PORT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/description.cpp)
include(../recomp-common/launcher/launcher.cmake)     # makes the target `launcher`
```

`description.cpp` defines `launcher::describe()` (`launcher/src/launcher.h`):

| Field | What |
|---|---|
| `original` | Where the original ran, as the Features page says it: "in the arcade", "on the console". |
| `file_heading`, `file_intro`, `file_empty`, `file_not_set_up`, `file_filter`, `file_key` | The words for the player's game file, the file dialog's filter, and its key in `launcher.ini`. |
| `check` | Looks at the player's file and says `Good` (the file the port was made from), `Usable` (it may run) or `NotUsable`, with a line for the player. It must only read the file. |
| `check_on_thread` | For a check that reads a large file: the page says "Checking..." meanwhile. |
| `find_nearby` | Optional: a game file near the program, for a build inside the source tree. |
| `arguments` | What the game is started with, from the file, the player's folder and the Display switches. |
| `mute_argument` | What the game is started with when the Game page's "Muted" switch is on (`launcher.ini`, `[audio]`); `"--mute"` unless the port says otherwise, `nullptr` for no switch. The game's menu should have a row that switches the sound off and on. |
| `folders` | Folders the player may choose for the game, as rows of the Sound card with Choose… and Clear (`FolderChoice`: the key in `launcher.ini`, beside the game's file; title; what it is for; the argument). While one is chosen the game is started with `ARGUMENT FOLDER` after the port's own arguments. The system's folder dialog chooses, or the launcher's own browser when a gamepad is in use. |
| `display` | The Display card's switches (`launcher.ini`, `[video]`): key, title, detail, the default, and another switch it needs. |
| `bindings` | The game's buttons and their usual keys and gamepad buttons (`bindings.h`). With it, the Controllers page lets the player choose them, for each of its players: one to four (`BindTable::players`; two unless it says). |
| `controls_heading`, `controls` | Rows of text for what is fixed (the menu's keys and so on). |
| `subtitle` | The line under the game's name in the sidebar. |
| `pages` | Pages of the port's own, before the launcher's in the sidebar (below). |
| `game_page` | `false`: no Game page, game file or Display card; the port's pages have Play. |
| `features_name`, `features_intro`, `presets` | Another name and other first lines for the Features page, and whether it has the preset chooser. |

**Pages of a port's own.** A `launcher::CustomPage` is a name and a `draw(PageContext&)`, and
optionally `drop` (a file dropped on the window) and `busy` (work is going on on another thread:
keep drawing). The page is drawn inside the launcher's page column with `ui.h` (text, buttons,
switches, dropdowns) and `layout.h` (`heading`, `label`, `Card`, `row`, `inset_separator`), so it
looks like the others. The `PageContext` has the column's width, the window (for file dialogs),
`launcher.ini` and `settings.ini`, the player's folder, the program's folder, whether the game is
running, and `play()`, which starts it with the port's `arguments`. A port's pages come out in the
`--screenshot` pictures like the others, in the sidebar's order.

The player's choices are kept in their folder (`SDL_GetPrefPath("", LAUNCHER_GAME_NAME)`, or the
folder `LAUNCHER_HOME_ENV` names): `launcher.ini` is the launcher's own (where the game file is, the
Display switches), `settings.ini` is the one the game reads too (features, keys and buttons) and is
handed to it by the port's `arguments`.

**Keys and buttons.** `launcher/src/bindings.h` (it needs only SDL 3) is the format and its reader:
`[keyboard1]`, `[gamepad1]` (and `2` for a second player), an action's name and a key, button or
trigger by SDL's name, or `none`. A game written against SDL 3 includes the same header and reads
the file once when it starts (`recomp::LoadBindings`); while playing it looks only at that table, so
choosing keys adds nothing to reading input. A game not on SDL 3 reads the same sections itself.

**A gamepad, or the arrow keys, alone.** Everything in the launcher can be reached and pressed
without a mouse: the d-pad, the left stick or the arrow keys move a ring; A, Space or Enter presses
what it is on; B or Esc goes back (closes a list, or goes from a page to the sidebar); the shoulder
buttons change the page; the right stick scrolls. (What the mouse does by dragging, a collection's
games' order in the sidebar, the ring does with Move up and Move down on the game's page.) A port's own pages need do nothing for this if
they are made of `ui.h`'s widgets. A game's in-game menu should answer to the same: Space, Enter and
a gamepad's A choose, Backspace and B go back, whatever the game's own buttons are set to.

**Checking it without a window.** `<game>-launcher --screenshot PREFIX [--size WxH]` writes a
picture per page and shows nothing. With `SDL_VIDEO_DRIVER=dummy SDL_RENDER_DRIVER=software` it
needs no display at all, and two runs give the same bytes, so a port can compare before and after.
`--screenshot PREFIX --press KEYS` presses keys one after another and then writes one picture,
`PREFIX.png`: `u d l r` are the arrow keys, `e` Enter, `s` Space, `x` Esc; `U D L R A B [ ]` are the
d-pad, A, B and the shoulder buttons of a gamepad that exists only in the program; a digit shows that
page; `.` waits; `@X,Y;` moves the mouse to that place in the window, and `(` and `)` press and let
go of its left button (a drag: `@100,288;(@100,350;@100,406;)`).

**A small window.** The launcher works down to 600×400 (and never opens bigger than the screen): below
760 wide its sidebar is narrower and the page has less room round it, a card's row puts its control
under its words when there is too little room beside them (`layout::row`), and the sidebar's list
scrolls when it is longer than the window. A port's own pages built of `layout.h`'s rows close up
the same way.

A port that gives out a built launcher must include the licenses of SDL, Dear ImGui and the Inter
font with it; see `THIRD_PARTY_NOTICES.md`.
