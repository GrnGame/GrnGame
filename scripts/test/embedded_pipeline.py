import shutil
import subprocess
import sys
import os
from pathlib import Path

RUNTIME_BUILD_DIR = Path("build/Runtime")
STD_DIR = Path("std")
DATA_DIR = Path("data")
GAME_CONTROLLER_DB = Path("grngame/input/gamecontrollerdb.txt")

EMBEDDED_PATTERN = "Embedded-*"
ASSETS_PAK = "Assets.pak"
METADATA_FILE = "metadata.json"
PAK_CONTENT = ["assets", "scripts", str(STD_DIR), "config", str(DATA_DIR)]

IGNORED_FILES = shutil.ignore_patterns("*.exp", "*.lib")


def find_embedded(dir_build_embedded: Path) -> Path:
    for path in dir_build_embedded.glob(EMBEDDED_PATTERN):
        if path.is_file() and path.suffix not in {".pdb", ".ilk", ".exp", ".lib", ".obj"}:
            return path
    raise FileNotFoundError(
        f"No {EMBEDDED_PATTERN} file found in {dir_build_embedded}"
    )


def stage_std(dir_racine: Path, dir_from: Path) -> None:
    shutil.copytree(
        dir_racine / STD_DIR,
        dir_from / STD_DIR,
        copy_function=shutil.copy2,
    )


def stage_data(dir_racine: Path, dir_from: Path) -> None:
    data_dir = dir_from / DATA_DIR
    data_dir.mkdir()

    shutil.copy2(
        dir_racine / GAME_CONTROLLER_DB,
        data_dir / GAME_CONTROLLER_DB.name,
    )

def cleanup(dir_from: Path) -> None:
    shutil.rmtree(dir_from / STD_DIR, ignore_errors=True)
    shutil.rmtree(dir_from / DATA_DIR, ignore_errors=True)


def build_pak(embedded: Path, dir_from: Path) -> Path:
    command = [str(embedded.resolve()), ASSETS_PAK, *PAK_CONTENT]

    subprocess.run(command, cwd=dir_from, check=True)

    return dir_from / ASSETS_PAK


def move_pak(pak: Path, dir_to: Path) -> None:
    dir_to.mkdir(parents=True, exist_ok=True)

    shutil.move(pak, dir_to / pak.name)


def bundle_runtime(dir_to: Path) -> None:
    shutil.copytree(
        RUNTIME_BUILD_DIR,
        dir_to,
        dirs_exist_ok=True,
        copy_function=shutil.copy2,
        ignore=IGNORED_FILES,
    )


def bundle_metadata(dir_from: Path, dir_to: Path) -> None:
    shutil.copy2(dir_from / METADATA_FILE, dir_to / METADATA_FILE)


if __name__ == "__main__":
    dir_from = Path(sys.argv[1]) #target name
    dir_to = Path(sys.argv[2]) #build dir
    dir_racine = Path(sys.argv[3]) #./
    dir_build_embedded = Path(sys.argv[4]) #target embedded

    embedded = find_embedded(dir_build_embedded)

    stage_std(dir_racine, dir_from)
    stage_data(dir_racine, dir_from)

    bundle_runtime(dir_to)
    bundle_metadata(dir_from, dir_to)

    try:
        pak = build_pak(embedded, dir_from)
    finally:
        cleanup(dir_from)

    move_pak(pak, dir_to)