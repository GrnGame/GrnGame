set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(FETCHCONTENT_QUIET OFF)

if (EMSCRIPTEN)
    set(_grn_sdl_pthreads ${GRNGAME_WASM_THREADS})
else ()
    set(_grn_sdl_pthreads ON)
endif ()

CPMAddPackage(
        NAME SDL3
        URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.12/SDL3-3.4.12.tar.gz
        OPTIONS
        "SDL_SHARED OFF"
        "SDL_STATIC ON"
        "SDL_TEST_LIBRARY OFF"
        "SDL_TESTS OFF"
        "SDL_INSTALL OFF"
        "SDL_PTHREADS ${_grn_sdl_pthreads}"
)

CPMAddPackage(
        NAME SDL3_image
        URL https://github.com/libsdl-org/SDL_image/releases/download/release-3.2.4/SDL3_image-3.2.4.tar.gz
        OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "SDLIMAGE_VENDORED ON"
        "SDLIMAGE_DEPS_SHARED OFF"
        "SDLIMAGE_INSTALL OFF"
        "SDLIMAGE_SAMPLES OFF"
        "SDLIMAGE_STRICT ON"
        "SDLIMAGE_AVIF OFF"
        "SDLIMAGE_TIF OFF"
        "SDLIMAGE_WEBP OFF"
        "SDLIMAGE_JXL OFF"
)
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_link_libraries(SDL3_image-static PUBLIC
            "-framework ImageIO"
            "-framework MobileCoreServices"
    )
endif ()

CPMAddPackage(
        NAME SDL3_mixer
        URL https://github.com/libsdl-org/SDL_mixer/releases/download/release-3.2.4/SDL3_mixer-3.2.4.tar.gz
        OPTIONS
        "SDLMIXER_VENDORED OFF"
        "SDLMIXER_FLAC OFF"
        "SDLMIXER_OPUS OFF"
        "SDLMIXER_MOD OFF"
        "SDLMIXER_MIDI OFF"
        "SDLMIXER_GME OFF"
        "SDLMIXER_WAVPACK OFF"
)

CPMAddPackage(
        NAME freetype
        URL https://github.com/freetype/freetype/archive/refs/tags/VER-2-15-0.tar.gz
        OPTIONS
        "FT_DISABLE_ZLIB ON" "FT_DISABLE_BZIP2 ON" "FT_DISABLE_PNG ON"
        "FT_DISABLE_HARFBUZZ ON" "FT_DISABLE_BROTLI ON"
)

CPMAddPackage(
        NAME klib
        URL https://github.com/attractivechaos/klib/archive/97a0fcb790b43b9e5da8994f4671021fec036f19.tar.gz
        DOWNLOAD_ONLY YES
)
if (klib_ADDED AND NOT TARGET klib)
    add_library(klib INTERFACE)
    target_include_directories(klib INTERFACE "${klib_SOURCE_DIR}")
endif ()

CPMAddPackage(
        NAME cglm
        URL https://github.com/recp/cglm/archive/refs/tags/v0.9.7.tar.gz
        OPTIONS "CGLM_STATIC ON" "CGLM_SHARED OFF" "CGLM_USE_TEST OFF"
)

CPMAddPackage(
        NAME highway
        URL https://github.com/google/highway/archive/refs/tags/1.4.0.tar.gz
        OPTIONS
        "HWY_ENABLE_TESTS OFF" "HWY_ENABLE_EXAMPLES OFF"
        "HWY_ENABLE_CONTRIB OFF" "HWY_ENABLE_INSTALL OFF"
)

# build the amalgamation
CPMAddPackage(
        NAME sqlite3
        URL https://www.sqlite.org/2025/sqlite-amalgamation-3500400.zip
        DOWNLOAD_ONLY YES
)
if (sqlite3_ADDED AND NOT TARGET sqlite3)
    add_library(sqlite3 STATIC "${sqlite3_SOURCE_DIR}/sqlite3.c")
    target_include_directories(sqlite3 PUBLIC "${sqlite3_SOURCE_DIR}")
endif ()

CPMAddPackage(
        NAME cjson
        URL https://github.com/DaveGamble/cJSON/archive/6d9f2443ab071f86e5d9b43025a40929ec41c46c.tar.gz
        DOWNLOAD_ONLY YES
)
if (cjson_ADDED AND NOT TARGET cjson)
    add_library(cjson STATIC "${cjson_SOURCE_DIR}/cJSON.c")
    target_include_directories(cjson PUBLIC "${cjson_SOURCE_DIR}")
endif ()

CPMAddPackage(
        NAME yyjson
        URL https://github.com/ibireme/yyjson/archive/refs/tags/0.13.0.tar.gz
        DOWNLOAD_ONLY YES
)
if (yyjson_ADDED AND NOT TARGET yyjson)
    add_library(yyjson STATIC "${yyjson_SOURCE_DIR}/src/yyjson.c")
    target_include_directories(yyjson PUBLIC "${yyjson_SOURCE_DIR}/src")
endif ()

if (IS_DEV)
    CPMAddPackage(
            NAME tinydir
            URL https://github.com/cxong/tinydir/archive/refs/tags/1.2.6.tar.gz
            DOWNLOAD_ONLY YES
    )
    if (tinydir_ADDED AND NOT TARGET tinydir)
        add_library(tinydir INTERFACE)
        target_include_directories(tinydir INTERFACE "${tinydir_SOURCE_DIR}")
    endif ()


    CPMAddPackage(
            NAME dmon
            GITHUB_REPOSITORY septag/dmon
            GIT_TAG 98364a4d1964d603075e5e2657cfbc6c3496c9c1 # dmon have no release dont forget to update commit
            DOWNLOAD_ONLY YES
    )
    if (dmon_ADDED AND NOT TARGET dmon)
        add_library(dmon INTERFACE)
        target_include_directories(dmon INTERFACE "${dmon_SOURCE_DIR}")
        if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
            target_link_libraries(dmon INTERFACE pthread)
        elseif (APPLE)
            target_link_libraries(dmon INTERFACE "-framework CoreFoundation" "-framework CoreServices")
        endif ()
    endif ()
endif ()

if (IS_DESKTOP)
    CPMAddPackage(
            NAME haclog
            URL https://github.com/baptistegrn/haclog/archive/d801a48bc434a79813f0ee246a3c486dfe9a2ca1.tar.gz
            OPTIONS "BUILD_SHARED_LIBS OFF"
    )
endif ()


CPMAddPackage(
        NAME angelscript
        URL https://github.com/anjo76/angelscript/archive/a4a4eb80eb815d44bf0bca479913c735b8ee268a.tar.gz
        DOWNLOAD_ONLY YES
)
if (angelscript_ADDED AND NOT TARGET angelscript)
    set(_sdk "${angelscript_SOURCE_DIR}/sdk")
    file(GLOB _src "${_sdk}/angelscript/source/*.cpp")
    add_library(angelscript STATIC ${_src}
            "${_sdk}/add_on/scriptany/scriptany.cpp"
            "${_sdk}/add_on/scriptarray/scriptarray.cpp"
            "${_sdk}/add_on/scriptdictionary/scriptdictionary.cpp"
            "${_sdk}/add_on/scriptstdstring/scriptstdstring.cpp"
            "${_sdk}/add_on/scriptstdstring/scriptstdstring_utils.cpp"
            "${_sdk}/add_on/scriptbuilder/scriptbuilder.cpp"
    )

    target_include_directories(angelscript PUBLIC "${_sdk}/angelscript/include" "${_sdk}/add_on")
    set_target_properties(angelscript PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if (MSVC)
        target_compile_definitions(angelscript PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif ()

    set(_as_asm "")

    # Détection arm64 pour Apple (macOS Apple Silicon + iOS)
    set(_as_apple_arm64 OFF)
    if (APPLE)
        if (CMAKE_SYSTEM_NAME STREQUAL "iOS"
                OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64"
                OR CMAKE_OSX_ARCHITECTURES MATCHES "arm64"
                OR (NOT CMAKE_OSX_ARCHITECTURES AND CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64"))
            set(_as_apple_arm64 ON)
        endif ()
    endif ()

    set(_as_asm "")
    set(_as_asm_lang "")

    # Normalisation de l'archi (Windows renvoie "ARM64" en majuscules)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _as_proc)
    if (MSVC AND CMAKE_GENERATOR_PLATFORM)
        string(TOLOWER "${CMAKE_GENERATOR_PLATFORM}" _as_proc)   # x64 / arm64 / win32
    endif ()

    # Apple arm64 (macOS Apple Silicon + iOS)
    set(_as_apple_arm64 OFF)
    if (APPLE)
        if (CMAKE_SYSTEM_NAME STREQUAL "iOS"
                OR _as_proc MATCHES "^(arm64|aarch64)$"
                OR CMAKE_OSX_ARCHITECTURES MATCHES "arm64"
                OR (NOT CMAKE_OSX_ARCHITECTURES AND CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64"))
            set(_as_apple_arm64 ON)
        endif ()
    endif ()

    if (_as_apple_arm64)
        enable_language(ASM)
        set(_as_asm as_callfunc_arm64_xcode.S)
        set(_as_asm_lang ASM)
    elseif (EMSCRIPTEN)
        #no asm
    elseif (MSVC AND _as_proc MATCHES "^(amd64|x86_64|x64)$")
        enable_language(ASM_MASM)
        set(_as_asm as_callfunc_x64_msvc_asm.asm)
        set(_as_asm_lang ASM_MASM)
    elseif (MSVC AND _as_proc MATCHES "^(arm64|aarch64)$")
        enable_language(ASM_MARMASM)
        set(_as_asm as_callfunc_arm64_msvc.asm)
        set(_as_asm_lang ASM_MARMASM)
    elseif (NOT MSVC AND _as_proc MATCHES "^(aarch64|arm64)$")
        enable_language(ASM)
        set(_as_asm as_callfunc_arm64_gcc.S)
        set(_as_asm_lang ASM)
    elseif (NOT APPLE AND _as_proc MATCHES "^arm")
        enable_language(ASM)
        set(_as_asm as_callfunc_arm_gcc.S)
        set(_as_asm_lang ASM)
        set_source_files_properties("${_sdk}/angelscript/source/${_as_asm}" PROPERTIES COMPILE_FLAGS "-Wa,-mimplicit-it=always")
    endif ()

    if (_as_asm)
        set(_as_asm_path "${_sdk}/angelscript/source/${_as_asm}")
        set_source_files_properties("${_as_asm_path}" PROPERTIES LANGUAGE ${_as_asm_lang})
        target_sources(angelscript PRIVATE "${_as_asm_path}")
    endif ()
endif ()

set(DAS_MODULES_INCLUDE "" CACHE STRING "" FORCE)
CPMAddPackage(
        NAME daScript
        GITHUB_REPOSITORY BorisBat/daScript
        GIT_TAG v0.6.5
        #no tests
        EXCLUDE_FROM_ALL YES
        SYSTEM YES

        OPTIONS
        "CMAKE_BUILD_TYPE Release"
        "BUILD_SHARED_LIBS OFF"
        "BUILD_TESTING OFF"
        "DAS_ENABLE_PIC ON"

        #no tierces libs
        "DAS_SQLITE_DISABLED ON"
        "DAS_CRYPTO_DISABLED ON"
        "DAS_PUGIXML_DISABLED ON"
        "DAS_STBIMAGE_DISABLED ON"
        "DAS_MESHOPTIMIZER_DISABLED ON"

        "DAS_HV_DISABLED ON"
        "DAS_GLFW_DISABLED ON"
        "DAS_IMGUI_DISABLED ON"
        "DAS_VULKAN_DISABLED ON"
        "DAS_AUDIO_DISABLED ON"
        "DAS_STDDLG_DISABLED ON"
        "DAS_CLIPBOARD_DISABLED ON"

        "DAS_FLEX_BISON_DISABLED ON"
        "DAS_CLANG_BIND_DISABLED ON"
        "DAS_LLVM_DISABLED ON"
        "DAS_SMT_DISABLED ON"
        "DAS_CUDA_DISABLED ON"
        "DAS_METAL_DISABLED ON"
        "DAS_ACCELERATE_DISABLED ON"
        "DAS_TOOLS_DISABLED ON"
        "DAS_TREE_SITTER_DISABLED ON"
        "DAS_TERMINAL_DISABLED ON"
        "DAS_AOT_EXAMPLES_DISABLED ON"
        "DAS_TUTORIAL_DISABLED ON"
        "DAS_TESTS_DISABLED ON"

        "DAS_USE_STATIC_STD_LIBS ON"
)


# remove dascript warning
if (NOT MSVC)
    foreach (_t libDaScript libDaScript_runtime libUriParser)
        if (TARGET ${_t})
            target_compile_options(${_t} PRIVATE -w)
        endif ()
    endforeach ()
endif ()
