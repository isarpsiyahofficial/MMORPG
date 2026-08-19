"""Build one original KO player from UPC_DefaultLooks JSON and export to Unity FBX.

Prerequisites:
- Blender 4.2+
- OpenKO Assets Blender extension installed/enabled
- SQL-free player look JSON produced by tools/extract_player_looks.py
- local KO 1.298 asset tree

Example:
    blender --background --python tools/ko_to_unity/export_player.py -- \
      --source-root /path/to/ko-assets-1298 \
      --looks-json /path/to/player_looks.json \
      --race 12 \
      --output /path/to/unity/Assets/LegacyConverted/el_male.fbx

The source KO files are read-only inputs and are never overwritten.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

import bpy


def _args_after_double_dash() -> list[str]:
    if "--" not in sys.argv:
        return []
    return sys.argv[sys.argv.index("--") + 1 :]


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--looks-json", required=True, type=Path)
    parser.add_argument("--race", required=True, type=int, choices=(1, 2, 3, 4, 11, 12, 13))
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--lod", type=int, default=0, choices=(0, 1, 2, 3))
    parser.add_argument("--scale", type=float, default=1.0)
    return parser.parse_args(_args_after_double_dash())


def _clear_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (
        bpy.data.meshes,
        bpy.data.armatures,
        bpy.data.materials,
        bpy.data.images,
        bpy.data.actions,
    ):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)


def _file_index(source_root: Path) -> dict[str, Path]:
    result: dict[str, Path] = {}
    for path in source_root.rglob("*"):
        if not path.is_file():
            continue
        relative = path.relative_to(source_root).as_posix().lower()
        result.setdefault(relative, path)
        result.setdefault(path.name.lower(), path)
    return result


def _resolve_reference(source_root: Path, index: dict[str, Path], reference: str) -> Path | None:
    reference = reference.strip().replace("\\", "/")
    if not reference:
        return None

    direct = source_root / reference
    if direct.is_file():
        return direct

    normalized = reference.lstrip("./").lower()
    if normalized in index:
        return index[normalized]

    basename = Path(reference).name.lower()
    return index.get(basename)


def _pick_lod(skins, lod: int):
    if not skins:
        return None
    if 0 <= lod < len(skins) and skins[lod] is not None:
        return skins[lod]
    return next((skin for skin in skins if skin is not None), None)


def _load_player_record(path: Path, race: int) -> dict:
    payload = json.loads(path.read_text(encoding="utf-8"))
    for record in payload.get("characters", []):
        if int(record.get("id", -1)) == race:
            return record
    raise SystemExit(f"KO player race {race} was not found in {path}")


def _export_fbx(output: Path) -> None:
    exportable = [obj for obj in bpy.context.scene.objects if obj.type in {"MESH", "ARMATURE", "EMPTY"}]
    if not exportable:
        raise SystemExit("No KO player objects were created.")

    bpy.ops.object.select_all(action="DESELECT")
    for obj in exportable:
        obj.hide_set(False)
        obj.hide_viewport = False
        obj.select_set(True)

    bpy.context.view_layer.objects.active = next((obj for obj in exportable if obj.type == "ARMATURE"), exportable[0])
    output.parent.mkdir(parents=True, exist_ok=True)

    result = bpy.ops.export_scene.fbx(
        filepath=str(output),
        use_selection=True,
        object_types={"ARMATURE", "EMPTY", "MESH"},
        axis_forward="-Z",
        axis_up="Y",
        apply_unit_scale=True,
        global_scale=1.0,
        add_leaf_bones=False,
        use_armature_deform_only=False,
        bake_anim=True,
        bake_anim_use_all_bones=True,
        bake_anim_use_nla_strips=False,
        bake_anim_use_all_actions=True,
        bake_anim_force_startend_keying=True,
        bake_anim_step=1.0,
        bake_anim_simplify_factor=0.0,
        path_mode="COPY",
        embed_textures=True,
    )
    if "FINISHED" not in result:
        raise SystemExit(f"FBX export failed: {result}")


def main() -> None:
    args = _parse_args()
    source_root = args.source_root.expanduser().resolve()
    looks_json = args.looks_json.expanduser().resolve()
    output = args.output.expanduser().resolve()

    if not source_root.is_dir():
        raise SystemExit(f"KO source root not found: {source_root}")
    if not looks_json.is_file():
        raise SystemExit(f"Player looks JSON not found: {looks_json}")

    from openko_blender.blender import armature_builder, material_builder, mesh_builder
    from openko_blender.formats import n3anim, n3cpart, n3joint

    record = _load_player_record(looks_json, args.race)
    index = _file_index(source_root)

    joint_path = _resolve_reference(source_root, index, str(record.get("jointFile", "")))
    anim_path = _resolve_reference(source_root, index, str(record.get("animationFile", "")))
    if joint_path is None:
        raise SystemExit(f"KO skeleton could not be resolved: {record.get('jointFile')!r}")
    if anim_path is None:
        raise SystemExit(f"KO animation file could not be resolved: {record.get('animationFile')!r}")

    _clear_scene()

    player_name = str(record.get("name") or f"KO_Player_{args.race}")
    player_collection = bpy.data.collections.new(player_name)
    bpy.context.scene.collection.children.link(player_collection)

    root_joint = n3joint.load(joint_path)
    arm_data = armature_builder.build_armature(bpy.context, root_joint, player_name, player_collection)
    if args.scale != 1.0:
        arm_data.rig.scale = (args.scale, args.scale, args.scale)

    part_keys = (
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
    )

    imported_parts = 0
    for part_key in part_keys:
        reference = str(record.get(part_key, "") or "")
        part_path = _resolve_reference(source_root, index, reference)
        if not reference:
            continue
        if part_path is None:
            raise SystemExit(f"KO player part could not be resolved: {part_key}={reference!r}")
        if part_path.suffix.lower() != ".n3cpart":
            raise SystemExit(f"Expected .n3cpart for {part_key}, found: {part_path}")

        part = n3cpart.load(part_path)
        skin = _pick_lod(part.skins, args.lod)
        if skin is None:
            raise SystemExit(f"KO player part has no usable skin: {part_path}")

        object_name = skin.name or part.name or part_path.stem
        obj = mesh_builder.build_skinned_mesh(skin, object_name)
        if args.scale != 1.0:
            obj.scale = (args.scale, args.scale, args.scale)

        mesh_builder.apply_skin_weights(obj, skin, arm_data.all_joints_by_idx)
        mesh_builder.add_armature_modifier(obj, arm_data.rig)
        player_collection.objects.link(obj)

        if part.tex_filename:
            image = material_builder.resolve_and_load_texture(part.tex_filename, part_path, object_name)
            material = material_builder.create_material(object_name, image, part.material)
            material_builder.apply_material(obj, material)

        imported_parts += 1

    if imported_parts == 0:
        raise SystemExit("KO player definition did not produce any body parts.")

    anim_control = n3anim.load(anim_path)
    if not anim_control.animations:
        raise SystemExit(f"KO animation file has no animations: {anim_path}")
    armature_builder.build_animations(bpy.context, arm_data, root_joint, anim_control)

    _export_fbx(output)
    print(
        f"KO player race {args.race} exported with {imported_parts} original parts and "
        f"{len(anim_control.animations)} animations: {output}"
    )


if __name__ == "__main__":
    main()
