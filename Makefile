CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Iinclude

.PHONY: all test cli engine tutorial_test command_test json_test clean

all: test_movement movement_cli tutorial_test command_test game_engine

test_movement: movement.c include/movement.h include/player.h tests/test_movement.c
	$(CC) $(CPPFLAGS) $(CFLAGS) movement.c tests/test_movement.c -o $@

movement_cli: movement.c include/movement.h include/player.h tests/movement_cli.c
	$(CC) $(CPPFLAGS) $(CFLAGS) movement.c tests/movement_cli.c -o $@

game_engine: game_engine.c command.c tutorial.c map.c include/command.h include/map.h include/tui.h include/tutorial.h include/movement.h include/player.h include/block_bit_utils.h
	$(CC) $(CPPFLAGS) $(CFLAGS) game_engine.c command.c tutorial.c map.c tui.c movement.c -o $@

tutorial_test: tutorial.c tui.c map.c movement.c include/tutorial.h include/tui.h include/map.h include/movement.h include/player.h include/block_bit_utils.h tests/test_tutorial.c
	$(CC) $(CPPFLAGS) $(CFLAGS) tutorial.c tui.c map.c movement.c tests/test_tutorial.c -o $@

command_test: command.c include/command.h tests/test_command.c
	$(CC) $(CPPFLAGS) $(CFLAGS) command.c tests/test_command.c -o $@

cli: movement_cli
	./movement_cli

engine: game_engine
	./game_engine

json_test:
	$(MAKE) tests/json_runner
	python3 tests/run_json_tests.py

tests/json_runner: tests/json_runner.c movement.c include/movement.h include/player.h
	$(CC) $(CPPFLAGS) $(CFLAGS) movement.c tests/json_engine.c tests/json_runner.c -o $@

test: test_movement
	./test_movement

clean:
	rm -f test_movement test_movement.exe movement_cli movement_cli.exe tutorial_test tutorial_test.exe command_test command_test.exe game_engine game_engine.exe tests/json_runner tests/json_runner.exe
