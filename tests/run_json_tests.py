#!/usr/bin/env python3

from __future__ import annotations

import json
import subprocess
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
RUNNER_BASE = ROOT / "tests" / "json_runner"
RUNNER = RUNNER_BASE if RUNNER_BASE.exists() else RUNNER_BASE.with_suffix(".exe")

ARRAY_KEYS = {
    "players": "id",
    "properties": "position",
    "map_items": "position",
    "display_players": "position",
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
    output_file.parent.mkdir(parents=True, exist_ok=True)
    try:
        case = json.loads(case_file.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return {
            "case_id": case_file.stem,
            "result": "ERROR",
            "errors": [{"code": "INVALID_JSON", "message": str(exc)}],
        }

    try:
        completed = subprocess.run(
            [str(RUNNER), str(case_file), str(output_file)],
            cwd=ROOT,
            check=False,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=10,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        return {
            "case_id": case.get("case_id", case_file.stem),
            "result": "ERROR",
            "errors": [{"code": "C_RUNNER_FAILED", "message": str(exc)}],
        }

    if completed.returncode != 0:
        message = completed.stderr.strip() or "C runner returned a non-zero status"
        return {
            "case_id": case.get("case_id", case_file.stem),
            "result": "ERROR",
            "errors": [{"code": "C_RUNNER_FAILED", "message": message}],
        }

    try:
        runner_report = json.loads(output_file.read_text(encoding="utf-8"))
        actual = runner_report["actual"]
        expected = case["expected"]
    except (OSError, json.JSONDecodeError, KeyError, TypeError) as exc:
        return {
            "case_id": case.get("case_id", case_file.stem),
            "result": "ERROR",
            "errors": [{"code": "INVALID_C_OUTPUT", "message": str(exc)}],
        }

    errors: list[dict[str, str]] = []
    _compare(expected, actual, "actual", errors)
    return {
        "schema_version": "1.0",
        "case_id": case.get("case_id", case_file.stem),
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
        total += 1
        output_file = output_dir / case_file.name
        report = run_case_file(case_file, output_file)
        result = report.get("result", "ERROR")
        case_id = report.get("case_id", case_file.stem)
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
