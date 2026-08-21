#!/usr/bin/env python3
"""Pure parser for KO 1.298 terrain (.gtd) data used by Android validation."""
from __future__ import annotations

from dataclasses import dataclass, asdict
from pathlib import Path
import struct

MAX_PATH = 260
PATCH_TILE_SIZE = 8
MAPDATA_SIZE = 8
VERTEX_WATER_SIZE = 44


class WorldParseError(ValueError):
    pass


@dataclass
class Cursor:
    data: bytes
    offset: int = 0

    @property
    def remaining(self) -> int:
        return len(self.data) - self.offset

    def take(self, count: int) -> bytes:
        if count < 0 or self.offset + count > len(self.data):
            raise WorldParseError(f"read past EOF: offset={self.offset}, count={count}, length={len(self.data)}")
        raw = self.data[self.offset:self.offset + count]
        self.offset += count
        return raw

    def i16(self) -> int:
        return struct.unpack("<h", self.take(2))[0]

    def i32(self) -> int:
        return struct.unpack("<i", self.take(4))[0]

    def f32(self) -> float:
        return struct.unpack("<f", self.take(4))[0]


@dataclass
class MapPoint:
    height: float
    tile_full: bool
    tex1_dir: int
    tex2_dir: int
    tex1_idx: int
    tex2_idx: int


@dataclass
class TerrainTileRef:
    source_index: int
    tile_index: int


@dataclass
class RiverInfo:
    vertex_count: int
    index_count: int
    texture: str


@dataclass
class PondInfo:
    vertex_count: int
    width_vertices: int
    index_count: int
    texture: str
    wave_variance: float


@dataclass
class TerrainData:
    format_version: int
    gtd_version: int
    name: str
    map_size: int
    patch_map_size: int
    height_min: float
    height_max: float
    tile_texture_count: int
    gtt_sources: list[str]
    tile_refs: list[TerrainTileRef]
    river: list[RiverInfo]
    pond: list[PondInfo]
    byte_length: int


def decode_c_string(raw: bytes) -> str:
    raw = raw.split(b"\0", 1)[0]
    for enc in ("utf-8", "cp949", "cp1252", "latin1"):
        try:
            return raw.decode(enc)
        except UnicodeDecodeError:
            continue
    return raw.decode("latin1", errors="replace")


def decode_mapdata(raw: bytes) -> MapPoint:
    if len(raw) != 8:
        raise WorldParseError("MAPDATA must be 8 bytes")
    height, bits = struct.unpack("<fI", raw)
    return MapPoint(
        height=height,
        tile_full=bool(bits & 0x1),
        tex1_dir=(bits >> 1) & 0x1F,
        tex2_dir=(bits >> 6) & 0x1F,
        tex1_idx=(bits >> 11) & 0x3FF,
        tex2_idx=(bits >> 21) & 0x3FF,
    )


def parse_rivers(r: Cursor) -> list[RiverInfo]:
    count = r.i32()
    if count < 0 or count > 1024:
        raise WorldParseError(f"invalid river count {count}")
    result: list[RiverInfo] = []
    for _ in range(count):
        vertex_count = r.i32()
        if vertex_count <= 0 or vertex_count % 4 != 0:
            raise WorldParseError(f"invalid river vertex count {vertex_count}")
        r.take(vertex_count * VERTEX_WATER_SIZE)
        index_count = r.i32()
        if index_count < 0 or index_count % 18 != 0:
            raise WorldParseError(f"invalid river index count {index_count}")
        name_len = r.i32()
        if name_len < 0 or name_len > 50:
            raise WorldParseError(f"invalid river texture name length {name_len}")
        texture = decode_c_string(r.take(name_len)) if name_len else ""
        result.append(RiverInfo(vertex_count, index_count, texture))
    return result


def parse_ponds(r: Cursor, gtd_version: int) -> list[PondInfo]:
    count = r.i32()
    if count < 0 or count > 1024:
        raise WorldParseError(f"invalid pond count {count}")
    result: list[PondInfo] = []
    for _ in range(count):
        vertex_count = r.i32()
        if vertex_count <= 0:
            result.append(PondInfo(vertex_count, 0, 0, "", 0.2))
            continue
        width = r.i32()
        if width <= 0 or vertex_count % width != 0:
            raise WorldParseError(f"invalid pond width {width} for {vertex_count} vertices")
        name_len = r.i32()
        if name_len < 0 or name_len > 50:
            raise WorldParseError(f"invalid pond texture name length {name_len}")
        texture = decode_c_string(r.take(name_len)) if name_len else ""
        r.take(vertex_count * VERTEX_WATER_SIZE)
        wave = r.f32() if gtd_version >= 2 else 0.2
        index_count = r.i32()
        if index_count < 0:
            raise WorldParseError(f"invalid pond index count {index_count}")
        result.append(PondInfo(vertex_count, width, index_count, texture, wave))
    return result


def parse_terrain_bytes(data: bytes, file_format_version: int) -> TerrainData:
    r = Cursor(data)
    gtd_version = 0
    name = ""

    if file_format_version >= 1264:
        gtd_version = r.i32()
        if gtd_version < 0 or gtd_version > 2:
            raise WorldParseError(f"invalid GTD version {gtd_version}")
        name_len = r.i32()
        if name_len < 0 or name_len > 30:
            raise WorldParseError(f"invalid terrain name length {name_len}")
        name = decode_c_string(r.take(name_len)) if name_len else ""

    map_size = r.i32()
    if map_size <= 0 or map_size - 1 > 4096 or (map_size - 1) % 4 != 0:
        raise WorldParseError(f"invalid terrain map size {map_size}")

    point_count = map_size * map_size
    map_raw = r.take(point_count * MAPDATA_SIZE)
    heights = [struct.unpack_from("<f", map_raw, i * MAPDATA_SIZE)[0] for i in range(point_count)]

    patch_map_size = (map_size - 1) // PATCH_TILE_SIZE
    r.take(patch_map_size * patch_map_size * 8)  # middleY + radius
    r.take(point_count)                           # grass attributes
    r.take(MAX_PATH)                              # grass filename

    tile_texture_count = r.i32()
    if tile_texture_count < 0 or tile_texture_count > 4096:
        raise WorldParseError(f"invalid tile texture count {tile_texture_count}")

    sources: list[str] = []
    refs: list[TerrainTileRef] = []
    if tile_texture_count > 0:
        source_count = r.i32()
        if source_count < 0 or source_count > 1024:
            raise WorldParseError(f"invalid GTT source count {source_count}")
        for _ in range(source_count):
            sources.append(decode_c_string(r.take(MAX_PATH)))
        for _ in range(tile_texture_count):
            source_index = r.i16()
            tile_index = r.i16()
            if source_index < 0 or source_index >= source_count or tile_index < 0:
                raise WorldParseError(
                    f"invalid terrain tile ref source={source_index}/{source_count} tile={tile_index}"
                )
            refs.append(TerrainTileRef(source_index, tile_index))

    lightmap_count = r.i32()
    if lightmap_count != 0:
        raise WorldParseError(f"deprecated lightmap payload not supported by pinned client: {lightmap_count}")

    rivers = parse_rivers(r)
    ponds = parse_ponds(r, gtd_version)
    if r.remaining != 0:
        raise WorldParseError(f"terrain has {r.remaining} trailing bytes")

    return TerrainData(
        format_version=file_format_version,
        gtd_version=gtd_version,
        name=name,
        map_size=map_size,
        patch_map_size=patch_map_size,
        height_min=min(heights) if heights else 0.0,
        height_max=max(heights) if heights else 0.0,
        tile_texture_count=tile_texture_count,
        gtt_sources=sources,
        tile_refs=refs,
        river=rivers,
        pond=ponds,
        byte_length=len(data),
    )


def parse_terrain(path: Path | str) -> TerrainData:
    path = Path(path)
    data = path.read_bytes()
    errors: list[str] = []
    for version in (1264, 1098):
        try:
            return parse_terrain_bytes(data, version)
        except (WorldParseError, struct.error) as exc:
            errors.append(f"v{version}: {exc}")
    raise WorldParseError("; ".join(errors))
