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
    TGA_UNCOMPRESSED,
    _decode_uncompressed,
    load_rgba,
    read_header,
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


def test_mislabeled_dxt_tga32_with_tga2_footer(tmp_path):
    # 2x2 uncompressed true-color TGA, bottom-left origin, 8 alpha bits.
    header = bytes(
        [
            0, 0, 2,          # no ID, no color map, image type 2
            0, 0, 0, 0, 0,   # color map spec
            0, 0, 0, 0,       # x/y origin
            2, 0, 2, 0,       # width=2, height=2
            32, 8,             # BGRA32, 8 alpha bits, bottom-left origin
        ]
    )
    # File row 0 is image bottom row because descriptor bit 5 is clear.
    # bottom-left red, bottom-right green, top-left blue, top-right white.
    pixels = bytes(
        [
            0, 0, 255, 255,
            0, 255, 0, 255,
            255, 0, 0, 255,
            255, 255, 255, 255,
        ]
    )
    footer = b"\x00" * 8 + b"TRUEVISION-XFILE.\x00"
    path = tmp_path / "actually_tga.dxt"
    path.write_bytes(header + pixels + footer)

    parsed = read_header(path)
    assert parsed.format_value == TGA_UNCOMPRESSED
    assert parsed.format_name == "TGA32"
    assert (parsed.width, parsed.height) == (2, 2)

    texture = load_rgba(path, tmp_path)
    assert texture.rgba == bytes(
        [
            0, 0, 255, 255,       # top-left blue
            255, 255, 255, 255,   # top-right white
            255, 0, 0, 255,       # bottom-left red
            0, 255, 0, 255,       # bottom-right green
        ]
    )
