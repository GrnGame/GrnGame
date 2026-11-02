import shutil
import sys
from pathlib import Path
import os

RUNTIME_BUILD_DIR = os.environ.get("GRN_RUNTIME_DIR", "build/Runtime")
STD_DIR = Path("std")
DATA_DIR = Path("data")
GAME_CONTROLLER_DB = Path("grngame/input/gamecontrollerdb.txt")

IGNORED_FILES = shutil.ignore_patterns("*.exp", "*.lib")


def copy_folder(src: Path, dst: Path, ignore=None) -> None:
    shutil.copytree(
        src,
        dst,
        dirs_exist_ok=True,
        copy_function=shutil.copy2,
        ignore=ignore,
    )


def bundle_assets(dir_from: Path, dir_to: Path) -> None:
    copy_folder(dir_from, dir_to)


def bundle_std(dir_to: Path) -> None:
    copy_folder(STD_DIR, dir_to / STD_DIR)


def bundle_controller_db(dir_to: Path) -> None:
    data_dir = dir_to / DATA_DIR
    data_dir.mkdir(parents=True, exist_ok=True)

    shutil.copy2(GAME_CONTROLLER_DB, data_dir / GAME_CONTROLLER_DB.name)


def bundle_runtime(dir_to: Path) -> None:
    copy_folder(RUNTIME_BUILD_DIR, dir_to, ignore=IGNORED_FILES)


if __name__ == "__main__":
    dir_from = Path(sys.argv[1]) # target
    dir_to = Path(sys.argv[2]) #build dir

    bundle_assets(dir_from, dir_to)
    bundle_std(dir_to)
    bundle_controller_db(dir_to)
    bundle_runtime(dir_to)