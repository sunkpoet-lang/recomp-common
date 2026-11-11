// SPDX-License-Identifier: GPL-3.0-only
// Part of recomp-common (ForkAde's fork): the lobby's country flags (flags.h).
#include "flags.h"

#include <cstdint>
#include <cstring>
#include <vector>

#include "imgui.h"

namespace launcher {
namespace {

#include "flags_atlas.inc"
#include "replay_icon.inc"

std::vector<unsigned char> from_base64(const char* text) {
    std::vector<unsigned char> bytes;
    unsigned value = 0;
    int bits = 0;
    for (const char* c = text; *c && *c != '='; c++) {
        const char* digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        const char* found = std::strchr(digits, *c);
        if (!found) {
            continue;
        }
        value = (value << 6) | static_cast<unsigned>(found - digits);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            bytes.push_back(static_cast<unsigned char>((value >> bits) & 0xFF));
        }
    }
    return bytes;
}

SDL_Renderer* renderer_ = nullptr;
SDL_Texture* texture_ = nullptr;
bool tried_ = false;

SDL_Texture* texture() {
    if (texture_ || tried_ || !renderer_) {
        return texture_;
    }
    tried_ = true;
    const std::vector<unsigned char> palette = from_base64(kFlagPalette), runs = from_base64(kFlagRuns);
    const size_t pixels = static_cast<size_t>(kFlagAtlasWidth) * kFlagAtlasHeight;
    if (palette.size() < 768) {
        return nullptr;
    }
    std::vector<unsigned char> rgba(pixels * 4);
    size_t at = 0;
    for (size_t r = 0; r + 1 < runs.size() && at < pixels; r += 2) {
        const unsigned char* colour = palette.data() + runs[r + 1] * 3;
        for (int n = 0; n < runs[r] && at < pixels; n++, at++) {
            rgba[at * 4 + 0] = colour[0];
            rgba[at * 4 + 1] = colour[1];
            rgba[at * 4 + 2] = colour[2];
            rgba[at * 4 + 3] = 255;
        }
    }
    if (at != pixels) {
        return nullptr;
    }
    SDL_Surface* surface = SDL_CreateSurfaceFrom(kFlagAtlasWidth, kFlagAtlasHeight, SDL_PIXELFORMAT_RGBA32, rgba.data(), kFlagAtlasWidth * 4);
    if (surface) {
        texture_ = SDL_CreateTextureFromSurface(renderer_, surface);
        SDL_DestroySurface(surface);
        if (texture_) {
            SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_LINEAR);
        }
    }
    return texture_;
}

// (native-replays) The replay icon's texture, made the first time it is drawn
SDL_Texture* replay_texture_ = nullptr;
bool replay_tried_ = false;

SDL_Texture* replay_texture() {
    if (replay_texture_ || replay_tried_ || !renderer_) {
        return replay_texture_;
    }
    replay_tried_ = true;
    const std::vector<unsigned char> palette = from_base64(kReplayIconPalette), runs = from_base64(kReplayIconRuns);
    const size_t pixels = static_cast<size_t>(kReplayIconSize) * kReplayIconSize;
    std::vector<unsigned char> rgba(pixels * 4);
    size_t at = 0;
    for (size_t r = 0; r + 1 < runs.size() && at < pixels; r += 2) {
        const size_t colour = static_cast<size_t>(runs[r + 1]) * 4;
        if (colour + 4 > palette.size()) {
            return nullptr;
        }
        for (int n = 0; n < runs[r] && at < pixels; n++, at++) {
            std::memcpy(&rgba[at * 4], palette.data() + colour, 4);
        }
    }
    if (at != pixels) {
        return nullptr;
    }
    SDL_Surface* surface = SDL_CreateSurfaceFrom(kReplayIconSize, kReplayIconSize, SDL_PIXELFORMAT_RGBA32, rgba.data(), kReplayIconSize * 4);
    if (surface) {
        replay_texture_ = SDL_CreateTextureFromSurface(renderer_, surface);
        SDL_DestroySurface(surface);
        if (replay_texture_) {
            SDL_SetTextureScaleMode(replay_texture_, SDL_SCALEMODE_LINEAR);
        }
    }
    return replay_texture_;
}

// Where a code's flag is in the atlas; false for one it hasn't
bool place(const std::string& code, ImVec2& uv0, ImVec2& uv1) {
    if (code.size() != 2) {
        return false;
    }
    const size_t count = (sizeof(kFlagCodes) - 1) / 2;
    for (size_t i = 0; i < count; i++) {
        if (kFlagCodes[i * 2] == code[0] && kFlagCodes[i * 2 + 1] == code[1]) {
            const float x = static_cast<float>((i % kFlagColumns) * kFlagWidth), y = static_cast<float>((i / kFlagColumns) * kFlagHeight);
            uv0 = ImVec2(x / kFlagAtlasWidth, y / kFlagAtlasHeight);
            uv1 = ImVec2((x + kFlagWidth) / kFlagAtlasWidth, (y + kFlagHeight) / kFlagAtlasHeight);
            return true;
        }
    }
    return false;
}

}  // namespace

void flags_use_renderer(SDL_Renderer* renderer) {
    renderer_ = renderer;
}

bool flag_image(const std::string& code, float height) {
    ImVec2 uv0, uv1;
    SDL_Texture* t = texture();
    if (!t || !place(code, uv0, uv1)) {
        return false;
    }
    ImGui::Image(ImTextureRef(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(t))), ImVec2(height * kFlagWidth / kFlagHeight, height), uv0, uv1);
    return true;
}

bool flag_draw(const std::string& code, float x, float y, float height) {
    ImVec2 uv0, uv1;
    SDL_Texture* t = texture();
    if (!t || !place(code, uv0, uv1)) {
        return false;
    }
    ImGui::GetWindowDrawList()->AddImage(ImTextureRef(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(t))), ImVec2(x, y),
                                         ImVec2(x + height * kFlagWidth / kFlagHeight, y + height), uv0, uv1);
    return true;
}

bool replay_icon_draw(float x, float y, float size, bool dim) {
    SDL_Texture* t = replay_texture();
    if (!t) {
        return false;
    }
    ImGui::GetWindowDrawList()->AddImage(ImTextureRef(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(t))), ImVec2(x, y),
                                         ImVec2(x + size, y + size), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, dim ? 80 : 255));
    return true;
}

void flags_release() {
    if (replay_texture_) {
        SDL_DestroyTexture(replay_texture_);
        replay_texture_ = nullptr;
    }
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    tried_ = false;
}

}  // namespace launcher
