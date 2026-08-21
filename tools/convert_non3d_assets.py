#!/usr/bin/env python3
"""Bulk-convert deterministic KO 1.298 non-3D assets for Unity/Android.

- runtime .tbl -> canonical JSON (no SQL)
- all legacy textures -> lossless PNG runtime pack
- UI textures -> Resources/LegacyUI/Textures for original UIF rendering
- lowercase indices preserve Windows-style case-insensitive lookup on Android
- Win32-only middleware is explicitly excluded instead of shipped in the APK

Dedicated runtime readers own UIF layout, N3/3D, world-zone and FX binary formats.
Nothing is silently ignored; unsupported records remain explicit pending entries.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import shutil
import sys

from ko_tbl import read_tbl, to_json_dict
from ko_texture import load_rgba

UNREFERENCED_LEGACY_TABLES = {"data/slander_us.tbl"}
TEXTURE_CATEGORIES = {"texture", "ui-texture", "world-texture", "fx-texture"}


def _copy(source_root: Path, output_root: Path, relative: Path, bucket: str) -> Path:
    destination = output_root / bucket / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source_root / relative, destination)
    return destination


def _ui_texture_destination(unity_assets: Path, relative: Path) -> Path:
    return unity_assets / "Resources" / "LegacyUI" / "Textures" / relative.with_suffix(".png")


def _runtime_texture_destination(unity_assets: Path, relative: Path) -> Path:
    return unity_assets / "StreamingAssets" / "KOConverted" / "Textures" / relative.with_suffix(".png")


def _legacy_key(path: str) -> str:
    return path.replace("\\", "/").lstrip("./").lower()


def _save_runtime_texture(image, unity_assets: Path, relative: Path) -> Path:
    destination = _runtime_texture_destination(unity_assets, relative)
    destination.parent.mkdir(parents=True, exist_ok=True)
    image.convert("RGBA").save(destination, format="PNG")
    return destination


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
    runtime_texture_root = unity_assets / "StreamingAssets" / "KOConverted" / "Textures"
    if runtime_texture_root.exists():
        shutil.rmtree(runtime_texture_root)
    runtime_texture_root.mkdir(parents=True, exist_ok=True)

    records: list[dict[str, object]] = []
    errors: list[str] = []
    ui_texture_index: dict[str, str] = {}
    runtime_texture_index: dict[str, str] = {}

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
            if strategy == "exclude-win32-audio-middleware":
                result.update(
                    status="platform-excluded",
                    reason="Native Win32 Miles Sound System middleware cannot execute on Android; Unity audio replaces it",
                    sourcePreservedByHash=entry.get("sha256"),
                )

            elif strategy == "preserve-unreferenced-legacy":
                destination = _copy(source_root, output_root, relative, "StreamingAssets/Raw")
                result.update(
                    status="embedded",
                    outputPath=str(destination),
                    reason="Unreferenced legacy artifact preserved byte-for-byte for provenance",
                )

            elif suffix == ".tbl" and normalized_source in UNREFERENCED_LEGACY_TABLES:
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
                image = Image.frombytes(
                    "RGBA",
                    (texture.header.width, texture.header.height),
                    texture.rgba,
                )
                runtime_destination = _save_runtime_texture(image, unity_assets, relative)
                runtime_path = "KOConverted/Textures/" + relative.with_suffix(".png").as_posix()
                runtime_texture_index[normalized_source] = runtime_path

                if category == "ui-texture":
                    ui_destination = _ui_texture_destination(unity_assets, relative)
                    ui_destination.parent.mkdir(parents=True, exist_ok=True)
                    image.save(ui_destination, format="PNG")
                    ui_texture_index[normalized_source] = (
                        "LegacyUI/Textures/" + relative.with_suffix("").as_posix()
                    )

                result.update(
                    status="converted",
                    outputPath=str(runtime_destination),
                    runtimePath=runtime_path,
                    width=texture.header.width,
                    height=texture.header.height,
                    format=texture.header.format_name,
                    formatValue=texture.header.format_value,
                    mipmapped=texture.header.has_mipmap,
                )

            elif category in TEXTURE_CATEGORIES and suffix != ".gtt":
                with Image.open(source_root / relative) as image:
                    runtime_destination = _save_runtime_texture(image, unity_assets, relative)
                    runtime_path = "KOConverted/Textures/" + relative.with_suffix(".png").as_posix()
                    runtime_texture_index[normalized_source] = runtime_path
                    if category == "ui-texture":
                        ui_destination = _ui_texture_destination(unity_assets, relative)
                        ui_destination.parent.mkdir(parents=True, exist_ok=True)
                        image.convert("RGBA").save(ui_destination, format="PNG")
                        ui_texture_index[normalized_source] = (
                            "LegacyUI/Textures/" + relative.with_suffix("").as_posix()
                        )
                result.update(status="converted", outputPath=str(runtime_destination), runtimePath=runtime_path)

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

    ui_index_path = unity_assets / "Resources" / "LegacyUI" / "texture_index.json"
    ui_index_path.parent.mkdir(parents=True, exist_ok=True)
    ui_index_path.write_text(
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

    runtime_index_path = unity_assets / "StreamingAssets" / "KOConverted" / "texture-index.json"
    runtime_index_path.parent.mkdir(parents=True, exist_ok=True)
    runtime_index_path.write_text(
        json.dumps(
            {
                "schema": 1,
                "entries": [
                    {"legacyPath": key, "runtimePath": value}
                    for key, value in sorted(runtime_texture_index.items())
                ],
            },
            indent=2,
            ensure_ascii=False,
        ),
        encoding="utf-8",
    )

    payload = {
        "schema": 3,
        "sourceFiles": int(inventory.get("totalFiles", len(records))),
        "uiTextureIndex": str(ui_index_path),
        "uiTextureCount": len(ui_texture_index),
        "runtimeTextureIndex": str(runtime_index_path),
        "runtimeTextureCount": len(runtime_texture_index),
        "files": records,
        "errors": errors,
    }
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")

    done = sum(
        1 for r in records
        if r["status"] in {"converted", "copied", "embedded", "generated", "platform-excluded"}
    )
    pending = sum(1 for r in records if r["status"] == "pending")
    failed = sum(1 for r in records if r["status"] == "failed")
    print(
        f"NON-3D CONVERSION: completed={done} pending={pending} failed={failed} "
        f"total={len(records)} ui_textures={len(ui_texture_index)} runtime_textures={len(runtime_texture_index)}"
    )
    if errors:
        for error in errors[:100]:
            print("ERROR", error)
        return 4
    return 0


if __name__ == "__main__":
    sys.exit(main())
