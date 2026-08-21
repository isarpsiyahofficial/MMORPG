#!/usr/bin/env python3
"""Validate KO 1.298 N3 assets before the Android runtime is allowed to claim support.

This uses the pinned OpenKO-blender pure binary parsers against every source file.
It is not a replacement for Unity runtime tests; it is a deterministic source-format
compatibility gate that finds malformed/unsupported legacy variants early.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
import traceback

SUPPORTED = {
    ".n3pmesh",
    ".n3cpart",
    ".n3cskins",
    ".n3cplug",
    ".n3joint",
    ".n3anim",
    ".n3chr",
    ".n3shape",
}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--vendor-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args(argv)

    source = args.source.resolve()
    vendor = (args.vendor_root.resolve() / "OpenKO-blender")
    if not source.is_dir():
        raise SystemExit(f"KO source root not found: {source}")
    if not vendor.is_dir():
        raise SystemExit(f"Pinned OpenKO-blender not found: {vendor}")
    sys.path.insert(0, str(vendor))

    from openko_blender.formats import n3anim, n3chr, n3cpart, n3cplug, n3joint, n3pmesh, n3shape

    loaders = {
        ".n3pmesh": n3pmesh.load,
        ".n3cpart": n3cpart.load,
        ".n3cskins": n3cpart.load_skins,
        ".n3cplug": n3cplug.load,
        ".n3joint": n3joint.load,
        ".n3anim": n3anim.load,
        ".n3chr": n3chr.load,
        ".n3shape": n3shape.load,
    }

    records: list[dict[str, object]] = []
    ok = 0
    failed = 0
    unsupported = 0

    candidates = [path for path in source.rglob("*") if path.is_file() and path.suffix.lower().startswith(".n3")]
    candidates.sort(key=lambda path: path.relative_to(source).as_posix().lower())

    for path in candidates:
        relative = path.relative_to(source).as_posix()
        extension = path.suffix.lower()
        record: dict[str, object] = {"path": relative, "extension": extension}
        loader = loaders.get(extension)
        if loader is None:
            record["status"] = "unsupported"
            unsupported += 1
            records.append(record)
            continue

        try:
            loader(path)
            record["status"] = "parse-ok"
            ok += 1
        except Exception as exc:
            record["status"] = "parse-failed"
            record["error"] = f"{type(exc).__name__}: {exc}"
            failed += 1
        records.append(record)

    payload = {
        "schema": 1,
        "supportedExtensions": sorted(SUPPORTED),
        "parseOk": ok,
        "parseFailed": failed,
        "unsupported": unsupported,
        "totalN3": len(records),
        "files": records,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")

    print(f"N3 RUNTIME VALIDATION: ok={ok} failed={failed} unsupported={unsupported} total={len(records)}")
    for record in records:
        if record["status"] == "parse-failed":
            print("FAILED", record["path"], record.get("error", ""))
    for record in records:
        if record["status"] == "unsupported":
            print("UNSUPPORTED", record["path"])

    return 4 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
