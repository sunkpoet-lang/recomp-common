// SPDX-License-Identifier: GPL-3.0-only
// (ForkAde's fork, replay-viewer) The viewer against a made-up board whose state is a chain of hashes, one per frame:
// a state put back wrong, a frame run twice or skipped, shows as a hash that is not the frame's.
#include "../src/replay_viewer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace recomp::replay;

namespace {
    int failures = 0;
    void Check(bool ok, const char *what, long a = 0, long b = 0) {
        if (!ok) {
            failures++;
            fprintf(stderr, "FAILED: %s (%ld, %ld)\n", what, a, b);
        }
    }

    uint64_t Mix(uint64_t h, uint32_t f) { return (h ^ (f * 0x9E3779B97F4A7C15ull)) * 0xBF58476D1CE4E5B9ull + 1; }

    struct FakeBoard : Board {
        uint32_t frame = 0, length;
        uint64_t hash = 1;
        std::vector<uint64_t> truth; // (the hash after each frame)
        uint32_t ran = 0, seen = 0, keeps = 0, lasting = 0;
        uint32_t lastSeen = 0;
        explicit FakeBoard(uint32_t n) : length(n) {
            truth.push_back(1);
            for (uint32_t f = 1; f <= n; f++) truth.push_back(Mix(truth.back(), f));
        }
        uint32_t Frame() const override { return frame; }
        uint32_t Length() const override { return length; }
        void RunFrame(bool s, bool) override {
            frame++;
            hash = Mix(hash, frame);
            ran++;
            if (s) {
                seen++;
                lastSeen = frame;
                Check(hash == truth[frame], "a frame shown is the match's", frame, 0);
            }
        }
        void Keep(std::vector<uint8_t> &state, bool l) override {
            state.resize(12);
            memcpy(state.data(), &frame, 4);
            memcpy(state.data() + 4, &hash, 8);
            keeps++;
            lasting += l;
        }
        void PutBack(const std::vector<uint8_t> &state, uint32_t f) override {
            uint32_t at;
            memcpy(&at, state.data(), 4);
            memcpy(&hash, state.data() + 4, 8);
            Check(at == f, "a state put back is the frame said", at, f);
            frame = at;
        }
        void GoBack(uint32_t f) override {
            // (keyframes every 300 frames)
            frame = f / 300 * 300;
            hash = truth[frame];
        }
    };

    void Ticks(Viewer &v, FakeBoard &b, int n, uint64_t &tick) {
        for (int i = 0; i < n; i++) v.Tick(b, tick++);
    }
} // namespace

int main() {
    uint64_t tick = 0;
    {
        // x1, then x2, x3, x4, x8
        FakeBoard b(3000);
        Viewer v;
        v.Begin(b.length);
        Ticks(v, b, 10, tick);
        Check(b.lastSeen == 10, "x1: a frame a tick", b.lastSeen, 10);
        v.Do(Command::Faster, tick);
        Ticks(v, b, 5, tick);
        Check(b.lastSeen == 20, "x2: two frames a tick", b.lastSeen, 20);
        v.Do(Command::Faster, tick);
        v.Do(Command::Faster, tick);
        Check(v.CurrentSpeed() == Speed::Four, "faster twice: x4");
        Ticks(v, b, 5, tick);
        Check(b.lastSeen == 40, "x4", b.lastSeen, 40);
        v.Do(Command::Faster, tick);
        v.Do(Command::Faster, tick);
        Check(v.CurrentSpeed() == Speed::Eight, "x8 is the fastest");
        Ticks(v, b, 5, tick);
        Check(b.lastSeen == 80, "x8", b.lastSeen, 80);
        Check(!v.Heard(), "no sound fast-forwarding");
    }
    {
        // Rewind seen frame by frame across windows and keyframes, then play on
        FakeBoard b(3000);
        Viewer v;
        v.Begin(b.length);
        Ticks(v, b, 700, tick);
        v.Do(Command::Rewind, tick);
        Check(v.CurrentSpeed() == Speed::Back1, "rewind: -x1");
        for (int i = 0; i < 150; i++) {
            v.Tick(b, tick++);
            Check(b.lastSeen == 700u - 1 - i, "rewind -x1: the frame before each time", b.lastSeen, 700 - 1 - i);
        }
        v.Do(Command::Play, tick);
        v.Do(Command::Normal, tick);
        Ticks(v, b, 10, tick);
        Check(b.lastSeen == 560, "playing on after rewinding", b.lastSeen, 560);
        // -x4 over a keyframe (300)
        v.Do(Command::Rewind, tick);
        v.Do(Command::Rewind, tick);
        v.Do(Command::Rewind, tick);
        v.Do(Command::Rewind, tick);
        Check(v.CurrentSpeed() == Speed::Back4, "rewind four times: -x4", static_cast<int>(v.CurrentSpeed()));
        Ticks(v, b, 80, tick);
        Check(b.lastSeen == 240, "-x4 for 80 ticks", b.lastSeen, 240);
        // (rewinding cost: about two frames run for each shown, once the viewer's marks are there)
        const uint32_t ranBefore = b.ran;
        v.Do(Command::Normal, tick);
        v.Do(Command::Rewind, tick);
        Ticks(v, b, 60, tick);
        Check(b.ran - ranBefore < 60 * 3, "-x1 runs under three frames a frame shown", b.ran - ranBefore, 180);
        // to the start: it stops there
        Ticks(v, b, 400, tick);
        Check(b.lastSeen == 1 && v.Paused(), "rewinding stops at the first frame", b.lastSeen, 1);
    }
    {
        // A frame at a time, both ways
        FakeBoard b(3000);
        Viewer v;
        v.Begin(b.length);
        Ticks(v, b, 100, tick);
        v.Do(Command::StepBack, tick);
        v.Tick(b, tick++);
        Check(b.lastSeen == 99 && v.Paused(), "a frame back pauses", b.lastSeen, 99);
        v.Do(Command::StepBack, tick);
        v.Do(Command::StepBack, tick);
        Ticks(v, b, 5, tick);
        Check(b.lastSeen == 97, "two frames back", b.lastSeen, 97);
        v.Do(Command::StepForward, tick);
        v.Do(Command::StepForward, tick);
        v.Do(Command::StepForward, tick);
        Ticks(v, b, 5, tick);
        Check(b.lastSeen == 100, "three frames forward", b.lastSeen, 100);
        Ticks(v, b, 30, tick);
        Check(b.lastSeen == 100, "paused: nothing more", b.lastSeen, 100);
    }
    {
        // Seeking, 5-second jumps, the end
        FakeBoard b(3000);
        Viewer v;
        v.Begin(b.length);
        Ticks(v, b, 10, tick);
        v.SeekTo(2000, tick);
        Ticks(v, b, 40, tick);
        Check(b.lastSeen >= 2000 && b.lastSeen < 2040, "seek to 2000 within a few ticks", b.lastSeen, 2000);
        v.Do(Command::Pause, tick);
        const uint32_t at = b.lastSeen;
        v.Do(Command::JumpBack, tick);
        Ticks(v, b, 10, tick);
        Check(b.lastSeen == at - 300, "5 seconds back", b.lastSeen, at - 300);
        v.Do(Command::ToEnd, tick);
        Ticks(v, b, 40, tick);
        Check(b.lastSeen == 3000, "to the end", b.lastSeen, 3000);
        v.Do(Command::Play, tick);
        Ticks(v, b, 5, tick);
        Check(b.frame == 3000 && v.Paused(), "nothing past the end");
    }
    {
        // Slow motion
        FakeBoard b(3000);
        Viewer v;
        v.Begin(b.length);
        v.Do(Command::Slower, tick);
        Check(v.CurrentSpeed() == Speed::ThreeQuarters, "slower: x0.75");
        Ticks(v, b, 40, tick);
        Check(b.lastSeen == 30, "x0.75: 3 frames in 4 ticks", b.lastSeen, 30);
        v.Do(Command::Slower, tick);
        v.Do(Command::Slower, tick);
        Check(v.CurrentSpeed() == Speed::Quarter, "x0.25");
        const uint32_t from = b.lastSeen;
        Ticks(v, b, 40, tick);
        Check(b.lastSeen == from + 10, "x0.25: a frame in 4 ticks", b.lastSeen, from + 10);
        Check(v.Heard(), "sound in slow motion");
    }
    {
        // The A-B loop, forward and backwards
        FakeBoard b(3000);
        Viewer v;
        v.Begin(b.length);
        Ticks(v, b, 50, tick);
        v.Do(Command::MarkA, tick);
        Ticks(v, b, 20, tick);
        v.Do(Command::MarkB, tick);
        Check(v.A() == 50 && v.B() == 70, "A and B", v.A(), v.B());
        Ticks(v, b, 100, tick);
        Check(b.lastSeen >= 50 && b.lastSeen <= 70, "the loop keeps within A-B", b.lastSeen, 0);
        v.Do(Command::ClearLoop, tick);
        Ticks(v, b, 100, tick);
        Check(b.lastSeen > 70, "cleared: it goes on", b.lastSeen, 70);
    }
    {
        // Markers
        FakeBoard b(3000);
        Viewer v;
        v.Begin(b.length);
        v.AddMarker(400);
        v.AddMarker(1200);
        Ticks(v, b, 10, tick);
        v.Do(Command::NextMarker, tick);
        Ticks(v, b, 20, tick);
        v.Do(Command::Pause, tick);
        Check(b.lastSeen >= 400 && b.lastSeen < 430, "next marker", b.lastSeen, 400);
        v.Do(Command::NextMarker, tick);
        Ticks(v, b, 30, tick);
        Check(b.lastSeen == 1200, "the next one", b.lastSeen, 1200);
        v.Do(Command::PreviousMarker, tick);
        Ticks(v, b, 30, tick);
        Check(b.lastSeen == 400, "previous marker", b.lastSeen, 400);
    }
    {
        // The HUD: clicks on the timeline and on the buttons
        Viewer v;
        v.Begin(1000);
        v.Aids().push_back({"hit_boxes", "HITBOX", false});
        std::vector<uint32_t> pixels(384 * 224);
        recomp::text::Canvas canvas{pixels.data(), 384, 224};
        v.Touched(0);
        v.DrawHud(canvas, 10, 1000, 0);
        const float barY = 224 - v.HudHeight() + 4;
        Viewer::Hit hit = v.HitAt(6 + (384 - 12) * 0.5f, barY, 384, 224, 1000);
        Check(hit.kind == Viewer::Hit::Seek && hit.frame >= 495 && hit.frame <= 505, "the timeline's middle", hit.frame, 500);
        Check(v.HitAt(100, 50, 384, 224, 1000).kind == Viewer::Hit::None, "above the strip: the game's");
        int aids = 0, buttons = 0;
        for (float x = 0; x < 384; x += 1) {
            const Viewer::Hit h = v.HitAt(x, 224 - 8, 384, 224, 1000);
            aids += h.kind == Viewer::Hit::Aid;
            buttons += h.kind == Viewer::Hit::Do;
        }
        Check(aids > 0 && buttons > 0, "buttons and the aid on the strip", buttons, aids);
    }
    {
        // The HUD shown or hidden by hand: hidden stays hidden whatever is pressed, paused or not
        Viewer v;
        v.Begin(1000);
        v.Touched(10);
        Check(v.HudShown(10), "shown on input");
        v.Do(Command::ToggleHud, 11);
        Check(!v.HudShown(11), "hidden by hand");
        v.Do(Command::Pause, 12);
        v.Touched(13);
        Check(!v.HudShown(13), "hidden stays hidden, paused and touched");
        v.Do(Command::ToggleHud, 14);
        Check(v.HudShown(10000), "shown again, and kept");
    }
    if (failures) {
        fprintf(stderr, "replay_viewer_test: %d failed\n", failures);
        return 1;
    }
    printf("replay_viewer_test: all as it should be\n");
    return 0;
}
