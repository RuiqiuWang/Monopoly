CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic

.PHONY: all test cli clean

all: test_movement movement_cli

test_movement: movement.c movement.h player.h test_movement.c
	$(CC) $(CFLAGS) -I. movement.c test_movement.c -o $@

movement_cli: movement.c movement.h player.h movement_cli.c
	$(CC) $(CFLAGS) -I. movement.c movement_cli.c -o $@

cli: movement_cli
	./movement_cli

test: test_movement
	./test_movement

clean:
	rm -f test_movement
