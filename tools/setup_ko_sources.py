#!/usr/bin/env python3
"""Fetch the exact public source inputs used by the phase-0 KO mobile port.

This keeps legacy assets outside Git history while making the conversion setup
repeatable. It never modifies the upstream source repositories.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys

KO_ASSETS_REPO = "https://github.com/ko4life-net/ko-assets.git"
KO_ASSETS_BRANCH = "1298"
KO_ASSETS_COMMIT = "705874881c8308e0a45a0c849e4b3881088d8138"

OPENKO_BLENDER_REPO = "https://github.com/Open-KO/OpenKO-blender.git"
OPENKO_BLENDER_COMMIT = "e47142e785f59529a894225471e328d9cd8b3ac4"


def run(*args: str, cwd: Path | None = None) -> str:
    process = subprocess.run(
        list(args),
        cwd=str(cwd) if cwd else None,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    return process.stdout.strip()


def clone_exact(repo: str, destination: Path, commit: str, branch: str | None = None, force: bool = False) -> None:
    if destination.exists():
        if not force:
            current = ""
            try:
                current = run("git", "rev-parse", "HEAD", cwd=destination)
            except Exception:
                pass
            if current == commit:
                print(f"OK {destination}: already pinned at {commit}")
                return
            raise SystemExit(
                f"Refusing to replace existing source folder: {destination}\n"
                "Use --force only if you intentionally want to recreate it."
            )
        shutil.rmtree(destination)

    destination.parent.mkdir(parents=True, exist_ok=True)
    clone_args = ["git", "clone", "--no-tags"]
    if branch:
        clone_args += ["--branch", branch]
    clone_args += [repo, str(destination)]
    run(*clone_args)
    run("git", "checkout", "--detach", commit, cwd=destination)

    actual = run("git", "rev-parse", "HEAD", cwd=destination)
    if actual != commit:
        raise SystemExit(f"Pinned source verification failed for {destination}: {actual} != {commit}")

    print(f"OK {destination}: {actual}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args(argv)

    repo_root = Path(__file__).resolve().parents[1]
    legacy_assets = repo_root / "unity" / "LegacySource" / "ko-assets-1298"
    blender_tools = repo_root / "tools" / "vendor" / "OpenKO-blender"

    clone_exact(
        KO_ASSETS_REPO,
        legacy_assets,
        KO_ASSETS_COMMIT,
        branch=KO_ASSETS_BRANCH,
        force=args.force,
    )
    clone_exact(
        OPENKO_BLENDER_REPO,
        blender_tools,
        OPENKO_BLENDER_COMMIT,
        force=args.force,
    )

    print("KO 1.298 source inputs are ready and pinned.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except subprocess.CalledProcessError as exc:
        print(exc.stdout or str(exc), file=sys.stderr)
        sys.exit(exc.returncode or 1)
