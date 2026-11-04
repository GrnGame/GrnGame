import shutil
from pathlib import Path

# dont update dist folder

PROJECT_MODEL_DIR = Path(__file__).resolve().parent.parent / "project_model"

DATA_DIR = Path("data")
STD_DIR = Path("std")
GAME_CONTROLLER_DB = DATA_DIR / "gamecontrollerdb.txt"

EMBEDDED_PATTERN = "Embedded-*"
RUNTIME_PATTERN = "desktop-*"


def require_file(path: Path) -> None:
    if not path.is_file():
        raise FileNotFoundError(
            f"Required file not found:{path},try to redownload release"
        )


def require_folder(path: Path) -> None:
    if not path.is_dir():
        raise FileNotFoundError(
            f"Required directory not found:\n  {path},try to redownload release"
        )


def is_executable_file(path: Path) -> bool:
    return path.is_file() and bool(path.stat().st_mode & 0o111)


def find_executable(directory: Path, pattern: str, label: str) -> Path:
    for file in directory.glob(pattern):
        if is_executable_file(file):
            return file
    raise FileNotFoundError(
        f"Error: {label} executable not found in:\n  {directory} try to redownload the release"
    )


def find_embedded(project_model: Path) -> Path:
    return find_executable(project_model, EMBEDDED_PATTERN, "Embedded")


def find_runtime(project_model: Path) -> Path:
    return find_executable(project_model, RUNTIME_PATTERN, RUNTIME_PATTERN)


def replace_binary(binary: Path, dir_to: Path, pattern: str) -> None:
    for old in dir_to.glob(pattern):
        if old.is_file():
            old.unlink()
    shutil.copy2(binary, dir_to / binary.name)


def update_embedded(dir_from: Path, dir_to: Path) -> None:
    replace_binary(find_embedded(dir_from), dir_to, EMBEDDED_PATTERN)


def update_runtime(dir_from: Path, dir_to: Path) -> None:
    replace_binary(find_runtime(dir_from), dir_to, RUNTIME_PATTERN)


def update_game_data(dir_from: Path, dir_to: Path) -> None:
    src = dir_from / GAME_CONTROLLER_DB
    require_file(src)

    dst = dir_to / GAME_CONTROLLER_DB
    dst.parent.mkdir(exist_ok=True)
    shutil.copy2(src, dst)


def update_std(dir_from: Path, dir_to: Path) -> None:
    src = dir_from / STD_DIR
    require_folder(src)

    dst = dir_to / STD_DIR
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst, copy_function=shutil.copy2)


def validate_project_model(project_model: Path) -> None:
    require_folder(project_model)
    find_embedded(project_model)
    find_runtime(project_model)
    require_file(project_model / GAME_CONTROLLER_DB)
    require_folder(project_model / STD_DIR)


if __name__ == "__main__":
    project_dir = Path.cwd()

    validate_project_model(PROJECT_MODEL_DIR)

    update_embedded(PROJECT_MODEL_DIR, project_dir)
    update_runtime(PROJECT_MODEL_DIR, project_dir)
    update_game_data(PROJECT_MODEL_DIR, project_dir)
    update_std(PROJECT_MODEL_DIR, project_dir)
