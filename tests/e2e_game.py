#!/usr/bin/env python3
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
state = ROOT / ".monopoly_state.json"
assets = ROOT / "player_assets.json"
backup = state.read_bytes() if state.exists() else None

try:
    state.write_text('{"has_run": 1}\n', encoding="utf-8")
    completed = subprocess.run(
        [str(ROOT / "game_engine.exe") if (ROOT / "game_engine.exe").exists()
         else str(ROOT / "game_engine")],
        cwd=ROOT,
        input="help\nstep 1 64\nquery 1\nstep 1 34\n2\nF\nquery 1\nquit\n",
        text=True,
        encoding="utf-8",
        errors="replace",
        capture_output=True,
        timeout=10,
        check=False,
    )
    assert completed.returncode == 0, completed.stderr
    output = completed.stdout
    assert "Commands:" in output
    assert "Player A (id=1)" in output
    assert "points=60 position=64" in output
    assert "Item purchased." in output
    assert "items: barrier=0 robot=1 bomb=0" in output
    payload = json.loads(assets.read_text(encoding="utf-8"))
    assert payload["id"] == 1
    assert payload["items"]["robot"] == 1
    print("PASS: game engine end-to-end workflow")
finally:
    if backup is None:
        state.unlink(missing_ok=True)
    else:
        state.write_bytes(backup)
    assets.unlink(missing_ok=True)
