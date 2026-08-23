CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic

.PHONY: all test cli engine clean

all: test_movement movement_cli game_engine

test_movement: movement.c movement.h player.h test_movement.c
	$(CC) $(CFLAGS) -I. movement.c test_movement.c -o $@

movement_cli: movement.c movement.h player.h movement_cli.c
	$(CC) $(CFLAGS) -I. movement.c movement_cli.c -o $@

game_engine: game_engine.c map.c map.h tui.c tui.h movement.c movement.h player.h block_bit_utils.h
	$(CC) $(CFLAGS) -I. game_engine.c map.c tui.c movement.c -o $@

cli: movement_cli
	./movement_cli

engine: game_engine
	./game_engine

test: test_movement
	./test_movement

clean:
	rm -f test_movement movement_cli game_engine
