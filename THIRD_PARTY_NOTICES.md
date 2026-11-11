# Third-party notices

recomp-common is licensed under the GNU General Public License v3.0 (see `LICENSE`).

## Code

`src/` (the in-game menu, its text, the settings rules) has none from other projects: every file was
written for this project and the ports it came out of, and its only dependencies are the C and C++
standard libraries.

`launcher/` contains code from one other project, and its build downloads three libraries.

### wiiparty-recomp (the launcher's look)

The launcher descends from the launcher of **wiiparty-recomp** by **arelkair**
(https://github.com/arelkair/wiiparty-recomp, folder `launcher/`), which is licensed
**GPL-3.0-or-later**. The GPL-3.0-or-later allows its code to be used under the GPL-3.0, which is
this repository's license. Compared with that project at commit `2860f2c` (2026-10-10):

| File here | From | What |
|---|---|---|
| `launcher/src/ui.cpp`, `ui.h` | `launcher/src/ui.cpp`, `ui.h` | The look: palette, fonts, buttons, switches, the dropdown, animation. As they were there but for the ring round what the arrow keys or a gamepad are on, and a few lines (that project has since added font decompression and a software-rendering switch, which are not here). |
| `launcher/src/embedded.h` | `launcher/src/embedded.h` | Two declarations, unchanged. |
| `launcher/src/layout.h`: `Card`, `row`, `inset_separator` | `launcher/src/layout.h` | The card and row helpers the pages are built from. |
| `launcher/src/main.cpp` | `launcher/src/main.cpp` | The window and the frame loop; about two thirds of its lines were that project's when it came here. |
| `launcher/embed_fonts.cmake` | `launcher/cmake/embed.cmake` | Written here, to the same scheme (fonts compiled in, handed out by `blob(name)`). |
| `launcher/src/game_process.cpp`: quoting an argument for Windows | `launcher/src/subprocess.cpp` | Follows that file's function (the usual algorithm for `CommandLineToArgvW`). |

Each of these files says so at its top. The three that are that project's files nearly unchanged
(`ui.cpp`, `ui.h`, `embedded.h`) are marked `GPL-3.0-or-later`, as that project licenses them; the
others, which are mostly this project's, are marked `GPL-3.0-only` with a note of what came from
there. That project's files carry no copyright line of their own; its author is credited here and in
each file. Everything else in `launcher/` (the pages, the settings file editor, starting the game,
"Add to Steam", the key and button chooser, the description a port supplies) was written for this
project and the ports it came out of.

### Libraries the launcher's build downloads

None of these is stored in this repository. `launcher/launcher.cmake` downloads each when a launcher
is configured and checks its published SHA-256.

- **SDL 3.4.16:** window, input, file dialogs and rendering (the system's is used where there is one;
  otherwise it is built in statically). **zlib License**. Copyright (C) 1997-2026 Sam Lantinga.
- **Dear ImGui 1.92.9b:** user interface, compiled into the launcher. **MIT License**.
  Copyright (c) 2014-2026 Omar Cornut.
- **Inter 4.1 (Regular and SemiBold):** the font, **compiled into the launcher program**.
  **SIL Open Font License 1.1**. Copyright (c) 2016 The Inter Project Authors.

A port that gives out a built launcher must include these three licenses with it (the font is inside
the program), and the GPL-3.0 with the offer of source that goes with it.

### Pictures compiled into the launcher (ForkAde's fork)

- **flag-icons 7.6.0:** the country flags of the public lobby (`launcher/src/flags_atlas.inc`, its 249
  ISO 3166-1 flags drawn at 24x18 into one 256-colour picture). **MIT License**. Copyright (c) 2013
  Panayiotis Lipiridis. https://github.com/lipis/flag-icons

Code added later from another project must have a license compatible with the GPL-3.0 and be listed
here, with its notice kept in the file. `tools/audit.sh` fails if a file marked as another project's
is not one of those listed above.

## The font of the menu

`src/text.h` contains a 5x7 dot-matrix font for ASCII 32 to 126 (475 bytes). It is the generic
dot-matrix character set that has been reproduced in LCD and LED display code since the 1980s,
the family that descends from character-generator ROMs. It was typed in for this project, not
copied from a file of any particular project; like every 5x7 table it shares many glyphs with the
others (a little over half of them are the same as in Adafruit GFX's `glcdfont.c`, for instance,
and the rest differ).

## Nothing from any game

No game's code, data, text, graphics or keys are in this repository, and none are needed to build,
test or use it.
