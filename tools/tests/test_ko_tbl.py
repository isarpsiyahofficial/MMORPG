from __future__ import annotations

import struct
import tempfile
from pathlib import Path
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1]
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from ko_tbl import (  # noqa: E402
    DT_BYTE,
    DT_DWORD,
    DT_INT,
    DT_STRING,
    KoTableError,
    decrypt_tbl,
    read_tbl,
)


def encrypt_tbl(plain: bytes) -> bytes:
    key_r = 0x0816
    key_c1 = 0x6081
    key_c2 = 0x1608
    output = bytearray(len(plain))
    for index, value in enumerate(plain):
        cipher = value ^ (key_r >> 8)
        output[index] = cipher
        key_r = ((cipher + key_r) * key_c1 + key_c2) & 0xFFFF
    return bytes(output)


def make_plain_table() -> bytes:
    types = [DT_DWORD, DT_STRING, DT_INT, DT_BYTE]
    payload = bytearray()
    payload += struct.pack("<i", len(types))
    payload += b"".join(struct.pack("<I", value) for value in types)
    payload += struct.pack("<i", 2)

    for row_id, name, number, small in (
        (11, "ElMorad", -7, 2),
        (12, "HumanMale", 42, 9),
    ):
        raw_name = name.encode("cp1252")
        payload += struct.pack("<I", row_id)
        payload += struct.pack("<i", len(raw_name)) + raw_name
        payload += struct.pack("<i", number)
        payload += struct.pack("<B", small)

    return bytes(payload)


class KoTableTests(unittest.TestCase):
    def test_decryption_matches_encryption(self):
        plain = make_plain_table()
        self.assertEqual(decrypt_tbl(encrypt_tbl(plain)), plain)

    def test_reads_encrypted_table(self):
        plain = make_plain_table()
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample.tbl"
            path.write_bytes(encrypt_tbl(plain))
            table = read_tbl(path)

        self.assertEqual(table.column_types, [DT_DWORD, DT_STRING, DT_INT, DT_BYTE])
        self.assertEqual(table.rows[0], [11, "ElMorad", -7, 2])
        self.assertEqual(table.rows[1], [12, "HumanMale", 42, 9])

    def test_reads_plain_community_table(self):
        plain = make_plain_table()
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample.tbl"
            path.write_bytes(plain)
            table = read_tbl(path)
        self.assertEqual(len(table.rows), 2)

    def test_rejects_corrupt_table(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "broken.tbl"
            path.write_bytes(b"not-a-ko-table")
            with self.assertRaises(KoTableError):
                read_tbl(path)


if __name__ == "__main__":
    unittest.main()
