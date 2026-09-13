# PracticeMe — Geometry Dash Geode Mod

PracticeMe is a practice-analysis mod. It watches your Practice Mode attempts, finds where you die repeatedly, and recommends focused practice ranges.

- Deterministic, lightweight, no heavy per-frame work
- Practice Mode focused, Normal Mode tracked separately
- Adaptive priorities: HIGH → MEDIUM → GOOD as you improve
- Session history & data confidence
- Safe storage in `practiceme.json` under the mod save dir

## Build

```bat
cmake -B build -G Ninja
cmake --build build --config Release
```

or

```
geode build --platform windows
```

## Assets

Place these in `resources/`:

- `level_menu_icon.png` —  the button in the level info screen (e.g., 64x64)
- `logo.png` — mod logo

The project builds without them (fallback sprites used), but they are referenced via `"_spr"` resources.

## Storage

`%LOCALAPPDATA%/GeometryDash/geode/mods/kobi.practiceme/practiceme.json`

Delete the file to reset all data.

## Developer

KOBI
