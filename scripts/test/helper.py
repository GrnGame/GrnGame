import os
import shutil
from pathlib import Path

STD_DIR = "std"
DATA_DIR = "data"
CONTROLLER_DB = "grngame/input/gamecontrollerdb.txt"
ROOT_DIR = Path(__file__).resolve().parents[2]

IGNORED_SUFFIXES = {".pdb", ".ilk", ".exp", ".lib", ".obj"}


def remove_path(path: Path) -> None:
    if path.is_symlink() or path.is_file():
        path.unlink()
    elif path.is_dir():
        shutil.rmtree(path)


def copy_path(src: Path, dst: Path) -> None:
    remove_path(dst)
    if src.is_dir():
        shutil.copytree(src, dst)
    else:
        shutil.copy2(src, dst)


def link_path(src: Path, dst: Path) -> None:
    remove_path(dst)
    os.symlink(src.resolve(), dst)


def populate(test_src: Path, root: Path, build_dir: Path, put) -> list[Path]:
    build_dir.mkdir(parents=True, exist_ok=True)
    created = []

    for entry in test_src.iterdir():
        if entry.name != DATA_DIR:
            put(entry, build_dir / entry.name)
            created.append(build_dir / entry.name)

    put(root / STD_DIR, build_dir / STD_DIR)
    created.append(build_dir / STD_DIR)

    data = build_dir / DATA_DIR
    remove_path(data)
    data.mkdir()
    test_data = test_src / DATA_DIR
    if test_data.is_dir():
        for entry in test_data.iterdir():
            put(entry, data / entry.name)
    put(root / Path(CONTROLLER_DB), data / Path(CONTROLLER_DB).name)
    created.append(data)

    return created


def find_embedded(embedded_dir: Path) -> Path:
    for path in embedded_dir.glob("Embedded-*"):
        if path.is_file() and path.suffix not in IGNORED_SUFFIXES:
            return path
    raise FileNotFoundError("Compile engine with embed_assets=OFF first to have Embedded executable.")
