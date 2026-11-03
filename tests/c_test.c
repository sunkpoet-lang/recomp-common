// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of recomp-common: https://github.com/tmcleroy/recomp-common
// The menu from a host written in C (menu_c.h).
#include "menu_c.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(what) \
  do { \
    if(!(what)) { \
      fprintf(stderr, "%s:%d: not so: %s\n", __FILE__, __LINE__, #what); \
      exit(1); \
    } \
  } while(0)

static int values[2];

static int value(void* user, int feature) {
  return ((int*)user)[feature];
}

static void setValue(void* user, int feature, int v) {
  ((int*)user)[feature] = v;
}

int main(void) {
  static const char* const kinds[] = {"Off", "Some", "All", NULL};
  static const RMenuRow rows[] = {
    {RMENU_ROW_SAVE, 0, 0, NULL, NULL, NULL},
    {RMENU_ROW_LOAD, 0, 0, NULL, NULL, NULL},
    {RMENU_ROW_SLOT, 0, 0, NULL, NULL, NULL},
    {RMENU_ROW_PAGES, 1, 0, NULL, NULL, NULL},
    {RMENU_ROW_ACTION, 1, 5, "Reset", "Starts the game again.", NULL},
    {RMENU_ROW_QUIT, 2, 0, NULL, NULL, NULL},
    {RMENU_ROW_RESUME, 2, 0, NULL, NULL, NULL},
  };
  static const RMenuFeature features[] = {
    {"Cheats", "Endless lives", "Lives are not lost.", NULL},
    {"Cheats", "Help", "How much help.", kinds},
  };
  RMenuConfig config = {0};
  config.rows = rows;
  config.rowCount = sizeof(rows) / sizeof(rows[0]);
  config.features = features;
  config.featureCount = 2;
  config.value = value;
  config.setValue = setValue;
  config.user = values;
  config.confirmName = "B or A";
  config.closeName = "Esc or Select";

  RMenu* menu = rmenu_new();
  rmenu_configure(menu, &config);
  rmenu_open(menu);
  CHECK(rmenu_isOpen(menu));
  CHECK(rmenu_buttons(menu, RMENU_CONFIRM, NULL) == RMENU_SAVE);
  rmenu_buttons(menu, RMENU_DOWN, NULL);
  rmenu_buttons(menu, RMENU_DOWN, NULL);
  rmenu_buttons(menu, RMENU_RIGHT, NULL);
  CHECK(rmenu_slot(menu) == 2);
  // the Cheats page
  rmenu_buttons(menu, RMENU_DOWN, NULL);
  rmenu_buttons(menu, RMENU_CONFIRM, NULL);
  rmenu_buttons(menu, RMENU_CONFIRM, NULL);
  CHECK(values[0] == 1);
  rmenu_buttons(menu, RMENU_DOWN, NULL);
  rmenu_buttons(menu, RMENU_RIGHT, NULL);
  rmenu_buttons(menu, RMENU_RIGHT, NULL);
  CHECK(values[1] == 2);
  rmenu_buttons(menu, RMENU_LEFT, NULL);
  CHECK(values[1] == 1);
  rmenu_buttons(menu, RMENU_UP, NULL);
  rmenu_buttons(menu, RMENU_LEFT, NULL);  // (no values to go through: the first page)
  int id = 0;
  rmenu_buttons(menu, RMENU_DOWN, NULL);
  CHECK(rmenu_buttons(menu, RMENU_CONFIRM, &id) == RMENU_ACTION && id == 5);
  rmenu_buttons(menu, RMENU_DOWN, NULL);
  CHECK(rmenu_buttons(menu, RMENU_CONFIRM, NULL) == RMENU_QUIT);

  static uint32_t pixels[256 * 224];
  rmenu_note(menu, "Nothing is kept in slot 2.");
  rmenu_draw(menu, pixels, 256, 224);
  CHECK(pixels[0] != 0);
  rmenu_buttons(menu, RMENU_BACK, NULL);
  CHECK(!rmenu_isOpen(menu));
  rmenu_free(menu);
  puts("c_test: all as it should be");
  return 0;
}
