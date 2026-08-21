"""Import one Knight Online .n3chr with OpenKO-blender and export it as Unity-ready FBX.

Run from Blender 4.2+ after installing/enabling the OpenKO Assets extension:

    blender --background --python tools/ko_to_unity/export_character.py -- \
        --input /path/to/character.n3chr \
        --output /path/to/unity/Assets/LegacyConverted/character.fbx

This script does not alter the source KO asset. It imports the original asset,
keeps the armature/skin/animations, then writes a separate FBX for Unity.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import sys

import bpy


def _arguments_after_double_dash() -> list[str]:
    if "--" not in sys.argv:
        return []
    return sys.argv[sys.argv.index("--") + 1 :]


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--lod", type=int, default=0, choices=(0, 1, 2, 3))
    parser.add_argument("--scale", type=float, default=1.0)
    return parser.parse_args(_arguments_after_double_dash())


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


def main() -> None:
    args = _parse_args()
    source = Path(args.input).expanduser().resolve()
    output = Path(args.output).expanduser().resolve()

    if source.suffix.lower() != ".n3chr":
        raise SystemExit("Phase-0 character exporter requires a .n3chr input.")
    if not source.is_file():
        raise SystemExit(f"KO character file not found: {source}")

    output.parent.mkdir(parents=True, exist_ok=True)
    _clear_scene()

    result = bpy.ops.import_ko.asset(
        filepath=str(source),
        lod_level=args.lod,
        scale=args.scale,
        skip_textures=False,
        skip_animations=False,
        add_lighting=False,
    )
    if "FINISHED" not in result:
        raise SystemExit(f"OpenKO import failed: {result}")

    exportable = [obj for obj in bpy.context.scene.objects if obj.type in {"MESH", "ARMATURE", "EMPTY"}]
    if not exportable:
        raise SystemExit("No character mesh/armature was produced by the KO importer.")

    bpy.ops.object.select_all(action="DESELECT")
    for obj in exportable:
        obj.hide_set(False)
        obj.hide_viewport = False
        obj.select_set(True)

    bpy.context.view_layer.objects.active = next(
        (obj for obj in exportable if obj.type == "ARMATURE"),
        exportable[0],
    )

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

    print(f"KO character exported without modifying source: {source} -> {output}")


if __name__ == "__main__":
    main()
