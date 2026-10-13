#!/usr/bin/env python3
"""Archive the current workspace using a temporary Git tree, without a commit.

Includes tracked and nonignored untracked files in this workspace. Applies its
export-ignore attributes, preserves source weights and executable permissions,
and never writes to the original repository's index or history.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

WORKSPACE = Path(__file__).resolve().parents[1]


def git(*args, cwd=WORKSPACE, data=None):
    # Keep the snapshot independent of a caller's alternate index/worktree.
    env = {k: v for k, v in os.environ.items() if not k.startswith("GIT_")}
    return subprocess.run(["git", *args], cwd=cwd, input=data, check=True,
                          stdout=subprocess.PIPE, env=env).stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=WORKSPACE / "dist/perception_ws.tar.gz")
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists():
        parser.error(f"Output already exists; choose another --output: {output}")
    candidates = sorted(set(os.fsdecode(p) for p in git(
        "ls-files", "--cached", "--others", "--exclude-standard", "-z", "--", ".").split(b"\0") if p))
    paths = sorted({str(parent) for name in candidates
                    for parent in [Path(name), *Path(name).parents] if str(parent) != "."})
    attr_data = git("check-attr", "-z", "--stdin", "export-ignore",
                    data=b"\0".join(os.fsencode(p) for p in paths) + b"\0").split(b"\0")
    excluded = {os.fsdecode(attr_data[i]) for i in range(0, len(attr_data) - 1, 3)
                if attr_data[i + 2] == b"set"}
    selected = [name for name in candidates
                if name != str(output.relative_to(WORKSPACE))] if output.is_relative_to(WORKSPACE) else candidates
    selected = [name for name in selected
                if not any(str(p) in excluded for p in [Path(name), *Path(name).parents])
                and (WORKSPACE / name).is_file()]
    if "src/yolo26/models/small.pt" not in selected:
        parser.error("Missing small.pt: the archive must include the default model's source weights")
    with tempfile.TemporaryDirectory(prefix="perception-export-") as temporary:
        snapshot = Path(temporary)
        for name in selected:
            destination = snapshot / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(WORKSPACE / name, destination, follow_symlinks=False)
        git("init", "--quiet", cwd=snapshot)
        git("-c", "core.autocrlf=false", "add", "--force", "--all", cwd=snapshot)
        tree = git("write-tree", cwd=snapshot).decode().strip()
        output.parent.mkdir(parents=True, exist_ok=True)
        git("archive", "--format=tar.gz", "--prefix=perception_ws/",
            f"--output={output}", tree, cwd=snapshot)
    print(f"Archived {len(selected)} files from the working tree: {output}")
    print(f"Size: {output.stat().st_size / 1024**2:.2f} MiB")


if __name__ == "__main__":
    main()
