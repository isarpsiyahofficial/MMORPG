#!/usr/bin/env python3
"""Build the exact KO 1.298 Android runtime pack.

Every non-Win32 source file is copied byte-for-byte into Unity StreamingAssets.
The pack index records source SHA-256, size, extension, category and strategy.
No legacy game asset is renamed, re-encoded or silently omitted here.

Runtime parsers are checked separately; this script only guarantees exact APK
provenance and Android-accessible packaging.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def copy_exact(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, destination)
    if source.stat().st_size != destination.stat().st_size:
        raise RuntimeError(f"size mismatch after copy: {source}")
    source_hash = sha256(source)
    destination_hash = sha256(destination)
    if source_hash != destination_hash:
        raise RuntimeError(
            f"hash mismatch after copy: {source} -> {destination}: {source_hash} != {destination_hash}"
        )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--inventory", required=True, type=Path)
    parser.add_argument("--unity-assets", required=True, type=Path)
    parser.add_argument("--output-manifest", required=True, type=Path)
    args = parser.parse_args(argv)

    source_root = args.source.resolve()
    unity_assets = args.unity_assets.resolve()
    runtime_root = unity_assets / "StreamingAssets" / "KO"
    inventory = json.loads(args.inventory.read_text(encoding="utf-8"))

    if runtime_root.exists():
        shutil.rmtree(runtime_root)
    runtime_root.mkdir(parents=True, exist_ok=True)

    records: list[dict[str, object]] = []
    copied = 0
    excluded = 0
    bytes_copied = 0

    for entry in inventory.get("files", []):
        relative = Path(str(entry["path"]))
        source = source_root / relative
        if not source.is_file():
            raise SystemExit(f"inventory source disappeared: {relative}")

        record: dict[str, object] = {
            "path": relative.as_posix(),
            "bytes": int(entry["bytes"]),
            "sha256": str(entry["sha256"]),
            "extension": str(entry.get("extension", "")),
            "category": str(entry.get("category", "")),
            "strategy": str(entry.get("strategy", "")),
        }

        if entry.get("category") == "windows-only":
            record["status"] = "platform-excluded"
            record["reason"] = "Win32 native middleware is replaced by Unity/Android platform services"
            excluded += 1
            records.append(record)
            continue

        destination = runtime_root / relative
        copy_exact(source, destination)
        if destination.stat().st_size != int(entry["bytes"]):
            raise SystemExit(f"inventory byte count mismatch: {relative}")
        if sha256(destination) != str(entry["sha256"]):
            raise SystemExit(f"inventory SHA-256 mismatch: {relative}")

        record["status"] = "embedded-exact"
        record["runtimePath"] = "KO/" + relative.as_posix()
        copied += 1
        bytes_copied += destination.stat().st_size
        records.append(record)

    manifest = {
        "schema": 1,
        "sourceCommit": "2055ee6ed77f1b5cfef23832dd5bc31909e14a66",
        "sourceFiles": int(inventory.get("totalFiles", len(records))),
        "embeddedFiles": copied,
        "platformExcludedFiles": excluded,
        "embeddedBytes": bytes_copied,
        "files": records,
    }
    args.output_manifest.parent.mkdir(parents=True, exist_ok=True)
    args.output_manifest.write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")

    print(
        f"ANDROID RUNTIME PACK: exact={copied} platform-excluded={excluded} "
        f"bytes={bytes_copied} total={len(records)}"
    )
    if copied + excluded != len(records):
        return 3
    return 0


if __name__ == "__main__":
    sys.exit(main())
