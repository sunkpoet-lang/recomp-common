// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of recomp-common: https://github.com/tmcleroy/recomp-common
//
// The launcher's updates (launcher/src/update.h): versions, manifests, SHA-256, and a whole check and install
// from releases in a folder (file:// URLs, through the system's curl, as a player's launcher does over https).
#include "update.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;
using launcher::Updater;

static int failures = 0;
#define CHECK(x)                                                         \
    do {                                                                 \
        if (!(x)) {                                                      \
            std::fprintf(stderr, "%s:%d: FAILED: %s\n", __FILE__, __LINE__, #x); \
            failures++;                                                  \
        }                                                                \
    } while (0)

static void write(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}
static std::string read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream all;
    all << in.rdbuf();
    return all.str();
}
static void wait(const Updater& updater) {
    for (int i = 0; i < 600 && updater.busy(); i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// A release in `folder`/<tag>/: its archive (the top folder "Top", with the files) and its manifest; `old`: a
// manifest of the first kind only (no archive). `files`: path, contents, executable
struct File {
    std::string path, text;
    bool executable;
};
static std::string release(const fs::path& folder, const std::string& tag, const std::vector<File>& files, bool corrupt = false, bool old = false) {
    const fs::path at = folder / tag, top = at / "pack" / "Top";
    fs::create_directories(at);
    std::string lines;
    for (const File& file : files) {
        write(top / file.path, file.text);
        const std::string sha = Updater::sha256_of_file(top / file.path);
        if (corrupt) {
            write(top / file.path, file.text + "!");
        }
        lines += "file " + sha + " " + std::to_string(file.text.size()) + (old ? " 0 " : " ") + (file.executable ? "x" : "-") + " " + file.path + "\n";
    }
    std::string manifest = std::string(old ? "recomp-update 1" : "recomp-update 2") + "\nversion " + tag.substr(1) + "\n";
    if (!old) {
#if defined(__APPLE__)
        const std::string name = "Top.zip", pack = "ditto -c -k --keepParent";
#elif defined(_WIN32)
        const std::string name = "Top.zip", pack = "tar -a -cf";
#else
        const std::string name = "Top.tar.gz", pack = "tar -czf";
#endif
#if defined(__APPLE__)
        const std::string command = pack + " \"" + top.string() + "\" \"" + (at / name).string() + "\"";
#else
        const std::string command = pack + " \"" + (at / name).string() + "\" -C \"" + (at / "pack").string() + "\" Top";
#endif
        CHECK(std::system(command.c_str()) == 0);
        manifest += "archive " + Updater::sha256_of_file(at / name) + " " + std::to_string(fs::file_size(at / name)) + " " + name + "\ntop Top\n";
    }
    fs::remove_all(at / "pack");
    write(at / (std::string("update-") + Updater::platform() + ".txt"), manifest + lines);
    return "{\"tag_name\": \"" + tag + "\", \"draft\": false, \"prerelease\": " + (tag.find('-') != std::string::npos ? "true" : "false") +
           ", \"html_url\": \"https://example.org/" + tag + "\", \"body\": \"Notes with \\\"quotes\\\" and \\u00e9\", \"assets\": [{\"name\": \"update-" +
           Updater::platform() + ".txt\", \"size\": 1, \"browser_download_url\": \"file://" + (at / "update-").generic_string() + Updater::platform() +
           ".txt\"}]}";
}

static void units() {
    CHECK(Updater::compare_versions("0.4.0-beta.1", "0.4.0-beta.2") < 0);
    CHECK(Updater::compare_versions("0.4.0-beta.2", "0.4.0") < 0);
    CHECK(Updater::compare_versions("v0.4.0", "0.4.0") == 0);
    CHECK(Updater::compare_versions("0.10.0", "0.9.9") > 0);
    CHECK(Updater::compare_versions("1.0.0-beta.10", "1.0.0-beta.9") > 0);
    CHECK(Updater::compare_versions("1.0.0-rc.1", "1.0.0-beta.3") > 0);
    CHECK(Updater::compare_versions("1.0", "1.0.0") == 0);

    Updater::Manifest m;
    const std::string sha(64, 'a');
    CHECK(Updater::parse_manifest("recomp-update 1\nversion 1.0.0\nfile " + sha + " 10 5 x Contents/MacOS/my game\n", m));
    CHECK(m.files.size() == 1 && m.files[0].path == "Contents/MacOS/my game" && m.files[0].executable && m.files[0].size == 10);
    CHECK(!Updater::parse_manifest("recomp-update 1\nversion 1.0.0\nfile " + sha + " 10 5 x ../outside\n", m));
    CHECK(!Updater::parse_manifest("recomp-update 1\nversion 1.0.0\nfile " + sha + " 10 5 x /etc/passwd\n", m));
    CHECK(!Updater::parse_manifest("recomp-update 1\nversion 1.0.0\nfile " + sha + " 10 5 x a//b\n", m));
    CHECK(!Updater::parse_manifest("recomp-update 1\nversion 1.0.0\nfile " + sha + " 10 5 x C:\\windows\n", m));
    CHECK(!Updater::parse_manifest("version 1.0.0\nfile " + sha + " 10 5 x a\n", m));  // (no header)
    CHECK(!Updater::parse_manifest("recomp-update 1\nversion 1.0.0\nfile " + sha + " 10 5 x a\nfile " + sha + " 10 5 x a\n", m));
    const std::string two = "recomp-update 2\nversion 1.0.0\narchive " + sha + " 99 Game-macOS.zip\ntop My Game.app\n";
    CHECK(Updater::parse_manifest(two + "file " + sha + " 10 x Contents/MacOS/my game\n", m));
    CHECK(m.archive == "Game-macOS.zip" && m.archive_size == 99 && m.top == "My Game.app" && m.files.size() == 1 && m.files[0].size == 10);
    CHECK(!Updater::parse_manifest(two + "file " + sha + " 10 5 x a\n", m));  // (the first kind's line)
    CHECK(!Updater::parse_manifest("recomp-update 2\nversion 1.0.0\ntop T\nfile " + sha + " 10 x a\n", m));  // (no archive)
    CHECK(!Updater::parse_manifest("recomp-update 2\nversion 1.0.0\narchive " + sha + " 9 ../a.zip\ntop T\nfile " + sha + " 10 x a\n", m));
    CHECK(!Updater::parse_manifest("recomp-update 1\nversion 1.0.0\narchive " + sha + " 9 a.zip\nfile " + sha + " 10 5 x a\n", m));

    // SHA256SUMS.txt (Android's APK is checked against it)
    const std::string other(64, 'b');
    const std::string sums = sha + "  Game-macOS.zip\n" + other + "  Game-android.apk\r\n" + sha + " *Game windows.zip\n";
    CHECK(Updater::sum_in(sums, "Game-android.apk") == other);
    CHECK(Updater::sum_in(sums, "Game windows.zip") == sha);
    CHECK(Updater::sum_in(sums, "Game-android") == "");  // (the whole name)
    CHECK(Updater::sum_in(sums, "apk") == "");
    CHECK(Updater::sum_in("ABC  Game-android.apk\n", "Game-android.apk") == "");

    const fs::path abc = fs::temp_directory_path() / "recomp-update-abc";
    write(abc, "abc");
    // (the FIPS 180-2 test vectors, each in pieces: the audit takes 32 hex digits together for a key)
    CHECK(Updater::sha256_of_file(abc) == std::string("ba7816bf8f01cfea") + "414140de5dae2223" + "b00361a396177a9c" + "b410ff61f20015ad");
    std::string million(1000000, 'a');
    write(abc, million);
    CHECK(Updater::sha256_of_file(abc) == std::string("cdc76e5c9914fb92") + "81a1c7e284d73e67" + "f1809a48a497200e" + "046d39ccc7112cd0");
    fs::remove(abc);
}

int main() {
    units();

    const fs::path root = fs::temp_directory_path() / "recomp-update-test";
    fs::remove_all(root);
    const fs::path app = root / "app", releases = root / "releases";
    const std::vector<File> old_files = {{"example-launcher", "old launcher", true}, {"game", "old game", true}, {"gone.txt", "obsolete", false},
                                         {"same.txt", "same", false}};
    const std::vector<File> new_files = {{"example-launcher", "new launcher", true}, {"game", "new game, longer", true}, {"same.txt", "same", false},
                                         {"sub/added.txt", "added", false}};
    for (const File& file : old_files) {
        write(app / file.path, file.text);
    }
    write(app / "player.ini", "the player's own file");
    write(app / "portable.txt", "");  // (portable mode: its marker and the player's folders beside the launcher)
    write(app / "user" / "Game" / "settings.ini", "the player's settings");
    const std::string list = "[" + release(releases, "v1.1.0-beta.1", {{"example-launcher", "beta", true}}) + ", " + release(releases, "v1.0.0", new_files) +
                             ", " + release(releases, "v0.9.0", old_files, false, true) + "]";
    write(releases / "releases.json", list);
    const std::string url = "file://" + (releases / "releases.json").generic_string();
#ifdef _WIN32
    _putenv_s("RECOMP_UPDATE_RELEASES", url.c_str());
#else
    setenv("RECOMP_UPDATE_RELEASES", url.c_str(), 1);
#endif

    {  // A copy that is not a release's download is told of the new version and left alone
        Updater updater(app, "some-other-launcher", "0.9.0", "owner/name");
        updater.check();
        wait(updater);
        CHECK(updater.status().state == Updater::State::NotInstalled);
        CHECK(updater.status().version == "1.0.0");  // (the beta is not offered to a version that is not one)
    }
    {  // The newest version
        Updater updater(app, "example-launcher", "1.0.0", "owner/name");
        updater.check();
        wait(updater);
        CHECK(updater.status().state == Updater::State::UpToDate);
    }
    {  // A file not as the release has it: nothing is changed
        const fs::path bad = root / "bad";
        const std::string bad_list = "[" + release(bad, "v1.0.0", new_files, true) + "]";
        write(bad / "releases.json", bad_list);
        const std::string bad_url = "file://" + (bad / "releases.json").generic_string();
#ifdef _WIN32
        _putenv_s("RECOMP_UPDATE_RELEASES", bad_url.c_str());
#else
        setenv("RECOMP_UPDATE_RELEASES", bad_url.c_str(), 1);
#endif
        Updater updater(app, "example-launcher", "0.9.0", "owner/name");
        updater.check();
        wait(updater);
        CHECK(updater.status().state == Updater::State::Available);
        updater.install();
        wait(updater);
        CHECK(updater.status().state == Updater::State::Failed);
        CHECK(read(app / "example-launcher") == "old launcher" && read(app / "gone.txt") == "obsolete" && !fs::exists(app / ".update"));
#ifdef _WIN32
        _putenv_s("RECOMP_UPDATE_RELEASES", url.c_str());
#else
        setenv("RECOMP_UPDATE_RELEASES", url.c_str(), 1);
#endif
    }
    {  // The update: three files to get (same.txt is already as it should be), one to take away
        Updater updater(app, "example-launcher", "0.9.0", "owner/name");
        updater.check();
        wait(updater);
        const Updater::Status found = updater.status();
        CHECK(found.state == Updater::State::Available);
        CHECK(found.version == "1.0.0" && found.notes_url == "https://example.org/v1.0.0");
        CHECK(found.message.find("3 files change") != std::string::npos);
        updater.install();
        wait(updater);
        CHECK(updater.status().state == Updater::State::Installed);
        if (updater.status().state != Updater::State::Installed) {
            std::fprintf(stderr, "  %s\n", updater.status().message.c_str());
        }
        for (const File& file : new_files) {
            CHECK(read(app / file.path) == file.text);
        }
        CHECK(!fs::exists(app / "gone.txt"));
        CHECK(read(app / "player.ini") == "the player's own file");
        CHECK(fs::exists(app / "portable.txt") && read(app / "user" / "Game" / "settings.ini") == "the player's settings");
        CHECK(!fs::exists(app / ".update"));
#ifndef _WIN32
        CHECK((fs::status(app / "game").permissions() & fs::perms::owner_exec) != fs::perms::none);
        CHECK((fs::status(app / "same.txt").permissions() & fs::perms::owner_exec) == fs::perms::none);
#endif
    }
    {  // A beta is offered to a beta
        Updater updater(app, "example-launcher", "1.0.0-beta.3", "owner/name");
        updater.check();
        wait(updater);
        CHECK(updater.status().state == Updater::State::Available && updater.status().version == "1.1.0-beta.1");
    }
    {  // The app on macOS: its top is the .app, above Contents/MacOS
        Updater updater(root / "My.app" / "Contents" / "MacOS", "x", "1.0.0", "owner/name");
        CHECK(updater.top() == root / "My.app");
    }
    fs::remove_all(root);
    if (failures == 0) {
        std::printf("update_test: all as it should be\n");
    }
    return failures == 0 ? 0 : 1;
}
