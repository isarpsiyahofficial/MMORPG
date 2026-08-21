#!/usr/bin/env python3
"""Extract KO 1.298 NewChrValue.tbl to SQL-free JSON for Unity."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

from ko_tbl import DT_DWORD, DT_INT, DT_STRING, KoTableError, read_tbl

EXPECTED_TYPES = [DT_DWORD, DT_STRING] + [DT_INT] * 6 + [DT_DWORD] * 12
FIELD_NAMES = [
    "id",
    "name",
    "strength",
    "stamina",
    "dexterity",
    "intelligence",
    "magicAttack",
    "bonus",
] + [f"reserved{index}" for index in range(12)]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Extract original KO starting character stats without SQL.")
    parser.add_argument("input", type=Path, help="NewChrValue.tbl")
    parser.add_argument("output", type=Path, help="Unity JSON output")
    parser.add_argument("--encoding", default="cp1252")
    args = parser.parse_args(argv)

    table = read_tbl(args.input, args.encoding)
    if table.column_types != EXPECTED_TYPES:
        raise KoTableError(
            "NewChrValue.tbl schema does not match the pinned KO 1.298 structure. "
            f"Found {table.column_types!r}"
        )

    entries = []
    for row in table.rows:
        if len(row) != len(FIELD_NAMES):
            raise KoTableError(f"NewChrValue row has {len(row)} columns; expected {len(FIELD_NAMES)}.")
        record = dict(zip(FIELD_NAMES, row))
        record["race"] = int(record["id"]) // 10000
        record["classId"] = int(record["id"]) % 10000
        entries.append(record)

    payload = {
        "source": args.input.name,
        "entries": entries,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Extracted {len(entries)} original KO character-start rows -> {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
