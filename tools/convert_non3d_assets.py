#!/usr/bin/env python3
"""Bulk-convert deterministic KO 1.298 non-3D assets for Unity/Android.

- runtime .tbl -> canonical JSON (no SQL)
- .dxt -> lossless PNG, including KO's raw Direct3D pixel formats
- UI textures -> Resources/LegacyUI/Textures so original UIF references resolve at runtime
- a lowercase legacy-path index preserves Windows-style case-insensitive resource lookup on Android
- other standard images/audio/config -> deterministic Unity asset buckets

Dedicated converters own UIF layout, 3D/N3, world-zone and FX binary formats.
Nothing is silently ignored; unsupported records remain explicit pending/failed entries.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import shutil
import sys

from ko_tbl import read_tbl, to_json_dict
from ko_texture import load_rgba

# This 77-byte file exists in the pinned 1.298 asset tree, but the pinned OpenKO
# runtime has no reference to "Slander" and it does not parse as CN3TableBase.
# Preserve the source bytes in the APK data corpus instead of inventing a table
# schema or allowing one unused legacy artifact to poison canonical runtime data.
UNREFERENCED_LEGACY_TABLES = {"data/slander_us.tbl"}


def _copy(source_root: Path, output_root: Path, relative: Path, bucket: str) -> Path:
    destination = output_root / bucket / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source_root / relative, destination)
    return destination


def _ui_texture_destination(unity_assets: Path, relative: Path) -> Path:
    return unity_assets / "Resources" / "LegacyUI" / "Textures" / relative.with_suffix(".png")


def _legacy_key(path: str) -> str:
    return path.replace("\\", "/").lstrip("./").lower()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--inventory", required=True, type=Path)
    parser.add_argument("--unity-assets", required=True, type=Path)
    parser.add_argument("--vendor-root", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    args = parser.parse_args(argv)

    source_root = args.source.resolve()
    unity_assets = args.unity_assets.resolve()
    vendor_root = args.vendor_root.resolve()
    output_root = unity_assets / "LegacyKO"
    inventory = json.loads(args.inventory.read_text(encoding="utf-8"))

    from PIL import Image

    output_root.mkdir(parents=True, exist_ok=True)
    records: list[dict[str, object]] = []
    errors: list[str] = []
    ui_texture_index: dict[str, str] = {}

    for entry in inventory.get("files", []):
        source_path = str(entry["path"])
        relative = Path(source_path)
        category = str(entry.get("category"))
        strategy = str(entry.get("strategy"))
        suffix = relative.suffix.lower()
        normalized_source = _legacy_key(source_path)
        result: dict[str, object] = {
            "sourcePath": source_path,
            "category": category,
            "strategy": strategy,
            "status": "pending",
        }

        try:
            if suffix == ".tbl" and normalized_source in UNREFERENCED_LEGACY_TABLES:
                destination = _copy(source_root, output_root, relative, "StreamingAssets/Raw")
                result.update(
                    status="embedded",
                    outputPath=str(destination),
                    tableConversion="unreferenced-legacy-raw",
                    reason="Not referenced by pinned OpenKO 1.298 runtime and not a valid CN3TableBase payload",
                )

            elif suffix == ".tbl":
                table = read_tbl(source_root / relative)
                destination = output_root / "StreamingAssets" / "Data" / relative.with_suffix(".json")
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_text(
                    json.dumps(to_json_dict(table), ensure_ascii=False, separators=(",", ":")),
                    encoding="utf-8",
                )
                result.update(status="converted", outputPath=str(destination), rows=len(table.rows))

            elif suffix == ".dxt":
                texture = load_rgba(source_root / relative, vendor_root)
                if category == "ui-texture":
                    destination = _ui_texture_destination(unity_assets, relative)
                else:
                    destination = output_root / "Textures" / relative.with_suffix(".png")
                destination.parent.mkdir(parents=True, exist_ok=True)
                Image.frombytes(
                    "RGBA",
                    (texture.header.width, texture.header.height),
                    texture.rgba,
                ).save(destination, format="PNG")
                result.update(
                    status="converted",
                    outputPath=str(destination),
                    width=texture.header.width,
                    height=texture.header.height,
                    format=texture.header.format_name,
                    formatValue=texture.header.format_value,
                    mipmapped=texture.header.has_mipmap,
                )
                if category == "ui-texture":
                    ui_texture_index[normalized_source] = (
                        "LegacyUI/Textures/" + relative.with_suffix("").as_posix()
                    )

            elif category == "ui-texture":
                destination = _ui_texture_destination(unity_assets, relative)
                destination.parent.mkdir(parents=True, exist_ok=True)
                if suffix == ".png":
                    shutil.copy2(source_root / relative, destination)
                else:
                    with Image.open(source_root / relative) as image:
                        image.convert("RGBA").save(destination, format="PNG")
                result.update(status="converted", outputPath=str(destination))
                ui_texture_index[normalized_source] = (
                    "LegacyUI/Textures/" + relative.with_suffix("").as_posix()
                )

            elif category in {"texture", "world-texture", "fx-texture"}:
                destination = _copy(source_root, output_root, relative, "Images")
                result.update(status="copied", outputPath=str(destination))

            elif category == "audio":
                destination = _copy(source_root, output_root, relative, "Audio")
                result.update(status="copied", outputPath=str(destination))

            elif category in {"config-text", "binary-data"}:
                destination = _copy(source_root, output_root, relative, "StreamingAssets/Raw")
                result.update(status="embedded", outputPath=str(destination))

        except Exception as exc:
            result["status"] = "failed"
            result["error"] = f"{type(exc).__name__}: {exc}"
            errors.append(f"{source_path}: {type(exc).__name__}: {exc}")

        records.append(result)

    index_path = unity_assets / "Resources" / "LegacyUI" / "texture_index.json"
    index_path.parent.mkdir(parents=True, exist_ok=True)
    index_path.write_text(
        json.dumps(
            {
                "schema": 1,
                "entries": [
                    {"legacyPath": key, "resourcePath": value}
                    for key, value in sorted(ui_texture_index.items())
                ],
            },
            indent=2,
            ensure_ascii=False,
        ),
        encoding="utf-8",
    )

    payload = {
        "schema": 1,
        "sourceFiles": int(inventory.get("totalFiles", len(records))),
        "uiTextureIndex": str(index_path),
        "uiTextureCount": len(ui_texture_index),
        "files": records,
        "errors": errors,
    }
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")

    done = sum(1 for r in records if r["status"] in {"converted", "copied", "embedded", "generated"})
    pending = sum(1 for r in records if r["status"] == "pending")
    failed = sum(1 for r in records if r["status"] == "failed")
    print(
        f"NON-3D CONVERSION: completed={done} pending={pending} failed={failed} "
        f"total={len(records)} ui_textures={len(ui_texture_index)}"
    )
    if errors:
        for error in errors[:100]:
            print("ERROR", error)
        return 4
    return 0


if __name__ == "__main__":
    sys.exit(main())
