#!/usr/bin/env python3
"""Create and verify SHA-256 manifests for local KO source assets.

The source assets are never modified. This tool is intentionally independent
from Unity and Blender so parity can be checked before and after conversion.
"""

from __future__ import annotations

import argparse
from hashlib import sha256
from pathlib import Path
import sys


def hash_file(path: Path) -> str:
    digest = sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def source_files(root: Path):
    for path in sorted(root.rglob("*")):
        if path.is_file() and path.name.lower() != "readme.md":
            yield path


def create_manifest(root: Path, manifest: Path) -> int:
    root = root.resolve()
    rows = []
    for path in source_files(root):
        relative = path.relative_to(root).as_posix()
        rows.append(f"{hash_file(path)}  {relative}")

    manifest.parent.mkdir(parents=True, exist_ok=True)
    manifest.write_text("\n".join(rows) + ("\n" if rows else ""), encoding="utf-8")
    print(f"Manifest created: {len(rows)} files")
    return 0


def read_manifest(manifest: Path) -> dict[str, str]:
    expected: dict[str, str] = {}
    for line_number, raw in enumerate(manifest.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if not line:
            continue
        try:
            digest, relative = line.split("  ", 1)
        except ValueError as exc:
            raise ValueError(f"Invalid manifest line {line_number}: {raw!r}") from exc
        expected[relative] = digest.lower()
    return expected


def verify_manifest(root: Path, manifest: Path) -> int:
    root = root.resolve()
    expected = read_manifest(manifest)
    actual = {path.relative_to(root).as_posix(): hash_file(path) for path in source_files(root)}

    failed = False
    for relative in sorted(expected.keys() | actual.keys()):
        if relative not in expected:
            print(f"UNEXPECTED {relative}")
            failed = True
        elif relative not in actual:
            print(f"MISSING    {relative}")
            failed = True
        elif expected[relative] != actual[relative]:
            print(f"CHANGED    {relative}")
            failed = True

    if failed:
        print("KO source integrity check FAILED")
        return 1

    print(f"KO source integrity check OK: {len(actual)} files unchanged")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)

    create = sub.add_parser("create")
    create.add_argument("root", type=Path)
    create.add_argument("manifest", type=Path)

    verify = sub.add_parser("verify")
    verify.add_argument("root", type=Path)
    verify.add_argument("manifest", type=Path)

    args = parser.parse_args(argv)

    if not args.root.is_dir():
        parser.error(f"Source folder does not exist: {args.root}")

    if args.command == "create":
        return create_manifest(args.root, args.manifest)

    if not args.manifest.is_file():
        parser.error(f"Manifest does not exist: {args.manifest}")
    return verify_manifest(args.root, args.manifest)


if __name__ == "__main__":
    sys.exit(main())
