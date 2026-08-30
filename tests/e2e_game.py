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
    state.unlink(missing_ok=True)
    interrupted = run_game("1000\n12\n")
    assert "是否进行新手教程" in interrupted
    assert not state.exists()

    state.write_text('{"has_run": 0}\n', encoding="utf-8")
    first_run = run_game("\n12\nN\nquit\n")
    assert "Choose 2-4 characters" in first_run
    assert "是否进行新手教程" in first_run
    assert json.loads(state.read_text(encoding="utf-8"))["has_run"] == 1

    invalid_money = run_game("999\n50001\n10000\n12\nquit\n")
    assert invalid_money.count("Initial money must be an integer") == 2
    assert "Choose 2-4 characters" in invalid_money

    state.write_text('{"has_run": 1}\n', encoding="utf-8")
    output = run_game(
            "1000\n"
            "12\n"
            "help\n"
            "step 1\n"
            "step 1\n"
            "query 1\n"
            "step 63\n"
            "query 1\n"
            "step 69\n"
            "step 34\n"
            "3\n"
            "query 1\n"
            "quit\n"
    )
    assert "Choose 2-4 characters" in output
    assert "MONOPOLY command help" in output
    assert "The board has 70 blocks (0-69)." in output
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

    bankruptcy_input = "1000\n12\nstep 36\nstep 36\n" + "step 70\n" * 24
    bankruptcy = run_game(bankruptcy_input)
    assert "Player A is bankrupt" in bankruptcy
    assert "Winner: Q" in bankruptcy
    assert "WINNER" in bankruptcy

    state.write_text('{"has_run": 1}\n', encoding="utf-8")
    reset_output = run_game("1000\n12\nreset\nquit\n")
    assert "Play record cleared" in reset_output
    assert not state.exists()

    state.write_text('{"has_run": 1}\n', encoding="utf-8")
    item_effect = run_game(
        "1000\n"
        "12\n"
        "step 2\n"
        "step 1\n"
        "step 62\n"
        "step 69\n"
        "step 34\n"
        "1\n"
        "step 1\n"
        "block 2\n"
        "step 1\n"
        "step 29\n"
        "quit\n"
    )
    assert "Barrier placed at 30." in item_effect
    assert "Stopped by a barrier." in item_effect

    state.write_text('{"has_run": 1}\n', encoding="utf-8")
    bomb_effect = run_game(
        "1000\n"
        "12\n"
        "step 2\n"
        "step 1\n"
        "step 62\n"
        "step 69\n"
        "step 34\n"
        "2\n"
        "F\n"
        "step 1\n"
        "bomb 2\n"
        "step 29\n"
        "step 1\n"
        "step 1\n"
        "step 1\n"
        "quit\n"
    )
    assert "Bomb placed at 30." in bomb_effect
    assert "Hit a bomb and was sent to hospital." in bomb_effect
    assert bomb_effect.count("skips this turn in hospital") == 3

    state.write_text('{"has_run": 1}\n', encoding="utf-8")
    gift_effect = run_game(
        "1000\n"
        "12\n"
        "step 35\n"
        "3\n"
        "step 1\n"
        "query 1\n"
        "quit\n"
    )
    assert "Gift house:" in gift_effect
    assert "god_of_wealth_rounds=5" in gift_effect

    state.write_text('{"has_run": 1}\n', encoding="utf-8")
    invalid_gift = run_game(
        "1000\n"
        "12\n"
        "step 35\n"
        "0\n"
        "query 1\n"
        "quit\n"
    )
    assert "Invalid gift. This opportunity was skipped." in invalid_gift
    assert "god_of_wealth_rounds=0" in invalid_gift

    state.write_text('{"has_run": 1}\n', encoding="utf-8")
    interrupted_gift = run_game("1000\n12\nstep 35\n")
    assert "Gift house:" in interrupted_gift
    print("PASS: game engine end-to-end workflow")
finally:
    if backup is None:
        state.unlink(missing_ok=True)
    else:
        state.write_bytes(backup)
    assets.unlink(missing_ok=True)
