import shutil
import subprocess
from pathlib import Path

RUNTIME_DIR = Path(__file__).resolve().parent.parent / "runtime"

DIST_DIR = Path("dist")
ANDROID_BUILD_DIR = Path("android-build")
ANDROID_DIST_DIR = Path("android")
IOS_SCRIPT = Path("xcode_ios_project.py")

SERVER_PLATFORM = "server"
IOS_PLATFORM = "ios-arm64"

PLATFORMS = [
    SERVER_PLATFORM,
    "linux-x86_64",
    "linux-arm64",
    "macos-x86_64",
    "macos-arm64",
    "windows-x64",
    "windows-arm64",
    IOS_PLATFORM,
]

SERVER_FILES = [
    "server.py",
    "index.html",
    "logo.png",
    "runtime-wasm.js",
    "runtime-wasm.wasm",
]

RUNTIME_BINARIES = {
    "runtime-linux-arm64": "linux-arm64",
    "runtime-linux-x64": "linux-x86_64",
    "runtime-macos-arm64": "macos-arm64",
    "runtime-macos-x64": "macos-x86_64",
    "runtime-windows-arm64.exe": "windows-arm64",
    "runtime-windows-x64.exe": "windows-x64",
    "runtime-ios-arm64": IOS_PLATFORM,
}

EMBEDDED_PATTERN = "Embedded-*"
ASSETS_PAK = "Assets.pak"
PAK_CONTENT = ["assets", "scripts", "std", "data", "config"]


def require_assets_bundle(path: Path) -> None:
    if not path.is_file():
        raise FileNotFoundError(
            f"Required file not found:{path},execute Embedded command to create {path}"
        )


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


def copy_required_file(src: Path, dst_dir: Path) -> None:
    require_file(src)
    shutil.copy2(src, dst_dir / src.name)


def copy_to_all_platforms(src: Path, dist_dir: Path) -> None:
    for platform in PLATFORMS:
        shutil.copy2(src, dist_dir / platform / src.name)


def clean_dist(project_dir: Path) -> None:
    dist_dir = project_dir / DIST_DIR

    if dist_dir.exists():
        shutil.rmtree(dist_dir)

    for platform in PLATFORMS:
        (dist_dir / platform).mkdir(parents=True)


def bundle_server_files(runtime_dir: Path, dist_dir: Path) -> None:
    for filename in SERVER_FILES:
        copy_required_file(runtime_dir / filename, dist_dir / SERVER_PLATFORM)


def bundle_binaries(runtime_dir: Path, dist_dir: Path) -> None:
    for filename, platform in RUNTIME_BINARIES.items():
        copy_required_file(runtime_dir / filename, dist_dir / platform)


def bundle_android_build(runtime_dir: Path, dist_dir: Path) -> None:
    src = runtime_dir / ANDROID_BUILD_DIR
    require_folder(src)

    shutil.copytree(
        src,
        dist_dir / ANDROID_DIST_DIR,
        dirs_exist_ok=True,
        copy_function=shutil.copy2,
    )


def bundle_ios_script(runtime_dir: Path, dist_dir: Path) -> None:
    copy_required_file(runtime_dir / IOS_SCRIPT, dist_dir / IOS_PLATFORM)


def bundle_runtime(runtime_dir: Path, project_dir: Path) -> None:
    dist_dir = project_dir / DIST_DIR

    bundle_server_files(runtime_dir, dist_dir)
    bundle_binaries(runtime_dir, dist_dir)
    bundle_android_build(runtime_dir, dist_dir)
    bundle_ios_script(runtime_dir, dist_dir)


def find_embedded(project_dir: Path) -> Path:
    for file in project_dir.glob(EMBEDDED_PATTERN):
        if is_executable_file(file):
            return file

    raise FileNotFoundError(
        f"Error: Embedded executable not found in:\n  {project_dir} try to redownload the release"
    )


def bundle_assets(project_dir: Path) -> None:
    embedded_bin = find_embedded(project_dir)

    subprocess.run(
        [str(embedded_bin), ASSETS_PAK, *PAK_CONTENT],
        check=True,
    )


def bundle_assets_pak(project_dir: Path) -> None:
    assets_pak = project_dir / ASSETS_PAK
    require_assets_bundle(assets_pak)

    android_assets = dist_dir / ANDROID_DIST_DIR / "app" / "src" / "main" / "assets"
    android_assets.mkdir(parents=True, exist_ok=True)
    shutil.copy2(assets_pak, android_assets / assets_pak.name)


if __name__ == "__main__":
    project_dir = Path.cwd()

    clean_dist(project_dir)
    bundle_runtime(RUNTIME_DIR, project_dir)
    bundle_assets(project_dir)
    bundle_assets_pak(project_dir)
