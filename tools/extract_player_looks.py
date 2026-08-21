#!/usr/bin/env python3
"""Extract the original KO player appearance rows from UPC_DefaultLooks.tbl.

Output is a SQL-free JSON description used to assemble the Unity player from the
same skeleton, animation, body, face, hair and attachment references as KO 1.298.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

from ko_tbl import DT_BYTE, DT_DWORD, DT_INT, DT_STRING, KoTableError, read_tbl

PLAYER_RACE_IDS = {1, 2, 3, 4, 11, 12, 13}
EXPECTED_TYPES = [DT_DWORD] + [DT_STRING] * 16 + [DT_INT] * 18 + [DT_BYTE] * 3

FIELD_NAMES = [
    "id",
    "name",
    "jointFile",
    "animationFile",
    "partUpper",
    "partLower",
    "partFace",
    "partHands",
    "partFeet",
    "partHairHelmet",
    "part6",
    "part7",
    "part8",
    "part9",
    "skinFile",
    "characterFile",
    "fxPlugFile",
    "unknown1",
    "rightHandJoint",
    "leftHandJoint",
    "leftForearmJoint",
    "cloakJoint",
    "soundMove",
    "soundAttack0",
    "soundAttack1",
    "soundStruck0",
    "soundStruck1",
    "soundDead0",
    "soundDead1",
    "soundBreathe0",
    "soundBreathe1",
    "soundReserved0",
    "soundReserved1",
    "unknown2",
    "unknown3",
    "unknownByte4",
    "unknownByte5",
    "unknownByte6",
]


def row_to_dict(row: list[object]) -> dict[str, object]:
    if len(row) != len(FIELD_NAMES):
        raise KoTableError(f"UPC_DefaultLooks row has {len(row)} columns; expected {len(FIELD_NAMES)}.")
    return dict(zip(FIELD_NAMES, row))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Extract KO 1.298 player appearance definitions without SQL.")
    parser.add_argument("input", type=Path, help="UPC_DefaultLooks.tbl")
    parser.add_argument("output", type=Path, help="Output JSON")
    parser.add_argument("--all", action="store_true", help="Keep non-player rows too if the table contains any.")
    parser.add_argument("--encoding", default="cp1252")
    args = parser.parse_args(argv)

    table = read_tbl(args.input, args.encoding)
    if table.column_types != EXPECTED_TYPES:
        raise KoTableError(
            "UPC_DefaultLooks.tbl schema does not match the pinned KO 1.298 player-look structure. "
            f"Found {table.column_types!r}"
        )

    records = []
    for row in table.rows:
        record = row_to_dict(row)
        if args.all or int(record["id"]) in PLAYER_RACE_IDS:
            records.append(record)

    found_ids = {int(record["id"]) for record in records}
    if not args.all:
        missing = sorted(PLAYER_RACE_IDS - found_ids)
        if missing:
            raise KoTableError(f"Missing expected KO player race rows: {missing}")

    payload = {
        "source": args.input.name,
        "playerRaceIds": sorted(found_ids),
        "characters": records,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Extracted {len(records)} KO player appearance definitions -> {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
