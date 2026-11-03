# Changelog

Versions follow [semantic versioning](https://semver.org/). Before 1.0 the interface may change in
a minor version. A port's copy only changes when it runs `tools/sync-common.sh`.

## Unreleased

- **Paths with accents on Windows** (cps2recomp #223): SDL's paths and the ports' are UTF-8, and a
  `std::filesystem::path` made from a narrow string reads it in the system's code page there, so a folder named with an
  é, ã or ç was another folder. `Ini` takes a path given as text (`std::string`, `const char *`) as UTF-8,
  `LoadBindings` reads settings.ini by its wide name, and the launcher keeps its own folder (`SDL_GetBasePath`), the
  file dialog's start, the player's folder (`SDL_GetPrefPath`) and the settings path as UTF-8, and gives the updater's
  programs (curl, tar) their paths as UTF-8.
- **Updating on Android from the launcher** (cps2recomp #211): the update card's **Update** downloads the release's APK
  into the app's cache, checks it against the release's `SHA256SUMS.txt` (`Updater::sum_in`), and hands it to the
  port's activity (`installPackage(String path)`), which opens Android's installer (or first Android's Settings, for
  the player to allow the app to install apps). Then the card says so (`State::Downloaded`), with **Install** to open
  the installer again. A release without `SHA256SUMS.txt` still has **Download**, for the browser.
- **A waiting room for a launcher of one game** (`Online::room`, `room_protocol`): what a collection's Online page has,
  on the Game page under the Online card: the invitations waiting for this player, and the players in the room to
  invite to the game. The launcher checks in while the Game page is open. `App::room_url()` and `room_protocol()` are
  the collection's or the game's; `Online::game`, the id the service knows the game by, is the one game's there (`App::room_game`).
- The Your files card says "Settings, saves, save states and the logs": "each board's memory" meant nothing to a
  port of a console game.
- **A new version on Android** (cps2recomp #209): the updater checks the releases there too. As an app can't replace
  its own files, it looks for the newest release's APK rather than a manifest, and the update card's **Download**
  (`Updater::Status::download_url`) opens it for the player to install. `Updater::platform` is `android` there.
- **What's worth attention stands out on the Online page** (cps2recomp #196): an invitation, open matches that can be
  joined (their Join the primary button), the waiting room while someone is in it, and a new version are cards tinted and
  edged with the accent (`Card(width, highlight)`). A new version also puts a dot on the Games row and a card at the top of
  the Online page, whose **Update…** goes to the Games page's update card. The page's order: open matches (and joining
  with a code), the waiting room, hosting a match, then the features about a match.
- **The player's name with Join too** (cps2recomp #196): `App::join_arguments` gives `Online::name_argument NAME` to
  every Join (an open match, a code, an invitation, an address), and Host gives it whether or not there's a lobby, so
  that a game can show each player the other's name.
- **The Online page second**, after Games (cps2recomp #196): the pages played from first, then Features, Controllers
  and Multi game.
- **Speed and Mute on a page** (cps2recomp #196): `MenuPage::rows` puts those rows at the top of a page rather than
  on the first one (a page with only them is there too). The Mute row is "Mute", Off or On, rather than "Sound", On
  or Muted.
- **A port's pages as extras** (`Description::open_on_game_page`): the launcher opens on the Game page, not on the
  first of the port's own pages, and a file dropped on the window goes to a page of the port's own only while that
  page is shown (else it is the game file). For a port whose own page is an extra (Soulcalibur II's Guest page).
- **Dark unless the system says light**: where the system says nothing of its theme (Steam's Gaming Mode on the Steam
  Deck and Steam Machine, a Linux desktop with no preference) the launcher is dark, as Steam is, rather than light.
- **The sidebar scrolls with a finger** on a touch screen (`App::scroll_sidebar`), as the page beside it does: its list of
  games was reached only with a mouse wheel. A finger's drag is a scroll before it is a row dragged to another place.
- **Android** (cps2recomp #104): `launcher.cmake` makes the launcher a library there, and the launcher starts each
  game's library through the port's activity (`startGame`, `gameResult`), asks a game's library for its features
  (`RecompListFeatures`), copies the files the player chooses with the system's picker into `roms/` in its folder
  (`importFiles`, `importResult`; on a collection's first page **Add ROM sets…**), and makes the online requests
  through the activity (`curl`). The player's folders are in the app's folder on the shared storage. A finger dragged
  up or down scrolls what is under it (on every system with a touch screen), and a phone's Back is Esc.
  `LAUNCHER_VERSION` no longer needs `LAUNCHER_UPDATE_REPO`.
- **Play at the top** of a game's page (a collection's game, and a port's Game page), under its heading: its ROM set,
  Display and Sound, Online and Steam follow.
- **The Online page's waiting room** (`Collection::room`, `room_protocol`; online.h `room_check_in`): while the page is
  open and no game runs, the launcher checks in every five seconds with the player's name, the games they can play and
  whether others may invite them (`launcher.ini` `[online] invitable`), and lists the other players there, each with
  **Invite** (Host with `Online::invite_argument` and `invite_name_argument`, in the game chosen in the card) and
  **Block** (`[online] blocked`, with **Unblock all**). Invitations to this player are a card at the top of the page:
  **Accept** joins the match's code (**Add…** for a game without its file), **Decline**, **Block**. The player's
  secret, from which the service makes their id, is `[online] room`.

- **A collection's games in series** (`CollectionGame::series`): two or more games of a series with a page are under
  one sidebar row, which opens and closes (a chevron; kept in `launcher.ini` `[games] open`) and is a page of the
  series' games, each with Play and its page. A series moves as one in the player's order (dragged, or Move up/down
  on its page); a game under it moves among the series' games. The Games page puts the series' name over its games.
- **Arcade while waiting** on the Online card and the Online page's Host a match (`Online::arcade_argument`, kept in
  `launcher.ini` `[online] arcade`): a switch that makes Host start the game with that argument too, for a game that
  lets its host play alone until the other player comes.
- The launcher's version under its name, at the top of the sidebar. Online play's matches must be on the same version:
  an open match says its host's version (`OpenMatch::version`), and one on another version is greyed, its Join off;
  Join with a code says so too (`CodeLookup::version`).
- The lobby follows the player's name: with **Your name** set, a match hosted from the Online card or the Online page
  is listed (`Online::list_argument`); without one it's joined only by its code. The card's "List my matches in the
  lobby" switch is gone, and Your name is on the card always. The Online page's section is **Host a match**. The code
  field's hint is "The host's code".
- **Join with a code** on the Online page (`Collection::code_lookup`): the code a host gave, and Join, which asks the
  service which game the match is for (`lookup_code`) and starts that game with its Online's join argument and the
  code; a game whose file isn't chosen is said, with Add…; one the collection hasn't, and a code that isn't a match's,
  are said too.
- The Online page puts Open matches first, above Host in the lobby and In a match.
- The Online page shows the features of one category (`Collection::online_features`: what a match shows, the ping
  and the like) under **In a match**, as the Features page does, set for every game that has them.
- The Online page's open matches include those of games whose file isn't chosen yet, greyed, with **Add…**, which
  chooses it as the Games page's Choose… does (`Collection::file_name`: what the file is called there), and those of games the
  collection hasn't, greyed, by the name the host's game gave (`OpenMatch::title`), with **Update…** to the Games
  page when the launcher updates itself. A row can be greyed (`layout::row`'s `dim`).
- A collection's **Online** page (`Collection::lobby`, `online_intro`), after Multi game: **Host in the lobby** (your
  name, any of the games that can be played, Host: the game started with its Online's host, list and name arguments)
  and **Open matches**, every game's, with the game's name and Join (a game whose file is not chosen says so). The
  list comes from `lobby` every five seconds while the page is shown (`OpenMatch::game`).
- A lobby on the Online card (`launcher::Online::lobby`, `list_argument`, `name_argument`): **List my matches in the
  lobby** (opt-in, kept in `launcher.ini` `[online] listed`) and **Your name** (`[online] name`) make Host start the
  game with the list and name arguments; **Open matches** shows the lobby's list (fetched from `lobby` every five
  seconds while it is shown, on another thread: `fetch_lobby`, `src/online.h`), each with Join. The curl runner moves
  from `update.cpp` to `run_program.h`/`run_program.cpp`, which every launcher builds.
- A collection's **Only the games you have**: a switch on the Games page (shown once a game is set up; `launcher.ini`
  `[games] installed_only`, off unless switched on) that leaves the games whose file has not been chosen out of the
  Games page, the Features page (a feature none of the player's games has, and their names in its note) and the
  Controllers page's Layout for list. What is chosen on the Features page is still set for every game that has it.
- The Online card for a game that gives hosts a code (`launcher::Online::codes`): Host says the game shows a code to
  hand out, and Join takes the code or an address.
- Online play: a port that sets `Description::online` (`launcher::Online`: its host and join arguments, the port,
  a line about it) gets an Online card under Play. **Host** starts the game with the host argument and says this
  machine's addresses (`local_addresses()`, `src/online.h`: the home network's and Tailscale's, not virtual
  machines' or containers'); **Join** starts it with the join argument and the address typed (`ui::text_field`),
  kept in `launcher.ini` under `[online]`. Windows links `iphlpapi` and `ws2_32` for it.
- A long choice can be made shorter in the game's menu: a feature marked `kFeatureMenuChoices` has, under its row on
  the launcher's Features page (or a collection game's page), "In the game's menu: N of M" and Choose, which opens a
  switch for each of its choices. The game's menu (`MenuConfig::offered`) then goes past those switched off with
  Left, Right and Confirm; off and the one chosen now always stay. Kept in `settings.ini`, `[menu] <name> = 1,3`
  (`MenuChoices`, `SetMenuChoices`; no value: every choice). A collection sets it for every game that has the feature.

- The Speed row can have speeds faster than full: `MenuConfig::normalSpeed` is full speed's place in `speeds` (slowest
  first; the slower before it, the faster after) and where the menu starts. Left at 0, the row is as it was.
- A multi-game mode can list the keys and buttons that work while its games play (`MultiMode::buttons`,
  `launcher::MultiButton`: a title, what it does, and keycaps for its keys): a While it plays card under the mode's
  choices, and the same rows in the Now playing card.
- A menu bar entry can show a small picture before its label (`BarEntry::picture`, `pictureWidth`, `pictureHeight`;
  a save state's, say), fitted to the entry's height; its menu keeps room for the pictures.
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
