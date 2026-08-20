from pathlib import Path

root = Path(__file__).resolve().parents[1]
src = (root / "src" / "GameplayBridge.cpp").read_text(encoding="utf-8")

assert "#if defined(GTASA)" in src
assert "return FindPlayerPed(-1);" in src
assert "#elif defined(GTAVC) || defined(GTA3)" in src
assert "return FindPlayerPed();" in src

print("cross-game FindPlayerPed signature audit: OK")
