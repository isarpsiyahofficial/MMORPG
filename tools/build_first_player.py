#!/usr/bin/env python3
"""Convert the first original KO player into the Unity project.

Default target is El Morad male (race 12). The script prepares original KO data,
then invokes Blender in background mode to export the assembled player FBX.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys


def run(*args: str, cwd: Path) -> None:
    subprocess.run(list(args), cwd=str(cwd), check=True)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--blender", required=True, type=Path, help="Path to Blender executable")
    parser.add_argument("--race", type=int, default=12, choices=(1, 2, 3, 4, 11, 12, 13))
    args = parser.parse_args(argv)

    blender = args.blender.expanduser().resolve()
    if not blender.is_file():
        parser.error(f"Blender executable not found: {blender}")

    repo_root = Path(__file__).resolve().parents[1]
    source_root = repo_root / "unity" / "LegacySource" / "ko-assets-1298"
    looks_json = repo_root / "unity" / "Assets" / "Resources" / "Data" / "player_looks.json"
    output = repo_root / "unity" / "Assets" / "LegacyConverted" / "Players" / f"ko_player_race_{args.race}.fbx"
    exporter = repo_root / "tools" / "ko_to_unity" / "export_player.py"

    run(sys.executable, str(repo_root / "tools" / "prepare_phase0_data.py"), cwd=repo_root)

    output.parent.mkdir(parents=True, exist_ok=True)
    run(
        str(blender),
        "--background",
        "--python",
        str(exporter),
        "--",
        "--source-root",
        str(source_root),
        "--looks-json",
        str(looks_json),
        "--race",
        str(args.race),
        "--output",
        str(output),
        cwd=repo_root,
    )

    if not output.is_file() or output.stat().st_size == 0:
        raise SystemExit(f"KO player conversion did not produce an FBX: {output}")

    print(f"Original KO player race {args.race} converted for Unity: {output}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except subprocess.CalledProcessError as exc:
        sys.exit(exc.returncode or 1)
