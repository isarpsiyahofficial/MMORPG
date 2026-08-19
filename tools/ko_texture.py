#!/usr/bin/env python3
"""Read Knight Online CN3Texture (.dxt) files into RGBA without Direct3D.

The uncompressed format handling follows the pinned OpenKO CN3Texture loader.
Compressed DXT1-DXT5 data is delegated to the pinned OpenKO-blender decoder.
Only the top mip is returned because Unity regenerates/platform-compresses its
own Android texture representation from the lossless PNG conversion output.
"""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import struct
import sys

D3DFMT_R8G8B8 = 20
D3DFMT_A8R8G8B8 = 21
D3DFMT_X8R8G8B8 = 22
D3DFMT_A1R5G5B5 = 25
D3DFMT_A4R4G4B4 = 26
D3DFMT_DXT1 = 0x31545844
D3DFMT_DXT2 = 0x32545844
D3DFMT_DXT3 = 0x33545844
D3DFMT_DXT4 = 0x34545844
D3DFMT_DXT5 = 0x35545844

COMPRESSED_FORMATS = {
    D3DFMT_DXT1: "DXT1",
    D3DFMT_DXT2: "DXT2",
    D3DFMT_DXT3: "DXT3",
    D3DFMT_DXT4: "DXT4",
    D3DFMT_DXT5: "DXT5",
}
UNCOMPRESSED_FORMATS = {
    D3DFMT_R8G8B8: ("R8G8B8", 3),
    D3DFMT_A8R8G8B8: ("A8R8G8B8", 4),
    D3DFMT_X8R8G8B8: ("X8R8G8B8", 4),
    D3DFMT_A1R5G5B5: ("A1R5G5B5", 2),
    D3DFMT_A4R4G4B4: ("A4R4G4B4", 2),
}


class KoTextureError(ValueError):
    pass


@dataclass(frozen=True)
class KoTextureHeader:
    name: str
    identifier: bytes
    width: int
    height: int
    format_value: int
    format_name: str
    has_mipmap: bool
    payload_offset: int


@dataclass(frozen=True)
class KoTextureRgba:
    header: KoTextureHeader
    rgba: bytes


def _decode_name(raw: bytes) -> str:
    for encoding in ("utf-8", "cp949", "cp1252", "latin-1"):
        try:
            return raw.decode(encoding)
        except UnicodeDecodeError:
            continue
    return raw.decode("latin-1", errors="replace")


def read_header(path: Path | str) -> KoTextureHeader:
    path = Path(path)
    data = path.read_bytes()
    if len(data) < 24:
        raise KoTextureError(f"Texture is too short: {path}")

    name_length = struct.unpack_from("<i", data, 0)[0]
    if name_length < 0 or name_length > 4096:
        raise KoTextureError(f"Invalid CN3BaseFileAccess name length {name_length}: {path.name}")

    header_offset = 4 + name_length
    if header_offset + 20 > len(data):
        raise KoTextureError(f"Texture header is truncated: {path.name}")

    name = _decode_name(data[4:header_offset])
    identifier = data[header_offset : header_offset + 4]
    width, height, format_value, mipmap_value = struct.unpack_from("<4i", data, header_offset + 4)

    if identifier[:3] != b"NTF":
        raise KoTextureError(
            f"Unsupported texture header {identifier!r}; expected Noah Texture File (NTF): {path.name}"
        )
    if identifier[3] == 7:
        raise KoTextureError(
            f"Encrypted NTF7 texture requires the original crypt key path and is not silently decoded: {path.name}"
        )
    if width <= 0 or height <= 0 or width > 16384 or height > 16384:
        raise KoTextureError(f"Invalid texture dimensions {width}x{height}: {path.name}")

    if format_value in COMPRESSED_FORMATS:
        format_name = COMPRESSED_FORMATS[format_value]
    elif format_value in UNCOMPRESSED_FORMATS:
        format_name = UNCOMPRESSED_FORMATS[format_value][0]
    else:
        raise KoTextureError(f"Unsupported Direct3D texture format {format_value}: {path.name}")

    return KoTextureHeader(
        name=name,
        identifier=identifier,
        width=width,
        height=height,
        format_value=format_value,
        format_name=format_name,
        has_mipmap=bool(mipmap_value),
        payload_offset=header_offset + 20,
    )


def load_rgba(path: Path | str, vendor_root: Path | str) -> KoTextureRgba:
    path = Path(path)
    header = read_header(path)

    if header.format_value in COMPRESSED_FORMATS:
        rgba = _load_compressed(path, Path(vendor_root))
    else:
        data = path.read_bytes()
        _, bytes_per_pixel = UNCOMPRESSED_FORMATS[header.format_value]
        byte_count = header.width * header.height * bytes_per_pixel
        end = header.payload_offset + byte_count
        if end > len(data):
            raise KoTextureError(
                f"Texture payload is truncated: need {byte_count} bytes from {header.payload_offset}, "
                f"file has {len(data)} bytes ({path.name})"
            )
        rgba = _decode_uncompressed(
            data[header.payload_offset:end],
            header.width,
            header.height,
            header.format_value,
        )

    expected = header.width * header.height * 4
    if len(rgba) != expected:
        raise KoTextureError(
            f"RGBA length mismatch for {path.name}: {len(rgba)} != {expected}"
        )
    return KoTextureRgba(header=header, rgba=rgba)


def _load_compressed(path: Path, vendor_root: Path) -> bytes:
    vendor = vendor_root / "OpenKO-blender"
    if not vendor.is_dir():
        raise KoTextureError(f"OpenKO-blender vendor source not found: {vendor}")

    vendor_text = str(vendor)
    if vendor_text not in sys.path:
        sys.path.insert(0, vendor_text)
    from openko_blender.formats import dxt_texture  # type: ignore

    texture = dxt_texture.load(path)
    return dxt_texture.decompress_to_rgba(texture)


def _decode_uncompressed(raw: bytes, width: int, height: int, format_value: int) -> bytes:
    pixel_count = width * height
    out = bytearray(pixel_count * 4)

    if format_value == D3DFMT_R8G8B8:
        for index in range(pixel_count):
            base = index * 3
            b, g, r = raw[base : base + 3]
            _write_rgba(out, index, r, g, b, 255)
        return bytes(out)

    if format_value in (D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8):
        for index in range(pixel_count):
            base = index * 4
            b, g, r, a_or_x = raw[base : base + 4]
            alpha = a_or_x if format_value == D3DFMT_A8R8G8B8 else 255
            _write_rgba(out, index, r, g, b, alpha)
        return bytes(out)

    if format_value == D3DFMT_A1R5G5B5:
        for index in range(pixel_count):
            value = struct.unpack_from("<H", raw, index * 2)[0]
            a = 255 if value & 0x8000 else 0
            r = _expand5((value >> 10) & 0x1F)
            g = _expand5((value >> 5) & 0x1F)
            b = _expand5(value & 0x1F)
            _write_rgba(out, index, r, g, b, a)
        return bytes(out)

    if format_value == D3DFMT_A4R4G4B4:
        for index in range(pixel_count):
            value = struct.unpack_from("<H", raw, index * 2)[0]
            a = ((value >> 12) & 0x0F) * 17
            r = ((value >> 8) & 0x0F) * 17
            g = ((value >> 4) & 0x0F) * 17
            b = (value & 0x0F) * 17
            _write_rgba(out, index, r, g, b, a)
        return bytes(out)

    raise KoTextureError(f"Decoder called with unsupported format {format_value}")


def _expand5(value: int) -> int:
    return (value << 3) | (value >> 2)


def _write_rgba(out: bytearray, index: int, r: int, g: int, b: int, a: int) -> None:
    base = index * 4
    out[base : base + 4] = bytes((r, g, b, a))
