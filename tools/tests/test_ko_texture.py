from pathlib import Path
import struct
import sys

TOOLS = Path(__file__).resolve().parents[1]
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from ko_texture import (  # noqa: E402
    D3DFMT_A1R5G5B5,
    D3DFMT_A4R4G4B4,
    D3DFMT_A8R8G8B8,
    D3DFMT_R8G8B8,
    D3DFMT_X8R8G8B8,
    _decode_uncompressed,
)


def test_r8g8b8_bgr_memory_order():
    assert _decode_uncompressed(bytes([3, 2, 1]), 1, 1, D3DFMT_R8G8B8) == bytes([1, 2, 3, 255])


def test_a8r8g8b8_bgra_memory_order():
    assert _decode_uncompressed(bytes([3, 2, 1, 4]), 1, 1, D3DFMT_A8R8G8B8) == bytes([1, 2, 3, 4])


def test_x8r8g8b8_forces_opaque_alpha():
    assert _decode_uncompressed(bytes([3, 2, 1, 0]), 1, 1, D3DFMT_X8R8G8B8) == bytes([1, 2, 3, 255])


def test_a1r5g5b5():
    packed = 0x8000 | (31 << 10) | (16 << 5) | 1
    rgba = _decode_uncompressed(struct.pack("<H", packed), 1, 1, D3DFMT_A1R5G5B5)
    assert rgba == bytes([255, 132, 8, 255])


def test_a4r4g4b4():
    packed = (0xA << 12) | (0xB << 8) | (0xC << 4) | 0xD
    rgba = _decode_uncompressed(struct.pack("<H", packed), 1, 1, D3DFMT_A4R4G4B4)
    assert rgba == bytes([0xBB, 0xCC, 0xDD, 0xAA])
