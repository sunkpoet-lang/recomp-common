# Changelog

Versions follow [semantic versioning](https://semver.org/). Before 1.0 the interface may change in
a minor version. A port's copy only changes when it runs `tools/sync-common.sh`.

## Unreleased

- **Menu bar** (`menubar.h`): the view for a mouse and a keyboard beside the in-game menu. A bar of menus along
  the top of the window and the same menus as a popup where the player right-clicks: entries with a label, the key
  that does the same, a check or a dot, dim when they cannot be used now, lines between groups and submenus. The
  pointer picks and opens submenus, a click chooses (a dim entry or a line does nothing), and menu.h's buttons work
  it too (Up and Down past dim entries, Right and Left through submenus and the bar's menus, Confirm, Back). Menus
  stay inside the canvas (up and to the left of a click near the corner, a submenu at the left where the right has
  no room); the bar can be hidden (fullscreen) with the popup still there. Drawn with text.h's font at a whole number
  of pixels to a dot. Nothing in `Menu` changes.
- The in-game menu's rows in the menu bar (`AddMenuEntries`, `ChooseMenuEntry`, `BarOptions`), so that the two
  offer the same: each row in the bar's menu its new `MenuRow::bar` names (C: `RMenuRow::bar`), or by its kind (Save,
  Load, Slot, actions and Quit in Game; Speed and the pages in Features; Mute in Audio). Save state, Load state, Slot
  and Speed are submenus of their values; a page is a submenu of its commands, the pages under it and its features
  (a check, or a submenu of choices); a line between groups; Resume left out. Choosing one changes the slot, the
  speed, a feature or the sound, or hands back what `Menu::Buttons` would. Load state's empty slots can be dim and a
  slot can say when it was kept. C hosts that fill `RMenuRow` in order add a `NULL` for `bar` (else
  `-Wmissing-field-initializers` says so; the menu is the same).
- **Multi game**: a collection's modes that play several of its games one after another (`Collection::multi`,
  `launcher::MultiMode`), on a page of their own after Controllers. The player chooses the games (those that have
  the mode and a page of their own; kept in `launcher.ini`, `[multi.<key>] off`), their order (the sidebar's, or
  shuffled, anew each time round) and the mode's choices (`MultiChoice`, each shown only with another's value if it
  says); Start starts the first with its Play's arguments and the mode's. A game that ends with `next_code` has had
  its turn and the next starts; ended any other way, the run stops (and says why), as does Stop after this game.
  `--page multi` opens the page. A collection with no modes has the three pages it had.
- A collection's games are in the player's order: dragging a game's row in the sidebar moves it (the others make
  room), and Move up and Move down on the game's page (under Play) do it with a gamepad or the keys. The Games page
  and the Controllers page's Layout for list them the same way. Kept in `launcher.ini`, `[games] order` (their ids;
  a game not named there comes after, in the collection's order).
- A small window: the launcher no longer opens bigger than the screen, and can be made as small as 600×400 (it was
  800×540, which a small screen with large text could not show: the rows' words went under their buttons). Below
  760 wide the sidebar is narrower (a long name is cut short with "…") and the page has less room round it;
  `layout::row` puts its control under the words when there is too little room beside them; and the sidebar's list
  scrolls (the mouse wheel, and to keep the ring's row or the page shown in view) when it is longer than the window.
  A port's launcher at its usual size is the same pixels as before.
- A collection's Games page: the Updates card is always at the top (it was at the page's end until there was a new
  version), and each game's Play comes before its Choose…, where the ring lands coming into the row.
- `--press`: `@X,Y;` moves the mouse, `(` and `)` press and let go of its left button.
- **Portable mode** (`src/folders.h`): `portable.txt` beside the launcher (on macOS beside the app, beside the
  AppImage for one) keeps the player's folders in `user/<name>/` there instead of the system's. The first time,
  the launcher offers to copy what the system's folders have, never moving or writing over anything; a folder it
  cannot write to is said, and it stops. Taking `portable.txt` away goes back to the system's folders. The first
  page has a Your files card: where they are kept, and Show to open the folder. `make test` tests the folders,
  and that an update keeps `portable.txt` and `user/`.

- A collection's games can each have a layout of their own: the Controllers page's Layout for chooses every
  game's or one game's, and Its own layout gives that game keys and buttons of its own, with its own table's
  names for its buttons (`Description::bindings`; `Collection::bindings` is the table every game shares). It
  starts as every game's is; switched off, the game has every game's again. Which games have their own is kept
  in `launcher.ini`, under `[controls]`; the shared actions a game's own table has not got are `none` in its
  `settings.ini`. A collection that gives its games no table of their own is as it was.

- **Updates**: a launcher built with `LAUNCHER_UPDATE_REPO` and `LAUNCHER_VERSION` checks the port's GitHub
  releases when it opens (the first page's Updates card has the switch and Check now) and offers a newer version
  at the top of that page. Update downloads the system's archive from the release, checks it against the
  manifest the release carries beside it (`launcher/src/update.h` has its format), unpacks it, checks each file
  that changed against its SHA-256 and puts them all in place or none, then starts the new launcher. (At first
  each file was a gzipped release file of its own: a release then had a file for every file of every download.
  A release now has only its archives and a manifest per system, and the launcher needs no zlib.) A beta is offered only to a beta; a build that is not a release's
  download is only told. `make test` runs a whole check and install from releases in a folder.

- Bindings for up to four players (`kBindMaxPlayers`; `BindTable::players` is still two unless a port says):
  `[keyboard3]`, `[gamepad3]` and so on, the third gamepad player 3's, and a button for each player on the
  Controllers page. For cps2recomp's three-player games. A port with one or two players is as it was.
- A collection's game is told the file chosen for another of its games (`Launch::file_of`, by the game's id):
  for a game that reads another's files too (cps2recomp's Vampire Savior Definitive, which reads Vampire Savior
  2's ROM set).

- The launcher's Sound card can have folders the player chooses for the game (`Description::folders`,
  `FolderChoice`): a row with Choose… (the system's folder dialog, or the launcher's own browser with a gamepad)
  and Clear, kept in `launcher.ini`, and given to the game as `ARGUMENT FOLDER`. For cps2recomp's custom
  soundtrack (the folder of the player's music files).

- A collection's games can run at once, one of each: Play is only off for a game that is running. A shared
  feature one game's menu changes while another runs is set in that one when it ends (unless its own menu changed
  it too). The launcher's arrow keys and gamepad stay on while a game runs, unless another window is in front.
  A port's launcher is as it was.

- **The launcher can be a collection**: one launcher for several games (`LAUNCHER_COLLECTION`,
  `launcher::describe_collection()`; README). Games, Features (what more than one game has, under the same
  name and with the same choices, set for all of them; what a game's menu changes of it is set for the others
  when the game ends), Controllers (every game's), and a page for each game whose file is chosen. Each game keeps
  its own folder and complete `settings.ini`, and the file chosen in a launcher of its own is taken over.
  `--play ID`, `--play [ID] -- ARGS` (more arguments for the game), `--page ID`; what `--play` cannot do is
  said on the terminal too. A port's launcher is unchanged (its pages' pictures are the same, pixel for pixel,
  and so are its pictures after keys and gamepad buttons pressed).
- `kFeatureOwn` (64): a game's feature that a collection's launcher never sets for another game.
- The launcher's `Description::check`, `find_nearby`, `arguments` and the new `candidates` (where in a
  folder a game's file may be) are `std::function`s; a plain function still does.

## 0.5.0 (2026-10-15)

- In-game menu: a description longer than three lines no longer runs over the hint at the panel's
  foot. The panel leaves room for the longest description on the page (three lines at least), and
  the rows close up when the picture is short.
- In-game menu: a value too long for the room beside its title (a long choice's name) is drawn in
  the small letters, and cut short with ".." if even those do not fit, instead of over the title.
- In-game menu: the panel is opaque; the game's own text no longer shows faintly through it.
- In-game menu: Right on a page's row goes into that page, as Confirm does (Left still comes back).
- **The launcher's Features page has a search box.** Only the features with every word typed in
  their name, description, category or choices are shown, with how many of them match. Ctrl+F (or
  Cmd+F) goes into it; Esc empties it, a second Esc leaves it; with a gamepad, A goes into it and B
  leaves it with what was typed kept. The other pages' pictures are the same bytes as before.
- Launcher tests: `--press` takes `f` for Ctrl+F and types what is between double quotes.

## 0.4.0 (2026-10-13)

- **The launcher works with a gamepad, and with the arrow keys, alone.** The d-pad, the left stick
  or the arrow keys move a ring from one button, switch, list or sidebar row to the next; A, Space
  or Enter presses what the ring is on; B or Esc closes a list, or goes from a page to the sidebar;
  the shoulder buttons go to the page before and after; the right stick scrolls a page. In the
  sidebar, up and down change the page and right goes into it (at its first control); the ring
  never jumps from a sidebar row to whatever in the page happens to be nearest. With a gamepad connected the ring
  is shown from the start; using the mouse hides it. A launcher nobody presses a key in looks as it
  did: its pages' pictures are the same bytes.
- A game file can be chosen with a gamepad: Choose, pressed with one, opens a list of folders and
  files in the launcher's own window (the system's file dialog wants a mouse or a keyboard).
- Choosing a button on the Controllers page with a gamepad: the button pressed is the button
  chosen and nothing more; while a key is waited for, any gamepad button leaves things as they were.
- **Muted**: the Game page has a Sound card with one switch, off unless the player switches it on.
  When it is on the game is started with `--mute` after the port's own arguments
  (`Description::mute_argument`; `nullptr` for no card). A game should take that argument and have
  a row in its menu that switches the sound off and on.
- **Menu**: a `MenuRow::Mute` row ("Sound": On / Muted) through `MenuConfig::muted` and `setMuted`
  (`RMENU_ROW_MUTE` for C). `kMenuBack` now leaves a page for the one it was reached from and closes
  the menu only on the first page (it closed the menu from anywhere). A host should give Space,
  Enter and a gamepad's A to Confirm, and Backspace and B to Back.
- `ui.h`: `focus_item`, `focus_next_item`, `show_focus`, `focus_shown`, `gamepad_in_use`, and
  `nav_item` takes `reachable`. A port's own pages get the ring from the widgets they already use.
- `--screenshot PREFIX --press KEYS` presses keys, and the buttons of a gamepad made for the test,
  then takes one picture: the launcher's navigation can be checked with no window and no gamepad.

## 0.3.0 (2026-10-13)

- **The launcher takes pages of a port's own** (`launcher.h`: `Description::pages`), shown in the
  sidebar before its own and drawn with the launcher's widgets. A page may take files dropped on
  the window and may say it is busy. `launcher/src/layout.h` now holds what pages are built from
  (`Card`, `row`, `inset_separator`, `heading`, `label`), for those pages to use.
- A port with no one game file can leave out the Game page (`game_page = false`); its own pages
  then start the game (`PageContext::play`).
- The Features page can be given another name and other first lines, and can be shown without the
  preset chooser (`features_name`, `features_intro`, `presets`). The sidebar's line under the
  game's name can be set (`subtitle`).
- When the game stops, the launcher reads `settings.ini` again, so a feature switched in the game's
  menu is not undone by the launcher's next save.
- A launcher that sets none of these is as it was: its pages' pictures are the same bytes.
- **Menu**: pages under pages, and a page's row wherever the host wants it. `MenuConfig::pages`
  names a page's parent (its row is then on the parent's page, after the parent's commands), what
  its row says, and whether the row counts the features that are on. `MenuRow::Page` is a row for
  one page on the first page; the `Pages` row leaves that page out. Left and Back go to the page a
  page was reached from. With no `pages` and no `Page` row the menu is as it was.

## 0.2.0 (2026-10-12)

- **Settings** (`settings.h`, and `settings_c.h` for C): the `[features]` rules a launcher and its
  game must agree on, in one place. A file editor that keeps every line it was not asked to change;
  the feature list as text (`--list-features`), now with a feature's choices as an optional sixth
  field (a reader of five fields still reads it, and a list of five fields still parses); reading
  every feature's value; changing one without rewriting the others; choosing a preset.
- **The launcher** (`launcher/`): the Game, Features and Controllers pages and Play, as one program
  for every port. A port supplies a description (`launcher.h`): the words for its game file, the
  check of it, the game's arguments, the Display card's switches, the actions whose keys and gamepad
  buttons can be chosen. Features that are a choice show as a dropdown. `--screenshot` shows no
  window and takes `--size WxH`. `launcher/example` builds it with a stand-in for the game.
- **Key and button bindings** (`launcher/src/bindings.h`, SDL 3): the `[keyboard1]` / `[gamepad1]`
  format, for the launcher and for a game's frontend.
- `tools/sync-common.sh` copies `launcher/` as well as `src/`. The `COMMIT` file is as before; a
  project's copy of the script must be replaced with this one.
- The launcher's look comes from another project and its build downloads three libraries:
  `THIRD_PARTY_NOTICES.md` credits them, and the audit knows which files are whose.

## 0.1.0 (2026-10-12)

The first public version.

- The in-game menu (`menu.h`): a first page in groups (save states, slot, speed, the host's own
  rows, quit, resume), a page per category of features and commands, on/off and choice features,
  a description and a note line, a hint line with the host's button names, scrolling about the
  chosen row where the rows do not fit, and room for a two-line hint on a narrow picture.
- A C interface (`menu_c.h`).
- Text (`text.h`): a 5x7 font, blended fills, wrapped text and its measurement.
- `tools/sync-common.sh`: a project's committed copy, the commit and hashes it records, `--check`.
- Tests, an example host with no game, the file audit, CI.
