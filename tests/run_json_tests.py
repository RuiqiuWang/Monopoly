#!/usr/bin/env python3

from __future__ import annotations

import json
import copy
from pathlib import Path
from typing import Any

from json_v2_engine import JsonEngineError, execute_case


ROOT = Path(__file__).resolve().parents[1]

ARRAY_KEYS = {
    "players": "id",
    "properties": "position",
    "map_items": "position",
    "display_players": "position",
    "display_cells": "position",
}


def _error(path: str, message: str, **details: str) -> dict[str, str]:
    return {"code": "ASSERT_NOT_EQUAL", "path": path, "message": message, **details}


def _compare(expected: Any, actual: Any, path: str, errors: list[dict[str, str]]) -> None:
    if isinstance(expected, dict):
        if not isinstance(actual, dict):
            errors.append(_error(path, "expected an object"))
            return
        for key, expected_value in expected.items():
            if key == "properties_absent":
                _check_absent(actual, expected_value, "properties", path, errors)
                continue
            if key == "map_items_absent":
                _check_absent(actual, expected_value, "map_items", path, errors)
                continue
            if key == "fields_absent":
                _check_fields_absent(actual, expected_value, path, errors)
                continue
            if key == "fortune_assert":
                _check_fortune_assert(actual, expected_value, path, errors)
                continue
            if key not in actual:
                errors.append({
                    "code": "ASSERT_NOT_FOUND",
                    "path": f"{path}.{key}",
                    "message": "expected field is missing from actual",
                })
                continue
            _compare(expected_value, actual[key], f"{path}.{key}", errors)
        return

    if isinstance(expected, list):
        if not isinstance(actual, list):
            errors.append(_error(path, "expected an array"))
            return
        key_name = ARRAY_KEYS.get(path.rsplit(".", 1)[-1])
        if key_name is None:
            if len(expected) != len(actual):
                errors.append(_error(path, "array length differs"))
                return
            for index, expected_value in enumerate(expected):
                _compare(expected_value, actual[index], f"{path}[{index}]", errors)
            return

        indexed_actual = {
            item.get(key_name): item
            for item in actual
            if isinstance(item, dict) and key_name in item
        }
        for index, expected_value in enumerate(expected):
            if not isinstance(expected_value, dict) or key_name not in expected_value:
                errors.append({
                    "code": "INVALID_EXPECTED",
                    "path": f"{path}[{index}]",
                    "message": f"array item must contain key {key_name!r}",
                })
                continue
            identity = expected_value[key_name]
            if identity not in indexed_actual:
                errors.append({
                    "code": "ASSERT_NOT_FOUND",
                    "path": f"{path}[{key_name}={identity}]",
                    "message": "expected array item is missing from actual",
                })
                continue
            _compare(
                expected_value,
                indexed_actual[identity],
                f"{path}[{key_name}={identity}]",
                errors,
            )
        return

    if type(expected) is not type(actual) or expected != actual:
        errors.append(_error(
            path,
            "scalar values differ",
            expected=repr(expected),
            actual=repr(actual),
        ))


def _check_fields_absent(
    actual: dict[str, Any],
    expected_fields: Any,
    path: str,
    errors: list[dict[str, str]],
) -> None:
    if not isinstance(expected_fields, list) or not all(isinstance(field, str) for field in expected_fields):
        errors.append({
            "code": "INVALID_EXPECTED",
            "path": f"{path}.fields_absent",
            "message": "fields_absent must be an array of strings",
        })
        return
    for field in expected_fields:
        if field in actual:
            errors.append({
                "code": "ASSERT_NOT_ABSENT",
                "path": f"{path}.{field}",
                "message": "field should be absent from actual",
            })


def _check_fortune_assert(
    actual: dict[str, Any],
    assertion: Any,
    path: str,
    errors: list[dict[str, str]],
) -> None:
    if not isinstance(assertion, dict):
        errors.append({
            "code": "INVALID_EXPECTED",
            "path": f"{path}.fortune_assert",
            "message": "fortune_assert must be an object",
        })
        return
    fortune = actual.get("fortune", {})
    position = fortune.get("position") if isinstance(fortune, dict) else None
    if assertion.get("present") is True and position is None:
        errors.append(_error(f"{path}.fortune.position", "fortune should be present"))
        return
    bounds = assertion.get("position_between")
    if isinstance(bounds, list) and len(bounds) == 2 and position is not None:
        if not bounds[0] <= position <= bounds[1]:
            errors.append(_error(f"{path}.fortune.position", "fortune position is outside expected bounds"))
    if position in assertion.get("position_not_in", []):
        errors.append(_error(f"{path}.fortune.position", "fortune position is excluded"))
    if assertion.get("unoccupied") is True and any(
        player.get("position") == position and player.get("status") != "BANKRUPT"
        for player in actual.get("players", [])
        if isinstance(player, dict)
    ):
        errors.append(_error(f"{path}.fortune.position", "fortune position is occupied by a player"))
    if assertion.get("without_map_item") is True and any(
        item.get("position") == position
        for item in actual.get("map_items", [])
        if isinstance(item, dict)
    ):
        errors.append(_error(f"{path}.fortune.position", "fortune position contains a map item"))


def _check_absent(
    actual: dict[str, Any],
    expected_positions: Any,
    collection_name: str,
    path: str,
    errors: list[dict[str, str]],
) -> None:
    if not isinstance(expected_positions, list):
        errors.append({
            "code": "INVALID_EXPECTED",
            "path": f"{path}.{collection_name}_absent",
            "message": "absent positions must be an array",
        })
        return
    actual_positions = {
        item.get("position")
        for item in actual.get(collection_name, [])
        if isinstance(item, dict) and "position" in item
    }
    for position in expected_positions:
        if position in actual_positions:
            errors.append({
                "code": "ASSERT_NOT_ABSENT",
                "path": f"{path}.{collection_name}[position={position}]",
                "message": "item should be absent from actual",
            })


def run_case_file(case_file: Path, output_file: Path) -> dict[str, Any]:
    try:
        case = json.loads(case_file.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return {
            "case_id": case_file.stem,
            "result": "ERROR",
            "errors": [{"code": "INVALID_JSON", "message": str(exc)}],
        }
    if isinstance(case, dict) and isinstance(case.get("tests"), list):
        tests = case["tests"]
        if len(tests) != 1 or not isinstance(tests[0], dict):
            return {
                "case_id": case_file.stem,
                "result": "ERROR",
                "errors": [{
                    "code": "AMBIGUOUS_BUNDLE",
                    "message": "single-case runner requires a bundle containing exactly one case",
                }],
            }
        materialized = copy.deepcopy(tests[0])
        materialized.setdefault("schema_version", case.get("schema_version"))
        materialized.setdefault("map_file", case.get("map_file"))
        case = materialized
    return run_case(case, output_file, case_file.stem)


def run_case(
    case: dict[str, Any],
    output_file: Path,
    fallback_case_id: str,
) -> dict[str, Any]:
    output_file.parent.mkdir(parents=True, exist_ok=True)

    try:
        actual = execute_case(case, ROOT / "tests")
    except JsonEngineError as exc:
        output_file.parent.mkdir(parents=True, exist_ok=True)
        output_file.write_text(
            json.dumps({"error": exc.details}, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
        errors: list[dict[str, str]] = []
        if case.get("expected_outcome") != "ERROR":
            errors.append({
                "code": "UNEXPECTED_ERROR",
                "path": "error",
                "message": f"engine returned {exc.details!r}",
            })
        else:
            _compare(case.get("expected_error", {}), exc.details, "error", errors)
        return {
            "schema_version": "2.0",
            "case_id": case.get("case_id", fallback_case_id),
            "error": exc.details,
            "result": "PASS" if not errors else "FAIL",
            "errors": errors,
        }

    output_file.parent.mkdir(parents=True, exist_ok=True)
    output_file.write_text(
        json.dumps({"actual": actual}, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    if case.get("expected_outcome") == "ERROR":
        return {
            "schema_version": "2.0",
            "case_id": case.get("case_id", fallback_case_id),
            "actual": actual,
            "result": "FAIL",
            "errors": [{
                "code": "EXPECTED_ERROR_NOT_RAISED",
                "path": "error",
                "message": f"expected {case.get('expected_error', {})!r}",
            }],
        }

    errors: list[dict[str, str]] = []
    _compare(case["expected"], actual, "actual", errors)
    return {
        "schema_version": "2.0",
        "case_id": case.get("case_id", fallback_case_id),
        "actual": actual,
        "result": "PASS" if not errors else "FAIL",
        "errors": errors,
    }


def main() -> int:
    input_dir = ROOT / "tests" / "input"
    output_dir = ROOT / "tests" / "output"
    output_dir.mkdir(parents=True, exist_ok=True)

    case_files = sorted(input_dir.glob("*.json"))
    total = 0
    passed = 0
    for case_file in case_files:
        try:
            parsed = json.loads(case_file.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            parsed = None
        cases = parsed.get("tests") if isinstance(parsed, dict) else None
        if not isinstance(cases, list):
            cases = [parsed]
        for index, source_case in enumerate(cases):
            case = copy.deepcopy(source_case)
            if isinstance(case, dict) and isinstance(parsed, dict):
                case.setdefault("schema_version", parsed.get("schema_version"))
                case.setdefault("map_file", parsed.get("map_file"))
            output_file = output_dir / f"{case_file.stem}_{index:03d}.json"
            total += 1
            report = run_case(case, output_file, f"{case_file.stem}_{index:03d}")
            result = report.get("result", "ERROR")
            case_id = report.get("case_id", f"{case_file.stem}_{index:03d}")
            print(f"[{result}] {case_id}")
            if result == "PASS":
                passed += 1
            elif report.get("errors"):
                print(json.dumps(report["errors"], ensure_ascii=False))

    summary = {"total": total, "passed": passed, "failed": total - passed}
    print(json.dumps(summary, ensure_ascii=False))
    return 0 if passed == total else 1


if __name__ == "__main__":
    raise SystemExit(main())
