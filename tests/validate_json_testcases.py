#!/usr/bin/env python3
"""Validate the schema-v2 JSON testcase corpus without executing the game."""

from __future__ import annotations

import json
import sys
from collections import Counter
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
INPUT_DIR = ROOT / "tests" / "input"
EXPECTED_TOTAL = 400
REQUIRED_BUNDLE_KEYS = {"schema_version", "suite", "map_file", "tests"}
REQUIRED_CASE_KEYS = {"case_id", "case_name", "map_file", "preset", "actions", "expected"}
REQUIRED_PRESET_KEYS = {
    "users",
    "current_user",
    "phase",
    "game_status",
    "turn_number",
    "players",
    "properties",
    "map_items",
    "fortune",
}
REMOVED_PRESET_FIELDS = {"dice_sequence"}
REMOVED_PLAYER_FIELDS = {"remaining_rounds"}
REMOVED_ITEMS = {"BOMB"}


def add_error(errors: list[str], location: str, message: str) -> None:
    errors.append(f"{location}: {message}")


def is_invalid_preset_case(case: dict[str, Any]) -> bool:
    return (
        case.get("expected_outcome") == "ERROR"
        and case.get("expected_error", {}).get("code") == "INVALID_PRESET"
    )


def validate_bundle(path: Path, value: Any, errors: list[str]) -> list[dict[str, Any]]:
    location = str(path.relative_to(ROOT))
    if not isinstance(value, dict):
        add_error(errors, location, "bundle must be an object")
        return []
    missing = REQUIRED_BUNDLE_KEYS - value.keys()
    if missing:
        add_error(errors, location, f"missing bundle fields: {sorted(missing)}")
    if value.get("schema_version") != "2.0":
        add_error(errors, location, "schema_version must be '2.0'")
    if not isinstance(value.get("suite"), str) or not value.get("suite"):
        add_error(errors, location, "suite must be a non-empty string")
    if value.get("map_file") != "map.json":
        add_error(errors, location, "bundle map_file must be 'map.json'")
    tests = value.get("tests")
    if not isinstance(tests, list):
        add_error(errors, location, "tests must be an array")
        return []
    return tests


def validate_case(case: Any, source: Path, index: int, errors: list[str]) -> None:
    location = f"{source.relative_to(ROOT)}.tests[{index}]"
    if not isinstance(case, dict):
        add_error(errors, location, "case must be an object")
        return
    missing = REQUIRED_CASE_KEYS - case.keys()
    if missing:
        add_error(errors, location, f"missing case fields: {sorted(missing)}")
        return
    if "schema_version" in case:
        add_error(errors, location, "schema_version belongs to the bundle, not an individual case")
    if any(key in case for key in ("expected_result", "expected_error_code")):
        add_error(errors, location, "legacy error assertion fields are not allowed")
    if not isinstance(case["case_id"], str) or not case["case_id"]:
        add_error(errors, location, "case_id must be a non-empty string")
    if not isinstance(case["case_name"], str) or not case["case_name"]:
        add_error(errors, location, "case_name must be a non-empty string")
    if not isinstance(case["actions"], list):
        add_error(errors, location, "actions must be an array")
    if not isinstance(case["expected"], dict):
        add_error(errors, location, "expected must be an object")

    map_path = ROOT / "tests" / str(case["map_file"])
    if not map_path.is_file():
        add_error(errors, location, f"map_file does not exist: {case['map_file']!r}")

    expected_outcome = case.get("expected_outcome")
    if expected_outcome is not None:
        if expected_outcome != "ERROR":
            add_error(errors, location, "expected_outcome, when present, must be 'ERROR'")
        expected_error = case.get("expected_error")
        if not isinstance(expected_error, dict) or not isinstance(expected_error.get("code"), str):
            add_error(errors, location, "ERROR cases require expected_error.code")
    elif "expected_error" in case:
        add_error(errors, location, "expected_error requires expected_outcome='ERROR'")

    preset = case["preset"]
    if not isinstance(preset, dict):
        add_error(errors, location, "preset must be an object")
        return
    if is_invalid_preset_case(case):
        return

    missing_preset = REQUIRED_PRESET_KEYS - preset.keys()
    if missing_preset:
        add_error(errors, location, f"missing v2 preset fields: {sorted(missing_preset)}")
    removed = REMOVED_PRESET_FIELDS & preset.keys()
    if removed:
        add_error(errors, location, f"removed preset fields in a valid preset: {sorted(removed)}")

    users = preset.get("users")
    players = preset.get("players")
    if not isinstance(users, list) or not 2 <= len(users) <= 4 or len(users) != len(set(users)):
        add_error(errors, location, "users must contain 2-4 distinct ids")
        return
    if not isinstance(players, list) or [p.get("id") for p in players if isinstance(p, dict)] != users:
        add_error(errors, location, "players must appear once each in users order")
        return
    if preset.get("current_user") not in users:
        add_error(errors, location, "current_user must appear in users")

    for player_index, player in enumerate(players):
        player_location = f"{location}.preset.players[{player_index}]"
        if not isinstance(player, dict):
            add_error(errors, player_location, "player must be an object")
            continue
        removed_player = REMOVED_PLAYER_FIELDS & player.keys()
        if removed_player:
            add_error(errors, player_location, f"removed player fields: {sorted(removed_player)}")
        items = player.get("items")
        if not isinstance(items, dict):
            add_error(errors, player_location, "items must be an object")
        elif REMOVED_ITEMS & items.keys():
            add_error(errors, player_location, "BOMB is removed from v2 inventory")

    map_items = preset.get("map_items")
    if isinstance(map_items, list):
        for item_index, item in enumerate(map_items):
            if isinstance(item, dict) and item.get("type") == "BOMB":
                add_error(errors, f"{location}.preset.map_items[{item_index}]", "BOMB is removed in v2")


def validate_standard_map(errors: list[str]) -> None:
    path = ROOT / "tests" / "map.json"
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        add_error(errors, "tests/map.json", str(exc))
        return
    if value.get("schema_version") != "2.0" or value.get("size") != 70:
        add_error(errors, "tests/map.json", "map must use schema 2.0 and contain 70 cells")
    actual = {
        item.get("position"): item.get("type")
        for item in value.get("blocks", [])
        if isinstance(item, dict)
    }
    expected = {
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
    if actual != expected:
        add_error(errors, "tests/map.json", f"special-cell layout differs: {actual!r}")


def semantic_signature(case: dict[str, Any]) -> str:
    comparable = {
        key: value
        for key, value in case.items()
        if key not in {"case_id", "case_name"}
    }
    return json.dumps(comparable, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def main() -> int:
    errors: list[str] = []
    all_cases: list[dict[str, Any]] = []
    sources: list[tuple[Path, list[dict[str, Any]]]] = []
    for path in sorted(INPUT_DIR.glob("*.json")):
        try:
            value = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
            add_error(errors, str(path.relative_to(ROOT)), str(exc))
            continue
        cases = validate_bundle(path, value, errors)
        sources.append((path, cases))
        all_cases.extend(case for case in cases if isinstance(case, dict))
        for index, case in enumerate(cases):
            validate_case(case, path, index, errors)

    ids = [case.get("case_id") for case in all_cases]
    duplicates = sorted(case_id for case_id, count in Counter(ids).items() if count > 1)
    if len(all_cases) != EXPECTED_TOTAL:
        add_error(errors, "tests/input", f"expected {EXPECTED_TOTAL} cases, found {len(all_cases)}")
    if duplicates:
        add_error(errors, "tests/input", f"duplicate case_id values: {duplicates}")
    signatures: dict[str, list[str]] = {}
    for case in all_cases:
        signatures.setdefault(semantic_signature(case), []).append(case["case_id"])
    semantic_duplicates = [case_ids for case_ids in signatures.values() if len(case_ids) > 1]
    if semantic_duplicates:
        add_error(errors, "tests/input", f"semantically duplicate cases: {semantic_duplicates}")
    validate_standard_map(errors)

    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        print(f"validation failed with {len(errors)} error(s)", file=sys.stderr)
        return 1
    print(
        f"validated {len(all_cases)} schema-v2 cases across {len(sources)} files; "
        "case ids and case bodies are unique"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
