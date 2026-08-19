#!/usr/bin/env python3
"""Read legacy Knight Online .tbl files without SQL or the original client.

The format and decryption routine mirror OpenKO 1.298's CN3TableBase loader.
This tool never writes to the source .tbl file.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import struct
import sys
from typing import Any

DT_NONE = 0
DT_CHAR = 1
DT_BYTE = 2
DT_SHORT = 3
DT_WORD = 4
DT_INT = 5
DT_DWORD = 6
DT_STRING = 7
DT_FLOAT = 8
DT_DOUBLE = 9

VALID_TYPES = {DT_CHAR, DT_BYTE, DT_SHORT, DT_WORD, DT_INT, DT_DWORD, DT_STRING, DT_FLOAT, DT_DOUBLE}


@dataclass(frozen=True)
class KoTable:
    column_types: list[int]
    rows: list[list[Any]]


class KoTableError(ValueError):
    pass


def decrypt_tbl(data: bytes) -> bytes:
    key_r = 0x0816
    key_c1 = 0x6081
    key_c2 = 0x1608
    output = bytearray(len(data))

    for index, cipher in enumerate(data):
        output[index] = cipher ^ (key_r >> 8)
        key_r = ((cipher + key_r) * key_c1 + key_c2) & 0xFFFF

    return bytes(output)


def _read_exact(data: bytes, offset: int, size: int) -> tuple[bytes, int]:
    end = offset + size
    if end > len(data):
        raise KoTableError("Unexpected end of TBL data.")
    return data[offset:end], end


def _read_struct(data: bytes, offset: int, fmt: str) -> tuple[Any, int]:
    size = struct.calcsize(fmt)
    raw, offset = _read_exact(data, offset, size)
    return struct.unpack(fmt, raw)[0], offset


def _decode_string(raw: bytes, encoding: str) -> str:
    try:
        return raw.decode(encoding)
    except UnicodeDecodeError:
        try:
            return raw.decode("utf-8")
        except UnicodeDecodeError:
            return raw.decode("latin-1")


def _read_value(data: bytes, offset: int, data_type: int, encoding: str) -> tuple[Any, int]:
    if data_type == DT_CHAR:
        return _read_struct(data, offset, "<b")
    if data_type == DT_BYTE:
        return _read_struct(data, offset, "<B")
    if data_type == DT_SHORT:
        return _read_struct(data, offset, "<h")
    if data_type == DT_WORD:
        return _read_struct(data, offset, "<H")
    if data_type == DT_INT:
        return _read_struct(data, offset, "<i")
    if data_type == DT_DWORD:
        return _read_struct(data, offset, "<I")
    if data_type == DT_FLOAT:
        return _read_struct(data, offset, "<f")
    if data_type == DT_DOUBLE:
        return _read_struct(data, offset, "<d")
    if data_type == DT_STRING:
        length, offset = _read_struct(data, offset, "<i")
        if length < 0 or length > 16 * 1024 * 1024:
            raise KoTableError(f"Invalid TBL string length: {length}")
        raw, offset = _read_exact(data, offset, length)
        return _decode_string(raw, encoding), offset
    raise KoTableError(f"Unsupported TBL data type: {data_type}")


def parse_plain_tbl(data: bytes, encoding: str = "cp1252") -> KoTable:
    offset = 0
    column_count, offset = _read_struct(data, offset, "<i")
    if column_count <= 0 or column_count > 4096:
        raise KoTableError(f"Invalid TBL column count: {column_count}")

    column_types = []
    for _ in range(column_count):
        data_type, offset = _read_struct(data, offset, "<I")
        if data_type not in VALID_TYPES:
            raise KoTableError(f"Invalid TBL data type: {data_type}")
        column_types.append(data_type)

    if column_types[0] != DT_DWORD:
        raise KoTableError("TBL first column must be DT_DWORD.")

    row_count, offset = _read_struct(data, offset, "<i")
    if row_count < 0 or row_count > 10_000_000:
        raise KoTableError(f"Invalid TBL row count: {row_count}")

    rows: list[list[Any]] = []
    for _ in range(row_count):
        row = []
        for data_type in column_types:
            value, offset = _read_value(data, offset, data_type, encoding)
            row.append(value)
        rows.append(row)

    if offset != len(data):
        trailing = len(data) - offset
        if trailing > 0:
            raise KoTableError(f"TBL has {trailing} unexpected trailing bytes.")

    return KoTable(column_types=column_types, rows=rows)


def read_tbl(path: Path, encoding: str = "cp1252") -> KoTable:
    encrypted = path.read_bytes()
    if not encrypted:
        raise KoTableError("TBL file is empty.")

    decrypted = decrypt_tbl(encrypted)
    try:
        return parse_plain_tbl(decrypted, encoding)
    except KoTableError as decrypted_error:
        # Some community-produced tables may already be plain. Accept those too,
        # but only if they pass the same strict parser.
        try:
            return parse_plain_tbl(encrypted, encoding)
        except KoTableError:
            raise decrypted_error


def to_json_dict(table: KoTable) -> dict[str, Any]:
    return {
        "columnTypes": table.column_types,
        "rowCount": len(table.rows),
        "rows": table.rows,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Decrypt and read a Knight Online .tbl file without SQL.")
    parser.add_argument("input", type=Path)
    parser.add_argument("--json", dest="json_output", type=Path)
    parser.add_argument("--encoding", default="cp1252")
    args = parser.parse_args(argv)

    if not args.input.is_file():
        parser.error(f"TBL file not found: {args.input}")

    table = read_tbl(args.input, args.encoding)
    payload = to_json_dict(table)

    if args.json_output:
        args.json_output.parent.mkdir(parents=True, exist_ok=True)
        args.json_output.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
        print(f"Read {len(table.rows)} rows -> {args.json_output}")
    else:
        print(json.dumps(payload, ensure_ascii=False, indent=2))

    return 0


if __name__ == "__main__":
    sys.exit(main())
