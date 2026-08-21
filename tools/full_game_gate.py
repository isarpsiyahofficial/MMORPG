#!/usr/bin/env python3
"""Final completeness gate for the KO -> Android conversion."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

SUCCESS = {"converted", "copied", "embedded", "generated", "platform-excluded"}
MANDATORY_CATEGORIES = {
    "3d-asset", "game-data", "audio", "world-zone", "texture", "ui-layout", "ui-texture",
}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--inventory", required=True, type=Path)
    parser.add_argument("--conversion", required=True, type=Path)
    args = parser.parse_args(argv)

    inventory = json.loads(args.inventory.read_text(encoding="utf-8"))
    conversion = json.loads(args.conversion.read_text(encoding="utf-8"))

    source = {entry["path"]: entry for entry in inventory.get("files", [])}
    converted = {entry["sourcePath"]: entry for entry in conversion.get("files", [])}

    missing = sorted(set(source) - set(converted))
    failed = sorted(
        path for path, entry in converted.items()
        if path in source and entry.get("status") not in SUCCESS
    )
    extra = sorted(set(converted) - set(source))
    unclassified = list(inventory.get("unclassified", []))

    invalid_exclusions = sorted(
        path for path, entry in converted.items()
        if entry.get("status") == "platform-excluded"
        and (
            source.get(path, {}).get("category") != "windows-only"
            or not str(entry.get("reason", "")).strip()
            or not str(entry.get("sourcePreservedByHash", "")).strip()
        )
    )

    present_categories = {entry.get("category") for entry in source.values()}
    category_gaps = sorted(MANDATORY_CATEGORIES - present_categories)

    print(f"SOURCE FILES: {len(source)}")
    print(f"CONVERSION RECORDS: {len(converted)}")
    print(f"MISSING: {len(missing)}")
    print(f"FAILED/PENDING: {len(failed)}")
    print(f"EXTRA: {len(extra)}")
    print(f"UNCLASSIFIED: {len(unclassified)}")
    print(f"INVALID PLATFORM EXCLUSIONS: {len(invalid_exclusions)}")

    for title, items in (
        ("MISSING", missing),
        ("FAILED/PENDING", failed),
        ("EXTRA", extra),
        ("UNCLASSIFIED", unclassified),
        ("INVALID PLATFORM EXCLUSIONS", invalid_exclusions),
        ("CATEGORY GAPS", category_gaps),
    ):
        if items:
            print(title + ":")
            for item in items[:100]:
                print("  -", item)
            if len(items) > 100:
                print(f"  ... and {len(items) - 100} more")

    ok = (
        not missing
        and not failed
        and not extra
        and not unclassified
        and not invalid_exclusions
        and not category_gaps
    )
    if ok:
        print("FULL KO CONVERSION GATE: PASS")
        return 0
    print("FULL KO CONVERSION GATE: FAIL")
    return 3


if __name__ == "__main__":
    sys.exit(main())
