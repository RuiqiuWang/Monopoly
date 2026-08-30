CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -finput-charset=UTF-8 -fexec-charset=UTF-8
CPPFLAGS ?= -Iinclude

.PHONY: all test cli engine tutorial_test command_test mine_test tool_room_test assets_test e2e_test json_test clean

all: test_movement movement_cli tutorial_test command_test mine_test tool_room_test assets_test game_engine

test_movement: movement.c include/movement.h include/player.h tests/test_movement.c
	$(CC) $(CPPFLAGS) $(CFLAGS) movement.c tests/test_movement.c -o $@

movement_cli: movement.c include/movement.h include/player.h tests/movement_cli.c
	$(CC) $(CPPFLAGS) $(CFLAGS) movement.c tests/movement_cli.c -o $@

game_engine: game_engine.c command.c tutorial.c map.c movement.c tui.c mine.c tool_room.c assets.c player.c include/command.h include/map.h include/tui.h include/tutorial.h include/movement.h include/player.h include/block_bit_utils.h include/mine.h include/tool_room.h include/assets.h
	$(CC) $(CPPFLAGS) $(CFLAGS) game_engine.c command.c tutorial.c map.c tui.c movement.c mine.c tool_room.c assets.c player.c -o $@

tutorial_test: tutorial.c tui.c map.c movement.c include/tutorial.h include/tui.h include/map.h include/movement.h include/player.h include/block_bit_utils.h tests/test_tutorial.c
	$(CC) $(CPPFLAGS) $(CFLAGS) tutorial.c tui.c map.c movement.c tests/test_tutorial.c -o $@

command_test: command.c include/command.h tests/test_command.c
	$(CC) $(CPPFLAGS) $(CFLAGS) command.c tests/test_command.c -o $@

mine_test: mine.c map.c player.c include/mine.h include/map.h include/player.h tests/test_mine.c
	$(CC) $(CPPFLAGS) $(CFLAGS) mine.c map.c player.c tests/test_mine.c -o $@

tool_room_test: tool_room.c player.c include/tool_room.h include/player.h tests/test_tool_room.c
	$(CC) $(CPPFLAGS) $(CFLAGS) tool_room.c player.c tests/test_tool_room.c -o $@

assets_test: assets.c map.c player.c include/assets.h include/map.h include/player.h tests/test_assets.c
	$(CC) $(CPPFLAGS) $(CFLAGS) assets.c map.c player.c tests/test_assets.c -o $@

e2e_test: game_engine
	python tests/e2e_game.py

cli: movement_cli
	./movement_cli

engine: game_engine
	./game_engine

json_test:
	$(MAKE) tests/json_runner
	python3 tests/run_json_tests.py

tests/json_runner: tests/json_runner.c movement.c include/movement.h include/player.h
	$(CC) $(CPPFLAGS) $(CFLAGS) movement.c tests/json_engine.c tests/json_runner.c -o $@

test: test_movement tutorial_test command_test mine_test tool_room_test assets_test e2e_test
	./test_movement
	./tutorial_test
	./command_test
	./mine_test
	./tool_room_test
	./assets_test

clean:
	rm -f test_movement test_movement.exe movement_cli movement_cli.exe tutorial_test tutorial_test.exe command_test command_test.exe mine_test mine_test.exe tool_room_test tool_room_test.exe assets_test assets_test.exe game_engine game_engine.exe tests/json_runner tests/json_runner.exe player_assets.json
