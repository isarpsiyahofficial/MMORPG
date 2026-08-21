#!/usr/bin/env python3
"""Convert Knight Online N3 UI (.uif) files into deterministic Android JSON.

The binary layout mirrors pinned OpenKO 7d6cf810. Every source UIF can be
batch parsed; unsupported/variant layouts fail loudly and are never silently
accepted. Converted JSON keeps original IDs, rectangles, styles, textures and
child hierarchy so Unity can render the same UI structure with touch input.
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
SUPPORTED = set(UI_TYPE_NAMES)
IMAGE_SERIALIZED_TYPES = {4, 11}
STATIC_SERIALIZED_TYPES = {2, 10}
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
        children.append(parse_node(reader, child_type, version))

    ui_id = reader.string_with_i32_length(limit=128)
    region = reader.rect()
    movable = reader.rect()
    style = reader.u32()
    reserved = reader.u32()
    tooltip = reader.string_with_i32_length(limit=4096)
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


def read_font(reader: Reader, *, include_color: bool, include_flags: bool) -> dict:
    font_name = reader.string_with_i32_length(limit=32)
    payload: dict[str, object] = {
        "fontName": font_name,
        "fontHeight": 0,
        "fontFlags": 0,
    }
    if font_name:
        payload["fontHeight"] = reader.u32()
        if include_color:
            payload["color"] = reader.u32()
        if include_flags:
            bold = reader.i32()
            italic = reader.i32()
            payload["fontBold"] = bool(bold)
            payload["fontItalic"] = bool(italic)
            payload["fontFlags"] = (1 if bold else 0) | (2 if italic else 0)
    return payload


def parse_node(reader: Reader, ui_type: int, version: int) -> dict:
    node = parse_base(reader, ui_type, version)

    # CN3UIImage and client CN3UIIcon (inherits CN3UIImage without Load override).
    if ui_type in IMAGE_SERIALIZED_TYPES:
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
        node["text"] = reader.string_with_i32_length(limit=65536)
        node["lineSpacing"] = reader.i32() if version >= 1264 else 0

    elif ui_type == 1:  # CN3UIButton
        node["clickRegion"] = reader.rect()
        node["soundOn"] = reader.string_with_i32_length(limit=1024)
        node["soundClick"] = reader.string_with_i32_length(limit=1024)

    elif ui_type in STATIC_SERIALIZED_TYPES:  # CN3UIStatic / CN3UITooltip
        node["soundClick"] = reader.string_with_i32_length(limit=1024)

    elif ui_type == 8:  # CN3UIEdit -> CN3UIStatic::Load + typing sound
        node["soundClick"] = reader.string_with_i32_length(limit=1024)
        node["soundTyping"] = reader.string_with_i32_length(limit=1024)

    elif ui_type == 9:  # CN3UIArea
        node["areaType"] = reader.i32()

    elif ui_type == 14:  # CN3UIList
        font_name = reader.string_with_i32_length(limit=32)
        node["fontName"] = font_name
        node["fontHeight"] = 0
        node["color"] = 0xFFFFFFFF
        node["fontBold"] = False
        node["fontItalic"] = False
        if font_name:
            node["fontHeight"] = reader.u32()
            node["color"] = reader.u32()
            node["fontBold"] = bool(reader.i32())
            node["fontItalic"] = bool(reader.i32())

    # Progress, ScrollBar and TrackBar serialize only CN3UIBase; their child
    # reserved values define foreground/background/thumb/button roles.
    # IconManager/IconSlot in 1.298 are treated as base containers first; the
    # full-tree batch gate proves whether any source instance has extra bytes.
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


def collect_type_counts(node: dict, counts: dict[str, int] | None = None) -> dict[str, int]:
    if counts is None:
        counts = {}
    type_name = str(node.get("type", "unknown"))
    counts[type_name] = counts.get(type_name, 0) + 1
    for child in node.get("children", []):
        collect_type_counts(child, counts)
    return counts


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
                "schema": 2,
                "formatVersion": version,
                "sourceFile": path.name,
                "byteLength": len(raw),
                "textures": sorted(collect_textures(root)),
                "ids": sorted(collect_ids(root)),
                "typeCounts": collect_type_counts(root),
                "root": root,
            }
        except (ParseError, struct.error, UnicodeError) as exc:
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


def convert_all(source_root: Path, output_root: Path, manifest_path: Path) -> int:
    files = sorted(source_root.rglob("*.uif"), key=lambda p: p.relative_to(source_root).as_posix().lower())
    records: list[dict[str, object]] = []
    failures = 0
    total_elements = 0
    global_type_counts: dict[str, int] = {}

    if output_root.exists():
        import shutil
        shutil.rmtree(output_root)
    output_root.mkdir(parents=True, exist_ok=True)

    for path in files:
        relative = path.relative_to(source_root)
        destination = output_root / relative.with_suffix(".json")
        record: dict[str, object] = {"sourcePath": relative.as_posix()}
        try:
            payload = parse_file(path)
            payload["sourcePath"] = relative.as_posix()
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_text(json.dumps(payload, separators=(",", ":"), ensure_ascii=False), encoding="utf-8")
            record.update(
                status="converted",
                outputPath=str(destination),
                formatVersion=payload["formatVersion"],
                elements=len(payload["ids"]),
                textures=len(payload["textures"]),
                typeCounts=payload["typeCounts"],
            )
            total_elements += len(payload["ids"])
            for name, count in payload["typeCounts"].items():
                global_type_counts[name] = global_type_counts.get(name, 0) + int(count)
        except Exception as exc:
            record.update(status="failed", error=f"{type(exc).__name__}: {exc}")
            failures += 1
        records.append(record)

    manifest = {
        "schema": 1,
        "sourceUifFiles": len(files),
        "converted": len(files) - failures,
        "failed": failures,
        "totalElements": total_elements,
        "typeCounts": dict(sorted(global_type_counts.items())),
        "files": records,
    }
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")

    print(
        f"FULL UIF CONVERSION: converted={len(files) - failures} failed={failures} "
        f"total={len(files)} elements={total_elements}"
    )
    for record in records:
        if record["status"] == "failed":
            print("UIF FAILED", record["sourcePath"], record.get("error", ""))
    return 4 if failures else 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--file", type=Path)
    parser.add_argument("--discover-character-create", action="store_true")
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--output-root", type=Path)
    parser.add_argument("--manifest", type=Path)
    args = parser.parse_args(argv)

    source_root = args.source_root.resolve()
    if not source_root.is_dir():
        raise SystemExit(f"KO source root not found: {source_root}")

    if args.all:
        if args.output_root is None or args.manifest is None:
            parser.error("--all requires --output-root and --manifest")
        return convert_all(source_root, args.output_root.resolve(), args.manifest.resolve())

    if args.output is None:
        parser.error("single UIF conversion requires --output")

    if args.discover_character_create:
        path, payload = find_character_create(source_root)
        print(f"CharacterCreate UIF: {path.relative_to(source_root)}")
    elif args.file:
        path = args.file if args.file.is_absolute() else source_root / args.file
        payload = parse_file(path)
        payload["sourcePath"] = path.relative_to(source_root).as_posix()
    else:
        parser.error("use --file, --discover-character-create, or --all")

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
