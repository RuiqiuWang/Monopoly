#!/usr/bin/env python3
"""Run one JSON case and return a non-zero status on failure."""
from __future__ import annotations

import json
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: run_json_case.py ROOT CASE_FILE OUTPUT_FILE", file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    case_file = Path(sys.argv[2]).resolve()
    output_file = Path(sys.argv[3]).resolve()
    sys.path.insert(0, str(root / "tests"))
    from run_json_tests import run_case_file

    report = run_case_file(case_file, output_file)
    if report.get("result") != "PASS":
        print(json.dumps(report.get("errors", []), ensure_ascii=False))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
