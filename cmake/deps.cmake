set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if (GRNGAME_TRACY)
    CPMAddPackage(
            NAME tracy
            GITHUB_REPOSITORY wolfpld/tracy
            GIT_TAG v0.11.1
            OPTIONS "TRACY_ENABLE ON" "TRACY_ON_DEMAND ON"
    )
endif ()


CPMAddPackage(
        NAME SDL3
        GITHUB_REPOSITORY libsdl-org/SDL
        GIT_TAG release-3.4.12
        OPTIONS
        "SDL_SHARED OFF"
        "SDL_STATIC ON"
        "SDL_TEST_LIBRARY OFF"
        "SDL_TESTS OFF"
        "SDL_INSTALL OFF"
)

CPMAddPackage(
        NAME SDL3_image
        GITHUB_REPOSITORY libsdl-org/SDL_image
        GIT_TAG release-3.2.4
        OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "SDLIMAGE_VENDORED ON"
        "SDLIMAGE_SAMPLES OFF"
        "SDLIMAGE_INSTALL OFF"
        "SDLIMAGE_AVIF OFF"
        "SDLIMAGE_JXL OFF"
        "SDLIMAGE_TIF OFF"
)

CPMAddPackage(
        NAME SDL3_mixer
        GITHUB_REPOSITORY libsdl-org/SDL_mixer
        GIT_TAG release-3.2.4
        OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "SDLMIXER_VENDORED ON"
        "SDLMIXER_SAMPLES OFF"
        "SDLMIXER_INSTALL OFF"
)

CPMAddPackage(
        NAME freetype
        GITHUB_REPOSITORY freetype/freetype
        GIT_TAG VER-2-15-0
        OPTIONS
        "FT_DISABLE_ZLIB ON" "FT_DISABLE_BZIP2 ON" "FT_DISABLE_PNG ON"
        "FT_DISABLE_HARFBUZZ ON" "FT_DISABLE_BROTLI ON"
)

CPMAddPackage(
        NAME klib
        GITHUB_REPOSITORY attractivechaos/klib
        GIT_TAG 97a0fcb790b43b9e5da8994f4671021fec036f19
        DOWNLOAD_ONLY YES
)
if (klib_ADDED AND NOT TARGET klib)
    add_library(klib INTERFACE)
    target_include_directories(klib INTERFACE "${klib_SOURCE_DIR}")
endif ()

CPMAddPackage(
        NAME cglm
        GITHUB_REPOSITORY recp/cglm
        GIT_TAG v0.9.7
        OPTIONS "CGLM_STATIC ON" "CGLM_SHARED OFF" "CGLM_USE_TEST OFF"
)

CPMAddPackage(
        NAME wren
        GITHUB_REPOSITORY Baptistegrn/wren   # <- replace with your fork if "Map-api" lives there
        GIT_TAG d18db2cffa5ba76e482d980c3f7bab143c077c2b
        DOWNLOAD_ONLY YES
)
if (wren_ADDED AND NOT TARGET wren)
    file(GLOB _wren_src
            "${wren_SOURCE_DIR}/src/vm/*.c"
            "${wren_SOURCE_DIR}/src/optional/*.c")
    add_library(wren STATIC ${_wren_src})
    target_include_directories(wren PUBLIC
            "${wren_SOURCE_DIR}/src/include"
            "${wren_SOURCE_DIR}/src/vm"
            "${wren_SOURCE_DIR}/src/optional")
endif ()

CPMAddPackage(
        NAME highway
        GITHUB_REPOSITORY google/highway
        GIT_TAG 1.4.0
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
        NAME yyjson
        GITHUB_REPOSITORY ibireme/yyjson
        GIT_TAG 0.13.0
        OPTIONS
            "YYJSON_INSTALL OFF"
)

if (IS_DEV)
    CPMAddPackage(
            NAME tinydir
            GITHUB_REPOSITORY cxong/tinydir
            GIT_TAG 1.2.6
            DOWNLOAD_ONLY YES
    )
    if (tinydir_ADDED AND NOT TARGET tinydir)
        add_library(tinydir INTERFACE)
        target_include_directories(tinydir INTERFACE "${tinydir_SOURCE_DIR}")
    endif ()
endif ()

if (IS_DESKTOP)
    CPMAddPackage(
            NAME haclog
            GITHUB_REPOSITORY baptistegrn/haclog
            GIT_TAG d801a48bc434a79813f0ee246a3c486dfe9a2ca1
            OPTIONS "BUILD_SHARED_LIBS OFF"
    )
endif ()
