function(is_desktop_platform result)
    if (CMAKE_SYSTEM_NAME STREQUAL "Windows"
            OR CMAKE_SYSTEM_NAME STREQUAL "Linux"
            OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(${result} TRUE PARENT_SCOPE)
    else ()
        set(${result} FALSE PARENT_SCOPE)
    endif ()
endfunction()

function(is_dev_mode result)
    is_desktop_platform(is_desktop)
    if (is_desktop AND NOT GRNGAME_EMBED_ASSETS)
        set(${result} TRUE PARENT_SCOPE)
    else ()
        set(${result} FALSE PARENT_SCOPE)
    endif ()
endfunction()

is_desktop_platform(IS_DESKTOP)
is_dev_mode(IS_DEV)
