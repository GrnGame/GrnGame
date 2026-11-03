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

ASSETS_PAK = "Assets.pak"

if __name__ == "__main__":
    test_dir = Path(sys.argv[1])
    runtime_exe = Path(sys.argv[2]) if len(sys.argv) > 2 else None
    (test_dir / ASSETS_PAK).unlink(missing_ok=True)
    if runtime_exe:
        remove_runtime(runtime_exe, test_dir)

    remove_std(test_dir)
    remove_controller_db(test_dir)