#!/usr/bin/env python3
"""Inventory the complete KO 1.298 client asset tree.

The migration is not allowed to silently skip files. Every source file is hashed,
classified and assigned a conversion or platform-exclusion strategy.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

MODEL_EXTS = {
    ".n3chr", ".n3cpart", ".n3cplug", ".n3joint", ".n3anim", ".n3pmesh", ".n3shape",
    ".n3scene", ".n3camera", ".n3light", ".n3transform", ".n3vm", ".n3skin",
    ".n3cskin", ".n3cskins", ".n3mesh", ".n3vmesh", ".n3fxplug",
}
TEXTURE_EXTS = {".dxt", ".tga", ".bmp", ".jpg", ".jpeg", ".png"}
TERRAIN_TEXTURE_EXTS = {".gtt"}
AUDIO_EXTS = {".wav", ".mp3", ".ogg"}
TABLE_EXTS = {".tbl"}
UI_EXTS = {".uif"}
ZONE_EXTS = {
    ".ens", ".gev", ".glo", ".gmd", ".gtd", ".opd", ".opdsub", ".opdext", ".tct", ".tlt",
    ".flag", ".evt", ".evtsub", ".map", ".path", ".wall", ".warp", ".regen", ".river", ".pond",
    ".gfo",
}
FX_EXTS = {".n3fx", ".fx", ".fxb", ".fxp"}
TEXT_EXTS = {".ini", ".txt", ".md", ".csv", ".log"}
BINARY_DATA_EXTS = {".dat", ".bin"}
AMBIENT_TEXT_EXTS = {".brd", ".lst"}
WORLD_ENV_EXTS = {".n3sky", ".grs", ".gea", ".mcn"}
WINDOWS_MIDDLEWARE_EXTS = {".dll", ".asi", ".m3d", ".flt"}

KNOWN_TOP_LEVEL = {
    "Chr", "ChrSelect", "DTex", "Data", "Intro", "Item", "Misc", "Object", "Snd",
    "UI", "UI_US", "Zones", "fx", "symbol_us",
}


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def classify(relative: Path) -> tuple[str, str]:
    suffix = relative.suffix.lower()
    top = relative.parts[0] if relative.parts else ""
    normalized = relative.as_posix().lower()

    if suffix in TABLE_EXTS:
        return "game-data", "tbl-to-canonical"
    if suffix in UI_EXTS:
        return "ui-layout", "uif-to-unity-ui"
    if top in {"UI", "UI_US", "Intro", "symbol_us"} and suffix in TEXTURE_EXTS:
        return "ui-texture", "texture-to-unity"
    if top == "DTex" and suffix in TERRAIN_TEXTURE_EXTS:
        return "world-texture", "gtt-to-unity-terrain-texture"
    if top == "Zones" and suffix in ZONE_EXTS:
        return "world-zone", "zone-to-unity-world"
    if top == "Zones" and suffix in TEXTURE_EXTS:
        return "world-texture", "texture-to-unity"
    if top == "fx" and suffix not in {".md", ".txt"}:
        if suffix in TEXTURE_EXTS:
            return "fx-texture", "texture-to-unity"
        return "fx-data", "fx-to-unity-vfx"
    if suffix in MODEL_EXTS:
        return "3d-asset", "n3-to-unity"
    if suffix in TEXTURE_EXTS:
        return "texture", "texture-to-unity"
    if suffix in AUDIO_EXTS:
        return "audio", "audio-to-unity"
    if suffix in FX_EXTS:
        return "fx-data", "fx-to-unity-vfx"
    if suffix in AMBIENT_TEXT_EXTS:
        return "world-zone", "ambient-text-to-canonical"
    if suffix in WORLD_ENV_EXTS:
        return "world-zone", "world-environment-to-unity"
    if suffix in WINDOWS_MIDDLEWARE_EXTS and top == "Snd":
        return "windows-only", "exclude-win32-audio-middleware"
    if suffix in TEXT_EXTS:
        return "config-text", "copy-runtime-data"
    if suffix in BINARY_DATA_EXTS:
        return "binary-data", "binary-to-canonical"

    # This extensionless 1.298 artifact is not referenced by the pinned runtime,
    # but retaining its exact hash/raw bytes keeps provenance complete.
    if normalized == "chrselect/ka_cave":
        return "binary-data", "preserve-unreferenced-legacy"

    return "unclassified", "none"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--fail-on-unclassified", action="store_true")
    args = parser.parse_args(argv)

    source = args.source.resolve()
    if not source.is_dir():
        raise SystemExit(f"KO source tree not found: {source}")

    files = [p for p in source.rglob("*") if p.is_file() and ".git" not in p.parts]
    files.sort(key=lambda p: p.relative_to(source).as_posix().lower())

    records: list[dict[str, object]] = []
    category_counts: Counter[str] = Counter()
    strategy_counts: Counter[str] = Counter()
    extension_counts: Counter[str] = Counter()
    unknown_top: set[str] = set()
    unclassified: list[str] = []
    total_bytes = 0

    for path in files:
        rel = path.relative_to(source)
        top = rel.parts[0] if len(rel.parts) > 1 else "<root>"
        if top not in KNOWN_TOP_LEVEL and top != "<root>":
            unknown_top.add(top)

        category, strategy = classify(rel)
        if category == "unclassified":
            unclassified.append(rel.as_posix())

        size = path.stat().st_size
        total_bytes += size
        suffix = path.suffix.lower() or "<none>"
        category_counts[category] += 1
        strategy_counts[strategy] += 1
        extension_counts[suffix] += 1
        records.append({
            "path": rel.as_posix(),
            "bytes": size,
            "sha256": sha256(path),
            "extension": suffix,
            "category": category,
            "strategy": strategy,
        })

    payload = {
        "schema": 2,
        "sourceRoot": str(source),
        "totalFiles": len(records),
        "totalBytes": total_bytes,
        "categories": dict(sorted(category_counts.items())),
        "strategies": dict(sorted(strategy_counts.items())),
        "extensions": dict(sorted(extension_counts.items())),
        "unknownTopLevelFolders": sorted(unknown_top),
        "unclassified": unclassified,
        "files": records,
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")

    print(f"KO FULL INVENTORY: {len(records)} files / {total_bytes} bytes")
    for category, count in sorted(category_counts.items()):
        print(f"  {category}: {count}")
    if unknown_top:
        print("UNKNOWN TOP-LEVEL FOLDERS:", ", ".join(sorted(unknown_top)))
    if unclassified:
        print(f"UNCLASSIFIED FILES: {len(unclassified)}")
        for path in unclassified[:100]:
            print("  -", path)
        if len(unclassified) > 100:
            print(f"  ... and {len(unclassified) - 100} more")

    if args.fail_on_unclassified and (unclassified or unknown_top):
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
