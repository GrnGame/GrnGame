if (IS_DESKTOP)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
endif ()

# creates a test as an executable you can run, which is good for integrating with tooling
function(grn_add_test_target name)
    set(tgt "tests_${name}")
    if (TARGET ${tgt})
        return() # same test listed in both lists
    endif ()

    if (NOT IS_DESKTOP)
        return()
    endif ()

    set(target_name "tests/${name}") # what the python scripts expect
    set(build_dir   "${CMAKE_BINARY_DIR}/tests/${name}")

    # test exe has the same sources as runtime
    add_executable(${tgt} "${CMAKE_SOURCE_DIR}/runtime/main.c")
    target_link_libraries(${tgt} PRIVATE grngame)
    set_target_properties(${tgt} PROPERTIES
            OUTPUT_NAME "Runtime-${GRNGAME_TAG}"
            RUNTIME_OUTPUT_DIRECTORY "${build_dir}"
            VS_DEBUGGER_WORKING_DIRECTORY "${build_dir}")

    set(deps Runtime)
    if (TARGET Embedded)
        list(APPEND deps Embedded)
    endif ()

    if (NOT GRNGAME_EMBED_ASSETS)
        set(pipeline_cmd
                ${CMAKE_COMMAND} -E env "GRN_RUNTIME_DIR=$<TARGET_FILE_DIR:Runtime>"
                ${Python3_EXECUTABLE}
                "${CMAKE_SOURCE_DIR}/scripts/test/dev_pipeline.py"
                "${target_name}"
                "${build_dir}")
    else ()
        set(pipeline_cmd
                ${CMAKE_COMMAND} -E env "GRN_RUNTIME_DIR=$<TARGET_FILE_DIR:Runtime>"
                ${Python3_EXECUTABLE}
                "${CMAKE_SOURCE_DIR}/scripts/test/embedded_pipeline.py"
                "${target_name}"
                "${build_dir}"
                "${CMAKE_SOURCE_DIR}"
                "${CMAKE_BINARY_DIR}/Embedded")
    endif ()

    # phony target that runs the pipeline
    add_custom_target(${tgt}_prepare
            COMMAND ${pipeline_cmd}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            DEPENDS ${deps}
            COMMENT "Preparing ${target_name}"
            VERBATIM)
    add_dependencies(${tgt}_prepare ${deps})

    # building the test exe runs the pipeline first
    add_dependencies(${tgt} ${tgt}_prepare)

    # also available through ctest
    add_test(NAME ${tgt}
            COMMAND ${tgt}
            WORKING_DIRECTORY "${build_dir}")
endfunction()

function(grn_add_group_target group)
    set(tests ${ARGN})
    set(commands)
    set(exes)
    foreach (name IN LISTS tests)
        if (TARGET tests_${name})
            list(APPEND exes tests_${name})
            # sequential, in listed order
            list(APPEND commands
                    COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/tests/${name}"
                    "$<TARGET_FILE:tests_${name}>")
        endif ()
    endforeach ()

    if (commands)
        add_custom_target(${group} ${commands} USES_TERMINAL VERBATIM)
        add_dependencies(${group} ${exes})
    else ()
        add_custom_target(${group} COMMAND ${CMAKE_COMMAND} -E echo "No tests in ${group}")
    endif ()
endfunction()

function(grn_setup_tests)
    cmake_parse_arguments(ARG "" "" "TARGET_TESTS;AUTO_TESTS" ${ARGN})

    enable_testing()

    foreach (name IN LISTS ARG_TARGET_TESTS ARG_AUTO_TESTS)
        grn_add_test_target(${name})
    endforeach ()

    grn_add_group_target(tests      ${ARG_TARGET_TESTS})
    grn_add_group_target(auto_tests ${ARG_AUTO_TESTS})
endfunction()