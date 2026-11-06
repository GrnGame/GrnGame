cmake_minimum_required(VERSION 3.22)
project(GrnGame C CXX)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(CMAKE_MACOSX_BUNDLE OFF)
endif ()

if (EMSCRIPTEN)
    add_compile_definitions(
            ACCESSPERMS=0777
            _EMSCRIPTEN_VER
    )
endif ()
#enable ctests
enable_testing()

set(GRNGAME_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/build")

# embed_assets defaults to ON everywhere except desktop (forced on for wasm/android/iOS)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(_grn_embed_default OFF)
else ()
    set(_grn_embed_default ON)
endif ()

option(GRNGAME_EMBED_ASSETS "Generate embedded assets (forced on for wasm/android/iOS)" ${_grn_embed_default})
option(GRNGAME_SOFTWARE_RENDERER "Use software renderer" OFF)
option(GRNGAME_PIXEL_INT_POSITION "Use integer pixel positions" ON)
option(GRNGAME_WASM_THREADS "Enable threads for wasm" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON) # lto on release

# set msvc runtime to static
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # when many cl work together use this
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/FS>)
endif ()

# wasm thread flags must also reach the dependencies, so set them before deps.cmake
if (EMSCRIPTEN AND GRNGAME_WASM_THREADS)
    add_compile_options(-pthread -matomics -mbulk-memory)
    add_link_options(-pthread)
endif ()

# cache CPM packages
include(cmake/CPM.cmake)
include(cmake/platform.cmake) # defines IS_DESKTOP / IS_DEV
include(cmake/deps.cmake)


string(TOLOWER "${CMAKE_SYSTEM_NAME}" GRNGAME_PLAT)
if (GRNGAME_PLAT STREQUAL "darwin")
    set(GRNGAME_PLAT "macosx")
elseif (GRNGAME_PLAT STREQUAL "emscripten")
    set(GRNGAME_PLAT "wasm")
elseif (GRNGAME_PLAT STREQUAL "ios")
    set(GRNGAME_PLAT "iphoneos")
endif ()

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" GRNGAME_ARCH)
if (GRNGAME_ARCH MATCHES "^(amd64|x86_64)$")
    if (GRNGAME_PLAT STREQUAL "windows")
        set(GRNGAME_ARCH "x64")
    else ()
        set(GRNGAME_ARCH "x86_64")
    endif ()
endif ()
if (GRNGAME_ARCH MATCHES "^(aarch64|arm64)$")
    set(GRNGAME_ARCH "arm64")
endif ()

set(GRNGAME_SUFFIX "")
if (GRNGAME_EMBED_ASSETS)
    set(GRNGAME_SUFFIX "embedded")
endif ()
set(GRNGAME_TAG "${GRNGAME_PLAT}-${GRNGAME_ARCH}-$<LOWER_CASE:$<CONFIG>>${GRNGAME_SUFFIX}")

if (GRNGAME_EMBED_ASSETS)
    set(GRNGAME_KIND "runtime")
else ()
    set(GRNGAME_KIND "desktop")
endif ()

set(_bin_plat "${GRNGAME_PLAT}")
set(_bin_arch "${GRNGAME_ARCH}")
if (_bin_plat STREQUAL "macosx")
    set(_bin_plat "macos")
elseif (_bin_plat STREQUAL "iphoneos")
    set(_bin_plat "ios")
endif ()
if (_bin_arch STREQUAL "x86_64")
    set(_bin_arch "x64")
endif ()

if (EMSCRIPTEN)
    set(GRNGAME_BIN_NAME "${GRNGAME_KIND}-wasm")
else ()
    set(GRNGAME_BIN_NAME "${GRNGAME_KIND}-${_bin_plat}-${_bin_arch}")
endif ()

# global flags
# todo trim down
if (MSVC)
    add_compile_options(/W4)
else ()
    add_compile_options(-Wall -Wextra)
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-frtti> $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>)
endif ()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|x64)$")
    add_compile_definitions(GRNGAME_X64)
endif ()

if (IS_DEV)
    add_compile_definitions(GRNGAME_HOT_RELOAD_ENABLE)
endif ()

# grngame library
#dont use recursive include because it's don't work on clion
set(GRNGAME_SOURCES
        grngame/assets/asset_manager.c
        grngame/assets/asset_manager.h
        grngame/assets/embedded_file_manager.c
        grngame/assets/embedded_file_manager.h
        grngame/assets/load.c
        grngame/assets/load.h
        grngame/audio/filter.c
        grngame/audio/filter.h
        grngame/audio/sound.c
        grngame/audio/sound.h
        grngame/audio/sound_info.c
        grngame/audio/sound_info.h
        grngame/audio/sound_manager.c
        grngame/audio/sound_manager.h
        grngame/bindings/angelscript/api/callback.cpp
        grngame/bindings/angelscript/api/callback.h
        grngame/bindings/angelscript/api/interpretor.cpp
        grngame/bindings/angelscript/api/interpretor.h
        grngame/bindings/angelscript/api/module.cpp
        grngame/bindings/angelscript/api/module.h
        grngame/core/app.c
        grngame/core/app.h
        grngame/core/data.c
        grngame/core/data.h
        grngame/core/init.c
        grngame/core/init.h
        grngame/core/message.c
        grngame/core/message.h
        grngame/core/text.c
        grngame/core/text.h
        grngame/core/thread.c
        grngame/core/thread.h
        grngame/core/window.c
        grngame/core/window.h
        grngame/data/file.c
        grngame/data/file.h
        grngame/data/json.c
        grngame/data/json.h
        grngame/data/json_read.c
        grngame/data/json_read.h
        grngame/data/sqlite.c
        grngame/data/sqlite.h
        grngame/dev/hotreload.c
        grngame/dev/hotreload.h
        grngame/dev/logging.c
        grngame/dev/logging.h
        grngame/dev/tracy.h
        grngame/embedded/embedded_file_generator.c
        grngame/embedded/embedded_file_generator.h
        grngame/embedded/embedded_file_generator_main.c
        grngame/input/controller.c
        grngame/input/controller.h
        grngame/input/input_data.h
        grngame/input/input_manager.c
        grngame/input/input_manager.h
        grngame/input/input_text.c
        grngame/input/input_text.h
        grngame/input/keyboard.c
        grngame/input/keyboard.h
        grngame/input/mouse.c
        grngame/input/mouse.h
        grngame/input/poll_events.c
        grngame/input/poll_events.h
        grngame/math/math.h
        grngame/math/types.h
        grngame/memory/bump_allocator.c
        grngame/memory/bump_allocator.h
        grngame/platform/check_type.h
        grngame/platform/check_types.c
        grngame/platform/directories.c
        grngame/platform/directories.h
        grngame/platform/paths.c
        grngame/platform/paths.h
        grngame/platform/virtual_memory.c
        grngame/platform/virtual_memory.h
        grngame/renderer/cielab.c
        grngame/renderer/cielab.h
        grngame/renderer/palette.c
        grngame/renderer/palette.h
        grngame/renderer/particule.c
        grngame/renderer/particule.h
        grngame/renderer/primitive.c
        grngame/renderer/primitive.h
        grngame/renderer/renderer.c
        grngame/renderer/renderer.h
        grngame/renderer/sprite.c
        grngame/renderer/sprite.h
        grngame/renderer/texture.c
        grngame/renderer/texture.h
        grngame/utils/attributes.h
        grngame/utils/c_cpp.h
        grngame/utils/clear.c
        grngame/utils/clear.h
        grngame/utils/macro.h
        grngame/utils/memory.h
        grngame/utils/random.c
        grngame/utils/random.h
        grngame/utils/string_compat.h
        grngame/utils/taskbar_icon.c
        grngame/utils/taskbar_icon.h
        grngame/utils/time.c
        grngame/utils/time.h
        grngame/utils/version.c
        grngame/utils/version.h
        grngame/web/web.h
        grngame/bindings/api/callback.h
        grngame/bindings/api/interpretor.h
        grngame/bindings/api/callback.cpp
        grngame/bindings/api/interpretor.cpp
        grngame/bindings/dascript/api/interpretor.h

)

# wasm doesn't like a double main declaration (segfault)
list(REMOVE_ITEM GRNGAME_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/grngame/embedded/embedded_file_generator_main.c")
add_library(grngame STATIC ${GRNGAME_SOURCES})
target_include_directories(grngame PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_definitions(grngame PRIVATE WITH_SDL3_STATIC)
set_target_properties(grngame PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${GRNGAME_OUTPUT_DIR}")

# libraries
target_link_libraries(grngame PUBLIC
        SDL3::SDL3-static
        SDL3_image::SDL3_image-static
        SDL3_mixer::SDL3_mixer-static
        klib
        cglm
        freetype
        sqlite3
        hwy
        cjson
        yyjson
        angelscript
        libDaScript
)

target_include_directories(grngame PUBLIC
        "${daScript_SOURCE_DIR}/include"
)

if (IS_DESKTOP)
    target_link_libraries(grngame PUBLIC haclog)
    target_compile_definitions(grngame PUBLIC GRNGAME_DESKTOP)
endif ()

if (IS_DEV)
    target_link_libraries(grngame PRIVATE tinydir dmon)
    target_compile_definitions(grngame PUBLIC GRNGAME_DEV_MODE)
endif ()

if (GRNGAME_SOFTWARE_RENDERER)
    target_compile_definitions(grngame PUBLIC GRNGAME_SOFTWARE)
endif ()

if (GRNGAME_PIXEL_INT_POSITION)
    target_compile_definitions(grngame PUBLIC PIXEL_INT_ONLY)
endif ()

if (GRNGAME_EMBED_ASSETS)
    target_compile_definitions(grngame PUBLIC GRNGAME_EMBED_ASSETS)
endif ()

target_compile_definitions(grngame PUBLIC CGLM_USE_ANONYMOUS_STRUCT=1)

target_compile_definitions(grngame PUBLIC
        $<$<CONFIG:Debug>:GRNGAME_DEBUG>
        $<$<CONFIG:Release>:GRNGAME_RELEASE>
)

# platform specifics
if (EMSCRIPTEN)
    target_compile_definitions(grngame PUBLIC GRNGAME_WASM)
    target_link_options(grngame PUBLIC
            -sFORCE_FILESYSTEM=1
            -sASYNCIFY
            -sINITIAL_MEMORY=512MB
            -sALLOW_MEMORY_GROWTH=1
    )
    if (GRNGAME_WASM_THREADS)
        target_compile_options(grngame PUBLIC -pthread -matomics -mbulk-memory)
        target_link_options(grngame PUBLIC
                "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency"
                -pthread
        )
    else ()
        target_compile_definitions(grngame PUBLIC NO_THREADS_WASM)
    endif ()

elseif (ANDROID)
    set_target_properties(grngame PROPERTIES POSITION_INDEPENDENT_CODE ON) # -fPIC
    target_compile_definitions(grngame PUBLIC GRNGAME_ANDROID)
    target_link_options(grngame PUBLIC
            -Wl,--export-dynamic
            -rdynamic
            -Wl,-u,JNI_OnLoad
            -Wl,-u,SDL_main
    )
    target_link_libraries(grngame PUBLIC log)

elseif (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_definitions(grngame PUBLIC GRNGAME_IOS)

elseif (APPLE)
    target_compile_definitions(grngame PUBLIC GRNGAME_MACOS)

elseif (WIN32)
    target_compile_definitions(grngame PUBLIC GRNGAME_WINDOWS)

elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(grngame PUBLIC GRNGAME_LINUX _GNU_SOURCE)
endif ()

# Build embed generator for the host in dev mode
if (IS_DEV AND NOT CMAKE_CROSSCOMPILING)
    add_executable(Embedded "${CMAKE_CURRENT_SOURCE_DIR}/grngame/embedded/embedded_file_generator_main.c")
    set_target_properties(Embedded PROPERTIES
            OUTPUT_NAME "Embedded-${GRNGAME_TAG}"
            RUNTIME_OUTPUT_DIRECTORY "${GRNGAME_OUTPUT_DIR}/embedded/$<LOWER_CASE:$<CONFIG>>")
    foreach (config IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${config}" config_upper)
        string(TOLOWER "${config}" config_lower)
        set_target_properties(Embedded PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${GRNGAME_OUTPUT_DIR}/embedded/${config_lower}")
    endforeach ()
    target_link_libraries(Embedded PRIVATE grngame)
endif ()

# the runtime
if (ANDROID)
    add_library(Runtime SHARED runtime/main.c)
    set_target_properties(Runtime PROPERTIES
            OUTPUT_NAME "GrnGame"
            LIBRARY_OUTPUT_DIRECTORY "${GRNGAME_OUTPUT_DIR}/runtime/$<LOWER_CASE:$<CONFIG>>")
else ()
    add_executable(Runtime runtime/main.c)
    set_target_properties(Runtime PROPERTIES
            OUTPUT_NAME "${GRNGAME_BIN_NAME}"
            RUNTIME_OUTPUT_DIRECTORY "${GRNGAME_OUTPUT_DIR}/runtime/$<LOWER_CASE:$<CONFIG>>" MACOSX_BUNDLE OFF)
    foreach (config IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${config}" config_upper)
        string(TOLOWER "${config}" config_lower)
        set_target_properties(Runtime PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${GRNGAME_OUTPUT_DIR}/runtime/${config_lower}")
    endforeach ()
endif ()
target_link_libraries(Runtime PRIVATE grngame)

# tests
include(cmake/tests.cmake)
grn_setup_tests(
        TARGET_TESTS engine_work
        AUTO_TESTS
)
