// SPDX-License-Identifier: GPL-3.0-only
// Part of recomp-common (ForkAde's fork): the country flags the public lobby shows beside a player's name (ForkAde's
// discord-community-lobby). The pictures are flag-icons' (MIT License: flags_atlas.inc), in one texture made the first
// time one is drawn.
#pragma once

#include <string>

#include <SDL3/SDL.h>

namespace launcher {

// The renderer the flags' texture is made with (main.cpp, once, before the first frame)
void flags_use_renderer(SDL_Renderer* renderer);
// A flag, `height` points tall, where the cursor is, as an ImGui item; nothing (and false) for a code it hasn't
bool flag_image(const std::string& code, float height);
// A flag drawn at a place on the window's draw list (no item); false for a code it hasn't
bool flag_draw(const std::string& code, float x, float y, float height);
// The texture freed (main.cpp, before the renderer goes)
void flags_release();

}  // namespace launcher
