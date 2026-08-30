#!/usr/bin/env python3
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
state = ROOT / ".monopoly_state.json"
assets = ROOT / "player_assets.json"
backup = state.read_bytes() if state.exists() else None


def run_game(input_text: str) -> str:
    completed = subprocess.run(
        [str(ROOT / "game_engine.exe") if (ROOT / "game_engine.exe").exists()
         else str(ROOT / "game_engine")],
        cwd=ROOT,
        input=input_text,
        text=True,
        encoding="utf-8",
        errors="replace",
        capture_output=True,
        timeout=10,
        check=False,
    )
    assert completed.returncode == 0, completed.stderr
    return completed.stdout

try:
    state.write_text('{"has_run": 0}\n', encoding="utf-8")
    first_run = run_game("12\nN\nquit\n")
    assert "Choose 2-4 characters" in first_run
    assert "是否进行新手教程" in first_run

    state.write_text('{"has_run": 1}\n', encoding="utf-8")
    output = run_game(
            "12\n"
            "help\n"
            "step 1 1\n"
            "step 2 1\n"
            "query 1\n"
            "step 1 63\n"
            "query 1\n"
            "step 1 34\n"
            "2\n"
            "F\n"
            "query 1\n"
            "quit\n"
    )
    assert "Choose 2-4 characters" in output
    assert "Commands:" in output
    assert "Player Q (id=1)" in output
    assert "Property purchased at level 1." in output
    assert "Paid rent 20 to Q." in output
    assert "owner=Q level=1/3" in output
    assert "points=60 position=64" in output
    assert "Item purchased." in output
    assert "items: barrier=0 robot=1 bomb=0" in output
    payload = json.loads(assets.read_text(encoding="utf-8"))
    assert payload["id"] == 1
    assert payload["name"] == "Q"
    assert payload["money"] == 820
    assert payload["items"]["robot"] == 1
    assert payload["properties"] == [{"position": 1, "level": 1}]

    bankruptcy_input = "12\nstep 1 36\nstep 2 36\n" + "step 2 70\n" * 11
    bankruptcy = run_game(bankruptcy_input)
    assert "Player A is bankrupt" in bankruptcy
    assert "Winner: Q" in bankruptcy
    assert "WINNER" in bankruptcy
    print("PASS: game engine end-to-end workflow")
finally:
    if backup is None:
        state.unlink(missing_ok=True)
    else:
        state.write_bytes(backup)
    assets.unlink(missing_ok=True)
