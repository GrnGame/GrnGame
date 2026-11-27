#!/usr/bin/env python3

# this file is expimental and havent been tested

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

ORIENTATIONS = {
    "landscape": [
        "UIInterfaceOrientationLandscapeLeft",
        "UIInterfaceOrientationLandscapeRight",
    ],
    "portrait": ["UIInterfaceOrientationPortrait"],
    "both": [
        "UIInterfaceOrientationPortrait",
        "UIInterfaceOrientationLandscapeLeft",
        "UIInterfaceOrientationLandscapeRight",
    ],
}

# size standard 
SIZE_ICON = 1024
# sdk version min
SDK_VERSION = "26.2"
# chmod standard macos
CHMOD = 755
CHMOD_BASE_8 = 0o755


class Fail(Exception):
    pass


def execute_command(cmd: list[str], cwd: Path | None = None) -> str:
    try:
        return subprocess.run(
            cmd, cwd=cwd, check=True, capture_output=True, text=True
        ).stdout
    except FileNotFoundError:
        raise Fail(f"command not found: {cmd[0]}")
    except subprocess.CalledProcessError as e:
        raise Fail(f"{cmd[0]} failed: {(e.stderr or e.stdout).strip()}")


def as_path(value: str) -> Path:
    return Path(value.strip()).expanduser()


def ask_user(value, question: str, default: str | None = None, check=None) -> None:
    while True:
        if not value:
            suffix = f" [{default}]" if default else ""
            try:
                value = input(f"{question}{suffix}: ").strip() or default
            except EOFError:
                value = default

            if not value:
                raise Fail(f"missing answer: {question}")

        if check is None:
            return value

        try:
            return check(value)
        except Fail as e:
            print(f"  {e}")
            value = None


def check_bundle_id(v: str) -> str:
    if not re.fullmatch(r"[A-Za-z0-9-]+(\.[A-Za-z0-9-]+)+", v):
        raise Fail("invalid bundle ID (example: com.mystudio.mygame).")
    return v.strip().lower()  # macos standard


def check_team_id(v: str) -> str:
    v = v.strip().upper()
    if not re.fullmatch(r"[A-Z0-9]{10}", v):
        raise Fail("the Team ID must be 10 characters (capital letters and digits).")
    return v


def check_orientation(v: str) -> str:
    v = v.strip().lower()
    if v not in ORIENTATIONS:
        raise Fail("choose landscape, portrait or both.")
    return v


def check_dist(v: str) -> Path:
    d = as_path(v)

    if not (d / "Assets.pak").is_file():
        raise Fail(f"Assets.pak not found in {d}")

    if not any(p.is_file() for p in d.glob("Runtime*")):
        raise Fail(f"no Runtime* file found in {d}")

    return d


def check_icon(v: str) -> Path:
    p = as_path(v)

    if not p.is_file():
        raise Fail(f"icon not found: {p}")

    dims = execute_command([
        "sips", "-g", "pixelWidth", "-g", "pixelHeight", str(p)
    ])

    if f"pixelWidth: {SIZE_ICON}" not in dims or f"pixelHeight: {SIZE_ICON}" not in dims:
        raise Fail(f"the icon must be exactly {SIZE_ICON}x{SIZE_ICON} pixels.")

    return p


def check_environment() -> None:
    if sys.platform != "darwin":
        raise Fail("this script must run on macOS.")

    if not shutil.which("xcodegen"):
        if not shutil.which("brew"):
            raise Fail("install Homebrew (https://brew.sh), then run the script again.")

        print("Installing xcodegen...")
        execute_command(["brew", "install", "xcodegen"])


def write_icon(icon: Path, work: Path) -> None:
    assets = work / "Assets.xcassets"
    iconset = assets / "AppIcon.appiconset"
    iconset.mkdir(parents=True)

    execute_command([
        "sips", "-s", "format", "png",
        str(icon), "--out", str(iconset / "icon1024.png")
    ])

    meta = {"author": "xcode", "version": 1}

    (assets / "Contents.json").write_text(
        json.dumps({"info": meta}, indent=2)
    )

    (iconset / "Contents.json").write_text(json.dumps({
        "images": [{
            "filename": f"icon{SIZE_ICON}.png",
            "idiom": "universal",
            "platform": "ios",
            "size": f"{SIZE_ICON}x{SIZE_ICON}",
        }],
        "info": meta,
    }, indent=2))


def write_project(args, safe_name: str, work: Path, has_privacy: bool) -> None:
    sources = [
        {"path": "stub.c"},  # factisse file
        {"path": "Assets.xcassets"},
        {"path": "Assets.pak", "buildPhase": "resources"},
    ]

    if has_privacy:
        sources.append({
            "path": "PrivacyInfo.xcprivacy",
            "buildPhase": "resources",
        })

    spec = {
        "name": safe_name,
        "targets": {
            safe_name: {
                "type": "application",
                "platform": "iOS",
                "deploymentTarget": "17.5",
                "sources": sources,
                "info": {
                    "path": "Info.plist",
                    "properties": {
                        "CFBundleDisplayName": args.name,
                        "CFBundleShortVersionString": args.version,
                        "CFBundleVersion": args.build,
                        "ITSAppUsesNonExemptEncryption": False,
                        "UILaunchScreen": {},
                        "UIRequiresFullScreen": True,
                        "UIApplicationSupportsIndirectInputEvents": True,
                        "UISupportedInterfaceOrientations": ORIENTATIONS[
                            args.orientation
                        ],
                    },
                },
                "settings": {
                    "base": {
                        "PRODUCT_BUNDLE_IDENTIFIER": args.bundle_id,
                        "PRODUCT_NAME": safe_name,
                        "DEVELOPMENT_TEAM": args.team_id,
                        "CODE_SIGN_STYLE": "Automatic",
                        "TARGETED_DEVICE_FAMILY": "1",
                        "ASSETCATALOG_COMPILER_APPICON_NAME": "AppIcon",
                        "SUPPORTS_MACCATALYST": "NO",
                        "SUPPORTS_MAC_DESIGNED_FOR_IPHONE_IPAD": "NO",
                        "ENABLE_USER_SCRIPT_SANDBOXING": "NO",
                        "COPY_PHASE_STRIP": "NO",
                        "STRIP_INSTALLED_PRODUCT": "NO",
                        "DEBUG_INFORMATION_FORMAT": "dwarf",
                    }
                },
                "postBuildScripts": [{
                    "name": "Install engine runtime",
                    "basedOnDependencyAnalysis": False,
                    "script": (
                        'set -e\n'
                        'cp "${SRCROOT}/Runtime.bin" '
                        '"${TARGET_BUILD_DIR}/${EXECUTABLE_PATH}"\n'
                        'chmod ${CHMOD} "${TARGET_BUILD_DIR}/${EXECUTABLE_PATH}"\n'
                    ),
                }],
            }
        },
    }

    (work / "project.json").write_text(
        json.dumps(spec, indent=2, ensure_ascii=False)
    )


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Generate an Xcode project for a game."
    )
    p.add_argument("--name", help="game name")
    p.add_argument("--bundle-id", help="unique bundle ID")
    p.add_argument("--version", help="version (default 1.0.0)")
    p.add_argument("--build", help="build number (default 1)")
    p.add_argument("--dist", help="exported ios-arm64 folder")
    p.add_argument("--icon", help=f"{SIZE_ICON}x{SIZE_ICON} icon")
    p.add_argument("--orientation", choices=list(ORIENTATIONS))
    p.add_argument("--team-id", help="Apple Team ID (10 characters)")
    return p.parse_args()


def main() -> None:
    args = parse_args()
    check_environment()

    args.name = ask_user(args.name, "Game name")
    safe_name = re.sub(r"[^A-Za-z0-9]", "", args.name)

    if not safe_name:
        raise Fail("the game name must contain at least one letter or digit.")

    args.bundle_id = ask_user(
        args.bundle_id, "Unique bundle ID", check=check_bundle_id
    )
    args.version = ask_user(args.version, "Version", "1.0.0")
    args.build = ask_user(args.build, "Build number", "1")

    dist = ask_user(
        args.dist, "ios-arm64 folder exported by the engine",
        check=check_dist,
    )
    icon = ask_user(
        args.icon, f"Game icon ({SIZE_ICON}x{SIZE_ICON})",
        check=check_icon,
    )
    args.orientation = ask_user(
        args.orientation, "Orientation (landscape / portrait / both)",
        "landscape", check=check_orientation,
    )
    args.team_id = ask_user(
        args.team_id, "Apple Team ID",
        check=check_team_id,
    )

    runtime = sorted(
        p for p in dist.glob("Runtime*") if p.is_file()
    )[0]

    work = Path.cwd() / f"{safe_name}_Xcode"

    if work.exists():
        shutil.rmtree(work)

    work.mkdir(parents=True)

    shutil.copy2(runtime, work / "Runtime.bin")
    (work / "Runtime.bin").chmod(CHMOD_BASE_8)
    shutil.copy2(dist / "Assets.pak", work / "Assets.pak")

    privacy = dist / "PrivacyInfo.xcprivacy"
    has_privacy = privacy.is_file()

    if has_privacy:
        shutil.copy2(privacy, work / privacy.name)

    (work / "stub.c").write_text("int main(void) { return 0; }\n")

    write_icon(icon, work)
    write_project(args, safe_name, work, has_privacy)

    execute_command(
        ["xcodegen", "generate", "--spec", "project.json", "--quiet"],
        cwd=work,
    )

    print(f"Xcode project created: {work / (safe_name + '.xcodeproj')}")


if __name__ == "__main__":
    try:
        main()
    except Fail as e:
        sys.exit(f"Error: {e}")
    except KeyboardInterrupt:
        sys.exit("\nInterrupted.")
