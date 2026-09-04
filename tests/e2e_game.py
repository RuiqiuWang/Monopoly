#!/usr/bin/env python3
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STATE = ROOT / ".monopoly_state.json"
ASSETS = ROOT / "player_assets.json"
BACKUP = STATE.read_bytes() if STATE.exists() else None


def run_game(input_text: str) -> str:
    executable = ROOT / ("game_engine.exe" if (ROOT / "game_engine.exe").exists() else "game_engine")
    completed = subprocess.run(
        [str(executable)],
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
    STATE.unlink(missing_ok=True)
    interrupted = run_game("1000\n12\n")
    assert "是否进行新手教程" in interrupted
    assert not STATE.exists()

    STATE.write_text('{"has_run": 0}\n', encoding="utf-8")
    first_run = run_game("\n12\nN\nq\n")
    assert "Choose 2-4 characters" in first_run
    assert "是否进行新手教程" in first_run
    assert json.loads(STATE.read_text(encoding="utf-8"))["has_run"] == 1

    invalid_money = run_game("999\n50001\n10000\n12\nq\n")
    assert invalid_money.count("Initial money must be an integer") == 2

    STATE.write_text('{"has_run": 1}\n', encoding="utf-8")
    property_flow = run_game(
        "1000\n"
        "12\n"
        "help\n"
        "step 1\n"
        "Y\n"
        "query 1\n"
        "step 1\n"
        "q\n"
    )
    assert "MONOPOLY command help" in property_flow
    assert "Property purchased at level 0." in property_flow
    assert "Paid rent 100 to Q." in property_flow
    payload = json.loads(ASSETS.read_text(encoding="utf-8"))
    assert payload["id"] == 1
    assert payload["money"] == 800
    assert payload["items"] == {"barrier": 0, "robot": 0}
    assert payload["properties"] == [{"position": 1, "level": 0}]

    STATE.write_text('{"has_run": 1}\n', encoding="utf-8")
    zero_step = run_game(
        "1000\n"
        "12\n"
        "step 1\n"
        "Y\n"
        "step 0\n"
        "step 0\n"
        "Y\n"
        "q\n"
    )
    assert zero_step.count("moved 0 of 0 steps") >= 2
    assert "Property upgraded to level 1." in zero_step

    STATE.write_text('{"has_run": 1}\n', encoding="utf-8")
    park_flow = run_game("1000\n12\nstep 14\nstep 49\nstep 49\nq\n")
    assert "到达14号位置" in park_flow
    assert "到达49号位置" in park_flow
    assert "到达63号位置" in park_flow
    assert "购买 14 号地产" not in park_flow

    STATE.write_text('{"has_run": 1}\n', encoding="utf-8")
    gift_flow = run_game(
        "1000\n"
        "12\n"
        "step 35\n"
        "3\n"
        "step 0\n"
        "query 1\n"
        "q\n"
    )
    assert "Gift house:" in gift_flow
    assert "god_of_wealth_rounds=5" in gift_flow

    STATE.write_text('{"has_run": 1}\n', encoding="utf-8")
    tool_room_balance = run_game(
        "1000\n"
        "12\n"
        "step 64\n"
        "step 0\n"
        "step 34\n"
        "1\n"
        "q\n"
    )
    assert "Item purchased." in tool_room_balance
    assert "当前点数10，低于最便宜道具所需的30点" in tool_room_balance

    STATE.write_text('{"has_run": 1}\n', encoding="utf-8")
    tool_room_entry = run_game("1000\n12\nstep 28\nq\n")
    assert "当前点数0，低于最便宜道具所需的30点" in tool_room_entry

    STATE.write_text('{"has_run": 1}\n', encoding="utf-8")
    quit_removed = run_game("1000\n12\nquit\nq\n")
    assert "未知命令" in quit_removed

    STATE.write_text('{"has_run": 1}\n', encoding="utf-8")
    reset_output = run_game("1000\n12\nreset\nq\n")
    assert "Play record cleared" in reset_output
    assert not STATE.exists()

    print("PASS: game engine end-to-end workflow")
finally:
    if BACKUP is None:
        STATE.unlink(missing_ok=True)
    else:
        STATE.write_bytes(BACKUP)
    ASSETS.unlink(missing_ok=True)
