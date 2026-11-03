import shutil
import subprocess
import sys
from pathlib import Path
from helper import (
    copy_controller_db,
    copy_runtime,
    copy_std,
    remove_controller_db,
    remove_runtime,
    remove_std,
)

STD_DIR = Path("std")
DATA_DIR = Path("data")
EMBEDDED_PATTERN = "Embedded-*"
ASSETS_PAK = "Assets.pak"
PAK_CONTENT = ["assets", "scripts", str(STD_DIR), "config", str(DATA_DIR)]


def find_embedded(embedded_dir: Path) -> Path:
    for path in embedded_dir.glob(EMBEDDED_PATTERN):
        if path.is_file() and path.suffix not in {".pdb", ".ilk", ".exp", ".lib", ".obj"}:
            return path
    raise FileNotFoundError(f"No {EMBEDDED_PATTERN} found in {embedded_dir}")


def build_pak(embedded: Path, test_dir: Path) -> None:
    cmd = [str(embedded.resolve()), ASSETS_PAK, *PAK_CONTENT]
    subprocess.run(cmd, cwd=test_dir, check=True)


if __name__ == "__main__":
    test_dir = Path(sys.argv[1])
    root_dir = Path(sys.argv[2])
    embedded_dir = Path(sys.argv[3])
    runtime_exe = Path(sys.argv[4]) if len(sys.argv) > 4 else None

    embedded = find_embedded(embedded_dir)

    copy_std(root_dir, test_dir)
    copy_controller_db(root_dir, test_dir)

    try:
        build_pak(embedded, test_dir)
    finally:
        remove_std(test_dir)

    if runtime_exe:
        copy_runtime(runtime_exe, test_dir)