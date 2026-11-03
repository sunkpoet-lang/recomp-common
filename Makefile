# recomp-common has no build of its own: each project compiles its copy of src/ and launcher/. This tests it
# (make test), builds the example host (make example) and the example launcher (make launcher), and checks the
# files before a push (make audit).
CXX ?= c++
CC ?= cc
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc
B := build

.PHONY: test example launcher launcher-shots audit clean
test: $(B)/menu_test $(B)/menubar_test $(B)/c_test $(B)/settings_test $(B)/settings_c_test $(B)/folders_test $(B)/update_test
	@$(B)/menu_test
	@$(B)/menubar_test
	@$(B)/c_test
	@$(B)/settings_test
	@cd $(B) && ./settings_c_test > /dev/null && echo "settings_c_test: all as it should be"
	@$(B)/folders_test
	@$(B)/update_test

$(B)/menu_test: tests/menu_test.cpp src/menu.cpp src/menu.h src/text.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined tests/menu_test.cpp src/menu.cpp -o $@

# The menu bar and the right-click menu (menubar.h)
$(B)/menubar_test: tests/menubar_test.cpp src/menubar.cpp src/menubar.h src/menu.cpp src/menu.h src/text.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined tests/menubar_test.cpp src/menubar.cpp src/menu.cpp -o $@

# (the C interface, from a host compiled as C)
$(B)/c_test: tests/c_test.c src/menu.cpp src/menu_c.cpp src/menu_c.h src/menu.h src/text.h
	@mkdir -p $(B)
	$(CC) -std=c99 -Wall -Wextra -Wpedantic -Isrc -c tests/c_test.c -o $(B)/c_test.o
	$(CXX) $(CXXFLAGS) $(B)/c_test.o src/menu.cpp src/menu_c.cpp -o $@

# The settings rules (settings.h), and from C
$(B)/settings_test: tests/settings_test.cpp src/settings.cpp src/settings.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined tests/settings_test.cpp src/settings.cpp -o $@

$(B)/settings_c_test: tests/settings_c_test.c src/settings.cpp src/settings_c.cpp src/settings_c.h src/settings.h
	@mkdir -p $(B)
	$(CC) -std=c99 -Wall -Wextra -Wpedantic -Isrc -c tests/settings_c_test.c -o $(B)/settings_c_test.o
	$(CXX) $(CXXFLAGS) $(B)/settings_c_test.o src/settings.cpp src/settings_c.cpp -o $@

# Where the player's things are, and portable mode (folders.h)
$(B)/folders_test: tests/folders_test.cpp src/folders.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined tests/folders_test.cpp -o $@

# The launcher's updates (launcher/src/update.h; needs curl and tar, or ditto, to run)
$(B)/update_test: tests/update_test.cpp launcher/src/update.cpp launcher/src/update.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) -Ilauncher/src -fsanitize=address,undefined tests/update_test.cpp launcher/src/update.cpp -o $@

# A host with no game: build/shot OUT.ppm [KEYS] [WIDTHxHEIGHT] (examples/shot.cpp)
example: $(B)/shot
$(B)/shot: examples/shot.cpp src/menu.cpp src/menu.h src/text.h
	@mkdir -p $(B)
	$(CXX) $(CXXFLAGS) examples/shot.cpp src/menu.cpp -o $@

# The launcher with a stand-in for the game (launcher/example; needs cmake, and fetches Dear ImGui, the Inter
# fonts and, where the system has none, SDL 3)
launcher:
	@cmake -S launcher/example -B $(B)/launcher > /dev/null
	@cmake --build $(B)/launcher
# Its pages as pictures, $(B)/launcher-shots/page0.png.., with no window and no display needed
launcher-shots: launcher
	@mkdir -p $(B)/launcher-shots/home
	@SDL_VIDEO_DRIVER=dummy SDL_RENDER_DRIVER=software EXAMPLE_HOME=$(B)/launcher-shots/home \
	  $(B)/launcher/example-launcher --screenshot $(B)/launcher-shots/page
	@ls $(B)/launcher-shots/page0.png $(B)/launcher-shots/page1.png $(B)/launcher-shots/page2.png

audit:
	@tools/audit.sh

clean:
	rm -rf $(B)
