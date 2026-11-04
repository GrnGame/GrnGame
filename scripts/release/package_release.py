import shutil
import sys
from contextlib import contextmanager
from pathlib import Path

ARTIFACTS_DIR = Path("artifacts")
OUTPUT_DIR = Path("release-assets")
SCRIPTS_DIR = Path("scripts")
RESSOURCES_DIR = Path("ressources")

STD_DIR = Path("std")
ANDROID_BUILD_DIR = Path("android-build")
GAME_CONTROLLER_DB = Path("grngame/input/gamecontrollerdb.txt")

CONFIG_FILE = RESSOURCES_DIR / "config.json"
MAIN_SCRIPT = RESSOURCES_DIR / "main.wren"
LOGO_FILE = RESSOURCES_DIR / "logo.png"

SETUP_SCRIPTS_DIR = SCRIPTS_DIR / "project"
SERVER_SCRIPT = SCRIPTS_DIR / "server.py"
INDEX = RESSOURCES_DIR / "index.html"
IOS_SCRIPT = SCRIPTS_DIR / "xcode_ios_project.py"

ANDROID_LIB_NAME = "libGrnGame.so"

PLATFORMS = [
    "linux-x64",
    "linux-arm64",
    "macos-x64",
    "macos-arm64",
    "windows-x64",
    "windows-arm64"
]

# ---------------------------------------------------------------------------
# Artefacts CI (noms des upload-artifact dans build-*.yml) -> dossier artifacts/
# Format : <variante>-<plateforme>
#   desktop-* : binaire desktop (embed assets = OFF)
#   runtime-* : runtime embarque (embed assets = ON)
# ---------------------------------------------------------------------------
DESKTOP_ARTIFACTS = {
    "linux-x64": "desktop-linux-x64",
    "linux-arm64": "desktop-linux-arm64",
    "macos-x64": "desktop-macos-x64",
    "macos-arm64": "desktop-macos-arm64",
    "windows-x64": "desktop-windows-x64",
    "windows-arm64": "desktop-windows-arm64",
}

RUNTIME_ARTIFACTS = [
    "runtime-linux-x64",
    "runtime-linux-arm64",
    "runtime-macos-x64",
    "runtime-macos-arm64",
    "runtime-windows-x64",
    "runtime-windows-arm64",
    "runtime-ios-arm64",
    "runtime-wasm",
]

ANDROID_ARTIFACTS = {
    "arm64-v8a": "runtime-android-arm64-v8a",
    "x86_64": "runtime-android-x86_64",
}

GITHUB_ASSET_PREFIX = "GrnGame"

SETUP_SCRIPTS = [
    "GrnGameCreate.py",
    "GrnGameCreate.sh",
    "GrnGameCreate.bat",
    "GrnGameDist.py",
    "GrnGameDist.sh",
    "GrnGameDist.bat",
    "GrnGameUpdate.py",
    "GrnGameUpdate.sh",
    "GrnGameUpdate.bat",
]

PROJECT_MODEL_DIRS = ["assets", "data", "config"]

IGNORED_SUFFIXES = {".lib", ".exp", ".pdb", ".a", ".ilk", ".html"}
IGNORED_FILES = shutil.ignore_patterns(*(f"*{suffix}" for suffix in IGNORED_SUFFIXES))


@contextmanager
def log_group(title: str):
    print(f"::group::{title}")
    try:
        yield
    finally:
        print("::endgroup::")


def copy_folder(src: Path, dst: Path, ignore=None) -> None:
    shutil.copytree(src, dst, dirs_exist_ok=True, copy_function=shutil.copy2, ignore=ignore)


def copy_file(src: Path, dst_folder: Path) -> None:
    dst_folder.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst_folder)


def is_ignored(file: Path) -> bool:
    return file.suffix in IGNORED_SUFFIXES


def project_model_dir(stage_dir: Path) -> Path:
    return stage_dir / "project_model"


def runtime_dir(stage_dir: Path) -> Path:
    return stage_dir / "runtime"


def bundle_project_model(stage_dir: Path) -> None:
    model_dir = project_model_dir(stage_dir)

    for name in PROJECT_MODEL_DIRS:
        (model_dir / name).mkdir(parents=True, exist_ok=True)

    copy_file(CONFIG_FILE, model_dir / "config")
    copy_file(MAIN_SCRIPT, model_dir / "scripts")
    copy_file(GAME_CONTROLLER_DB, model_dir / "data")
    copy_folder(STD_DIR, model_dir / "std")


def bundle_logo(stage_dir: Path) -> None:
    copy_file(LOGO_FILE, runtime_dir(stage_dir))


def bundle_setup_scripts(stage_dir: Path) -> None:
    for name in SETUP_SCRIPTS:
        copy_file(SETUP_SCRIPTS_DIR / name, stage_dir / "scripts")


def bundle_server_script(stage_dir: Path) -> None:
    copy_file(SERVER_SCRIPT, runtime_dir(stage_dir))


def bundle_server_index(stage_dir: Path) -> None:
    copy_file(INDEX, runtime_dir(stage_dir))


def bundle_ios_script(stage_dir: Path) -> None:
    copy_file(IOS_SCRIPT, runtime_dir(stage_dir))


def bundle_desktop_binary(platform: str, stage_dir: Path) -> None:
    src = ARTIFACTS_DIR / DESKTOP_ARTIFACTS[platform]

    for file in src.rglob("*"):
        if file.is_file() and not is_ignored(file):
            copy_file(file, project_model_dir(stage_dir))


# with desktop action the artefact conserve the file path
def resolve_runtime_source(artifact: Path) -> Path:
    nested_v4 = artifact / "runtime" / "release"
    if nested_v4.is_dir():
        return nested_v4

    release = artifact / "release"
    if release.is_dir():
        return release

    nested = artifact / "Runtime"
    return nested if nested.is_dir() else artifact


def bundle_runtimes(stage_dir: Path) -> None:
    destination = runtime_dir(stage_dir)
    destination.mkdir(parents=True, exist_ok=True)

    for artifact_name in RUNTIME_ARTIFACTS:
        artifact = ARTIFACTS_DIR / artifact_name

        if artifact.is_dir():
            copy_folder(resolve_runtime_source(artifact), destination, ignore=IGNORED_FILES)


def bundle_android_libs(android_dst: Path) -> None:
    for abi, artifact_name in ANDROID_ARTIFACTS.items():
        lib = resolve_runtime_source(ARTIFACTS_DIR / artifact_name) / ANDROID_LIB_NAME

        if lib.is_file():
            copy_file(lib, android_dst / "app" / "jni" / "src" / abi)


def bundle_android(stage_dir: Path) -> None:
    android_dst = runtime_dir(stage_dir) / "android-build"

    copy_folder(ANDROID_BUILD_DIR, android_dst)
    bundle_android_libs(android_dst)


def make_zip(platform: str, stage_dir: Path, tag: str) -> None:
    OUTPUT_DIR.mkdir(exist_ok=True)
    version = tag.lstrip("v")
    archive = OUTPUT_DIR / f"{GITHUB_ASSET_PREFIX}-{platform}-{version}"
    shutil.make_archive(str(archive), "zip", root_dir=stage_dir)


def build_stage(platform: str, stage_dir: Path) -> None:
    bundle_project_model(stage_dir)
    bundle_setup_scripts(stage_dir)
    bundle_server_script(stage_dir)
    bundle_server_index(stage_dir)
    bundle_desktop_binary(platform, stage_dir)
    bundle_runtimes(stage_dir)
    bundle_android(stage_dir)
    bundle_ios_script(stage_dir)
    bundle_logo(stage_dir)


def package(platform: str, tag: str) -> None:
    with log_group(f"Packaging {platform}"):
        stage_dir = Path(f"staging_{platform}")
        stage_dir.mkdir(exist_ok=True)

        build_stage(platform, stage_dir)
        make_zip(platform, stage_dir, tag)


def main() -> None:
    tag = sys.argv[1]

    for platform in PLATFORMS:
        package(platform, tag)


if __name__ == "__main__":
    main()
