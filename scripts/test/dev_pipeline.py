import sys
from pathlib import Path
from helper import ROOT_DIR, link_path, populate

if __name__ == "__main__":
    target_name = Path(sys.argv[1])
    build_dir = Path(sys.argv[2])

    populate(ROOT_DIR / target_name, ROOT_DIR, build_dir, link_path)
