#!/usr/bin/env python3
"""Run a single JSON test case and print errors as JSON on failure.

Usage: run_one_case.py <root> <case_file> <output_file>
Exit code 0 on PASS, 1 otherwise.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

root = Path(sys.argv[1])
case_file = Path(sys.argv[2])
output_file = Path(sys.argv[3])
sys.path.insert(0, str(root / "tests"))

from run_json_tests import run_case_file

report = run_case_file(case_file, output_file)
if report.get("result") != "PASS":
    print(json.dumps(report.get("errors", []), ensure_ascii=False))
    raise SystemExit(1)
