import json
import sys

ALLOWED_DEPENDENCIES = {
    # Windows
    "KERNEL32.dll",
    "USER32.dll",
    "ADVAPI32.dll",
    "SHELL32.dll",
    "ole32.dll",
    "OLEAUT32.dll",
    "GDI32.dll",
    "IMM32.dll",
    "WINMM.dll",
    "VERSION.dll",
    "SETUPAPI.dll",

    # Linux x86_64
    "libc.so.6",
    "libm.so.6",
    "libgcc_s.so.1",
    "libstdc++.so.6",
    "ld-linux-x86-64.so.2",

    # Linux ARM64
    "ld-linux-aarch64.so.1",

    # macOS
    "/usr/lib/libz.1.dylib",
    "/usr/lib/libiconv.2.dylib",
    "/usr/lib/libSystem.B.dylib",
    "/usr/lib/libc++.1.dylib",
    "/usr/lib/libobjc.A.dylib",

    "/System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics",
    "/System/Library/Frameworks/ImageIO.framework/Versions/A/ImageIO",
    "/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices",
    "/System/Library/Frameworks/CoreHaptics.framework/Versions/A/CoreHaptics",
    "/System/Library/Frameworks/CoreMedia.framework/Versions/A/CoreMedia",
    "/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo",
    "/System/Library/Frameworks/GameController.framework/Versions/A/GameController",
    "/System/Library/Frameworks/Metal.framework/Versions/A/Metal",
    "/System/Library/Frameworks/QuartzCore.framework/Versions/A/QuartzCore",
    "/System/Library/Frameworks/UniformTypeIdentifiers.framework/Versions/A/UniformTypeIdentifiers",
    "/System/Library/Frameworks/Cocoa.framework/Versions/A/Cocoa",
    "/System/Library/Frameworks/Carbon.framework/Versions/A/Carbon",
    "/System/Library/Frameworks/ForceFeedback.framework/Versions/A/ForceFeedback",
    "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
    "/System/Library/Frameworks/AudioToolbox.framework/Versions/A/AudioToolbox",
    "/System/Library/Frameworks/CoreAudio.framework/Versions/A/CoreAudio",
    "/System/Library/Frameworks/AudioUnit.framework/Versions/A/AudioUnit",
    "/System/Library/Frameworks/AVFoundation.framework/Versions/A/AVFoundation",
    "/System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation",
    "/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation",
    "/System/Library/Frameworks/AppKit.framework/Versions/C/AppKit",
}


def get_all_dependencies(file_path: str) -> list:
    with open(file_path, "r", encoding="utf-8") as file:
        data = json.load(file)

    dependencies = []
    def walk(deps):
        for dep in deps:
            dependencies.append(dep)
            walk(dep["deps"])

    walk(data["deps"])

    return dependencies


if __name__ == "__main__":
    file = sys.argv[1]
    dependencies = get_all_dependencies(file)
    for dependency in dependencies:
        if dependency["name"] not in ALLOWED_DEPENDENCIES:
            print(f"Dependecie not allowed :{dependency["name"]}")
            sys.exit(1)
    sys.exit(0)
