cmake_minimum_required(VERSION 3.22)
project(GrnGame C CXX)

# embed_assets defaults to ON everywhere except desktop (forced on for wasm/android/iOS)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(_grn_embed_default OFF)
else ()
    set(_grn_embed_default ON)
endif ()

option(GRNGAME_TRACY "Enable tracy profiler instrumentation" OFF)
option(GRNGAME_EMBED_ASSETS "Generate embedded assets (forced on for wasm/android/iOS)" ${_grn_embed_default})
option(GRNGAME_SOFTWARE_RENDERER "Use software renderer" OFF)
option(GRNGAME_PIXEL_INT_POSITION "Use integer pixel positions" ON)
option(GRNGAME_BIG_BUFFER "Use bigger buffers" ON)
option(GRNGAME_WASM_THREADS "Enable threads for wasm" OFF)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON) # lto on release

# set msvc runtime to static
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

# wasm thread flags must also reach the dependencies, so set them before deps.cmake
if (EMSCRIPTEN AND GRNGAME_WASM_THREADS)
    add_compile_options(-pthread -matomics -mbulk-memory)
    add_link_options(-pthread)
endif ()

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
file(GLOB_RECURSE GRNGAME_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/grngame/*.c")
# wasm doesn't like a double main declaration (segfault)
list(REMOVE_ITEM GRNGAME_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/grngame/embedded/embedded_file_generator_main.c")
add_library(grngame STATIC ${GRNGAME_SOURCES})
target_include_directories(grngame PUBLIC ${CMAKE_SOURCE_DIR})
target_compile_definitions(grngame PRIVATE WITH_SDL3_STATIC)

# libraries
target_link_libraries(grngame PUBLIC
        SDL3::SDL3-static
        SDL3_image::SDL3_image-static
        SDL3_mixer::SDL3_mixer-static
        klib
        cglm
        wren
        freetype
        sqlite3
        hwy
        yyjson
)

if (IS_DESKTOP)
    target_link_libraries(grngame PUBLIC haclog)
    target_compile_definitions(grngame PUBLIC GRNGAME_DESKTOP)
endif ()

if (IS_DEV)
    target_link_libraries(grngame PRIVATE tinydir)
    target_compile_definitions(grngame PUBLIC GRNGAME_DEV_MODE)
endif ()

# handle features
if (GRNGAME_TRACY)
    target_link_libraries(grngame PUBLIC Tracy::TracyClient)
    target_compile_definitions(grngame PUBLIC TRACY_ENABLE)
endif ()

if (GRNGAME_SOFTWARE_RENDERER)
    target_compile_definitions(grngame PUBLIC GRNGAME_SOFTWARE)
endif ()

if (GRNGAME_PIXEL_INT_POSITION)
    target_compile_definitions(grngame PUBLIC PIXEL_INT_ONLY)
endif ()

if (GRNGAME_BIG_BUFFER)
    target_compile_definitions(grngame PUBLIC BUFFER_BIG_MEMORY_SPACE)
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
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Embedded")
    target_link_libraries(Embedded PRIVATE grngame)
endif ()

# the runtime
if (ANDROID)
    add_library(Runtime SHARED runtime/main.c)
    set_target_properties(Runtime PROPERTIES
            OUTPUT_NAME "GrnGame"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Runtime")
else ()
    add_executable(Runtime runtime/main.c)
    set_target_properties(Runtime PROPERTIES
            OUTPUT_NAME "Runtime-${GRNGAME_TAG}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Runtime")
    # uncomment to produce an html page instead of bare js
    # if (EMSCRIPTEN)
    #     set_target_properties(Runtime PROPERTIES SUFFIX ".html")
    # endif ()
endif ()
target_link_libraries(Runtime PRIVATE grngame)

# cross platform symbols
include(CheckSymbolExists)

# string compat
check_symbol_exists(strdup        "string.h"  HAVE_STRDUP)
check_symbol_exists(strcasecmp    "strings.h" HAVE_STRCASECMP)
check_symbol_exists(_stricmp      "string.h"  HAVE__STRICMP)
check_symbol_exists(_strdup       "string.h"  HAVE__STRDUP)
check_symbol_exists(strtok_r      "string.h"  HAVE_STRTOK_R)

target_compile_definitions(grngame PRIVATE
        $<$<BOOL:${HAVE_STRDUP}>:HAVE_STRDUP>
        $<$<BOOL:${HAVE_STRCASECMP}>:HAVE_STRCASECMP>
        $<$<BOOL:${HAVE__STRICMP}>:HAVE__STRICMP>
        $<$<BOOL:${HAVE__STRDUP}>:HAVE__STRDUP>
        $<$<BOOL:${HAVE_STRTOK_R}>:HAVE_STRTOK_R>
)