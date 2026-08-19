#!/usr/bin/env python3
"""Convert Knight Online N3 UI (.uif) files into deterministic Unity JSON.

The binary layout mirrors the pinned OpenKO 7d6cf810 implementation:
CN3UIBase plus the Image/String/Button/Static/Edit/Area subclasses used by
CharacterCreate. Unsupported UI types fail loudly instead of being skipped.
"""
from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path
import struct
import sys

UI_TYPE_NAMES = {
    0: "base",
    1: "button",
    2: "static",
    3: "progress",
    4: "image",
    5: "scrollbar",
    6: "string",
    7: "trackbar",
    8: "edit",
    9: "area",
    10: "tooltip",
    11: "icon",
    12: "icon-manager",
    13: "iconslot",
    14: "list",
}
SUPPORTED = {0, 1, 2, 4, 6, 8, 9}
REQUIRED_CHARACTER_CREATE_IDS = {
    "edit_name",
    "area_character",
    "btn_create",
    "btn_face_left",
    "btn_face_right",
    "btn_hair_left",
    "btn_hair_right",
    "text_bonus",
}


class ParseError(RuntimeError):
    pass


@dataclass
class Reader:
    data: bytes
    offset: int = 0

    def take(self, size: int) -> bytes:
        if size < 0 or self.offset + size > len(self.data):
            raise ParseError(f"read past EOF at {self.offset} (+{size})")
        chunk = self.data[self.offset : self.offset + size]
        self.offset += size
        return chunk

    def i16(self) -> int:
        return struct.unpack("<h", self.take(2))[0]

    def i32(self) -> int:
        return struct.unpack("<i", self.take(4))[0]

    def u32(self) -> int:
        return struct.unpack("<I", self.take(4))[0]

    def f32(self) -> float:
        return struct.unpack("<f", self.take(4))[0]

    def rect(self) -> list[int]:
        return list(struct.unpack("<4i", self.take(16)))

    def float_rect(self) -> list[float]:
        return list(struct.unpack("<4f", self.take(16)))

    def string_with_i32_length(self, *, limit: int = 8192) -> str:
        length = self.i32()
        if length < 0 or length > limit:
            raise ParseError(f"invalid string length {length} at {self.offset - 4}")
        return decode_legacy(self.take(length)) if length else ""


def decode_legacy(raw: bytes) -> str:
    if not raw:
        return ""
    for encoding in ("utf-8", "cp949", "cp1252", "latin1"):
        try:
            return raw.decode(encoding)
        except UnicodeDecodeError:
            pass
    return raw.decode("latin1", errors="replace")


def parse_base(reader: Reader, ui_type: int, version: int) -> dict:
    name = reader.string_with_i32_length(limit=256)

    if version >= 1264:
        child_count = reader.i16()
        reader.i16()  # legacy reserved/idk0
    else:
        child_count = reader.i32()

    if child_count < 0 or child_count > 4096:
        raise ParseError(f"invalid child count {child_count}")

    children = []
    for _ in range(child_count):
        child_type = reader.u32()
        if child_type not in UI_TYPE_NAMES:
            raise ParseError(f"invalid UI type {child_type} at {reader.offset - 4}")
        if child_type not in SUPPORTED:
            raise ParseError(
                f"UI type {child_type} ({UI_TYPE_NAMES[child_type]}) is not ported yet"
            )
        children.append(parse_node(reader, child_type, version))

    ui_id = reader.string_with_i32_length(limit=128)
    region = reader.rect()
    movable = reader.rect()
    style = reader.u32()
    reserved = reader.u32()
    tooltip = reader.string_with_i32_length(limit=1024)
    sound_open = reader.string_with_i32_length(limit=1024)
    sound_close = reader.string_with_i32_length(limit=1024)

    return {
        "type": UI_TYPE_NAMES[ui_type],
        "typeId": ui_type,
        "name": name,
        "id": ui_id,
        "region": region,
        "movable": movable,
        "style": style,
        "reserved": reserved,
        "tooltip": tooltip,
        "soundOpen": sound_open,
        "soundClose": sound_close,
        "children": children,
    }


def parse_node(reader: Reader, ui_type: int, version: int) -> dict:
    node = parse_base(reader, ui_type, version)

    if ui_type == 4:  # CN3UIImage
        node["texture"] = reader.string_with_i32_length(limit=1024)
        node["uv"] = reader.float_rect()
        node["animationFps"] = reader.f32()

    elif ui_type == 6:  # CN3UIString
        font_name = reader.string_with_i32_length(limit=32)
        font_height = 0
        font_flags = 0
        if font_name:
            font_height = reader.u32()
            font_flags = reader.u32()
        node["fontName"] = font_name
        node["fontHeight"] = font_height
        node["fontFlags"] = font_flags
        node["color"] = reader.u32()
        node["text"] = reader.string_with_i32_length(limit=8192)
        node["lineSpacing"] = reader.i32() if version >= 1264 else 0

    elif ui_type == 1:  # CN3UIButton
        node["clickRegion"] = reader.rect()
        node["soundOn"] = reader.string_with_i32_length(limit=1024)
        node["soundClick"] = reader.string_with_i32_length(limit=1024)

    elif ui_type == 2:  # CN3UIStatic
        node["soundClick"] = reader.string_with_i32_length(limit=1024)

    elif ui_type == 8:  # CN3UIEdit -> CN3UIStatic::Load + typing sound
        node["soundClick"] = reader.string_with_i32_length(limit=1024)
        node["soundTyping"] = reader.string_with_i32_length(limit=1024)

    elif ui_type == 9:  # CN3UIArea
        node["areaType"] = reader.i32()

    return node


def collect_ids(node: dict) -> set[str]:
    result = {str(node.get("id", ""))} if node.get("id") else set()
    for child in node.get("children", []):
        result.update(collect_ids(child))
    return result


def collect_textures(node: dict) -> set[str]:
    result: set[str] = set()
    texture = str(node.get("texture", "") or "")
    if texture:
        result.add(texture.replace("\\", "/"))
    for child in node.get("children", []):
        result.update(collect_textures(child))
    return result


def parse_file(path: Path) -> dict:
    raw = path.read_bytes()
    errors: list[str] = []
    for version in (1264, 1098):
        reader = Reader(raw)
        try:
            root = parse_node(reader, 0, version)
            trailing = len(raw) - reader.offset
            if trailing != 0:
                raise ParseError(f"{trailing} trailing bytes remain")
            return {
                "schema": 1,
                "formatVersion": version,
                "sourceFile": path.name,
                "byteLength": len(raw),
                "textures": sorted(collect_textures(root)),
                "root": root,
            }
        except (ParseError, struct.error) as exc:
            errors.append(f"v{version}: {exc}")
    raise ParseError("; ".join(errors))


def find_character_create(source_root: Path) -> tuple[Path, dict]:
    candidates = sorted(source_root.rglob("*.uif"), key=lambda p: p.as_posix().lower())
    parsed_count = 0
    failures: list[str] = []
    for path in candidates:
        try:
            payload = parse_file(path)
            parsed_count += 1
        except ParseError as exc:
            failures.append(f"{path.relative_to(source_root)}: {exc}")
            continue

        ids = collect_ids(payload["root"])
        if REQUIRED_CHARACTER_CREATE_IDS.issubset(ids):
            payload["sourcePath"] = path.relative_to(source_root).as_posix()
            payload["ids"] = sorted(ids)
            return path, payload

    detail = "\n".join(failures[:20])
    raise SystemExit(
        f"CharacterCreate UIF was not found. parsed={parsed_count}, failures={len(failures)}\n{detail}"
    )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--file", type=Path)
    parser.add_argument("--discover-character-create", action="store_true")
    args = parser.parse_args(argv)

    source_root = args.source_root.resolve()
    if not source_root.is_dir():
        raise SystemExit(f"KO source root not found: {source_root}")

    if args.discover_character_create:
        path, payload = find_character_create(source_root)
        print(f"CharacterCreate UIF: {path.relative_to(source_root)}")
    elif args.file:
        path = args.file if args.file.is_absolute() else source_root / args.file
        payload = parse_file(path)
        payload["sourcePath"] = path.relative_to(source_root).as_posix()
        payload["ids"] = sorted(collect_ids(payload["root"]))
    else:
        parser.error("use --file or --discover-character-create")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"UIF converted: {args.output}")
    print(f"elements={len(payload['ids'])} textures={len(payload['textures'])}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ParseError as exc:
        print(f"UIF parse failed: {exc}", file=sys.stderr)
        sys.exit(2)
