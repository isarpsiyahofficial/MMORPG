#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

from ko_world import parse_terrain


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args(argv)

    source = args.source.resolve()
    files = sorted(source.rglob("*.gtd"), key=lambda p: p.relative_to(source).as_posix().lower())
    records: list[dict[str, object]] = []
    failures = 0

    for path in files:
        relative = path.relative_to(source).as_posix()
        try:
            data = parse_terrain(path)
            records.append({
                "path": relative,
                "status": "parse-ok",
                "formatVersion": data.format_version,
                "gtdVersion": data.gtd_version,
                "name": data.name,
                "mapSize": data.map_size,
                "heightMin": data.height_min,
                "heightMax": data.height_max,
                "tileTextures": data.tile_texture_count,
                "gttSources": data.gtt_sources,
                "rivers": len(data.river),
                "ponds": len(data.pond),
                "bytes": data.byte_length,
            })
        except Exception as exc:
            records.append({"path": relative, "status": "parse-failed", "error": f"{type(exc).__name__}: {exc}"})
            failures += 1

    payload = {
        "schema": 1,
        "terrainFiles": len(files),
        "parseOk": len(files) - failures,
        "parseFailed": failures,
        "files": records,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"TERRAIN VALIDATION: ok={len(files) - failures} failed={failures} total={len(files)}")
    for record in records:
        if record["status"] == "parse-failed":
            print("FAILED", record["path"], record.get("error", ""))
    return 4 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
