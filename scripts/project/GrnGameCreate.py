import shutil
import sys
from pathlib import Path

PROJECT_MODEL_DIR = Path(__file__).resolve().parent.parent / "project_model"


def require_folder(path: Path) -> None:
    if not path.is_dir():
        raise FileNotFoundError(
            f"Required directory not found:\n  {path},try to redownload release"
        )


def bundle_project(name: str, dir_to: str) -> None:
    require_folder(PROJECT_MODEL_DIR)

    shutil.copytree(
        PROJECT_MODEL_DIR,
        Path(dir_to) / name,
        dirs_exist_ok=True,
        copy_function=shutil.copy2,
    )


if __name__ == "__main__":
    name = sys.argv[1]
    dir_to = sys.argv[2]
    bundle_project(name, dir_to)
