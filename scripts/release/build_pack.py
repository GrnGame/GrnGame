import os
import subprocess
import sys
import shutil
from pathlib import Path

OS = {"Linux": "linux", "Windows": "windows", "macOS": "macos"}
ARCH = {"X64": "x64", "ARM64": "arm64"}

if __name__ == "__main__":
    tag = sys.argv[1]
    platform = f"{OS[os.environ['RUNNER_OS']]}-{ARCH[os.environ['RUNNER_ARCH']]}"

    stage_dir = Path(f"staging_{platform}").resolve()
    output_dir = Path("release-assets").resolve()
    output_dir.mkdir(exist_ok=True)

    if not stage_dir.exists():
        sys.exit(1)

    version = tag.lstrip("v")

    print("::group::Configure CMake")
    subprocess.run(
        [
            "cmake", "-B", "build_pack", "-S", "pack",
            f"-DSTAGING_DIR={stage_dir}",
            f"-DGRNGAME_VERSION={version}",
            f"-DCPACK_SYSTEM_NAME={platform}"
        ],
        check=True,
    )
    print("::endgroup::")

    print("::group::Run CPack")
    subprocess.run(
        ["cpack", "-B", str(output_dir)],
        cwd="build_pack",
        check=True,
    )
    print("::endgroup::")

    for f in output_dir.glob("*.wixpdb"):
        f.unlink()

    for f in output_dir.glob("*.zip"):
        f.unlink()

    for f in output_dir.glob("*.tar.gz"):
        f.unlink()
