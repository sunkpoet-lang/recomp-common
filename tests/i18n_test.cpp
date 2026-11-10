// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of recomp-common: https://github.com/tmcleroy/recomp-common
// The words in the player's language (i18n.h) and the font's accented letters (text.h).
#include "i18n.h"
#include "text.h"

#include <cstdio>
#include <cstring>

static int failures = 0;
#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        if (!(x)) {                                                                                                    \
            std::fprintf(stderr, "i18n_test: %s:%d: %s\n", __FILE__, __LINE__, #x);                                  \
            failures++;                                                                                                \
        }                                                                                                              \
    } while (0)

int main() {
    using namespace recomp::i18n;
    // (the language as the system writes it)
    CHECK(Known("pt_BR.UTF-8") == "pt-BR");
    CHECK(Known("pt-PT") == "pt-BR");
    CHECK(Known("es-MX") == "es");
    CHECK(Known("en_US") == "en");
    CHECK(Known("fr") == "");
    CHECK(Resolve("", {"fr-FR", "es-AR", "en"}) == "es");
    CHECK(Resolve("auto", {"de"}) == "en");
    CHECK(Resolve("en", {"pt-BR"}) == "en");
    CHECK(Resolve("pt-BR", {}) == "pt-BR");

    // (English: as written)
    SetLanguage("en");
    CHECK(!std::strcmp(Tr("Resume"), "Resume"));
    // (recomp-common's table, a port's (before it), and a text no table has)
    SetLanguage("pt-BR");
    CHECK(std::strcmp(Tr("Resume"), "Resume") != 0);
    static const Entry mine[] = {{"Resume", "Seguir"}, {"Only mine", "Só meu"}};
    Add("pt-BR", mine);
    CHECK(!std::strcmp(Tr("Resume"), "Seguir"));
    CHECK(!std::strcmp(Tr("Only mine"), "Só meu"));
    CHECK(!std::strcmp(Tr("Nobody has this"), "Nobody has this"));
    SetLanguage("es");
    CHECK(!std::strcmp(Tr("Only mine"), "Only mine"));

    // (patterns: in turn, and by place)
    CHECK(Format("{} of {}", {"3", "9"}) == "3 of 9");
    CHECK(Format("{2} de {1}", {"3", "9"}) == "9 de 3");
    CHECK(Format("no {braces} here {}", {"x"}) == "no {braces} here x");

    // (the font: a character of several bytes is one character wide; an accented letter is its letter and a mark)
    using namespace recomp::text;
    CHECK(TextWidth("ação", 1) == 4 * kCharWidth);
    CHECK(TextLines("É só você", 1, 4 * kCharWidth) == 2); // ("É só", "você": four characters a line)
    const Glyph a = GlyphOf(0xE1), plain = GlyphOf('a'); // á
    CHECK(a.columns[3] != plain.columns[3]);
    const Glyph capital = GlyphOf(0xC9); // É: its mark above the cell
    CHECK(capital.over[3] != 0 && capital.columns[0] == GlyphOf('E').columns[0]);
    CHECK(GlyphOf(0x4E2D).columns[2] == GlyphOf('?').columns[2]); // (none: '?')

    if (failures) {
        return 1;
    }
    std::puts("i18n_test: all as it should be");
    return 0;
}
