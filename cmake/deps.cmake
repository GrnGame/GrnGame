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
        NAME wren
        URL https://github.com/Baptistegrn/wren/archive/d18db2cffa5ba76e482d980c3f7bab143c077c2b.tar.gz
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