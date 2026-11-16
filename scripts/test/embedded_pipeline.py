import subprocess
import sys
from pathlib import Path
from helper import copy_path, populate, remove_path, find_embedded

ASSETS_PAK = "Assets.pak"
PAK_CONTENT = ["assets", "scripts", "std", "config", "data"]

if __name__ == "__main__":
    target_name = Path(sys.argv[1])
    build_dir = Path(sys.argv[2])
    root_dir = Path(sys.argv[3])
    embedded_dir = Path(sys.argv[4])

    embedded = find_embedded(embedded_dir)

    remove_path(build_dir / ASSETS_PAK)
    created = populate(root_dir / target_name, root_dir, build_dir, copy_path)

    try:
        content = [name for name in PAK_CONTENT if (build_dir / name).exists()]
        subprocess.run([str(embedded.resolve()), ASSETS_PAK, *content], cwd=build_dir, check=True)
    finally:
        for path in created:
            remove_path(path)
