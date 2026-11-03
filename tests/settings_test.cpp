// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of recomp-common: https://github.com/tmcleroy/recomp-common
// The settings rules a launcher and its game share: presets, values, choices, exclusive features, and that
// a file keeps everything nobody asked to change.
#include "settings.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

using namespace recomp;

#define CHECK(what)                                                    \
    do {                                                               \
        if (!(what)) {                                                 \
            fprintf(stderr, "%s:%d: not so: %s\n", __FILE__, __LINE__, #what); \
            exit(1);                                                   \
        }                                                              \
    } while (0)

namespace {

    std::vector<FeatureInfo> Features() {
        return {
            {"free_play", "Game", "Free play", "Start works without a coin.", kFeatureEnhanced, {}},
            {"easy", "Difficulty", "Easy", "", kFeatureExclusive, {}},
            {"hard", "Difficulty", "Hard", "", kFeatureExclusive, {}},
            {"health", "Cheats", "Health", "Whose is kept full.", 0, {"Off", "Player 1", "Both"}},
            {"training", "Training", "Training", "This run only.", kFeatureSession, {}},
            {"wide", "Picture", "Wide", "", kFeatureEnhanced | kFeatureExclusive, {}},
        };
    }

    std::filesystem::path Write(const char *name, const std::string &text) {
        const std::filesystem::path path = std::filesystem::temp_directory_path() / name;
        std::ofstream(path, std::ios::binary) << text;
        return path;
    }

    std::string Read(const std::filesystem::path &path) {
        std::ifstream file(path, std::ios::binary);
        std::stringstream text;
        text << file.rdbuf();
        return text.str();
    }

    Ini Loaded(const std::string &text) {
        Ini ini(Write("recomp_common_settings_test.ini", text));
        ini.load();
        return ini;
    }

} // namespace

int main() {
    const std::vector<FeatureInfo> features = Features();

    // The list as text and back; session features are not in it; choices are a sixth field
    const std::string text = FeatureListText(features);
    CHECK(text.find("training") == std::string::npos);
    CHECK(text.find("health\tCheats\t0\tHealth\tWhose is kept full.\tOff|Player 1|Both\n") != std::string::npos);
    CHECK(text.find("free_play\tGame\t2\tFree play\tStart works without a coin.\n") != std::string::npos);
    const std::vector<FeatureInfo> parsed = ParseFeatureList(text);
    CHECK(parsed.size() == 5);
    CHECK(parsed[3].name == "health" && parsed[3].choices.size() == 3 && parsed[3].choices[2] == "Both");
    CHECK(parsed[0].choices.empty() && parsed[0].flags == kFeatureEnhanced);
    // (a list from a game that knows nothing of choices, and lines that are not features)
    CHECK(ParseFeatureList("a\tB\t4\tA\tAn a.\r\nnot a feature\n\n").size() == 1);

    // No file: pure, everything off
    {
        Ini ini("/nonexistent/recomp-common/settings.ini");
        ini.load();
        CHECK(Preset(ini) == "pure");
        for (int value : FeatureValues(ini, features)) {
            CHECK(value == 0);
        }
    }
    // Enhanced: the enhanced ones; a value in the file wins; a choice by number; a choice it has not is off
    {
        Ini ini = Loaded("[features]\npreset = enhanced\nhealth = 2\neasy=on\n");
        const std::vector<int> v = FeatureValues(ini, features);
        CHECK(v[0] == 1 && v[1] == 1 && v[2] == 0 && v[3] == 2 && v[4] == 0 && v[5] == 1);
        ini = Loaded("[features]\npreset=enhanced\nfree_play=off\nhealth=7\ntraining=1\nwide=no\n");
        const std::vector<int> w = FeatureValues(ini, features);
        CHECK(w[0] == 0 && w[3] == 0 && w[4] == 0 && w[5] == 0);
        // (on or off features are never more than 1; of two exclusive ones on, the first is)
        ini = Loaded("[features]\npreset=custom\nfree_play=5\neasy=1\nhard=1\n");
        const std::vector<int> x = FeatureValues(ini, features);
        CHECK(x[0] == 1 && x[1] == 1 && x[2] == 0);
    }
    // Changing one from a preset: the others are written as they were, a choice keeps its number, the
    // preset becomes custom, and the rest of the file is as it was
    {
        const std::filesystem::path path =
            Write("recomp_common_settings_test.ini", "; mine\n[video]\nfullscreen=1\n\n[features]\npreset=enhanced\nhealth = 2\n\n[keyboard1]\nup = W\n");
        Ini ini(path);
        ini.load();
        SetFeature(ini, features, 2, 1);
        CHECK(ini.save());
        const std::string after = Read(path);
        CHECK(after.find("; mine\n[video]\nfullscreen=1\n") == 0);
        CHECK(after.find("[keyboard1]\nup = W\n") != std::string::npos);
        CHECK(after.find("health = 2\n") != std::string::npos); // (not rewritten, not made 1)
        CHECK(after.find("training") == std::string::npos);
        Ini again(path);
        again.load();
        CHECK(Preset(again) == "custom");
        const std::vector<int> v = FeatureValues(again, features);
        CHECK(v[0] == 1 && v[1] == 0 && v[2] == 1 && v[3] == 2 && v[5] == 1);
        // An exclusive one on: its category's other goes off, another category's stays
        SetFeature(again, features, 1, 1);
        const std::vector<int> w = FeatureValues(again, features);
        CHECK(w[1] == 1 && w[2] == 0 && w[5] == 1);
        // A choice
        SetFeature(again, features, 3, 1);
        CHECK(FeatureValues(again, features)[3] == 1);
        CHECK(again.get("features", "health") == "1");
        // A session feature is never written
        SetFeature(again, features, 4, 1);
        CHECK(again.get("features", "training").empty());
        // Back to a preset: the features follow it again
        SetPreset(again, features, "pure");
        for (int value : FeatureValues(again, features)) {
            CHECK(value == 0);
        }
        SetPreset(again, features, "enhanced");
        CHECK(FeatureValues(again, features)[0] == 1 && FeatureValues(again, features)[3] == 0);
        CHECK(again.get("video", "fullscreen") == "1" && again.get("keyboard1", "up") == "W");
    }
    // A file with no section yet
    {
        Ini ini = Loaded("");
        SetFeature(ini, features, 0, 1);
        CHECK(Preset(ini) == "custom" && FeatureValues(ini, features)[0] == 1);
    }
    // The choices a kFeatureMenuChoices feature's menu offers: all until narrowed, off always, the rest as [menu] says
    {
        const FeatureInfo shader = {"shader", "Display", "Shader", "", kFeatureMenuChoices, {"Off", "CRT", "Scanlines", "xBRZ"}};
        Ini ini = Loaded("[features]\nshader=2\n");
        CHECK((MenuChoices(ini, shader) == std::vector<bool>{true, true, true, true}));
        SetMenuChoices(ini, shader, {false, true, false, true});
        CHECK(ini.get("menu", "shader") == "1,3" && ini.get("features", "shader") == "2");
        CHECK((MenuChoices(ini, shader) == std::vector<bool>{true, true, false, true}));
        SetMenuChoices(ini, shader, {true, false, false, false});
        CHECK(ini.get("menu", "shader") == "0" && (MenuChoices(ini, shader) == std::vector<bool>{true, false, false, false}));
        SetMenuChoices(ini, shader, {true, true, true, true});
        CHECK(ini.get("menu", "shader").empty() && (MenuChoices(ini, shader) == std::vector<bool>{true, true, true, true}));
        // (numbers it has not, and a feature that is not one: every choice)
        CHECK((MenuChoices(Loaded("[menu]\nshader=9,x,2\n"), shader) == std::vector<bool>{true, false, true, false}));
        CHECK((MenuChoices(Loaded("[menu]\nhealth=1\n"), features[3]) == std::vector<bool>{true, true, true}));
    }
    std::error_code error;
    std::filesystem::remove(std::filesystem::temp_directory_path() / "recomp_common_settings_test.ini", error);
    printf("settings: ok\n");
    return 0;
}
