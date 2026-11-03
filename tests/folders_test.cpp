// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of recomp-common: https://github.com/tmcleroy/recomp-common
//
// Where the player's things are (src/folders.h): the folder a download was unpacked to, and portable mode.
#include "folders.h"

#include <cstdio>
#include <fstream>

namespace fs = std::filesystem;

static int failures = 0;
#define CHECK(x)                                                         \
    do {                                                                 \
        if (!(x)) {                                                      \
            std::fprintf(stderr, "%s:%d: FAILED: %s\n", __FILE__, __LINE__, #x); \
            failures++;                                                  \
        }                                                                \
    } while (0)

static void unset_appimage() {
#ifdef _WIN32
    _putenv_s("APPIMAGE", "");
#else
    unsetenv("APPIMAGE");
#endif
}

int main() {
    unset_appimage();
    const fs::path root = fs::temp_directory_path() / "recomp-folders-test";
    fs::remove_all(root);
    // The folder the download was unpacked to
    CHECK(recomp::InstallFolder(root / "Game") == root / "Game");
    CHECK(recomp::InstallFolder((root / "Game").string() + "/") == root / "Game");
    CHECK(recomp::InstallFolder(root / "Games" / "My Game.app" / "Contents" / "MacOS") == root / "Games");
    CHECK(recomp::InstallFolder(root / "Games" / "My Game.app" / "Contents" / "Resources") == root / "Games");
    CHECK(recomp::InstallFolder(root / "Contents" / "MacOS") == root / "Contents" / "MacOS");  // (not in an app)
    CHECK(recomp::InstallFolder(root / "x.app") == root / "x.app");  // (a folder that only looks like one)
    const fs::path image = root / "Drive" / "Game.AppImage";
#ifdef _WIN32
    _putenv_s("APPIMAGE", image.string().c_str());
#else
    setenv("APPIMAGE", image.string().c_str(), 1);
#endif
    CHECK(recomp::InstallFolder("/tmp/.mount_Game123/usr/bin") == root / "Drive");
    unset_appimage();

    // Portable mode: only with portable.txt, a file, where the download was unpacked
    fs::create_directories(root / "Game");
    CHECK(recomp::PortableFolder(root / "Game").empty());
    std::ofstream(root / "Game" / "portable.txt") << "";
    CHECK(recomp::PortableFolder(root / "Game") == root / "Game" / "user");
    fs::create_directories(root / "Mac" / "My Game.app" / "Contents" / "MacOS");
    std::ofstream(root / "Mac" / "My Game.app" / "Contents" / "MacOS" / "portable.txt") << "";  // (in the app: not it)
    CHECK(recomp::PortableFolder(root / "Mac" / "My Game.app" / "Contents" / "MacOS").empty());
    std::ofstream(root / "Mac" / "portable.txt") << "";
    CHECK(recomp::PortableFolder(root / "Mac" / "My Game.app" / "Contents" / "MacOS") == root / "Mac" / "user");
    fs::create_directories(root / "Folder" / "portable.txt");  // (a folder of that name is not the marker)
    CHECK(recomp::PortableFolder(root / "Folder").empty());

    // A folder's name: kept, but for what Windows refuses in one
    CHECK(recomp::FolderName("X-Men: Children of the Atom", false) == "X-Men: Children of the Atom");
    CHECK(recomp::FolderName("X-Men: Children of the Atom", true) == "X-Men - Children of the Atom");
    CHECK(recomp::FolderName("Marvel vs. Capcom", true) == "Marvel vs. Capcom");
    CHECK(recomp::FolderName("A <b> \"c\" d/e\\f|g?h*", true) == "A b c defgh");
    CHECK(recomp::FolderName("Trailing. ", true) == "Trailing");
    CHECK(recomp::FolderName("??", true) == "_");

    fs::remove_all(root);
    if (failures == 0) {
        std::printf("folders_test: all as it should be\n");
    }
    return failures == 0 ? 0 : 1;
}
