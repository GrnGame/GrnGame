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

if __name__ == "__main__":
    action = sys.argv[1]
    test_dir = Path(sys.argv[2])
    #prepare before run executable
    if action == "prepare":
        root_dir = Path(sys.argv[3])
        runtime_exe = Path(sys.argv[4]) if len(sys.argv) > 4 else None

        copy_std(root_dir, test_dir)
        copy_controller_db(root_dir, test_dir)
        if runtime_exe:
            copy_runtime(runtime_exe, test_dir)
    #clean after run executable
    elif action == "clean":
        runtime_exe = Path(sys.argv[3]) if len(sys.argv) > 3 else None

        remove_std(test_dir)
        remove_controller_db(test_dir)
        if runtime_exe:
            remove_runtime(runtime_exe, test_dir)