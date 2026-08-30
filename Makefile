CC = gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -finput-charset=UTF-8 -fexec-charset=UTF-8
CPPFLAGS ?= -Iinclude

# Python is named differently on common platforms.  Windows installations
# normally provide the Python Launcher (`py -3`) instead of `python3`.
ifeq ($(OS),Windows_NT)
PYTHON ?= py -3
else
PYTHON ?= python3
endif

.PHONY: all test cli engine tutorial_test command_test mine_test tool_room_test assets_test character_select_test map_test item_usage_test item_effect_test gift_house_test help_query_test jail_test jail_item_effect_test property_test e2e_test json_test clean

all: test_movement movement_cli character_select_cli tutorial_test command_test mine_test tool_room_test assets_test character_select_test map_test item_usage_test item_effect_test gift_house_test help_query_test jail_test jail_item_effect_test property_test game_engine

test_movement: movement.c include/movement.h include/player.h tests/test_movement.c
	$(CC) $(CPPFLAGS) $(CFLAGS) movement.c tests/test_movement.c -o $@

movement_cli: movement.c include/movement.h include/player.h tests/movement_cli.c
	$(CC) $(CPPFLAGS) $(CFLAGS) movement.c tests/movement_cli.c -o $@

game_engine: game_engine.c command.c tutorial.c map.c movement.c tui.c mine.c tool_room.c assets.c player.c character_select.c input.c item_usage.c item_effect.c gift_house.c help_query.c jail.c property.c
	$(CC) $(CPPFLAGS) $(CFLAGS) game_engine.c command.c tutorial.c map.c tui.c movement.c mine.c tool_room.c assets.c player.c character_select.c input.c item_usage.c item_effect.c gift_house.c help_query.c jail.c property.c -o $@

tutorial_test: tutorial.c input.c tui.c map.c movement.c include/tutorial.h include/input.h include/tui.h include/map.h include/movement.h include/player.h include/block_bit_utils.h tests/test_tutorial.c
	$(CC) $(CPPFLAGS) $(CFLAGS) tutorial.c input.c tui.c map.c movement.c tests/test_tutorial.c -o $@

command_test: command.c include/command.h tests/test_command.c
	$(CC) $(CPPFLAGS) $(CFLAGS) command.c tests/test_command.c -o $@

mine_test: mine.c map.c player.c include/mine.h include/map.h include/player.h tests/test_mine.c
	$(CC) $(CPPFLAGS) $(CFLAGS) mine.c map.c player.c tests/test_mine.c -o $@

tool_room_test: tool_room.c input.c player.c include/tool_room.h include/input.h include/player.h tests/test_tool_room.c
	$(CC) $(CPPFLAGS) $(CFLAGS) tool_room.c input.c player.c tests/test_tool_room.c -o $@

assets_test: assets.c map.c player.c include/assets.h include/map.h include/player.h tests/test_assets.c
	$(CC) $(CPPFLAGS) $(CFLAGS) assets.c map.c player.c tests/test_assets.c -o $@

character_select_cli: character_select.c input.c character_select_cli.c include/character_select.h include/input.h include/player.h
	$(CC) $(CPPFLAGS) $(CFLAGS) character_select.c input.c character_select_cli.c -o $@

character_select_test: character_select.c input.c tests/test_character_select.c include/character_select.h include/input.h include/player.h
	$(CC) $(CPPFLAGS) $(CFLAGS) character_select.c input.c tests/test_character_select.c -o $@

map_test: map.c tests/test_map.c include/map.h
	$(CC) $(CPPFLAGS) $(CFLAGS) map.c tests/test_map.c -o $@

item_usage_test: item_usage.c map.c tests/test_item_usage.c include/item_usage.h
	$(CC) $(CPPFLAGS) $(CFLAGS) item_usage.c map.c tests/test_item_usage.c -o $@

item_effect_test: item_effect.c movement.c map.c tests/test_item_effect.c include/item_effect.h
	$(CC) $(CPPFLAGS) $(CFLAGS) item_effect.c movement.c map.c tests/test_item_effect.c -o $@

gift_house_test: gift_house.c input.c tests/test_gift_house.c include/gift_house.h
	$(CC) $(CPPFLAGS) $(CFLAGS) gift_house.c input.c tests/test_gift_house.c -o $@

help_query_test: help_query.c tests/test_help_query.c include/help_query.h
	$(CC) $(CPPFLAGS) $(CFLAGS) help_query.c tests/test_help_query.c -o $@

jail_test: jail.c movement.c map.c tests/test_jail.c include/jail.h
	$(CC) $(CPPFLAGS) $(CFLAGS) jail.c movement.c map.c tests/test_jail.c -o $@

jail_item_effect_test: jail.c item_effect.c movement.c map.c tests/test_jail_item_effect_integration.c include/jail.h include/item_effect.h
	$(CC) $(CPPFLAGS) $(CFLAGS) jail.c item_effect.c movement.c map.c tests/test_jail_item_effect_integration.c -o $@

property_test: property.c map.c tests/test_property.c include/property.h
	$(CC) $(CPPFLAGS) $(CFLAGS) property.c map.c tests/test_property.c -o $@

e2e_test: game_engine
	$(PYTHON) tests/e2e_game.py

cli: movement_cli
	./movement_cli

engine: game_engine
	./game_engine

json_test:
	$(MAKE) tests/json_runner
	$(PYTHON) tests/run_json_tests.py

tests/json_runner: tests/json_runner.c movement.c include/movement.h include/player.h
	$(CC) $(CPPFLAGS) $(CFLAGS) movement.c tests/json_engine.c tests/json_runner.c -o $@

test: test_movement tutorial_test command_test mine_test tool_room_test assets_test character_select_test map_test item_usage_test item_effect_test gift_house_test help_query_test jail_test jail_item_effect_test property_test e2e_test
	./test_movement
	./tutorial_test
	./command_test
	./mine_test
	./tool_room_test
	./assets_test
	./character_select_test
	./map_test
	./item_usage_test
	./item_effect_test
	./gift_house_test
	./help_query_test
	./jail_test
	./jail_item_effect_test
	./property_test

clean:
	rm -f test_movement test_movement.exe movement_cli movement_cli.exe character_select_cli character_select_cli.exe tutorial_test tutorial_test.exe command_test command_test.exe mine_test mine_test.exe tool_room_test tool_room_test.exe assets_test assets_test.exe character_select_test character_select_test.exe map_test map_test.exe item_usage_test item_usage_test.exe item_effect_test item_effect_test.exe gift_house_test gift_house_test.exe help_query_test help_query_test.exe jail_test jail_test.exe jail_item_effect_test jail_item_effect_test.exe property_test property_test.exe game_engine game_engine.exe tests/json_runner tests/json_runner.exe player_assets.json
