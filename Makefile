CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Iinclude

.PHONY: all test cli engine json_test clean

all: test_movement movement_cli game_engine

test_movement: movement.c include/movement.h include/player.h tests/test_movement.c
	$(CC) $(CPPFLAGS) $(CFLAGS) movement.c tests/test_movement.c -o $@

movement_cli: movement.c include/movement.h include/player.h tests/movement_cli.c
	$(CC) $(CPPFLAGS) $(CFLAGS) movement.c tests/movement_cli.c -o $@

game_engine: game_engine.c map.c include/map.h include/tui.h include/movement.h include/player.h include/block_bit_utils.h
	$(CC) $(CPPFLAGS) $(CFLAGS) game_engine.c map.c tui.c movement.c -o $@

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
	rm -f test_movement movement_cli game_engine tests/json_runner
