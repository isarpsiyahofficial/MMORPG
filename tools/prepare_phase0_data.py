#!/usr/bin/env python3
"""Prepare the original KO 1.298 character data used by the offline Unity APK."""

from __future__ import annotations

from pathlib import Path
import subprocess
import sys


def run(*args: str, cwd: Path) -> None:
    subprocess.run(list(args), cwd=str(cwd), check=True)


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    tools = repo_root / "tools"
    source_root = repo_root / "unity" / "LegacySource" / "ko-assets-1298"
    data_root = source_root / "Data"
    unity_data = repo_root / "unity" / "Assets" / "Resources" / "Data"
    baseline_dir = repo_root / "unity" / "Assets" / "StreamingAssets" / "Baseline"

    required = [
        data_root / "UPC_DefaultLooks.tbl",
        data_root / "NewChrValue.tbl",
    ]
    missing = [path for path in required if not path.is_file()]
    if missing:
        print("KO 1.298 source files are missing. Run: python tools/setup_ko_sources.py", file=sys.stderr)
        for path in missing:
            print(f"Missing: {path}", file=sys.stderr)
        return 2

    unity_data.mkdir(parents=True, exist_ok=True)
    baseline_dir.mkdir(parents=True, exist_ok=True)

    manifest = baseline_dir / "ko-source.generated.sha256"
    run(
        sys.executable,
        str(tools / "baseline_hash.py"),
        "create",
        str(source_root),
        str(manifest),
        cwd=repo_root,
    )

    run(
        sys.executable,
        str(tools / "extract_player_looks.py"),
        str(data_root / "UPC_DefaultLooks.tbl"),
        str(unity_data / "player_looks.json"),
        cwd=repo_root,
    )

    run(
        sys.executable,
        str(tools / "extract_new_character_values.py"),
        str(data_root / "NewChrValue.tbl"),
        str(unity_data / "new_character_values.json"),
        cwd=repo_root,
    )

    run(
        sys.executable,
        str(tools / "baseline_hash.py"),
        "verify",
        str(source_root),
        str(manifest),
        cwd=repo_root,
    )

    print("Phase-0 KO character data prepared without SQL and source files verified unchanged.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except subprocess.CalledProcessError as exc:
        sys.exit(exc.returncode or 1)
