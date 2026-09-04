#!/usr/bin/env python3
"""Deterministic schema-2.0 state engine used by the JSON test harness."""

from __future__ import annotations

import copy
import json
from pathlib import Path
from typing import Any


MAP_SIZE = 70
PLAYER_IDS = {"A", "Q", "S", "J"}
PARKS = {14, 49, 63}
MINES = {64: 60, 65: 80, 66: 40, 67: 100, 68: 80, 69: 20}
SPECIAL_TYPES = {
    0: "START",
    14: "PARK",
    28: "TOOL_SHOP",
    35: "GIFT_SHOP",
    49: "PARK",
    63: "PARK",
    64: "MINE",
    65: "MINE",
    66: "MINE",
    67: "MINE",
    68: "MINE",
    69: "MINE",
}
BASE_SYMBOLS = {
    "START": "S",
    "PARK": "P",
    "TOOL_SHOP": "T",
    "GIFT_SHOP": "G",
    "MINE": "M",
    "LAND_1": "0",
    "LAND_2": "0",
    "LAND_3": "0",
}


class JsonEngineError(Exception):
    def __init__(
        self,
        code: str,
        *,
        action_index: int | None = None,
        path: str | None = None,
    ) -> None:
        super().__init__(code)
        self.details: dict[str, Any] = {"code": code}
        if action_index is not None:
            self.details["action_index"] = action_index
        if path is not None:
            self.details["path"] = path


def _is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def _property_price(position: int) -> int:
    if 1 <= position <= 13 or 15 <= position <= 27:
        return 200
    if 29 <= position <= 34:
        return 500
    if 36 <= position <= 48 or 50 <= position <= 62:
        return 300
    return 0


def _base_type(position: int) -> str:
    special = SPECIAL_TYPES.get(position)
    if special is not None:
        return special
    if 1 <= position <= 27:
        return "LAND_1"
    if 29 <= position <= 34:
        return "LAND_2"
    return "LAND_3"


class RandomControl:
    def __init__(self, value: Any) -> None:
        self.mode = "PRNG"
        self.streams: dict[str, list[int]] = {}
        self.seeds: dict[str, int] = {}
        if value is None:
            return
        if not isinstance(value, dict):
            raise JsonEngineError("INVALID_PRESET")
        self.mode = value.get("mode", "")
        if self.mode == "SEQUENCE":
            streams = value.get("streams")
            if not isinstance(streams, dict):
                raise JsonEngineError("INVALID_PRESET")
            for name, sequence in streams.items():
                if not isinstance(sequence, list):
                    raise JsonEngineError("INVALID_PRESET")
                self.streams[name] = copy.deepcopy(sequence)
        elif self.mode == "PRNG":
            if value.get("algorithm", "XORSHIFT32") != "XORSHIFT32":
                raise JsonEngineError("INVALID_PRESET")
            seeds = value.get("stream_seeds", {})
            if not isinstance(seeds, dict):
                raise JsonEngineError("INVALID_PRESET")
            for name, seed in seeds.items():
                if not _is_int(seed):
                    raise JsonEngineError("INVALID_PRESET")
                self.seeds[name] = seed & 0xFFFFFFFF
        else:
            raise JsonEngineError("INVALID_PRESET")

    def _raw(self, stream: str, action_index: int) -> int:
        if self.mode == "SEQUENCE":
            sequence = self.streams.get(stream)
            if sequence is None or not sequence:
                raise JsonEngineError("RANDOM_SEQUENCE_EMPTY", action_index=action_index)
            value = sequence.pop(0)
            if not _is_int(value):
                raise JsonEngineError("RANDOM_VALUE_OUT_OF_RANGE", action_index=action_index)
            return value
        state = self.seeds.get(stream, 0x6D2B79F5)
        state ^= (state << 13) & 0xFFFFFFFF
        state ^= state >> 17
        state ^= (state << 5) & 0xFFFFFFFF
        state &= 0xFFFFFFFF
        self.seeds[stream] = state
        return state

    def ranged(self, stream: str, low: int, high: int, action_index: int) -> int:
        value = self._raw(stream, action_index)
        if self.mode == "SEQUENCE":
            if value < low or value > high:
                raise JsonEngineError("RANDOM_VALUE_OUT_OF_RANGE", action_index=action_index)
            return value
        return low + value % (high - low + 1)


class JsonV2Engine:
    def __init__(self, case: dict[str, Any], tests_dir: Path) -> None:
        self.case = case
        self.tests_dir = tests_dir
        self._validate_map(case.get("map_file"))
        preset = self._validate_preset(case.get("preset"))
        self.users: list[str] = copy.deepcopy(preset["users"])
        self.current_user: str = preset["current_user"]
        self.phase: str = preset["phase"]
        self.game_status: str = preset["game_status"]
        self.turn_number: int = preset["turn_number"]
        self.players: list[dict[str, Any]] = copy.deepcopy(preset["players"])
        self.properties: list[dict[str, Any]] = copy.deepcopy(preset["properties"])
        self.map_items: list[dict[str, Any]] = copy.deepcopy(preset["map_items"])
        self.fortune: dict[str, Any] = copy.deepcopy(preset["fortune"])
        self.random = RandomControl(preset.get("random_control"))
        self.pending_prompt: str | None = None
        self.winner: str | None = None
        self._effect_acquired_this_turn = False

    def _validate_map(self, map_file: Any) -> None:
        if not isinstance(map_file, str):
            raise JsonEngineError("INVALID_MAP")
        path = self.tests_dir / map_file
        try:
            value = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, UnicodeDecodeError, json.JSONDecodeError):
            raise JsonEngineError("INVALID_MAP") from None
        blocks = value.get("blocks") if isinstance(value, dict) else None
        if value.get("schema_version") != "2.0" or value.get("size") != MAP_SIZE or not isinstance(blocks, list):
            raise JsonEngineError("INVALID_MAP")
        observed: dict[int, str] = {}
        for block in blocks:
            if not isinstance(block, dict) or not _is_int(block.get("position")) or not isinstance(block.get("type"), str):
                raise JsonEngineError("INVALID_MAP")
            position = block["position"]
            if position in observed:
                raise JsonEngineError("INVALID_MAP")
            observed[position] = block["type"]
        if observed != SPECIAL_TYPES:
            raise JsonEngineError("INVALID_MAP")

    def _validate_preset(self, value: Any) -> dict[str, Any]:
        required = {
            "users", "current_user", "phase", "game_status", "turn_number",
            "players", "properties", "map_items", "fortune",
        }
        if not isinstance(value, dict) or not required <= value.keys() or "dice_sequence" in value:
            raise JsonEngineError("INVALID_PRESET")
        users = value["users"]
        players = value["players"]
        if (
            not isinstance(users, list)
            or not 2 <= len(users) <= 4
            or any(user not in PLAYER_IDS for user in users)
            or len(set(users)) != len(users)
            or not isinstance(players, list)
            or len(players) != len(users)
        ):
            raise JsonEngineError("INVALID_PRESET")
        if value["phase"] != "COMMAND" or value["game_status"] != "RUNNING":
            raise JsonEngineError("INVALID_PRESET")
        if not _is_int(value["turn_number"]) or value["turn_number"] < 1:
            raise JsonEngineError("INVALID_PRESET")

        player_ids: list[str] = []
        live_ids: set[str] = set()
        for player in players:
            if not isinstance(player, dict) or "remaining_rounds" in player:
                raise JsonEngineError("INVALID_PRESET")
            required_player = {
                "id", "fund", "credit", "position", "status", "items",
                "god_of_wealth_rounds",
            }
            if not required_player <= player.keys():
                raise JsonEngineError("INVALID_PRESET")
            player_ids.append(player["id"])
            if player["status"] not in {"NORMAL", "BANKRUPT"}:
                raise JsonEngineError("INVALID_PRESET")
            if player["status"] == "NORMAL":
                live_ids.add(player["id"])
            for field in ("fund", "credit", "position", "god_of_wealth_rounds"):
                if not _is_int(player[field]):
                    raise JsonEngineError("INVALID_PRESET")
            if not 0 <= player["position"] < MAP_SIZE or not 0 <= player["god_of_wealth_rounds"] <= 5:
                raise JsonEngineError("INVALID_PRESET")
            items = player["items"]
            if not isinstance(items, dict) or set(items) - {"BLOCK", "ROBOT"}:
                raise JsonEngineError("INVALID_PRESET")
            if any(not _is_int(count) or count < 0 for count in items.values()):
                raise JsonEngineError("INVALID_PRESET")
            if sum(items.values()) > 10:
                raise JsonEngineError("INVALID_PRESET")
            player["items"].setdefault("BLOCK", 0)
            player["items"].setdefault("ROBOT", 0)
        if player_ids != users or value["current_user"] not in live_ids:
            raise JsonEngineError("INVALID_PRESET")

        properties = value["properties"]
        if not isinstance(properties, list):
            raise JsonEngineError("INVALID_PRESET")
        property_positions: set[int] = set()
        for prop in properties:
            if not isinstance(prop, dict):
                raise JsonEngineError("INVALID_PRESET")
            position = prop.get("position")
            if (
                not _is_int(position)
                or _property_price(position) == 0
                or position in property_positions
                or prop.get("owner") not in live_ids
                or not _is_int(prop.get("level"))
                or not 0 <= prop["level"] <= 3
            ):
                raise JsonEngineError("INVALID_PRESET")
            property_positions.add(position)

        map_items = value["map_items"]
        if not isinstance(map_items, list):
            raise JsonEngineError("INVALID_PRESET")
        item_positions: set[int] = set()
        for item in map_items:
            if not isinstance(item, dict):
                raise JsonEngineError("INVALID_PRESET")
            position = item.get("position")
            if (
                not _is_int(position)
                or not 0 <= position < MAP_SIZE
                or position in item_positions
                or item.get("type") != "BLOCK"
            ):
                raise JsonEngineError("INVALID_PRESET")
            item_positions.add(position)

        fortune = value["fortune"]
        fortune_fields = {
            "position", "spawned_after_turn", "remaining_map_turns",
            "next_spawn_after_turn",
        }
        if not isinstance(fortune, dict) or not fortune_fields <= fortune.keys():
            raise JsonEngineError("INVALID_PRESET")
        position = fortune["position"]
        remaining = fortune["remaining_map_turns"]
        if not _is_int(remaining):
            raise JsonEngineError("INVALID_PRESET")
        if position is None:
            if fortune["spawned_after_turn"] is not None or remaining != 0:
                raise JsonEngineError("INVALID_PRESET")
        else:
            if (
                not _is_int(position)
                or not 0 <= position < MAP_SIZE
                or not _is_int(fortune["spawned_after_turn"])
                or not 1 <= remaining <= 5
                or fortune["next_spawn_after_turn"] is not None
                or position in item_positions
            ):
                raise JsonEngineError("INVALID_PRESET")
        next_spawn = fortune["next_spawn_after_turn"]
        if next_spawn is not None and (not _is_int(next_spawn) or next_spawn < value["turn_number"]):
            raise JsonEngineError("INVALID_PRESET")
        return value

    def player(self, player_id: str | None = None) -> dict[str, Any]:
        wanted = self.current_user if player_id is None else player_id
        return next(player for player in self.players if player["id"] == wanted)

    def property_at(self, position: int) -> dict[str, Any] | None:
        return next((prop for prop in self.properties if prop["position"] == position), None)

    def map_item_at(self, position: int) -> dict[str, Any] | None:
        return next((item for item in self.map_items if item["position"] == position), None)

    def _advance_current_user(self) -> None:
        start = self.users.index(self.current_user)
        for offset in range(1, len(self.users) + 1):
            candidate = self.users[(start + offset) % len(self.users)]
            if self.player(candidate)["status"] != "BANKRUPT":
                self.current_user = candidate
                return

    def _random_respawn_delay(self, action_index: int) -> int:
        return self.random.ranged("FORTUNE_RESPAWN_DELAY", 1, 10, action_index)

    def _spawn_fortune(self, completed_turn: int, action_index: int) -> None:
        while True:
            candidate = self.random.ranged("FORTUNE_POSITION", 0, 69, action_index)
            occupied = any(
                player["status"] != "BANKRUPT" and player["position"] == candidate
                for player in self.players
            )
            if candidate in {28, 35} or occupied or self.map_item_at(candidate) is not None:
                continue
            self.fortune = {
                "position": candidate,
                "spawned_after_turn": completed_turn,
                "remaining_map_turns": 5,
                "next_spawn_after_turn": None,
            }
            return

    def _tick_fortune(self, completed_turn: int, action_index: int) -> None:
        if self.fortune["position"] is not None:
            self.fortune["remaining_map_turns"] -= 1
            if self.fortune["remaining_map_turns"] == 0:
                delay = self._random_respawn_delay(action_index)
                self.fortune = {
                    "position": None,
                    "spawned_after_turn": None,
                    "remaining_map_turns": 0,
                    "next_spawn_after_turn": completed_turn + delay,
                }
            return
        if self.fortune["next_spawn_after_turn"] == completed_turn:
            self._spawn_fortune(completed_turn, action_index)

    def _complete_turn(self, actor: dict[str, Any], action_index: int) -> None:
        if actor["god_of_wealth_rounds"] > 0 and not self._effect_acquired_this_turn:
            actor["god_of_wealth_rounds"] -= 1
        completed_turn = self.turn_number
        self._tick_fortune(completed_turn, action_index)
        self.turn_number += 1
        self._effect_acquired_this_turn = False
        if self.phase != "ENDED":
            self._advance_current_user()

    def _schedule_after_collection(self, action_index: int) -> None:
        delay = self._random_respawn_delay(action_index)
        self.fortune = {
            "position": None,
            "spawned_after_turn": None,
            "remaining_map_turns": 0,
            "next_spawn_after_turn": self.turn_number + delay,
        }

    def _collect_fortune_on_path(
        self,
        actor: dict[str, Any],
        visited: list[int],
        action_index: int,
    ) -> None:
        fortune_position = self.fortune["position"]
        if fortune_position is None or fortune_position not in visited:
            return
        self._schedule_after_collection(action_index)
        actor["god_of_wealth_rounds"] = 5
        self._effect_acquired_this_turn = True

    def _check_winner(self) -> None:
        living = [player for player in self.players if player["status"] != "BANKRUPT"]
        if len(living) == 1:
            self.phase = "ENDED"
            self.game_status = "FINISHED"
            self.winner = living[0]["id"]
            self.current_user = living[0]["id"]
            self.pending_prompt = None

    def _land(self, actor: dict[str, Any], action_index: int) -> None:
        position = actor["position"]
        if position in MINES:
            actor["credit"] = min(1_000_000_000, actor["credit"] + MINES[position])
            self._complete_turn(actor, action_index)
            return
        if position == 28:
            if sum(actor["items"].values()) >= 10 or actor["credit"] < 30:
                self._complete_turn(actor, action_index)
            else:
                self.phase = "PROMPT"
                self.pending_prompt = "TOOL_SHOP"
            return
        if position == 35:
            self.phase = "PROMPT"
            self.pending_prompt = "GIFT_SHOP"
            return

        price = _property_price(position)
        prop = self.property_at(position)
        if price == 0:
            self._complete_turn(actor, action_index)
            return
        if prop is None:
            self.phase = "PROMPT"
            self.pending_prompt = "BUY"
            return
        if prop["owner"] == actor["id"]:
            if prop["level"] < 3:
                self.phase = "PROMPT"
                self.pending_prompt = "UPGRADE"
            else:
                self._complete_turn(actor, action_index)
            return

        owner = self.player(prop["owner"])
        if actor["god_of_wealth_rounds"] == 0:
            rent = price * (prop["level"] + 1) // 2
            actor["fund"] -= rent
            owner["fund"] += rent
            if actor["fund"] < 0:
                actor["fund"] = 0
                actor["status"] = "BANKRUPT"
                self.properties = [owned for owned in self.properties if owned["owner"] != actor["id"]]
                self._check_winner()
        if self.phase != "ENDED":
            self._complete_turn(actor, action_index)

    def _move(self, actor: dict[str, Any], steps: int, action_index: int) -> None:
        effective_steps = steps % MAP_SIZE
        visited: list[int] = []
        for distance in range(1, effective_steps + 1):
            position = (actor["position"] + distance) % MAP_SIZE
            visited.append(position)
            if self.map_item_at(position) is not None:
                break
        if visited:
            actor["position"] = visited[-1]
        hit = self.map_item_at(actor["position"]) if visited else None
        if hit is not None:
            self.map_items.remove(hit)
        self._collect_fortune_on_path(actor, visited, action_index)
        self._land(actor, action_index)

    def _answer(self, actor: dict[str, Any], value: Any, action_index: int) -> None:
        if self.phase != "PROMPT" or self.pending_prompt is None:
            raise JsonEngineError("INVALID_PHASE", action_index=action_index)
        if not isinstance(value, str):
            raise JsonEngineError(
                "INVALID_PARAMS",
                action_index=action_index,
                path=f"actions[{action_index}].params.value",
            )
        prompt = self.pending_prompt
        if prompt in {"BUY", "UPGRADE"}:
            if len(value) != 1 or value.upper() not in {"Y", "N"}:
                raise JsonEngineError(
                    "INVALID_PARAMS",
                    action_index=action_index,
                    path=f"actions[{action_index}].params.value",
                )
            price = _property_price(actor["position"])
            prop = self.property_at(actor["position"])
            if value.upper() == "Y" and actor["fund"] >= price:
                if prompt == "BUY" and prop is None:
                    self.properties.append({"position": actor["position"], "owner": actor["id"], "level": 0})
                    actor["fund"] -= price
                elif prompt == "UPGRADE" and prop is not None and prop["level"] < 3:
                    prop["level"] += 1
                    actor["fund"] -= price
            self.phase = "COMMAND"
            self.pending_prompt = None
            self._complete_turn(actor, action_index)
            return
        if prompt == "TOOL_SHOP":
            if len(value) == 1 and value.upper() == "F":
                self.phase = "COMMAND"
                self.pending_prompt = None
                self._complete_turn(actor, action_index)
                return
            if value not in {"1", "2"}:
                return
            item = "BLOCK" if value == "1" else "ROBOT"
            cost = 50 if value == "1" else 30
            if sum(actor["items"].values()) >= 10 or actor["credit"] < cost:
                return
            actor["credit"] -= cost
            actor["items"][item] += 1
            if sum(actor["items"].values()) >= 10 or actor["credit"] < 30:
                self.phase = "COMMAND"
                self.pending_prompt = None
                self._complete_turn(actor, action_index)
            return
        if prompt == "GIFT_SHOP":
            if value == "1":
                actor["fund"] += 2000
            elif value == "2":
                actor["credit"] += 200
            elif value == "3":
                actor["god_of_wealth_rounds"] = 5
                self._effect_acquired_this_turn = True
            self.phase = "COMMAND"
            self.pending_prompt = None
            self._complete_turn(actor, action_index)

    def apply(self, action: Any, action_index: int) -> None:
        if not isinstance(action, dict) or not isinstance(action.get("command"), str):
            raise JsonEngineError("INVALID_COMMAND", action_index=action_index)
        command = action["command"].upper()
        if self.phase == "ENDED":
            raise JsonEngineError("ACTION_AFTER_END", action_index=action_index)
        if command == "QUIT":
            self.phase = "ENDED"
            self.game_status = "FINISHED"
            self.pending_prompt = None
            self.winner = None
            return
        if command in {"QUERY", "HELP"}:
            return
        if command not in {
            "STEP", "ROLL", "SELL", "BLOCK", "ROBOT", "ANSWER", "ADVANCE_TURN",
        }:
            raise JsonEngineError("INVALID_COMMAND", action_index=action_index)
        actor = self.player()
        if command == "ANSWER":
            params = action.get("params")
            value = params.get("value") if isinstance(params, dict) else None
            self._answer(actor, value, action_index)
            return
        if self.phase != "COMMAND":
            raise JsonEngineError("INVALID_PHASE", action_index=action_index)
        if command == "ADVANCE_TURN":
            self._complete_turn(actor, action_index)
            return
        params = action.get("params")
        if command == "STEP":
            steps = params.get("steps") if isinstance(params, dict) else None
            if not _is_int(steps) or steps < 0:
                raise JsonEngineError(
                    "INVALID_PARAMS",
                    action_index=action_index,
                    path=f"actions[{action_index}].params.steps",
                )
            self._move(actor, steps, action_index)
            return
        if command == "ROLL":
            steps = self.random.ranged("DICE", 1, 6, action_index)
            self._move(actor, steps, action_index)
            return
        if command == "SELL":
            position = params.get("position") if isinstance(params, dict) else None
            prop = self.property_at(position) if _is_int(position) else None
            if prop is None or prop["owner"] != actor["id"]:
                raise JsonEngineError("INVALID_PARAMS", action_index=action_index)
            actor["fund"] += 2 * _property_price(position) * (prop["level"] + 1)
            self.properties.remove(prop)
            return
        if command == "BLOCK":
            offset = params.get("offset") if isinstance(params, dict) else None
            if not _is_int(offset) or not -10 <= offset <= 10:
                raise JsonEngineError(
                    "INVALID_PARAMS",
                    action_index=action_index,
                    path=f"actions[{action_index}].params.offset",
                )
            target = (actor["position"] + offset) % MAP_SIZE
            if (
                actor["items"]["BLOCK"] <= 0
                or self.map_item_at(target) is not None
                or self.fortune["position"] == target
            ):
                raise JsonEngineError("INVALID_PARAMS", action_index=action_index)
            actor["items"]["BLOCK"] -= 1
            self.map_items.append({"position": target, "type": "BLOCK"})
            return
        if command == "ROBOT":
            if actor["items"]["ROBOT"] <= 0:
                raise JsonEngineError("INVALID_PARAMS", action_index=action_index)
            actor["items"]["ROBOT"] -= 1
            cleared = {(actor["position"] + distance) % MAP_SIZE for distance in range(1, 11)}
            self.map_items = [item for item in self.map_items if item["position"] not in cleared]

    def _display_players(self) -> list[dict[str, Any]]:
        positions = sorted({
            player["position"]
            for player in self.players
            if player["status"] != "BANKRUPT"
        })
        result: list[dict[str, Any]] = []
        for position in positions:
            occupants = [
                player["id"]
                for player in self.players
                if player["status"] != "BANKRUPT" and player["position"] == position
            ]
            visible = self.current_user if self.current_user in occupants else next(
                user for user in self.users if user in occupants
            )
            result.append({"position": position, "visible_user": visible})
        return result

    def _display_cells(self) -> list[dict[str, Any]]:
        visible_players = {item["position"]: item["visible_user"] for item in self._display_players()}
        cells: list[dict[str, Any]] = []
        for position in range(MAP_SIZE):
            base_type = _base_type(position)
            base_symbol = BASE_SYMBOLS[base_type]
            if position in visible_players:
                symbol = visible_players[position]
                entity = "PLAYER"
            elif self.fortune["position"] == position:
                symbol = "F"
                entity = "FORTUNE"
            elif self.map_item_at(position) is not None:
                symbol = "#"
                entity = "MAP_ITEM"
            else:
                symbol = base_symbol
                entity = "BASE"
            cells.append({
                "position": position,
                "base_type": base_type,
                "base_symbol": base_symbol,
                "visible_symbol": symbol,
                "visible_entity": entity,
            })
        return cells

    def actual(self) -> dict[str, Any]:
        fortune = copy.deepcopy(self.fortune)
        fortune["symbol"] = "F" if fortune["position"] is not None else None
        return {
            "users": copy.deepcopy(self.users),
            "current_user": self.current_user,
            "phase": self.phase,
            "pending_prompt": self.pending_prompt,
            "game_status": self.game_status,
            "winner": self.winner,
            "turn_number": self.turn_number,
            "players": copy.deepcopy(self.players),
            "properties": sorted(copy.deepcopy(self.properties), key=lambda item: item["position"]),
            "map_items": sorted(copy.deepcopy(self.map_items), key=lambda item: item["position"]),
            "fortune": fortune,
            "display_players": self._display_players(),
            "display_cells": self._display_cells(),
        }


def execute_case(case: dict[str, Any], tests_dir: Path) -> dict[str, Any]:
    if case.get("schema_version") != "2.0":
        raise JsonEngineError("UNSUPPORTED_VERSION")
    engine = JsonV2Engine(case, tests_dir)
    actions = case.get("actions")
    if not isinstance(actions, list):
        raise JsonEngineError("INVALID_PRESET")
    for action_index, action in enumerate(actions):
        engine.apply(action, action_index)
    return engine.actual()
