import shutil
from pathlib import Path

STD_DIR = Path("std")
DATA_DIR = Path("data")
GAME_CONTROLLER_DB = Path("grngame/input/gamecontrollerdb.txt")


def copy_tree(src: Path, dst: Path) -> None:
    shutil.copytree(src, dst, dirs_exist_ok=True, copy_function=shutil.copy2)


def copy_runtime(runtime: Path, dst_dir: Path) -> None:
    dst_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(runtime, dst_dir / runtime.name)


def remove_runtime(runtime: Path, dst_dir: Path) -> None:
    (dst_dir / runtime.name).unlink(missing_ok=True)


def copy_std(root: Path, dst_dir: Path) -> None:
    copy_tree(root / STD_DIR, dst_dir / STD_DIR)


def remove_std(dst_dir: Path) -> None:
    shutil.rmtree(dst_dir / STD_DIR, ignore_errors=True)


def copy_controller_db(root: Path, dst_dir: Path) -> None:
    data_dir = dst_dir / DATA_DIR
    data_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(root / GAME_CONTROLLER_DB, data_dir / GAME_CONTROLLER_DB.name)


def remove_controller_db(dst_dir: Path) -> None:
    shutil.rmtree(dst_dir / DATA_DIR, ignore_errors=True)